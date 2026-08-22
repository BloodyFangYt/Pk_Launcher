#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>

struct UserProfile {
    QString id;
    QString email;
    QString username;
    QString avatarUrl;
    bool isVerified = false;
    bool isPremium = false;
    QDateTime premiumUntil;
    int coinBalance = 0;
};

class AuthManager : public QObject
{
    Q_OBJECT

public:
    explicit AuthManager(QObject* parent = nullptr);
    ~AuthManager();

    bool isLoggedIn() const { return !m_userProfile.username.isEmpty(); }
    bool isOffline() const { return m_userProfile.email == "Offline Mode"; }
    bool isMicrosoftAuth() const { return m_isMicrosoftAuth; }
    QString accessToken() const { return m_accessToken; }
    QString refreshToken() const { return m_refreshToken; }
    UserProfile userProfile() const { return m_userProfile; }

    void login(const QString& email, const QString& password);
    void registerAccount(const QString& email, const QString& username, const QString& password);
    void logout();
    void refreshAccessToken();
    void microsoftOAuth();
    void offlineLogin(const QString& username);

signals:
    void loginSuccess(const UserProfile& user, const QString& accessToken);
    void loginFailed(const QString& error);
    void logoutComplete();
    void microsoftAuthStarted();
    void microsoftAuthComplete(const QString& minecraftToken);
    void microsoftAuthFailed(const QString& error);

private slots:
    void onLoginReply(QNetworkReply* reply);
    void onRegisterReply(QNetworkReply* reply);
    void onRefreshReply(QNetworkReply* reply);
    void onMeReply(QNetworkReply* reply);
    void onMicrosoftAuthCallback();

private:
    QNetworkAccessManager* m_networkManager;
    QString m_apiBaseUrl = "http://localhost:3001/api/v1";
    QString m_accessToken;
    QString m_refreshToken;
    UserProfile m_userProfile;

    // Microsoft OAuth (PKCE)
    QTcpServer* m_oauthServer = nullptr;
    QString m_microsoftClientId;
    QString m_microsoftRedirectUri;
    QString m_oauthState;
    QString m_codeVerifier;
    QString m_microsoftRefreshToken;
    bool m_isMicrosoftAuth = false;

    void startLocalServer();
    QString generateCodeVerifier();
    QString generateCodeChallenge(const QString& verifier);
    QString generateState();
    void exchangeCodeForToken(const QString& code);
    void getXboxToken(const QString& msAccessToken);
    void getXstsToken(const QString& xboxToken, const QString& uhs);
    void getMinecraftToken(const QString& xstsToken, const QString& uhs);
    void validateMinecraftAccount(const QString& mcAccessToken);
    void refreshMicrosoftToken();
    void cleanupOAuthServer();

    void saveTokens();
    void loadTokens();
    void clearTokens();
};