#include "TrayController.h"
#include "UpdateController.h"

#include <QApplication>
#include <QIcon>
#include <QMenu>
#include <QProcess>
#include <QSystemTrayIcon>
#include <QTimer>

TrayController::TrayController(UpdateController *updater, QObject *parent)
    : QObject(parent)
    , m_updater(updater)
{
    m_tray = new QSystemTrayIcon(this);
    m_tray->setToolTip(QStringLiteral("Cachy Updater"));
    connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
            openWindow();
    });

    buildMenu();
    m_tray->setContextMenu(m_menu);

    connect(m_updater, &UpdateController::updatesChanged, this, &TrayController::onUpdatesChanged);
    connect(m_updater, &UpdateController::checkFinished, this, &TrayController::onCheckFinished);

    m_timer = new QTimer(this);
    m_timer->setInterval(30 * 60 * 1000);
    connect(m_timer, &QTimer::timeout, this, &TrayController::checkNow);
}

TrayController::~TrayController() = default;

bool TrayController::available() const
{
    return QSystemTrayIcon::isSystemTrayAvailable();
}

void TrayController::show()
{
    updateAppearance();
    m_tray->show();
    m_timer->start();
    QTimer::singleShot(2500, this, &TrayController::checkNow);
}

void TrayController::buildMenu()
{
    m_menu = new QMenu;
    m_openAction = m_menu->addAction(QStringLiteral("Open Cachy Updater"), this, &TrayController::openWindow);
    m_checkAction = m_menu->addAction(QStringLiteral("Check for updates"), this, &TrayController::checkNow);
    m_menu->addSeparator();
    m_quitAction = m_menu->addAction(QStringLiteral("Quit"), this, [this]() {
        emit quitRequested();
        QApplication::quit();
    });
    Q_UNUSED(m_openAction)
    Q_UNUSED(m_checkAction)
    Q_UNUSED(m_quitAction)
}

void TrayController::openWindow()
{
    const QString program = QApplication::applicationFilePath();
    QProcess::startDetached(program, {});
}

void TrayController::checkNow()
{
    if (!m_updater->busy())
        m_updater->check();
}

void TrayController::onUpdatesChanged()
{
    updateAppearance();
}

void TrayController::onCheckFinished()
{
    const int count = m_updater->packageCount();
    if (m_lastCount >= 0 && count > 0 && count != m_lastCount)
        m_tray->showMessage(QStringLiteral("Cachy Updater"),
                            count == 1 ? QStringLiteral("1 update available")
                                       : QStringLiteral("%1 updates available").arg(count),
                            QSystemTrayIcon::Information, 6000);
    m_lastCount = count;
    updateAppearance();
}

void TrayController::updateAppearance()
{
    const int count = m_updater->packageCount();
    QIcon icon;
    QString tip;

    if (count > 0) {
        icon = QIcon::fromTheme(QStringLiteral("software-update-available"));
        if (icon.isNull())
            icon = QIcon::fromTheme(QStringLiteral("system-software-update"));
        tip = count == 1 ? QStringLiteral("Cachy Updater — 1 update available")
                         : QStringLiteral("Cachy Updater — %1 updates available").arg(count);
    } else {
        icon = QIcon::fromTheme(QStringLiteral("update-none"));
        if (icon.isNull())
            icon = QIcon::fromTheme(QStringLiteral("system-software-update"));
        tip = QStringLiteral("Cachy Updater — up to date");
    }

    if (!icon.isNull())
        m_tray->setIcon(icon);
    m_tray->setToolTip(tip);
}
