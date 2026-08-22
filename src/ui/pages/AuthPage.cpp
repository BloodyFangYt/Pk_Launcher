#include "ui/pages/AuthPage.h"
#include "core/AuthManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>

static const QString ACCENT = "#FF0033";
static const QString BG_CARD = "#1c1b1b";
static const QString BG_INPUT = "#262626";
static const QString TEXT_PRIMARY = "#e5e2e1";
static const QString TEXT_SECONDARY = "#8a8886";
static const QString TEXT_ERROR = "#FF4444";
static const QString TEXT_SUCCESS = "#44FF44";
static const QString BORDER = "#353534";

AuthPage::AuthPage(AuthManager* authManager, QWidget* parent)
    : QWidget(parent), m_authManager(authManager)
{
    m_stack = new QStackedWidget(this);

    setupLoginView();
    setupProfileView();

    m_stack->addWidget(m_loginWidget);
    m_stack->addWidget(m_profileWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_stack);

    // Connect signals
    connect(m_authManager, &AuthManager::loginSuccess, this, &AuthPage::onLoginSuccess);
    connect(m_authManager, &AuthManager::loginFailed, this, &AuthPage::onLoginFailed);
    connect(m_authManager, &AuthManager::logoutComplete, this, &AuthPage::onLogoutComplete);
    connect(m_authManager, &AuthManager::microsoftAuthStarted, this, &AuthPage::onMicrosoftAuthStarted);
    connect(m_authManager, &AuthManager::microsoftAuthComplete, this, &AuthPage::onMicrosoftAuthComplete);
    connect(m_authManager, &AuthManager::microsoftAuthFailed, this, &AuthPage::onMicrosoftAuthFailed);

    // Show correct view on startup
    if (m_authManager->isLoggedIn()) {
        m_stack->setCurrentIndex(1); // Profile
    } else {
        m_stack->setCurrentIndex(0); // Login
    }
}

void AuthPage::setupLoginView()
{
    m_loginWidget = new QWidget();

    // Center the form
    QVBoxLayout* outerLayout = new QVBoxLayout(m_loginWidget);
    outerLayout->addStretch();

    // Card container
    QFrame* card = new QFrame();
    card->setMaximumWidth(420);
    card->setStyleSheet(QString(R"(
        QFrame {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 12px;
            padding: 32px;
        }
    )").arg(BG_CARD, BORDER));
    card->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(24);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 80));
    card->setGraphicsEffect(shadow);

    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setSpacing(16);

    // Title
    QLabel* title = new QLabel("Sign In");
    title->setStyleSheet(QString(R"(
        color: %1;
        font-size: 24px;
        font-weight: bold;
        font-family: 'Inter';
    )").arg(TEXT_PRIMARY));
    title->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(title);

    QLabel* subtitle = new QLabel("Access your Minecraft account");
    subtitle->setStyleSheet(QString(R"(
        color: %2;
        font-size: 13px;
        font-family: 'Inter';
    )").arg(TEXT_PRIMARY, TEXT_SECONDARY));
    subtitle->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(subtitle);

    cardLayout->addSpacing(8);

    // Microsoft OAuth button
    m_microsoftButton = new QPushButton("  Sign in with Microsoft");
    m_microsoftButton->setMinimumHeight(44);
    m_microsoftButton->setStyleSheet(QString(R"(
        QPushButton {
            background-color: #2F2F2F;
            color: %1;
            border: 1px solid %2;
            border-radius: 6px;
            padding: 10px 20px;
            font-size: 14px;
            font-weight: bold;
            font-family: 'Inter';
        }
        QPushButton:hover {
            background-color: #3A3A3A;
            border-color: #555;
        }
        QPushButton:pressed {
            background-color: #252525;
        }
    )").arg(TEXT_PRIMARY, BORDER));
    cardLayout->addWidget(m_microsoftButton);

    // Divider
    QHBoxLayout* divLayout = new QHBoxLayout();
    divLayout->setSpacing(12);
    QLabel* divLine1 = new QLabel();
    divLine1->setFixedHeight(1);
    divLine1->setStyleSheet(QString("background-color: %1;").arg(BORDER));
    QLabel* divText = new QLabel("or");
    divText->setStyleSheet(QString("color: %1; font-size: 12px;").arg(TEXT_SECONDARY));
    divText->setAlignment(Qt::AlignCenter);
    QLabel* divLine2 = new QLabel();
    divLine2->setFixedHeight(1);
    divLine2->setStyleSheet(QString("background-color: %1;").arg(BORDER));
    divLayout->addWidget(divLine1);
    divLayout->addWidget(divText);
    divLayout->addWidget(divLine2);
    cardLayout->addLayout(divLayout);

    // Email field
    QLabel* emailLabel = new QLabel("Email");
    emailLabel->setStyleSheet(QString("color: %1; font-size: 12px; font-family: 'Inter';").arg(TEXT_SECONDARY));
    cardLayout->addWidget(emailLabel);

    m_emailEdit = new QLineEdit();
    m_emailEdit->setPlaceholderText("your@email.com");
    m_emailEdit->setMinimumHeight(40);
    m_emailEdit->setStyleSheet(QString(R"(
        QLineEdit {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 6px;
            padding: 8px 12px;
            color: %3;
            font-size: 14px;
            font-family: 'Inter';
        }
        QLineEdit:focus {
            border-color: %4;
        }
    )").arg(BG_INPUT, BORDER, TEXT_PRIMARY, ACCENT));
    m_emailEdit->setEchoMode(QLineEdit::Normal);
    cardLayout->addWidget(m_emailEdit);

    // Password field
    QLabel* passwordLabel = new QLabel("Password");
    passwordLabel->setStyleSheet(QString("color: %1; font-size: 12px; font-family: 'Inter';").arg(TEXT_SECONDARY));
    cardLayout->addWidget(passwordLabel);

    m_passwordEdit = new QLineEdit();
    m_passwordEdit->setPlaceholderText("Enter your password");
    m_passwordEdit->setMinimumHeight(40);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setStyleSheet(m_emailEdit->styleSheet());
    cardLayout->addWidget(m_passwordEdit);

    cardLayout->addSpacing(4);

    // Login button
    m_loginButton = new QPushButton("Sign In");
    m_loginButton->setMinimumHeight(44);
    m_loginButton->setStyleSheet(QString(R"(
        QPushButton {
            background-color: %1;
            color: white;
            border: none;
            border-radius: 6px;
            padding: 10px 20px;
            font-size: 14px;
            font-weight: bold;
            font-family: 'Inter';
        }
        QPushButton:hover {
            background-color: #cc0029;
        }
        QPushButton:disabled {
            background-color: #353534;
            color: #474746;
        }
    )").arg(ACCENT));
    cardLayout->addWidget(m_loginButton);

    // Register link
    m_registerButton = new QPushButton("Don't have an account? Create one");
    m_registerButton->setFlat(true);
    m_registerButton->setStyleSheet(QString(R"(
        QPushButton {
            background: transparent;
            color: %1;
            border: none;
            font-size: 12px;
            font-family: 'Inter';
            text-decoration: underline;
        }
        QPushButton:hover {
            color: %2;
        }
    )").arg(TEXT_SECONDARY, ACCENT));
    cardLayout->addWidget(m_registerButton);

    cardLayout->addSpacing(12);

    // ── Offline Mode Section ───────────────────────────────────────
    QHBoxLayout* offlineDivLayout = new QHBoxLayout();
    offlineDivLayout->setSpacing(12);
    QLabel* offlineDivLine1 = new QLabel();
    offlineDivLine1->setFixedHeight(1);
    offlineDivLine1->setStyleSheet(QString("background-color: %1;").arg(BORDER));
    QLabel* offlineDivText = new QLabel("or play offline");
    offlineDivText->setStyleSheet(QString("color: %1; font-size: 12px; font-family: 'Inter';").arg(TEXT_SECONDARY));
    offlineDivText->setAlignment(Qt::AlignCenter);
    QLabel* offlineDivLine2 = new QLabel();
    offlineDivLine2->setFixedHeight(1);
    offlineDivLine2->setStyleSheet(QString("background-color: %1;").arg(BORDER));
    offlineDivLayout->addWidget(offlineDivLine1);
    offlineDivLayout->addWidget(offlineDivText);
    offlineDivLayout->addWidget(offlineDivLine2);
    cardLayout->addLayout(offlineDivLayout);

    // Offline username field
    QLabel* offlineLabel = new QLabel("Username");
    offlineLabel->setStyleSheet(QString("color: %1; font-size: 12px; font-family: 'Inter';").arg(TEXT_SECONDARY));
    cardLayout->addWidget(offlineLabel);

    m_offlineEdit = new QLineEdit();
    m_offlineEdit->setPlaceholderText("Enter a username to play offline");
    m_offlineEdit->setMaxLength(16);
    m_offlineEdit->setMinimumHeight(40);
    m_offlineEdit->setStyleSheet(QString(R"(
        QLineEdit {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 6px;
            padding: 8px 12px;
            color: %3;
            font-size: 14px;
            font-family: 'Inter';
        }
        QLineEdit:focus {
            border-color: %4;
        }
    )").arg(BG_INPUT, BORDER, TEXT_PRIMARY, ACCENT));
    cardLayout->addWidget(m_offlineEdit);

    // Offline play button
    m_offlineButton = new QPushButton("Play Offline");
    m_offlineButton->setMinimumHeight(44);
    m_offlineButton->setStyleSheet(QString(R"(
        QPushButton {
            background-color: #2F2F2F;
            color: %1;
            border: 1px solid %2;
            border-radius: 6px;
            padding: 10px 20px;
            font-size: 14px;
            font-weight: bold;
            font-family: 'Inter';
        }
        QPushButton:hover {
            background-color: #3A3A3A;
            border-color: #555;
        }
        QPushButton:pressed {
            background-color: #252525;
        }
    )").arg(TEXT_PRIMARY, BORDER));
    cardLayout->addWidget(m_offlineButton);

    // Status label (hidden by default)
    m_statusLabel = new QLabel();
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setStyleSheet(QString(R"(
        color: %1;
        font-size: 12px;
        font-family: 'Inter';
        padding: 8px;
        border-radius: 4px;
    )").arg(TEXT_ERROR));
    m_statusLabel->hide();
    cardLayout->addWidget(m_statusLabel);

    outerLayout->addWidget(card, 0, Qt::AlignCenter);
    outerLayout->addStretch();

    // Connect buttons
    connect(m_loginButton, &QPushButton::clicked, this, &AuthPage::onLoginClicked);
    connect(m_registerButton, &QPushButton::clicked, this, &AuthPage::onRegisterClicked);
    connect(m_microsoftButton, &QPushButton::clicked, this, &AuthPage::onMicrosoftLoginClicked);
    connect(m_offlineButton, &QPushButton::clicked, this, &AuthPage::onOfflineLoginClicked);
    connect(m_offlineEdit, &QLineEdit::returnPressed, this, &AuthPage::onOfflineLoginClicked);
    connect(m_passwordEdit, &QLineEdit::returnPressed, this, &AuthPage::onLoginClicked);
}

void AuthPage::setupProfileView()
{
    m_profileWidget = new QWidget();

    QVBoxLayout* outerLayout = new QVBoxLayout(m_profileWidget);
    outerLayout->addStretch();

    // Card container
    QFrame* card = new QFrame();
    card->setMaximumWidth(420);
    card->setStyleSheet(QString(R"(
        QFrame {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 12px;
            padding: 32px;
        }
    )").arg(BG_CARD, BORDER));
    card->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(24);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0, 0, 0, 80));
    card->setGraphicsEffect(shadow);

    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setSpacing(16);

    // Title
    QLabel* title = new QLabel("Profile");
    title->setStyleSheet(QString(R"(
        color: %1;
        font-size: 24px;
        font-weight: bold;
        font-family: 'Inter';
    )").arg(TEXT_PRIMARY));
    title->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(title);

    cardLayout->addSpacing(8);

    // Username
    QLabel* usernameTitle = new QLabel("Username");
    usernameTitle->setStyleSheet(QString("color: %1; font-size: 12px; font-family: 'Inter';").arg(TEXT_SECONDARY));
    cardLayout->addWidget(usernameTitle);

    m_profileUsername = new QLabel("—");
    m_profileUsername->setStyleSheet(QString(R"(
        color: %1;
        font-size: 18px;
        font-weight: bold;
        font-family: 'Inter';
        padding: 8px 12px;
        background-color: %2;
        border: 1px solid %3;
        border-radius: 6px;
    )").arg(TEXT_PRIMARY, BG_INPUT, BORDER));
    cardLayout->addWidget(m_profileUsername);

    // Email
    QLabel* emailTitle = new QLabel("Account");
    emailTitle->setStyleSheet(QString("color: %1; font-size: 12px; font-family: 'Inter';").arg(TEXT_SECONDARY));
    cardLayout->addWidget(emailTitle);

    m_profileEmail = new QLabel("—");
    m_profileEmail->setStyleSheet(QString(R"(
        color: %1;
        font-size: 14px;
        font-family: 'Inter';
        padding: 8px 12px;
        background-color: %2;
        border: 1px solid %3;
        border-radius: 6px;
    )").arg(TEXT_PRIMARY, BG_INPUT, BORDER));
    cardLayout->addWidget(m_profileEmail);

    // Status
    m_profileStatus = new QLabel("✓ Signed in");
    m_profileStatus->setStyleSheet(QString(R"(
        color: %1;
        font-size: 13px;
        font-family: 'Inter';
        padding: 8px;
        text-align: center;
    )").arg(TEXT_SUCCESS));
    m_profileStatus->setAlignment(Qt::AlignCenter);
    cardLayout->addWidget(m_profileStatus);

    cardLayout->addSpacing(8);

    // Logout button
    m_logoutButton = new QPushButton("Sign Out");
    m_logoutButton->setMinimumHeight(44);
    m_logoutButton->setStyleSheet(QString(R"(
        QPushButton {
            background-color: #2F2F2F;
            color: %1;
            border: 1px solid %2;
            border-radius: 6px;
            padding: 10px 20px;
            font-size: 14px;
            font-weight: bold;
            font-family: 'Inter';
        }
        QPushButton:hover {
            background-color: #3A3A3A;
            border-color: #555;
        }
        QPushButton:pressed {
            background-color: #252525;
        }
    )").arg(TEXT_PRIMARY, BORDER));
    cardLayout->addWidget(m_logoutButton);

    outerLayout->addWidget(card, 0, Qt::AlignCenter);
    outerLayout->addStretch();

    connect(m_logoutButton, &QPushButton::clicked, this, &AuthPage::onLogoutClicked);
}

void AuthPage::onLoginClicked()
{
    QString email = m_emailEdit->text().trimmed();
    QString password = m_passwordEdit->text();

    if (email.isEmpty() || password.isEmpty()) {
        setStatus("Please enter email and password.", true);
        return;
    }

    m_loginButton->setEnabled(false);
    m_registerButton->setEnabled(false);
    m_microsoftButton->setEnabled(false);
    setStatus("Signing in...", false);

    m_authManager->login(email, password);
}

void AuthPage::onRegisterClicked()
{
    QString email = m_emailEdit->text().trimmed();
    QString password = m_passwordEdit->text();

    if (email.isEmpty() || password.isEmpty()) {
        setStatus("Please enter email and password to register.", true);
        return;
    }

    // Use email prefix as username
    QString username = email.split('@').first();

    m_loginButton->setEnabled(false);
    m_registerButton->setEnabled(false);
    m_microsoftButton->setEnabled(false);
    setStatus("Creating account...", false);

    m_authManager->registerAccount(email, username, password);
}

void AuthPage::onMicrosoftLoginClicked()
{
    m_loginButton->setEnabled(false);
    m_registerButton->setEnabled(false);
    m_microsoftButton->setEnabled(false);
    setStatus("Opening browser for Microsoft sign-in...", false);

    m_authManager->microsoftOAuth();
}

void AuthPage::onOfflineLoginClicked()
{
    QString username = m_offlineEdit->text().trimmed();

    if (username.isEmpty()) {
        setStatus("Please enter a username.", true);
        return;
    }

    m_loginButton->setEnabled(false);
    m_registerButton->setEnabled(false);
    m_microsoftButton->setEnabled(false);
    m_offlineButton->setEnabled(false);
    setStatus("Starting offline mode...", false);

    m_authManager->offlineLogin(username);
}

void AuthPage::onLogoutClicked()
{
    m_authManager->logout();
}

void AuthPage::onLoginSuccess()
{
    UserProfile profile = m_authManager->userProfile();

    m_profileUsername->setText(profile.username.isEmpty() ? "—" : profile.username);
    m_profileEmail->setText(profile.email.isEmpty() ? "—" : profile.email);
    m_profileStatus->setText(profile.isPremium ? "✓ Signed in — Premium" : "✓ Signed in");

    m_stack->setCurrentIndex(1); // Profile view

    // Reset login form
    m_emailEdit->clear();
    m_passwordEdit->clear();
    m_offlineEdit->clear();
    m_statusLabel->hide();
}

void AuthPage::onLoginFailed(const QString& error)
{
    setStatus(error, true);
    m_loginButton->setEnabled(true);
    m_registerButton->setEnabled(true);
    m_microsoftButton->setEnabled(true);
    m_offlineButton->setEnabled(true);
}

void AuthPage::onLogoutComplete()
{
    m_stack->setCurrentIndex(0); // Login view
    m_loginButton->setEnabled(true);
    m_registerButton->setEnabled(true);
    m_microsoftButton->setEnabled(true);
    m_offlineButton->setEnabled(true);
    m_statusLabel->hide();
}

void AuthPage::onMicrosoftAuthStarted()
{
    setStatus("Browser opened. Complete sign-in in your browser.", false);
}

void AuthPage::onMicrosoftAuthComplete(const QString& /*token*/)
{
    // onLoginSuccess will handle the UI update
}

void AuthPage::onMicrosoftAuthFailed(const QString& error)
{
    setStatus(error, true);
    m_loginButton->setEnabled(true);
    m_registerButton->setEnabled(true);
    m_microsoftButton->setEnabled(true);
    m_offlineButton->setEnabled(true);
}

void AuthPage::setStatus(const QString& text, bool isError)
{
    m_statusLabel->setText(text);
    m_statusLabel->setStyleSheet(QString(R"(
        color: %1;
        font-size: 12px;
        font-family: 'Inter';
        padding: 8px;
        border-radius: 4px;
        background-color: %2;
    )").arg(isError ? TEXT_ERROR : TEXT_SECONDARY,
           isError ? "rgba(255, 68, 68, 0.1)" : "rgba(138, 136, 134, 0.1)"));
    m_statusLabel->show();
}
