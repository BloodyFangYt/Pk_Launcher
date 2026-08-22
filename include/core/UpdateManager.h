#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QVersionNumber>

struct UpdateInfo {
    QString version;
    QString releaseNotes;
    QString downloadUrl;
    QString sha256;
    QString signature;
    QDateTime releaseDate;
    bool mandatory = false;
};

class UpdateManager : public QObject
{
    Q_OBJECT

public:
    explicit UpdateManager(QObject* parent = nullptr);
    ~UpdateManager();

    void checkForUpdates(bool silent = false);
    void downloadUpdate(const QString& url, const QString& expectedSha256);
    bool verifySignature(const QByteArray& data, const QString& signature);
    QString currentVersion() const { return "0.1.0"; }

signals:
    void updateAvailable(const UpdateInfo& info);
    void noUpdateAvailable();
    void updateCheckFailed(const QString& error);
    void downloadProgress(qint64 current, qint64 total);
    void updateDownloaded(const QString& filePath);
    void updateDownloadFailed(const QString& error);

private slots:
    void onCheckReply(QNetworkReply* reply);
    void onDownloadReply(QNetworkReply* reply, const QString& expectedSha256);

private:
    QNetworkAccessManager* m_networkManager;
    QString m_updateUrl = "https://releases.pklauncher.dev/update.json";
    QNetworkReply* m_downloadReply = nullptr;

    bool isNewerVersion(const QString& latest, const QString& current);
    QString platformSuffix() const;
    QByteArray computeHmac(const QByteArray& data);
};