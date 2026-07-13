#pragma once

#include "Pkg.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QVector>

// Flat list of all pending package updates. Grouping (by source) and the
// kernel/non-kernel split are handled by lightweight proxies so the QML views
// stay reactive on selection changes without any full rebuilds.
class UpdatesModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        OldVersionRole,
        NewVersionRole,
        SourceRole,
        RepoRole,
        SizeTextRole,
        SizeBytesRole,
        SeverityRole,
        SeverityLabelRole,
        IsKernelRole,
        SummaryRole,
        CommandRole,
        SelectedRole,
    };

    explicit UpdatesModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setItems(const QVector<cachy::Pkg> &items);
    const QVector<cachy::Pkg> &items() const { return m_items; }

    void setAllSelected(bool selected);
    int selectedCount() const;

signals:
    void selectionChanged();

private:
    QVector<cachy::Pkg> m_items;
};

// Filters by kernel flag (kernel packages become cards; the rest become the
// grouped list) and sorts by source order then name.
class PkgFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit PkgFilterProxy(bool wantKernel, QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int sourceRow,
                          const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left,
                  const QModelIndex &right) const override;

private:
    bool m_wantKernel;
};
