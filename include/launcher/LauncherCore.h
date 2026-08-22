#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QThread>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QQueue>
#include <QUrl>
#include <QSize>
#include <QDateTime>
#include <QRegularExpression>
#include <QSysInfo>
#include <QFileInfo>
#include <QPair>
#include <QHash>
#include <QSet>
#include <QList>
#include <QMap>
#include <QVector>
#include <functional>

struct VersionEntry {
    QString id;
    QString type;
    QString url;
    QDateTime releaseTime;
};

struct VersionDetail {
    QString id;
    QString type;
    QString mainClass;
    int minimumLauncherVersion = 0;
    int javaVersion = 8;
    QJsonArray libraries;
    QJsonObject assetIndex;
    QString assets;
    QJsonObject downloads;
    QJsonObject arguments;
    QString minecraftArguments;
};

struct JavaInfo {
    QString path;
    QString version;
    int majorVersion = 0;
    bool is64Bit = false;
};

struct LaunchConfig {
    QString username;
    QString uuid;
    QString version;
    QString gameDir;
    QString javaPath;
    QStringList jvmArgs;
    QStringList gameArgs;
    QString accessToken;
    QString clientToken;
    QSize resolution;
};

struct DownloadProgress {
    QString currentFile;
    qint64 bytesDownloaded = 0;
    qint64 totalBytes = 0;
    int completed = 0;
    int total = 0;
};

class LauncherCore : public QObject
{
    Q_OBJECT

public:
    explicit LauncherCore(QObject* parent = nullptr);
    ~LauncherCore();

    bool initialize();

    // Version management
    QList<VersionEntry> fetchVersions(bool forceRefresh = false);
    VersionDetail fetchVersionDetail(const QString& versionId);
    QPair<QString, QString> getLatestVersions();

    // Java management
    QList<JavaInfo> detectJava();
    JavaInfo findBestJava(int requiredVersion, const QList<JavaInfo>& installed);
    bool validateJava(const QString& javaPath);
    QStringList getRecommendedJvmArgs(int javaVersion);

    // Download management
    void downloadVersion(const QString& versionId, const QString& gameDir);
    void downloadAssets(const VersionDetail& detail, const QString& assetsDir);
    void downloadLibraries(const VersionDetail& detail, const QString& librariesDir);
    void downloadClientJar(const VersionDetail& detail, const QString& versionsDir);

    // Launch
    bool launchGame(const LaunchConfig& config);

signals:
    void versionsFetched(const QList<VersionEntry>& versions);
    void versionDetailFetched(const VersionDetail& detail);
    void javaDetected(const QList<JavaInfo>& javaList);
    void downloadProgress(const DownloadProgress& progress);
    void downloadComplete(const QString& versionId);
    void downloadError(const QString& error);
    void launchStarted();
    void launchFinished(bool success, const QString& error = "");
    void logMessage(const QString& message);

private:
    void setupDirectories();
    QString getManifestUrl() const;
    VersionDetail parseVersionDetail(const QJsonObject& obj);
    QList<VersionEntry> parseManifest(const QJsonObject& obj);
    JavaInfo parseJavaInfo(const QString& javaPath);
    QString replacePlaceholders(const QString& arg, const LaunchConfig& config,
                                const VersionDetail& detail, const QString& clientJar,
                                const QString& assetsDir, const QString& nativesDir,
                                const QString& classpath);
    QStringList buildJvmArguments(const VersionDetail& detail, const LaunchConfig& config,
                                  const QString& clientJar, const QString& assetsDir,
                                  const QString& nativesDir, const QString& classpath);
    QStringList buildGameArguments(const VersionDetail& detail, const LaunchConfig& config,
                                   const QString& clientJar, const QString& assetsDir,
                                   const QString& nativesDir, const QString& classpath);
    bool checkRules(const QJsonArray& rules, const LaunchConfig& config);
    void extractNatives(const QString& librariesDir, const QString& nativesDir,
                        const std::function<void()>& onComplete = {});
    void downloadFile(const QUrl& url, const QString& destination,
                      const QString& expectedSha1, int completed, int total,
                      const std::function<void()>& onSuccess);
    void startDownloadPipeline(const QString& versionId, const QString& gameDir);

    QNetworkAccessManager* m_networkManager;
    QList<VersionEntry> m_cachedVersions;
    QHash<QString, VersionDetail> m_versionCache;
    QDateTime m_manifestCacheTime;
    QJsonObject m_manifestLatest;
    const int CACHE_TTL_MINUTES = 5;

    // Download manager internals
    struct DownloadItem {
        QUrl url;
        QString destPath;
        QString expectedSha1;
        QNetworkReply* reply = nullptr;
    };
    QList<DownloadItem> m_activeDownloads;
    int m_maxConcurrent = 8;
};
