#include "MaintainController.h"

#include "Helpers.h"
#include "Helpers.h"
#include "ProcessRunner.h"
#include "SettingsController.h"

#include <QRegularExpression>
#include <QStandardPaths>

using namespace cachy::helpers;

MaintainController::MaintainController(SettingsController *settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
{}

int MaintainController::keepOld() const
{
    return m_settings ? m_settings->cacheKeepVersions() : 3;
}

QString MaintainController::which(const QString &program)
{
    return cachy::helpers::which(program);
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

    const QString running = cachy::helpers::runningKernel();
    incInflight(1);
    runQuiet(QStringLiteral("pacman"), {QStringLiteral("-Qsq"), QStringLiteral("linux")},
             [this, running](int, const QString &out) {
                 m_oldKernels.clear();
                 const QList<QStringView> lines =
                     QStringView(out).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
                 for (const QStringView &l : lines) {
                     const QString name = l.trimmed().toString();
                     if (!name.startsWith(QLatin1String("linux")))
                         continue;
                     if (!running.contains(name, Qt::CaseInsensitive))
                         m_oldKernels << name;
                 }
                 emit changed();
                 incInflight(-1);
             });

    if (!which(QStringLiteral("flatpak")).isEmpty()) {
        incInflight(1);
        runQuiet(QStringLiteral("flatpak"), {QStringLiteral("uninstall"), QStringLiteral("--unused"), QStringLiteral("--dry-run")},
                 [this](int, const QString &out) {
                     m_flatpakUnused = 0;
                     const QList<QStringView> lines =
                         QStringView(out).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
                     for (const QStringView &l : lines) {
                         if (!l.trimmed().isEmpty())
                             ++m_flatpakUnused;
                     }
                     emit changed();
                     incInflight(-1);
                 });
    }

    const QString aur = cachy::helpers::aurHelper();
    if (!aur.isEmpty()) {
        incInflight(1);
        runQuiet(aur, {QStringLiteral("-Sc"), QStringLiteral("--dry-run")},
                 [this](int, const QString &out) {
                     m_aurCacheCount = 0;
                     const QList<QStringView> lines =
                         QStringView(out).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
                     for (const QStringView &l : lines) {
                         if (l.contains(QLatin1String("Packages to keep")))
                             continue;
                         if (!l.trimmed().isEmpty())
                             ++m_aurCacheCount;
                     }
                     emit changed();
                     incInflight(-1);
                 });
    }

    if (which(QStringLiteral("paccache")).isEmpty()) {
        m_warnings << QStringLiteral("paccache not found (install pacman-contrib).");
    } else {
        static const QRegularExpression candRe(
            QStringLiteral(":\\s*(\\d+)\\s+candidate"),
            QRegularExpression::CaseInsensitiveOption);

        incInflight(1);
        runQuiet(QStringLiteral("paccache"),
                 {QStringLiteral("-dk"), QString::number(keepOld())},
                 [this](int, const QString &out) {
                     const QRegularExpressionMatch m = candRe.match(out);
                     m_cacheOld = m.hasMatch() ? m.captured(1).toInt() : 0;
                     emit changed();
                     incInflight(-1);
                 });

        incInflight(1);
        runQuiet(QStringLiteral("paccache"),
                 {QStringLiteral("-duk"), QString::number(keepUninstalled())},
                 [this](int, const QString &out) {
                     const QRegularExpressionMatch m = candRe.match(out);
                     m_cacheUninstalled = m.hasMatch() ? m.captured(1).toInt() : 0;
                     updateDiskSummary();
                     emit changed();
                     incInflight(-1);
                 });
    }
}

void MaintainController::updateDiskSummary()
{
    const int total = cacheTotal() + m_flatpakUnused + m_aurCacheCount + m_oldKernels.size();
    if (total == 0)
        m_diskSummary = QStringLiteral("Nothing to reclaim right now.");
    else
        m_diskSummary = QStringLiteral("%1 cleanup target(s) across cache, kernels, Flatpak, and AUR.")
                            .arg(total);
}

void MaintainController::removeOrphans()
{
    if (m_busy)
        return;
    if (m_orphans.isEmpty()) {
        emitLine(QStringLiteral("No orphan packages to remove."), QStringLiteral("warn"));
        return;
    }
    if (which(QStringLiteral("pkexec")).isEmpty()) {
        emitLine(QStringLiteral("pkexec required."), QStringLiteral("error"));
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
    if (which(QStringLiteral("pkexec")).isEmpty() || which(QStringLiteral("paccache")).isEmpty()) {
        emitLine(QStringLiteral("pkexec/paccache required."), QStringLiteral("error"));
        return;
    }
    setBusy(true);
    runStreaming(QStringLiteral("pkexec"),
                 {QStringLiteral("paccache"), QStringLiteral("-rk"),
                  QString::number(keepOld())},
                 [this](int code) {
                     if (code != 0) {
                         setBusy(false);
                         scan();
                         return;
                     }
                     runStreaming(QStringLiteral("pkexec"),
                                  {QStringLiteral("paccache"), QStringLiteral("-ruk"),
                                   QString::number(keepUninstalled())},
                                  [this](int code2) {
                                      if (code2 == 0)
                                          emitLine(QStringLiteral("Cache cleaned."),
                                                   QStringLiteral("ok"));
                                      setBusy(false);
                                      scan();
                                  });
                 });
}

void MaintainController::removeOldKernels()
{
    if (m_busy || m_oldKernels.isEmpty())
        return;
    setBusy(true);
    QStringList args{QStringLiteral("pacman"), QStringLiteral("-Rns"),
                     QStringLiteral("--noconfirm"), QStringLiteral("--")};
    args += m_oldKernels;
    runStreaming(QStringLiteral("pkexec"), args, [this](int code) {
        if (code == 0)
            emitLine(QStringLiteral("Old kernels removed."), QStringLiteral("ok"));
        setBusy(false);
        scan();
    });
}

void MaintainController::cleanFlatpakUnused()
{
    if (m_busy || which(QStringLiteral("flatpak")).isEmpty())
        return;
    setBusy(true);
    runStreaming(QStringLiteral("flatpak"),
                 {QStringLiteral("uninstall"), QStringLiteral("--unused"), QStringLiteral("-y")},
                 [this](int code) {
                     if (code == 0)
                         emitLine(QStringLiteral("Unused Flatpak runtimes removed."),
                                  QStringLiteral("ok"));
                     setBusy(false);
                     scan();
                 });
}

void MaintainController::cleanAurCache()
{
    if (m_busy)
        return;
    const QString aur = cachy::helpers::aurHelper();
    if (aur.isEmpty())
        return;
    setBusy(true);
    runStreaming(aur, {QStringLiteral("-Sc"), QStringLiteral("--noconfirm")},
                 [this](int code) {
                     if (code == 0)
                         emitLine(QStringLiteral("AUR cache cleaned."), QStringLiteral("ok"));
                     setBusy(false);
                     scan();
                 });
}
