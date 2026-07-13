#pragma once

#include <QObject>
#include <QStringList>
#include <functional>

class ProcessRunner;

// Orphan-package and pacman-cache maintenance. Read-only scans run freely;
// destructive actions are single-flight and stream output to an in-panel log.
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
    Q_PROPERTY(QString warningText READ warningText NOTIFY changed)

public:
    explicit MaintainController(QObject *parent = nullptr);

    bool busy() const { return m_busy; }
    bool scanning() const { return m_inflight > 0; }
    int orphanCount() const { return m_orphans.size(); }
    QString orphansText() const { return m_orphans.join(QLatin1Char(' ')); }
    int cacheOldCount() const { return m_cacheOld; }
    int cacheUninstalledCount() const { return m_cacheUninstalled; }
    int cacheTotal() const { return m_cacheOld + m_cacheUninstalled; }
    QString warningText() const { return m_warnings.join(QStringLiteral("  \u2022  ")); }

    Q_INVOKABLE void scan();
    Q_INVOKABLE void removeOrphans();
    Q_INVOKABLE void cleanCache();

signals:
    void busyChanged();
    void scanningChanged();
    void changed();
    void lineEmitted(const QString &text, const QString &kind);

private:
    void setBusy(bool busy);
    void incInflight(int delta);
    void emitLine(const QString &text, const QString &kind = QStringLiteral("out"));
    void runQuiet(const QString &program, const QStringList &args,
                  std::function<void(int, const QString &)> onDone);
    void runStreaming(const QString &program, const QStringList &args,
                      std::function<void(int)> onDone);
    static QString which(const QString &program);

    QStringList m_orphans;
    QStringList m_warnings;
    int m_cacheOld = 0;
    int m_cacheUninstalled = 0;
    int m_inflight = 0;
    bool m_busy = false;

    static constexpr int kKeepOld = 3;
    static constexpr int kKeepUninstalled = 0;
};
