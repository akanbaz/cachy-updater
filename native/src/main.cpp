#include "MaintainController.h"
#include "NewsController.h"
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
    app.setApplicationVersion(QStringLiteral("1.0.0"));
    app.setOrganizationName(QStringLiteral("CachyOS"));
    app.setDesktopFileName(QStringLiteral("org.cachyos.updater"));
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("system-software-update")));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("CachyOS native system updater"));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption trayOption(QStringList{QStringLiteral("tray")},
                                        QStringLiteral("Run as a background system tray applet"));
    parser.addOption(trayOption);
    parser.process(app);

    if (QQuickStyle::name().isEmpty())
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));

    UpdateController updater;
    NewsController news;
    MaintainController maintain;

    if (parser.isSet(trayOption)) {
        if (!acquireTrayLock())
            return 0;

        TrayController tray(&updater);
        if (!tray.available())
            return 1;

        tray.show();
        return app.exec();
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("Updater"), &updater);
    engine.rootContext()->setContextProperty(QStringLiteral("News"), &news);
    engine.rootContext()->setContextProperty(QStringLiteral("Maintain"), &maintain);
    engine.rootContext()->setContextProperty(
        QStringLiteral("StartTab"), qEnvironmentVariableIntValue("CACHYOS_TAB"));
    engine.rootContext()->setContextProperty(
        QStringLiteral("AppVersion"), app.applicationVersion());
    engine.loadFromModule("org.cachyos.updater", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
