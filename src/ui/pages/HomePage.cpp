#include "ui/pages/HomePage.h"
#include "launcher/LauncherCore.h"
#include "core/Settings.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QScrollArea>
#include <QLinearGradient>
#include <QPainter>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QDateTime>
#include <QDebug>

HomePage::HomePage(LauncherCore* launcherCore, QWidget* parent)
    : QWidget(parent), m_launcherCore(launcherCore)
{
    setupUI();

    // Start animation for play button glow
    m_glowAnimation = new QPropertyAnimation(m_glowEffect, "opacity", this);
    m_glowAnimation->setDuration(2000);
    m_glowAnimation->setStartValue(0.3);
    m_glowAnimation->setEndValue(1.0);
    m_glowAnimation->setLoopCount(-1);
    m_glowAnimation->start();

    // Try to load hero background
    QTimer::singleShot(0, this, &HomePage::createHeroBackground);
}

void HomePage::setupUI()
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

    // ========== HERO SECTION ==========
    m_heroWidget = new QFrame();
    m_heroWidget->setFixedHeight(450);
    m_heroWidget->setStyleSheet(R"(
        QFrame {
            background: #131313;
            border-radius: 12px;
        }
    )");
    m_heroWidget->setAttribute(Qt::WA_TranslucentBackground, true);

    QVBoxLayout* heroLayout = new QVBoxLayout(m_heroWidget);
    heroLayout->setContentsMargins(48, 48, 48, 48);
    heroLayout->setSpacing(16);

    // Status badge with pulsing dot
    QFrame* statusBadge = new QFrame();
    statusBadge->setFixedWidth(220);
    statusBadge->setStyleSheet(R"(
        QFrame {
            background: rgba(38, 38, 38, 0.8);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 8px;
        }
    )");

    QHBoxLayout* badgeLayout = new QHBoxLayout(statusBadge);
    badgeLayout->setContentsMargins(12, 8, 12, 8);
    badgeLayout->setSpacing(8);

    QLabel* pulseDot = new QLabel();
    pulseDot->setFixedSize(8, 8);
    pulseDot->setStyleSheet(R"(
        QLabel {
            background: #00FF66;
            border-radius: 4px;
        }
    )");
    pulseDot->setAlignment(Qt::AlignCenter);

    // Pulse animation for the dot
    QPropertyAnimation* pulseAnim = new QPropertyAnimation(pulseDot, "geometry", this);
    pulseAnim->setDuration(1500);
    pulseAnim->setLoopCount(-1);
    pulseAnim->setKeyValueAt(0, QRect(0, 0, 8, 8));
    pulseAnim->setKeyValueAt(0.5, QRect(0, 0, 12, 12));
    pulseAnim->setKeyValueAt(1, QRect(0, 0, 8, 8));
    pulseAnim->start(QAbstractAnimation::DeleteWhenStopped);

    QLabel* statusText = new QLabel("SYSTEM ONLINE");
    statusText->setStyleSheet(R"(
        QLabel {
            color: #00FF66;
            font-family: 'Inter';
            font-size: 11px;
            font-weight: 800;
            letter-spacing: 0.1em;
        }
    )");

    badgeLayout->addWidget(pulseDot);
    badgeLayout->addWidget(statusText);
    badgeLayout->addStretch();

    heroLayout->addWidget(statusBadge, 0, Qt::AlignLeft);
    heroLayout->addStretch();

    // Title
    QLabel* title = new QLabel("Welcome Back.");
    title->setStyleSheet(R"(
        QLabel {
            color: #e5e2e1;
            font-size: 48px;
            font-weight: 800;
            letter-spacing: -0.02em;
            font-family: 'Inter';
        }
    )");
    heroLayout->addWidget(title, 0, Qt::AlignLeft);

    QLabel* subtitle = new QLabel("Your primary instance is ready. All systems nominal. Awaiting command.");
    subtitle->setStyleSheet(R"(
        QLabel {
            color: #c8c6c5;
            font-size: 16px;
            font-weight: 400;
            line-height: 1.6;
            font-family: 'Inter';
            max-width: 500px;
        }
    )");
    subtitle->setWordWrap(true);
    heroLayout->addWidget(subtitle, 0, Qt::AlignLeft);

    heroLayout->addStretch();

    // Play Card - Glassmorphism
    m_playCard = new QFrame();
    m_playCard->setFixedWidth(380);
    m_playCard->setStyleSheet(R"(
        QFrame {
            background: rgba(38, 38, 38, 0.8);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-top: 1px solid rgba(255, 255, 255, 0.2);
            border-radius: 12px;
        }
    )");
    m_playCard->setGraphicsEffect(new QGraphicsDropShadowEffect(m_playCard));
    if (auto* effect = qobject_cast<QGraphicsDropShadowEffect*>(m_playCard->graphicsEffect())) {
        effect->setBlurRadius(30);
        effect->setOffset(0, 10);
        effect->setColor(QColor(0, 0, 0, 180));
    }

    QVBoxLayout* cardLayout = new QVBoxLayout(m_playCard);
    cardLayout->setContentsMargins(32, 32, 32, 32);
    cardLayout->setSpacing(16);

    // Instance info row
    QHBoxLayout* infoRow = new QHBoxLayout();
    infoRow->setSpacing(12);

    QLabel* instanceName = new QLabel("PK Client");
    instanceName->setStyleSheet(R"(
        QLabel {
            color: #e5e2e1;
            font-size: 20px;
            font-weight: 700;
            font-family: 'Inter';
        }
    )");
    infoRow->addWidget(instanceName);

    infoRow->addStretch();

    QLabel* instanceIcon = new QLabel();
    instanceIcon->setFixedSize(48, 48);
    instanceIcon->setStyleSheet(R"(
        QLabel {
            background: rgba(255, 255, 255, 0.05);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 8px;
        }
    )");
    instanceIcon->setAlignment(Qt::AlignCenter);

    // Load logo from resources
    QPixmap logo(":/images/applogo.png");
    if (!logo.isNull()) {
        QPixmap scaled = logo.scaled(36, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        instanceIcon->setPixmap(scaled);
    } else {
        instanceIcon->setText("📦");
    }
    infoRow->addWidget(instanceIcon);

    cardLayout->addLayout(infoRow);

    QLabel* instanceVersion = new QLabel("Fabric 1.16.5");
    instanceVersion->setStyleSheet(R"(
        QLabel {
            color: #c8c6c5;
            font-family: 'JetBrains Mono';
            font-size: 13px;
            font-weight: 500;
        }
    )");
    cardLayout->addWidget(instanceVersion);

    cardLayout->addSpacing(24);

    // Play button with glow effect
    m_playButton = new QPushButton("Ready to Play");
    m_playButton->setFixedHeight(56);
    m_playButton->setCursor(Qt::PointingHandCursor);

    m_glowEffect = new QGraphicsOpacityEffect(m_playButton);
    m_glowEffect->setOpacity(0.3);
    m_playButton->setGraphicsEffect(m_glowEffect);

    m_playButton->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #FF0033, stop:1 #cc0029);
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 16px;
            font-weight: 800;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            font-family: 'Inter';
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #cc0029, stop:1 #b30024);
        }
        QPushButton:pressed {
            background: #b30024;
        }
    )");
    cardLayout->addWidget(m_playButton);

    // Add decorative glow element
    QFrame* glowDecor = new QFrame(m_playCard);
    glowDecor->setFixedSize(80, 80);
    glowDecor->move(300, -40);
    glowDecor->setStyleSheet(R"(
        QFrame {
            background: rgba(255, 0, 51, 0.1);
            border-radius: 40px;
        }
    )");
    glowDecor->lower();
    glowDecor->show();

    // Animate the decorative glow
    QPropertyAnimation* decorAnim = new QPropertyAnimation(glowDecor, "geometry", this);
    decorAnim->setDuration(4000);
    decorAnim->setLoopCount(-1);
    decorAnim->setKeyValueAt(0, QRect(300, -40, 80, 80));
    decorAnim->setKeyValueAt(0.5, QRect(280, -20, 120, 120));
    decorAnim->setKeyValueAt(1, QRect(300, -40, 80, 80));
    decorAnim->start(QAbstractAnimation::DeleteWhenStopped);

    heroLayout->addWidget(m_playCard, 0, Qt::AlignRight);

    layout->addWidget(m_heroWidget);

    // ========== TELEMETRY CARDS ==========
    QWidget* sectionHeader = new QWidget();
    QHBoxLayout* headerLayout = new QHBoxLayout(sectionHeader);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(12);

    QLabel* telemetryIcon = new QLabel("📊");
    telemetryIcon->setStyleSheet("font-size: 24px; color: #FF0033;");
    headerLayout->addWidget(telemetryIcon);

    QLabel* sectionTitle = new QLabel("Instance Telemetry");
    sectionTitle->setStyleSheet(R"(
        QLabel {
            color: #e5e2e1;
            font-size: 24px;
            font-weight: 600;
            font-family: 'Inter';
        }
    )");
    headerLayout->addWidget(sectionTitle);
    headerLayout->addStretch();

    layout->addWidget(sectionHeader);

    // Grid of 4 telemetry cards
    QWidget* gridWidget = new QWidget();
    QGridLayout* grid = new QGridLayout(gridWidget);
    grid->setSpacing(16);

    struct TelemetryCard {
        QString icon;
        QString iconBgColor;
        QString iconColor;
        QString tag;
        QString tagColor;
        QString label;
        QString value;
        QString valueColor;
    };

    QVector<TelemetryCard> cards = {
        {"💻", "#353534", "#c8c6c5", "ENV", "#353534", "Runtime", "Java 17", "#c8c6c5"},
        {"✅", "rgba(0, 255, 102, 0.1)", "#00FF66", "DISK", "#353534", "Integrity", "Verified", "#00FF66"},
        {"🧩", "#353534", "#c8c6c5", "LOADERS", "#353534", "Active Mods", "24 Loaded", "#e5e2e1"},
        {"🧠", "#353534", "#c8c6c5", "ALLOC", "#353534", "Memory Allocation", "4GB / 8GB", "#e5e2e1"}
    };

    for (int i = 0; i < cards.size(); ++i) {
        QFrame* card = new QFrame();
        card->setStyleSheet(QString(R"(
            QFrame {
                background: #1c1b1b;
                border: 1px solid #262626;
                border-radius: 8px;
            }
            QFrame:hover {
                border-color: rgba(255, 255, 255, 0.1);
            }
        )"));

        QVBoxLayout* cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(24, 24, 24, 24);
        cardLayout->setSpacing(12);

        QHBoxLayout* header = new QHBoxLayout();
        header->setSpacing(12);

        QLabel* iconLabel = new QLabel(cards[i].icon);
        iconLabel->setFixedSize(40, 40);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setStyleSheet(QString(R"(
            QLabel {
                background: %1;
                border-radius: 8px;
                color: %2;
                font-size: 18px;
            }
        )").arg(cards[i].iconBgColor, cards[i].iconColor));

        QLabel* tag = new QLabel(cards[i].tag);
        tag->setStyleSheet(QString(R"(
            QLabel {
                background: %1;
                color: %2;
                font-size: 11px;
                font-weight: 800;
                letter-spacing: 0.1em;
                font-family: 'Inter';
                padding: 4px 8px;
                border-radius: 4px;
            }
        )").arg(cards[i].tagColor).arg(cards[i].tagColor == "#353534" ? "#c8c6c5" : "#00FF66"));

        header->addWidget(iconLabel);
        header->addStretch();
        header->addWidget(tag);
        cardLayout->addLayout(header);

        QLabel* labelWidget = new QLabel(cards[i].label);
        labelWidget->setStyleSheet(R"(
            QLabel {
                color: #c8c6c5;
                font-family: 'JetBrains Mono';
                font-size: 12px;
                font-weight: 400;
            }
        )");
        cardLayout->addWidget(labelWidget);

        QLabel* valueLabel = new QLabel(cards[i].value);
        valueLabel->setStyleSheet(QString(R"(
            QLabel {
                color: %1;
                font-size: 24px;
                font-weight: 700;
                font-family: 'Inter';
            }
        )").arg(cards[i].valueColor));
        cardLayout->addWidget(valueLabel);

        grid->addWidget(card, 0, i);
    }

    layout->addWidget(gridWidget);

    // ========== BOTTOM STATUS BAR ==========
    QFrame* footer = new QFrame();
    footer->setFixedHeight(40);
    footer->setStyleSheet(R"(
        QFrame {
            background: #1c1b1b;
            border-top: 1px solid #353534;
        }
    )");

    QHBoxLayout* footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(16, 0, 16, 0);

    m_statusLabel = new QLabel("ℹ️  v1.20.1    ✅  Idle");
    m_statusLabel->setStyleSheet(R"(
        QLabel {
            color: #c8c6c5;
            font-family: 'JetBrains Mono';
            font-size: 10px;
            font-weight: 500;
        }
    )");
    footerLayout->addWidget(m_statusLabel);

    footerLayout->addStretch();

    QLabel* downloadLabel = new QLabel("📥  0 KB/s");
    downloadLabel->setStyleSheet(R"(
        QLabel {
            color: #c8c6c5;
            font-family: 'JetBrains Mono';
            font-size: 10px;
            font-weight: 500;
        }
    )");
    footerLayout->addWidget(downloadLabel);

    layout->addWidget(footer);

    scroll->setWidget(content);
    mainLayout->addWidget(scroll);
}

void HomePage::createHeroBackground()
{
    // Create a procedural gradient background similar to the reference
    m_heroBackground = QPixmap(800, 450);
    m_heroBackground.fill(Qt::transparent);

    QPainter painter(&m_heroBackground);
    painter.setRenderHint(QPainter::Antialiasing);

    // Base dark background
    QLinearGradient baseGrad(0, 0, 0, 450);
    baseGrad.setColorAt(0, QColor("#131313"));
    baseGrad.setColorAt(1, QColor("#0a0a0a"));
    painter.fillRect(m_heroBackground.rect(), baseGrad);

    // Add atmospheric gradient layers
    QLinearGradient atmGrad(0, 0, 0, 450);
    atmGrad.setColorAt(0, QColor(255, 0, 51, 15));
    atmGrad.setColorAt(0.5, QColor(255, 0, 51, 5));
    atmGrad.setColorAt(1, QColor(0, 0, 0, 0));
    painter.fillRect(m_heroBackground.rect(), atmGrad);

    // Add radial glow in corner
    QRadialGradient radial(600, 100, 300);
    radial.setColorAt(0, QColor(255, 0, 51, 20));
    radial.setColorAt(1, QColor(255, 0, 51, 0));
    painter.setBrush(radial);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(600, 100, 300, 300);

    // Add mountain-like silhouette
    QLinearGradient mountainGrad(0, 300, 800, 450);
    mountainGrad.setColorAt(0, QColor(0, 0, 0, 180));
    mountainGrad.setColorAt(1, QColor(0, 0, 0, 220));
    painter.fillRect(0, 300, 800, 150, mountainGrad);

    painter.end();

    m_hasHeroBackground = true;
    m_heroWidget->update();
}

void HomePage::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
}

void HomePage::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (m_heroWidget) {
        m_heroWidget->update();
    }
}