#include "launcher/LauncherCore.h"
#include "core/Settings.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QThread>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QDebug>
#include <QDirIterator>
#include <memory>

LauncherCore::LauncherCore(QObject* parent) : QObject(parent)
{
    m_networkManager = new QNetworkAccessManager(this);
}

LauncherCore::~LauncherCore()
{
}

bool LauncherCore::initialize()
{
    setupDirectories();
    emit logMessage("Launcher core initialized");
    return true;
}

void LauncherCore::setupDirectories()
{
    QDir dir;
    dir.mkpath(Settings::dataDir());
    dir.mkpath(Settings::defaultInstancesDir());
    dir.mkpath(Settings::javaDir());
    dir.mkpath(Settings::assetsDir());
    dir.mkpath(Settings::librariesDir());
}

QString LauncherCore::getManifestUrl() const
{
    return "https://launchermeta.mojang.com/mc/game/version_manifest_v2.json";
}

QList<VersionEntry> LauncherCore::fetchVersions(bool forceRefresh)
{
    if (!forceRefresh && !m_cachedVersions.isEmpty() &&
        m_manifestCacheTime.secsTo(QDateTime::currentDateTime()) < CACHE_TTL_MINUTES * 60) {
        emit versionsFetched(m_cachedVersions);
        return m_cachedVersions;
    }

    QNetworkRequest request(getManifestUrl());
    QNetworkReply* reply = m_networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit logMessage("Failed to fetch versions: " + reply->errorString());
            reply->deleteLater();
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isObject()) {
            m_cachedVersions = parseManifest(doc.object());
            m_manifestLatest = doc.object().value("latest").toObject();
            m_manifestCacheTime = QDateTime::currentDateTime();
            emit versionsFetched(m_cachedVersions);
        }
        reply->deleteLater();
    });

    return m_cachedVersions;
}

QList<VersionEntry> LauncherCore::parseManifest(const QJsonObject& obj)
{
    QList<VersionEntry> versions;
    QJsonArray versionsArray = obj.value("versions").toArray();

    for (const QJsonValue& val : versionsArray) {
        QJsonObject v = val.toObject();
        VersionEntry entry;
        entry.id = v.value("id").toString();
        entry.type = v.value("type").toString();
        entry.url = v.value("url").toString();
        entry.releaseTime = QDateTime::fromString(v.value("time").toString(), Qt::ISODate);
        versions.append(entry);
    }

    return versions;
}

VersionDetail LauncherCore::fetchVersionDetail(const QString& versionId)
{
    if (m_versionCache.contains(versionId)) {
        emit versionDetailFetched(m_versionCache[versionId]);
        return m_versionCache[versionId];
    }

    auto requestDetail = [this, versionId]() {
        for (const VersionEntry& entry : m_cachedVersions) {
            if (entry.id != versionId) continue;
            QNetworkReply* reply = m_networkManager->get(QNetworkRequest(QUrl(entry.url)));
            connect(reply, &QNetworkReply::finished, this, [this, reply, versionId]() {
                const QByteArray data = reply->readAll();
                if (reply->error() != QNetworkReply::NoError) {
                    emit logMessage("Failed to fetch version detail: " + reply->errorString());
                } else {
                    const QJsonDocument doc = QJsonDocument::fromJson(data);
                    if (doc.isObject()) {
                        const VersionDetail detail = parseVersionDetail(doc.object());
                        m_versionCache.insert(versionId, detail);
                        emit versionDetailFetched(detail);
                    } else {
                        emit logMessage("Invalid version detail for " + versionId);
                    }
                }
                reply->deleteLater();
            });
            return;
        }
        VersionDetail empty;
        empty.id = versionId;
        emit versionDetailFetched(empty);
    };

    if (!m_cachedVersions.isEmpty()) {
        requestDetail();
        return VersionDetail();
    }

    QNetworkReply* manifestReply = m_networkManager->get(QNetworkRequest(getManifestUrl()));
    connect(manifestReply, &QNetworkReply::finished, this,
            [this, manifestReply, requestDetail]() {
        const QByteArray data = manifestReply->readAll();
        if (manifestReply->error() != QNetworkReply::NoError) {
            emit logMessage("Failed to fetch versions: " + manifestReply->errorString());
        } else {
            const QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isObject()) {
                m_cachedVersions = parseManifest(doc.object());
                m_manifestLatest = doc.object().value("latest").toObject();
                m_manifestCacheTime = QDateTime::currentDateTime();
                requestDetail();
            } else {
                emit logMessage("Invalid version manifest");
            }
        }
        manifestReply->deleteLater();
    });

    return VersionDetail();
}

VersionDetail LauncherCore::parseVersionDetail(const QJsonObject& obj)
{
    VersionDetail detail;
    detail.id = obj.value("id").toString();
    detail.type = obj.value("type").toString();
    detail.mainClass = obj.value("mainClass").toString();
    detail.minimumLauncherVersion = obj.value("minimumLauncherVersion").toInt(0);

    if (obj.contains("javaVersion")) {
        QJsonObject javaVer = obj.value("javaVersion").toObject();
        detail.javaVersion = javaVer.value("majorVersion").toInt(8);
    }

    detail.libraries = obj.value("libraries").toArray();
    detail.assetIndex = obj.value("assetIndex").toObject();
    detail.assets = obj.value("assets").toString();
    detail.downloads = obj.value("downloads").toObject();
    detail.arguments = obj.value("arguments").toObject();
    detail.minecraftArguments = obj.value("minecraftArguments").toString();

    return detail;
}

QPair<QString, QString> LauncherCore::getLatestVersions()
{
    fetchVersions(false);
    return {m_manifestLatest.value("release").toString(),
            m_manifestLatest.value("snapshot").toString()};
}

QList<JavaInfo> LauncherCore::detectJava()
{
    QList<JavaInfo> javaList;

    // Check JAVA_HOME
    QString javaHome = qEnvironmentVariable("JAVA_HOME");
    if (!javaHome.isEmpty()) {
        QString javaBin = javaHome + "/bin/java";
        if (QFile::exists(javaBin)) {
            const JavaInfo info = parseJavaInfo(javaBin);
            if (info.majorVersion > 0) javaList.append(info);
        }
    }

    // Common paths
    QStringList commonPaths = {
        "/usr/bin/java",
        "/usr/local/bin/java",
        "/usr/lib/jvm/*/bin/java",
        "/Library/Java/JavaVirtualMachines/*/Contents/Home/bin/java"
    };

    for (const QString& pattern : commonPaths) {
        if (pattern.contains("*")) {
            QDir dir(pattern.section("*", 0, 0));
            QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QString& entry : entries) {
                QString path = dir.filePath(entry) + pattern.section("*", 1);
                if (QFile::exists(path)) {
                    const JavaInfo info = parseJavaInfo(path);
                    if (info.majorVersion > 0) javaList.append(info);
                }
            }
        } else if (QFile::exists(pattern)) {
            const JavaInfo info = parseJavaInfo(pattern);
            if (info.majorVersion > 0) javaList.append(info);
        }
    }

    // Check PATH
    QProcess which;
    which.start("which", {"java"});
    which.waitForFinished();
    QString pathJava = which.readAllStandardOutput().trimmed();
    if (!pathJava.isEmpty()) {
        const JavaInfo info = parseJavaInfo(pathJava);
        if (info.majorVersion > 0) javaList.append(info);
    }

    // Deduplicate
    QSet<QString> seen;
    QList<JavaInfo> unique;
    for (const JavaInfo& info : javaList) {
        if (!seen.contains(info.path)) {
            seen.insert(info.path);
            unique.append(info);
        }
    }

    // Sort by version descending
    std::sort(unique.begin(), unique.end(),
              [](const JavaInfo& a, const JavaInfo& b) {
                  return a.majorVersion > b.majorVersion;
              });

    emit javaDetected(unique);
    return unique;
}

JavaInfo LauncherCore::parseJavaInfo(const QString& javaPath)
{
    JavaInfo info;
    info.path = javaPath;

    QProcess proc;
    proc.start(javaPath, {"-version"});
    proc.waitForFinished();
    QString output = proc.readAllStandardError() + proc.readAllStandardOutput();

    QRegularExpression re(R"REGEX(version\s+"(\d+(?:\.\d+)*(?:\+\d+(?:-\w+)?)?)")REGEX");
    QRegularExpressionMatch match = re.match(output);
    if (match.hasMatch()) {
        info.version = match.captured(1);
        QString majorStr = info.version.split('.').first();
        bool ok;
        int major = majorStr.toInt(&ok);
        if (ok) {
            info.majorVersion = (major == 1) ? info.version.split('.')[1].toInt() : major;
        }
    }

    info.is64Bit = output.contains("64-Bit", Qt::CaseInsensitive) ||
                   output.contains("64 Bit", Qt::CaseInsensitive);

    return info;
}

JavaInfo LauncherCore::findBestJava(int requiredVersion, const QList<JavaInfo>& installed)
{
    // Exact match
    for (const JavaInfo& info : installed) {
        if (info.majorVersion == requiredVersion) return info;
    }

    // Higher version (forward compatible)
    for (const JavaInfo& info : installed) {
        if (info.majorVersion > requiredVersion) return info;
    }

    // Lower version (fallback)
    for (int i = installed.size() - 1; i >= 0; --i) {
        if (installed[i].majorVersion < requiredVersion) return installed[i];
    }

    return JavaInfo();
}

bool LauncherCore::validateJava(const QString& javaPath)
{
    QProcess proc;
    proc.start(javaPath, {"-version"});
    return proc.waitForFinished() && proc.exitCode() == 0;
}

QStringList LauncherCore::getRecommendedJvmArgs(int javaVersion)
{
    QStringList args = {
        "-Xmx4G", "-Xms1G"
    };

    if (javaVersion >= 17) {
        args << "-XX:+UseG1GC" << "-XX:+ParallelRefProcEnabled"
             << "-XX:MaxGCPauseMillis=200" << "-XX:+UnlockExperimentalVMOptions"
             << "-XX:+DisableExplicitGC" << "-XX:+AlwaysPreTouch"
             << "-XX:G1NewSizePercent=30" << "-XX:G1MaxNewSizePercent=40"
             << "-XX:G1HeapRegionSize=8M" << "-XX:G1ReservePercent=20"
             << "-XX:G1HeapWastePercent=5" << "-XX:G1MixedGCCountTarget=4"
             << "-XX:InitiatingHeapOccupancyPercent=15"
             << "-XX:G1MixedGCLiveThresholdPercent=90"
             << "-XX:G1RSetUpdatingPauseTimePercent=5"
             << "-XX:SurvivorRatio=32" << "-XX:+PerfDisableSharedMem"
             << "-XX:MaxTenuringThreshold=1";
    } else if (javaVersion >= 11) {
        args << "-XX:+UseG1GC" << "-XX:+ParallelRefProcEnabled"
             << "-XX:MaxGCPauseMillis=200" << "-XX:+UnlockExperimentalVMOptions"
             << "-XX:+DisableExplicitGC" << "-XX:+AlwaysPreTouch";
    } else {
        args << "-XX:+UseConcMarkSweepGC" << "-XX:+UnlockExperimentalVMOptions"
             << "-XX:MaxGCPauseMillis=100" << "-XX:+DisableExplicitGC"
             << "-XX:+AlwaysPreTouch";
    }

    return args;
}

void LauncherCore::downloadVersion(const QString& versionId, const QString& gameDir)
{
    if (versionId.isEmpty() || gameDir.isEmpty()) {
        emit downloadError("Version and game directory are required");
        return;
    }
    startDownloadPipeline(versionId, gameDir);
}

void LauncherCore::startDownloadPipeline(const QString& versionId, const QString& gameDir)
{
    auto run = [this, versionId, gameDir](const VersionDetail& detail) {
        QList<QJsonObject> files;
        const QJsonObject client = detail.downloads.value("client").toObject();
        if (client.isEmpty()) {
            emit downloadError("No client.jar for " + detail.id);
            return;
        }
        files.append({{"url", client.value("url")}, {"path", gameDir + "/versions/" + detail.id + "/" + detail.id + ".jar"},
                      {"sha1", client.value("sha1")}});
        const QString platform = QSysInfo::kernelType().toLower();
        const QString nativeKey = platform == "winnt" ? "natives-windows" :
                                  platform == "darwin" ? "natives-osx" : "natives-linux";
        for (const QJsonValue& value : detail.libraries) {
            const QJsonObject library = value.toObject();
            if (!checkRules(library.value("rules").toArray(), {})) continue;
            const QJsonObject downloads = library.value("downloads").toObject();
            if (!downloads.value("artifact").toObject().isEmpty()) files.append(downloads.value("artifact").toObject());
            if (!downloads.value("classifiers").toObject().value(nativeKey).toObject().isEmpty())
                files.append(downloads.value("classifiers").toObject().value(nativeKey).toObject());
        }
        const QString librariesDir = gameDir + "/libraries";
        const QString assetsDir = gameDir + "/assets";
        for (QJsonObject& file : files) file["path"] = librariesDir + "/" + file.value("path").toString();
        files[0]["path"] = gameDir + "/versions/" + detail.id + "/" + detail.id + ".jar";
        auto next = std::make_shared<std::function<void(int)>>();
        *next = [this, next, files, detail, gameDir, assetsDir, versionId](int index) mutable {
            if (index < files.size()) {
                const QJsonObject file = files.at(index);
                downloadFile(QUrl(file.value("url").toString()), file.value("path").toString(),
                             file.value("sha1").toString(), index, files.size() + 1,
                             [next, index]() { (*next)(index + 1); });
                return;
            }
            if (detail.assetIndex.isEmpty()) {
                extractNatives(gameDir + "/libraries", gameDir + "/libraries/natives",
                               [this, versionId]() { emit downloadComplete(versionId); });
                return;
            }
            const QString indexPath = assetsDir + "/indexes/" + detail.assets + ".json";
            downloadFile(QUrl(detail.assetIndex.value("url").toString()), indexPath,
                         detail.assetIndex.value("sha1").toString(), index, files.size() + 1,
                         [this, indexPath, assetsDir, detail, gameDir, versionId, files]() {
                QFile file(indexPath);
                if (!file.open(QIODevice::ReadOnly)) { emit downloadError("Cannot read asset index"); return; }
                const QJsonObject objects = QJsonDocument::fromJson(file.readAll()).object().value("objects").toObject();
                auto assets = std::make_shared<QList<QJsonObject>>();
                for (auto it = objects.constBegin(); it != objects.constEnd(); ++it) assets->append(it.value().toObject());
                auto assetNext = std::make_shared<std::function<void(int)>>();
                *assetNext = [this, assetNext, assets, assetsDir, files, detail, gameDir, versionId](int i) {
                    if (i >= assets->size()) {
                        extractNatives(gameDir + "/libraries", gameDir + "/libraries/natives",
                                       [this, versionId]() { emit downloadComplete(versionId); });
                        return;
                    }
                    const QString hash = assets->at(i).value("hash").toString();
                    downloadFile(QUrl("https://resources.download.minecraft.net/" + hash.left(2) + "/" + hash),
                                 assetsDir + "/objects/" + hash.left(2) + "/" + hash, hash,
                                 files.size() + i, files.size() + assets->size() + 1,
                                 [assetNext, i]() { (*assetNext)(i + 1); });
                };
                (*assetNext)(0);
            });
        };
        (*next)(0);
    };

    auto fetchDetail = [this, versionId, run]() {
        if (m_versionCache.contains(versionId)) { run(m_versionCache.value(versionId)); return; }
        for (const VersionEntry& entry : m_cachedVersions) if (entry.id == versionId) {
            QNetworkReply* reply = m_networkManager->get(QNetworkRequest(QUrl(entry.url)));
            connect(reply, &QNetworkReply::finished, this, [this, reply, versionId, run]() {
                const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
                if (reply->error() != QNetworkReply::NoError || !doc.isObject()) {
                    emit downloadError("Failed to fetch version detail: " + reply->errorString());
                } else { const VersionDetail detail = parseVersionDetail(doc.object()); m_versionCache.insert(versionId, detail); run(detail); }
                reply->deleteLater();
            });
            return;
        }
        emit downloadError("Version not found: " + versionId);
    };

    if (m_cachedVersions.isEmpty()) {
        QNetworkReply* reply = m_networkManager->get(QNetworkRequest(getManifestUrl()));
        connect(reply, &QNetworkReply::finished, this, [this, reply, fetchDetail]() {
            const QByteArray data = reply->readAll();
            if (reply->error() != QNetworkReply::NoError) {
                emit downloadError("Failed to fetch versions: " + reply->errorString());
                reply->deleteLater();
                return;
            }
            const QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isObject()) {
                emit downloadError("Invalid version manifest");
                reply->deleteLater();
                return;
            }
            m_cachedVersions = parseManifest(doc.object());
            m_manifestLatest = doc.object().value("latest").toObject();
            m_manifestCacheTime = QDateTime::currentDateTime();
            fetchDetail();
            reply->deleteLater();
        });
    } else {
        fetchDetail();
    }
}

void LauncherCore::downloadFile(const QUrl& url, const QString& destination,
                                const QString& expectedSha1, int completed, int total,
                                const std::function<void()>& onSuccess)
{
    if (!url.isValid() || url.scheme().isEmpty()) {
        emit downloadError("Invalid download URL: " + url.toString());
        return;
    }
    QDir().mkpath(QFileInfo(destination).absolutePath());
    QFileInfo existing(destination);
    if (existing.exists() && (expectedSha1.isEmpty() ||
        QCryptographicHash::hash([&]() {
            QFile file(destination);
            file.open(QIODevice::ReadOnly);
            return file.readAll();
        }(), QCryptographicHash::Sha1).toHex() == expectedSha1.toUtf8().toLower())) {
        emit downloadProgress({destination, existing.size(), existing.size(), completed + 1, total});
        onSuccess();
        return;
    }

    QNetworkReply* reply = m_networkManager->get(QNetworkRequest(url));
    auto* file = new QFile(destination + ".part", reply);
    if (!file->open(QIODevice::WriteOnly)) {
        emit downloadError("Cannot write download: " + destination);
        reply->abort();
        return;
    }
    connect(reply, &QNetworkReply::downloadProgress, this,
            [this, destination, completed, total](qint64 received, qint64 size) {
        emit downloadProgress({destination, received, size, completed, total});
    });
    connect(reply, &QNetworkReply::readyRead, file, [reply, file]() {
        file->write(reply->readAll());
    });
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, file, destination, expectedSha1, completed, total, onSuccess]() {
        file->write(reply->readAll());
        file->close();
        if (reply->error() != QNetworkReply::NoError) {
            emit downloadError("Download failed: " + reply->errorString());
            file->deleteLater();
            reply->deleteLater();
            return;
        }
        QFile verify(file->fileName());
        if (!verify.open(QIODevice::ReadOnly)) {
            emit downloadError("Cannot verify download: " + destination);
        } else {
            const QByteArray hash = QCryptographicHash::hash(verify.readAll(), QCryptographicHash::Sha1).toHex();
            if (!expectedSha1.isEmpty() && hash != expectedSha1.toUtf8().toLower()) {
                emit downloadError("Checksum mismatch: " + destination);
            } else if (QFile::rename(file->fileName(), destination)) {
                emit downloadProgress({destination, QFileInfo(destination).size(),
                                        QFileInfo(destination).size(), completed + 1, total});
                onSuccess();
            } else {
                emit downloadError("Cannot finalize download: " + destination);
            }
            verify.close();
        }
        file->deleteLater();
        reply->deleteLater();
    });
}

void LauncherCore::downloadClientJar(const VersionDetail& detail, const QString& versionsDir)
{
    const QJsonObject client = detail.downloads.value("client").toObject();
    if (client.isEmpty()) {
        emit downloadError("No client.jar for " + detail.id);
        return;
    }
    const QString destination = versionsDir + "/" + detail.id + "/" + detail.id + ".jar";
    downloadFile(QUrl(client.value("url").toString()), destination,
                 client.value("sha1").toString(), 0, 1,
                 [this, versionId = detail.id]() { emit downloadComplete(versionId); });
}

void LauncherCore::downloadLibraries(const VersionDetail& detail, const QString& librariesDir)
{
    QDir().mkpath(librariesDir + "/natives");
    // Map QSysInfo::kernelType() to Minecraft platform names
    QString kernelType = QSysInfo::kernelType().toLower();
    QString platform;
    if (kernelType == "darwin") platform = "osx";
    else if (kernelType == "winnt") platform = "windows";
    else platform = kernelType;

    QList<QJsonObject> items;
    for (const QJsonValue& value : detail.libraries) {
        const QJsonObject library = value.toObject();
        if (!checkRules(library.value("rules").toArray(), {})) continue;
        const QJsonObject downloads = library.value("downloads").toObject();
        const QJsonObject artifact = downloads.value("artifact").toObject();
        if (!artifact.isEmpty()) items.append(artifact);
        const QJsonObject classifiers = downloads.value("classifiers").toObject();
        QString nativeKey = platform == "windows" ? "natives-windows" :
                            platform == "osx" ? "natives-osx" : "natives-linux";
        const QJsonObject native = classifiers.value(nativeKey).toObject();
        if (!native.isEmpty()) items.append(native);
    }
    auto downloadNext = std::make_shared<std::function<void(int)>>();
    *downloadNext = [this, items, librariesDir, downloadNext](int index) {
        if (index >= items.size()) return;
        const QJsonObject item = items.at(index);
        downloadFile(QUrl(item.value("url").toString()), librariesDir + "/" + item.value("path").toString(),
                     item.value("sha1").toString(), index, items.size(),
                     [downloadNext, index]() { (*downloadNext)(index + 1); });
    };
    (*downloadNext)(0);
}

void LauncherCore::downloadAssets(const VersionDetail& detail, const QString& assetsDir)
{
    if (detail.assetIndex.isEmpty()) {
        emit logMessage("No asset index for " + detail.id);
        return;
    }

    QDir().mkpath(assetsDir + "/objects");
    const QString indexPath = assetsDir + "/indexes/" + detail.assets + ".json";
    downloadFile(QUrl(detail.assetIndex.value("url").toString()), indexPath,
                 detail.assetIndex.value("sha1").toString(), 0, 1,
                 [this, indexPath, assetsDir]() {
        QFile file(indexPath);
        if (!file.open(QIODevice::ReadOnly)) {
            emit downloadError("Cannot read asset index: " + indexPath);
            return;
        }
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        const QJsonObject objects = doc.object().value("objects").toObject();
        auto entries = std::make_shared<QList<QPair<QString, QJsonObject>>>();
        for (auto it = objects.constBegin(); it != objects.constEnd(); ++it)
            entries->append({it.key(), it.value().toObject()});
        auto next = std::make_shared<std::function<void(int)>>();
        *next = [this, entries, assetsDir, next](int index) {
            if (index >= entries->size()) return;
            const auto entry = entries->at(index);
            const QString hash = entry.second.value("hash").toString();
            downloadFile(QUrl("https://resources.download.minecraft.net/" + hash.left(2) + "/" + hash),
                         assetsDir + "/objects/" + hash.left(2) + "/" + hash, hash,
                         index + 1, entries->size() + 1,
                         [next, index]() { (*next)(index + 1); });
        };
        (*next)(0);
    });
}

bool LauncherCore::launchGame(const LaunchConfig& config)
{
    VersionDetail detail = fetchVersionDetail(config.version);
    if (detail.id.isEmpty()) {
        emit launchFinished(false, "Version not found: " + config.version);
        return false;
    }

    // Build paths
    QString versionsDir = config.gameDir + "/versions/" + config.version;
    QString clientJar = versionsDir + "/" + config.version + ".jar";
    QString librariesDir = config.gameDir + "/libraries";
    QString nativesDir = librariesDir + "/natives";
    QString assetsDir = config.gameDir + "/assets";

    if (!QFileInfo::exists(clientJar)) {
        emit launchFinished(false, "Client JAR is missing: " + clientJar);
        return false;
    }

    // Build classpath
    QStringList classpath;
    classpath << clientJar;

    // Libraries are stored in Maven-style nested directories.
    QDirIterator libraryIterator(librariesDir, {"*.jar"}, QDir::Files,
                                 QDirIterator::Subdirectories);
    while (libraryIterator.hasNext()) {
        const QString libraryPath = libraryIterator.next();
        if (QFileInfo(libraryPath).absoluteFilePath() !=
            QFileInfo(clientJar).absoluteFilePath()) {
            classpath << libraryPath;
        }
    }

    QString classpathStr = classpath.join(QDir::listSeparator());

    // Build arguments
    QStringList jvmArgs = buildJvmArguments(detail, config, clientJar, assetsDir, nativesDir, classpathStr);
    QStringList gameArgs = buildGameArguments(detail, config, clientJar, assetsDir, nativesDir, classpathStr);

    QStringList allArgs = jvmArgs + gameArgs;

    // Launch
    QProcess* process = new QProcess(this);
    process->setWorkingDirectory(config.gameDir);
    process->setProgram(config.javaPath);
    process->setArguments(allArgs);

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, process](int exitCode, QProcess::ExitStatus) {
                emit launchFinished(exitCode == 0, exitCode == 0 ? "" : "Game exited with code " + QString::number(exitCode));
                process->deleteLater();
            });

    connect(process, &QProcess::errorOccurred, this, [this, process](QProcess::ProcessError) {
        emit launchFinished(false, "Launch failed: " + process->errorString());
        process->deleteLater();
    });

    emit launchStarted();
    process->start();
    return true;
}

QString LauncherCore::replacePlaceholders(const QString& arg, const LaunchConfig& config,
                                          const VersionDetail& detail, const QString& clientJar,
                                          const QString& assetsDir, const QString& nativesDir,
                                          const QString& classpath)
{
    Q_UNUSED(clientJar);
    QString result = arg;
    QString classpathSep = QDir::listSeparator();

    QHash<QString, QString> replacements = {
        {"${auth_player_name}", config.username},
        {"${auth_session}", config.accessToken},
        {"${auth_player_id}", config.uuid},
        {"${auth_uuid}", config.uuid},
        {"${auth_access_token}", config.accessToken},
        {"${auth_xuid}", ""},
        {"${auth_client_id}", ""},
        {"${user_type}", config.accessToken.isEmpty() ? "offline" : "msa"},
        {"${user_properties}", "{}"},
        {"${version_name}", detail.id},
        {"${game_directory}", config.gameDir},
        {"${assets_root}", assetsDir},
        {"${assets_index_name}", detail.assets},
        {"${game_assets}", assetsDir},
        {"${version_type}", detail.type},
        {"${classpath}", classpath},
        {"${natives_directory}", nativesDir},
        {"${launcher_name}", "PkLauncher"},
        {"${launcher_version}", "0.1.0"},
        {"${classpath_separator}", classpathSep}
    };

    for (auto it = replacements.constBegin(); it != replacements.constEnd(); ++it) {
        result.replace(it.key(), it.value());
    }

    return result;
}

QStringList LauncherCore::buildJvmArguments(const VersionDetail& detail, const LaunchConfig& config,
                                            const QString& clientJar, const QString& assetsDir,
                                            const QString& nativesDir, const QString& classpath)
{
    QStringList args;
    QJsonObject jvmObj = detail.arguments.value("jvm").toObject();
    QJsonArray jvmArray = jvmObj.isEmpty() ? detail.arguments.value("jvm").toArray() : QJsonArray();

    if (jvmArray.isEmpty()) {
        // Legacy
        args << "-Djava.library.path=" + nativesDir;
        args << "-cp" << classpath;
    } else {
        for (const QJsonValue& val : jvmArray) {
            if (val.isString()) {
                args << replacePlaceholders(val.toString(), config, detail, clientJar, assetsDir, nativesDir, classpath);
            } else if (val.isObject()) {
                QJsonObject obj = val.toObject();
                if (checkRules(obj.value("rules").toArray(), config)) {
                    QJsonValue value = obj.value("value");
                    if (value.isString()) {
                        args << replacePlaceholders(value.toString(), config, detail, clientJar, assetsDir, nativesDir, classpath);
                    } else if (value.isArray()) {
                        for (const QJsonValue& v : value.toArray()) {
                            args << replacePlaceholders(v.toString(), config, detail, clientJar, assetsDir, nativesDir, classpath);
                        }
                    }
                }
            }
        }
    }

    // Add custom JVM args
    args << config.jvmArgs;

    return args;
}

QStringList LauncherCore::buildGameArguments(const VersionDetail& detail, const LaunchConfig& config,
                                             const QString& clientJar, const QString& assetsDir,
                                             const QString& nativesDir, const QString& classpath)
{
    QStringList args;
    args << detail.mainClass;

    QJsonObject gameObj = detail.arguments.value("game").toObject();
    QJsonArray gameArray = gameObj.isEmpty() ? detail.arguments.value("game").toArray() : QJsonArray();

    if (gameArray.isEmpty()) {
        // Legacy
        args << "--username" << config.username
             << "--version" << detail.id
             << "--gameDir" << config.gameDir
             << "--assetsDir" << assetsDir
             << "--assetIndex" << detail.assets
             << "--accessToken" << config.accessToken
             << "--uuid" << config.uuid
             << "--userType" << (config.accessToken.isEmpty() ? "offline" : "msa");

        if (config.resolution.isValid()) {
            args << "--width" << QString::number(config.resolution.width())
                 << "--height" << QString::number(config.resolution.height());
        }
    } else {
        for (const QJsonValue& val : gameArray) {
            if (val.isString()) {
                args << replacePlaceholders(val.toString(), config, detail, clientJar, assetsDir, nativesDir, classpath);
            } else if (val.isObject()) {
                QJsonObject obj = val.toObject();
                if (checkRules(obj.value("rules").toArray(), config)) {
                    QJsonValue value = obj.value("value");
                    if (value.isString()) {
                        args << replacePlaceholders(value.toString(), config, detail, clientJar, assetsDir, nativesDir, classpath);
                    } else if (value.isArray()) {
                        for (const QJsonValue& v : value.toArray()) {
                            args << replacePlaceholders(v.toString(), config, detail, clientJar, assetsDir, nativesDir, classpath);
                        }
                    }
                }
            }
        }
    }

    args << config.gameArgs;
    return args;
}

bool LauncherCore::checkRules(const QJsonArray& rules, const LaunchConfig& config)
{
    Q_UNUSED(config);
    if (rules.isEmpty()) return true;
    bool allowed = false;

    // Map QSysInfo::kernelType() to Minecraft OS names
    QString kernelType = QSysInfo::kernelType().toLower();
    QString platform;
    if (kernelType == "darwin") platform = "osx";
    else if (kernelType == "winnt") platform = "windows";
    else platform = kernelType;

    QString arch = (QSysInfo::currentCpuArchitecture() == "x86_64") ? "x86" : QSysInfo::currentCpuArchitecture();

    for (const QJsonValue& val : rules) {
        QJsonObject rule = val.toObject();
        bool ruleMatch = true;

        // Check OS constraints
        if (rule.contains("os")) {
            QJsonObject os = rule.value("os").toObject();
            QString osName = os.value("name").toString().toLower();
            if (!osName.isEmpty() && osName != platform) ruleMatch = false;

            QString osArch = os.value("arch").toString().toLower();
            if (ruleMatch && !osArch.isEmpty() && osArch != arch) ruleMatch = false;

            // We don't enforce os.version — too platform-specific
        }

        // Check features constraints (is_demo_user, has_custom_resolution, etc.)
        // Features are opt-in: if a rule requires a feature we don't support, skip it
        if (ruleMatch && rule.contains("features")) {
            QJsonObject features = rule.value("features").toObject();
            for (auto it = features.constBegin(); it != features.constEnd(); ++it) {
                if (it.value().toBool()) {
                    ruleMatch = false;
                    break;
                }
            }
        }

        if (ruleMatch) {
            if (rule.value("action").toString() == "allow") allowed = true;
            else if (rule.value("action").toString() == "deny") allowed = false;
        }
    }

    return allowed;
}

void LauncherCore::extractNatives(const QString& librariesDir, const QString& nativesDir,
                                  const std::function<void()>& onComplete)
{
    QDir().mkpath(nativesDir);
    auto jars = std::make_shared<QStringList>();
    QDirIterator iterator(librariesDir, {"*-natives-*.jar"}, QDir::Files,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext()) jars->append(iterator.next());

    auto next = std::make_shared<std::function<void(int)>>();
    *next = [this, jars, nativesDir, onComplete, next](int index) {
        if (index >= jars->size()) {
            if (onComplete) onComplete();
            return;
        }
        auto* process = new QProcess(this);
        process->setWorkingDirectory(nativesDir);
        const QString jarPath = jars->at(index);
        process->start("unzip", {"-o", "-q", "-j", jarPath, "*.dll", "*.so", "*.dylib"});
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, process, jarPath, index, next](int exitCode, QProcess::ExitStatus) {
            if (exitCode != 0) {
                emit downloadError("Failed to extract native library: " + QFileInfo(jarPath).fileName());
                process->deleteLater();
                return;
            }
            process->deleteLater();
            (*next)(index + 1);
        });
        connect(process, &QProcess::errorOccurred, this, [this, process, jarPath](QProcess::ProcessError) {
            emit downloadError("Failed to start native extraction: " + QFileInfo(jarPath).fileName());
            process->deleteLater();
        });
    };
    (*next)(0);
}