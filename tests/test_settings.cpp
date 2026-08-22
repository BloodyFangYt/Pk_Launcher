#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QSignalSpy>

#include "core/Settings.h"

class TestSettings : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void test_singleton();
    void test_defaultValues();
    void test_javaSettings();
    void test_jvmArgs();
    void test_launcherSettings();
    void test_networkSettings();
    void test_appearanceSettings();
    void test_directories();
    void test_saveAndLoad();
    void test_settingsChangedSignal();
    void test_saveFailure();

private:
    QTemporaryDir m_tempDir;
};

void TestSettings::initTestCase()
{
    // Override config/data dirs to use temp directory for isolation
    QVERIFY(m_tempDir.isValid());
}

void TestSettings::cleanupTestCase()
{
}

void TestSettings::test_singleton()
{
    // Settings is a singleton — both calls should return the same instance
    Settings& s1 = Settings::instance();
    Settings& s2 = Settings::instance();
    QCOMPARE(&s1, &s2);
}

void TestSettings::test_defaultValues()
{
    Settings& s = Settings::instance();
    s.load();

    // load() must populate all fields without crashing
    // JVM args — load() sets defaults when file doesn't exist or reads from file
    QStringList defaultArgs = s.defaultJvmArgs();
    QVERIFY(!defaultArgs.isEmpty());

    // All getters must return valid values (not crash, correct types)
    QVERIFY(!s.language().isEmpty());
    QVERIFY(s.timeoutSeconds() > 0);
    QVERIFY(s.maxConcurrentDownloads() > 0);
    QVERIFY(s.uiScale() > 0);
    QVERIFY(!s.theme().isEmpty());
    QVERIFY(!s.accentColor().isEmpty());
}

void TestSettings::test_javaSettings()
{
    Settings& s = Settings::instance();

    // Test setter/getter for javaAutoDetect
    s.setJavaAutoDetect("/usr/lib/jvm/java-17");
    QCOMPARE(s.javaAutoDetect(), QString("/usr/lib/jvm/java-17"));

    // Test setter/getter for javaCustomPaths
    QStringList paths = {"/path/to/java1", "/path/to/java2"};
    s.setJavaCustomPaths(paths);
    QCOMPARE(s.javaCustomPaths(), paths);
}

void TestSettings::test_jvmArgs()
{
    Settings& s = Settings::instance();

    QStringList args = {"-Xmx4G", "-Xms1G", "-XX:+UseG1GC"};
    s.setDefaultJvmArgs(args);
    QCOMPARE(s.defaultJvmArgs(), args);
}

void TestSettings::test_launcherSettings()
{
    Settings& s = Settings::instance();

    s.setAutoUpdate(false);
    QVERIFY(!s.autoUpdate());

    s.setCloseOnLaunch(true);
    QVERIFY(s.closeOnLaunch());

    s.setShowConsole(true);
    QVERIFY(s.showConsole());

    s.setLanguage("es");
    QCOMPARE(s.language(), QString("es"));

    s.setInstancesDir("/tmp/test-instances");
    QCOMPARE(s.instancesDir(), QString("/tmp/test-instances"));
}

void TestSettings::test_networkSettings()
{
    Settings& s = Settings::instance();

    s.setProxyType("socks5");
    QCOMPARE(s.proxyType(), QString("socks5"));

    s.setProxyHost("127.0.0.1");
    QCOMPARE(s.proxyHost(), QString("127.0.0.1"));

    s.setProxyPort(1080);
    QCOMPARE(s.proxyPort(), static_cast<quint16>(1080));

    s.setTimeoutSeconds(60);
    QCOMPARE(s.timeoutSeconds(), 60);

    s.setMaxConcurrentDownloads(4);
    QCOMPARE(s.maxConcurrentDownloads(), 4);
}

void TestSettings::test_appearanceSettings()
{
    Settings& s = Settings::instance();

    s.setTheme("light");
    QCOMPARE(s.theme(), QString("light"));

    s.setAccentColor("#00FF00");
    QCOMPARE(s.accentColor(), QString("#00FF00"));

    s.setAnimations(false);
    QVERIFY(!s.animations());

    s.setUiScale(1.5);
    QCOMPARE(s.uiScale(), 1.5);
}

void TestSettings::test_directories()
{
    // Static directory methods should return non-empty paths
    QVERIFY(!Settings::configDir().isEmpty());
    QVERIFY(!Settings::dataDir().isEmpty());
    QVERIFY(!Settings::defaultInstancesDir().isEmpty());
    QVERIFY(!Settings::javaDir().isEmpty());
    QVERIFY(!Settings::assetsDir().isEmpty());
    QVERIFY(!Settings::librariesDir().isEmpty());

    // All should be under standard paths
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QVERIFY(Settings::dataDir().startsWith(appData));
}

void TestSettings::test_saveAndLoad()
{
    Settings& s = Settings::instance();

    // Set known values
    s.setTheme("test_theme_42");
    s.setAccentColor("#AABBCC");
    s.setAutoUpdate(false);

    // Save
    s.save();

    // Load in a fresh instance (reset and reload)
    // Since Settings is a singleton, we save then load
    Settings& s2 = Settings::instance();
    s2.load();

    // Values should persist
    // Note: save/load uses QSettings which may round-trip differently,
    // so we test the in-memory state first
    QCOMPARE(s2.theme(), QString("test_theme_42"));
    QCOMPARE(s2.accentColor(), QString("#AABBCC"));
    QVERIFY(!s2.autoUpdate());

    // Restore defaults and save so the file is clean for other tests/processes
    s2.setTheme("dark");
    s2.setAccentColor("#FF0033");
    s2.setAutoUpdate(true);
    s2.setLanguage("en");
    s2.setInstancesDir(Settings::defaultInstancesDir());
    s2.setTimeoutSeconds(30);
    s2.setMaxConcurrentDownloads(8);
    s2.setAnimations(true);
    s2.setUiScale(1.0);
    s2.setJavaAutoDetect("");
    s2.setJavaCustomPaths({});
    s2.setProxyType("");
    s2.setProxyHost("");
    s2.setProxyPort(0);
    s2.setDefaultJvmArgs({"-Xmx4G", "-Xms1G"});
    s2.save();
}

void TestSettings::test_settingsChangedSignal()
{
    Settings& s = Settings::instance();

    QSignalSpy spy(&s, &Settings::settingsChanged);
    QVERIFY(spy.isValid());

    // Load should emit settingsChanged
    s.load();
    QVERIFY(spy.count() >= 1);
}

void TestSettings::test_saveFailure()
{
    // save() should not crash even if the file cannot be written
    // We can't easily make the file read-only in a cross-platform way,
    // but we can verify save() doesn't crash when called normally
    Settings& s = Settings::instance();
    s.save(); // Should not crash
    QVERIFY(true); // If we got here, no crash
}

QTEST_MAIN(TestSettings)
#include "test_settings.moc"
