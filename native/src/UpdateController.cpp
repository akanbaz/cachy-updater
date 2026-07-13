#include "UpdateController.h"

#include "Classifier.h"
#include "Helpers.h"
#include "HistoryController.h"
#include "ProcessRunner.h"
#include "SettingsController.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QRegularExpression>
#include <QSet>
#include <QStandardPaths>
#include <QUrl>
#include <QUrlQuery>

using namespace cachy;

UpdateController::UpdateController(SettingsController *settings,
                                   HistoryController *history, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
    , m_history(history)
{
    m_model = new UpdatesModel(this);
    m_listProxy = new UpdateListProxy(this);
    m_listProxy->setSourceModel(m_model);
    m_kernelProxy = new PkgFilterProxy(true, this);
    m_kernelProxy->setSourceModel(m_model);

    connect(m_model, &UpdatesModel::selectionChanged, this,
            &UpdateController::selectionChanged);
    refreshKernelInfo();
}

void UpdateController::setNewsGateChecker(std::function<bool(QString *)> checker)
{
    m_archGateChecker = std::move(checker);
}

QObject *UpdateController::updatesModelObject() const { return m_listProxy; }
QObject *UpdateController::kernelModelObject() const { return m_kernelProxy; }

QString UpdateController::which(const QString &program)
{
    return helpers::which(program);
}

bool UpdateController::snapshotsAvailable() const { return helpers::snapshotsAvailable(); }

QString UpdateController::searchText() const { return m_listProxy->searchText(); }
int UpdateController::minSeverity() const { return m_listProxy->minSeverity(); }
QString UpdateController::sourceFilter() const { return m_listProxy->sourceFilter(); }

void UpdateController::setSearchText(const QString &text)
{
    m_listProxy->setSearchText(text);
    emit filtersChanged();
}

void UpdateController::setMinSeverity(int severity)
{
    m_listProxy->setMinSeverity(severity);
    emit filtersChanged();
}

void UpdateController::setSourceFilter(const QString &source)
{
    m_listProxy->setSourceFilter(source);
    emit filtersChanged();
}

void UpdateController::clearFilters()
{
    m_listProxy->setSearchText({});
    m_listProxy->setMinSeverity(0);
    m_listProxy->setSourceFilter({});
    emit filtersChanged();
}

QString UpdateController::aurProgram() const
{
    if (m_settings && !m_settings->enableAur())
        return {};
    return helpers::aurHelper();
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

int UpdateController::criticalCount() const
{
    int n = 0;
    for (const Pkg &p : m_model->items())
        if (static_cast<int>(p.severity) >= static_cast<int>(Severity::Important))
            ++n;
    return n;
}

QString UpdateController::sourceCommandFor(const QString &source) const
{
    if (source == QLatin1String("aur")) {
        const QString helper = aurProgram();
        return helper.isEmpty() ? sourceCommand(Source::Aur)
                              : helper + QStringLiteral(" -Sua");
    }
    if (source == QLatin1String("flatpak"))
        return sourceCommand(Source::Flatpak);
    return sourceCommand(Source::Repo);
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

QStringList UpdateController::pacmanBandwidthArgs() const
{
    QStringList args;
    if (m_settings && m_settings->maxDownloads() > 0)
        args << QStringLiteral("--max-downloads")
             << QString::number(m_settings->maxDownloads());
    return args;
}

QProcessEnvironment UpdateController::aurEnv() const
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (m_settings && m_settings->paruConcurrency() > 0) {
        env.insert(QStringLiteral("PARU_MAX_CONCURRENT"),
                   QString::number(m_settings->paruConcurrency()));
    }
    return env;
}

void UpdateController::refreshKernelInfo()
{
    m_runningKernel = helpers::runningKernel();
    m_installedKernels.clear();
    runStep(QStringLiteral("pacman"), {QStringLiteral("-Qsq"), QStringLiteral("linux")},
            QProcessEnvironment(), [this](int, const QString &out) {
                const QList<QStringView> lines =
                    QStringView(out).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
                for (const QStringView &l : lines) {
                    const QString name = l.trimmed().toString();
                    if (classifier::isBootableKernelPackage(name))
                        m_installedKernels << name;
                }
                updateKernelCopy();
                emit kernelInfoChanged();
            });
}

void UpdateController::updateKernelCopy()
{
    if (m_runningKernel.isEmpty()) {
        m_kernelHeadline.clear();
        m_kernelDetail.clear();
        return;
    }

    m_kernelHeadline =
        QStringLiteral("You're running kernel version %1").arg(m_runningKernel);

    if (m_installedKernels.isEmpty()) {
        m_kernelDetail.clear();
        return;
    }

    if (m_installedKernels.size() == 1) {
        m_kernelDetail =
            QStringLiteral("Boot image installed: %1").arg(m_installedKernels.first());
        return;
    }

    m_kernelDetail =
        QStringLiteral("Boot images on disk: %1")
            .arg(m_installedKernels.join(QStringLiteral(", ")));
}

void UpdateController::holdPackage(const QString &name)
{
    if (m_settings)
        m_settings->addHold(name);
    applyHolds();
}

void UpdateController::unholdPackage(const QString &name)
{
    if (m_settings)
        m_settings->removeHold(name);
    applyHolds();
}

void UpdateController::applyHolds()
{
    if (!m_settings)
        return;
    QVector<Pkg> items = m_model->items();
    if (items.isEmpty())
        items = m_collect;
    bool changed = false;
    for (Pkg &p : items) {
        const bool held = m_settings->isHeld(p.name);
        if (p.held != held || (held && p.selected)) {
            p.held = held;
            if (held)
                p.selected = false;
            changed = true;
        }
    }
    if (changed && !items.isEmpty()) {
        m_collect = items;
        m_model->setItems(items);
        emit selectionChanged();
        emit updatesChanged();
        updateSafetyFlags();
    }
}

void UpdateController::refreshSafety()
{
    updateSafetyFlags();
    updateArchGate();
}

void UpdateController::updateSafetyFlags()
{
    bool reboot = false;
    bool nvidiaWarn = false;
    bool hasKernel = false;
    bool hasNvidia = false;

    for (const Pkg &p : m_model->items()) {
        if (!p.selected)
            continue;
        if (p.kernel)
            reboot = true, hasKernel = true;
        const QString n = p.name.toLower();
        if (n == QLatin1String("systemd") || n.startsWith(QLatin1String("linux-firmware")))
            reboot = true;
        if (n.contains(QLatin1String("nvidia")))
            hasNvidia = true;
    }
    nvidiaWarn = hasKernel && hasNvidia;

    if (m_rebootRequired != reboot || m_nvidiaKernelWarning != nvidiaWarn) {
        m_rebootRequired = reboot;
        m_nvidiaKernelWarning = nvidiaWarn;
        emit safetyChanged();
    }
}

void UpdateController::updateArchGate()
{
    QString text;
    bool blocked = false;
    if (m_archGateChecker)
        blocked = m_archGateChecker(&text);
    if (m_archNewsBlocked != blocked || m_archGateText != text) {
        m_archNewsBlocked = blocked;
        m_archGateText = text;
        emit safetyChanged();
    }
}

void UpdateController::acknowledgeArchNews()
{
    m_archNewsBlocked = false;
    m_archGateText.clear();
    emit safetyChanged();
}

void UpdateController::reboot()
{
    QProcess::startDetached(QStringLiteral("systemctl"),
                            {QStringLiteral("reboot")});
}

void UpdateController::saveCachedCheck()
{
    QJsonArray arr;
    for (const Pkg &p : m_collect) {
        QJsonObject o;
        o.insert(QStringLiteral("name"), p.name);
        o.insert(QStringLiteral("old"), p.oldVersion);
        o.insert(QStringLiteral("new"), p.newVersion);
        o.insert(QStringLiteral("source"), sourceKey(p.source));
        o.insert(QStringLiteral("flatpakId"), p.flatpakId);
        arr.append(o);
    }
    QJsonObject root;
    root.insert(QStringLiteral("when"), QDateTime::currentDateTime().toString(Qt::ISODate));
    root.insert(QStringLiteral("packages"), arr);
    QFile f(cacheFilePath());
    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        f.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

bool UpdateController::loadCachedCheckData()
{
    QFile f(cacheFilePath());
    if (!f.open(QIODevice::ReadOnly))
        return false;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject())
        return false;
    m_collect.clear();
    const QJsonArray arr = doc.object().value(QStringLiteral("packages")).toArray();
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        Pkg p;
        p.name = o.value(QStringLiteral("name")).toString();
        p.oldVersion = o.value(QStringLiteral("old")).toString();
        p.newVersion = o.value(QStringLiteral("new")).toString();
        const QString src = o.value(QStringLiteral("source")).toString();
        if (src == QLatin1String("aur"))
            p.source = Source::Aur;
        else if (src == QLatin1String("flatpak"))
            p.source = Source::Flatpak;
        else
            p.source = Source::Repo;
        p.flatpakId = o.value(QStringLiteral("flatpakId")).toString();
        p.selected = true;
        m_collect << p;
    }
    return !m_collect.isEmpty();
}

void UpdateController::loadCachedCheck()
{
    if (!loadCachedCheckData()) {
        emitLine(QStringLiteral("No cached check results."), QStringLiteral("warn"));
        return;
    }
    finalizeCheck();
}

QString UpdateController::cacheFilePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
           + QStringLiteral("/last-check.json");
}

void UpdateController::seedFromCache()
{
    if (!loadCachedCheckData())
        return;

    for (Pkg &p : m_collect) {
        p.kernel = classifier::isKernel(p.name, p.source);
        p.severity = classifier::classify(p);
        p.summary = classifier::buildSummary(p);
    }
    applyHolds();
    m_model->setItems(m_collect);

    const int n = m_model->items().size();
    if (n == 0)
        setStatus(QStringLiteral("System is up to date."), QStringLiteral("uptodate"));
    else
        setStatus(QStringLiteral("%1 update%2 available.")
                      .arg(n)
                      .arg(n == 1 ? QString() : QStringLiteral("s")),
                  QStringLiteral("ready"));

    emit updatesChanged();
    emit checkFinished();
}

// ---------------------------------------------------------------------------
// Check pipeline
// ---------------------------------------------------------------------------

void UpdateController::check()
{
    if (m_busy)
        return;

    if (m_settings && m_settings->offlineMode()) {
        emitLine(QStringLiteral("Offline mode \u2014 loading cached results."),
                 QStringLiteral("warn"));
        loadCachedCheck();
        return;
    }

    setBusy(true);
    setStatus(QStringLiteral("Checking for updates\u2026"), QStringLiteral("checking"));
    m_collect.clear();
    m_warnings.clear();
    m_flatpakInstalled.clear();
    m_flatpakKinds.clear();

    emitLine(QStringLiteral("Checking for updates (repo / AUR / Flatpak)\u2026"),
             QStringLiteral("cmd"));

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
                if (code == 0) {
                    parsePacmanStyle(out, Source::Repo);
                } else {
                    if (code < 0 || code > 2
                        || out.contains(QLatin1String("failed to synchronize"),
                                        Qt::CaseInsensitive)) {
                        m_warnings << QStringLiteral(
                            "Could not reach package mirrors — try again later or pick another mirror.");
                    } else {
                        m_warnings << QStringLiteral(
                            "Repository check failed — see terminal output for details.");
                    }
                }
                checkAur();
            });
}

void UpdateController::checkAur()
{
    const QString helper = aurProgram();
    if (helper.isEmpty()) {
        checkFlatpak();
        return;
    }
    emitLine(QStringLiteral("$ %1 -Qua").arg(helper), QStringLiteral("cmd"));
    runStep(helper, {QStringLiteral("--color"), QStringLiteral("never"),
                     QStringLiteral("-Qua")},
            aurEnv(), [this](int, const QString &out) {
                parsePacmanStyle(out, Source::Aur);
                checkFlatpak();
            });
}

void UpdateController::loadFlatpakInstalled()
{
    if (which(QStringLiteral("flatpak")).isEmpty())
        return;
    runStep(QStringLiteral("flatpak"),
            {QStringLiteral("list"), QStringLiteral("--columns=application,version,type")},
            QProcessEnvironment(), [this](int, const QString &out) {
                const QList<QStringView> lines =
                    QStringView(out).split(QLatin1Char('\n'), Qt::SkipEmptyParts);
                for (const QStringView &lv : lines) {
                    const QStringList cols =
                        lv.toString().split(QLatin1Char('\t'), Qt::SkipEmptyParts);
                    if (cols.size() >= 2)
                        m_flatpakInstalled.insert(cols.at(0), cols.value(1));
                    if (cols.size() >= 3)
                        m_flatpakKinds.insert(cols.at(0), cols.value(2));
                }
            });
}

void UpdateController::checkFlatpak()
{
    if (m_settings && !m_settings->enableFlatpak()) {
        enrich();
        return;
    }
    if (which(QStringLiteral("flatpak")).isEmpty()) {
        enrich();
        return;
    }
    loadFlatpakInstalled();
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
        p.oldVersion = m_flatpakInstalled.value(p.flatpakId);
        p.flatpakKind = m_flatpakKinds.value(p.flatpakId, QStringLiteral("app"));
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

    auto afterRepo = [this]() {
        if (m_collect.isEmpty()
            || std::none_of(m_collect.begin(), m_collect.end(),
                            [](const Pkg &p) { return p.source == Source::Aur; })) {
            finalizeCheck();
            return;
        }
        fetchAurChangelogs();
    };

    if (repoNames.isEmpty() || which(QStringLiteral("expac")).isEmpty()) {
        afterRepo();
        return;
    }

    QStringList args;
    args << QStringLiteral("-S")
         << QStringLiteral("%n\t%r\t%k\t%m\t%G\t%d\t%C") << QStringLiteral("--");
    args += repoNames;

    runStep(QStringLiteral("expac"), args, QProcessEnvironment(),
            [this, afterRepo](int, const QString &out) {
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
                    p.changelog = f.value(6).trimmed();
                }
                afterRepo();
            });
}

void UpdateController::fetchAurChangelogs()
{
    if (m_settings && m_settings->offlineMode()) {
        finalizeCheck();
        return;
    }

    QStringList names;
    for (const Pkg &p : m_collect)
        if (p.source == Source::Aur)
            names << p.name;
    if (names.isEmpty()) {
        finalizeCheck();
        return;
    }

    QUrl url(QStringLiteral("https://aur.archlinux.org/rpc/"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("v"), QStringLiteral("5"));
    q.addQueryItem(QStringLiteral("type"), QStringLiteral("info"));
    for (const QString &n : names)
        q.addQueryItem(QStringLiteral("arg[]"), n);
    url.setQuery(q);

    auto *nam = new QNetworkAccessManager(this);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader,
                  QStringLiteral("cachyos-updater/1.0"));
    req.setTransferTimeout(15000);
    QNetworkReply *reply = nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, nam]() {
        const auto err = reply->error();
        const QByteArray data = reply->readAll();
        reply->deleteLater();
        nam->deleteLater();
        if (err == QNetworkReply::NoError) {
            const QJsonDocument doc = QJsonDocument::fromJson(data);
            const QJsonArray results =
                doc.object().value(QStringLiteral("results")).toArray();
            QHash<QString, QString> descByName;
            for (const QJsonValue &v : results) {
                const QJsonObject o = v.toObject();
                descByName.insert(o.value(QStringLiteral("Name")).toString(),
                                  o.value(QStringLiteral("Description")).toString());
            }
            for (Pkg &p : m_collect) {
                if (p.source != Source::Aur)
                    continue;
                const QString d = descByName.value(p.name);
                if (!d.isEmpty())
                    p.changelog = d;
            }
        }
        finalizeCheck();
    });
}

void UpdateController::finalizeCheck()
{
    const QString running = helpers::runningKernel();
    for (Pkg &p : m_collect) {
        p.kernel = classifier::isKernel(p.name, p.source);
        if (p.kernel && !running.isEmpty()
            && running.contains(p.name, Qt::CaseInsensitive))
            p.runningKernel = true;
        p.severity = classifier::classify(p);
        p.summary = classifier::buildSummary(p);
        if (!p.changelog.isEmpty()) {
            p.summary = p.changelog.left(400);
            if (p.changelog.size() > 400)
                p.summary += QStringLiteral("\u2026");
        }
    }

    applyHolds();
    m_model->setItems(m_collect);
    m_lastChecked = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm"));
    saveCachedCheck();
    updateSafetyFlags();
    updateArchGate();

    qint64 reclaim = 0;
    for (const Pkg &p : m_collect)
        reclaim += p.sizeBytes;
    m_reclaimableSpace = formatBytes(reclaim);

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
    const QString helper = aurProgram();

    for (const Pkg &p : m_model->items()) {
        switch (p.source) {
        case Source::Repo:
            ++repoAll;
            if (p.selected)
                ++repoSel;
            else
                deselectedRepo << p.name;
            break;
        case Source::Aur:
            ++aurAll;
            if (p.selected) {
                ++aurSel;
                aurNames << p.name;
            }
            break;
        case Source::Flatpak:
            ++flatAll;
            if (p.selected)
                flatIds << (p.flatpakId.isEmpty() ? p.name : p.flatpakId), ++flatSel;
            break;
        }
    }

    if (repoSel > 0) {
        QString c = QStringLiteral("pkexec pacman -Syu --noconfirm");
        c += QStringLiteral(" ") + pacmanBandwidthArgs().join(QLatin1Char(' '));
        if (!deselectedRepo.isEmpty())
            c += QStringLiteral(" --ignore ") + deselectedRepo.join(QLatin1Char(','));
        cmds << c.trimmed();
    }
    if (aurSel > 0 && !helper.isEmpty()) {
        cmds << (aurSel == aurAll
                     ? helper + QStringLiteral(" -Sua --noconfirm")
                     : helper + QStringLiteral(" -S --noconfirm ") + aurNames.join(QLatin1Char(' ')));
    }
    if (flatSel > 0) {
        cmds << (flatSel == flatAll
                     ? QStringLiteral("flatpak update -y")
                     : QStringLiteral("flatpak update -y ") + flatIds.join(QLatin1Char(' ')));
    }
    return cmds.join(QLatin1Char('\n'));
}

void UpdateController::apply() { beginRun(Mode::Apply); }
void UpdateController::applyRepo() { beginRun(Mode::Apply, Source::Repo, true); }
void UpdateController::applyAur() { beginRun(Mode::Apply, Source::Aur, true); }
void UpdateController::applyFlatpak() { beginRun(Mode::Apply, Source::Flatpak, true); }
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

void UpdateController::beginRun(Mode mode, Source only, bool onlySet)
{
    if (m_busy || m_running)
        return;
    if (m_archNewsBlocked && mode == Mode::Apply) {
        emitLine(QStringLiteral("Arch news must be acknowledged before applying repo updates."),
                 QStringLiteral("warn"));
        return;
    }

    m_singleSourceRun = onlySet;
    m_singleSource = only;

    int selected = 0;
    for (const Pkg &p : m_model->items()) {
        if (!p.selected)
            continue;
        if (onlySet && p.source != only)
            continue;
        ++selected;
    }
    if (selected == 0) {
        emitLine(QStringLiteral("Nothing selected."), QStringLiteral("warn"));
        return;
    }

    auto startRun = [this, mode]() {
        m_mode = mode;
        m_running = true;
        setBusy(true);
        m_warnings.clear();
        m_runQueue.clear();
        QSet<QString> present;
        for (const Pkg &p : m_model->items()) {
            if (!p.selected)
                continue;
            if (m_singleSourceRun && p.source != m_singleSource)
                continue;
            if (mode == Mode::DownloadOnly && p.source != Source::Repo)
                continue;
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
    };

    const bool wantSnapshot = m_settings && m_settings->enableSnapshots()
                              && snapshotsAvailable() && mode == Mode::Apply
                              && !m_singleSourceRun;
    if (wantSnapshot) {
        createSnapshot([this, startRun](bool) { startRun(); });
    } else {
        startRun();
    }
}

void UpdateController::createSnapshot(std::function<void(bool)> onDone)
{
    emitLine(QStringLiteral("Creating pre-update snapshot\u2026"), QStringLiteral("cmd"));
    if (!which(QStringLiteral("snapper")).isEmpty()) {
        runStep(QStringLiteral("pkexec"),
                {QStringLiteral("snapper"), QStringLiteral("create"),
                 QStringLiteral("--type"), QStringLiteral("pre"),
                 QStringLiteral("--description"), QStringLiteral("cachyos-updater")},
                QProcessEnvironment(),
                [this, onDone](int code, const QString &) {
                    emitLine(code == 0 ? QStringLiteral("Snapshot created.")
                                       : QStringLiteral("Snapshot skipped."),
                             code == 0 ? QStringLiteral("ok") : QStringLiteral("warn"));
                    onDone(code == 0);
                });
        return;
    }
    onDone(false);
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
            if (!p.selected || (m_singleSourceRun && m_singleSource != Source::Repo))
                deselectedRepo << p.name;
        } else if (p.source == Source::Aur) {
            ++aurAll;
            if (p.selected && (!m_singleSourceRun || m_singleSource == Source::Aur))
                aurNames << p.name, ++aurSel;
        } else {
            ++flatAll;
            if (p.selected && (!m_singleSourceRun || m_singleSource == Source::Flatpak)) {
                flatIds << (p.flatpakId.isEmpty() ? p.name : p.flatpakId);
                ++flatSel;
            }
        }
    }

    QString program;
    QStringList args;
    QProcessEnvironment env;

    if (src == Source::Repo) {
        program = QStringLiteral("pkexec");
        args << QStringLiteral("pacman")
             << (m_mode == Mode::DownloadOnly ? QStringLiteral("-Syuw")
                                              : QStringLiteral("-Syu"))
             << QStringLiteral("--noconfirm");
        args += pacmanBandwidthArgs();
        if (!deselectedRepo.isEmpty() && !m_singleSourceRun) {
            args << QStringLiteral("--ignore") << deselectedRepo.join(QLatin1Char(','));
        }
        setStage(QStringLiteral("Sync"), 0.15);
    } else if (src == Source::Aur) {
        program = aurProgram();
        if (program.isEmpty()) {
            ++m_runIndex;
            runNextGroup();
            return;
        }
        env = aurEnv();
        if (aurSel == aurAll && !m_singleSourceRun)
            args << QStringLiteral("-Sua") << QStringLiteral("--noconfirm");
        else
            args << QStringLiteral("-S") << QStringLiteral("--noconfirm") << aurNames;
        setStage(QStringLiteral("AUR build"), 0.55);
    } else {
        program = QStringLiteral("flatpak");
        args << QStringLiteral("update") << QStringLiteral("-y");
        if (flatSel != flatAll || m_singleSourceRun)
            args += flatIds;
        setStage(QStringLiteral("Flatpak"), 0.8);
    }

    emitLine(QStringLiteral("$ %1 %2").arg(program, args.join(QLatin1Char(' '))),
             QStringLiteral("cmd"));

    auto *r = new ProcessRunner(this);
    r->setMerged(true);
    r->setTimeout(0);
    connect(r, &ProcessRunner::line, this, [this](const QString &l) {
        emitLine(l, classifyLineKind(l));
        const QString low = l.toLower();
        if (low.contains(QLatin1String("synchronizing package")))
            setStage(QStringLiteral("Sync"), 0.2);
        else if (low.contains(QLatin1String("downloading"))
                 || low.contains(QLatin1String("retrieving packages")))
            setStage(QStringLiteral("Download"), 0.4);
        else if (low.contains(QLatin1String("checking"))
                 || low.contains(QLatin1String("processing package changes"))
                 || low.startsWith(QLatin1String("installing"))
                 || low.startsWith(QLatin1String("upgrading")))
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
    r->start(program, args, env);
}

void UpdateController::finishRun(bool ok)
{
    m_running = false;
    setBusy(false);
    setStage(ok ? QStringLiteral("Summary") : QStringLiteral("Failed"),
             ok ? 1.0 : m_progress);

    const QString detail = plannedCommands();
    if (m_history)
        m_history->record(m_mode == Mode::DownloadOnly ? QStringLiteral("Download")
                                                     : QStringLiteral("Apply"),
                          detail, ok);

    if (ok) {
        emitLine(QStringLiteral("Done."), QStringLiteral("ok"));
        setStatus(m_mode == Mode::DownloadOnly
                      ? QStringLiteral("Download complete.")
                      : QStringLiteral("Update complete. Re-checking\u2026"),
                  QStringLiteral("done"));
        updateSafetyFlags();
        emit applyFinished(true);
        if (m_mode != Mode::DownloadOnly)
            check();
    } else {
        setStatus(QStringLiteral("Update failed \u2014 see terminal for details."),
                  QStringLiteral("error"));
        emit applyFinished(false);
    }
}

void UpdateController::cancel()
{
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
    if (low.contains(QLatin1String("error")) || low.contains(QLatin1String("failed"))
        || low.startsWith(QLatin1String("==> error")))
        return QStringLiteral("error");
    if (low.contains(QLatin1String("warning")) || low.startsWith(QLatin1String("==> warning")))
        return QStringLiteral("warn");
    if (line.startsWith(QLatin1String("::")) || line.startsWith(QLatin1String("==>")))
        return QStringLiteral("cmd");
    return QStringLiteral("out");
}
