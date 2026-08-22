#include "ui/MainWindow.h"
#include "ui/pages/HomePage.h"
#include "ui/pages/PlayPage.h"
#include "ui/pages/InstancesPage.h"
#include "ui/pages/VersionsPage.h"
#include "ui/pages/AuthPage.h"
#include "ui/pages/Pages.h"
#include "ui/DownloadManager.h"
#include "launcher/LauncherCore.h"
#include "launcher/InstanceManager.h"
#include "core/AuthManager.h"
#include "core/Settings.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QProgressBar>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QCloseEvent>
#include <QStyle>
#include <QIcon>
#include <QDebug>
#include <QCoreApplication>

MainWindow::MainWindow(LauncherCore* launcherCore, InstanceManager* instanceManager, AuthManager* authManager, QWidget* parent)
    : QMainWindow(parent), m_launcherCore(launcherCore), m_instanceManager(instanceManager), m_authManager(authManager)
{
    setupUI();
    setupNavigation();
    setupStatusBar();
    setupTrayIcon();
    applyTheme();

    connect(m_launcherCore, &LauncherCore::downloadProgress, this,
            [this](const DownloadProgress& progress) {
                onDownloadProgress(progress.currentFile, progress.bytesDownloaded,
                                    progress.totalBytes);
            });
    connect(m_launcherCore, &LauncherCore::launchStarted,
            this, &MainWindow::onLaunchStarted);
    connect(m_launcherCore, &LauncherCore::launchFinished,
            this, &MainWindow::onLaunchFinished);

    navigateTo(0); // Home
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    setWindowTitle("PK Launcher");
    resize(1200, 800);
    setMinimumSize(900, 600);

    // Central widget with horizontal layout
    QWidget* central = new QWidget(this);
    QHBoxLayout* mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Navigation sidebar
    m_navList = new QListWidget();
    m_navList->setFixedWidth(260);
    m_navList->setFrameShape(QFrame::NoFrame);
    m_navList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_navList->setStyleSheet(R"(
        QListWidget {
            background-color: #1c1b1b;
            border-right: 1px solid #353534;
            outline: none;
        }
        QListWidget::item {
            padding: 12px 16px;
            color: #c8c6c5;
            font-family: 'JetBrains Mono';
            font-size: 12px;
            border-left: 2px solid transparent;
        }
        QListWidget::item:selected {
            background-color: rgba(255, 0, 51, 0.05);
            color: #FF0033;
            border-left: 2px solid #FF0033;
            font-weight: bold;
        }
        QListWidget::item:hover:!selected {
            background-color: rgba(255, 255, 255, 0.05);
        }
    )");

    m_navItems = {
        {"Home", "🏠", 0},
        {"Play", "▶️", 1},
        {"Instances", "📦", 2},
        {"Versions", "🕐", 3},
        {"Auth", "🔐", 4},
        {"Mods", "🧩", 5},
        {"Servers", "🌐", 6},
        {"Worlds", "🌍", 7},
        {"Cosmetics", "👕", 8},
        {"News", "📰", 9},
        {"Settings", "⚙️", 10}
    };

    for (const auto& item : m_navItems) {
        QListWidgetItem* listItem = new QListWidgetItem(item.icon + "  " + item.label);
        listItem->setData(Qt::UserRole, item.pageIndex);
        m_navList->addItem(listItem);
    }

    connect(m_navList, &QListWidget::currentRowChanged, this, &MainWindow::navigateTo);

    // Stacked widget for pages
    m_stackedWidget = new QStackedWidget();
    m_stackedWidget->setStyleSheet("background-color: #131313; background-image: url(":/images/background.png");");

    // Create pages
    m_homePage = new HomePage(m_launcherCore);
    m_playPage = new PlayPage(m_launcherCore, m_instanceManager);
    m_instancesPage = new InstancesPage(m_launcherCore, m_instanceManager);
    m_versionsPage = new VersionsPage(m_launcherCore);
    m_authPage = new AuthPage(m_authManager);
    m_modsPage = new ModsPage(m_launcherCore);
    m_serversPage = new ServersPage(m_launcherCore);
    m_worldsPage = new WorldsPage(m_launcherCore);
    m_cosmeticsPage = new CosmeticsPage(m_launcherCore);
    m_newsPage = new NewsPage(m_launcherCore);
    m_settingsPage = new SettingsPage(m_launcherCore);
    m_downloadManager = new DownloadManager(m_launcherCore);

    m_stackedWidget->addWidget(m_homePage);
    m_stackedWidget->addWidget(m_playPage);
    m_stackedWidget->addWidget(m_instancesPage);
    m_stackedWidget->addWidget(m_versionsPage);
    m_stackedWidget->addWidget(m_authPage);
    m_stackedWidget->addWidget(m_modsPage);
    m_stackedWidget->addWidget(m_serversPage);
    m_stackedWidget->addWidget(m_worldsPage);
    m_stackedWidget->addWidget(m_cosmeticsPage);
    m_stackedWidget->addWidget(m_newsPage);
    m_stackedWidget->addWidget(m_settingsPage);
    m_stackedWidget->addWidget(m_downloadManager);

    mainLayout->addWidget(m_navList);
    mainLayout->addWidget(m_stackedWidget, 1);

    setCentralWidget(central);

    // Window drag region
    setAttribute(Qt::WA_TranslucentBackground, false);
}

void MainWindow::setupNavigation()
{
    // Top toolbar
    m_toolBar = new QToolBar();
    m_toolBar->setFixedHeight(40);
    m_toolBar->setMovable(false);
    m_toolBar->setFloatable(false);
    m_toolBar->setStyleSheet(R"(
        QToolBar {
            background-color: #131313;
            border-bottom: 1px solid #262626;
            spacing: 8px;
            padding: 0 16px;
        }
        QToolButton {
            background: transparent;
            border: none;
            color: #c8c6c5;
            padding: 6px 12px;
            border-radius: 4px;
        }
        QToolButton:hover {
            background-color: #353534;
        }
        QToolButton:pressed {
            background-color: #FF0033;
            color: white;
        }
    )");

    // Title
    QLabel* title = new QLabel("PK Launcher");
    title->setStyleSheet("color: #ffb3af; font-weight: bold; font-size: 14px;");
    m_toolBar->addWidget(title);

    m_toolBar->addSeparator();

    // Window controls
    QAction* minimizeAction = new QAction(style()->standardIcon(QStyle::SP_TitleBarMinButton), "", this);
    QAction* maximizeAction = new QAction(style()->standardIcon(QStyle::SP_TitleBarMaxButton), "", this);
    QAction* closeAction = new QAction(style()->standardIcon(QStyle::SP_TitleBarCloseButton), "", this);

    connect(minimizeAction, &QAction::triggered, this, &QWidget::showMinimized);
    connect(maximizeAction, &QAction::triggered, this, [this]() {
        isMaximized() ? showNormal() : showMaximized();
    });
    connect(closeAction, &QAction::triggered, this, &QWidget::close);

    m_toolBar->addAction(minimizeAction);
    m_toolBar->addAction(maximizeAction);
    m_toolBar->addAction(closeAction);

    addToolBar(Qt::TopToolBarArea, m_toolBar);
}

void MainWindow::setupStatusBar()
{
    m_statusBar = new QStatusBar();
    m_statusBar->setFixedHeight(28);
    m_statusBar->setStyleSheet(R"(
        QStatusBar {
            background-color: #1c1b1b;
            border-top: 1px solid #353534;
            color: #c8c6c5;
            font-family: 'JetBrains Mono';
            font-size: 10px;
        }
    )");

    m_statusLabel = new QLabel("Ready");
    m_progressBar = new QProgressBar();
    m_progressBar->setFixedSize(200, 16);
    m_progressBar->setTextVisible(false);
    m_progressBar->setStyleSheet(R"(
        QProgressBar {
            background: #353534;
            border: none;
            border-radius: 3px;
        }
        QProgressBar::chunk {
            background: #FF0033;
            border-radius: 3px;
        }
    )");
    m_progressBar->hide();

    m_statusBar->addWidget(m_statusLabel, 1);
    m_statusBar->addPermanentWidget(m_progressBar);
    setStatusBar(m_statusBar);
}

void MainWindow::setupTrayIcon()
{
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(":/images/icon.png"));
    m_trayIcon->setToolTip("PK Launcher");

    QMenu* trayMenu = new QMenu(this);
    trayMenu->addAction("Show", this, &QWidget::showNormal);
    trayMenu->addAction("Quit", qApp, &QCoreApplication::quit);
    m_trayIcon->setContextMenu(trayMenu);

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            showNormal();
            raise();
            activateWindow();
        }
    });

    m_trayIcon->show();
}

void MainWindow::applyTheme()
{
    QString theme = Settings::instance().theme();
    QString accent = Settings::instance().accentColor();

    QString styleSheet = QString(R"(
        QMainWindow {
            background-color: #0e0e0e;
        }
        QWidget {
            color: #e5e2e1;
            font-family: 'Inter', system-ui, sans-serif;
        }
        QScrollBar:vertical {
            background: #131313;
            width: 8px;
            border: none;
        }
        QScrollBar::handle:vertical {
            background: #353534;
            border-radius: 4px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background: #474746;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }
    )");

    if (theme == "dark") {
        styleSheet += R"(
            QLineEdit, QTextEdit, QPlainTextEdit {
                background: #1c1b1b;
                border: 1px solid #262626;
                border-radius: 4px;
                padding: 8px;
                color: #e5e2e1;
            }
            QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus {
                border-color: )" + accent + R"(;
            }
            QPushButton {
                background: )" + accent + R"(;
                color: white;
                border: none;
                border-radius: 4px;
                padding: 10px 20px;
                font-weight: bold;
            }
            QPushButton:hover {
                background: #cc0029;
            }
            QPushButton:disabled {
                background: #353534;
                color: #474746;
            }
            QGroupBox {
                border: 1px solid #262626;
                border-radius: 8px;
                margin-top: 16px;
                padding-top: 16px;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 16px;
                padding: 0 8px;
                color: #c8c6c5;
            }
        )";
    }

    setStyleSheet(styleSheet);
}

void MainWindow::navigateTo(int index)
{
    if (index >= 0 && index < m_stackedWidget->count()) {
        m_stackedWidget->setCurrentIndex(index);
        m_navList->setCurrentRow(index);
    }
}

void MainWindow::onDownloadProgress(const QString& file, qint64 current, qint64 total)
{
    m_progressBar->show();
    m_progressBar->setMaximum(total);
    m_progressBar->setValue(current);
    m_statusLabel->setText(QString("Downloading: %1 (%2/%3)")
                           .arg(file).arg(current).arg(total));
}

void MainWindow::onLaunchStarted()
{
    m_statusLabel->setText("Launching game...");
    m_progressBar->hide();
}

void MainWindow::onLaunchFinished(bool success, const QString& error)
{
    if (success) {
        m_statusLabel->setText("Game launched successfully");
    } else {
        m_statusLabel->setText("Launch failed: " + error);
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (Settings::instance().closeOnLaunch()) {
        hide();
        event->ignore();
    } else {
        m_trayIcon->hide();
        event->accept();
    }
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized() && Settings::instance().closeOnLaunch()) {
            hide();
            event->ignore();
            return;
        }
    }
    QMainWindow::changeEvent(event);
}