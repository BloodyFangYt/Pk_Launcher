#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QListWidget>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QProgressBar>
#include <QSystemTrayIcon>

class LauncherCore;
class InstanceManager;
class AuthManager;
class HomePage;
class PlayPage;
class InstancesPage;
class VersionsPage;
class AuthPage;
class ModsPage;
class ServersPage;
class WorldsPage;
class CosmeticsPage;
class NewsPage;
#include "ui/pages/SettingsPage.h"
class DownloadManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(LauncherCore* launcherCore, InstanceManager* instanceManager, AuthManager* authManager, QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;

private:
    void setupUI();
    void setupNavigation();
    void setupStatusBar();
    void setupTrayIcon();
    void applyTheme();

    void navigateTo(int index);
    void onDownloadProgress(const QString& file, qint64 current, qint64 total);
    void onLaunchStarted();
    void onLaunchFinished(bool success, const QString& error);

    LauncherCore* m_launcherCore;
    InstanceManager* m_instanceManager;
    AuthManager* m_authManager;
    QStackedWidget* m_stackedWidget;
    QListWidget* m_navList;
    QToolBar* m_toolBar;
    QStatusBar* m_statusBar;
    QLabel* m_statusLabel;
    QProgressBar* m_progressBar;
    QSystemTrayIcon* m_trayIcon;

    // Pages
    HomePage* m_homePage = nullptr;
    PlayPage* m_playPage = nullptr;
    InstancesPage* m_instancesPage = nullptr;
    VersionsPage* m_versionsPage = nullptr;
    AuthPage* m_authPage = nullptr;
    ModsPage* m_modsPage = nullptr;
    ServersPage* m_serversPage = nullptr;
    WorldsPage* m_worldsPage = nullptr;
    CosmeticsPage* m_cosmeticsPage = nullptr;
    NewsPage* m_newsPage = nullptr;
    SettingsPage* m_settingsPage = nullptr;
    DownloadManager* m_downloadManager = nullptr;

    struct NavItem {
        QString label;
        QString icon;
        int pageIndex;
    };
    QVector<NavItem> m_navItems;
};