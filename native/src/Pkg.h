#pragma once

#include <QString>
#include <QStringList>
#include <QtGlobal>

namespace cachy {

enum class Source { Repo, Aur, Flatpak };
enum class Severity { Routine, Notice, Important, Critical };

struct Pkg {
    QString name;
    QString oldVersion;
    QString newVersion;
    QString repo;
    QString description;
    QString summary;
    QString flatpakId;
    Source source = Source::Repo;
    Severity severity = Severity::Routine;
    qint64 sizeBytes = 0;
    QStringList groups;
    bool selected = true;
    bool kernel = false;
};

inline QString sourceKey(Source s)
{
    switch (s) {
    case Source::Repo: return QStringLiteral("repo");
    case Source::Aur: return QStringLiteral("aur");
    case Source::Flatpak: return QStringLiteral("flatpak");
    }
    return QStringLiteral("repo");
}

inline QString sourceLabel(Source s)
{
    switch (s) {
    case Source::Repo: return QStringLiteral("REPO");
    case Source::Aur: return QStringLiteral("AUR");
    case Source::Flatpak: return QStringLiteral("FLATPAK");
    }
    return QStringLiteral("REPO");
}

inline QString sourceCommand(Source s)
{
    switch (s) {
    case Source::Repo: return QStringLiteral("pacman -Syu");
    case Source::Aur: return QStringLiteral("paru -Sua");
    case Source::Flatpak: return QStringLiteral("flatpak update");
    }
    return QString();
}

inline int sourceOrder(Source s)
{
    switch (s) {
    case Source::Repo: return 0;
    case Source::Aur: return 1;
    case Source::Flatpak: return 2;
    }
    return 3;
}

inline QString formatBytes(qint64 n)
{
    if (n <= 0)
        return QStringLiteral("\u2014"); // em dash
    const double d = static_cast<double>(n);
    if (n >= 1000000000LL)
        return QString::number(d / 1e9, 'f', 1) + QStringLiteral(" GB");
    if (n >= 1000000LL)
        return QString::number(qRound(d / 1e6)) + QStringLiteral(" MB");
    if (n >= 1000LL)
        return QString::number(qRound(d / 1e3)) + QStringLiteral(" KB");
    return QString::number(n) + QStringLiteral(" B");
}

} // namespace cachy
