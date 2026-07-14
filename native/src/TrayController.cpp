#include "TrayController.h"
#include "SettingsController.h"
#include "UpdateController.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QIcon>
#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <QProcess>
#include <QSystemTrayIcon>
#include <QTimer>

namespace {

QIcon trayIconFor(bool hasUpdates)
{
    const QString themeName =
        hasUpdates ? QStringLiteral("org.cachyos.updater-tray-updates")
                   : QStringLiteral("org.cachyos.updater-tray");
    const QString resourceName =
        hasUpdates ? QStringLiteral("tray-updates.svg")
                   : QStringLiteral("tray-uptodate.svg");

    auto sizedIcon = [](const QIcon &source) -> QIcon {
        if (source.isNull())
            return {};
        QIcon out;
        for (const int size : {16, 22, 32, 48, 64}) {
            const QPixmap px = source.pixmap(size, size);
            if (!px.isNull())
                out.addPixmap(px);
        }
        return out;
    };

    QIcon icon = sizedIcon(QIcon(QStringLiteral(":/tray/assets/%1").arg(resourceName)));
    if (icon.isNull())
        icon = sizedIcon(QIcon::fromTheme(themeName));
    if (!icon.isNull())
        return icon;

    if (hasUpdates)
        return sizedIcon(QIcon::fromTheme(QStringLiteral("cachy-update_updates-available-blue")));
    return sizedIcon(QIcon::fromTheme(QStringLiteral("cachy-update-blue")));
}

} // namespace

TrayController::TrayController(UpdateController *updater, SettingsController *settings,
                               QObject *parent)
    : QObject(parent)
    , m_updater(updater)
    , m_settings(settings)
{
    m_tray = new QSystemTrayIcon(this);
    m_tray->setToolTip(QStringLiteral("Cachy Updater"));
    connect(m_tray, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
                if (reason == QSystemTrayIcon::Trigger
                    || reason == QSystemTrayIcon::DoubleClick)
                    openWindow();
                else if (reason == QSystemTrayIcon::MiddleClick)
                    checkNow();
            });

    buildMenu();
    m_tray->setContextMenu(m_menu);

    connect(m_updater, &UpdateController::updatesChanged, this,
            &TrayController::onUpdatesChanged);
    connect(m_updater, &UpdateController::checkFinished, this,
            &TrayController::onCheckFinished);
    connect(m_updater, &UpdateController::busyChanged, this,
            &TrayController::updateAppearance);
    connect(m_updater, &UpdateController::stageChanged, this,
            &TrayController::onStageChanged);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &TrayController::checkNow);
    if (m_settings)
        connect(m_settings, &SettingsController::settingsChanged, this, [this]() {
            m_timer->setInterval(m_settings->trayIntervalMinutes() * 60 * 1000);
        });

    m_cacheDebounce = new QTimer(this);
    m_cacheDebounce->setSingleShot(true);
    m_cacheDebounce->setInterval(200);
    connect(m_cacheDebounce, &QTimer::timeout, this, &TrayController::onCacheChanged);

    m_cacheWatcher = new QFileSystemWatcher(this);
    connect(m_cacheWatcher, &QFileSystemWatcher::fileChanged, this,
            [this](const QString &) { m_cacheDebounce->start(); });
    connect(m_cacheWatcher, &QFileSystemWatcher::directoryChanged, this,
            [this](const QString &dir) {
                const QString file = UpdateController::cacheFilePath();
                if (QFile::exists(file) && !m_cacheWatcher->files().contains(file))
                    m_cacheWatcher->addPath(file);
                Q_UNUSED(dir)
            });
}

TrayController::~TrayController()
{
    // QSystemTrayIcon does not take ownership of its context menu.
    delete m_menu;
}

bool TrayController::available() const
{
    return QSystemTrayIcon::isSystemTrayAvailable();
}

void TrayController::watchCheckCache()
{
    const QString file = UpdateController::cacheFilePath();
    const QString dir = QFileInfo(file).absolutePath();
    if (QFile::exists(file))
        m_cacheWatcher->addPath(file);
    else if (QDir(dir).exists())
        m_cacheWatcher->addPath(dir);
}

void TrayController::show()
{
    if (m_settings)
        m_timer->setInterval(m_settings->trayIntervalMinutes() * 60 * 1000);

    watchCheckCache();
    m_updater->seedFromCache();
    updateAppearance();
    m_tray->show();
    m_timer->start();
    if (!m_settings || m_settings->autoCheckOnStartup())
        QTimer::singleShot(2500, this, &TrayController::checkNow);
}

void TrayController::onCacheChanged()
{
    m_updater->seedFromCache();
    updateAppearance();
    watchCheckCache();
}

void TrayController::buildMenu()
{
    m_menu = new QMenu;
    m_openAction = m_menu->addAction(QStringLiteral("Open Cachy Updater"), this,
                                     &TrayController::openWindow);
    m_checkAction = m_menu->addAction(QStringLiteral("Check for updates"), this,
                                      &TrayController::checkNow);
    m_applyAction = m_menu->addAction(QStringLiteral("Apply all updates"), this,
                                      &TrayController::applyAll);
    m_menu->addSeparator();
    m_quitAction = m_menu->addAction(QStringLiteral("Quit"), this, [this]() {
        emit quitRequested();
        QApplication::quit();
    });
    Q_UNUSED(m_openAction)
    Q_UNUSED(m_checkAction)
    Q_UNUSED(m_applyAction)
    Q_UNUSED(m_quitAction)
}

void TrayController::openWindow()
{
    QProcess::startDetached(QApplication::applicationFilePath(), {});
}

void TrayController::checkNow()
{
    if (!m_updater->busy())
        m_updater->check();
}

void TrayController::applyAll()
{
    if (m_updater->busy())
        return;
    const int count = m_updater->packageCount();
    if (count <= 0)
        return;

    const auto reply = QMessageBox::question(
        nullptr, QStringLiteral("Apply all updates?"),
        count == 1 ? QStringLiteral("Apply 1 pending update now?")
                   : QStringLiteral("Apply all %1 pending updates now?").arg(count),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reply != QMessageBox::Yes)
        return;

    m_updater->setAllSelected(true);
    m_updater->apply();
}

bool TrayController::shouldNotify(int count) const
{
    if (count <= 0)
        return false;
    if (!m_settings || !m_settings->notifyCriticalOnly())
        return true;
    return m_updater->criticalCount() > 0;
}

void TrayController::onUpdatesChanged()
{
    updateAppearance();
}

void TrayController::onCheckFinished()
{
    const int count = m_updater->packageCount();
    if (m_lastCount >= 0 && count > 0 && count != m_lastCount && shouldNotify(count)) {
        const int crit = m_updater->criticalCount();
        QString body = count == 1 ? QStringLiteral("1 update available")
                                  : QStringLiteral("%1 updates available").arg(count);
        if (crit > 0)
            body += QStringLiteral(" (%1 important)").arg(crit);
        m_tray->showMessage(QStringLiteral("Cachy Updater"), body,
                            QSystemTrayIcon::Information, 8000);
    }
    m_lastCount = count;
    updateAppearance();
}

void TrayController::onStageChanged()
{
    updateAppearance();
}

void TrayController::updateAppearance()
{
    const int count = m_updater->packageCount();
    const bool busy = m_updater->busy();
    QString tip;

    if (busy) {
        tip = QStringLiteral("Cachy Updater — %1 (%2%)")
                  .arg(m_updater->stage())
                  .arg(qRound(m_updater->progress() * 100));
    } else if (count > 0) {
        tip = count == 1 ? QStringLiteral("Cachy Updater — 1 update available")
                         : QStringLiteral("Cachy Updater — %1 updates available").arg(count);
    } else {
        tip = QStringLiteral("Cachy Updater — up to date");
    }

    const QIcon icon = busy ? trayIconFor(false) : trayIconFor(count > 0);
    if (!icon.isNull())
        m_tray->setIcon(icon);
    m_tray->setToolTip(tip);
    if (m_applyAction)
        m_applyAction->setEnabled(!busy && count > 0);
}
