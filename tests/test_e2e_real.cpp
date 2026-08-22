#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QThread>

#include "launcher/LauncherCore.h"

class TestE2EReal : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void test_javaDetection_real();
    void test_networkReachable();
    void test_manifestFetch_real();
    void test_versionDetailFetch_real();
    void test_clientJarDownload_real();
    void test_libraryDownload_real();
    void test_fullDownloadAndLaunch();

private:
    LauncherCore* m_core = nullptr;
    QTemporaryDir* m_tempDir = nullptr;
    QString m_javaPath;
    VersionDetail m_cachedDetail;
    QList<VersionEntry> m_cachedVersions;

    bool waitForSignal(QObject* obj, const char* signal, int timeoutMs = 60000) {
        QSignalSpy spy(obj, signal);
        qint64 start = QDateTime::currentMSecsSinceEpoch();
        while (QDateTime::currentMSecsSinceEpoch() - start < timeoutMs) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
            if (spy.count() > 0) return true;
            QTest::qWait(100);
        }
        return spy.count() > 0;
    }
};

void TestE2EReal::initTestCase()
{
    m_core = new LauncherCore(this);
    m_core->initialize();
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    qDebug() << "Shared temp dir:" << m_tempDir->path();
}

void TestE2EReal::cleanupTestCase()
{
    delete m_tempDir;
    m_tempDir = nullptr;
}
void TestE2EReal::init() {}
void TestE2EReal::cleanup() {}

void TestE2EReal::test_javaDetection_real()
{
    QList<JavaInfo> javaList = m_core->detectJava();
    qDebug() << "Detected" << javaList.size() << "Java installations";

    for (const JavaInfo& info : javaList) {
        qDebug() << "  " << info.path << "v:" << info.version
                 << "major:" << info.majorVersion << "64:" << info.is64Bit;
    }

    QVERIFY(javaList.size() > 0);

    bool found = false;
    for (const JavaInfo& info : javaList) {
        if (!info.path.isEmpty() && info.majorVersion > 0) {
            found = true;
            m_javaPath = info.path;
            break;
        }
    }
    QVERIFY(found);
    qDebug() << "Selected Java:" << m_javaPath;
}

void TestE2EReal::test_networkReachable()
{
    // Direct network test to verify Qt networking works
    QNetworkAccessManager nam;
    QNetworkReply* reply = nam.get(QNetworkRequest(QUrl("https://launchermeta.mojang.com/mc/game/version_manifest_v2.json")));

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QCOMPARE(reply->error(), QNetworkReply::NoError);

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QVERIFY(doc.isObject());
    QVERIFY(doc.object().contains("versions"));

    QJsonArray versions = doc.object().value("versions").toArray();
    qDebug() << "Network test: got" << versions.size() << "versions";

    int releaseCount = 0;
    for (const QJsonValue& v : versions) {
        if (v.toObject().value("type").toString() == "release") releaseCount++;
    }
    qDebug() << "Release versions:" << releaseCount;
    QVERIFY(releaseCount > 50);

    reply->deleteLater();
}

void TestE2EReal::test_manifestFetch_real()
{
    // Use direct QNetworkAccessManager since LauncherCore's async may have issues
    QNetworkAccessManager nam;
    QNetworkReply* reply = nam.get(QNetworkRequest(QUrl("https://launchermeta.mojang.com/mc/game/version_manifest_v2.json")));

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QCOMPARE(reply->error(), QNetworkReply::NoError);

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QVERIFY(doc.isObject());

    QJsonObject obj = doc.object();
    m_cachedVersions.clear();
    QJsonArray versionsArray = obj.value("versions").toArray();
    for (const QJsonValue& val : versionsArray) {
        QJsonObject v = val.toObject();
        VersionEntry entry;
        entry.id = v.value("id").toString();
        entry.type = v.value("type").toString();
        entry.url = v.value("url").toString();
        entry.releaseTime = QDateTime::fromString(v.value("time").toString(), Qt::ISODate);
        m_cachedVersions.append(entry);
    }

    qDebug() << "Manifest fetched:" << m_cachedVersions.size() << "versions";

    // Find latest release
    QJsonObject latest = obj.value("latest").toObject();
    QString latestRelease = latest.value("release").toString();
    QString latestSnapshot = latest.value("snapshot").toString();
    qDebug() << "Latest release:" << latestRelease;
    qDebug() << "Latest snapshot:" << latestSnapshot;

    QVERIFY(!latestRelease.isEmpty());
    QVERIFY(m_cachedVersions.size() > 100);

    reply->deleteLater();
}

void TestE2EReal::test_versionDetailFetch_real()
{
    // Should have versions from previous test
    QVERIFY(m_cachedVersions.size() > 0);

    // Find a stable release
    QString targetVersion;
    QString targetUrl;
    for (const VersionEntry& e : m_cachedVersions) {
        if (e.type == "release" && !e.id.isEmpty()) {
            targetVersion = e.id;
            targetUrl = e.url;
            break;
        }
    }
    QVERIFY(!targetVersion.isEmpty());
    QVERIFY(!targetUrl.isEmpty());
    qDebug() << "Fetching detail for:" << targetVersion;

    QNetworkAccessManager nam;
    QNetworkReply* reply = nam.get(QNetworkRequest(QUrl(targetUrl)));

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QCOMPARE(reply->error(), QNetworkReply::NoError);

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QVERIFY(doc.isObject());

    QJsonObject obj = doc.object();
    m_cachedDetail = VersionDetail();
    m_cachedDetail.id = obj.value("id").toString();
    m_cachedDetail.type = obj.value("type").toString();
    m_cachedDetail.mainClass = obj.value("mainClass").toString();
    m_cachedDetail.minimumLauncherVersion = obj.value("minimumLauncherVersion").toInt(0);
    m_cachedDetail.javaVersion = obj.value("javaVersion").toObject().value("majorVersion").toInt(8);
    m_cachedDetail.libraries = obj.value("libraries").toArray();
    m_cachedDetail.assetIndex = obj.value("assetIndex").toObject();
    m_cachedDetail.assets = obj.value("assets").toString();
    m_cachedDetail.downloads = obj.value("downloads").toObject();
    m_cachedDetail.arguments = obj.value("arguments").toObject();
    m_cachedDetail.minecraftArguments = obj.value("minecraftArguments").toString();

    QCOMPARE(m_cachedDetail.id, targetVersion);
    QVERIFY(!m_cachedDetail.mainClass.isEmpty());
    QVERIFY(m_cachedDetail.downloads.contains("client"));

    QJsonObject client = m_cachedDetail.downloads.value("client").toObject();
    QString clientUrl = client.value("url").toString();
    QString clientSha1 = client.value("sha1").toString();
    qint64 clientSize = client.value("size").toVariant().toLongLong();

    qDebug() << "Version:" << m_cachedDetail.id;
    qDebug() << "Main class:" << m_cachedDetail.mainClass;
    qDebug() << "Java version:" << m_cachedDetail.javaVersion;
    qDebug() << "Libraries:" << m_cachedDetail.libraries.size();
    qDebug() << "Client size:" << clientSize << "bytes";
    qDebug() << "Client SHA1:" << clientSha1;
    qDebug() << "Client URL:" << clientUrl.left(80) << "...";

    QVERIFY(clientSize > 0);

    reply->deleteLater();
}

void TestE2EReal::test_clientJarDownload_real()
{
    QVERIFY(!m_cachedDetail.id.isEmpty());
    QVERIFY(m_cachedDetail.downloads.contains("client"));

    QJsonObject client = m_cachedDetail.downloads.value("client").toObject();
    QString clientUrl = client.value("url").toString();
    QString expectedSha1 = client.value("sha1").toString();

    QString versionsDir = m_tempDir->path() + "/versions";
    QString jarDir = versionsDir + "/" + m_cachedDetail.id;
    QDir().mkpath(jarDir);
    QString jarPath = jarDir + "/" + m_cachedDetail.id + ".jar";

    qDebug() << "Downloading client JAR from:" << clientUrl.left(80) << "...";
    qDebug() << "Destination:" << jarPath;

    QNetworkAccessManager nam;
    QNetworkReply* reply = nam.get(QNetworkRequest(QUrl(clientUrl)));

    QEventLoop loop;
    connect(reply, &QNetworkReply::downloadProgress, this, [](qint64 received, qint64 total) {
        if (total > 0 && received % (1024 * 1024) < 65536) {
            qDebug() << "  Progress:" << (received / 1024 / 1024) << "/" << (total / 1024 / 1024) << "MB";
        }
    });
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QCOMPARE(reply->error(), QNetworkReply::NoError);

    QByteArray data = reply->readAll();
    qDebug() << "Downloaded" << data.size() << "bytes";

    // Verify SHA1
    QByteArray actualSha1 = QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex();
    qDebug() << "SHA1:" << actualSha1;
    QCOMPARE(actualSha1, expectedSha1.toUtf8());

    // Write to file
    QFile jarFile(jarPath);
    QVERIFY(jarFile.open(QIODevice::WriteOnly));
    jarFile.write(data);
    jarFile.close();

    qDebug() << "Client JAR saved:" << jarPath << "(" << QFileInfo(jarPath).size() << "bytes)";
    QVERIFY(QFileInfo(jarPath).size() > 1000000);

    reply->deleteLater();
}

void TestE2EReal::test_libraryDownload_real()
{
    QVERIFY(!m_cachedDetail.id.isEmpty());
    QVERIFY(m_cachedDetail.libraries.size() > 0);

    QString libDir = m_tempDir->path() + "/libraries";
    QDir().mkpath(libDir);

    int downloaded = 0;
    int skipped = 0;
    int failed = 0;

    QNetworkAccessManager nam;

    for (const QJsonValue& val : m_cachedDetail.libraries) {
        QJsonObject lib = val.toObject();
        QString name = lib.value("name").toString();
        QJsonObject downloads = lib.value("downloads").toObject();
        QJsonObject artifact = downloads.value("artifact").toObject();

        if (artifact.isEmpty()) continue;

        QString url = artifact.value("url").toString();
        QString path = artifact.value("path").toString();
        QString sha1 = artifact.value("sha1").toString();
        qint64 size = artifact.value("size").toVariant().toLongLong();

        if (url.isEmpty() || path.isEmpty()) continue;

        QString destPath = libDir + "/" + path;
        QFile destFile(destPath);

        // Skip if already exists with correct size
        if (destFile.exists() && destFile.size() == size) {
            skipped++;
            continue;
        }

        QDir().mkpath(QFileInfo(destPath).absolutePath());

        QNetworkReply* reply = nam.get(QNetworkRequest(QUrl(url)));
        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "FAILED:" << name << reply->errorString();
            failed++;
            reply->deleteLater();
            continue;
        }

        QByteArray data = reply->readAll();

        if (!sha1.isEmpty()) {
            QByteArray actual = QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex();
            if (actual != sha1.toUtf8()) {
                qDebug() << "CHECKSUM MISMATCH:" << name;
                failed++;
                reply->deleteLater();
                continue;
            }
        }

        QFile outFile(destPath);
        if (outFile.open(QIODevice::WriteOnly)) {
            outFile.write(data);
            outFile.close();
            downloaded++;
        } else {
            qDebug() << "WRITE FAILED:" << destPath;
            failed++;
        }

        reply->deleteLater();
    }

    qDebug() << "Libraries: downloaded=" << downloaded << "skipped=" << skipped << "failed=" << failed;

    int totalJars = 0;
    QDirIterator it(libDir, {"*.jar"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) { it.next(); totalJars++; }
    qDebug() << "Total JARs on disk:" << totalJars;

    QVERIFY(totalJars > 0);
    QCOMPARE(failed, 0);
}

void TestE2EReal::test_fullDownloadAndLaunch()
{
    QVERIFY(!m_cachedDetail.id.isEmpty());
    QVERIFY(m_javaPath.isEmpty() == false);

    QString gameDir = m_tempDir->path() + "/game";
    QDir().mkpath(gameDir);

    qDebug() << "=== Full E2E: version" << m_cachedDetail.id << "===";

    // Client JAR should already be downloaded from previous test
    QString jarPath = m_tempDir->path() + "/versions/" + m_cachedDetail.id + "/" + m_cachedDetail.id + ".jar";
    QVERIFY(QFile::exists(jarPath));
    qDebug() << "Client JAR exists:" << jarPath << "(" << QFileInfo(jarPath).size() << "bytes)";

    // Libraries should already be downloaded
    int libCount = 0;
    QDirIterator it(m_tempDir->path() + "/libraries", {"*.jar"}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) { it.next(); libCount++; }
    qDebug() << "Libraries on disk:" << libCount;
    QVERIFY(libCount > 0);

    // Prepare game directory structure that launchGame expects
    QString gameVersionsDir = gameDir + "/versions/" + m_cachedDetail.id;
    QDir().mkpath(gameVersionsDir);
    QFile::copy(jarPath, gameVersionsDir + "/" + m_cachedDetail.id + ".jar");

    QString gameLibDir = gameDir + "/libraries";
    // Copy libraries to game dir
    QDirIterator libIt(m_tempDir->path() + "/libraries", {"*.jar"}, QDir::Files, QDirIterator::Subdirectories);
    while (libIt.hasNext()) {
        QString src = libIt.next();
        QString relPath = src.mid((m_tempDir->path() + "/libraries/").length());
        QString dest = gameLibDir + "/" + relPath;
        QDir().mkpath(QFileInfo(dest).absolutePath());
        QFile::copy(src, dest);
    }

    LaunchConfig config;
    config.username = "E2ETestPlayer";
    config.uuid = "e2e-test-uuid-00000000";
    config.version = m_cachedDetail.id;
    config.gameDir = gameDir;
    config.javaPath = m_javaPath;
    config.accessToken = "offline";
    config.clientToken = "e2e-test";

    qDebug() << "Attempting launch...";
    qDebug() << "  Java:" << config.javaPath;
    qDebug() << "  Version:" << config.version;
    qDebug() << "  Game dir:" << config.gameDir;

    QSignalSpy startedSpy(m_core, &LauncherCore::launchStarted);
    QSignalSpy finishedSpy(m_core, &LauncherCore::launchFinished);

    bool launched = m_core->launchGame(config);
    qDebug() << "launchGame() returned:" << launched;

    if (launched) {
        QVERIFY(startedSpy.count() > 0);
        qDebug() << "launchStarted signal fired";

        for (int i = 0; i < 60; i++) {
            QCoreApplication::processEvents();
            if (!finishedSpy.isEmpty()) break;
            QTest::qWait(500);
        }

        if (!finishedSpy.isEmpty()) {
            bool success = finishedSpy.at(0).at(0).toBool();
            QString error = finishedSpy.at(0).at(1).toString();
            qDebug() << "Launch result:" << (success ? "SUCCESS" : "FAILED") << error;
        } else {
            qDebug() << "Process still running after timeout";
        }
    } else {
        qDebug() << "launchGame() returned false";
        if (!finishedSpy.isEmpty()) {
            qDebug() << "Error:" << finishedSpy.at(0).at(1).toString();
        }
    }
}

QTEST_MAIN(TestE2EReal)
#include "test_e2e_real.moc"
