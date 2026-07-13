#pragma once

#include "Pkg.h"
#include "UpdatesModel.h"

#include <QObject>
#include <QProcessEnvironment>
#include <QString>
#include <QStringList>
#include <QVector>
#include <functional>

class ProcessRunner;

// Central controller for the Updates page: runs the check pipeline, owns the
// data model, and drives apply / dry-run / download-only through pacman/paru/
// flatpak. Everything is async via ProcessRunner; the UI thread never blocks.
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
    Q_PROPERTY(QObject *updatesModel READ updatesModelObject CONSTANT)
    Q_PROPERTY(QObject *kernelModel READ kernelModelObject CONSTANT)

public:
    explicit UpdateController(QObject *parent = nullptr);

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
    QObject *updatesModelObject() const;
    QObject *kernelModelObject() const;

    Q_INVOKABLE void check();
    Q_INVOKABLE void apply();
    Q_INVOKABLE void dryRun();
    Q_INVOKABLE void downloadOnly();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void setAllSelected(bool selected) { m_model->setAllSelected(selected); }

    Q_INVOKABLE QString sourceCommandFor(const QString &source) const;
    Q_INVOKABLE int sourceCountFor(const QString &source) const;
    Q_INVOKABLE QString sourceSizeTextFor(const QString &source) const;
    Q_INVOKABLE QString plannedCommands() const;

signals:
    void busyChanged();
    void statusChanged();
    void updatesChanged();
    void selectionChanged();
    void stageChanged();
    void lineEmitted(const QString &text, const QString &kind);
    void checkFinished();
    void applyFinished(bool ok);

private:
    enum class Mode { Apply, DryRun, DownloadOnly };

    void setBusy(bool busy);
    void setStatus(const QString &text, const QString &state);
    void setStage(const QString &stage, qreal progress);
    void emitLine(const QString &text, const QString &kind = QStringLiteral("out"));

    void runStep(const QString &program, const QStringList &args,
                 const QProcessEnvironment &env,
                 std::function<void(int, const QString &)> onDone);

    // Check pipeline stages.
    void checkAur();
    void checkFlatpak();
    void enrich();
    void finalizeCheck();
    void parsePacmanStyle(const QString &out, cachy::Source source);
    void parseFlatpak(const QString &out);

    // Apply pipeline.
    void beginRun(Mode mode);
    void runNextGroup();
    void finishRun(bool ok);

    QString classifyLineKind(const QString &line) const;
    static QString which(const QString &program);

    UpdatesModel *m_model;
    PkgFilterProxy *m_listProxy;
    PkgFilterProxy *m_kernelProxy;
    ProcessRunner *m_runner = nullptr;

    QVector<cachy::Pkg> m_collect; // accumulator during a check
    QStringList m_warnings;
    QString m_checkDbPath;

    bool m_busy = false;
    QString m_statusText = QStringLiteral("Ready.");
    QString m_statusState = QStringLiteral("idle");
    QString m_lastChecked;
    QString m_stage = QStringLiteral("Idle");
    qreal m_progress = 0.0;

    // Apply run state.
    Mode m_mode = Mode::Apply;
    QVector<cachy::Source> m_runQueue;
    int m_runIndex = 0;
    bool m_running = false;
};
