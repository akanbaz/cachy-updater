#pragma once

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcessEnvironment>
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

// Arch/CachyOS: /usr/lib/modules/<uname -r>/pkgbase holds the pacman package name
// of the running kernel (e.g. "linux-cachyos"). Far more reliable than substring
// matching osrelease against package names.
inline QString runningKernelPkgbase()
{
    const QString release = runningKernel();
    if (release.isEmpty())
        return {};
    QFile f(QStringLiteral("/usr/lib/modules/%1/pkgbase").arg(release));
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return QString::fromUtf8(f.readAll()).trimmed();
}

inline bool isRunningKernelPackage(const QString &packageName)
{
    const QString pkgbase = runningKernelPkgbase();
    if (pkgbase.isEmpty())
        return false;
    return packageName.compare(pkgbase, Qt::CaseInsensitive) == 0;
}

inline bool pacmanDbLocked()
{
    return QFile::exists(QStringLiteral("/var/lib/pacman/db.lck"));
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

inline QString aurHelperName(const QString &program)
{
    return QFileInfo(program).fileName();
}

inline void forceCLocale(QProcessEnvironment &env)
{
    env.insert(QStringLiteral("LC_ALL"), QStringLiteral("C"));
    env.insert(QStringLiteral("LANG"), QStringLiteral("C"));
    env.insert(QStringLiteral("LANGUAGE"), QStringLiteral("C"));
}

// Build non-interactive AUR helper args (check uses -Qua separately).
// GUI sessions have no TTY, so plain `sudo` fails with "a terminal is
// required". Drive privilege escalation through pkexec (Polkit dialog)
// the same way repo updates do.
inline QStringList aurApplyArgs(const QString &helper, bool fullUpgrade,
                                const QStringList &names = {})
{
    QStringList args;
    if (fullUpgrade)
        args << QStringLiteral("-Sua");
    else
        args << QStringLiteral("-S") << names;
    args << QStringLiteral("--noconfirm");

    const QString name = aurHelperName(helper);
    if (name == QLatin1String("paru")) {
        args << QStringLiteral("--skipreview")
             << QStringLiteral("--sudo") << QStringLiteral("pkexec")
             << QStringLiteral("--nosudoloop");
    } else if (name == QLatin1String("yay")) {
        args << QStringLiteral("--answerdiff") << QStringLiteral("None")
             << QStringLiteral("--answerclean") << QStringLiteral("None")
             << QStringLiteral("--answeredit") << QStringLiteral("None")
             << QStringLiteral("--answerupgrade") << QStringLiteral("None")
             << QStringLiteral("--sudo") << QStringLiteral("pkexec");
    } else {
        // Unknown helper: still prefer pkexec when the flag exists upstream.
        args << QStringLiteral("--sudo") << QStringLiteral("pkexec");
    }
    return args;
}

inline bool ensureUserDirWritable(const QString &path, QString *error = nullptr)
{
    QDir dir(path);
    if (!dir.exists() && !QDir().mkpath(path)) {
        if (error)
            *error = QStringLiteral("Cannot create directory: %1").arg(path);
        return false;
    }
    QFileInfo fi(path);
    if (!fi.isWritable()) {
        if (error)
            *error = QStringLiteral(
                         "Directory is not writable (check ownership): %1")
                         .arg(path);
        return false;
    }
    return true;
}

} // namespace cachy::helpers
