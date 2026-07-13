#include "UpdateController.h"

#include "Classifier.h"
#include "ProcessRunner.h"

#include <QDateTime>
#include <QDir>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>

using namespace cachy;

UpdateController::UpdateController(QObject *parent) : QObject(parent)
{
    m_model = new UpdatesModel(this);
    m_listProxy = new PkgFilterProxy(false, this);
    m_listProxy->setSourceModel(m_model);
    m_kernelProxy = new PkgFilterProxy(true, this);
    m_kernelProxy->setSourceModel(m_model);

    connect(m_model, &UpdatesModel::selectionChanged, this,
            &UpdateController::selectionChanged);
}

QObject *UpdateController::updatesModelObject() const { return m_listProxy; }
QObject *UpdateController::kernelModelObject() const { return m_kernelProxy; }

QString UpdateController::which(const QString &program)
{
    return QStandardPaths::findExecutable(program);
}

int UpdateController::packageCount() const { return m_model->items().size(); }

int UpdateController::sourceCount() const
{
    QSet<QString> sources;
    for (const Pkg &p : m_model->items())
        sources.insert(sourceKey(p.source));
    return sources.size();
}

QString UpdateController::downloadText() const
{
    qint64 total = 0;
    for (const Pkg &p : m_model->items())
        total += p.sizeBytes;
    return formatBytes(total);
}

QString UpdateController::sourceCommandFor(const QString &source) const
{
    if (source == QLatin1String("aur"))
        return cachy::sourceCommand(Source::Aur);
    if (source == QLatin1String("flatpak"))
        return cachy::sourceCommand(Source::Flatpak);
    return cachy::sourceCommand(Source::Repo);
}

int UpdateController::sourceCountFor(const QString &source) const
{
    int n = 0;
    for (const Pkg &p : m_model->items())
        if (!p.kernel && sourceKey(p.source) == source)
            ++n;
    return n;
}

QString UpdateController::sourceSizeTextFor(const QString &source) const
{
    qint64 total = 0;
    for (const Pkg &p : m_model->items())
        if (!p.kernel && sourceKey(p.source) == source)
            total += p.sizeBytes;
    return formatBytes(total);
}

void UpdateController::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit busyChanged();
}

void UpdateController::setStatus(const QString &text, const QString &state)
{
    m_statusText = text;
    m_statusState = state;
    emit statusChanged();
}

void UpdateController::setStage(const QString &stage, qreal progress)
{
    m_stage = stage;
    m_progress = progress;
    emit stageChanged();
}

void UpdateController::emitLine(const QString &text, const QString &kind)
{
    emit lineEmitted(text, kind);
}

void UpdateController::runStep(const QString &program, const QStringList &args,
                              const QProcessEnvironment &env,
                              std::function<void(int, const QString &)> onDone)
{
    auto *r = new ProcessRunner(this);
    r->setMerged(false);
    r->setTimeout(120000);
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
    r->start(program, args, env);
}

// ---------------------------------------------------------------------------
// Check pipeline
// ---------------------------------------------------------------------------

void UpdateController::check()
{
    if (m_busy)
        return;

    setBusy(true);
    setStatus(QStringLiteral("Checking for updates\u2026"), QStringLiteral("checking"));
    m_collect.clear();
    m_warnings.clear();

    emitLine(QStringLiteral("Checking for updates (repo / AUR / Flatpak)\u2026"),
             QStringLiteral("cmd"));

    // Writable fake DB so checkupdates does not need root.
    const QString cache =
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir().mkpath(cache + QStringLiteral("/checkupdates-db"));
    m_checkDbPath = cache + QStringLiteral("/checkupdates-db");

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("CHECKUPDATES_DB"), m_checkDbPath);

    if (which(QStringLiteral("checkupdates")).isEmpty()) {
        m_warnings << QStringLiteral(
            "checkupdates not found (install pacman-contrib) \u2014 repo updates skipped");
        checkAur();
        return;
    }

    emitLine(QStringLiteral("$ checkupdates"), QStringLiteral("cmd"));
    runStep(QStringLiteral("checkupdates"), {}, env,
            [this](int code, const QString &out) {
                if (code == 0)
                    parsePacmanStyle(out, Source::Repo);
                // exit code 2 == no updates; other codes already warned.
                checkAur();
            });
}

void UpdateController::checkAur()
{
    if (which(QStringLiteral("paru")).isEmpty()) {
        checkFlatpak();
        return;
    }
    emitLine(QStringLiteral("$ paru -Qua"), QStringLiteral("cmd"));
    runStep(QStringLiteral("paru"), {QStringLiteral("--color"),
                                     QStringLiteral("never"), QStringLiteral("-Qua")},
            QProcessEnvironment(), [this](int, const QString &out) {
                parsePacmanStyle(out, Source::Aur);
                checkFlatpak();
            });
}

void UpdateController::checkFlatpak()
{
    if (which(QStringLiteral("flatpak")).isEmpty()) {
        enrich();
        return;
    }
    emitLine(QStringLiteral("$ flatpak remote-ls --updates"), QStringLiteral("cmd"));
    runStep(QStringLiteral("flatpak"),
            {QStringLiteral("remote-ls"), QStringLiteral("--updates"),
             QStringLiteral("--columns=application,version")},
            QProcessEnvironment(), [this](int, const QString &out) {
                parseFlatpak(out);
                enrich();
            });
}

void UpdateController::parsePacmanStyle(const QString &out, Source source)
{
    static const QRegularExpression re(
        QStringLiteral("^(\\S+)\\s+(\\S+)\\s+->\\s+(\\S+)\\s*$"));
    const QList<QStringView> lines = QStringView(out).split(QLatin1Char('\n'));
    for (const QStringView &lv : lines) {
        const QRegularExpressionMatch m = re.match(lv.toString());
        if (!m.hasMatch())
            continue;
        Pkg p;
        p.name = m.captured(1);
        p.oldVersion = m.captured(2);
        p.newVersion = m.captured(3);
        p.source = source;
        p.selected = true;
        m_collect.push_back(p);
    }
}

void UpdateController::parseFlatpak(const QString &out)
{
    const QList<QStringView> lines = QStringView(out).split(QLatin1Char('\n'));
    for (const QStringView &lv : lines) {
        const QString line = lv.trimmed().toString();
        if (line.isEmpty())
            continue;
        const QStringList cols = line.split(QLatin1Char('\t'), Qt::SkipEmptyParts);
        if (cols.isEmpty())
            continue;
        Pkg p;
        p.flatpakId = cols.at(0).trimmed();
        p.name = p.flatpakId.section(QLatin1Char('.'), -1);
        p.newVersion = cols.value(1).trimmed();
        p.source = Source::Flatpak;
        p.selected = true;
        m_collect.push_back(p);
    }
}

void UpdateController::enrich()
{
    QStringList repoNames;
    for (const Pkg &p : m_collect)
        if (p.source == Source::Repo)
            repoNames << p.name;

    if (repoNames.isEmpty() || which(QStringLiteral("expac")).isEmpty()) {
        finalizeCheck();
        return;
    }

    QStringList args;
    args << QStringLiteral("-S")
         << QStringLiteral("%n\t%r\t%k\t%m\t%G\t%d") << QStringLiteral("--");
    args += repoNames;

    runStep(QStringLiteral("expac"), args, QProcessEnvironment(),
            [this](int, const QString &out) {
                QHash<QString, QStringList> byName;
                const QList<QStringView> lines =
                    QStringView(out).split(QLatin1Char('\n'));
                for (const QStringView &lv : lines) {
                    const QString line = lv.toString();
                    if (line.trimmed().isEmpty())
                        continue;
                    const QStringList f = line.split(QLatin1Char('\t'));
                    if (f.size() < 2)
                        continue;
                    if (!byName.contains(f.at(0)))
                        byName.insert(f.at(0), f);
                }
                for (Pkg &p : m_collect) {
                    if (p.source != Source::Repo)
                        continue;
                    const QStringList f = byName.value(p.name);
                    if (f.isEmpty())
                        continue;
                    p.repo = f.value(1).trimmed();
                    p.sizeBytes = f.value(2).trimmed().toLongLong();
                    const QString groups = f.value(4).trimmed();
                    if (!groups.isEmpty())
                        p.groups = groups.split(QLatin1Char(' '), Qt::SkipEmptyParts);
                    p.description = f.value(5).trimmed();
                }
                finalizeCheck();
            });
}

void UpdateController::finalizeCheck()
{
    for (Pkg &p : m_collect) {
        p.kernel = classifier::isKernel(p.name, p.source);
        p.severity = classifier::classify(p);
        p.summary = classifier::buildSummary(p);
    }

    m_model->setItems(m_collect);
    m_lastChecked = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm"));

    const int n = m_model->items().size();
    if (n == 0) {
        setStatus(QStringLiteral("System is up to date."), QStringLiteral("uptodate"));
        emitLine(QStringLiteral("No updates available."), QStringLiteral("ok"));
    } else {
        setStatus(QStringLiteral("%1 update%2 available.")
                      .arg(n)
                      .arg(n == 1 ? QString() : QStringLiteral("s")),
                  QStringLiteral("ready"));
        emitLine(QStringLiteral("%1 update(s) found across %2 source(s).")
                     .arg(n)
                     .arg(sourceCount()),
                 QStringLiteral("ok"));
    }

    setBusy(false);
    emit updatesChanged();
    emit selectionChanged();
    emit checkFinished();
}

// ---------------------------------------------------------------------------
// Apply pipeline
// ---------------------------------------------------------------------------

QString UpdateController::plannedCommands() const
{
    QStringList cmds;

    QStringList deselectedRepo;
    int repoSel = 0, aurSel = 0, flatSel = 0, repoAll = 0, aurAll = 0, flatAll = 0;
    QStringList aurNames, flatIds;
    for (const Pkg &p : m_model->items()) {
        switch (p.source) {
        case Source::Repo:
            ++repoAll;
            if (p.selected) ++repoSel; else deselectedRepo << p.name;
            break;
        case Source::Aur:
            ++aurAll;
            if (p.selected) { ++aurSel; aurNames << p.name; }
            break;
        case Source::Flatpak:
            ++flatAll;
            if (p.selected) { ++flatSel; flatIds << (p.flatpakId.isEmpty() ? p.name : p.flatpakId); }
            break;
        }
    }

    if (repoSel > 0) {
        QString c = QStringLiteral("pkexec pacman -Syu --noconfirm");
        if (!deselectedRepo.isEmpty())
            c += QStringLiteral(" --ignore ") + deselectedRepo.join(QLatin1Char(','));
        cmds << c;
    }
    if (aurSel > 0) {
        cmds << (aurSel == aurAll
                     ? QStringLiteral("paru -Sua --noconfirm")
                     : QStringLiteral("paru -S --noconfirm ") + aurNames.join(QLatin1Char(' ')));
    }
    if (flatSel > 0) {
        cmds << (flatSel == flatAll
                     ? QStringLiteral("flatpak update -y")
                     : QStringLiteral("flatpak update -y ") + flatIds.join(QLatin1Char(' ')));
    }
    return cmds.join(QLatin1Char('\n'));
}

void UpdateController::apply() { beginRun(Mode::Apply); }
void UpdateController::downloadOnly() { beginRun(Mode::DownloadOnly); }

void UpdateController::dryRun()
{
    const QString plan = plannedCommands();
    emitLine(QStringLiteral("\u2014 Dry run (no changes will be made) \u2014"),
             QStringLiteral("cmd"));
    if (plan.isEmpty()) {
        emitLine(QStringLiteral("Nothing selected."), QStringLiteral("warn"));
        return;
    }
    const QList<QStringView> lines = QStringView(plan).split(QLatin1Char('\n'));
    for (const QStringView &l : lines)
        emitLine(QStringLiteral("$ ") + l.toString(), QStringLiteral("cmd"));
    emitLine(QStringLiteral("These commands would run. Nothing was executed."),
             QStringLiteral("ok"));
}

void UpdateController::beginRun(Mode mode)
{
    if (m_busy || m_running)
        return;
    if (m_model->selectedCount() == 0) {
        emitLine(QStringLiteral("Nothing selected."), QStringLiteral("warn"));
        return;
    }

    m_mode = mode;
    m_running = true;
    setBusy(true);
    m_warnings.clear();

    m_runQueue.clear();
    QSet<QString> present;
    for (const Pkg &p : m_model->items()) {
        if (!p.selected)
            continue;
        if (mode == Mode::DownloadOnly && p.source != Source::Repo)
            continue; // only repo supports a clean download-only step
        const QString k = sourceKey(p.source);
        if (!present.contains(k)) {
            present.insert(k);
            m_runQueue << p.source;
        }
    }
    m_runIndex = 0;

    const QString verb = mode == Mode::DownloadOnly
                             ? QStringLiteral("Downloading updates")
                             : QStringLiteral("Applying updates");
    setStatus(verb + QStringLiteral("\u2026"), QStringLiteral("applying"));
    setStage(QStringLiteral("Sync"), 0.05);
    runNextGroup();
}

void UpdateController::runNextGroup()
{
    if (m_runIndex >= m_runQueue.size()) {
        finishRun(true);
        return;
    }

    const Source src = m_runQueue.at(m_runIndex);

    QStringList deselectedRepo, aurNames, flatIds;
    int repoAll = 0, aurAll = 0, flatAll = 0, aurSel = 0, flatSel = 0;
    for (const Pkg &p : m_model->items()) {
        if (p.source == Source::Repo) {
            ++repoAll;
            if (!p.selected)
                deselectedRepo << p.name;
        } else if (p.source == Source::Aur) {
            ++aurAll;
            if (p.selected) { ++aurSel; aurNames << p.name; }
        } else {
            ++flatAll;
            if (p.selected) { ++flatSel; flatIds << (p.flatpakId.isEmpty() ? p.name : p.flatpakId); }
        }
    }

    QString program;
    QStringList args;

    if (src == Source::Repo) {
        program = QStringLiteral("pkexec");
        args << QStringLiteral("pacman")
             << (m_mode == Mode::DownloadOnly ? QStringLiteral("-Syuw")
                                              : QStringLiteral("-Syu"))
             << QStringLiteral("--noconfirm");
        if (!deselectedRepo.isEmpty()) {
            args << QStringLiteral("--ignore")
                 << deselectedRepo.join(QLatin1Char(','));
            emitLine(QStringLiteral(
                         "Partial selection: %1 repo package(s) held back via --ignore.")
                         .arg(deselectedRepo.size()),
                     QStringLiteral("warn"));
        }
        setStage(QStringLiteral("Sync"), 0.15);
    } else if (src == Source::Aur) {
        program = QStringLiteral("paru");
        if (aurSel == aurAll)
            args << QStringLiteral("-Sua") << QStringLiteral("--noconfirm");
        else
            args << QStringLiteral("-S") << QStringLiteral("--noconfirm") << aurNames;
        setStage(QStringLiteral("AUR build"), 0.55);
    } else {
        program = QStringLiteral("flatpak");
        args << QStringLiteral("update") << QStringLiteral("-y");
        if (flatSel != flatAll)
            args += flatIds;
        setStage(QStringLiteral("Flatpak"), 0.8);
    }

    emitLine(QStringLiteral("$ %1 %2").arg(program, args.join(QLatin1Char(' '))),
             QStringLiteral("cmd"));

    auto *r = new ProcessRunner(this);
    r->setMerged(true);
    r->setTimeout(0); // updates can take a long time; no timeout
    connect(r, &ProcessRunner::line, this, [this](const QString &l) {
        emitLine(l, classifyLineKind(l));
        // Coarse stage machine driven by pacman output.
        const QString low = l.toLower();
        if (low.contains(QLatin1String("synchronizing package")))
            setStage(QStringLiteral("Sync"), 0.2);
        else if (low.contains(QLatin1String("downloading")) ||
                 low.contains(QLatin1String("retrieving packages")))
            setStage(QStringLiteral("Download"), 0.4);
        else if (low.contains(QLatin1String("checking")) ||
                 low.contains(QLatin1String("processing package changes")) ||
                 low.startsWith(QLatin1String("installing")) ||
                 low.startsWith(QLatin1String("upgrading")))
            setStage(QStringLiteral("Install"), 0.6);
        else if (low.contains(QLatin1String("running post-transaction hooks")))
            setStage(QStringLiteral("Hooks"), 0.75);
    });
    connect(r, &ProcessRunner::finished, this,
            [this, r](int code, const QString &) {
                r->deleteLater();
                if (code != 0) {
                    m_warnings << QStringLiteral("A step exited with code %1.").arg(code);
                    emitLine(QStringLiteral("Step failed with exit code %1.").arg(code),
                             QStringLiteral("error"));
                    finishRun(false);
                    return;
                }
                ++m_runIndex;
                runNextGroup();
            });
    connect(r, &ProcessRunner::failed, this, [this, r](const QString &err) {
        r->deleteLater();
        m_warnings << err;
        emitLine(err, QStringLiteral("error"));
        finishRun(false);
    });
    r->start(program, args);
}

void UpdateController::finishRun(bool ok)
{
    m_running = false;
    setBusy(false);
    setStage(ok ? QStringLiteral("Summary") : QStringLiteral("Failed"),
             ok ? 1.0 : m_progress);

    if (ok) {
        emitLine(QStringLiteral("Done."), QStringLiteral("ok"));
        setStatus(m_mode == Mode::DownloadOnly
                      ? QStringLiteral("Download complete.")
                      : QStringLiteral("Update complete. Re-checking\u2026"),
                  QStringLiteral("done"));
        emit applyFinished(true);
        if (m_mode != Mode::DownloadOnly)
            check(); // refresh the list after a successful apply
    } else {
        setStatus(QStringLiteral("Update failed \u2014 see terminal for details."),
                  QStringLiteral("error"));
        emit applyFinished(false);
    }
}

void UpdateController::cancel()
{
    // Cooperative: the running ProcessRunner children are terminated.
    for (ProcessRunner *r : findChildren<ProcessRunner *>())
        r->stop();
    if (m_running) {
        m_running = false;
        setBusy(false);
        setStatus(QStringLiteral("Cancelled."), QStringLiteral("idle"));
    }
}

QString UpdateController::classifyLineKind(const QString &line) const
{
    const QString low = line.toLower();
    if (low.contains(QLatin1String("error")) || low.contains(QLatin1String("failed")) ||
        low.startsWith(QLatin1String("==> error")))
        return QStringLiteral("error");
    if (low.contains(QLatin1String("warning")) || low.startsWith(QLatin1String("==> warning")))
        return QStringLiteral("warn");
    if (line.startsWith(QLatin1String("::")) || line.startsWith(QLatin1String("==>")))
        return QStringLiteral("cmd");
    return QStringLiteral("out");
}
