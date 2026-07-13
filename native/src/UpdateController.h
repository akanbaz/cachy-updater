#pragma once

#include "Pkg.h"
#include "UpdatesModel.h"

#include <QObject>
#include <QProcessEnvironment>
#include <QString>
#include <QStringList>
#include <QVector>
#include <functional>

class HistoryController;
class ProcessRunner;
class SettingsController;

class UpdateController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(QString statusState READ statusState NOTIFY statusChanged)
    Q_PROPERTY(int packageCount READ packageCount NOTIFY updatesChanged)
    Q_PROPERTY(int sourceCount READ sourceCount NOTIFY updatesChanged)
    Q_PROPERTY(QString downloadText READ downloadText NOTIFY updatesChanged)
    Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectionChanged)
    Q_PROPERTY(QString warningText READ warningText NOTIFY updatesChanged)
    Q_PROPERTY(QString lastChecked READ lastChecked NOTIFY updatesChanged)
    Q_PROPERTY(QString stage READ stage NOTIFY stageChanged)
    Q_PROPERTY(qreal progress READ progress NOTIFY stageChanged)
    Q_PROPERTY(bool rebootRequired READ rebootRequired NOTIFY safetyChanged)
    Q_PROPERTY(bool nvidiaKernelWarning READ nvidiaKernelWarning NOTIFY safetyChanged)
    Q_PROPERTY(bool archNewsBlocked READ archNewsBlocked NOTIFY safetyChanged)
    Q_PROPERTY(QString archGateText READ archGateText NOTIFY safetyChanged)
    Q_PROPERTY(QString runningKernel READ runningKernel NOTIFY kernelInfoChanged)
    Q_PROPERTY(QStringList installedKernels READ installedKernels NOTIFY kernelInfoChanged)
    Q_PROPERTY(QString kernelHeadline READ kernelHeadline NOTIFY kernelInfoChanged)
    Q_PROPERTY(QString kernelDetail READ kernelDetail NOTIFY kernelInfoChanged)
    Q_PROPERTY(bool showKernelInfo READ showKernelInfo NOTIFY kernelInfoChanged)
    Q_PROPERTY(bool snapshotsAvailable READ snapshotsAvailable CONSTANT)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY filtersChanged)
    Q_PROPERTY(int minSeverity READ minSeverity WRITE setMinSeverity NOTIFY filtersChanged)
    Q_PROPERTY(QString sourceFilter READ sourceFilter WRITE setSourceFilter NOTIFY filtersChanged)
    Q_PROPERTY(QString reclaimableSpace READ reclaimableSpace NOTIFY updatesChanged)
    Q_PROPERTY(QObject *updatesModel READ updatesModelObject CONSTANT)
    Q_PROPERTY(QObject *kernelModel READ kernelModelObject CONSTANT)

public:
    explicit UpdateController(SettingsController *settings = nullptr,
                              HistoryController *history = nullptr,
                              QObject *parent = nullptr);

    void setNewsGateChecker(std::function<bool(QString *)> checker);

    bool busy() const { return m_busy; }
    QString statusText() const { return m_statusText; }
    QString statusState() const { return m_statusState; }
    int packageCount() const;
    int sourceCount() const;
    QString downloadText() const;
    int selectedCount() const { return m_model->selectedCount(); }
    QString warningText() const { return m_warnings.join(QStringLiteral("  \u2022  ")); }
    QString lastChecked() const { return m_lastChecked; }
    QString stage() const { return m_stage; }
    qreal progress() const { return m_progress; }
    bool rebootRequired() const { return m_rebootRequired; }
    bool nvidiaKernelWarning() const { return m_nvidiaKernelWarning; }
    bool archNewsBlocked() const { return m_archNewsBlocked; }
    QString archGateText() const { return m_archGateText; }
    QString runningKernel() const { return m_runningKernel; }
    QStringList installedKernels() const { return m_installedKernels; }
    QString kernelHeadline() const { return m_kernelHeadline; }
    QString kernelDetail() const { return m_kernelDetail; }
    bool showKernelInfo() const { return !m_runningKernel.isEmpty(); }
    bool snapshotsAvailable() const;
    QString searchText() const;
    int minSeverity() const;
    QString sourceFilter() const;
    QString reclaimableSpace() const { return m_reclaimableSpace; }
    QObject *updatesModelObject() const;
    QObject *kernelModelObject() const;

    Q_INVOKABLE void check();
    Q_INVOKABLE void apply();
    Q_INVOKABLE void applyRepo();
    Q_INVOKABLE void applyAur();
    Q_INVOKABLE void applyFlatpak();
    Q_INVOKABLE void dryRun();
    Q_INVOKABLE void downloadOnly();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void setAllSelected(bool selected) { m_model->setAllSelected(selected); }
    Q_INVOKABLE void setSearchText(const QString &text);
    Q_INVOKABLE void setMinSeverity(int severity);
    Q_INVOKABLE void setSourceFilter(const QString &source);
    Q_INVOKABLE void clearFilters();
    Q_INVOKABLE void holdPackage(const QString &name);
    Q_INVOKABLE void unholdPackage(const QString &name);
    Q_INVOKABLE void acknowledgeArchNews();
    Q_INVOKABLE void reboot();
    Q_INVOKABLE void refreshKernelInfo();
    Q_INVOKABLE void refreshSafety();
    Q_INVOKABLE void loadCachedCheck();
    Q_INVOKABLE void seedFromCache();
    static QString cacheFilePath();

    Q_INVOKABLE QString sourceCommandFor(const QString &source) const;
    Q_INVOKABLE int sourceCountFor(const QString &source) const;
    Q_INVOKABLE QString sourceSizeTextFor(const QString &source) const;
    Q_INVOKABLE QString plannedCommands() const;
    Q_INVOKABLE int criticalCount() const;

signals:
    void busyChanged();
    void statusChanged();
    void updatesChanged();
    void selectionChanged();
    void stageChanged();
    void lineEmitted(const QString &text, const QString &kind);
    void checkFinished();
    void applyFinished(bool ok);
    void safetyChanged();
    void kernelInfoChanged();
    void filtersChanged();
    void notifyRequested(const QString &title, const QString &body);

private:
    enum class Mode { Apply, DryRun, DownloadOnly };

    void setBusy(bool busy);
    void setStatus(const QString &text, const QString &state);
    void setStage(const QString &stage, qreal progress);
    void emitLine(const QString &text, const QString &kind = QStringLiteral("out"));
    void updateSafetyFlags();
    void updateArchGate();
    void applyHolds();
    void saveCachedCheck();
    bool loadCachedCheckData();
    QString aurProgram() const;

    void updateKernelCopy();

    void runStep(const QString &program, const QStringList &args,
                 const QProcessEnvironment &env,
                 std::function<void(int, const QString &)> onDone);

    void checkAur();
    void checkFlatpak();
    void enrich();
    void fetchAurChangelogs();
    void finalizeCheck();
    void parsePacmanStyle(const QString &out, cachy::Source source);
    void parseFlatpak(const QString &out);
    void loadFlatpakInstalled();

    void beginRun(Mode mode, cachy::Source only = cachy::Source::Repo, bool onlySet = false);
    void createSnapshot(std::function<void(bool)> onDone);
    void runNextGroup();
    void finishRun(bool ok);

    QString classifyLineKind(const QString &line) const;
    static QString which(const QString &program);
    QStringList pacmanBandwidthArgs() const;
    QProcessEnvironment aurEnv() const;

    SettingsController *m_settings;
    HistoryController *m_history;
    UpdatesModel *m_model;
    UpdateListProxy *m_listProxy;
    PkgFilterProxy *m_kernelProxy;

    QVector<cachy::Pkg> m_collect;
    QStringList m_warnings;
    QString m_checkDbPath;
    QString m_lastChecked;
    QString m_runningKernel;
    QStringList m_installedKernels;
    QString m_kernelHeadline;
    QString m_kernelDetail;
    QString m_reclaimableSpace;
    QString m_archGateText;
    bool m_archNewsBlocked = false;
    bool m_rebootRequired = false;
    bool m_nvidiaKernelWarning = false;

    bool m_busy = false;
    QString m_statusText = QStringLiteral("Ready.");
    QString m_statusState = QStringLiteral("idle");
    QString m_stage = QStringLiteral("Idle");
    qreal m_progress = 0.0;

    Mode m_mode = Mode::Apply;
    QVector<cachy::Source> m_runQueue;
    int m_runIndex = 0;
    bool m_running = false;
    bool m_singleSourceRun = false;
    cachy::Source m_singleSource = cachy::Source::Repo;

    std::function<bool(QString *)> m_archGateChecker;
    QHash<QString, QString> m_flatpakInstalled;
    QHash<QString, QString> m_flatpakKinds;
};
