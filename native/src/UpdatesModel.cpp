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
    default: return {};
    }
}

bool UpdatesModel::setData(const QModelIndex &index, const QVariant &value,
                           int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return false;
    if (role == SelectedRole) {
        if (m_items[index.row()].held)
            return false;
        const bool sel = value.toBool();
        if (m_items[index.row()].selected == sel)
            return true;
        m_items[index.row()].selected = sel;
        emit dataChanged(index, index, {SelectedRole});
        emit selectionChanged();
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
    };
}

void UpdatesModel::setItems(const QVector<Pkg> &items)
{
    beginResetModel();
    m_items = items;
    endResetModel();
    emit selectionChanged();
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
    m_searchText = text;
    invalidateFilter();
    emit filtersChanged();
}

void UpdateListProxy::setMinSeverity(int severity)
{
    severity = qBound(0, severity, 3);
    if (m_minSeverity == severity)
        return;
    m_minSeverity = severity;
    invalidateFilter();
    emit filtersChanged();
}

void UpdateListProxy::setSourceFilter(const QString &source)
{
    if (m_sourceFilter == source)
        return;
    m_sourceFilter = source;
    invalidateFilter();
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
