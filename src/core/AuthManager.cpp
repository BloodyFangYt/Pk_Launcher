#include "core/AuthManager.h"
#include "core/Settings.h"

#include <QNetworkRequest>
#include <QUrlQuery>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRandomGenerator>
#include <QDesktopServices>
#include <QDebug>
#include <QCoreApplication>

AuthManager::AuthManager(QObject* parent) : QObject(parent)
{
    m_networkManager = new QNetworkAccessManager(this);
    loadTokens();
}

AuthManager::~AuthManager()
{
    cleanupOAuthServer();
    m_networkManager->deleteLater();
}

// ─── Existing email/password auth ───────────────────────────────────────────

void AuthManager::login(const QString& email, const QString& password)
{
    QNetworkRequest request(m_apiBaseUrl + "/auth/login");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    body["email"] = email;
    body["password"] = password;

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onLoginReply(reply); });
}

void AuthManager::registerAccount(const QString& email, const QString& username, const QString& password)
{
    QNetworkRequest request(m_apiBaseUrl + "/auth/register");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    body["email"] = email;
    body["username"] = username;
    body["password"] = password;

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onRegisterReply(reply); });
}

void AuthManager::logout()
{
    // If we have a backend refresh token, notify the backend
    if (!m_refreshToken.isEmpty() && !m_isMicrosoftAuth) {
        QNetworkRequest request(m_apiBaseUrl + "/auth/logout");
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("Authorization", "Bearer " + m_accessToken.toUtf8());

        QJsonObject body;
        body["refreshToken"] = m_refreshToken;

        QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(body).toJson());
        connect(reply, &QNetworkReply::finished, [reply]() { reply->deleteLater(); });
    }

    clearTokens();
    emit logoutComplete();
}

void AuthManager::refreshAccessToken()
{
    if (m_refreshToken.isEmpty() && m_microsoftRefreshToken.isEmpty())
        return;

    // If this was a Microsoft auth, refresh using Microsoft's token endpoint
    if (m_isMicrosoftAuth && !m_microsoftRefreshToken.isEmpty()) {
        refreshMicrosoftToken();
        return;
    }

    // Backend refresh
    if (m_refreshToken.isEmpty()) return;

    QNetworkRequest request(m_apiBaseUrl + "/auth/refresh");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    body["refreshToken"] = m_refreshToken;

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onRefreshReply(reply); });
}

// ─── Microsoft OAuth (PKCE) ────────────────────────────────────────────────

// Microsoft OAuth constants
static const QString MS_CLIENT_ID = "00000000402b5328"; // Official Minecraft launcher client ID
static const QString MS_AUTH_ENDPOINT = "https://login.live.com/oauth20_authorize.srf";
static const QString MS_TOKEN_ENDPOINT = "https://login.live.com/oauth20_token.srf";
static const QString MS_REDIRECT_PATH = "/callback";
static const QString XBOX_AUTH_ENDPOINT = "https://user.auth.xboxlive.com/user/authenticate";
static const QString XBOX_XSTS_ENDPOINT = "https://xsts.auth.xboxlive.com/xsts/authorize";
static const QString MC_LOGIN_WITH_XBOX = "https://api.minecraftservices.com/authentication/login_with_xbox";
static const QString MC_PROFILE_ENDPOINT = "https://api.minecraftservices.com/minecraft/profile";
static const QStringList MS_SCOPES = {"service::user.auth.xboxlive.com::MBI_SSL"};

void AuthManager::microsoftOAuth()
{
    // Clean up any previous server
    cleanupOAuthServer();

    // Generate PKCE parameters
    m_codeVerifier = generateCodeVerifier();
    QString codeChallenge = generateCodeChallenge(m_codeVerifier);
    m_oauthState = generateState();

    // Start local HTTP server to receive the callback
    startLocalServer();
    if (!m_oauthServer || !m_oauthServer->isListening()) {
        emit microsoftAuthFailed("Failed to start local server for OAuth callback");
        return;
    }

    int port = m_oauthServer->serverPort();
    m_microsoftRedirectUri = QString("http://127.0.0.1:%1%2").arg(port).arg(MS_REDIRECT_PATH);
    m_microsoftClientId = MS_CLIENT_ID;

    // Build authorization URL
    QUrl authUrl(MS_AUTH_ENDPOINT);
    QUrlQuery params;
    params.addQueryItem("client_id", m_microsoftClientId);
    params.addQueryItem("response_type", "code");
    params.addQueryItem("scope", MS_SCOPES.join(" "));
    params.addQueryItem("redirect_uri", m_microsoftRedirectUri);
    params.addQueryItem("state", m_oauthState);
    params.addQueryItem("code_challenge", codeChallenge);
    params.addQueryItem("code_challenge_method", "S256");
    authUrl.setQuery(params);

    qDebug() << "Opening browser for Microsoft OAuth:" << authUrl.toString();
    emit microsoftAuthStarted();

    // Open system browser
    QDesktopServices::openUrl(authUrl);
}

void AuthManager::offlineLogin(const QString& username)
{
    QString trimmed = username.trimmed();
    if (trimmed.isEmpty()) {
        emit loginFailed("Username cannot be empty");
        return;
    }

    if (trimmed.length() > 16) {
        emit loginFailed("Username must be 16 characters or less");
        return;
    }

    // Generate deterministic UUID from username (same as Minecraft does for offline)
    QByteArray hash = QCryptographicHash::hash(
        ("OfflinePlayer:" + trimmed).toUtf8(),
        QCryptographicHash::Md5
    ).toHex();

    // Format as UUID: 8-4-4-4-12
    QString uuid;
    uuid += QString::fromLatin1(hash.mid(0, 8));
    uuid += "-";
    uuid += QString::fromLatin1(hash.mid(8, 4));
    uuid += "-";
    uuid += QString::fromLatin1(hash.mid(12, 4));
    uuid += "-";
    uuid += QString::fromLatin1(hash.mid(16, 4));
    uuid += "-";
    uuid += QString::fromLatin1(hash.mid(20, 12));

    // Populate profile
    m_userProfile.id = uuid;
    m_userProfile.username = trimmed;
    m_userProfile.email = "Offline Mode";
    m_userProfile.isPremium = false;

    // Clear any existing tokens — offline mode has no token
    m_accessToken.clear();
    m_refreshToken.clear();
    m_microsoftRefreshToken.clear();
    m_isMicrosoftAuth = false;

    // Save the offline profile so it persists across restarts
    saveTokens();

    qDebug() << "Offline login as:" << trimmed << "UUID:" << uuid;
    emit loginSuccess(m_userProfile, m_accessToken);
}

void AuthManager::startLocalServer()
{
    m_oauthServer = new QTcpServer(this);

    if (!m_oauthServer->listen(QHostAddress::LocalHost, 0)) {
        qWarning() << "Failed to start OAuth callback server:" << m_oauthServer->errorString();
        return;
    }

    qDebug() << "OAuth callback server listening on port" << m_oauthServer->serverPort();

    connect(m_oauthServer, &QTcpServer::newConnection, this, &AuthManager::onMicrosoftAuthCallback);
}

void AuthManager::onMicrosoftAuthCallback()
{
    if (!m_oauthServer || !m_oauthServer->hasPendingConnections())
        return;

    QTcpSocket* socket = m_oauthServer->nextPendingConnection();

    // Read the HTTP request
    socket->waitForReadyRead(3000);
    QByteArray request = socket->readAll();

    // Parse the request line
    QByteArray requestLine = request.split('\r').first().split('\n').first();
    QList<QByteArray> parts = requestLine.split(' ');
    QString path;
    if (parts.size() >= 2) {
        path = QString::fromUtf8(parts[1]);
    }

    qDebug() << "OAuth callback received:" << path;

    // Send response to browser
    QByteArray httpResponse;
    QString body;

    if (path.startsWith(MS_REDIRECT_PATH)) {
        QUrl callbackUrl("http://127.0.0.1" + path);
        QUrlQuery query(callbackUrl);

        QString error = query.queryItemValue("error");
        QString errorDesc = query.queryItemValue("error_description");
        QString code = query.queryItemValue("code");
        QString state = query.queryItemValue("state");

        if (!error.isEmpty()) {
            body = "<html><body><h2>Authentication Failed</h2><p>" + errorDesc.toHtmlEscaped() + "</p>"
                   "<p>You can close this window.</p></body></html>";
            httpResponse = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
                          "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body.toUtf8();
            socket->write(httpResponse);
            socket->flush();
            socket->waitForBytesWritten(2000);
            socket->disconnectFromHost();

            emit microsoftAuthFailed(error + ": " + errorDesc);
            cleanupOAuthServer();
            return;
        }

        if (state != m_oauthState) {
            body = "<html><body><h2>Authentication Failed</h2><p>State mismatch. Try again.</p></body></html>";
            httpResponse = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
                          "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body.toUtf8();
            socket->write(httpResponse);
            socket->flush();
            socket->waitForBytesWritten(2000);
            socket->disconnectFromHost();

            emit microsoftAuthFailed("OAuth state mismatch — possible CSRF attack");
            cleanupOAuthServer();
            return;
        }

        if (code.isEmpty()) {
            body = "<html><body><h2>Authentication Failed</h2><p>No authorization code received.</p></body></html>";
            httpResponse = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
                          "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body.toUtf8();
            socket->write(httpResponse);
            socket->flush();
            socket->waitForBytesWritten(2000);
            socket->disconnectFromHost();

            emit microsoftAuthFailed("No authorization code received");
            cleanupOAuthServer();
            return;
        }

        // Success page
        body = "<html><body><h2>Authentication Successful!</h2>"
               "<p>You can close this window and return to PK Launcher.</p></body></html>";
        httpResponse = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n"
                      "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body.toUtf8();
        socket->write(httpResponse);
        socket->flush();
        socket->waitForBytesWritten(2000);
        socket->disconnectFromHost();

        // Close the server — we only need one callback
        cleanupOAuthServer();

        // Exchange the authorization code for tokens
        exchangeCodeForToken(code);
    } else {
        body = "<html><body><h2>Not Found</h2></body></html>";
        httpResponse = "HTTP/1.1 404 Not Found\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body.toUtf8();
        socket->write(httpResponse);
        socket->flush();
        socket->waitForBytesWritten(2000);
        socket->disconnectFromHost();
    }
}

void AuthManager::exchangeCodeForToken(const QString& code)
{
    qDebug() << "Exchanging authorization code for Microsoft access token...";

    QUrl tokenUrl(MS_TOKEN_ENDPOINT);
    QUrlQuery params;
    params.addQueryItem("client_id", m_microsoftClientId);
    params.addQueryItem("code", code);
    params.addQueryItem("grant_type", "authorization_code");
    params.addQueryItem("redirect_uri", m_microsoftRedirectUri);
    params.addQueryItem("scope", MS_SCOPES.join(" "));
    params.addQueryItem("code_verifier", m_codeVerifier);

    QNetworkRequest request(tokenUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply* reply = m_networkManager->post(request, params.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "Microsoft token exchange failed:" << reply->errorString();
            emit microsoftAuthFailed("Microsoft token exchange failed: " + reply->errorString());
            reply->deleteLater();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();

        QString msAccessToken = obj.value("access_token").toString();
        m_microsoftRefreshToken = obj.value("refresh_token").toString();

        if (msAccessToken.isEmpty()) {
            QString error = obj.value("error").toString();
            QString errorDesc = obj.value("error_description").toString();
            qWarning() << "Microsoft token error:" << error << errorDesc;
            emit microsoftAuthFailed("Microsoft auth failed: " + errorDesc);
            reply->deleteLater();
            return;
        }

        qDebug() << "Microsoft access token obtained, fetching Xbox Live token...";
        reply->deleteLater();

        // Next step: get Xbox Live token
        getXboxToken(msAccessToken);
    });
}

void AuthManager::getXboxToken(const QString& msAccessToken)
{
    qDebug() << "Fetching Xbox Live token...";

    QJsonObject props;
    props["AuthMethod"] = "RPS";
    props["SiteName"] = "user.auth.xboxlive.com";
    props["RpsTicket"] = msAccessToken;

    QJsonObject body;
    body["Properties"] = props;
    body["RelyingParty"] = "http://auth.xboxlive.com";
    body["TokenType"] = "JWT";

    QUrl xboxAuthUrl(XBOX_AUTH_ENDPOINT);
    QNetworkRequest request(xboxAuthUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {            if (reply->error() != QNetworkReply::NoError) {
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            qWarning() << "Xbox Live auth failed (HTTP" << statusCode << "):" << reply->errorString();
            emit microsoftAuthFailed("Xbox Live authentication failed. Please check your Xbox account status.");
            reply->deleteLater();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();

        QString xboxToken = obj.value("Token").toString();
        QJsonArray xui = obj.value("DisplayClaims").toObject().value("xui").toArray();

        if (xboxToken.isEmpty()) {
            qWarning() << "No Xbox Live token received — response missing Token field";
            emit microsoftAuthFailed("Xbox Live did not return a valid token. Please ensure your Xbox account is set up correctly.");
            reply->deleteLater();
            return;
        }

        // Extract user hash (UHS) from display claims
        QString uhs;
        if (!xui.isEmpty()) {
            uhs = xui.first().toObject().value("uhs").toString();
        }

        qDebug() << "Xbox Live token obtained, fetching XSTS token...";
        reply->deleteLater();

        getXstsToken(xboxToken, uhs);
    });
}

void AuthManager::getXstsToken(const QString& xboxToken, const QString& uhs)
{
    qDebug() << "Fetching XSTS token...";

    QJsonObject props;
    props["SandboxId"] = "RETAIL";
    props["UserTokens"] = QJsonArray{xboxToken};

    QJsonObject body;
    body["Properties"] = props;
    body["RelyingParty"] = "rp://api.minecraftservices.com/";
    body["TokenType"] = "JWT";

    QUrl xstsUrl(XBOX_XSTS_ENDPOINT);
    QNetworkRequest request(xstsUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply, xboxToken, uhs]() {
        if (reply->error() != QNetworkReply::NoError) {
            // Check if the error body contains XSTS-specific errors
            QByteArray data = reply->readAll();
            QJsonDocument errDoc = QJsonDocument::fromJson(data);
            QJsonObject errObj = errDoc.object();
            qint64 errCode = errObj.value("XErr").toVariant().toLongLong();

            QString errorMsg;
            if (errCode == 2148916235LL) {
                errorMsg = "Xbox account does not have an Xbox profile. Please create one at xbox.com first.";
            } else if (errCode == 2148916236LL) {
                errorMsg = "Xbox account is banned. Cannot authenticate.";
            } else if (errCode == 2148916237LL) {
                errorMsg = "Xbox account requires adult verification.";
            } else if (errCode == 2148916233LL) {
                errorMsg = "Xbox account is from a region where Minecraft is not available.";
            } else {
                errorMsg = "XSTS auth failed: " + reply->errorString();
            }

            qWarning() << errorMsg;
            emit microsoftAuthFailed(errorMsg);
            reply->deleteLater();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();

        QString xstsToken = obj.value("Token").toString();
        QJsonArray xui = obj.value("DisplayClaims").toObject().value("xui").toArray();

        if (xstsToken.isEmpty()) {
            qWarning() << "No XSTS token received";
            emit microsoftAuthFailed("XSTS authentication failed");
            reply->deleteLater();
            return;
        }

        // Extract UHS from XSTS response
        QString xstsUhs = uhs;
        if (!xui.isEmpty()) {
            xstsUhs = xui.first().toObject().value("uhs").toString();
        }

        qDebug() << "XSTS token obtained, logging in with Xbox to Minecraft services...";
        reply->deleteLater();

        getMinecraftToken(xstsToken, xstsUhs);
    });
}

void AuthManager::getMinecraftToken(const QString& xstsToken, const QString& uhs)
{
    qDebug() << "Logging in with Xbox to Minecraft services...";

    QJsonObject body;
    body["identityToken"] = "XBL3.0 x=" + uhs + ";" + xstsToken;

    QUrl mcLoginUrl(MC_LOGIN_WITH_XBOX);
    QNetworkRequest request(mcLoginUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(body).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            qWarning() << "Minecraft login failed (HTTP" << statusCode << "):" << reply->errorString();
            if (statusCode == 401) {
                emit microsoftAuthFailed("Xbox credentials were rejected by Minecraft services. Please try signing in again.");
            } else {
                emit microsoftAuthFailed("Minecraft services are temporarily unavailable. Please try again later.");
            }
            reply->deleteLater();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();

        QString mcAccessToken = obj.value("access_token").toString();

        if (mcAccessToken.isEmpty()) {
            qWarning() << "No Minecraft access token received";
            emit microsoftAuthFailed("Minecraft authentication failed — the service did not return a token.");
            reply->deleteLater();
            return;
        }

        qDebug() << "Minecraft access token obtained, fetching profile...";
        reply->deleteLater();

        // Validate and get profile
        validateMinecraftAccount(mcAccessToken);
    });
}

void AuthManager::validateMinecraftAccount(const QString& mcAccessToken)
{
    qDebug() << "Validating Minecraft account and fetching profile...";

    QUrl mcProfileUrl(MC_PROFILE_ENDPOINT);
    QNetworkRequest request(mcProfileUrl);
    request.setRawHeader("Authorization", ("Bearer " + mcAccessToken).toUtf8());

    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, mcAccessToken]() {
        if (reply->error() != QNetworkReply::NoError) {
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

            if (statusCode == 404) {
                qWarning() << "No Minecraft profile found — account may not own Minecraft";
                emit microsoftAuthFailed("This Microsoft account does not own Minecraft Java Edition. Please purchase the game first.");
            } else if (statusCode == 403) {
                qWarning() << "Minecraft profile access denied — no game entitlement";
                emit microsoftAuthFailed("This account does not have Minecraft Java Edition entitlement. Please check your purchases.");
            } else if (statusCode == 401) {
                qWarning() << "Minecraft token rejected (unauthorized)";
                emit microsoftAuthFailed("Authentication expired during profile fetch. Please try again.");
            } else {
                qWarning() << "Minecraft profile fetch failed (HTTP" << statusCode << "):" << reply->errorString();
                emit microsoftAuthFailed("Failed to fetch Minecraft profile. Please try again.");
            }
            reply->deleteLater();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();

        // Store the Minecraft access token as our primary token
        m_accessToken = mcAccessToken;
        m_isMicrosoftAuth = true;

        // Populate user profile from Minecraft profile
        m_userProfile.id = obj.value("id").toString();
        m_userProfile.username = obj.value("name").toString();
        m_userProfile.isPremium = true;
        m_userProfile.email = "Microsoft Account";

        qDebug() << "Minecraft authentication complete for user:" << m_userProfile.username;

        saveTokens();
        emit microsoftAuthComplete(mcAccessToken);
        emit loginSuccess(m_userProfile, m_accessToken);

        reply->deleteLater();
    });
}

void AuthManager::refreshMicrosoftToken()
{
    qDebug() << "Refreshing Microsoft token...";

    QUrl tokenUrl(MS_TOKEN_ENDPOINT);
    QUrlQuery params;
    params.addQueryItem("client_id", MS_CLIENT_ID);
    params.addQueryItem("grant_type", "refresh_token");
    params.addQueryItem("refresh_token", m_microsoftRefreshToken);
    params.addQueryItem("scope", MS_SCOPES.join(" "));

    QNetworkRequest request(tokenUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply* reply = m_networkManager->post(request, params.toString(QUrl::FullyEncoded).toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "Microsoft token refresh failed:" << reply->errorString();
            // Token refresh failed — need full re-auth
            clearTokens();
            emit loginFailed("Microsoft session expired. Please sign in again.");
            reply->deleteLater();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();

        QString msAccessToken = obj.value("access_token").toString();
        QString newRefreshToken = obj.value("refresh_token").toString();

        if (msAccessToken.isEmpty()) {
            QString error = obj.value("error").toString();
            QString errorDesc = obj.value("error_description").toString();
            qWarning() << "Microsoft token refresh error:" << error << errorDesc;
            clearTokens();
            emit loginFailed("Microsoft session expired: " + errorDesc);
            reply->deleteLater();
            return;
        }

        // Update tokens (Microsoft may or may not return a new refresh token)
        if (!newRefreshToken.isEmpty()) {
            m_microsoftRefreshToken = newRefreshToken;
        }

        qDebug() << "Microsoft token refreshed, re-authenticating with Xbox...";
        reply->deleteLater();

        // Re-do the Xbox → XSTS → MC chain with the new Microsoft token
        getXboxToken(msAccessToken);
    });
}

void AuthManager::cleanupOAuthServer()
{
    if (m_oauthServer) {
        if (m_oauthServer->isListening()) {
            m_oauthServer->close();
        }
        m_oauthServer->deleteLater();
        m_oauthServer = nullptr;
    }
}

// ─── PKCE / Crypto Helpers ─────────────────────────────────────────────────

QString AuthManager::generateCodeVerifier()
{
    // Generate 32 random bytes and base64url-encode them (43 chars)
    QByteArray randomBytes;
    randomBytes.resize(32);
    QRandomGenerator* rng = QRandomGenerator::global();
    for (int i = 0; i < 32; ++i) {
        randomBytes[i] = static_cast<char>(rng->bounded(256));
    }
    return randomBytes.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

QString AuthManager::generateCodeChallenge(const QString& verifier)
{
    // SHA256 hash of the verifier, then base64url-encode
    QByteArray hash = QCryptographicHash::hash(verifier.toUtf8(), QCryptographicHash::Sha256);
    return hash.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
}

QString AuthManager::generateState()
{
    // Generate 16 random bytes as a hex string (32 chars)
    QByteArray randomBytes;
    randomBytes.resize(16);
    QRandomGenerator* rng = QRandomGenerator::global();
    for (int i = 0; i < 16; ++i) {
        randomBytes[i] = static_cast<char>(rng->bounded(256));
    }
    return randomBytes.toHex();
}

// ─── Reply Handlers (existing email/password auth) ──────────────────────────

void AuthManager::onLoginReply(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit loginFailed(reply->errorString());
        reply->deleteLater();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    if (obj.value("success").toBool()) {
        QJsonObject data = obj.value("data").toObject();
        m_accessToken = data.value("accessToken").toString();
        m_refreshToken = data.value("refreshToken").toString();
        m_isMicrosoftAuth = false;

        QJsonObject userObj = data.value("user").toObject();
        m_userProfile.id = userObj.value("id").toString();
        m_userProfile.email = userObj.value("email").toString();
        m_userProfile.username = userObj.value("username").toString();
        m_userProfile.avatarUrl = userObj.value("avatarUrl").toString();
        m_userProfile.isVerified = userObj.value("isVerified").toBool();
        m_userProfile.isPremium = userObj.value("isPremium").toBool();
        m_userProfile.premiumUntil = QDateTime::fromString(userObj.value("premiumUntil").toString(), Qt::ISODate);
        m_userProfile.coinBalance = userObj.value("coinBalance").toInt();

        saveTokens();
        emit loginSuccess(m_userProfile, m_accessToken);
    } else {
        QJsonObject error = obj.value("error").toObject();
        emit loginFailed(error.value("message").toString());
    }

    reply->deleteLater();
}

void AuthManager::onRegisterReply(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit loginFailed(reply->errorString());
        reply->deleteLater();
        return;
    }

    // Same as login
    onLoginReply(reply);
}

void AuthManager::onRefreshReply(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        logout();
        reply->deleteLater();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    if (obj.value("success").toBool()) {
        QJsonObject data = obj.value("data").toObject();
        m_accessToken = data.value("accessToken").toString();
        saveTokens();
    } else {
        logout();
    }

    reply->deleteLater();
}

void AuthManager::onMeReply(QNetworkReply* reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();
        if (obj.value("success").toBool()) {
            QJsonObject data = obj.value("data").toObject();
            m_userProfile.id = data.value("id").toString();
            m_userProfile.email = data.value("email").toString();
            m_userProfile.username = data.value("username").toString();
            m_userProfile.avatarUrl = data.value("avatarUrl").toString();
            m_userProfile.isVerified = data.value("isVerified").toBool();
            m_userProfile.isPremium = data.value("isPremium").toBool();
            m_userProfile.premiumUntil = QDateTime::fromString(data.value("premiumUntil").toString(), Qt::ISODate);
            m_userProfile.coinBalance = data.value("coinBalance").toInt();

            qDebug() << "Loaded backend auth for user:" << m_userProfile.username;
            emit loginSuccess(m_userProfile, m_accessToken);
        }
    } else {
        // Backend call failed — token is likely invalid/expired
        qDebug() << "Backend /auth/me failed, clearing stale tokens";
        clearTokens();
        emit loginFailed("Session expired. Please sign in again.");
    }
    reply->deleteLater();
}

// ─── Token Persistence ─────────────────────────────────────────────────────

void AuthManager::saveTokens()
{
    QSettings settings(Settings::configDir() + "/auth.ini", QSettings::IniFormat);
    settings.setValue("accessToken", m_accessToken);
    settings.setValue("refreshToken", m_refreshToken);
    settings.setValue("isMicrosoftAuth", m_isMicrosoftAuth);

    // Persist profile for restart recovery (works for all auth types)
    settings.setValue("profileUsername", m_userProfile.username);
    settings.setValue("profileId", m_userProfile.id);
    settings.setValue("profileEmail", m_userProfile.email);
    settings.setValue("profileIsPremium", m_userProfile.isPremium);

    // Offline mode
    bool isOffline = m_accessToken.isEmpty() && m_refreshToken.isEmpty() &&
                     !m_userProfile.username.isEmpty();
    settings.setValue("isOffline", isOffline);

    // Save or clear Microsoft-specific tokens based on auth type
    if (m_isMicrosoftAuth && !m_microsoftRefreshToken.isEmpty()) {
        settings.setValue("microsoftRefreshToken", m_microsoftRefreshToken);
    } else {
        settings.remove("microsoftRefreshToken");
    }
}

void AuthManager::loadTokens()
{
    QSettings settings(Settings::configDir() + "/auth.ini", QSettings::IniFormat);
    m_accessToken = settings.value("accessToken").toString();
    m_refreshToken = settings.value("refreshToken").toString();
    m_isMicrosoftAuth = settings.value("isMicrosoftAuth", false).toBool();

    if (m_isMicrosoftAuth) {
        m_microsoftRefreshToken = settings.value("microsoftRefreshToken").toString();
    }

    // Load cached profile (used as fallback if network calls fail)
    m_userProfile.username = settings.value("profileUsername").toString();
    m_userProfile.id = settings.value("profileId").toString();
    m_userProfile.email = settings.value("profileEmail").toString();
    m_userProfile.isPremium = settings.value("profileIsPremium", false).toBool();

    // Check for offline mode
    bool isOffline = settings.value("isOffline", false).toBool();
    if (isOffline && m_accessToken.isEmpty()) {
        if (!m_userProfile.username.isEmpty()) {
            qDebug() << "Loaded offline profile:" << m_userProfile.username;
            emit loginSuccess(m_userProfile, m_accessToken);
        }
        return;
    }

    if (m_accessToken.isEmpty()) return;

    if (m_isMicrosoftAuth) {
        // For Microsoft auth, try to fetch Minecraft profile directly
        // If the token is expired, this will fail and we'll need to refresh
        QUrl mcProfileUrl(MC_PROFILE_ENDPOINT);
        QNetworkRequest request(mcProfileUrl);
        request.setRawHeader("Authorization", ("Bearer " + m_accessToken).toUtf8());

        QNetworkReply* reply = m_networkManager->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            if (reply->error() == QNetworkReply::NoError) {
                // Token is still valid — update profile from live data
                QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
                QJsonObject obj = doc.object();

                m_userProfile.id = obj.value("id").toString();
                m_userProfile.username = obj.value("name").toString();
                m_userProfile.isPremium = true;
                m_userProfile.email = "Microsoft Account";

                // Re-save with updated profile
                saveTokens();

                qDebug() << "Loaded Microsoft auth for user:" << m_userProfile.username;
                emit loginSuccess(m_userProfile, m_accessToken);
            } else {
                // Network failed or token expired — try cached profile first
                if (!m_userProfile.username.isEmpty()) {
                    qDebug() << "Microsoft profile fetch failed, using cached profile:" << m_userProfile.username;
                    emit loginSuccess(m_userProfile, m_accessToken);
                } else if (!m_microsoftRefreshToken.isEmpty()) {
                    qDebug() << "Microsoft token expired, refreshing...";
                    refreshMicrosoftToken();
                } else {
                    qDebug() << "Microsoft token expired and no refresh token or cached profile";
                    clearTokens();
                    emit loginFailed("Session expired. Please sign in again.");
                }
            }
            reply->deleteLater();
        });
    } else {
        // Backend auth — fetch profile from backend
        QNetworkRequest request(m_apiBaseUrl + "/auth/me");
        request.setRawHeader("Authorization", "Bearer " + m_accessToken.toUtf8());
        QNetworkReply* reply = m_networkManager->get(request);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            onMeReply(reply);
        });
    }
}

void AuthManager::clearTokens()
{
    m_accessToken.clear();
    m_refreshToken.clear();
    m_microsoftRefreshToken.clear();
    m_isMicrosoftAuth = false;
    m_userProfile = UserProfile();

    QSettings settings(Settings::configDir() + "/auth.ini", QSettings::IniFormat);
    settings.remove("accessToken");
    settings.remove("refreshToken");
    settings.remove("isMicrosoftAuth");
    settings.remove("microsoftRefreshToken");
    settings.remove("isOffline");
    settings.remove("profileUsername");
    settings.remove("profileId");
    settings.remove("profileEmail");
    settings.remove("profileIsPremium");
}
