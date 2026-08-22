#include "ui/pages/PlayPage.h"
#include "launcher/LauncherCore.h"
#include "launcher/InstanceManager.h"
#include "core/Settings.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QTextEdit>
#include <QGroupBox>
#include <QFrame>
#include <QScrollArea>
#include <QScrollBar>
#include <QDebug>

PlayPage::PlayPage(LauncherCore* launcherCore, InstanceManager* instanceManager, QWidget* parent)
    : QWidget(parent), m_launcherCore(launcherCore), m_instanceManager(instanceManager)
{
    setupUI();

    connect(m_instanceManager, &InstanceManager::instanceCreated, this, &PlayPage::onInstanceListChanged);
    connect(m_instanceManager, &InstanceManager::instanceDeleted, this, &PlayPage::onInstanceListChanged);
    connect(m_instanceManager, &InstanceManager::instanceUpdated, this, &PlayPage::onInstanceListChanged);

    connect(m_launcherCore, &LauncherCore::launchStarted, this, &PlayPage::onLaunchStarted);
    connect(m_launcherCore, &LauncherCore::launchFinished, this, &PlayPage::onLaunchFinished);
    connect(m_launcherCore, &LauncherCore::logMessage, this, &PlayPage::onLogMessage);

    connect(m_launchButton, &QPushButton::clicked, this, &PlayPage::onLaunchClicked);

    refreshInstanceList();
}

void PlayPage::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(32, 32, 32, 32);
    mainLayout->setSpacing(24);

    // Header section
    QLabel* pageTitle = new QLabel("Play");
    pageTitle->setStyleSheet("color: #e5e2e1; font-size: 28px; font-weight: bold; font-family: 'Inter';");
    mainLayout->addWidget(pageTitle);

    QLabel* pageSubtitle = new QLabel("Select an instance and launch the game");
    pageSubtitle->setStyleSheet("color: #c8c6c5; font-size: 14px;");
    mainLayout->addWidget(pageSubtitle);

    // Instance selection card
    m_instanceGroup = new QGroupBox("Instance");
    m_instanceGroup->setStyleSheet(R"(
        QGroupBox {
            border: 1px solid #262626;
            border-radius: 8px;
            margin-top: 16px;
            padding-top: 16px;
            background: #1c1b1b;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 16px;
            padding: 0 8px;
            color: #c8c6c5;
            font-size: 14px;
            font-weight: bold;
        }
    )");

    QVBoxLayout* instanceLayout = new QVBoxLayout(m_instanceGroup);
    instanceLayout->setContentsMargins(16, 24, 16, 16);
    instanceLayout->setSpacing(16);

    // Instance combo box
    QHBoxLayout* comboLayout = new QHBoxLayout();
    comboLayout->setSpacing(12);

    QLabel* instanceLabel = new QLabel("Instance:");
    instanceLabel->setStyleSheet("color: #e5e2e1; font-size: 14px; min-width: 80px;");
    instanceLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    comboLayout->addWidget(instanceLabel);

    m_instanceCombo = new QComboBox();
    m_instanceCombo->setStyleSheet(R"(
        QComboBox {
            background: #131313;
            border: 1px solid #262626;
            border-radius: 4px;
            padding: 10px 16px;
            color: #e5e2e1;
            font-family: 'JetBrains Mono';
            font-size: 13px;
            min-width: 300px;
        }
        QComboBox:hover {
            border-color: #353534;
        }
        QComboBox:focus {
            border-color: #FF0033;
        }
        QComboBox::drop-down {
            border: none;
            width: 24px;
        }
        QComboBox::down-arrow {
            image: none;
            border-left: 5px solid transparent;
            border-right: 5px solid transparent;
            border-top: 6px solid #c8c6c5;
            margin-right: 10px;
        }
        QComboBox QAbstractItemView {
            background: #1c1b1b;
            border: 1px solid #262626;
            border-radius: 4px;
            selection-background-color: #FF0033;
            color: #e5e2e1;
            outline: none;
        }
    )");
    comboLayout->addWidget(m_instanceCombo, 1);

    QPushButton* refreshButton = new QPushButton("Refresh");
    refreshButton->setStyleSheet(R"(
        QPushButton {
            background: #353534;
            color: #e5e2e1;
            border: 1px solid #262626;
            border-radius: 4px;
            padding: 10px 20px;
            font-weight: bold;
        }
        QPushButton:hover {
            background: #474746;
            border-color: #353534;
        }
        QPushButton:pressed {
            background: #FF0033;
            color: white;
            border-color: #FF0033;
        }
    )");
    connect(refreshButton, &QPushButton::clicked, this, &PlayPage::refreshInstanceList);
    comboLayout->addWidget(refreshButton);

    instanceLayout->addLayout(comboLayout);

    // Version display
    QHBoxLayout* versionLayout = new QHBoxLayout();
    versionLayout->setSpacing(12);

    QLabel* versionLabelTitle = new QLabel("Version:");
    versionLabelTitle->setStyleSheet("color: #c8c6c5; font-size: 13px; min-width: 80px;");
    versionLabelTitle->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    versionLayout->addWidget(versionLabelTitle);

    m_versionLabel = new QLabel("Select an instance");
    m_versionLabel->setStyleSheet("color: #e5e2e1; font-size: 13px; font-family: 'JetBrains Mono';");
    versionLayout->addWidget(m_versionLabel);
    versionLayout->addStretch();

    instanceLayout->addLayout(versionLayout);
    mainLayout->addWidget(m_instanceGroup);

    // Launch button and progress
    QFrame* launchFrame = new QFrame();
    launchFrame->setStyleSheet("background: transparent; border: none;");
    QVBoxLayout* launchLayout = new QVBoxLayout(launchFrame);
    launchLayout->setContentsMargins(0, 0, 0, 0);
    launchLayout->setSpacing(12);

    m_launchButton = new QPushButton("LAUNCH GAME");
    m_launchButton->setFixedHeight(56);
    m_launchButton->setCursor(Qt::PointingHandCursor);
    m_launchButton->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #FF0033, stop:1 #cc0029);
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 16px;
            font-weight: bold;
            font-family: 'Inter';
            letter-spacing: 0.5px;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #cc0029, stop:1 #b30024);
        }
        QPushButton:pressed {
            background: #b30024;
        }
        QPushButton:disabled {
            background: #353534;
            color: #474746;
        }
    )");
    m_launchButton->setEnabled(false);
    launchLayout->addWidget(m_launchButton);

    m_progressBar = new QProgressBar();
    m_progressBar->setFixedHeight(6);
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
    launchLayout->addWidget(m_progressBar);

    mainLayout->addWidget(launchFrame);

    // Log output area
    QGroupBox* logGroup = new QGroupBox("Launch Log");
    logGroup->setStyleSheet(R"(
        QGroupBox {
            border: 1px solid #262626;
            border-radius: 8px;
            margin-top: 16px;
            padding-top: 16px;
            background: #1c1b1b;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 16px;
            padding: 0 8px;
            color: #c8c6c5;
            font-size: 14px;
            font-weight: bold;
        }
    )");

    QVBoxLayout* logLayout = new QVBoxLayout(logGroup);
    logLayout->setContentsMargins(12, 20, 12, 12);
    logLayout->setSpacing(8);

    m_logOutput = new QTextEdit();
    m_logOutput->setReadOnly(true);
    m_logOutput->setStyleSheet(R"(
        QTextEdit {
            background: #131313;
            border: 1px solid #262626;
            border-radius: 4px;
            color: #c8c6c5;
            font-family: 'JetBrains Mono';
            font-size: 11px;
            padding: 8px;
        }
    )");
    m_logOutput->setMaximumHeight(200);
    logLayout->addWidget(m_logOutput);

    QHBoxLayout* logButtonLayout = new QHBoxLayout();
    logButtonLayout->addStretch();

    QPushButton* clearLogButton = new QPushButton("Clear");
    clearLogButton->setStyleSheet(R"(
        QPushButton {
            background: transparent;
            color: #c8c6c5;
            border: 1px solid #262626;
            border-radius: 4px;
            padding: 6px 16px;
            font-size: 11px;
        }
        QPushButton:hover {
            background: #353534;
            border-color: #353534;
        }
    )");
    connect(clearLogButton, &QPushButton::clicked, m_logOutput, &QTextEdit::clear);
    logButtonLayout->addWidget(clearLogButton);

    logLayout->addLayout(logButtonLayout);
    mainLayout->addWidget(logGroup, 1);

    mainLayout->addStretch();

    // Connect instance selection change
    connect(m_instanceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        if (index >= 0) {
            QString version = m_instanceCombo->itemData(index, Qt::UserRole).toString();
            m_versionLabel->setText(version.isEmpty() ? "Unknown" : version);
            m_launchButton->setEnabled(true);
        } else {
            m_versionLabel->setText("Select an instance");
            m_launchButton->setEnabled(false);
        }
    });
}

void PlayPage::refreshInstanceList()
{
    m_instanceCombo->clear();
    m_instanceCombo->addItem("Select an instance...", "");
    m_instanceCombo->setItemData(0, QVariant(), Qt::UserRole - 1); // Disable first item

    QList<InstanceInfo> instances = m_instanceManager->listInstances();
    for (const auto& instance : instances) {
        QString displayText = QString("%1 (%2)").arg(instance.name, instance.version);
        m_instanceCombo->addItem(displayText, instance.version);
        m_instanceCombo->setItemData(m_instanceCombo->count() - 1, instance.id, Qt::UserRole + 1);
    }

    if (instances.isEmpty()) {
        m_versionLabel->setText("No instances created yet");
        m_launchButton->setEnabled(false);
        m_logOutput->append("[INFO] No instances found. Create one in the Instances page.");
    }
}

void PlayPage::onInstanceListChanged()
{
    QString currentId = m_instanceCombo->currentData(Qt::UserRole + 1).toString();
    refreshInstanceList();

    // Try to restore selection
    for (int i = 0; i < m_instanceCombo->count(); ++i) {
        if (m_instanceCombo->itemData(i, Qt::UserRole + 1).toString() == currentId) {
            m_instanceCombo->setCurrentIndex(i);
            break;
        }
    }
}

void PlayPage::onLaunchClicked()
{
    int index = m_instanceCombo->currentIndex();
    if (index <= 0) {
        m_logOutput->append("[ERROR] Please select an instance first");
        return;
    }

    QString instanceId = m_instanceCombo->itemData(index, Qt::UserRole + 1).toString();
    InstanceInfo instance = m_instanceManager->getInstance(instanceId);

    if (instance.id.isEmpty()) {
        m_logOutput->append("[ERROR] Instance not found");
        return;
    }

    m_logOutput->append(QString("[INFO] Preparing to launch: %1 (%2)").arg(instance.name, instance.version));
    m_logOutput->append(QString("[INFO] Game directory: %1").arg(instance.gameDir));
    m_logOutput->append(QString("[INFO] RAM: %1 MB").arg(instance.ramMb));

    m_launchButton->setEnabled(false);
    m_progressBar->show();
    m_progressBar->setRange(0, 0); // Indeterminate

    // Build launch config
    LaunchConfig config;
    config.username = "Player"; // TODO: Get from auth
    config.uuid = "00000000-0000-0000-0000-000000000000"; // TODO: Get from auth
    config.version = instance.version;
    config.gameDir = instance.gameDir;
    config.javaPath = Settings::instance().javaAutoDetect();
    config.jvmArgs = instance.jvmArgs.split(' ', Qt::SkipEmptyParts);
    config.resolution = QSize(854, 480);

    // Use LauncherCore to launch
    bool launched = m_launcherCore->launchGame(config);
    if (!launched) {
        m_logOutput->append("[ERROR] Failed to start launch process");
        m_launchButton->setEnabled(true);
        m_progressBar->hide();
    }
}

void PlayPage::onLaunchStarted()
{
    m_logOutput->append("[INFO] Game process started");
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(50);
}

void PlayPage::onLaunchFinished(bool success, const QString& error)
{
    m_launchButton->setEnabled(true);
    m_progressBar->hide();

    if (success) {
        m_logOutput->append("[SUCCESS] Game launched successfully");
    } else {
        m_logOutput->append("[ERROR] Launch failed: " + error);
    }
}

void PlayPage::onLogMessage(const QString& message)
{
    m_logOutput->append(message);
    m_logOutput->verticalScrollBar()->setValue(m_logOutput->verticalScrollBar()->maximum());
}