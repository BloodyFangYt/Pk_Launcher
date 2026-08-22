#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QDateTime>

#include "launcher/InstanceManager.h"

class TestInstanceManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void test_createInstance();
    void test_listInstances();
    void test_getInstance();
    void test_updateInstance();
    void test_deleteInstance();
    void test_getInstanceGameDir();
    void test_multipleInstances();
    void test_signals();
    void test_emptyNameRejected();
    void test_emptyVersionRejected();
    void test_pathSeparatorsRejected();
    void test_gameDirUsesId();

private:
    QTemporaryDir m_tempDir;
    InstanceManager* m_manager = nullptr;
};

void TestInstanceManager::initTestCase()
{
    QVERIFY(m_tempDir.isValid());
    m_manager = new InstanceManager(this);
    QVERIFY(m_manager != nullptr);
}

void TestInstanceManager::cleanupTestCase()
{
    delete m_manager;
    m_manager = nullptr;
}

void TestInstanceManager::test_createInstance()
{
    InstanceInfo info = m_manager->createInstance("Test Instance", "1.20.1");

    // Verify returned info
    QVERIFY(!info.id.isEmpty());
    QCOMPARE(info.name, QString("Test Instance"));
    QCOMPARE(info.version, QString("1.20.1"));
    QCOMPARE(info.javaVersion, 17);
    QCOMPARE(info.ramMb, 2048);
    QVERIFY(info.createdAt.isValid());
}

void TestInstanceManager::test_listInstances()
{
    QList<InstanceInfo> instances = m_manager->listInstances();

    // Should have at least the one we created
    QVERIFY(instances.size() >= 1);

    // Find our test instance
    bool found = false;
    for (const InstanceInfo& inst : instances) {
        if (inst.name == "Test Instance") {
            found = true;
            QCOMPARE(inst.version, QString("1.20.1"));
            break;
        }
    }
    QVERIFY(found);
}

void TestInstanceManager::test_getInstance()
{
    // Create an instance to get its ID
    InstanceInfo created = m_manager->createInstance("Get Test", "1.19.4");

    InstanceInfo retrieved = m_manager->getInstance(created.id);
    QCOMPARE(retrieved.id, created.id);
    QCOMPARE(retrieved.name, QString("Get Test"));
    QCOMPARE(retrieved.version, QString("1.19.4"));
}

void TestInstanceManager::test_updateInstance()
{
    InstanceInfo created = m_manager->createInstance("Update Test", "1.18.2");

    // Modify and update
    InstanceInfo updated = created;
    updated.name = "Updated Test Name";
    updated.ramMb = 4096;
    updated.jvmArgs = "-Xmx4G -XX:+UseG1GC";

    bool result = m_manager->updateInstance(updated);
    QVERIFY(result);

    // Verify the update
    InstanceInfo retrieved = m_manager->getInstance(created.id);
    QCOMPARE(retrieved.name, QString("Updated Test Name"));
    QCOMPARE(retrieved.ramMb, 4096);
    QCOMPARE(retrieved.jvmArgs, QString("-Xmx4G -XX:+UseG1GC"));
}

void TestInstanceManager::test_deleteInstance()
{
    InstanceInfo created = m_manager->createInstance("Delete Test", "1.17.1");

    bool result = m_manager->deleteInstance(created.id);
    QVERIFY(result);

    // Verify deletion — list should not contain it
    QList<InstanceInfo> instances = m_manager->listInstances();
    for (const InstanceInfo& inst : instances) {
        QVERIFY(inst.id != created.id);
    }
}

void TestInstanceManager::test_getInstanceGameDir()
{
    InstanceInfo created = m_manager->createInstance("GameDir Test", "1.20.1");

    QString gameDir = m_manager->getInstanceGameDir(created.id);
    QVERIFY(!gameDir.isEmpty());
    QVERIFY(gameDir.contains(created.id));
}

void TestInstanceManager::test_multipleInstances()
{
    // Create several instances
    m_manager->createInstance("Multi 1", "1.20.1");
    m_manager->createInstance("Multi 2", "1.19.4");
    m_manager->createInstance("Multi 3", "1.18.2");

    QList<InstanceInfo> instances = m_manager->listInstances();
    QVERIFY(instances.size() >= 3);

    // Verify all names are unique in the list
    QSet<QString> names;
    for (const InstanceInfo& inst : instances) {
        names.insert(inst.name);
    }
    QVERIFY(names.contains("Multi 1"));
    QVERIFY(names.contains("Multi 2"));
    QVERIFY(names.contains("Multi 3"));
}

void TestInstanceManager::test_signals()
{
    // Test that signals are emitted
    QSignalSpy createdSpy(m_manager, &InstanceManager::instanceCreated);
    QSignalSpy deletedSpy(m_manager, &InstanceManager::instanceDeleted);

    InstanceInfo info = m_manager->createInstance("Signal Test", "1.20.1");
    QCOMPARE(createdSpy.count(), 1);

    m_manager->deleteInstance(info.id);
    QCOMPARE(deletedSpy.count(), 1);
}

void TestInstanceManager::test_emptyNameRejected()
{
    // createInstance with empty name should return empty InstanceInfo
    InstanceInfo info = m_manager->createInstance("", "1.20.1");
    QVERIFY(info.id.isEmpty());
    QCOMPARE(info.name, QString(""));
}

void TestInstanceManager::test_emptyVersionRejected()
{
    // createInstance with empty version should return empty InstanceInfo
    InstanceInfo info = m_manager->createInstance("Test Name", "");
    QVERIFY(info.id.isEmpty());
}

void TestInstanceManager::test_pathSeparatorsRejected()
{
    // createInstance with / in name should return empty InstanceInfo
    InstanceInfo info1 = m_manager->createInstance("bad/name", "1.20.1");
    QVERIFY(info1.id.isEmpty());

    // createInstance with \\ in name should also fail
    InstanceInfo info2 = m_manager->createInstance("bad\\name", "1.20.1");
    QVERIFY(info2.id.isEmpty());
}

void TestInstanceManager::test_gameDirUsesId()
{
    // Verify gameDir contains the instance id, not the name
    InstanceInfo info = m_manager->createInstance("My Cool Instance", "1.20.1");
    QVERIFY(!info.id.isEmpty());

    // gameDir should contain the UUID id, not the name
    QVERIFY(info.gameDir.contains(info.id));
    QVERIFY(!info.gameDir.contains("My Cool Instance"));
}

QTEST_MAIN(TestInstanceManager)
#include "test_instance_manager.moc"
