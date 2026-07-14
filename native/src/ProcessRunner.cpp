#include "ProcessRunner.h"

ProcessRunner::ProcessRunner(QObject *parent) : QObject(parent)
{
    m_timer.setSingleShot(true);

    connect(&m_proc, &QProcess::readyReadStandardOutput, this,
            &ProcessRunner::handleReadyRead);
    connect(&m_proc, &QProcess::readyReadStandardError, this,
            &ProcessRunner::handleReadyRead);

    connect(&m_proc,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus status) {
                if (m_settled)
                    return;
                m_settled = true;
                m_timer.stop();
                handleReadyRead();
                emitPartialAsLine();
                if (status == QProcess::CrashExit) {
                    emit failed(QStringLiteral("Process crashed: %1")
                                    .arg(m_proc.program()));
                    return;
                }
                emit finished(code, m_accumulated);
            });

    connect(&m_proc, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError error) {
                if (m_settled)
                    return;
                if (error == QProcess::FailedToStart) {
                    m_settled = true;
                    m_timer.stop();
                    emit failed(QStringLiteral("Failed to start: %1")
                                    .arg(m_proc.program()));
                }
            });

    connect(&m_timer, &QTimer::timeout, this, [this]() {
        if (m_settled)
            return;
        m_settled = true;
        m_proc.kill();
        emit failed(QStringLiteral("Timed out after %1s: %2")
                        .arg(m_timeoutMs / 1000)
                        .arg(m_proc.program()));
    });
}

void ProcessRunner::start(const QString &program, const QStringList &args,
                          const QProcessEnvironment &env)
{
    m_settled = false;
    m_accumulated.clear();
    m_partial.clear();

    m_proc.setProcessChannelMode(m_merged ? QProcess::MergedChannels
                                          : QProcess::SeparateChannels);
    if (!env.isEmpty())
        m_proc.setProcessEnvironment(env);

    if (m_timeoutMs > 0)
        m_timer.start(m_timeoutMs);

    m_proc.setProgram(program);
    m_proc.setArguments(args);
    m_proc.start();
}

void ProcessRunner::stop()
{
    if (isRunning()) {
        m_proc.terminate();
        if (!m_proc.waitForFinished(1500))
            m_proc.kill();
    }
}

void ProcessRunner::stopAll(QObject *root)
{
    if (!root)
        return;
    const auto runners = root->findChildren<ProcessRunner *>();
    for (ProcessRunner *r : runners)
        r->stop();
    if (auto *self = qobject_cast<ProcessRunner *>(root))
        self->stop();
}

void ProcessRunner::trimAccumulated()
{
    if (m_accumulated.size() <= kMaxAccumulatedChars)
        return;
    m_accumulated = m_accumulated.right(kMaxAccumulatedChars / 2);
}

void ProcessRunner::handleReadyRead()
{
    QByteArray chunk = m_proc.readAllStandardOutput();
    if (!m_merged)
        chunk += m_proc.readAllStandardError();
    if (chunk.isEmpty())
        return;

    const QString text = QString::fromUtf8(chunk);
    m_accumulated += text;
    trimAccumulated();
    m_partial += text;

    int idx;
    while ((idx = m_partial.indexOf(QLatin1Char('\n'))) >= 0) {
        QString ln = m_partial.left(idx);
        if (ln.endsWith(QLatin1Char('\r')))
            ln.chop(1);
        m_partial.remove(0, idx + 1);
        emit line(ln);
    }
}

void ProcessRunner::emitPartialAsLine()
{
    if (!m_partial.isEmpty()) {
        emit line(m_partial);
        m_partial.clear();
    }
}
