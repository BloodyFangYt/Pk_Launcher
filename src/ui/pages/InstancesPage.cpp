#include "ui/pages/InstancesPage.h"
#include "launcher/LauncherCore.h"
#include "launcher/InstanceManager.h"
#include "core/Settings.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QGridLayout>
#include <QMessageBox>
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QTextEdit>
#include <QCheckBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QGraphicsDropShadowEffect>
#include <QFileDialog>
#include <QStandardPaths>
#include <QProcess>
#include <QDesktopServices>
#include <QPainter>
#include <QRadialGradient>
#include <QDebug>
#include <QPropertyAnimation>

InstancesPage::InstancesPage(LauncherCore* launcherCore, InstanceManager* instanceManager, QWidget* parent)
    : QWidget(parent), m_launcherCore(launcherCore), m_instanceManager(instanceManager)
{
    setupUI();

    connect(m_instanceManager, &InstanceManager::instanceCreated, this, &InstancesPage::onInstanceListChanged);
    connect(m_instanceManager, &InstanceManager::instanceDeleted, this, &InstancesPage::onInstanceListChanged);
    connect(m_instanceManager, &InstanceManager::instanceUpdated, this, &InstancesPage::onInstanceListChanged);

    connect(m_createButton, &QPushButton::clicked, this, &InstancesPage::onCreateInstance);
    connect(m_refreshButton, &QPushButton::clicked, this, &InstancesPage::onRefreshClicked);

    refreshInstanceGrid();
}

void InstancesPage::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Scroll area
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

    QWidget* content = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setContentsMargins(32, 32, 32, 32);
    layout->setSpacing(24);

    // Header
    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(16);

    QLabel* pageTitle = new QLabel("Instances");
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

    m_instanceCountLabel = new QLabel("0 instances");
    m_instanceCountLabel->setStyleSheet(R"(
        QLabel {
            color: #c8c6c5;
            font-size: 14px;
            font-family: 'Inter';
        }
    )");
    headerLayout->addWidget(m_instanceCountLabel);

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

    m_createButton = new QPushButton("+ Create Instance");
    m_createButton->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #FF0033, stop:1 #cc0029);
            color: white;
            border: none;
            border-radius: 8px;
            padding: 10px 20px;
            font-weight: bold;
            font-family: 'Inter';
            font-size: 13px;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #cc0029, stop:1 #b30024);
        }
        QPushButton:pressed {
            background: #b30024;
        }
    )");
    headerLayout->addWidget(m_createButton);

    layout->addLayout(headerLayout);

    // Instance Grid
    QWidget* gridContainer = new QWidget();
    QVBoxLayout* gridContainerLayout = new QVBoxLayout(gridContainer);
    gridContainerLayout->setContentsMargins(0, 0, 0, 0);
    gridContainerLayout->setSpacing(0);

    m_gridWidget = new QWidget();
    m_gridLayout = new QGridLayout(m_gridWidget);
    m_gridLayout->setSpacing(24);
    m_gridLayout->setAlignment(Qt::AlignTop);

    QScrollArea* gridScroll = new QScrollArea();
    gridScroll->setWidgetResizable(true);
    gridScroll->setFrameShape(QFrame::NoFrame);
    gridScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    gridScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    gridScroll->setWidget(m_gridWidget);
    gridScroll->setStyleSheet(R"(
        QScrollArea {
            background: transparent;
        }
    )");

    gridContainerLayout->addWidget(gridScroll);

    // Empty state label
    m_emptyStateLabel = new QLabel();
    m_emptyStateLabel->setStyleSheet(R"(
        QLabel {
            color: #474746;
            font-size: 16px;
            font-family: 'Inter';
        }
    )");
    m_emptyStateLabel->setAlignment(Qt::AlignCenter);
    m_emptyStateLabel->setText("No instances created yet.\nClick \"Create Instance\" to get started.");
    m_emptyStateLabel->hide();
    gridContainerLayout->addWidget(m_emptyStateLabel);

    layout->addWidget(gridContainer, 1);

    scroll->setWidget(content);
    mainLayout->addWidget(scroll);
}

void InstancesPage::refreshInstanceGrid()
{
    // Clear existing cards
    QLayoutItem* item;
    while ((item = m_gridLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    QList<InstanceInfo> instances = m_instanceManager->listInstances();

    if (instances.isEmpty()) {
        m_emptyStateLabel->show();
        m_gridWidget->hide();
    } else {
        m_emptyStateLabel->hide();
        m_gridWidget->show();

        int row = 0, col = 0;
        const int maxCols = 3;

        for (const auto& instance : instances) {
            QFrame* card = createInstanceCard(instance);
            m_gridLayout->addWidget(card, row, col);

            col++;
            if (col >= maxCols) {
                col = 0;
                row++;
            }
        }
    }

    m_instanceCountLabel->setText(QString("%1 instance%2").arg(instances.size()).arg(instances.size() == 1 ? "" : "s"));
}

QFrame* InstancesPage::createInstanceCard(const InstanceInfo& instance)
{
    QFrame* card = new QFrame();
    card->setFixedSize(340, 420);
    card->setCursor(Qt::PointingHandCursor);
    card->setStyleSheet(R"(
        QFrame {
            background: #1A1A1A;
            border: 1px solid rgba(255, 255, 255, 0.06);
            border-radius: 12px;
        }
        QFrame:hover {
            border-color: rgba(255, 255, 255, 0.15);
        }
    )");

    // Add drop shadow
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(30);
    shadow->setOffset(0, 10);
    shadow->setColor(QColor(0, 0, 0, 180));
    card->setGraphicsEffect(shadow);

    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(0);

    // Image area
    QFrame* imageFrame = new QFrame();
    imageFrame->setFixedHeight(200);
    imageFrame->setStyleSheet(R"(
        QFrame {
            background: #131313;
            border-top-left-radius: 12px;
            border-top-right-radius: 12px;
        }
    )");

    // Generate a procedural background based on instance type
    QPixmap bg(340, 200);
    bg.fill(QColor("#131313"));
    QPainter painter(&bg);
    painter.setRenderHint(QPainter::Antialiasing);

    // Base gradient
    QLinearGradient grad(0, 0, 340, 200);
    grad.setColorAt(0, QColor("#1A1A1A"));
    grad.setColorAt(1, QColor("#131313"));
    painter.fillRect(bg.rect(), grad);

    // Add accent based on loader
    QColor accent = instance.loader == "fabric" ? QColor("#FF0033") :
                    instance.loader == "forge" ? QColor("#FF8C00") :
                    instance.loader == "quilt" ? QColor("#8B5CF6") : QColor("#00FF66");
    accent.setAlpha(30);

    QRadialGradient radial(170, 100, 150);
    radial.setColorAt(0, accent);
    radial.setColorAt(1, QColor(0, 0, 0, 0));
    painter.setBrush(radial);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(170, 100, 150, 150);

    // Add grid pattern
    painter.setPen(QColor(255, 255, 255, 5));
    for (int x = 0; x < 340; x += 20) painter.drawLine(x, 0, x, 200);
    for (int y = 0; y < 200; y += 20) painter.drawLine(0, y, 340, y);

    painter.end();

    QLabel* imageLabel = new QLabel();
    imageLabel->setPixmap(bg);
    imageLabel->setFixedSize(340, 200);
    imageLabel->setAlignment(Qt::AlignCenter);

    QVBoxLayout* imageLayout = new QVBoxLayout(imageFrame);
    imageLayout->setContentsMargins(0, 0, 0, 0);
    imageLayout->addWidget(imageLabel);

    // Overlay info at bottom of image
    QFrame* overlay = new QFrame(imageFrame);
    overlay->setFixedHeight(80);
    overlay->move(0, 120);
    overlay->resize(340, 80);
    overlay->setStyleSheet(R"(
        QFrame {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                stop:0 rgba(0,0,0,0), stop:1 rgba(0,0,0,0.9));
            border: none;
        }
    )");

    QVBoxLayout* overlayLayout = new QVBoxLayout(overlay);
    overlayLayout->setContentsMargins(16, 8, 16, 16);
    overlayLayout->setSpacing(4);

    QLabel* nameLabel = new QLabel(instance.name);
    nameLabel->setStyleSheet(R"(
        QLabel {
            color: white;
            font-size: 18px;
            font-weight: 700;
            font-family: 'Inter';
        }
    )");
    nameLabel->setWordWrap(true);
    overlayLayout->addWidget(nameLabel);

    QString loaderDisplay = instance.loader.isEmpty() ? "Vanilla" : 
                            instance.loader.toUpper() + (instance.loaderVersion.isEmpty() ? "" : " " + instance.loaderVersion);
    QString versionDisplay = QString("%1  •  %2").arg(instance.version, loaderDisplay);

    QLabel* versionLabel = new QLabel(versionDisplay);
    versionLabel->setStyleSheet(QString(R"(
        QLabel {
            color: %1;
            font-family: 'JetBrains Mono';
            font-size: 12px;
            font-weight: 500;
        }
    )").arg(accent.name()));
    overlayLayout->addWidget(versionLabel);

    overlay->show();

    cardLayout->addWidget(imageFrame);

    // Info section
    QWidget* infoWidget = new QWidget();
    infoWidget->setStyleSheet("background: transparent;");
    QVBoxLayout* infoLayout = new QVBoxLayout(infoWidget);
    infoLayout->setContentsMargins(16, 16, 16, 16);
    infoLayout->setSpacing(16);

    // Meta info
    QHBoxLayout* metaLayout = new QHBoxLayout();
    metaLayout->setSpacing(16);

    QString lastPlayed = instance.lastPlayed.isValid() ? 
                         instance.lastPlayed.toString("MMM d, yyyy") : "Never";
    QString sizeDisplay = QString("%1 MB").arg(instance.ramMb); // Using RAM as size indicator for now

    QLabel* lastPlayedLabel = new QLabel(QString("Last played: %1").arg(lastPlayed));
    lastPlayedLabel->setStyleSheet(R"(
        QLabel {
            color: #474746;
            font-family: 'JetBrains Mono';
            font-size: 11px;
        }
    )");
    metaLayout->addWidget(lastPlayedLabel);

    metaLayout->addStretch();

    QLabel* sizeLabel = new QLabel(QString("RAM: %1 MB").arg(instance.ramMb));
    sizeLabel->setStyleSheet(R"(
        QLabel {
            color: #474746;
            font-family: 'JetBrains Mono';
            font-size: 11px;
        }
    )");
    metaLayout->addWidget(sizeLabel);

    infoLayout->addLayout(metaLayout);

    // Action buttons
    QHBoxLayout* actionLayout = new QHBoxLayout();
    actionLayout->setSpacing(8);

    QPushButton* playBtn = new QPushButton("PLAY");
    playBtn->setProperty("instanceId", instance.id);
    playBtn->setFixedHeight(40);
    playBtn->setCursor(Qt::PointingHandCursor);
    playBtn->setStyleSheet(R"(
        QPushButton {
            background: #FF0033;
            color: white;
            border: none;
            border-radius: 6px;
            font-family: 'JetBrains Mono';
            font-size: 11px;
            font-weight: bold;
        }
        QPushButton:hover {
            background: #cc0029;
        }
        QPushButton:pressed {
            background: #b30024;
        }
    )");
    connect(playBtn, &QPushButton::clicked, this, [this, instanceId = instance.id]() {
        onPlayInstance(instanceId);
    });

    QPushButton* editBtn = new QPushButton();
    editBtn->setProperty("instanceId", instance.id);
    editBtn->setFixedSize(40, 40);
    editBtn->setCursor(Qt::PointingHandCursor);
    editBtn->setStyleSheet(R"(
        QPushButton {
            background: rgba(255, 255, 255, 0.05);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 6px;
            color: white;
        }
        QPushButton:hover {
            background: rgba(255, 255, 255, 0.1);
            border-color: rgba(255, 255, 255, 0.2);
        }
    )");
    editBtn->setText("✏️");
    connect(editBtn, &QPushButton::clicked, this, [this, instanceId = instance.id]() {
        onEditInstance(instanceId);
    });

    QPushButton* folderBtn = new QPushButton();
    folderBtn->setProperty("instanceId", instance.id);
    folderBtn->setFixedSize(40, 40);
    folderBtn->setCursor(Qt::PointingHandCursor);
    folderBtn->setStyleSheet(R"(
        QPushButton {
            background: rgba(255, 255, 255, 0.05);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 6px;
            color: white;
        }
        QPushButton:hover {
            background: rgba(255, 255, 255, 0.1);
            border-color: rgba(255, 255, 255, 0.2);
        }
    )");
    folderBtn->setText("📁");
    connect(folderBtn, &QPushButton::clicked, this, [this, instanceId = instance.id]() {
        onOpenInstanceFolder(instanceId);
    });

    actionLayout->addWidget(playBtn, 1);
    actionLayout->addWidget(editBtn);
    actionLayout->addWidget(folderBtn);

    infoLayout->addLayout(actionLayout);
    cardLayout->addWidget(infoWidget);

    return card;
}

void InstancesPage::onInstanceListChanged()
{
    refreshInstanceGrid();
}

void InstancesPage::onCreateInstance()
{
    showCreateDialog();
}

void InstancesPage::onEditInstance(const QString& instanceId)
{
    showEditDialog(instanceId);
}

void InstancesPage::onRefreshClicked()
{
    refreshInstanceGrid();
}

void InstancesPage::onPlayInstance(const QString& instanceId)
{
    InstanceInfo instance = m_instanceManager->getInstance(instanceId);
    if (instance.id.isEmpty()) return;

    // Navigate to Play page and select this instance
    // For now, just launch directly
    // In a full implementation, we'd emit a signal to MainWindow to switch pages
}

void InstancesPage::onOpenInstanceFolder(const QString& instanceId)
{
    InstanceInfo instance = m_instanceManager->getInstance(instanceId);
    if (instance.id.isEmpty() || instance.gameDir.isEmpty()) return;

    QDesktopServices::openUrl(QUrl::fromLocalFile(instance.gameDir));
}

bool InstancesPage::confirmDelete(const QString& name)
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Delete Instance");
    msgBox.setText(QString("Are you sure you want to delete '%1'?").arg(name));
    msgBox.setInformativeText("This action cannot be undone. All instance data will be permanently removed.");
    msgBox.setStandardButtons(QMessageBox::StandardButton::No | QMessageBox::StandardButton::Cancel);
    msgBox.setDefaultButton(QMessageBox::StandardButton::Cancel);
    msgBox.setStyleSheet(R"(
        QMessageBox {
            background: #1c1b1b;
            color: #e5e2e1;
        }
        QLabel {
            color: #e5e2e1;
        }
        QPushButton {
            background: #353534;
            color: #e5e2e1;
            border: 1px solid #262626;
            border-radius: 4px;
            padding: 8px 24px;
            min-width: 80px;
        }
        QPushButton:hover {
            background: #474746;
        }
        QPushButton[text="No"] {
            background: #b30024;
            color: white;
            border: 1px solid #FF0033;
        }
        QPushButton[text="No"]:hover {
            background: #FF0033;
        }
    )");
    return msgBox.exec() == QMessageBox::StandardButton::No;
}

void InstancesPage::showCreateDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Create New Instance");
    dialog.setModal(true);
    dialog.setMinimumWidth(480);
    dialog.setStyleSheet(R"(
        QDialog {
            background: #1c1b1b;
            color: #e5e2e1;
        }
        QLabel {
            color: #e5e2e1;
            font-size: 13px;
        }
        QLineEdit, QComboBox, QSpinBox, QTextEdit {
            background: #131313;
            border: 1px solid #262626;
            border-radius: 4px;
            padding: 8px;
            color: #e5e2e1;
        }
        QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QTextEdit:focus {
            border-color: #FF0033;
        }
    )");

    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    layout->setSpacing(16);

    QFormLayout* form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight);
    form->setFormAlignment(Qt::AlignLeft);
    form->setSpacing(12);

    QLineEdit* nameEdit = new QLineEdit();
    nameEdit->setPlaceholderText("My Survival World");
    form->addRow("Instance Name *:", nameEdit);

    QLineEdit* versionEdit = new QLineEdit();
    versionEdit->setPlaceholderText("e.g. 1.20.1");
    form->addRow("Minecraft Version *:", versionEdit);

    QComboBox* loaderCombo = new QComboBox();
    loaderCombo->addItem("Vanilla", "");
    loaderCombo->addItem("Fabric", "fabric");
    loaderCombo->addItem("Forge", "forge");
    loaderCombo->addItem("Quilt", "quilt");
    form->addRow("Mod Loader:", loaderCombo);

    QLineEdit* loaderVersionEdit = new QLineEdit();
    loaderVersionEdit->setPlaceholderText("e.g. 0.14.21 (auto if empty)");
    form->addRow("Loader Version:", loaderVersionEdit);

    QSpinBox* ramSpin = new QSpinBox();
    ramSpin->setRange(512, 32768);
    ramSpin->setSingleStep(512);
    ramSpin->setValue(2048);
    ramSpin->setSuffix(" MB");
    form->addRow("RAM:", ramSpin);

    QLineEdit* javaVersionEdit = new QLineEdit();
    javaVersionEdit->setPlaceholderText("17");
    form->addRow("Java Version:", javaVersionEdit);

    QTextEdit* jvmArgsEdit = new QTextEdit();
    jvmArgsEdit->setFixedHeight(80);
    jvmArgsEdit->setPlaceholderText("Optional: -Xms2G -Xmx4G ...");
    form->addRow("JVM Args:", jvmArgsEdit);

    layout->addLayout(form);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->setStyleSheet(R"(
        QPushButton {
            background: #353534;
            color: #e5e2e1;
            border: 1px solid #262626;
            border-radius: 4px;
            padding: 8px 24px;
            font-weight: bold;
            min-width: 80px;
        }
        QPushButton:hover {
            background: #474746;
        }
        QPushButton[text="OK"] {
            background: #FF0033;
            color: white;
            border: none;
        }
        QPushButton[text="OK"]:hover {
            background: #cc0029;
        }
    )");
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() == QDialog::Accepted) {
        QString name = nameEdit->text().trimmed();
        QString version = versionEdit->text().trimmed();

        if (name.isEmpty() || version.isEmpty()) {
            QMessageBox::warning(this, "Invalid Input", "Name and Version are required.");
            return;
        }

        QString loader = loaderCombo->currentData().toString();
        QString loaderVersion = loaderVersionEdit->text().trimmed();
        int ramMb = ramSpin->value();
        int javaVersion = javaVersionEdit->text().trimmed().toInt();
        QString jvmArgs = jvmArgsEdit->toPlainText().trimmed();

        if (javaVersion == 0) javaVersion = 17;

        InstanceInfo info = m_instanceManager->createInstance(name, version);
        if (!info.id.isEmpty()) {
            info.loader = loader;
            info.loaderVersion = loaderVersion;
            info.javaVersion = javaVersion;
            info.ramMb = ramMb;
            info.jvmArgs = jvmArgs;
            m_instanceManager->updateInstance(info);
        }
    }
}

void InstancesPage::showEditDialog(const QString& instanceId)
{
    InstanceInfo instance = m_instanceManager->getInstance(instanceId);
    if (instance.id.isEmpty()) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Edit Instance: " + instance.name);
    dialog.setModal(true);
    dialog.setMinimumWidth(480);
    dialog.setStyleSheet(R"(
        QDialog {
            background: #1c1b1b;
            color: #e5e2e1;
        }
        QLabel {
            color: #e5e2e1;
            font-size: 13px;
        }
        QLineEdit, QComboBox, QSpinBox, QTextEdit {
            background: #131313;
            border: 1px solid #262626;
            border-radius: 4px;
            padding: 8px;
            color: #e5e2e1;
        }
        QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QTextEdit:focus {
            border-color: #FF0033;
        }
    )");

    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    layout->setSpacing(16);

    QFormLayout* form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight);
    form->setFormAlignment(Qt::AlignLeft);
    form->setSpacing(12);

    QLineEdit* nameEdit = new QLineEdit(instance.name);
    form->addRow("Instance Name *:", nameEdit);

    QLineEdit* versionEdit = new QLineEdit(instance.version);
    form->addRow("Minecraft Version *:", versionEdit);

    QComboBox* loaderCombo = new QComboBox();
    loaderCombo->addItem("Vanilla", "");
    loaderCombo->addItem("Fabric", "fabric");
    loaderCombo->addItem("Forge", "forge");
    loaderCombo->addItem("Quilt", "quilt");
    int loaderIdx = 0;
    if (instance.loader == "fabric") loaderIdx = 1;
    else if (instance.loader == "forge") loaderIdx = 2;
    else if (instance.loader == "quilt") loaderIdx = 3;
    loaderCombo->setCurrentIndex(loaderIdx);
    form->addRow("Mod Loader:", loaderCombo);

    QLineEdit* loaderVersionEdit = new QLineEdit(instance.loaderVersion);
    form->addRow("Loader Version:", loaderVersionEdit);

    QSpinBox* ramSpin = new QSpinBox();
    ramSpin->setRange(512, 32768);
    ramSpin->setSingleStep(512);
    ramSpin->setValue(instance.ramMb);
    ramSpin->setSuffix(" MB");
    form->addRow("RAM:", ramSpin);

    QLineEdit* javaVersionEdit = new QLineEdit(QString::number(instance.javaVersion));
    form->addRow("Java Version:", javaVersionEdit);

    QTextEdit* jvmArgsEdit = new QTextEdit(instance.jvmArgs);
    jvmArgsEdit->setFixedHeight(80);
    form->addRow("JVM Args:", jvmArgsEdit);

    // Delete button
    QPushButton* deleteBtn = new QPushButton("Delete Instance");
    deleteBtn->setStyleSheet(R"(
        QPushButton {
            background: #b30024;
            color: white;
            border: 1px solid #FF0033;
            border-radius: 4px;
            padding: 8px 16px;
            font-weight: bold;
        }
        QPushButton:hover {
            background: #FF0033;
        }
    )");
    connect(deleteBtn, &QPushButton::clicked, [&dialog, instanceId, this]() {
        if (confirmDelete(m_instanceManager->getInstance(instanceId).name)) {
            m_instanceManager->deleteInstance(instanceId);
            dialog.accept();
        }
    });

    layout->addLayout(form);
    layout->addWidget(deleteBtn, 0, Qt::AlignLeft);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->setStyleSheet(R"(
        QPushButton {
            background: #353534;
            color: #e5e2e1;
            border: 1px solid #262626;
            border-radius: 4px;
            padding: 8px 24px;
            font-weight: bold;
            min-width: 80px;
        }
        QPushButton:hover {
            background: #474746;
        }
        QPushButton[text="OK"] {
            background: #FF0033;
            color: white;
            border: none;
        }
        QPushButton[text="OK"]:hover {
            background: #cc0029;
        }
    )");
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() == QDialog::Accepted) {
        QString name = nameEdit->text().trimmed();
        QString version = versionEdit->text().trimmed();

        if (name.isEmpty() || version.isEmpty()) {
            QMessageBox::warning(this, "Invalid Input", "Name and Version are required.");
            return;
        }

        instance.name = name;
        instance.version = version;
        instance.loader = loaderCombo->currentData().toString();
        instance.loaderVersion = loaderVersionEdit->text().trimmed();
        instance.ramMb = ramSpin->value();
        instance.javaVersion = javaVersionEdit->text().trimmed().toInt();
        instance.jvmArgs = jvmArgsEdit->toPlainText().trimmed();

        if (instance.javaVersion == 0) instance.javaVersion = 17;

        m_instanceManager->updateInstance(instance);
    }
}