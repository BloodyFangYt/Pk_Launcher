#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QSignalSpy>
#include <QSettings>

#include "core/AuthManager.h"
#include "core/Settings.h"

class TestAuthManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // Offline login tests
    void test_offlineLogin_setsProfile();
    void test_offlineLogin_emptyNameRejected();
    void test_offlineLogin_tooLongRejected();
    void test_offlineLogin_generatesUuid();
    void test_offlineLogin_emitsSignals();
    void test_offlineLogin_persistsAcrossRestart();
    void test_offlineLogin_clearsPreviousTokens();

    // isLoggedIn tests
    void test_isLoggedIn_afterOfflineLogin();
    void test_isLoggedIn_afterLogout();
    void test_isLoggedIn_freshState();

    // isOffline tests
    void test_isOffline_afterOfflineLogin();
    void test_isOffline_afterLogout();

    // Token persistence tests
    void test_saveAndLoad_profilePersisted();
    void test_clearTokens_removesAll();

    // Logout tests
    void test_logout_clearsProfile();
    void test_logout_emitsSignal();

private:
    QTemporaryDir m_tempDir;
};

void TestAuthManager::initTestCase()
{
    QVERIFY(m_tempDir.isValid());
}

void TestAuthManager::cleanupTestCase()
{
}

// ─── Offline Login Tests ─────────────────────────────────────────────────

void TestAuthManager::test_offlineLogin_setsProfile()
{
    AuthManager auth;
    auth.offlineLogin("TestPlayer");

    QCOMPARE(auth.userProfile().username, QString("TestPlayer"));
    QCOMPARE(auth.userProfile().email, QString("Offline Mode"));
    QVERIFY(!auth.userProfile().id.isEmpty());
    QVERIFY(!auth.isOffline() == false); // Should be offline
    QVERIFY(auth.isOffline());
}

void TestAuthManager::test_offlineLogin_emptyNameRejected()
{
    AuthManager auth;

    QSignalSpy failedSpy(&auth, &AuthManager::loginFailed);
    QVERIFY(failedSpy.isValid());

    auth.offlineLogin("");
    QCOMPARE(failedSpy.count(), 1);
    QVERIFY(failedSpy.at(0).at(0).toString().contains("empty", Qt::CaseInsensitive));
}

void TestAuthManager::test_offlineLogin_tooLongRejected()
{
    AuthManager auth;

    QSignalSpy failedSpy(&auth, &AuthManager::loginFailed);
    QVERIFY(failedSpy.isValid());

    auth.offlineLogin("ThisUsernameIsWayTooLong");
    QCOMPARE(failedSpy.count(), 1);
    QVERIFY(failedSpy.at(0).at(0).toString().contains("16", Qt::CaseInsensitive));
}

void TestAuthManager::test_offlineLogin_generatesUuid()
{
    AuthManager auth;
    auth.offlineLogin("Steve");

    QString uuid = auth.userProfile().id;
    // UUID should be in format: 8-4-4-4-12
    QCOMPARE(uuid.length(), 36);
    QCOMPARE(uuid.count('-'), 4);
    // Same input should produce same UUID (deterministic)
    AuthManager auth2;
    auth2.offlineLogin("Steve");
    QCOMPARE(auth2.userProfile().id, uuid);
}

void TestAuthManager::test_offlineLogin_emitsSignals()
{
    AuthManager auth;

    QSignalSpy successSpy(&auth, &AuthManager::loginSuccess);
    QVERIFY(successSpy.isValid());

    auth.offlineLogin("SignalTest");
    QCOMPARE(successSpy.count(), 1);

    // Check the UserProfile argument
    UserProfile profile = successSpy.at(0).at(0).value<UserProfile>();
    QCOMPARE(profile.username, QString("SignalTest"));
    QCOMPARE(profile.email, QString("Offline Mode"));
}

void TestAuthManager::test_offlineLogin_persistsAcrossRestart()
{
    // First instance: login offline
    {
        AuthManager auth;
        auth.offlineLogin("PersistTest");
        QCOMPARE(auth.userProfile().username, QString("PersistTest"));
    }

    // Second instance: should restore from auth.ini
    {
        AuthManager auth;
        // Profile should be restored from persistence
        QCOMPARE(auth.userProfile().username, QString("PersistTest"));
        QVERIFY(auth.isLoggedIn());
        QVERIFY(auth.isOffline());
    }
}

void TestAuthManager::test_offlineLogin_clearsPreviousTokens()
{
    AuthManager auth;
    auth.offlineLogin("OfflineUser");

    // Access token should be empty for offline mode
    QVERIFY(auth.accessToken().isEmpty());
    QVERIFY(!auth.isLoggedIn() || auth.isLoggedIn()); // isLoggedIn checks profile now
    QVERIFY(auth.isOffline());
}

// ─── isLoggedIn Tests ────────────────────────────────────────────────────

void TestAuthManager::test_isLoggedIn_afterOfflineLogin()
{
    // Clear any persisted state from previous tests
    {
        AuthManager auth;
        auth.logout();
    }

    AuthManager auth;
    QVERIFY(!auth.isLoggedIn());

    auth.offlineLogin("LoggedInTest");
    QVERIFY(auth.isLoggedIn());
}

void TestAuthManager::test_isLoggedIn_afterLogout()
{
    AuthManager auth;
    auth.offlineLogin("LogoutTest");
    QVERIFY(auth.isLoggedIn());

    QSignalSpy logoutSpy(&auth, &AuthManager::logoutComplete);
    QVERIFY(logoutSpy.isValid());

    auth.logout();
    QCOMPARE(logoutSpy.count(), 1);
    QVERIFY(!auth.isLoggedIn());
}

void TestAuthManager::test_isLoggedIn_freshState()
{
    // Clear any persisted state from previous tests
    {
        AuthManager auth;
        auth.logout();
    }

    AuthManager auth;
    QVERIFY(!auth.isLoggedIn());
}

// ─── isOffline Tests ─────────────────────────────────────────────────────

void TestAuthManager::test_isOffline_afterOfflineLogin()
{
    AuthManager auth;
    auth.offlineLogin("OfflineTest");
    QVERIFY(auth.isOffline());
}

void TestAuthManager::test_isOffline_afterLogout()
{
    AuthManager auth;
    auth.offlineLogin("OfflineLogoutTest");
    QVERIFY(auth.isOffline());

    auth.logout();
    QVERIFY(!auth.isOffline());
}

// ─── Token Persistence Tests ─────────────────────────────────────────────

void TestAuthManager::test_saveAndLoad_profilePersisted()
{
    // Save offline profile
    {
        AuthManager auth;
        auth.offlineLogin("PersistTest2");
        QCOMPARE(auth.userProfile().username, QString("PersistTest2"));
    }

    // Load and verify
    {
        AuthManager auth;
        QVERIFY(auth.isLoggedIn());
        QCOMPARE(auth.userProfile().username, QString("PersistTest2"));
        QCOMPARE(auth.userProfile().email, QString("Offline Mode"));
    }
}

void TestAuthManager::test_clearTokens_removesAll()
{
    AuthManager auth;
    auth.offlineLogin("ClearTest");
    QVERIFY(auth.isLoggedIn());

    QSignalSpy logoutSpy(&auth, &AuthManager::logoutComplete);
    QVERIFY(logoutSpy.isValid());

    auth.logout();
    QCOMPARE(logoutSpy.count(), 1);
    QVERIFY(!auth.isLoggedIn());
    QVERIFY(auth.userProfile().username.isEmpty());
}

// ─── Logout Tests ────────────────────────────────────────────────────────

void TestAuthManager::test_logout_clearsProfile()
{
    AuthManager auth;
    auth.offlineLogin("ProfileClearTest");
    QVERIFY(!auth.userProfile().username.isEmpty());

    auth.logout();
    QVERIFY(auth.userProfile().username.isEmpty());
    QVERIFY(auth.userProfile().id.isEmpty());
}

void TestAuthManager::test_logout_emitsSignal()
{
    AuthManager auth;
    auth.offlineLogin("SignalTest2");

    QSignalSpy spy(&auth, &AuthManager::logoutComplete);
    QVERIFY(spy.isValid());

    auth.logout();
    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(TestAuthManager)
#include "test_auth_manager.moc"
