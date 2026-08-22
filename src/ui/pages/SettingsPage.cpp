#include "ui/pages/SettingsPage.h"

#include "core/Settings.h"
#include "core/UpdateManager.h"
#include "launcher/LauncherCore.h"

#include <QScrollArea>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QColorDialog>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QShowEvent>
#include <QColor>
#include <QRegularExpression>
#include <QtGlobal>

SettingsPage::SettingsPage(LauncherCore* launcherCore, QWidget* parent)
    : QWidget(parent), m_launcherCore(launcherCore)
{
    m_updateManager = new UpdateManager(this);
    setupUI();
}

QFrame* SettingsPage::createCard(const QString& title)
{
    QFrame* card = new QFrame();
    card->setStyleSheet(R"(
        QFrame {
            background: #1c1b1b;
            border: 1px solid #262626;
            border-radius: 8px;
        }
    )");

    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(24, 24, 24, 24);
    cardLayout->setSpacing(12);

    QLabel* header = new QLabel(title);
    header->setStyleSheet("color: #e5e2e1; font-size: 20px; font-weight: bold;");
    cardLayout->addWidget(header);

    return card;
}

void SettingsPage::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QScrollArea* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setStyleSheet("QScrollArea { background: transparent; }");

    QWidget* content = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(content);
    layout->setContentsMargins(32, 32, 32, 32);
    layout->setSpacing(16);

    // ---- JAVA section ----
    QFrame* javaCard = createCard("🖥️  Java");
    QVBoxLayout* javaLayout = qobject_cast<QVBoxLayout*>(javaCard->layout());

    // Java path row
    QHBoxLayout* javaPathRow = new QHBoxLayout();
    m_javaPath = new QLineEdit();
    m_javaPath->setPlaceholderText("Path to Java executable (java or java.exe)");
    m_javaPath->setStyleSheet(R"(
        QLineEdit {
            background: #1c1b1b;
            border: 1px solid #262626;
            border-radius: 4px;
            padding: 8px;
            color: #e5e2e1;
        }
    )");

    QPushButton* browseBtn = new QPushButton("Browse");
    browseBtn->setStyleSheet(R"(
        QPushButton {
            background: #FF0033;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 10px 20px;
            font-weight: bold;
        }
        QPushButton:hover { background: #cc0029; }
    )");

    javaPathRow->addWidget(m_javaPath, 1);
    javaPathRow->addWidget(browseBtn);
    javaLayout->addLayout(javaPathRow);

    QPushButton* detectBtn = new QPushButton("Detect Java");
    detectBtn->setStyleSheet(R"(
        QPushButton {
            background: #353534;
            color: #e5e2e1;
            border: none;
            border-radius: 4px;
            padding: 10px 20px;
            font-weight: bold;
        }
        QPushButton:hover { background: #474746; }
    )");
    javaLayout->addWidget(detectBtn, 0, Qt::AlignLeft);

    QLabel* customArgsLabel = new QLabel("Custom JVM Args");
    customArgsLabel->setStyleSheet("color: #c8c6c5; font-size: 12px;");
    javaLayout->addWidget(customArgsLabel);

    m_customJvmArgs = new QTextEdit();
    m_customJvmArgs->setFixedHeight(90);
    javaLayout->addWidget(m_customJvmArgs);

    layout->addWidget(javaCard);

    // ---- LAUNCHER section ----
    QFrame* launcherCard = createCard("🚀  Launcher");
    QVBoxLayout* launcherLayout = qobject_cast<QVBoxLayout*>(launcherCard->layout());

    // RAM row
    QHBoxLayout* ramRow = new QHBoxLayout();
    m_ramSlider = new QSlider(Qt::Horizontal);
    m_ramSlider->setRange(512, 32768);
    m_ramSlider->setSingleStep(512);
    m_ramSlider->setPageStep(512);
    m_ramValue = new QLabel("4096 MB");
    m_ramValue->setStyleSheet("color: #c8c6c5; font-size: 12px;");
    m_ramValue->setFixedWidth(90);
    m_ramValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    ramRow->addWidget(m_ramSlider, 1);
    ramRow->addWidget(m_ramValue);
    launcherLayout->addLayout(ramRow);

    QLabel* defaultArgsLabel = new QLabel("Default JVM Args");
    defaultArgsLabel->setStyleSheet("color: #c8c6c5; font-size: 12px;");
    launcherLayout->addWidget(defaultArgsLabel);

    m_defaultJvmArgs = new QTextEdit();
    m_defaultJvmArgs->setFixedHeight(90);
    launcherLayout->addWidget(m_defaultJvmArgs);

    m_closeOnLaunch = new QCheckBox("Close launcher when game starts");
    m_showConsole = new QCheckBox("Show game console");
    m_closeOnLaunch->setStyleSheet("color: #c8c6c5; font-size: 12px;");
    m_showConsole->setStyleSheet("color: #c8c6c5; font-size: 12px;");
    launcherLayout->addWidget(m_closeOnLaunch);
    launcherLayout->addWidget(m_showConsole);

    layout->addWidget(launcherCard);

    // ---- NETWORK section ----
    QFrame* networkCard = createCard("🌐  Network");
    QVBoxLayout* networkLayout = qobject_cast<QVBoxLayout*>(networkCard->layout());

    QHBoxLayout* proxyTypeRow = new QHBoxLayout();
    QLabel* proxyTypeLabel = new QLabel("Proxy Type");
    proxyTypeLabel->setStyleSheet("color: #c8c6c5; font-size: 12px;");
    m_proxyType = new QComboBox();
    m_proxyType->addItems({"None", "HTTP", "SOCK5"});
    proxyTypeRow->addWidget(proxyTypeLabel);
    proxyTypeRow->addWidget(m_proxyType, 1);
    networkLayout->addLayout(proxyTypeRow);

    // Proxy host + port
    QHBoxLayout* proxyHostRow = new QHBoxLayout();
    QLabel* hostLabel = new QLabel("Proxy Host");
    hostLabel->setStyleSheet("color: #c8c6c5; font-size: 12px;");
    m_proxyHost = new QLineEdit();
    m_proxyHost->setPlaceholderText("e.g. 127.0.0.1");
    proxyHostRow->addWidget(hostLabel);
    proxyHostRow->addWidget(m_proxyHost, 1);

    QLabel* portLabel = new QLabel("Port");
    portLabel->setStyleSheet("color: #c8c6c5; font-size: 12px;");
    m_proxyPort = new QSpinBox();
    m_proxyPort->setRange(0, 65535);
    proxyHostRow->addWidget(portLabel);
    proxyHostRow->addWidget(m_proxyPort);
    networkLayout->addLayout(proxyHostRow);

    // Timeout
    QHBoxLayout* timeoutRow = new QHBoxLayout();
    QLabel* timeoutLabel = new QLabel("Timeout (seconds)");
    timeoutLabel->setStyleSheet("color: #c8c6c5; font-size: 12px;");
    m_timeout = new QSpinBox();
    m_timeout->setRange(5, 300);
    timeoutRow->addWidget(timeoutLabel);
    timeoutRow->addWidget(m_timeout, 1);
    networkLayout->addLayout(timeoutRow);

    // Max concurrent downloads
    QHBoxLayout* concurrentRow = new QHBoxLayout();
    QLabel* concurrentLabel = new QLabel("Max Concurrent Downloads");
    concurrentLabel->setStyleSheet("color: #c8c6c5; font-size: 12px;");
    m_maxConcurrent = new QSpinBox();
    m_maxConcurrent->setRange(1, 32);
    concurrentRow->addWidget(concurrentLabel);
    concurrentRow->addWidget(m_maxConcurrent, 1);
    networkLayout->addLayout(concurrentRow);

    layout->addWidget(networkCard);

    // ---- APPEARANCE section ----
    QFrame* appearanceCard = createCard("🎨  Appearance");
    QVBoxLayout* appearanceLayout = qobject_cast<QVBoxLayout*>(appearanceCard->layout());

    QHBoxLayout* themeRow = new QHBoxLayout();
    QLabel* themeLabel = new QLabel("Theme");
    themeLabel->setStyleSheet("color: #c8c6c5; font-size: 12px;");
    m_theme = new QComboBox();
    m_theme->addItem("Dark", "dark");
    m_theme->addItem("Light", "light");
    themeRow->addWidget(themeLabel);
    themeRow->addWidget(m_theme, 1);
    appearanceLayout->addLayout(themeRow);

    // Accent color indicator
    QHBoxLayout* accentRow = new QHBoxLayout();
    QLabel* accentLabel = new QLabel("Accent Color");
    accentLabel->setStyleSheet("color: #c8c6c5; font-size: 12px;");
    m_accentIndicator = new QPushButton("#FF0033");
    m_accentIndicator->setFixedWidth(120);
    m_accentIndicator->setStyleSheet(R"(
        QPushButton {
            background: #FF0033;
            color: white;
            border: 1px solid rgba(255, 255, 255, 0.2);
            border-radius: 4px;
            padding: 8px 12px;
            font-weight: bold;
        }
        QPushButton:hover { border-color: #e5e2e1; }
    )");
    accentRow->addWidget(accentLabel);
    accentRow->addWidget(m_accentIndicator, 0, Qt::AlignLeft);
    accentRow->addStretch();
    appearanceLayout->addLayout(accentRow);

    m_animations = new QCheckBox("Enable animations");
    m_animations->setStyleSheet("color: #c8c6c5; font-size: 12px;");
    appearanceLayout->addWidget(m_animations);

    // UI scale row
    QHBoxLayout* scaleRow = new QHBoxLayout();
    QLabel* scaleLabel = new QLabel("UI Scale");
    scaleLabel->setStyleSheet("color: #c8c6c5; font-size: 12px;");
    m_uiScale = new QSlider(Qt::Horizontal);
    m_uiScale->setRange(50, 200);
    m_uiScale->setSingleStep(10);
    m_uiScale->setValue(100);
    m_uiScaleValue = new QLabel("1.00");
    m_uiScaleValue->setStyleSheet("color: #c8c6c5; font-size: 12px;");
    m_uiScaleValue->setFixedWidth(40);
    m_uiScaleValue->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    scaleRow->addWidget(scaleLabel);
    scaleRow->addWidget(m_uiScale, 1);
    scaleRow->addWidget(m_uiScaleValue);
    appearanceLayout->addLayout(scaleRow);

    layout->addWidget(appearanceCard);

    // ---- ABOUT section ----
    QFrame* aboutCard = createCard("ℹ️  About");
    QVBoxLayout* aboutLayout = qobject_cast<QVBoxLayout*>(aboutCard->layout());

    QLabel* versionLabel = new QLabel("PK Launcher  v0.1.0");
    versionLabel->setStyleSheet("color: #e5e2e1; font-size: 16px; font-weight: bold;");
    aboutLayout->addWidget(versionLabel);

    QHBoxLayout* updateRow = new QHBoxLayout();
    QPushButton* checkUpdatesBtn = new QPushButton("Check for Updates");
    checkUpdatesBtn->setStyleSheet(R"(
        QPushButton {
            background: #FF0033;
            color: white;
            border: none;
            border-radius: 4px;
            padding: 10px 20px;
            font-weight: bold;
        }
        QPushButton:hover { background: #cc0029; }
    )");
    updateRow->addWidget(checkUpdatesBtn, 0, Qt::AlignLeft);

    m_updateStatus = new QLabel("v0.1.0");
    m_updateStatus->setStyleSheet("color: #c8c6c5; font-size: 12px;");
    m_updateStatus->setWordWrap(true);
    updateRow->addWidget(m_updateStatus, 1);

    aboutLayout->addLayout(updateRow);

    layout->addWidget(aboutCard);
    layout->addStretch();

    scroll->setWidget(content);
    mainLayout->addWidget(scroll);

    // Wire signals
    connect(browseBtn, &QPushButton::clicked, this, &SettingsPage::onBrowseJava);
    connect(detectBtn, &QPushButton::clicked, this, &SettingsPage::onDetectJava);
    connect(checkUpdatesBtn, &QPushButton::clicked, this, &SettingsPage::onCheckUpdates);
    connect(m_accentIndicator, &QPushButton::clicked, this, &SettingsPage::onAccentPick);

    // Update manager feedback
    connect(m_updateManager, &UpdateManager::updateAvailable, this,
            [this](const UpdateInfo& info) {
                m_updateStatus->setText(QString("Update available: v%1").arg(info.version));
            });
    connect(m_updateManager, &UpdateManager::noUpdateAvailable, this,
            [this]() { m_updateStatus->setText("You are up to date."); });
    connect(m_updateManager, &UpdateManager::updateCheckFailed, this,
            [this](const QString& error) {
                m_updateStatus->setText(QString("Update check failed: %1").arg(error));
            });

    connect(m_javaPath, &QLineEdit::textChanged, this, &SettingsPage::saveSettings);
    // Keep the two JVM args editors in sync (both bind to defaultJvmArgs)
    connect(m_customJvmArgs, &QTextEdit::textChanged, this, [this]() {
        if (m_loading) return;
        QSignalBlocker blocker(m_defaultJvmArgs);
        m_defaultJvmArgs->setPlainText(m_customJvmArgs->toPlainText());
        saveSettings();
    });
    connect(m_defaultJvmArgs, &QTextEdit::textChanged, this, [this]() {
        if (m_loading) return;
        QSignalBlocker blocker(m_customJvmArgs);
        m_customJvmArgs->setPlainText(m_defaultJvmArgs->toPlainText());
        saveSettings();
    });
    connect(m_ramSlider, &QSlider::valueChanged, this, [this](int v) {
        m_ramValue->setText(QString("%1 MB").arg(v));
    });
    connect(m_closeOnLaunch, &QCheckBox::toggled, this, &SettingsPage::saveSettings);
    connect(m_showConsole, &QCheckBox::toggled, this, &SettingsPage::saveSettings);
    connect(m_proxyType, &QComboBox::currentIndexChanged, this, &SettingsPage::saveSettings);
    connect(m_proxyHost, &QLineEdit::textChanged, this, &SettingsPage::saveSettings);
    connect(m_proxyPort, &QSpinBox::valueChanged, this, &SettingsPage::saveSettings);
    connect(m_timeout, &QSpinBox::valueChanged, this, &SettingsPage::saveSettings);
    connect(m_maxConcurrent, &QSpinBox::valueChanged, this, &SettingsPage::saveSettings);
    connect(m_theme, &QComboBox::currentIndexChanged, this, &SettingsPage::saveSettings);
    connect(m_animations, &QCheckBox::toggled, this, &SettingsPage::saveSettings);
    connect(m_uiScale, &QSlider::valueChanged, this, [this](int v) {
        m_uiScaleValue->setText(QString::asprintf("%.2f", v / 100.0));
        saveSettings();
    });
}

void SettingsPage::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    loadSettings();
}

void SettingsPage::loadSettings()
{
    m_loading = true;

    const Settings& s = Settings::instance();

    m_javaPath->setText(s.javaAutoDetect());

    // Default JVM args, one per line, mirrored into both editors
    QString jvmText = s.defaultJvmArgs().join('\n');
    m_defaultJvmArgs->setPlainText(jvmText);
    m_customJvmArgs->setPlainText(jvmText);

    // RAM is not persisted in Settings (no backing field) — display a default.
    m_ramSlider->setValue(4096);

    m_closeOnLaunch->setChecked(s.closeOnLaunch());
    m_showConsole->setChecked(s.showConsole());

    const QString proxyType = s.proxyType();
    int proxyIdx = 0; // None
    if (proxyType == QLatin1String("HTTP")) proxyIdx = 1;
    else if (proxyType == QLatin1String("SOCK5")) proxyIdx = 2;
    m_proxyType->setCurrentIndex(proxyIdx);
    m_proxyHost->setText(s.proxyHost());
    m_proxyPort->setValue(s.proxyPort());
    m_timeout->setValue(s.timeoutSeconds());
    m_maxConcurrent->setValue(s.maxConcurrentDownloads());

    int themeIdx = (s.theme() == QLatin1String("light")) ? 1 : 0;
    m_theme->setCurrentIndex(themeIdx);
    updateAccentButton(s.accentColor());
    m_animations->setChecked(s.animations());

    int scaleVal = qBound(50, static_cast<int>(s.uiScale() * 100.0), 200);
    m_uiScale->setValue(scaleVal);
    m_uiScaleValue->setText(QString::asprintf("%.2f", scaleVal / 100.0));

    m_loading = false;
}

void SettingsPage::saveSettings()
{
    if (m_loading) return;

    Settings& s = Settings::instance();

    s.setJavaAutoDetect(m_javaPath->text());
    s.setDefaultJvmArgs(splitJvmArgs(m_defaultJvmArgs->toPlainText()));

    s.setCloseOnLaunch(m_closeOnLaunch->isChecked());
    s.setShowConsole(m_showConsole->isChecked());

    switch (m_proxyType->currentIndex()) {
    case 1: s.setProxyType("HTTP"); break;
    case 2: s.setProxyType("SOCK5"); break;
    default: s.setProxyType("None"); break;
    }
    s.setProxyHost(m_proxyHost->text());
    s.setProxyPort(static_cast<quint16>(m_proxyPort->value()));
    s.setTimeoutSeconds(m_timeout->value());
    s.setMaxConcurrentDownloads(m_maxConcurrent->value());

    s.setTheme(m_theme->currentData().toString());
    s.setAnimations(m_animations->isChecked());
    s.setUiScale(m_uiScale->value() / 100.0);

    s.save();
}

void SettingsPage::onBrowseJava()
{
    const QString path = QFileDialog::getOpenFileName(
        this, "Select Java Executable",
        Settings::javaDir(),
        "Java Executable (java java.exe);;All Files (*)");

    if (!path.isEmpty()) {
        m_javaPath->setText(path);
        Settings::instance().setJavaAutoDetect(path);
        Settings::instance().save();
    }
}

void SettingsPage::onDetectJava()
{
    const QList<JavaInfo> javaList = m_launcherCore->detectJava();
    if (javaList.isEmpty()) {
        m_updateStatus->setText("No Java installations found.");
        return;
    }
    // Prefer the newest major version found.
    const JavaInfo* best = &javaList.first();
    for (const JavaInfo& j : javaList) {
        if (j.majorVersion > best->majorVersion) best = &j;
    }
    m_javaPath->setText(best->path);
    Settings::instance().setJavaAutoDetect(best->path);
    Settings::instance().save();
    m_updateStatus->setText(QString("Detected Java %1").arg(best->version));
}

void SettingsPage::onAccentPick()
{
    const Settings& s = Settings::instance();
    const QColor current(s.accentColor());
    const QColor chosen = QColorDialog::getColor(current, this, "Choose Accent Color");
    if (!chosen.isValid()) return;
    const QString hex = chosen.name(QColor::HexRgb).toUpper();
    updateAccentButton(hex);
    Settings::instance().setAccentColor(hex);
    Settings::instance().save();
}

void SettingsPage::onCheckUpdates()
{
    m_updateStatus->setText("Checking for updates...");
    m_updateManager->checkForUpdates(false);
}

void SettingsPage::updateAccentButton(const QString& color)
{
    m_accentIndicator->setText(color.toUpper());
    m_accentIndicator->setStyleSheet(QString(R"(
        QPushButton {
            background: %1;
            color: white;
            border: 1px solid rgba(255, 255, 255, 0.2);
            border-radius: 4px;
            padding: 8px 12px;
            font-weight: bold;
        }
        QPushButton:hover { border-color: #e5e2e1; }
    )").arg(color));
}

QStringList SettingsPage::splitJvmArgs(const QString& text) const
{
    QStringList result;
    const QStringList lines = text.split(
        QRegularExpression("[\\r\\n\\s]+"), Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        result.append(line.trimmed());
    }
    return result;
}