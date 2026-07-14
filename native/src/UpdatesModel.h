#pragma once

#include "Pkg.h"

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QVector>

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
        RunningKernelRole,
        SummaryRole,
        ChangelogRole,
        CommandRole,
        SelectedRole,
        HeldRole,
        FlatpakKindRole,
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
    void selectSource(cachy::Source source, bool selected);
    int selectedCount() const;
    int selectedCountFor(cachy::Source source) const;

signals:
    void selectionChanged();

private:
    QVector<cachy::Pkg> m_items;
};

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

// Search / severity / source filters for the non-kernel list.
class UpdateListProxy : public PkgFilterProxy
{
    Q_OBJECT
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY filtersChanged)
    Q_PROPERTY(int minSeverity READ minSeverity WRITE setMinSeverity NOTIFY filtersChanged)
    Q_PROPERTY(QString sourceFilter READ sourceFilter WRITE setSourceFilter NOTIFY filtersChanged)
    Q_PROPERTY(int sortMode READ sortMode WRITE setSortMode NOTIFY filtersChanged)

public:
    enum class SortMode { Default = 0, Important, Size, Name };
    Q_ENUM(SortMode)

    explicit UpdateListProxy(QObject *parent = nullptr);

    QString searchText() const { return m_searchText; }
    int minSeverity() const { return m_minSeverity; }
    QString sourceFilter() const { return m_sourceFilter; }
    int sortMode() const { return static_cast<int>(m_sortMode); }

    void setSearchText(const QString &text);
    void setMinSeverity(int severity);
    void setSourceFilter(const QString &source);
    void setSortMode(int mode);

signals:
    void filtersChanged();

protected:
    bool filterAcceptsRow(int sourceRow,
                          const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left,
                  const QModelIndex &right) const override;

private:
    QString m_searchText;
    int m_minSeverity = 0;
    QString m_sourceFilter;
    SortMode m_sortMode = SortMode::Default;
};
