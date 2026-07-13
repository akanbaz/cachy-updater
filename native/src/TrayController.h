#pragma once

#include <QObject>

class QMenu;
class QSystemTrayIcon;
class QAction;
class QTimer;
class SettingsController;
class UpdateController;

class TrayController : public QObject
{
    Q_OBJECT
public:
    explicit TrayController(UpdateController *updater,
                            SettingsController *settings = nullptr,
                            QObject *parent = nullptr);
    ~TrayController() override;

    bool available() const;
    void show();

public slots:
    void checkNow();
    void openWindow();
    void applyAll();

signals:
    void quitRequested();

private slots:
    void onUpdatesChanged();
    void onCheckFinished();
    void onStageChanged();

private:
    void updateAppearance();
    void buildMenu();
    bool shouldNotify(int count) const;

    UpdateController *m_updater;
    SettingsController *m_settings;
    QSystemTrayIcon *m_tray = nullptr;
    QMenu *m_menu = nullptr;
    QAction *m_openAction = nullptr;
    QAction *m_checkAction = nullptr;
    QAction *m_applyAction = nullptr;
    QAction *m_quitAction = nullptr;
    QTimer *m_timer = nullptr;
    int m_lastCount = -1;
};
