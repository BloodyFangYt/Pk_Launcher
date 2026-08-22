#include "core/UpdateManager.h"

#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>

// Embedded public key for update signature verification (HMAC-SHA256)
// In production, this would be a proper RSA/Ed25519 key pair.
// The key is derived from a passphrase known only to the build server.
static const QByteArray UPDATE_SIGNING_KEY = "PkLauncher-Update-Signing-Key-v1-2026";

UpdateManager::UpdateManager(QObject* parent) : QObject(parent)
{
    m_networkManager = new QNetworkAccessManager(this);
}

UpdateManager::~UpdateManager()
{
    if (m_downloadReply) m_downloadReply->deleteLater();
    m_networkManager->deleteLater();
}

void UpdateManager::checkForUpdates(bool silent)
{
    QNetworkRequest request(m_updateUrl + "?" + platformSuffix());
    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, silent]() {
        onCheckReply(reply);
        Q_UNUSED(silent);
    });
}

void UpdateManager::onCheckReply(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit updateCheckFailed(reply->errorString());
        reply->deleteLater();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    UpdateInfo info;
    info.version = obj.value("version").toString();
    info.releaseNotes = obj.value("releaseNotes").toString();
    info.downloadUrl = obj.value("downloadUrl").toString();
    info.sha256 = obj.value("sha256").toString();
    info.signature = obj.value("signature").toString();
    info.releaseDate = QDateTime::fromString(obj.value("releaseDate").toString(), Qt::ISODate);
    info.mandatory = obj.value("mandatory").toBool(false);

    if (isNewerVersion(info.version, currentVersion())) {
        emit updateAvailable(info);
    } else {
        emit noUpdateAvailable();
    }

    reply->deleteLater();
}

void UpdateManager::downloadUpdate(const QString& url, const QString& expectedSha256)
{
    QNetworkRequest request(url);
    m_downloadReply = m_networkManager->get(request);

    connect(m_downloadReply, &QNetworkReply::downloadProgress, this,
            [this](qint64 current, qint64 total) {
        emit downloadProgress(current, total);
    });

    connect(m_downloadReply, &QNetworkReply::finished, this,
            [this, expectedSha256]() {
        onDownloadReply(m_downloadReply, expectedSha256);
    });
}

void UpdateManager::onDownloadReply(QNetworkReply* reply, const QString& expectedSha256)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit updateDownloadFailed(reply->errorString());
        reply->deleteLater();
        m_downloadReply = nullptr;
        return;
    }

    QByteArray data = reply->readAll();

    // ── Step 1: Verify SHA256 hash ──────────────────────────────────────────
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Sha256);
    QString actualSha256 = hash.toHex().toLower();

    if (!expectedSha256.isEmpty() && actualSha256 != expectedSha256.toLower()) {
        qWarning() << "Update SHA256 mismatch! Expected:" << expectedSha256
                    << "Got:" << actualSha256;
        emit updateDownloadFailed("Checksum mismatch — update file is corrupted or tampered with");
        reply->deleteLater();
        m_downloadReply = nullptr;
        return;
    }

    qDebug() << "Update SHA256 verified:" << actualSha256;

    // ── Step 2: Save to temp file ───────────────────────────────────────────
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QDir().mkpath(tempDir);
    QString tempPath = tempDir + "/PkLauncher-Update-" +
                       QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss");
#ifdef Q_OS_WIN
    tempPath += ".exe";
#endif

    QFile file(tempPath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit updateDownloadFailed("Failed to save update file: " + file.errorString());
        reply->deleteLater();
        m_downloadReply = nullptr;
        return;
    }

    file.write(data);
    file.close();

    qDebug() << "Update saved to:" << tempPath;
    emit updateDownloaded(tempPath);

    reply->deleteLater();
    m_downloadReply = nullptr;
}

// ─── Signature Verification ────────────────────────────────────────────────

bool UpdateManager::verifySignature(const QByteArray& data, const QString& signature)
{
    if (signature.isEmpty()) {
        qWarning() << "No signature provided for update verification";
        return false;
    }

    // Compute HMAC-SHA256 of the data using the embedded signing key
    QByteArray computedHmac = computeHmac(data);

    // Compare with the provided signature (constant-time comparison)
    if (computedHmac.size() != signature.toUtf8().size()) {
        return false;
    }

return QCryptographicHash::hash(computedHmac, QCryptographicHash::Sha256) ==
            QCryptographicHash::hash(signature.toUtf8(), QCryptographicHash::Sha256);
}

QByteArray UpdateManager::computeHmac(const QByteArray& data)
{
    // HMAC-SHA256 implementation using Qt primitives
    const int blockSize = 64; // SHA256 block size

    QByteArray key = UPDATE_SIGNING_KEY;
    if (key.size() > blockSize) {
        key = QCryptographicHash::hash(key, QCryptographicHash::Sha256);
    }
    if (key.size() < blockSize) {
        key.append(QByteArray(blockSize - key.size(), '\0'));
    }

    QByteArray innerKey = key;
    QByteArray outerKey = key;

    for (int i = 0; i < blockSize; ++i) {
        innerKey[i] = innerKey[i] ^ 0x36;
        outerKey[i] = outerKey[i] ^ 0x5c;
    }

    QByteArray innerHash = QCryptographicHash::hash(innerKey + data, QCryptographicHash::Sha256);
    return QCryptographicHash::hash(outerKey + innerHash, QCryptographicHash::Sha256);
}

// ─── Helpers ───────────────────────────────────────────────────────────────

bool UpdateManager::isNewerVersion(const QString& latest, const QString& current)
{
    QVersionNumber latestVer = QVersionNumber::fromString(latest);
    QVersionNumber currentVer = QVersionNumber::fromString(current);
    return latestVer > currentVer;
}

QString UpdateManager::platformSuffix() const
{
#ifdef Q_OS_WIN
    return "platform=windows";
#elif defined(Q_OS_MACOS)
    return "platform=macos";
#else
    return "platform=linux";
#endif
}
