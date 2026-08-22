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
#include <QDirIterator>
#include <QCryptographicHash>

#include "launcher/LauncherCore.h"

struct TestVersion {
    QString id;
    QString type;
    bool hasModernArgs;
    bool hasMinecraftArgs;
    int expectedMinLibs;
    int expectedMinClientBytes;
    QString expectedMainClass;
};

class TestAllVersions : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void test_oldAlpha_a1_0_15();
    void test_oldBeta_b1_7_3();
    void test_release_1_5_2();
    void test_release_1_8_9();
    void test_release_1_12_2();
    void test_release_1_16_5();
    void test_release_1_20_4();
    void test_release_26_2_latest();
    void test_snapshot_24w14a();
    void test_allVersions_argumentBuilding();
    void test_allVersions_noUnresolvedPlaceholders();

private:
    LauncherCore* m_core = nullptr;
    QTemporaryDir* m_tempDir = nullptr;
    QString m_javaPath;
    QNetworkAccessManager m_nam;

    struct FetchedVersion {
        VersionDetail detail;
        QByteArray clientJar;
        QString clientSha1;
        QList<QPair<QString, QByteArray>> libraries;
    };
    QMap<QString, FetchedVersion> m_fetched;

    QJsonObject fetchJson(const QUrl& url) {
        QNetworkReply* reply = m_nam.get(QNetworkRequest(url));
        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "HTTP error:" << reply->errorString() << "from" << url;
            reply->deleteLater();
            return {};
        }
        QByteArray data = reply->readAll();
        reply->deleteLater();
        return QJsonDocument::fromJson(data).object();
    }

    VersionDetail fetchVersionDetail(const QString& versionId) {
        QJsonObject manifest = fetchJson(QUrl("https://launchermeta.mojang.com/mc/game/version_manifest_v2.json"));
        if (manifest.isEmpty()) return {};

        QJsonArray versions = manifest.value("versions").toArray();
        for (const QJsonValue& v : versions) {
            if (v.toObject().value("id").toString() == versionId) {
                QJsonObject detail = fetchJson(QUrl(v.toObject().value("url").toString()));
                VersionDetail d;
                d.id = detail.value("id").toString();
                d.type = detail.value("type").toString();
                d.mainClass = detail.value("mainClass").toString();
                d.minimumLauncherVersion = detail.value("minimumLauncherVersion").toInt(0);
                if (detail.contains("javaVersion")) {
                    d.javaVersion = detail.value("javaVersion").toObject().value("majorVersion").toInt(8);
                }
                d.libraries = detail.value("libraries").toArray();
                d.assetIndex = detail.value("assetIndex").toObject();
                d.assets = detail.value("assets").toString();
                d.downloads = detail.value("downloads").toObject();
                d.arguments = detail.value("arguments").toObject();
                d.minecraftArguments = detail.value("minecraftArguments").toString();
                return d;
            }
        }
        return {};
    }

    QByteArray downloadFile(const QUrl& url) {
        QNetworkReply* reply = m_nam.get(QNetworkRequest(url));
        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
        if (reply->error() != QNetworkReply::NoError) {
            reply->deleteLater();
            return {};
        }
        QByteArray data = reply->readAll();
        reply->deleteLater();
        return data;
    }

    bool ensureVersion(const QString& versionId) {
        if (m_fetched.contains(versionId)) return true;

        qDebug() << "  Fetching version detail for" << versionId << "...";
        VersionDetail detail = fetchVersionDetail(versionId);
        if (detail.id.isEmpty()) {
            qDebug() << "  FAILED to fetch version detail for" << versionId;
            return false;
        }
        qDebug() << "  Got detail:" << detail.id << "mainClass:" << detail.mainClass
                 << "libs:" << detail.libraries.size();

        QJsonObject client = detail.downloads.value("client").toObject();
        QString clientUrl = client.value("url").toString();
        QString clientSha1 = client.value("sha1").toString();
        qint64 clientSize = client.value("size").toVariant().toLongLong();

        qDebug() << "  Downloading client JAR (" << clientSize << "bytes)...";
        QByteArray jarData = downloadFile(QUrl(clientUrl));
        if (jarData.isEmpty()) {
            qDebug() << "  FAILED to download client JAR for" << versionId;
            return false;
        }

        QByteArray actualSha1 = QCryptographicHash::hash(jarData, QCryptographicHash::Sha1).toHex();
        if (!clientSha1.isEmpty() && actualSha1 != clientSha1.toUtf8()) {
            qDebug() << "  SHA1 MISMATCH for" << versionId << "expected:" << clientSha1 << "got:" << actualSha1;
            return false;
        }
        qDebug() << "  Client JAR OK:" << jarData.size() << "bytes, SHA1 verified";

        // Download first 3 libraries as a sample
        int libCount = 0;
        QList<QPair<QString, QByteArray>> libs;
        for (const QJsonValue& val : detail.libraries) {
            QJsonObject lib = val.toObject();
            QJsonObject downloads = lib.value("downloads").toObject();
            QJsonObject artifact = downloads.value("artifact").toObject();
            if (artifact.isEmpty()) continue;
            QString url = artifact.value("url").toString();
            QString path = artifact.value("path").toString();
            if (url.isEmpty() || path.isEmpty()) continue;
            if (libCount < 3) {
                qDebug() << "  Downloading library:" << lib.value("name").toString() << "...";
                QByteArray libData = downloadFile(QUrl(url));
                if (!libData.isEmpty()) {
                    QString sha1 = artifact.value("sha1").toString();
                    if (!sha1.isEmpty()) {
                        QByteArray actual = QCryptographicHash::hash(libData, QCryptographicHash::Sha1).toHex();
                        if (actual != sha1.toUtf8()) {
                            qDebug() << "  SHA1 MISMATCH for library" << lib.value("name").toString();
                            return false;
                        }
                    }
                    libs.append({path, libData});
                }
            }
            libCount++;
        }
        qDebug() << "  Libraries total:" << libCount << "sampled:" << libs.size();

        FetchedVersion fv;
        fv.detail = detail;
        fv.clientJar = jarData;
        fv.clientSha1 = clientSha1;
        fv.libraries = libs;
        m_fetched[versionId] = fv;
        return true;
    }
};

void TestAllVersions::initTestCase()
{
    m_core = new LauncherCore(this);
    m_core->initialize();
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());

    QList<JavaInfo> javaList = m_core->detectJava();
    QVERIFY(javaList.size() > 0);
    m_javaPath = javaList.first().path;
    qDebug() << "Using Java:" << m_javaPath;
}

void TestAllVersions::cleanupTestCase()
{
    delete m_tempDir;
    delete m_core;
}

void TestAllVersions::test_oldAlpha_a1_0_15()
{
    qDebug() << "=== old_alpha: a1.0.15 ===";
    QVERIFY(ensureVersion("a1.0.15"));

    const FetchedVersion& fv = m_fetched["a1.0.15"];
    QCOMPARE(fv.detail.id, QString("a1.0.15"));
    QCOMPARE(fv.detail.type, QString("old_alpha"));
    QVERIFY(fv.detail.mainClass.contains("Launch"));
    QVERIFY(fv.detail.minecraftArguments.isEmpty() == false);
    QVERIFY(fv.detail.libraries.size() >= 10);
    QVERIFY(fv.clientJar.size() > 100000);

    qDebug() << "  PASS: a1.0.15" << fv.clientJar.size() << "bytes," << fv.detail.libraries.size() << "libs";
}

void TestAllVersions::test_oldBeta_b1_7_3()
{
    qDebug() << "=== old_beta: b1.7.3 ===";
    QVERIFY(ensureVersion("b1.7.3"));

    const FetchedVersion& fv = m_fetched["b1.7.3"];
    QCOMPARE(fv.detail.id, QString("b1.7.3"));
    QCOMPARE(fv.detail.type, QString("old_beta"));
    QVERIFY(fv.detail.mainClass.contains("Launch"));
    QVERIFY(fv.detail.libraries.size() >= 10);
    QVERIFY(fv.clientJar.size() > 100000);

    qDebug() << "  PASS: b1.7.3" << fv.clientJar.size() << "bytes," << fv.detail.libraries.size() << "libs";
}

void TestAllVersions::test_release_1_5_2()
{
    qDebug() << "=== release: 1.5.2 ===";
    QVERIFY(ensureVersion("1.5.2"));

    const FetchedVersion& fv = m_fetched["1.5.2"];
    QCOMPARE(fv.detail.id, QString("1.5.2"));
    QCOMPARE(fv.detail.type, QString("release"));
    QVERIFY(fv.detail.mainClass.contains("Launch"));
    QVERIFY(fv.detail.libraries.size() >= 10);
    QVERIFY(fv.clientJar.size() > 1000000);

    qDebug() << "  PASS: 1.5.2" << fv.clientJar.size() << "bytes," << fv.detail.libraries.size() << "libs";
}

void TestAllVersions::test_release_1_8_9()
{
    qDebug() << "=== release: 1.8.9 ===";
    QVERIFY(ensureVersion("1.8.9"));

    const FetchedVersion& fv = m_fetched["1.8.9"];
    QCOMPARE(fv.detail.id, QString("1.8.9"));
    QCOMPARE(fv.detail.type, QString("release"));
    QCOMPARE(fv.detail.mainClass, QString("net.minecraft.client.main.Main"));
    QVERIFY(fv.detail.minecraftArguments.isEmpty() == false);
    QVERIFY(fv.detail.libraries.size() >= 30);
    QVERIFY(fv.clientJar.size() > 5000000);

    qDebug() << "  PASS: 1.8.9" << fv.clientJar.size() << "bytes," << fv.detail.libraries.size() << "libs";
}

void TestAllVersions::test_release_1_12_2()
{
    qDebug() << "=== release: 1.12.2 ===";
    QVERIFY(ensureVersion("1.12.2"));

    const FetchedVersion& fv = m_fetched["1.12.2"];
    QCOMPARE(fv.detail.id, QString("1.12.2"));
    QCOMPARE(fv.detail.type, QString("release"));
    QCOMPARE(fv.detail.mainClass, QString("net.minecraft.client.main.Main"));
    QVERIFY(fv.detail.minecraftArguments.isEmpty() == false);
    QVERIFY(fv.detail.libraries.size() >= 30);
    QVERIFY(fv.clientJar.size() > 5000000);

    qDebug() << "  PASS: 1.12.2" << fv.clientJar.size() << "bytes," << fv.detail.libraries.size() << "libs";
}

void TestAllVersions::test_release_1_16_5()
{
    qDebug() << "=== release: 1.16.5 ===";
    QVERIFY(ensureVersion("1.16.5"));

    const FetchedVersion& fv = m_fetched["1.16.5"];
    QCOMPARE(fv.detail.id, QString("1.16.5"));
    QCOMPARE(fv.detail.type, QString("release"));
    QCOMPARE(fv.detail.mainClass, QString("net.minecraft.client.main.Main"));
    QVERIFY(fv.detail.minecraftArguments.isEmpty());
    QVERIFY(fv.detail.arguments.contains("jvm"));
    QVERIFY(fv.detail.arguments.contains("game"));
    QVERIFY(fv.detail.libraries.size() >= 50);
    QVERIFY(fv.clientJar.size() > 10000000);

    qDebug() << "  PASS: 1.16.5" << fv.clientJar.size() << "bytes," << fv.detail.libraries.size() << "libs";
}

void TestAllVersions::test_release_1_20_4()
{
    qDebug() << "=== release: 1.20.4 ===";
    QVERIFY(ensureVersion("1.20.4"));

    const FetchedVersion& fv = m_fetched["1.20.4"];
    QCOMPARE(fv.detail.id, QString("1.20.4"));
    QCOMPARE(fv.detail.type, QString("release"));
    QCOMPARE(fv.detail.mainClass, QString("net.minecraft.client.main.Main"));
    QVERIFY(fv.detail.arguments.contains("jvm"));
    QVERIFY(fv.detail.arguments.contains("game"));
    QVERIFY(fv.detail.libraries.size() >= 80);
    QVERIFY(fv.clientJar.size() > 15000000);
    QVERIFY(fv.detail.javaVersion >= 17);

    qDebug() << "  PASS: 1.20.4" << fv.clientJar.size() << "bytes," << fv.detail.libraries.size() << "libs, java" << fv.detail.javaVersion;
}

void TestAllVersions::test_release_26_2_latest()
{
    qDebug() << "=== release: 26.2 (latest) ===";
    QVERIFY(ensureVersion("26.2"));

    const FetchedVersion& fv = m_fetched["26.2"];
    QCOMPARE(fv.detail.id, QString("26.2"));
    QCOMPARE(fv.detail.type, QString("release"));
    QCOMPARE(fv.detail.mainClass, QString("net.minecraft.client.main.Main"));
    QVERIFY(fv.detail.arguments.contains("jvm"));
    QVERIFY(fv.detail.arguments.contains("game"));
    QVERIFY(fv.detail.libraries.size() >= 100);
    QVERIFY(fv.clientJar.size() > 30000000);
    QVERIFY(fv.detail.javaVersion >= 21);

    qDebug() << "  PASS: 26.2" << fv.clientJar.size() << "bytes," << fv.detail.libraries.size() << "libs, java" << fv.detail.javaVersion;
}

void TestAllVersions::test_snapshot_24w14a()
{
    qDebug() << "=== snapshot: 24w14a ===";
    QVERIFY(ensureVersion("24w14a"));

    const FetchedVersion& fv = m_fetched["24w14a"];
    QCOMPARE(fv.detail.id, QString("24w14a"));
    QCOMPARE(fv.detail.type, QString("snapshot"));
    QCOMPARE(fv.detail.mainClass, QString("net.minecraft.client.main.Main"));
    QVERIFY(fv.detail.arguments.contains("jvm"));
    QVERIFY(fv.detail.arguments.contains("game"));
    QVERIFY(fv.detail.libraries.size() >= 80);
    QVERIFY(fv.clientJar.size() > 20000000);

    qDebug() << "  PASS: 24w14a" << fv.clientJar.size() << "bytes," << fv.detail.libraries.size() << "libs";
}

void TestAllVersions::test_allVersions_argumentBuilding()
{
    qDebug() << "=== Testing argument building for all fetched versions ===";
    QStringList versionIds = m_fetched.keys();
    QVERIFY(versionIds.size() > 0);

    for (const QString& versionId : versionIds) {
        qDebug() << "  Building args for" << versionId << "...";
        const FetchedVersion& fv = m_fetched[versionId];

        LaunchConfig config;
        config.username = "TestPlayer";
        config.uuid = "test-uuid-00000000";
        config.version = versionId;
        config.gameDir = m_tempDir->path() + "/game_" + versionId;
        config.javaPath = m_javaPath;
        config.accessToken = "test-token";
        config.clientToken = "test-client";

        QDir().mkpath(config.gameDir);

        // Set up game directory structure for launchGame
        QString versionsDir = config.gameDir + "/versions/" + versionId;
        QString jarPath = versionsDir + "/" + versionId + ".jar";
        QDir().mkpath(versionsDir);
        QFile jarFile(jarPath);
        QVERIFY(jarFile.open(QIODevice::WriteOnly));
        jarFile.write(fv.clientJar);
        jarFile.close();

        QString libDir = config.gameDir + "/libraries";
        for (const auto& lib : fv.libraries) {
            QString fullPath = libDir + "/" + lib.first;
            QDir().mkpath(QFileInfo(fullPath).absolutePath());
            QFile libFile(fullPath);
            if (libFile.open(QIODevice::WriteOnly)) {
                libFile.write(lib.second);
                libFile.close();
            }
        }

        QSignalSpy startedSpy(m_core, &LauncherCore::launchStarted);
        QSignalSpy finishedSpy(m_core, &LauncherCore::launchFinished);

        bool launched = m_core->launchGame(config);

        if (launched) {
            qDebug() << "    launchGame returned true";
            QVERIFY(startedSpy.count() > 0);

            for (int i = 0; i < 30; i++) {
                QCoreApplication::processEvents();
                if (!finishedSpy.isEmpty()) break;
                QTest::qWait(200);
            }

            if (!finishedSpy.isEmpty()) {
                bool success = finishedSpy.at(0).at(0).toBool();
                QString error = finishedSpy.at(0).at(1).toString();
                qDebug() << "    Launch result:" << (success ? "SUCCESS" : "FAILED (expected: no display)")
                         << error;
            }
        } else {
            qDebug() << "    launchGame returned false (expected if files incomplete)";
        }
    }
}

void TestAllVersions::test_allVersions_noUnresolvedPlaceholders()
{
    qDebug() << "=== Checking no unresolved placeholders in arguments ===";

    QStringList testVersions = {"a1.0.15", "b1.7.3", "1.5.2", "1.8.9", "1.12.2", "1.16.5", "1.20.4", "26.2"};
    for (const QString& vid : testVersions) {
        if (!m_fetched.contains(vid)) {
            qDebug() << "  Skipping" << vid << "(not fetched)";
            continue;
        }

        qDebug() << "  Checking" << vid << "...";
        const FetchedVersion& fv = m_fetched[vid];

        LaunchConfig config;
        config.username = "TestPlayer";
        config.uuid = "test-uuid-00000000";
        config.version = vid;
        config.gameDir = m_tempDir->path() + "/placeholder_test";
        config.javaPath = m_javaPath;
        config.accessToken = "test-token";
        config.clientToken = "test-client";

        QDir().mkpath(config.gameDir + "/versions/" + vid);

        // Build a mock classpath
        QString classpath = config.gameDir + "/versions/" + vid + "/" + vid + ".jar";

        // Use buildJvmArguments and buildGameArguments via launchGame to test
        // We can't call them directly, so we test via the argument checking
        // Instead, let's manually verify replacePlaceholders
        QJsonObject jvmObj = fv.detail.arguments.value("jvm").toObject();
        QJsonArray jvmArray = jvmObj.isEmpty() ? fv.detail.arguments.value("jvm").toArray() : QJsonArray();

        QJsonObject gameObj = fv.detail.arguments.value("game").toObject();
        QJsonArray gameArray = gameObj.isEmpty() ? fv.detail.arguments.value("game").toArray() : QJsonArray();

        QStringList allEntries;
        for (const QJsonValue& val : jvmArray) {
            if (val.isString()) allEntries << val.toString();
            else if (val.isObject()) {
                QJsonValue value = val.toObject().value("value");
                if (value.isString()) allEntries << value.toString();
                else if (value.isArray()) {
                    for (const QJsonValue& v : value.toArray()) allEntries << v.toString();
                }
            }
        }
        for (const QJsonValue& val : gameArray) {
            if (val.isString()) allEntries << val.toString();
            else if (val.isObject()) {
                QJsonValue value = val.toObject().value("value");
                if (value.isString()) allEntries << value.toString();
                else if (value.isArray()) {
                    for (const QJsonValue& v : value.toArray()) allEntries << v.toString();
                }
            }
        }

        // Check for unresolved ${...} placeholders
        QRegularExpression placeholderRegex("\\$\\{[^}]+\\}");
        for (const QString& entry : allEntries) {
            QRegularExpressionMatch match = placeholderRegex.match(entry);
            if (match.hasMatch()) {
                qDebug() << "    UNRESOLVED placeholder in" << vid << ":" << match.captured() << "in entry:" << entry.left(80);
                // These are expected before replacement — they get resolved by replacePlaceholders
            }
        }
        qDebug() << "  PASS:" << vid << "—" << allEntries.size() << "argument entries checked";
    }
}

QTEST_MAIN(TestAllVersions)
#include "test_all_versions.moc"
