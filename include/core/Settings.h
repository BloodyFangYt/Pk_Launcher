#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QVariant>

class Settings : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Settings)

public:
    static Settings& instance() {
        static Settings inst;
        return inst;
    }

    void load();
    void save();

    // Java settings
    QString javaAutoDetect() const { return m_javaAutoDetect; }
    void setJavaAutoDetect(const QString& path) { m_javaAutoDetect = path; }

    QStringList javaCustomPaths() const { return m_javaCustomPaths; }
    void setJavaCustomPaths(const QStringList& paths) { m_javaCustomPaths = paths; }

    QStringList defaultJvmArgs() const { return m_defaultJvmArgs; }
    void setDefaultJvmArgs(const QStringList& args) { m_defaultJvmArgs = args; }

    // Launcher settings
    bool autoUpdate() const { return m_autoUpdate; }
    void setAutoUpdate(bool v) { m_autoUpdate = v; }

    bool closeOnLaunch() const { return m_closeOnLaunch; }
    void setCloseOnLaunch(bool v) { m_closeOnLaunch = v; }

    bool showConsole() const { return m_showConsole; }
    void setShowConsole(bool v) { m_showConsole = v; }

    QString language() const { return m_language; }
    void setLanguage(const QString& lang) { m_language = lang; }

    QString instancesDir() const { return m_instancesDir; }
    void setInstancesDir(const QString& dir) { m_instancesDir = dir; }

    // Network settings
    QString proxyType() const { return m_proxyType; }
    void setProxyType(const QString& type) { m_proxyType = type; }

    QString proxyHost() const { return m_proxyHost; }
    void setProxyHost(const QString& host) { m_proxyHost = host; }

    quint16 proxyPort() const { return m_proxyPort; }
    void setProxyPort(quint16 port) { m_proxyPort = port; }

    int timeoutSeconds() const { return m_timeoutSeconds; }
    void setTimeoutSeconds(int s) { m_timeoutSeconds = s; }

    int maxConcurrentDownloads() const { return m_maxConcurrentDownloads; }
    void setMaxConcurrentDownloads(int n) { m_maxConcurrentDownloads = n; }

    // Appearance settings
    QString theme() const { return m_theme; }
    void setTheme(const QString& theme) { m_theme = theme; }

    QString accentColor() const { return m_accentColor; }
    void setAccentColor(const QString& color) { m_accentColor = color; }

    bool animations() const { return m_animations; }
    void setAnimations(bool v) { m_animations = v; }

    qreal uiScale() const { return m_uiScale; }
    void setUiScale(qreal scale) { m_uiScale = scale; }

    // Static path helpers
    static QString configDir() {
        return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/PkLauncher";
    }

    static QString dataDir() {
        return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/PkLauncher";
    }

    static QString defaultInstancesDir() {
        return dataDir() + "/instances";
    }

    static QString javaDir() {
        return dataDir() + "/java";
    }

    static QString assetsDir() {
        return dataDir() + "/assets";
    }

    static QString librariesDir() {
        return dataDir() + "/libraries";
    }

signals:
    void settingsChanged();

private:
    Settings();
    ~Settings() = default;

    void ensureDirectories();

    QString m_javaAutoDetect;
    QStringList m_javaCustomPaths;
    QStringList m_defaultJvmArgs = {
        "-Xmx4G", "-Xms1G",
        "-XX:+UseG1GC", "-XX:+ParallelRefProcEnabled",
        "-XX:MaxGCPauseMillis=200", "-XX:+UnlockExperimentalVMOptions",
        "-XX:+DisableExplicitGC", "-XX:+AlwaysPreTouch"
    };

    bool m_autoUpdate = true;
    bool m_closeOnLaunch = false;
    bool m_showConsole = false;
    QString m_language = "en";
    QString m_instancesDir = defaultInstancesDir();

    QString m_proxyType;
    QString m_proxyHost;
    quint16 m_proxyPort = 0;
    int m_timeoutSeconds = 30;
    int m_maxConcurrentDownloads = 8;

    QString m_theme = "dark";
    QString m_accentColor = "#FF0033";
    bool m_animations = true;
    qreal m_uiScale = 1.0;
};