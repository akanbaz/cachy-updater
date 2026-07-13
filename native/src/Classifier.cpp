#include "Classifier.h"

#include <QRegularExpression>
#include <QSet>

namespace cachy::classifier {

namespace {

const QSet<QString> kCriticalExact = {
    "linux", "linux-lts", "linux-zen", "linux-hardened",
    "linux-cachyos", "linux-cachyos-lto", "linux-cachyos-bore",
    "linux-firmware", "systemd", "systemd-libs", "glibc", "pacman",
    "mesa", "nvidia", "nvidia-dkms", "nvidia-utils", "filesystem",
    "bash", "coreutils",
};

const QStringList kCriticalPrefixes = {
    "linux-cachyos", "linux-firmware", "nvidia-", "mesa-", "systemd-",
    "glibc", "gcc-", "amd-ucode", "intel-ucode",
};

const QStringList kNoticePrefixes = {
    "plasma-", "kf6-", "qt6-", "pipewire", "wireplumber", "firefox", "chromium",
};

const QSet<QString> kKernelNames = {
    "linux", "linux-lts", "linux-zen", "linux-hardened", "linux-rt",
};

bool startsWithAny(const QString &name, const QStringList &prefixes)
{
    for (const QString &p : prefixes) {
        if (name.startsWith(p))
            return true;
    }
    return false;
}

struct Version {
    bool ok = false;
    int major = 0;
    int minor = 0;
    int patch = 0;
};

Version parseVersion(const QString &raw)
{
    Version v;
    // Strip epoch and pkgrel: (epoch:)?main(-rel)?
    static const QRegularExpression re(
        QStringLiteral("^(?:\\d+:)?([^-]+?)(?:-.+)?$"));
    const QRegularExpressionMatch m = re.match(raw.trimmed());
    if (!m.hasMatch())
        return v;
    const QString main = m.captured(1);
    static const QRegularExpression num(QStringLiteral("\\d+"));
    QRegularExpressionMatchIterator it = num.globalMatch(main);
    QList<int> nums;
    while (it.hasNext())
        nums << it.next().captured(0).toInt();
    if (nums.isEmpty())
        return v;
    v.ok = true;
    v.major = nums.value(0, 0);
    v.minor = nums.value(1, 0);
    v.patch = nums.value(2, 0);
    return v;
}

} // namespace

bool isKernel(const QString &name, Source source)
{
    if (source != Source::Repo)
        return false;
    const QString n = name.toLower();
    if (kKernelNames.contains(n))
        return true;
    return n.startsWith("linux-cachyos") || n.startsWith("linux-rt-");
}

bool isBootableKernelPackage(const QString &name)
{
    if (!isKernel(name, Source::Repo))
        return false;
    const QString n = name.toLower();
    return !n.contains(QStringLiteral("-headers"))
           && !n.contains(QStringLiteral("api-headers"));
}

QString versionBump(const QString &oldVer, const QString &newVer)
{
    const Version o = parseVersion(oldVer);
    const Version n = parseVersion(newVer);
    if (!o.ok || !n.ok)
        return QStringLiteral("unknown");
    if (o.major != n.major)
        return QStringLiteral("major");
    if (o.minor != n.minor)
        return QStringLiteral("minor");
    return QStringLiteral("patch");
}

Severity classify(const Pkg &pkg)
{
    const QString name = pkg.name.toLower();
    if (kCriticalExact.contains(name) || startsWithAny(name, kCriticalPrefixes))
        return Severity::Critical;
    if (pkg.groups.contains("base") || pkg.groups.contains("base-devel"))
        return Severity::Important;
    const QString bump = versionBump(pkg.oldVersion, pkg.newVersion);
    if (bump == "major")
        return Severity::Important;
    if (startsWithAny(name, kNoticePrefixes))
        return Severity::Notice;
    if (bump == "minor")
        return Severity::Notice;
    return Severity::Routine;
}

QString buildSummary(const Pkg &pkg)
{
    QStringList bits;
    const QString bump = versionBump(pkg.oldVersion, pkg.newVersion);
    if (bump == "major")
        bits << QStringLiteral("Major version bump \u2014 review notes if this is a core package.");
    else if (bump == "minor")
        bits << QStringLiteral("Minor version update with likely feature or API changes.");
    else if (bump == "patch")
        bits << QStringLiteral("Patch/release update \u2014 typically fixes and small improvements.");
    else
        bits << QStringLiteral("Version update available.");

    if (pkg.severity == Severity::Critical)
        bits << QStringLiteral("Marked important: may need a reboot or affect the running system.");

    const QString desc = pkg.description.trimmed();
    if (!desc.isEmpty())
        bits << (desc.endsWith('.') ? desc : desc + QLatin1Char('.'));
    else
        bits << QStringLiteral("No package description was available from the repositories.");

    if (pkg.source == Source::Aur)
        bits << QStringLiteral("Source: AUR (built locally via paru).");
    else if (pkg.source == Source::Flatpak)
        bits << QStringLiteral("Source: Flatpak (%1).")
                    .arg(pkg.flatpakId.isEmpty() ? pkg.name : pkg.flatpakId);
    else if (!pkg.repo.isEmpty())
        bits << QStringLiteral("Repository: %1.").arg(pkg.repo);

    if (pkg.sizeBytes > 0)
        bits << QStringLiteral("Size: %1.").arg(formatBytes(pkg.sizeBytes));

    return bits.join(QLatin1Char(' '));
}

} // namespace cachy::classifier
