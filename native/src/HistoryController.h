#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QVector>

struct HistoryEntry {
    QString timestamp;
    QString action;
    QString detail;
    bool success = true;
};

class HistoryModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        TimestampRole = Qt::UserRole + 1,
        ActionRole,
        DetailRole,
        SuccessRole,
    };
    explicit HistoryModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setItems(const QVector<HistoryEntry> &items);
    void prepend(const HistoryEntry &entry);

private:
    QVector<HistoryEntry> m_items;
};

// Append-only log of update and maintenance actions.
class HistoryController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(QObject *historyModel READ historyModelObject CONSTANT)

public:
    explicit HistoryController(QObject *parent = nullptr);

    int count() const { return m_model->rowCount(); }
    QObject *historyModelObject() const;

    Q_INVOKABLE void record(const QString &action, const QString &detail, bool success);
    Q_INVOKABLE void clear();
    Q_INVOKABLE void reload();

signals:
    void changed();

private:
    void persist();
    QString logPath() const;

    HistoryModel *m_model;
    static constexpr int kMaxEntries = 200;
};
