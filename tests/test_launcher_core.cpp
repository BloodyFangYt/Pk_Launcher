#include <QtTest/QtTest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "launcher/LauncherCore.h"

class TestLauncherCore : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void test_construction();
    void test_javaDetection();
    void test_findBestJava();
    void test_getRecommendedJvmArgs();
    void test_versionEntryDefaults();
    void test_versionDetailDefaults();
    void test_javaInfoDefaults();
    void test_launchConfigDefaults();
    void test_downloadProgressDefaults();

private:
    LauncherCore* m_core = nullptr;
};

void TestLauncherCore::initTestCase()
{
    m_core = new LauncherCore(this);
    QVERIFY(m_core != nullptr);
}

void TestLauncherCore::cleanupTestCase()
{
    delete m_core;
    m_core = nullptr;
}

void TestLauncherCore::test_construction()
{
    // LauncherCore should construct without crashing
    LauncherCore core;
    QVERIFY(true); // If we got here, construction succeeded
}

void TestLauncherCore::test_javaDetection()
{
    // detectJava should return a list (may be empty in CI, but shouldn't crash)
    QList<JavaInfo> javaList = m_core->detectJava();
    QVERIFY(javaList.size() >= 0); // Can be empty in test env

    // Each detected Java should have valid info
    for (const JavaInfo& info : javaList) {
        QVERIFY(!info.path.isEmpty());
        QVERIFY(!info.version.isEmpty());
        QVERIFY(info.majorVersion > 0);
    }
}

void TestLauncherCore::test_findBestJava()
{
    // Create a mock Java list
    JavaInfo java8;
    java8.path = "/usr/lib/jvm/java-8";
    java8.version = "1.8.0_362";
    java8.majorVersion = 8;
    java8.is64Bit = true;

    JavaInfo java17;
    java17.path = "/usr/lib/jvm/java-17";
    java17.version = "17.0.2";
    java17.majorVersion = 17;
    java17.is64Bit = true;

    JavaInfo java21;
    java21.path = "/usr/lib/jvm/java-21";
    java21.version = "21.0.1";
    java21.majorVersion = 21;
    java21.is64Bit = true;

    QList<JavaInfo> javaList = {java8, java17, java21};

    // Should find Java 17 when requiring 17
    JavaInfo best = m_core->findBestJava(17, javaList);
    QCOMPARE(best.majorVersion, 17);

    // Should find Java 21 when requiring 21
    best = m_core->findBestJava(21, javaList);
    QCOMPARE(best.majorVersion, 21);

    // Should find Java 8 when requiring 8
    best = m_core->findBestJava(8, javaList);
    QCOMPARE(best.majorVersion, 8);
}

void TestLauncherCore::test_getRecommendedJvmArgs()
{
    // Java 8 args
    QStringList java8Args = m_core->getRecommendedJvmArgs(8);
    QVERIFY(!java8Args.isEmpty());
    // Should not contain G1GC (not available in Java 8)
    for (const QString& arg : java8Args) {
        QVERIFY(!arg.contains("UseG1GC"));
    }

    // Java 17 args
    QStringList java17Args = m_core->getRecommendedJvmArgs(17);
    QVERIFY(!java17Args.isEmpty());
    // Should contain G1GC
    bool hasG1 = false;
    for (const QString& arg : java17Args) {
        if (arg.contains("UseG1GC")) hasG1 = true;
    }
    QVERIFY(hasG1);

    // Java 21 args
    QStringList java21Args = m_core->getRecommendedJvmArgs(21);
    QVERIFY(!java21Args.isEmpty());
}

void TestLauncherCore::test_versionEntryDefaults()
{
    VersionEntry entry;
    QVERIFY(entry.id.isEmpty());
    QVERIFY(entry.type.isEmpty());
    QVERIFY(entry.url.isEmpty());
    QVERIFY(!entry.releaseTime.isValid());
}

void TestLauncherCore::test_versionDetailDefaults()
{
    VersionDetail detail;
    QVERIFY(detail.id.isEmpty());
    QVERIFY(detail.type.isEmpty());
    QVERIFY(detail.mainClass.isEmpty());
    QCOMPARE(detail.minimumLauncherVersion, 0);
    QCOMPARE(detail.javaVersion, 8);
    QVERIFY(detail.libraries.isEmpty());
    QVERIFY(detail.assetIndex.isEmpty());
    QVERIFY(detail.assets.isEmpty());
    QVERIFY(detail.downloads.isEmpty());
    QVERIFY(detail.arguments.isEmpty());
    QVERIFY(detail.minecraftArguments.isEmpty());
}

void TestLauncherCore::test_javaInfoDefaults()
{
    JavaInfo info;
    QVERIFY(info.path.isEmpty());
    QVERIFY(info.version.isEmpty());
    QCOMPARE(info.majorVersion, 0);
    QVERIFY(!info.is64Bit);
}

void TestLauncherCore::test_launchConfigDefaults()
{
    LaunchConfig config;
    QVERIFY(config.username.isEmpty());
    QVERIFY(config.uuid.isEmpty());
    QVERIFY(config.version.isEmpty());
    QVERIFY(config.gameDir.isEmpty());
    QVERIFY(config.javaPath.isEmpty());
    QVERIFY(config.jvmArgs.isEmpty());
    QVERIFY(config.gameArgs.isEmpty());
    QVERIFY(config.accessToken.isEmpty());
    QVERIFY(config.clientToken.isEmpty());
    QVERIFY(!config.resolution.isValid());
}

void TestLauncherCore::test_downloadProgressDefaults()
{
    DownloadProgress progress;
    QVERIFY(progress.currentFile.isEmpty());
    QCOMPARE(progress.bytesDownloaded, static_cast<qint64>(0));
    QCOMPARE(progress.totalBytes, static_cast<qint64>(0));
    QCOMPARE(progress.completed, 0);
    QCOMPARE(progress.total, 0);
}

QTEST_MAIN(TestLauncherCore)
#include "test_launcher_core.moc"
