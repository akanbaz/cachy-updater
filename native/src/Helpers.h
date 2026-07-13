#pragma once

#include <QFile>
#include <QString>
#include <QStringList>
#include <QStandardPaths>

namespace cachy::helpers {

inline QString which(const QString &program)
{
    return QStandardPaths::findExecutable(program);
}

inline QString runningKernel()
{
    QFile f(QStringLiteral("/proc/sys/kernel/osrelease"));
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return QString::fromUtf8(f.readAll()).trimmed();
}

inline bool snapshotsAvailable()
{
    return !which(QStringLiteral("snapper")).isEmpty()
           || !which(QStringLiteral("btrfs")).isEmpty();
}

inline QString aurHelper()
{
    if (!which(QStringLiteral("paru")).isEmpty())
        return QStringLiteral("paru");
    if (!which(QStringLiteral("yay")).isEmpty())
        return QStringLiteral("yay");
    return {};
}

} // namespace cachy::helpers
