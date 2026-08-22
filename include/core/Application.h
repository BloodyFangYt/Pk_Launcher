#pragma once

#include <QObject>
#include <QThread>

class LauncherCore;
class AuthManager;
class InstanceManager;
class UpdateManager;

class Application : public QObject
{
    Q_OBJECT

public:
    explicit Application(QObject* parent = nullptr);
    ~Application();

    bool initialize();
    void shutdown();

    LauncherCore* launcherCore() const { return m_launcherCore; }
    AuthManager* authManager() const { return m_authManager; }
    InstanceManager* instanceManager() const { return m_instanceManager; }
    UpdateManager* updateManager() const { return m_updateManager; }

signals:
    void initialized();
    void shutdownRequested();

private:
    LauncherCore* m_launcherCore = nullptr;
    AuthManager* m_authManager = nullptr;
    InstanceManager* m_instanceManager = nullptr;
    UpdateManager* m_updateManager = nullptr;
};