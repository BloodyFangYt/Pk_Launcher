#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>

class AuthManager;

class AuthPage : public QWidget
{
    Q_OBJECT

public:
    explicit AuthPage(AuthManager* authManager, QWidget* parent = nullptr);

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void onMicrosoftLoginClicked();
    void onLogoutClicked();
    void onOfflineLoginClicked();
    void onLoginSuccess();
    void onLoginFailed(const QString& error);
    void onLogoutComplete();
    void onMicrosoftAuthStarted();
    void onMicrosoftAuthComplete(const QString& token);
    void onMicrosoftAuthFailed(const QString& error);

private:
    void setupLoginView();
    void setupProfileView();
    void setStatus(const QString& text, bool isError = false);

    AuthManager* m_authManager;

    // Login view
    QStackedWidget* m_stack = nullptr;
    QWidget* m_loginWidget = nullptr;
    QWidget* m_profileWidget = nullptr;

    QLineEdit* m_emailEdit = nullptr;
    QLineEdit* m_passwordEdit = nullptr;
    QLineEdit* m_offlineEdit = nullptr;
    QPushButton* m_loginButton = nullptr;
    QPushButton* m_registerButton = nullptr;
    QPushButton* m_microsoftButton = nullptr;
    QPushButton* m_offlineButton = nullptr;
    QLabel* m_statusLabel = nullptr;

    // Profile view
    QLabel* m_profileUsername = nullptr;
    QLabel* m_profileEmail = nullptr;
    QLabel* m_profileStatus = nullptr;
    QPushButton* m_logoutButton = nullptr;
};
