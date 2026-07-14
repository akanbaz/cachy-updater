#include "MaintainController.h"

#include "Classifier.h"
#include "Helpers.h"
#include "ProcessRunner.h"
#include "SettingsController.h"

#include <QDir>
#include <QFutureWatcher>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QtConcurrent>
#include <memory>

namespace {

QString aurCacheRoot(const QString &helper)
{
    return QDir::homePath() + QStringLiteral("/.cache/") + helper;
}

int countAurCacheEntries(const QString &helper)
{
    const QDir clone(aurCacheRoot(helper) + QStringLiteral("/clone"));
    if (clone.exists())
        return clone.entryList(QDir::Dirs | QDir::NoDotAndDotDot).size();

    if (helper == QLatin1String("yay")) {
        const QDir yayRoot(aurCacheRoot(helper));
        if (yayRoot.exists())
            return yayRoot.entryList(QDir::Dirs | QDir::NoDotAndDotDot).size();
    }
    return 0;
}

bool removeAurCacheEntries(const QString &helper)
{
    bool ok = true;
    const QString root = aurCacheRoot(helper);
    const QStringList subdirs =
        helper == QLatin1String("paru")
            ? QStringList{QStringLiteral("clone"), QStringLiteral("diff")}
            : QStringList{QString()};

    if (helper == QLatin1String("yay")) {
        const QDir yayRoot(root);
        if (!yayRoot.exists())
            return true;
        for (const QString &name :
             yayRoot.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            if (!QDir(yayRoot.absoluteFilePath(name)).removeRecursively())
                ok = false;
        }
        return ok;
    }

    for (const QString &sub : subdirs) {
        const QDir dir(root + QLatin1Char('/') + sub);
        if (!dir.exists())
            continue;
        for (const QString &name : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            if (!QDir(dir.absoluteFilePath(name)).removeRecursively())
                ok = false;
        }
    }
    return ok;
}

int parseFlatpakUnusedCount(const QString &out)
{
    const QString text = out.trimmed();
    if (text.isEmpty()
        || text.contains(QStringLiteral("Nothing unused to uninstall"), Qt::CaseInsensitive))
        return 0;

    // flatpak prints CLI errors (e.g. unknown --dry-run) as a single line — not a candidate.
    if (text.startsWith(QStringLiteral("error:"), Qt::CaseInsensitive))
        return 0;

    static const QRegularExpression rowRe(
        QStringLiteral("^\\s*\\d+\\.\\s+\\S+"), QRegularExpression::MultilineOption);
    const int rows = text.count(rowRe);
    return rows > 0 ? rows : 0;
}

} // namespace

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
    // Reset counters so stale values don't linger if an optional tool went away
    // between scans; each async probe below repopulates what it can.
    m_flatpakUnused = 0;
    m_aurCacheCount = 0;
    m_cacheOld = 0;
    m_cacheUninstalled = 0;

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

    const QString runningPkg = cachy::helpers::runningKernelPkgbase();
    incInflight(1);
    runQuiet(QStringLiteral("pacman"), {QStringLiteral("-Qsq"), QStringLiteral("linux")},
             [this, runningPkg](int, const QString &out) {
                 m_oldKernels.clear();
                 const QList<QStringView> lines =
                     QStringView(out).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
                 for (const QStringView &l : lines) {
                     const QString name = l.trimmed().toString();
                     if (!cachy::classifier::isBootableKernelPackage(name))
                         continue;
                     // Never offer to remove the currently running kernel package.
                     if (!runningPkg.isEmpty()
                         && name.compare(runningPkg, Qt::CaseInsensitive) == 0)
                         continue;
                     // Fallback when pkgbase is missing: keep all if we cannot tell.
                     if (runningPkg.isEmpty())
                         continue;
                     m_oldKernels << name;
                 }
                 emit changed();
                 incInflight(-1);
             });

    if (!which(QStringLiteral("flatpak")).isEmpty()) {
        incInflight(1);
        // No --dry-run support; probe without -y so nothing is removed during scan.
        runQuiet(QStringLiteral("flatpak"),
                 {QStringLiteral("uninstall"), QStringLiteral("--unused"),
                  QStringLiteral("--noninteractive")},
                 [this](int, const QString &out) {
                     m_flatpakUnused = parseFlatpakUnusedCount(out);
                     updateDiskSummary();
                     emit changed();
                     incInflight(-1);
                 });
    }

    const QString aur = cachy::helpers::aurHelper();
    if (!aur.isEmpty()) {
        m_aurCacheCount = countAurCacheEntries(aur);
        updateDiskSummary();
        emit changed();
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

    // Ensure the disk summary is populated even when no optional tool
    // (flatpak / paccache / AUR helper) triggered an async update.
    updateDiskSummary();
    emit changed();
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
    if (cachy::helpers::pacmanDbLocked()) {
        emitLine(QStringLiteral("Pacman database is locked — try again later."),
                 QStringLiteral("error"));
        return;
    }
    if (which(QStringLiteral("pkexec")).isEmpty()) {
        emitLine(QStringLiteral("pkexec required."), QStringLiteral("error"));
        return;
    }
    setBusy(true);
    QStringList args{QStringLiteral("/usr/bin/pacman"), QStringLiteral("-Rns"),
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
                         emitLine(QStringLiteral("Cache cleanup failed (exit code %1).")
                                      .arg(code),
                                  QStringLiteral("error"));
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
    if (cachy::helpers::pacmanDbLocked()) {
        emitLine(QStringLiteral("Pacman database is locked — try again later."),
                 QStringLiteral("error"));
        return;
    }
    setBusy(true);
    QStringList args{QStringLiteral("/usr/bin/pacman"), QStringLiteral("-Rns"),
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
    auto captured = std::make_shared<QString>();
    emitLine(QStringLiteral("$ flatpak uninstall --unused -y"), QStringLiteral("cmd"));
    auto *r = new ProcessRunner(this);
    r->setMerged(true);
    r->setTimeout(0);
    connect(r, &ProcessRunner::line, this, [this, captured](const QString &l) {
        *captured += l + QLatin1Char('\n');
        const QString low = l.toLower();
        QString kind = QStringLiteral("out");
        if (low.contains(QLatin1String("error")) || low.contains(QLatin1String("failed")))
            kind = QStringLiteral("error");
        else if (low.contains(QLatin1String("warning")))
            kind = QStringLiteral("warn");
        emitLine(l, kind);
    });
    connect(r, &ProcessRunner::finished, this, [this, r, captured](int code, const QString &out) {
        const QString text = out.isEmpty() ? *captured : out;
        if (code == 0
            && !text.contains(QStringLiteral("Nothing unused to uninstall"),
                              Qt::CaseInsensitive))
            emitLine(QStringLiteral("Unused Flatpak runtimes removed."), QStringLiteral("ok"));
        else if (code == 0)
            emitLine(QStringLiteral("No unused Flatpak runtimes to remove."),
                     QStringLiteral("ok"));
        r->deleteLater();
        setBusy(false);
        scan();
    });
    connect(r, &ProcessRunner::failed, this, [this, r](const QString &err) {
        emitLine(err, QStringLiteral("error"));
        m_warnings << err;
        r->deleteLater();
        setBusy(false);
    });
    r->start(QStringLiteral("flatpak"),
             {QStringLiteral("uninstall"), QStringLiteral("--unused"), QStringLiteral("-y")});
}

void MaintainController::cleanAurCache()
{
    if (m_busy)
        return;
    const QString aur = cachy::helpers::aurHelper();
    if (aur.isEmpty())
        return;
    setBusy(true);
    emitLine(QStringLiteral("$ cleaning %1 build cache in ~/.cache/%1")
                 .arg(aur),
             QStringLiteral("cmd"));

    // Removal can touch thousands of files; run it off the UI thread so the
    // window doesn't freeze on a large cache.
    auto *watcher = new QFutureWatcher<bool>(this);
    connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher, aur]() {
        const bool ok = watcher->result();
        watcher->deleteLater();
        m_aurCacheCount = countAurCacheEntries(aur);
        updateDiskSummary();
        if (ok && m_aurCacheCount == 0)
            emitLine(QStringLiteral("AUR cache cleaned."), QStringLiteral("ok"));
        else if (!ok)
            emitLine(QStringLiteral("Some AUR cache entries could not be removed."),
                     QStringLiteral("warn"));
        else
            emitLine(QStringLiteral("AUR cache partially cleaned (%1 entries remain).")
                         .arg(m_aurCacheCount),
                     QStringLiteral("warn"));
        emit changed();
        setBusy(false);
    });
    watcher->setFuture(QtConcurrent::run([aur]() { return removeAurCacheEntries(aur); }));
}

void MaintainController::cancel()
{
    ProcessRunner::stopAll(this);
    if (m_busy) {
        setBusy(false);
        emitLine(QStringLiteral("Cancelled."), QStringLiteral("warn"));
    }
}
