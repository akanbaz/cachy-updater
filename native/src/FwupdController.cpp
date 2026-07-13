#include "FwupdController.h"

#include "ProcessRunner.h"

#include <QRegularExpression>
#include <QStandardPaths>

FirmwareModel::FirmwareModel(QObject *parent) : QAbstractListModel(parent) {}

int FirmwareModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant FirmwareModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return {};
    const FirmwareDevice &d = m_items.at(index.row());
    switch (role) {
    case DeviceIdRole: return d.deviceId;
    case NameRole: return d.name;
    case VersionRole: return d.version;
    case NewVersionRole: return d.newVersion;
    case SummaryRole: return d.summary;
    default: return {};
    }
}

QHash<int, QByteArray> FirmwareModel::roleNames() const
{
    return {
        {DeviceIdRole, "deviceId"},
        {NameRole, "name"},
        {VersionRole, "version"},
        {NewVersionRole, "newVersion"},
        {SummaryRole, "summary"},
    };
}

void FirmwareModel::setItems(const QVector<FirmwareDevice> &items)
{
    beginResetModel();
    m_items = items;
    endResetModel();
}

// ---------------------------------------------------------------------------

FwupdController::FwupdController(QObject *parent)
    : QObject(parent)
    , m_model(new FirmwareModel(this))
{
    m_available = !which(QStringLiteral("fwupdmgr")).isEmpty();
}

QObject *FwupdController::firmwareModelObject() const { return m_model; }

QString FwupdController::which(const QString &program)
{
    return QStandardPaths::findExecutable(program);
}

void FwupdController::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit busyChanged();
}

void FwupdController::emitLine(const QString &text, const QString &kind)
{
    emit lineEmitted(text, kind);
}

void FwupdController::scan()
{
    if (m_busy)
        return;
    m_warnings.clear();
    m_available = !which(QStringLiteral("fwupdmgr")).isEmpty();
    if (!m_available) {
        m_warnings << QStringLiteral("fwupdmgr not found (install fwupd).");
        m_model->setItems({});
        emit changed();
        return;
    }

    setBusy(true);
    emitLine(QStringLiteral("$ fwupdmgr get-updates"), QStringLiteral("cmd"));
    auto *r = new ProcessRunner(this);
    r->setMerged(true);
    r->setTimeout(120000);
    connect(r, &ProcessRunner::line, this, [this](const QString &l) { emitLine(l); });
    connect(r, &ProcessRunner::finished, this, [this, r](int code, const QString &out) {
        r->deleteLater();
        if (code != 0 && code != 2)
            m_warnings << QStringLiteral("fwupdmgr get-updates failed.");
        parseUpdates(out);
        setBusy(false);
        emit changed();
    });
    connect(r, &ProcessRunner::failed, this, [this, r](const QString &err) {
        r->deleteLater();
        m_warnings << err;
        setBusy(false);
        emit changed();
    });
    r->start(QStringLiteral("fwupdmgr"), {QStringLiteral("get-updates")});
}

void FwupdController::parseUpdates(const QString &out)
{
    QVector<FirmwareDevice> items;
    FirmwareDevice cur;
    static const QRegularExpression idRe(QStringLiteral("^\\s*Device ID:\\s*(.+)$"));
    static const QRegularExpression nameRe(QStringLiteral("^\\s*Summary:\\s*(.+)$"));
    static const QRegularExpression verRe(QStringLiteral("^\\s*Current version:\\s*(.+)$"));
    static const QRegularExpression newRe(QStringLiteral("^\\s*New version:\\s*(.+)$"));

    auto flush = [&]() {
        if (!cur.deviceId.isEmpty())
            items << cur;
        cur = {};
    };

    const QList<QStringView> lines = QStringView(out).split(QLatin1Char('\n'));
    for (const QStringView &lv : lines) {
        const QString line = lv.toString();
        if (line.trimmed().isEmpty()) {
            flush();
            continue;
        }
        QRegularExpressionMatch m;
        if ((m = idRe.match(line)).hasMatch()) {
            flush();
            cur.deviceId = m.captured(1).trimmed();
        } else if ((m = nameRe.match(line)).hasMatch()) {
            cur.name = m.captured(1).trimmed();
            cur.summary = cur.name;
        } else if ((m = verRe.match(line)).hasMatch()) {
            cur.version = m.captured(1).trimmed();
        } else if ((m = newRe.match(line)).hasMatch()) {
            cur.newVersion = m.captured(1).trimmed();
        }
    }
    flush();
    m_model->setItems(items);
}

void FwupdController::updateAll()
{
    if (m_busy || m_model->rowCount() == 0)
        return;
    if (which(QStringLiteral("pkexec")).isEmpty()) {
        emitLine(QStringLiteral("pkexec required for firmware updates."), QStringLiteral("error"));
        return;
    }
    setBusy(true);
    emitLine(QStringLiteral("$ pkexec fwupdmgr update"), QStringLiteral("cmd"));
    auto *r = new ProcessRunner(this);
    r->setMerged(true);
    r->setTimeout(0);
    connect(r, &ProcessRunner::line, this, [this](const QString &l) { emitLine(l); });
    connect(r, &ProcessRunner::finished, this, [this, r](int code, const QString &) {
        r->deleteLater();
        if (code == 0)
            emitLine(QStringLiteral("Firmware update complete."), QStringLiteral("ok"));
        setBusy(false);
        scan();
    });
    connect(r, &ProcessRunner::failed, this, [this, r](const QString &err) {
        r->deleteLater();
        emitLine(err, QStringLiteral("error"));
        setBusy(false);
    });
    r->start(QStringLiteral("pkexec"), {QStringLiteral("fwupdmgr"), QStringLiteral("update")});
}

void FwupdController::updateDevice(const QString &deviceId)
{
    if (m_busy || deviceId.trimmed().isEmpty())
        return;
    if (which(QStringLiteral("pkexec")).isEmpty()) {
        emitLine(QStringLiteral("pkexec required for firmware updates."), QStringLiteral("error"));
        return;
    }
    setBusy(true);
    emitLine(QStringLiteral("$ pkexec fwupdmgr install %1").arg(deviceId), QStringLiteral("cmd"));
    auto *r = new ProcessRunner(this);
    r->setMerged(true);
    r->setTimeout(0);
    connect(r, &ProcessRunner::line, this, [this](const QString &l) { emitLine(l); });
    connect(r, &ProcessRunner::finished, this, [this, r](int code, const QString &) {
        r->deleteLater();
        if (code == 0)
            emitLine(QStringLiteral("Firmware update complete."), QStringLiteral("ok"));
        setBusy(false);
        scan();
    });
    connect(r, &ProcessRunner::failed, this, [this, r](const QString &err) {
        r->deleteLater();
        emitLine(err, QStringLiteral("error"));
        setBusy(false);
    });
    r->start(QStringLiteral("pkexec"),
             {QStringLiteral("fwupdmgr"), QStringLiteral("install"), deviceId});
}
