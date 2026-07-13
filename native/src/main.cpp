#include "MaintainController.h"
#include "NewsController.h"
#include "UpdateController.h"

#include <QApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("cachyos-updater"));
    app.setApplicationDisplayName(QStringLiteral("Cachy Updater"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));
    app.setOrganizationName(QStringLiteral("CachyOS"));
    app.setDesktopFileName(QStringLiteral("org.cachyos.updater"));
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("system-software-update")));

    // Native Plasma look via qqc2-desktop-style when available.
    if (QQuickStyle::name().isEmpty()) {
        QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));
    }

    UpdateController updater;
    NewsController news;
    MaintainController maintain;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("Updater"), &updater);
    engine.rootContext()->setContextProperty(QStringLiteral("News"), &news);
    engine.rootContext()->setContextProperty(QStringLiteral("Maintain"), &maintain);
    engine.rootContext()->setContextProperty(
        QStringLiteral("StartTab"), qEnvironmentVariableIntValue("CACHYOS_TAB"));
    engine.rootContext()->setContextProperty(
        QStringLiteral("AppVersion"), app.applicationVersion());
    engine.loadFromModule("org.cachyos.updater", "Main");
    if (engine.rootObjects().isEmpty()) {
        return -1;
    }

    return app.exec();
}
