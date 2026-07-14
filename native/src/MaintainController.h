#pragma once

#include <QObject>
#include <QStringList>
#include <functional>

class ProcessRunner;
class SettingsController;

class MaintainController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
    Q_PROPERTY(int orphanCount READ orphanCount NOTIFY changed)
    Q_PROPERTY(QString orphansText READ orphansText NOTIFY changed)
    Q_PROPERTY(int cacheOldCount READ cacheOldCount NOTIFY changed)
    Q_PROPERTY(int cacheUninstalledCount READ cacheUninstalledCount NOTIFY changed)
    Q_PROPERTY(int cacheTotal READ cacheTotal NOTIFY changed)
    Q_PROPERTY(int oldKernelCount READ oldKernelCount NOTIFY changed)
    Q_PROPERTY(int flatpakUnusedCount READ flatpakUnusedCount NOTIFY changed)
    Q_PROPERTY(int aurCacheCount READ aurCacheCount NOTIFY changed)
    Q_PROPERTY(QString diskSummary READ diskSummary NOTIFY changed)
    Q_PROPERTY(QString warningText READ warningText NOTIFY changed)

public:
    explicit MaintainController(SettingsController *settings = nullptr,
                                QObject *parent = nullptr);

    bool busy() const { return m_busy; }
    bool scanning() const { return m_inflight > 0; }
    int orphanCount() const { return m_orphans.size(); }
    QString orphansText() const { return m_orphans.join(QLatin1Char(' ')); }
    int cacheOldCount() const { return m_cacheOld; }
    int cacheUninstalledCount() const { return m_cacheUninstalled; }
    int cacheTotal() const { return m_cacheOld + m_cacheUninstalled; }
    int oldKernelCount() const { return m_oldKernels.size(); }
    int flatpakUnusedCount() const { return m_flatpakUnused; }
    int aurCacheCount() const { return m_aurCacheCount; }
    QString diskSummary() const { return m_diskSummary; }
    QString warningText() const { return m_warnings.join(QStringLiteral("  \u2022  ")); }

    Q_INVOKABLE void scan();
    Q_INVOKABLE void removeOrphans();
    Q_INVOKABLE void cleanCache();
    Q_INVOKABLE void removeOldKernels();
    Q_INVOKABLE void cleanFlatpakUnused();
    Q_INVOKABLE void cleanAurCache();
    Q_INVOKABLE void cancel();

signals:
    void busyChanged();
    void scanningChanged();
    void changed();
    void lineEmitted(const QString &text, const QString &kind);

private:
    int keepOld() const;
    int keepUninstalled() const { return 0; }
    void setBusy(bool busy);
    void incInflight(int delta);
    void emitLine(const QString &text, const QString &kind = QStringLiteral("out"));
    void runQuiet(const QString &program, const QStringList &args,
                  std::function<void(int, const QString &)> onDone);
    void runStreaming(const QString &program, const QStringList &args,
                      std::function<void(int)> onDone);
    void updateDiskSummary();
    static QString which(const QString &program);

    SettingsController *m_settings;
    QStringList m_orphans;
    QStringList m_oldKernels;
    QStringList m_warnings;
    int m_cacheOld = 0;
    int m_cacheUninstalled = 0;
    int m_flatpakUnused = 0;
    int m_aurCacheCount = 0;
    QString m_diskSummary;
    int m_inflight = 0;
    bool m_busy = false;
};
