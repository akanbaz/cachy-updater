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
    case SummaryRole: return p.summary;
    case CommandRole: return sourceCommand(p.source);
    case SelectedRole: return p.selected;
    default: return {};
    }
}

bool UpdatesModel::setData(const QModelIndex &index, const QVariant &value,
                           int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return false;
    if (role == SelectedRole) {
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
        {SummaryRole, "summary"},
        {CommandRole, "command"},
        {SelectedRole, "selected"},
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
    const int lo = sourceOrder(static_cast<Source>(
        [](const QString &k) {
            if (k == "aur") return Source::Aur;
            if (k == "flatpak") return Source::Flatpak;
            return Source::Repo;
        }(left.data(UpdatesModel::SourceRole).toString())));
    const int ro = sourceOrder(static_cast<Source>(
        [](const QString &k) {
            if (k == "aur") return Source::Aur;
            if (k == "flatpak") return Source::Flatpak;
            return Source::Repo;
        }(right.data(UpdatesModel::SourceRole).toString())));
    if (lo != ro)
        return lo < ro;
    return left.data(UpdatesModel::NameRole).toString()
        < right.data(UpdatesModel::NameRole).toString();
}
