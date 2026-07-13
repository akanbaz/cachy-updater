#pragma once

#include <QObject>
#include <QSettings>
#include <QStringList>

// Persistent user preferences via QSettings (org.cachyos / cachyos-updater).
class SettingsController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int trayIntervalMinutes READ trayIntervalMinutes WRITE setTrayIntervalMinutes
                   NOTIFY settingsChanged)
    Q_PROPERTY(bool autoCheckOnStartup READ autoCheckOnStartup WRITE setAutoCheckOnStartup
                   NOTIFY settingsChanged)
    Q_PROPERTY(int defaultTab READ defaultTab WRITE setDefaultTab NOTIFY settingsChanged)
    Q_PROPERTY(bool enableAur READ enableAur WRITE setEnableAur NOTIFY settingsChanged)
    Q_PROPERTY(bool enableFlatpak READ enableFlatpak WRITE setEnableFlatpak NOTIFY settingsChanged)
    Q_PROPERTY(bool enableNews READ enableNews WRITE setEnableNews NOTIFY settingsChanged)
    Q_PROPERTY(bool offlineMode READ offlineMode WRITE setOfflineMode NOTIFY settingsChanged)
    Q_PROPERTY(int cacheKeepVersions READ cacheKeepVersions WRITE setCacheKeepVersions
                   NOTIFY settingsChanged)
    Q_PROPERTY(bool notifyCriticalOnly READ notifyCriticalOnly WRITE setNotifyCriticalOnly
                   NOTIFY settingsChanged)
    Q_PROPERTY(int maxDownloads READ maxDownloads WRITE setMaxDownloads NOTIFY settingsChanged)
    Q_PROPERTY(int paruConcurrency READ paruConcurrency WRITE setParuConcurrency
                   NOTIFY settingsChanged)
    Q_PROPERTY(bool enableSnapshots READ enableSnapshots WRITE setEnableSnapshots
                   NOTIFY settingsChanged)
    Q_PROPERTY(bool scheduledChecks READ scheduledChecks WRITE setScheduledChecks
                   NOTIFY settingsChanged)
    Q_PROPERTY(QStringList holdPackages READ holdPackages NOTIFY holdPackagesChanged)

public:
    explicit SettingsController(QObject *parent = nullptr);

    int trayIntervalMinutes() const { return m_trayIntervalMinutes; }
    bool autoCheckOnStartup() const { return m_autoCheckOnStartup; }
    int defaultTab() const { return m_defaultTab; }
    bool enableAur() const { return m_enableAur; }
    bool enableFlatpak() const { return m_enableFlatpak; }
    bool enableNews() const { return m_enableNews; }
    bool offlineMode() const { return m_offlineMode; }
    int cacheKeepVersions() const { return m_cacheKeepVersions; }
    bool notifyCriticalOnly() const { return m_notifyCriticalOnly; }
    int maxDownloads() const { return m_maxDownloads; }
    int paruConcurrency() const { return m_paruConcurrency; }
    bool enableSnapshots() const { return m_enableSnapshots; }
    bool scheduledChecks() const { return m_scheduledChecks; }
    QStringList holdPackages() const { return m_holdPackages; }
    QStringList acknowledgedArchNews() const { return m_acknowledgedArchNews; }

    void setTrayIntervalMinutes(int v);
    void setAutoCheckOnStartup(bool v);
    void setDefaultTab(int v);
    void setEnableAur(bool v);
    void setEnableFlatpak(bool v);
    void setEnableNews(bool v);
    void setOfflineMode(bool v);
    void setCacheKeepVersions(int v);
    void setNotifyCriticalOnly(bool v);
    void setMaxDownloads(int v);
    void setParuConcurrency(int v);
    void setEnableSnapshots(bool v);
    void setScheduledChecks(bool v);

    Q_INVOKABLE bool isHeld(const QString &pkg) const;
    Q_INVOKABLE void addHold(const QString &pkg);
    Q_INVOKABLE void removeHold(const QString &pkg);
    Q_INVOKABLE void acknowledgeArchNews(const QString &id);
    Q_INVOKABLE bool isArchNewsAcknowledged(const QString &id) const;
    Q_INVOKABLE void save();
    Q_INVOKABLE void load();

signals:
    void settingsChanged();
    void holdPackagesChanged();

private:
    QSettings m_store;
    int m_trayIntervalMinutes = 30;
    bool m_autoCheckOnStartup = true;
    int m_defaultTab = 0;
    bool m_enableAur = true;
    bool m_enableFlatpak = true;
    bool m_enableNews = true;
    bool m_offlineMode = false;
    int m_cacheKeepVersions = 3;
    bool m_notifyCriticalOnly = false;
    int m_maxDownloads = 0;
    int m_paruConcurrency = 0;
    bool m_enableSnapshots = true;
    bool m_scheduledChecks = false;
    QStringList m_holdPackages;
    QStringList m_acknowledgedArchNews;
};
