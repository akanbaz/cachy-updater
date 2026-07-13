#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QVector>

struct FirmwareDevice {
    QString deviceId;
    QString name;
    QString version;
    QString newVersion;
    QString summary;
};

class FirmwareModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        DeviceIdRole = Qt::UserRole + 1,
        NameRole,
        VersionRole,
        NewVersionRole,
        SummaryRole,
    };
    explicit FirmwareModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setItems(const QVector<FirmwareDevice> &items);

private:
    QVector<FirmwareDevice> m_items;
};

// fwupdmgr integration for the Firmware tab.
class FwupdController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(QString warningText READ warningText NOTIFY changed)
    Q_PROPERTY(QObject *firmwareModel READ firmwareModelObject CONSTANT)

public:
    explicit FwupdController(QObject *parent = nullptr);

    bool busy() const { return m_busy; }
    bool available() const { return m_available; }
    int count() const { return m_model->rowCount(); }
    QString warningText() const { return m_warnings.join(QStringLiteral("  \u2022  ")); }
    QObject *firmwareModelObject() const;

    Q_INVOKABLE void scan();
    Q_INVOKABLE void updateAll();
    Q_INVOKABLE void updateDevice(const QString &deviceId);

signals:
    void busyChanged();
    void changed();
    void lineEmitted(const QString &text, const QString &kind);

private:
    void setBusy(bool busy);
    void emitLine(const QString &text, const QString &kind = QStringLiteral("out"));
    void parseUpdates(const QString &out);
    static QString which(const QString &program);

    FirmwareModel *m_model;
    QStringList m_warnings;
    bool m_busy = false;
    bool m_available = false;
};
