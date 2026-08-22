#include "core/Application.h"
#include "launcher/LauncherCore.h"
#include "core/AuthManager.h"
#include "launcher/InstanceManager.h"
#include "core/UpdateManager.h"
#include "core/Settings.h"

Application::Application(QObject* parent) : QObject(parent)
{
}

Application::~Application()
{
    shutdown();
}

bool Application::initialize()
{
    // Initialize launcher core
    m_launcherCore = new LauncherCore(this);
    if (!m_launcherCore->initialize()) {
        return false;
    }

    // Initialize auth manager
    m_authManager = new AuthManager(this);

    // Initialize instance manager
    m_instanceManager = new InstanceManager(this);

    // Initialize update manager
    m_updateManager = new UpdateManager(this);

    emit initialized();
    return true;
}

void Application::shutdown()
{
    if (m_updateManager) {
        delete m_updateManager;
        m_updateManager = nullptr;
    }
    if (m_instanceManager) {
        delete m_instanceManager;
        m_instanceManager = nullptr;
    }
    if (m_authManager) {
        delete m_authManager;
        m_authManager = nullptr;
    }
    if (m_launcherCore) {
        delete m_launcherCore;
        m_launcherCore = nullptr;
    }
}