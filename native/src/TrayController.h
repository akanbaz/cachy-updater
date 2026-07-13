#pragma once

#include <QObject>

class QAction;
class QMenu;
class QSystemTrayIcon;
class QTimer;
class UpdateController;

// System tray applet: periodic background checks, status icon, and a menu to
// open the main window or trigger a manual check.
class TrayController : public QObject
{
    Q_OBJECT
public:
    explicit TrayController(UpdateController *updater, QObject *parent = nullptr);
    ~TrayController() override;

    bool available() const;
    void show();

public slots:
    void checkNow();
    void openWindow();

signals:
    void quitRequested();

private slots:
    void onUpdatesChanged();
    void onCheckFinished();

private:
    void updateAppearance();
    void buildMenu();

    UpdateController *m_updater;
    QSystemTrayIcon *m_tray = nullptr;
    QMenu *m_menu = nullptr;
    QAction *m_openAction = nullptr;
    QAction *m_checkAction = nullptr;
    QAction *m_quitAction = nullptr;
    QTimer *m_timer = nullptr;
    int m_lastCount = -1;
};
