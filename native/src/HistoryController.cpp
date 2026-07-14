#include "HistoryController.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

HistoryModel::HistoryModel(QObject *parent) : QAbstractListModel(parent) {}

int HistoryModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant HistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return {};
    const HistoryEntry &e = m_items.at(index.row());
    switch (role) {
    case TimestampRole: return e.timestamp;
    case ActionRole: return e.action;
    case DetailRole: return e.detail;
    case SuccessRole: return e.success;
    default: return {};
    }
}

QHash<int, QByteArray> HistoryModel::roleNames() const
{
    return {
        {TimestampRole, "timestamp"},
        {ActionRole, "action"},
        {DetailRole, "detail"},
        {SuccessRole, "success"},
    };
}

void HistoryModel::setItems(const QVector<HistoryEntry> &items)
{
    beginResetModel();
    m_items = items;
    endResetModel();
}

void HistoryModel::prepend(const HistoryEntry &entry)
{
    beginInsertRows({}, 0, 0);
    m_items.prepend(entry);
    endInsertRows();
}

void HistoryModel::trimTo(int maxEntries)
{
    if (maxEntries < 0 || m_items.size() <= maxEntries)
        return;
    const int first = maxEntries;
    const int last = m_items.size() - 1;
    beginRemoveRows({}, first, last);
    m_items.resize(maxEntries);
    endRemoveRows();
}

// ---------------------------------------------------------------------------

HistoryController::HistoryController(QObject *parent)
    : QObject(parent)
    , m_model(new HistoryModel(this))
{
    reload();
}

QObject *HistoryController::historyModelObject() const { return m_model; }

QString HistoryController::logPath() const
{
    const QString dir =
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/history.json");
}

void HistoryController::reload()
{
    QFile f(logPath());
    if (!f.open(QIODevice::ReadOnly))
        return;

    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isArray())
        return;

    QVector<HistoryEntry> items;
    const QJsonArray arr = doc.array();
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        HistoryEntry e;
        e.timestamp = o.value(QStringLiteral("timestamp")).toString();
        e.action = o.value(QStringLiteral("action")).toString();
        e.detail = o.value(QStringLiteral("detail")).toString();
        e.success = o.value(QStringLiteral("success")).toBool(true);
        items << e;
    }
    m_model->setItems(items);
    emit changed();
}

void HistoryController::record(const QString &action, const QString &detail, bool success)
{
    HistoryEntry e;
    // ISO-8601 with offset so entries stay correct if the user changes timezone.
    e.timestamp = QDateTime::currentDateTime().toOffsetFromUtc(
                                     QDateTime::currentDateTime().offsetFromUtc())
                         .toString(Qt::ISODate);
    e.action = action;
    e.detail = detail;
    e.success = success;
    m_model->prepend(e);
    m_model->trimTo(kMaxEntries);
    persist();
    emit changed();
}

void HistoryController::clear()
{
    m_model->setItems({});
    persist();
    emit changed();
}

void HistoryController::persist()
{
    QJsonArray arr;
    const int n = qMin(m_model->rowCount(), kMaxEntries);
    for (int i = 0; i < n; ++i) {
        const QModelIndex idx = m_model->index(i, 0);
        QJsonObject o;
        o.insert(QStringLiteral("timestamp"), idx.data(HistoryModel::TimestampRole).toString());
        o.insert(QStringLiteral("action"), idx.data(HistoryModel::ActionRole).toString());
        o.insert(QStringLiteral("detail"), idx.data(HistoryModel::DetailRole).toString());
        o.insert(QStringLiteral("success"), idx.data(HistoryModel::SuccessRole).toBool());
        arr.append(o);
    }

    QFile f(logPath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    f.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
