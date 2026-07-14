#include "UpdatesModel.h"

using namespace cachy;

namespace {
QString severityLabel(Severity s)
{
    switch (s) {
    case Severity::Critical: return QStringLiteral("Important");
    case Severity::Important: return QStringLiteral("Notable");
    case Severity::Notice: return QStringLiteral("Notice");
    case Severity::Routine: return QString();
    }
    return QString();
}
}

UpdatesModel::UpdatesModel(QObject *parent) : QAbstractListModel(parent) {}

int UpdatesModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant UpdatesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return {};
    const Pkg &p = m_items.at(index.row());
    switch (role) {
    case NameRole: return p.name;
    case OldVersionRole: return p.oldVersion;
    case NewVersionRole: return p.newVersion;
    case SourceRole: return sourceKey(p.source);
    case RepoRole: return p.repo;
    case SizeTextRole: return formatBytes(p.sizeBytes);
    case SizeBytesRole: return static_cast<qlonglong>(p.sizeBytes);
    case SeverityRole: return static_cast<int>(p.severity);
    case SeverityLabelRole: return severityLabel(p.severity);
    case IsKernelRole: return p.kernel;
    case RunningKernelRole: return p.runningKernel;
    case SummaryRole: return p.summary;
    case ChangelogRole: return p.changelog;
    case CommandRole: return sourceCommand(p.source);
    case SelectedRole: return p.selected;
    case HeldRole: return p.held;
    case FlatpakKindRole: return p.flatpakKind;
    case LockedGroupRole:
        return p.source == Source::Repo && !p.pkgbase.isEmpty()
               && m_lockedBases.contains(p.pkgbase);
    default: return {};
    }
}

bool UpdatesModel::setData(const QModelIndex &index, const QVariant &value,
                           int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return false;
    if (role == SelectedRole) {
        Pkg &target = m_items[index.row()];
        if (target.held)
            return false;
        const bool sel = value.toBool();

        // Version-locked split packages (same pkgbase) toggle as one unit, so
        // the selection can never produce a broken partial upgrade.
        const bool locked = target.source == Source::Repo
                            && m_lockedBases.contains(target.pkgbase);
        if (!locked) {
            if (target.selected == sel)
                return true;
            target.selected = sel;
            emit dataChanged(index, index, {SelectedRole});
            emit selectionChanged();
            return true;
        }

        int lo = -1, hi = -1;
        bool any = false;
        for (int i = 0; i < m_items.size(); ++i) {
            Pkg &p = m_items[i];
            if (p.source != Source::Repo || p.pkgbase != target.pkgbase || p.held)
                continue;
            if (p.selected != sel) {
                p.selected = sel;
                any = true;
            }
            lo = (lo < 0) ? i : qMin(lo, i);
            hi = qMax(hi, i);
        }
        if (any) {
            emit dataChanged(this->index(lo), this->index(hi), {SelectedRole});
            emit selectionChanged();
        }
        return true;
    }
    return false;
}

Qt::ItemFlags UpdatesModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

QHash<int, QByteArray> UpdatesModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {OldVersionRole, "oldVersion"},
        {NewVersionRole, "newVersion"},
        {SourceRole, "source"},
        {RepoRole, "repo"},
        {SizeTextRole, "sizeText"},
        {SizeBytesRole, "sizeBytes"},
        {SeverityRole, "severity"},
        {SeverityLabelRole, "severityLabel"},
        {IsKernelRole, "isKernel"},
        {RunningKernelRole, "runningKernel"},
        {SummaryRole, "summary"},
        {ChangelogRole, "changelog"},
        {CommandRole, "command"},
        {SelectedRole, "selected"},
        {HeldRole, "held"},
        {FlatpakKindRole, "flatpakKind"},
        {LockedGroupRole, "lockedGroup"},
    };
}

void UpdatesModel::setItems(const QVector<Pkg> &items)
{
    beginResetModel();
    m_items = items;
    recomputeLockedBases();
    endResetModel();
    emit selectionChanged();
}

void UpdatesModel::recomputeLockedBases()
{
    QHash<QString, int> counts;
    for (const Pkg &p : m_items)
        if (p.source == Source::Repo && !p.pkgbase.isEmpty())
            ++counts[p.pkgbase];
    m_lockedBases.clear();
    for (auto it = counts.cbegin(); it != counts.cend(); ++it)
        if (it.value() > 1)
            m_lockedBases.insert(it.key());
}

bool UpdatesModel::enforceLockedGroups()
{
    // If any member of a locked group is selected, select all its (unheld)
    // members so a split package never ends up half-upgraded.
    QSet<QString> selectedBases;
    for (const Pkg &p : m_items)
        if (p.source == Source::Repo && p.selected
            && m_lockedBases.contains(p.pkgbase))
            selectedBases.insert(p.pkgbase);
    bool changed = false;
    for (Pkg &p : m_items)
        if (p.source == Source::Repo && !p.held && !p.selected
            && selectedBases.contains(p.pkgbase)) {
            p.selected = true;
            changed = true;
        }
    return changed;
}

void UpdatesModel::setAllSelected(bool selected)
{
    if (m_items.isEmpty())
        return;
    bool any = false;
    for (Pkg &p : m_items) {
        if (p.held)
            continue;
        if (p.selected != selected) {
            p.selected = selected;
            any = true;
        }
    }
    if (any) {
        emit dataChanged(index(0), index(m_items.size() - 1), {SelectedRole});
        emit selectionChanged();
    }
}

void UpdatesModel::selectSource(Source source, bool selected)
{
    bool any = false;
    for (int i = 0; i < m_items.size(); ++i) {
        Pkg &p = m_items[i];
        if (p.source != source || p.held)
            continue;
        if (p.selected != selected) {
            p.selected = selected;
            any = true;
        }
    }
    if (any) {
        emit dataChanged(index(0), index(m_items.size() - 1), {SelectedRole});
        emit selectionChanged();
    }
}

void UpdatesModel::selectBySeverity(int minSeverity)
{
    bool any = false;
    for (Pkg &p : m_items) {
        if (p.held)
            continue;
        const bool want = static_cast<int>(p.severity) >= minSeverity;
        if (p.selected != want) {
            p.selected = want;
            any = true;
        }
    }
    // A locked group whose members span severities must not be split.
    any = enforceLockedGroups() || any;
    if (any) {
        emit dataChanged(index(0), index(m_items.size() - 1), {SelectedRole});
        emit selectionChanged();
    }
}

int UpdatesModel::selectedCount() const
{
    int n = 0;
    for (const Pkg &p : m_items)
        if (p.selected)
            ++n;
    return n;
}

int UpdatesModel::selectedCountFor(Source source) const
{
    int n = 0;
    for (const Pkg &p : m_items)
        if (p.source == source && p.selected)
            ++n;
    return n;
}

// ---------------------------------------------------------------------------

PkgFilterProxy::PkgFilterProxy(bool wantKernel, QObject *parent)
    : QSortFilterProxyModel(parent), m_wantKernel(wantKernel)
{
    setDynamicSortFilter(true);
    sort(0);
}

bool PkgFilterProxy::filterAcceptsRow(int sourceRow,
                                      const QModelIndex &sourceParent) const
{
    const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
    return idx.data(UpdatesModel::IsKernelRole).toBool() == m_wantKernel;
}

bool PkgFilterProxy::lessThan(const QModelIndex &left,
                              const QModelIndex &right) const
{
    const auto orderFor = [](const QString &k) {
        if (k == QLatin1String("aur"))
            return Source::Aur;
        if (k == QLatin1String("flatpak"))
            return Source::Flatpak;
        return Source::Repo;
    };
    const int lo = sourceOrder(orderFor(left.data(UpdatesModel::SourceRole).toString()));
    const int ro = sourceOrder(orderFor(right.data(UpdatesModel::SourceRole).toString()));
    if (lo != ro)
        return lo < ro;
    const QString lk = left.data(UpdatesModel::FlatpakKindRole).toString();
    const QString rk = right.data(UpdatesModel::FlatpakKindRole).toString();
    if (lk != rk)
        return lk < rk;
    return left.data(UpdatesModel::NameRole).toString()
        < right.data(UpdatesModel::NameRole).toString();
}

// ---------------------------------------------------------------------------

UpdateListProxy::UpdateListProxy(QObject *parent)
    : PkgFilterProxy(false, parent)
{}

void UpdateListProxy::setSearchText(const QString &text)
{
    if (m_searchText == text)
        return;
    beginFilterChange();
    m_searchText = text;
    endFilterChange();
    emit filtersChanged();
}

void UpdateListProxy::setMinSeverity(int severity)
{
    severity = qBound(0, severity, 3);
    if (m_minSeverity == severity)
        return;
    beginFilterChange();
    m_minSeverity = severity;
    endFilterChange();
    emit filtersChanged();
}

void UpdateListProxy::setSourceFilter(const QString &source)
{
    if (m_sourceFilter == source)
        return;
    beginFilterChange();
    m_sourceFilter = source;
    endFilterChange();
    emit filtersChanged();
}

void UpdateListProxy::setSortMode(int mode)
{
    mode = qBound(0, mode, static_cast<int>(SortMode::Name));
    const auto next = static_cast<SortMode>(mode);
    if (m_sortMode == next)
        return;
    m_sortMode = next;
    invalidate();
    sort(0);
    emit filtersChanged();
}

bool UpdateListProxy::filterAcceptsRow(int sourceRow,
                                       const QModelIndex &sourceParent) const
{
    if (!PkgFilterProxy::filterAcceptsRow(sourceRow, sourceParent))
        return false;

    const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
    if (!m_sourceFilter.isEmpty()
        && idx.data(UpdatesModel::SourceRole).toString() != m_sourceFilter)
        return false;

    if (m_minSeverity > 0
        && idx.data(UpdatesModel::SeverityRole).toInt() < m_minSeverity)
        return false;

    if (!m_searchText.isEmpty()) {
        const QString needle = m_searchText.trimmed();
        const QString name = idx.data(UpdatesModel::NameRole).toString();
        if (!name.contains(needle, Qt::CaseInsensitive))
            return false;
    }
    return true;
}

bool UpdateListProxy::lessThan(const QModelIndex &left,
                               const QModelIndex &right) const
{
    const auto nameLess = [&] {
        return left.data(UpdatesModel::NameRole).toString()
            < right.data(UpdatesModel::NameRole).toString();
    };

    switch (m_sortMode) {
    case SortMode::Important: {
        const int ls = left.data(UpdatesModel::SeverityRole).toInt();
        const int rs = right.data(UpdatesModel::SeverityRole).toInt();
        if (ls != rs)
            return ls > rs;
        return nameLess();
    }
    case SortMode::Size: {
        const qlonglong ls = left.data(UpdatesModel::SizeBytesRole).toLongLong();
        const qlonglong rs = right.data(UpdatesModel::SizeBytesRole).toLongLong();
        if (ls != rs)
            return ls > rs;
        return nameLess();
    }
    case SortMode::Name:
        return nameLess();
    case SortMode::Default:
    default:
        return PkgFilterProxy::lessThan(left, right);
    }
}
