#pragma once

#include <QObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QString>
#include <QStringList>
#include <QTimer>

// Thin async wrapper around QProcess. Emits per-line output for streaming into
// the terminal panel, and a single finished(exitCode, fullOutput) at the end.
// The UI thread is never blocked; there are no QThreads.
class ProcessRunner : public QObject
{
    Q_OBJECT
public:
    explicit ProcessRunner(QObject *parent = nullptr);

    void setMerged(bool merged) { m_merged = merged; }
    void setTimeout(int ms) { m_timeoutMs = ms; }

    void start(const QString &program, const QStringList &args,
               const QProcessEnvironment &env = QProcessEnvironment());
    bool isRunning() const { return m_proc.state() != QProcess::NotRunning; }
    void stop();

signals:
    void line(const QString &text);
    void finished(int exitCode, const QString &output);
    void failed(const QString &error);

private:
    void handleReadyRead();
    void emitPartialAsLine();

    QProcess m_proc;
    QTimer m_timer;
    QString m_accumulated;
    QString m_partial;
    bool m_merged = true;
    int m_timeoutMs = 60000;
    bool m_settled = false;
};
