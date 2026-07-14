#include "FwupdController.h"
#include "HistoryController.h"
#include "MaintainController.h"
#include "NewsController.h"
#include "SettingsController.h"
#include "TrayController.h"
#include "UpdateController.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QIcon>
#include <QLockFile>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <QTimer>
#include <memory>

static std::unique_ptr<QLockFile> g_trayLock;

static bool acquireTrayLock()
{
    const QString path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                         + QStringLiteral("/cachyos-updater-tray.lock");
    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));

    g_trayLock = std::make_unique<QLockFile>(path);
    g_trayLock->setStaleLockTime(0);
    return g_trayLock->tryLock(100);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("cachyos-updater"));
    app.setApplicationVersion(QStringLiteral("1.1.0"));
    app.setOrganizationName(QStringLiteral("CachyOS"));
    app.setDesktopFileName(QStringLiteral("org.cachyos.updater"));
    {
        // Prefer the embedded brand mark — theme may still have an old /usr icon cached.
        QIcon appIcon(QStringLiteral(":/tray/assets/logo.svg"));
        if (appIcon.isNull())
            appIcon = QIcon(QStringLiteral(":/tray/assets/tray-uptodate.svg"));
        if (appIcon.isNull())
            appIcon = QIcon::fromTheme(QStringLiteral("org.cachyos.updater-tray"));
        if (appIcon.isNull())
            appIcon = QIcon::fromTheme(QStringLiteral("org.cachyos.updater"));
        app.setWindowIcon(appIcon);
    }

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("CachyOS native system updater"));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption trayOption(QStringList{QStringLiteral("tray")},
                                        QStringLiteral("Run as a background system tray applet"));
    const QCommandLineOption checkOption(QStringList{QStringLiteral("check")},
                                         QStringLiteral("Check for updates and exit"));
    const QCommandLineOption notifyOption(QStringList{QStringLiteral("notify")},
                                          QStringLiteral("Send a tray notification if updates exist"));
    parser.addOption(trayOption);
    parser.addOption(checkOption);
    parser.addOption(notifyOption);
    parser.process(app);

    if (QQuickStyle::name().isEmpty())
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));

    SettingsController settings;
    HistoryController history;
    UpdateController updater(&settings, &history);
    NewsController news;
    MaintainController maintain(&settings);
    FwupdController firmware;

    updater.setNewsGateChecker([&news, &settings](QString *text) {
        return news.checkArchGate(&settings, text);
    });
    QObject::connect(&news, &NewsController::changed, &updater,
                     &UpdateController::refreshSafety);

    if (parser.isSet(checkOption)) {
        QObject::connect(&updater, &UpdateController::checkFinished, &app, [&]() {
            const int count = updater.packageCount();
            if (parser.isSet(notifyOption) && count > 0
                && QSystemTrayIcon::isSystemTrayAvailable()) {
                QSystemTrayIcon tray;
                QIcon notifyIcon = QIcon::fromTheme(QStringLiteral("org.cachyos.updater-tray-updates"));
                if (notifyIcon.isNull())
                    notifyIcon = QIcon(QStringLiteral(":/tray/assets/tray-updates.svg"));
                if (notifyIcon.isNull())
                    notifyIcon = app.windowIcon();
                tray.setIcon(notifyIcon);
                tray.show();
                tray.showMessage(QStringLiteral("Cachy Updater"),
                                 count == 1 ? QStringLiteral("1 update available")
                                            : QStringLiteral("%1 updates available").arg(count),
                                 QSystemTrayIcon::Information, 6000);
            }
            QTimer::singleShot(500, &app, [&app, count]() {
                app.exit(count > 0 ? 100 : 0);
            });
        });
        updater.check();
        return app.exec();
    }

    if (parser.isSet(trayOption)) {
        if (!acquireTrayLock())
            return 0;

        TrayController tray(&updater, &settings);
        if (!tray.available())
            return 1;

        tray.show();
        return app.exec();
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("Settings"), &settings);
    engine.rootContext()->setContextProperty(QStringLiteral("History"), &history);
    engine.rootContext()->setContextProperty(QStringLiteral("Updater"), &updater);
    engine.rootContext()->setContextProperty(QStringLiteral("News"), &news);
    engine.rootContext()->setContextProperty(QStringLiteral("Maintain"), &maintain);
    engine.rootContext()->setContextProperty(QStringLiteral("Firmware"), &firmware);
    engine.rootContext()->setContextProperty(
        QStringLiteral("StartTab"), settings.defaultTab());
    engine.rootContext()->setContextProperty(
        QStringLiteral("AppVersion"), app.applicationVersion());
    engine.loadFromModule("org.cachyos.updater", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;

    if (settings.autoCheckOnStartup())
        updater.check();

    return app.exec();
}
