#include "SettingsController.h"

#include "Helpers.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

SettingsController::SettingsController(QObject *parent)
    : QObject(parent)
    , m_store(QStringLiteral("CachyOS"), QStringLiteral("cachyos-updater"))
{
    load();
    refreshConfigWritable();
}

void SettingsController::refreshConfigWritable()
{
    QString err;
    const QString confDir =
        QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
        + QStringLiteral("/CachyOS");
    const QString cacheDir =
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation);

    m_configWarning.clear();
    if (!cachy::helpers::ensureUserDirWritable(confDir, &err))
        m_configWarning = err;
    else if (!cachy::helpers::ensureUserDirWritable(cacheDir, &err))
        m_configWarning = err;

    const QString confFile = confDir + QStringLiteral("/cachyos-updater.conf");
    if (QFileInfo::exists(confFile)) {
        QFileInfo fi(confFile);
        if (!fi.isWritable()) {
            m_configWarning = QStringLiteral(
                "Config file is not writable (often owned by root after a sudo run): %1")
                                  .arg(confFile);
        }
    }
}

void SettingsController::setTrayIntervalMinutes(int v)
{
    v = qBound(5, v, 1440);
    if (m_trayIntervalMinutes == v)
        return;
    m_trayIntervalMinutes = v;
    emit settingsChanged();
    save();
}

void SettingsController::setAutoCheckOnStartup(bool v)
{
    if (m_autoCheckOnStartup == v)
        return;
    m_autoCheckOnStartup = v;
    emit settingsChanged();
    save();
}

void SettingsController::setDefaultTab(int v)
{
    v = qBound(0, v, 5);
    if (m_defaultTab == v)
        return;
    m_defaultTab = v;
    emit settingsChanged();
    save();
}

void SettingsController::setEnableAur(bool v)
{
    if (m_enableAur == v)
        return;
    m_enableAur = v;
    emit settingsChanged();
    save();
}

void SettingsController::setEnableFlatpak(bool v)
{
    if (m_enableFlatpak == v)
        return;
    m_enableFlatpak = v;
    emit settingsChanged();
    save();
}

void SettingsController::setEnableNews(bool v)
{
    if (m_enableNews == v)
        return;
    m_enableNews = v;
    emit settingsChanged();
    save();
}

void SettingsController::setOfflineMode(bool v)
{
    if (m_offlineMode == v)
        return;
    m_offlineMode = v;
    emit settingsChanged();
    save();
}

void SettingsController::setCacheKeepVersions(int v)
{
    v = qBound(1, v, 10);
    if (m_cacheKeepVersions == v)
        return;
    m_cacheKeepVersions = v;
    emit settingsChanged();
    save();
}

void SettingsController::setNotifyCriticalOnly(bool v)
{
    if (m_notifyCriticalOnly == v)
        return;
    m_notifyCriticalOnly = v;
    emit settingsChanged();
    save();
}

void SettingsController::setMaxDownloads(int v)
{
    v = qBound(0, v, 20);
    if (m_maxDownloads == v)
        return;
    m_maxDownloads = v;
    emit settingsChanged();
    save();
}

void SettingsController::setParuConcurrency(int v)
{
    v = qBound(0, v, 16);
    if (m_paruConcurrency == v)
        return;
    m_paruConcurrency = v;
    emit settingsChanged();
    save();
}

void SettingsController::setEnableSnapshots(bool v)
{
    if (m_enableSnapshots == v)
        return;
    m_enableSnapshots = v;
    emit settingsChanged();
    save();
}

void SettingsController::setScheduledChecks(bool v)
{
    if (m_scheduledChecks == v)
        return;
    m_scheduledChecks = v;
    emit settingsChanged();
    save();
}

bool SettingsController::isHeld(const QString &pkg) const
{
    return m_holdPackages.contains(pkg, Qt::CaseInsensitive);
}

void SettingsController::addHold(const QString &pkg)
{
    const QString p = pkg.trimmed();
    if (p.isEmpty() || isHeld(p))
        return;
    m_holdPackages << p;
    emit holdPackagesChanged();
    save();
}

void SettingsController::removeHold(const QString &pkg)
{
    for (int i = m_holdPackages.size() - 1; i >= 0; --i) {
        if (m_holdPackages.at(i).compare(pkg, Qt::CaseInsensitive) == 0) {
            m_holdPackages.removeAt(i);
            emit holdPackagesChanged();
            save();
            return;
        }
    }
}

void SettingsController::acknowledgeArchNews(const QString &id)
{
    const QString key = id.trimmed();
    if (key.isEmpty() || m_acknowledgedArchNews.contains(key))
        return;
    m_acknowledgedArchNews << key;
    save();
}

bool SettingsController::isArchNewsAcknowledged(const QString &id) const
{
    return m_acknowledgedArchNews.contains(id.trimmed());
}

void SettingsController::save()
{
    refreshConfigWritable();
    if (!m_configWarning.isEmpty()) {
        emit settingsChanged();
        return;
    }
    m_store.setValue(QStringLiteral("trayIntervalMinutes"), m_trayIntervalMinutes);
    m_store.setValue(QStringLiteral("autoCheckOnStartup"), m_autoCheckOnStartup);
    m_store.setValue(QStringLiteral("defaultTab"), m_defaultTab);
    m_store.setValue(QStringLiteral("enableAur"), m_enableAur);
    m_store.setValue(QStringLiteral("enableFlatpak"), m_enableFlatpak);
    m_store.setValue(QStringLiteral("enableNews"), m_enableNews);
    m_store.setValue(QStringLiteral("offlineMode"), m_offlineMode);
    m_store.setValue(QStringLiteral("cacheKeepVersions"), m_cacheKeepVersions);
    m_store.setValue(QStringLiteral("notifyCriticalOnly"), m_notifyCriticalOnly);
    m_store.setValue(QStringLiteral("maxDownloads"), m_maxDownloads);
    m_store.setValue(QStringLiteral("paruConcurrency"), m_paruConcurrency);
    m_store.setValue(QStringLiteral("enableSnapshots"), m_enableSnapshots);
    m_store.setValue(QStringLiteral("scheduledChecks"), m_scheduledChecks);
    m_store.setValue(QStringLiteral("holdPackages"), m_holdPackages);
    m_store.setValue(QStringLiteral("acknowledgedArchNews"), m_acknowledgedArchNews);
    m_store.sync();
}

void SettingsController::load()
{
    m_trayIntervalMinutes = m_store.value(QStringLiteral("trayIntervalMinutes"), 30).toInt();
    m_autoCheckOnStartup = m_store.value(QStringLiteral("autoCheckOnStartup"), true).toBool();
    m_defaultTab = m_store.value(QStringLiteral("defaultTab"), 0).toInt();
    m_enableAur = m_store.value(QStringLiteral("enableAur"), true).toBool();
    m_enableFlatpak = m_store.value(QStringLiteral("enableFlatpak"), true).toBool();
    m_enableNews = m_store.value(QStringLiteral("enableNews"), true).toBool();
    m_offlineMode = m_store.value(QStringLiteral("offlineMode"), false).toBool();
    m_cacheKeepVersions = m_store.value(QStringLiteral("cacheKeepVersions"), 3).toInt();
    m_notifyCriticalOnly =
        m_store.value(QStringLiteral("notifyCriticalOnly"), false).toBool();
    m_maxDownloads = m_store.value(QStringLiteral("maxDownloads"), 0).toInt();
    m_paruConcurrency = m_store.value(QStringLiteral("paruConcurrency"), 0).toInt();
    m_enableSnapshots = m_store.value(QStringLiteral("enableSnapshots"), true).toBool();
    m_scheduledChecks = m_store.value(QStringLiteral("scheduledChecks"), false).toBool();
    m_holdPackages = m_store.value(QStringLiteral("holdPackages")).toStringList();
    m_acknowledgedArchNews =
        m_store.value(QStringLiteral("acknowledgedArchNews")).toStringList();
}
