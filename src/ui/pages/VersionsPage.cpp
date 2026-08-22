#include "ui/pages/VersionsPage.h"
#include "launcher/LauncherCore.h"
#include "core/Settings.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QStackedWidget>
#include <QButtonGroup>
#include <QFrame>
#include <QGridLayout>
#include <QGraphicsDropShadowEffect>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDebug>

VersionsPage::VersionsPage(LauncherCore* launcherCore, QWidget* parent)
    : QWidget(parent), m_launcherCore(launcherCore)
{
    setupUI();

    connect(m_launcherCore, &LauncherCore::versionsFetched, this, &VersionsPage::onVersionsFetched);
    connect(m_launcherCore, &LauncherCore::downloadProgress, this, &VersionsPage::onDownloadProgress);
    connect(m_launcherCore, &LauncherCore::downloadComplete, this, &VersionsPage::onDownloadComplete);
    connect(m_launcherCore, &LauncherCore::downloadError, this, &VersionsPage::onDownloadError);

    connect(m_searchEdit, &QLineEdit::textChanged, this, &VersionsPage::onSearchChanged);
    connect(m_typeFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &VersionsPage::onFilterChanged);
    connect(m_refreshButton, &QPushButton::clicked, this, [this]() {
        m_launcherCore->fetchVersions(true);
    });

    // Connect tab changes
    connect(m_tabGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, &VersionsPage::onTabChanged);

    // Initial fetch
    m_launcherCore->fetchVersions(false);
}

void VersionsPage::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(32, 32, 32, 32);
    mainLayout->setSpacing(24);

    // Header section
    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(16);

    QLabel* pageTitle = new QLabel("Instances & Versions");
    pageTitle->setStyleSheet(R"(
        QLabel {
            color: #e5e2e1;
            font-size: 28px;
            font-weight: 700;
            font-family: 'Inter';
        }
    )");
    headerLayout->addWidget(pageTitle);

    headerLayout->addStretch();

    m_statusLabel = new QLabel("Loading versions...");
    m_statusLabel->setStyleSheet(R"(
        QLabel {
            color: #c8c6c5;
            font-size: 14px;
            font-family: 'Inter';
        }
    )");
    headerLayout->addWidget(m_statusLabel);

    m_refreshButton = new QPushButton("Refresh");
    m_refreshButton->setStyleSheet(R"(
        QPushButton {
            background: #353534;
            color: #e5e2e1;
            border: 1px solid #262626;
            border-radius: 4px;
            padding: 8px 16px;
            font-weight: bold;
            font-size: 12px;
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
    headerLayout->addWidget(m_refreshButton);

    mainLayout->addLayout(headerLayout);

    // Filter bar
    QGroupBox* filterGroup = new QGroupBox("Filters");
    filterGroup->setStyleSheet(R"(
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

    QHBoxLayout* filterLayout = new QHBoxLayout(filterGroup);
    filterLayout->setContentsMargins(16, 24, 16, 16);
    filterLayout->setSpacing(16);

    QLabel* searchLabel = new QLabel("Search:");
    searchLabel->setStyleSheet("color: #c8c6c5; font-size: 13px;");
    filterLayout->addWidget(searchLabel);

    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText("Search versions... (e.g. 1.20, snapshot, release)");
    m_searchEdit->setStyleSheet(R"(
        QLineEdit {
            background: #131313;
            border: 1px solid #262626;
            border-radius: 4px;
            padding: 8px 12px;
            color: #e5e2e1;
            font-family: 'JetBrains Mono';
            font-size: 13px;
        }
        QLineEdit:focus {
            border-color: #FF0033;
        }
    )");
    filterLayout->addWidget(m_searchEdit, 1);

    QLabel* typeLabel = new QLabel("Type:");
    typeLabel->setStyleSheet("color: #c8c6c5; font-size: 13px;");
    filterLayout->addWidget(typeLabel);

    m_typeFilter = new QComboBox();
    m_typeFilter->addItems({"All", "Release", "Snapshot", "Old Beta", "Old Alpha"});
    m_typeFilter->setStyleSheet(R"(
        QComboBox {
            background: #131313;
            border: 1px solid #262626;
            border-radius: 4px;
            padding: 8px 16px;
            color: #e5e2e1;
            font-family: 'JetBrains Mono';
            font-size: 13px;
            min-width: 140px;
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
    filterLayout->addWidget(m_typeFilter);

    mainLayout->addWidget(filterGroup);

    // Content area with tabs
    QWidget* contentContainer = new QWidget();
    QVBoxLayout* contentLayout = new QVBoxLayout(contentContainer);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(8);

    // Tab buttons
    QHBoxLayout* tabLayout = new QHBoxLayout();
    tabLayout->setSpacing(0);
    tabLayout->setContentsMargins(0, 0, 0, 0);

    m_tabGroup = new QButtonGroup(this);
    m_tabGroup->setExclusive(true);

    QPushButton* instancesTab = new QPushButton("Instances");
    instancesTab->setCheckable(true);
    instancesTab->setChecked(true);
    instancesTab->setFixedHeight(40);
    instancesTab->setStyleSheet(R"(
        QPushButton {
            background: transparent;
            border: none;
            border-bottom: 2px solid #262626;
            color: #c8c6c5;
            font-family: 'Inter';
            font-size: 14px;
            padding: 8px 16px;
        }
        QPushButton:checked {
            background: transparent;
            border-bottom: 2px solid #FF0033;
            color: #FF0033;
            font-weight: 600;
        }
        QPushButton:hover:!checked {
            background: rgba(255, 255, 255, 0.05);
        }
    )");
    m_tabGroup->addButton(instancesTab, 0);
    tabLayout->addWidget(instancesTab);

    QPushButton* versionsTab = new QPushButton("Versions");
    versionsTab->setCheckable(true);
    versionsTab->setFixedHeight(40);
    versionsTab->setStyleSheet(R"(
        QPushButton {
            background: transparent;
            border: none;
            border-bottom: 2px solid #262626;
            color: #c8c6c5;
            font-family: 'Inter';
            font-size: 14px;
            padding: 8px 16px;
        }
        QPushButton:checked {
            background: transparent;
            border-bottom: 2px solid #FF0033;
            color: #FF0033;
            font-weight: 600;
        }
        QPushButton:hover:!checked {
            background: rgba(255, 255, 255, 0.05);
        }
    )");
    m_tabGroup->addButton(versionsTab, 1);
    tabLayout->addWidget(versionsTab);

    contentLayout->addLayout(tabLayout);

    // Content stack
    m_contentStack = new QStackedWidget();
    contentLayout->addWidget(m_contentStack, 1);

    // Instances tab content
    m_instancesWidget = new QWidget();
    m_instancesLayout = new QVBoxLayout(m_instancesWidget);
    m_instancesLayout->setContentsMargins(16, 24, 16, 16);
    m_instancesLayout->setSpacing(24);

    QLabel* instancesTitle = new QLabel("Your Instances");
    instancesTitle->setStyleSheet(R"(
        QLabel {
            color: #e5e2e1;
            font-size: 20px;
            font-weight: 600;
            font-family: 'Inter';
        }
    )");
    m_instancesLayout->addWidget(instancesTitle);

    m_instancesGrid = new QWidget();
    m_instancesGridLayout = new QGridLayout(m_instancesGrid);
    m_instancesGridLayout->setSpacing(16);
    m_instancesLayout->addWidget(m_instancesGrid, 1);

    m_instancesLayout->addStretch();

    m_contentStack->addWidget(m_instancesWidget);

    // Versions tab content
    m_versionsWidget = new QWidget();
    m_versionsLayout = new QVBoxLayout(m_versionsWidget);
    m_versionsLayout->setContentsMargins(16, 24, 16, 16);
    m_versionsLayout->setSpacing(24);

    QLabel* versionsTitle = new QLabel("Available Versions");
    versionsTitle->setStyleSheet(R"(
        QLabel {
            color: #e5e2e1;
            font-size: 20px;
            font-weight: 600;
            font-family: 'Inter';
        }
    )");
    m_versionsLayout->addWidget(versionsTitle);

    // Version grid with scroll
    QScrollArea* versionsScroll = new QScrollArea();
    versionsScroll->setWidgetResizable(true);
    versionsScroll->setFrameShape(QFrame::NoFrame);
    versionsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    versionsScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    versionsScroll->setStyleSheet(R"(
        QScrollArea {
            background: transparent;
        }
    )");

    m_versionsGrid = new QWidget();
    m_versionsGridLayout = new QGridLayout(m_versionsGrid);
    m_versionsGridLayout->setSpacing(16);
    versionsScroll->setWidget(m_versionsGrid);
    m_versionsLayout->addWidget(versionsScroll, 1);

    m_contentStack->addWidget(m_versionsWidget);

    // Progress bar at bottom
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
    mainLayout->addWidget(m_progressBar);

    // Main content
    QWidget* mainContent = new QWidget();
    QVBoxLayout* mainContentLayout = new QVBoxLayout(mainContent);
    mainContentLayout->setContentsMargins(0, 0, 0, 0);
    mainContentLayout->setSpacing(0);
    mainContentLayout->addWidget(contentContainer);
    mainContentLayout->addWidget(m_progressBar);

    QScrollArea* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setStyleSheet(R"(
        QScrollArea {
            background: transparent;
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
    )");
    scroll->setWidget(mainContent);

    QVBoxLayout* finalLayout = new QVBoxLayout(this);
    finalLayout->setContentsMargins(0, 0, 0, 0);
    finalLayout->addWidget(scroll);
    setLayout(finalLayout);

    // Initialize instances grid with empty state
    showInstancesEmptyState();
}

void VersionsPage::onVersionsFetched(const QList<VersionEntry>& versions)
{
    m_allVersions = versions;
    m_statusLabel->setText(QString("Found %1 versions").arg(versions.size()));
    populateVersionGrid(versions);
    populateInstanceGrid();  // For now, reuse versions for instances tab
}

void VersionsPage::populateVersionGrid(const QList<VersionEntry>& versions)
{
    // Clear existing cards
    QLayoutItem* item;
    while ((item = m_versionsGridLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    // Apply filters
    QList<VersionEntry> filteredVersions;
    for (const auto& version : versions) {
        bool typeMatch = (m_currentFilter == "All") || (version.type == m_currentFilter.toLower());
        bool searchMatch = m_currentSearch.isEmpty() || version.id.contains(m_currentSearch, Qt::CaseInsensitive);

        if (typeMatch && searchMatch) {
            filteredVersions.append(version);
        }
    }

    if (filteredVersions.isEmpty()) {
        showVersionsEmptyState();
        return;
    }

    int row = 0, col = 0;
    const int maxCols = 3;

    for (const auto& version : filteredVersions) {
        QFrame* card = createVersionCard(version);
        m_versionsGridLayout->addWidget(card, row, col);

        col++;
        if (col >= maxCols) {
            col = 0;
            row++;
        }
    }
}

QFrame* VersionsPage::createVersionCard(const VersionEntry& version)
{
    QFrame* card = new QFrame();
    card->setFixedSize(300, 200);
    card->setStyleSheet(R"(
        QFrame {
            background: #1c1b1b;
            border: 1px solid #262626;
            border-radius: 8px;
        }
        QFrame:hover {
            border-color: rgba(255, 255, 255, 0.15);
            background: #1e1e1e;
        }
    )");

    // Add subtle drop shadow
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(15);
    shadow->setOffset(0, 5);
    shadow->setColor(QColor(0, 0, 0, 80));
    card->setGraphicsEffect(shadow);

    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 20, 20, 20);
    cardLayout->setSpacing(16);

    // Version header
    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(12);

    QLabel* versionLabel = new QLabel(version.id);
    versionLabel->setStyleSheet(R"(
        QLabel {
            color: #e5e2e1;
            font-size: 18px;
            font-weight: 700;
            font-family: 'Inter';
        }
    )");
    headerLayout->addWidget(versionLabel);

    headerLayout->addStretch();

    // Type badge
    QString typeDisplay = version.type;
    QColor typeColor;
    if (typeDisplay == "release") {
        typeDisplay = "Release";
        typeColor = QColor("#00FF66");
    } else if (typeDisplay == "snapshot") {
        typeDisplay = "Snapshot";
        typeColor = QColor("#FFD700");
    } else if (typeDisplay == "old_beta") {
        typeDisplay = "Old Beta";
        typeColor = QColor("#FFA500");
    } else if (typeDisplay == "old_alpha") {
        typeDisplay = "Old Alpha";
        typeColor = QColor("#FF8C00");
    } else {
        typeDisplay = typeDisplay.toUpper();
        typeColor = QColor("#c8c6c5");
    }

    QLabel* typeLabel = new QLabel(typeDisplay);
    typeLabel->setStyleSheet(QString(R"(
        QLabel {
            background: %1;
            color: white;
            border-radius: 4px;
            padding: 4px 8px;
            font-size: 10px;
            font-weight: bold;
        }
    )").arg(typeColor.name()));
    headerLayout->addWidget(typeLabel);

    cardLayout->addLayout(headerLayout);

    // Release date
    QLabel* dateLabel = new QLabel(version.releaseTime.toString("MMM d, yyyy"));
    dateLabel->setStyleSheet(R"(
        QLabel {
            color: #c8c6c5;
            font-family: 'JetBrains Mono';
            font-size: 12px;
        }
    )");
    cardLayout->addWidget(dateLabel);

    // Download button
    QPushButton* downloadBtn = new QPushButton("Download");
    downloadBtn->setProperty("versionId", version.id);
    downloadBtn->setCursor(Qt::PointingHandCursor);
    downloadBtn->setStyleSheet(R"(
        QPushButton {
            background: #FF0033;
            color: white;
            border: none;
            border-radius: 6px;
            padding: 10px 20px;
            font-weight: bold;
            font-family: 'Inter';
            font-size: 13px;
        }
        QPushButton:hover {
            background: #cc0029;
        }
        QPushButton:pressed {
            background: #b30024;
        }
    )");
    connect(downloadBtn, &QPushButton::clicked, this, &VersionsPage::onDownloadClicked);
    cardLayout->addWidget(downloadBtn);

    return card;
}

void VersionsPage::populateInstanceGrid()
{
    // Clear existing cards
    QLayoutItem* item;
    while ((item = m_instancesGridLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    // For now, show empty state
    showInstancesEmptyState();
}

void VersionsPage::filterVersions()
{
    populateVersionGrid(m_allVersions);
}

void VersionsPage::onFilterChanged()
{
    m_currentFilter = m_typeFilter->currentText();
    filterVersions();
}

void VersionsPage::onSearchChanged(const QString& text)
{
    m_currentSearch = text.trimmed();
    filterVersions();
}

void VersionsPage::onDownloadClicked()
{
    QPushButton* btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    QString versionId = btn->property("versionId").toString();
    if (versionId.isEmpty()) return;

    btn->setEnabled(false);
    btn->setText("Downloading...");

    m_progressBar->show();
    m_progressBar->setRange(0, 0); // Indeterminate
    m_statusLabel->setText(QString("Downloading %1...").arg(versionId));

    // Get game directory from settings
    QString gameDir = Settings::instance().defaultInstancesDir() + "/" + versionId;
    m_launcherCore->downloadVersion(versionId, gameDir);
}

void VersionsPage::onTabChanged(int index)
{
    m_currentTab = index;
    m_contentStack->setCurrentIndex(index);
    
    // Update tab button styles
    for (int i = 0; i < m_tabGroup->buttons().size(); ++i) {
        QPushButton* btn = qobject_cast<QPushButton*>(m_tabGroup->button(i));
        if (btn) {
            if (i == index) {
                btn->setStyleSheet(R"(
                    QPushButton {
                        background: transparent;
                        border: none;
                        border-bottom: 2px solid #FF0033;
                        color: #FF0033;
                        font-family: 'Inter';
                        font-size: 14px;
                        padding: 8px 16px;
                        font-weight: 600;
                    }
                )");
            } else {
                btn->setStyleSheet(R"(
                    QPushButton {
                        background: transparent;
                        border: none;
                        border-bottom: 2px solid #262626;
                        color: #c8c6c5;
                        font-family: 'Inter';
                        font-size: 14px;
                        padding: 8px 16px;
                    }
                    QPushButton:hover:!checked {
                        background: rgba(255, 255, 255, 0.05);
                    }
                )");
            }
        }
    }
}

void VersionsPage::showVersionsEmptyState()
{
    // Clear existing cards
    QLayoutItem* item;
    while ((item = m_versionsGridLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    QLabel* emptyLabel = new QLabel("No versions available");
    emptyLabel->setStyleSheet(R"(
        QLabel {
            color: #474746;
            font-size: 16px;
            font-family: 'Inter';
        }
    )");
    emptyLabel->setAlignment(Qt::AlignCenter);
    m_versionsGridLayout->addWidget(emptyLabel, 0, 0, Qt::AlignCenter);
}

void VersionsPage::showInstancesEmptyState()
{
    // Clear existing cards
    QLayoutItem* item;
    while ((item = m_instancesGridLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    QLabel* emptyLabel = new QLabel("No instances created yet");
    emptyLabel->setStyleSheet(R"(
        QLabel {
            color: #474746;
            font-size: 16px;
            font-family: 'Inter';
        }
    )");
    emptyLabel->setAlignment(Qt::AlignCenter);
    m_instancesGridLayout->addWidget(emptyLabel, 0, 0, Qt::AlignCenter);
}

void VersionsPage::onDownloadProgress(const DownloadProgress& progress)
{
    if (progress.totalBytes > 0) {
        m_progressBar->setRange(0, progress.totalBytes);
        m_progressBar->setValue(progress.bytesDownloaded);
    }
    m_statusLabel->setText(QString("Downloading: %1 (%2/%3)").arg(progress.currentFile).arg(progress.bytesDownloaded).arg(progress.totalBytes));
}

void VersionsPage::onDownloadComplete(const QString& versionId)
{
    m_progressBar->hide();
    m_statusLabel->setText(QString("%1 downloaded successfully").arg(versionId));

    // Update version card status
    for (int i = 0; i < m_versionsGridLayout->count(); ++i) {
        QLayoutItem* item = m_versionsGridLayout->itemAt(i);
        if (item && item->widget()) {
            QFrame* card = qobject_cast<QFrame*>(item->widget());
            if (card) {
                // Find the download button in the card
                QPushButton* downloadBtn = card->findChild<QPushButton*>();
                if (downloadBtn && downloadBtn->property("versionId").toString() == versionId) {
                    downloadBtn->setText("Installed");
                    downloadBtn->setEnabled(false);
                    downloadBtn->setStyleSheet(R"(
                        QPushButton {
                            background: #353534;
                            color: #00FF66;
                            border: 1px solid #262626;
                            border-radius: 6px;
                            font-weight: bold;
                            font-family: 'Inter';
                            font-size: 13px;
                        }
                    )");
                    break;
                }
            }
        }
    }
}

void VersionsPage::onDownloadError(const QString& error)
{
    m_progressBar->hide();
    m_statusLabel->setText("Download failed: " + error);
    QMessageBox::critical(this, "Download Failed", error);
}