#include "MaintainController.h"

#include "ProcessRunner.h"

#include <QRegularExpression>
#include <QStandardPaths>

MaintainController::MaintainController(QObject *parent) : QObject(parent) {}

QString MaintainController::which(const QString &program)
{
    return QStandardPaths::findExecutable(program);
}

void MaintainController::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit busyChanged();
}

void MaintainController::incInflight(int delta)
{
    const bool was = m_inflight > 0;
    m_inflight += delta;
    if ((m_inflight > 0) != was)
        emit scanningChanged();
}

void MaintainController::emitLine(const QString &text, const QString &kind)
{
    emit lineEmitted(text, kind);
}

void MaintainController::runQuiet(const QString &program, const QStringList &args,
                                 std::function<void(int, const QString &)> onDone)
{
    auto *r = new ProcessRunner(this);
    r->setMerged(true);
    r->setTimeout(60000);
    connect(r, &ProcessRunner::finished, this,
            [r, onDone](int code, const QString &out) {
                onDone(code, out);
                r->deleteLater();
            });
    connect(r, &ProcessRunner::failed, this,
            [this, r, onDone](const QString &err) {
                m_warnings << err;
                onDone(-1, QString());
                r->deleteLater();
            });
    r->start(program, args);
}

void MaintainController::runStreaming(const QString &program,
                                     const QStringList &args,
                                     std::function<void(int)> onDone)
{
    emitLine(QStringLiteral("$ %1 %2").arg(program, args.join(QLatin1Char(' '))),
             QStringLiteral("cmd"));
    auto *r = new ProcessRunner(this);
    r->setMerged(true);
    r->setTimeout(0);
    connect(r, &ProcessRunner::line, this, [this](const QString &l) {
        const QString low = l.toLower();
        QString kind = QStringLiteral("out");
        if (low.contains(QLatin1String("error")) || low.contains(QLatin1String("failed")))
            kind = QStringLiteral("error");
        else if (low.contains(QLatin1String("warning")))
            kind = QStringLiteral("warn");
        emitLine(l, kind);
    });
    connect(r, &ProcessRunner::finished, this, [r, onDone](int code, const QString &) {
        onDone(code);
        r->deleteLater();
    });
    connect(r, &ProcessRunner::failed, this, [this, r, onDone](const QString &err) {
        emitLine(err, QStringLiteral("error"));
        m_warnings << err;
        onDone(-1);
        r->deleteLater();
    });
    r->start(program, args);
}

void MaintainController::scan()
{
    m_warnings.clear();

    // Orphans: pacman -Qtdq (exit 1 == none).
    incInflight(1);
    runQuiet(QStringLiteral("pacman"), {QStringLiteral("-Qtdq")},
             [this](int code, const QString &out) {
                 m_orphans.clear();
                 if (code == 0) {
                     const QList<QStringView> lines =
                         QStringView(out).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
                     for (const QStringView &l : lines) {
                         const QString s = l.trimmed().toString();
                         if (!s.isEmpty())
                             m_orphans << s;
                     }
                 } else if (code != 1 && code >= 0) {
                     m_warnings << QStringLiteral("pacman -Qtdq failed.");
                 }
                 emit changed();
                 incInflight(-1);
             });

    if (which(QStringLiteral("paccache")).isEmpty()) {
        m_warnings << QStringLiteral("paccache not found (install pacman-contrib).");
        emit changed();
        return;
    }

    static const QRegularExpression candRe(
        QStringLiteral(":\\s*(\\d+)\\s+candidate"),
        QRegularExpression::CaseInsensitiveOption);

    incInflight(1);
    runQuiet(QStringLiteral("paccache"),
             {QStringLiteral("-dk"), QString::number(kKeepOld)},
             [this](int, const QString &out) {
                 const QRegularExpressionMatch m = candRe.match(out);
                 m_cacheOld = m.hasMatch() ? m.captured(1).toInt() : 0;
                 emit changed();
                 incInflight(-1);
             });

    incInflight(1);
    runQuiet(QStringLiteral("paccache"),
             {QStringLiteral("-duk"), QString::number(kKeepUninstalled)},
             [this](int, const QString &out) {
                 const QRegularExpressionMatch m = candRe.match(out);
                 m_cacheUninstalled = m.hasMatch() ? m.captured(1).toInt() : 0;
                 emit changed();
                 incInflight(-1);
             });
}

void MaintainController::removeOrphans()
{
    if (m_busy)
        return;
    if (m_orphans.isEmpty()) {
        emitLine(QStringLiteral("No orphan packages to remove."), QStringLiteral("warn"));
        return;
    }
    if (which(QStringLiteral("pkexec")).isEmpty() ||
        which(QStringLiteral("pacman")).isEmpty()) {
        emitLine(QStringLiteral("pkexec/pacman required to remove orphans."),
                 QStringLiteral("error"));
        return;
    }

    setBusy(true);
    QStringList args{QStringLiteral("pacman"), QStringLiteral("-Rns"),
                     QStringLiteral("--noconfirm"), QStringLiteral("--")};
    args += m_orphans;
    runStreaming(QStringLiteral("pkexec"), args, [this](int code) {
        if (code == 0)
            emitLine(QStringLiteral("Orphans removed."), QStringLiteral("ok"));
        setBusy(false);
        scan();
    });
}

void MaintainController::cleanCache()
{
    if (m_busy)
        return;
    if (which(QStringLiteral("pkexec")).isEmpty() ||
        which(QStringLiteral("paccache")).isEmpty()) {
        emitLine(QStringLiteral("pkexec/paccache required to clean the cache."),
                 QStringLiteral("error"));
        return;
    }

    setBusy(true);
    runStreaming(QStringLiteral("pkexec"),
                 {QStringLiteral("paccache"), QStringLiteral("-rk"),
                  QString::number(kKeepOld)},
                 [this](int code) {
                     if (code != 0) {
                         setBusy(false);
                         scan();
                         return;
                     }
                     runStreaming(QStringLiteral("pkexec"),
                                  {QStringLiteral("paccache"), QStringLiteral("-ruk"),
                                   QString::number(kKeepUninstalled)},
                                  [this](int code2) {
                                      if (code2 == 0)
                                          emitLine(QStringLiteral("Cache cleaned."),
                                                   QStringLiteral("ok"));
                                      setBusy(false);
                                      scan();
                                  });
                 });
}
