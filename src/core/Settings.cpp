#include "core/Settings.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDebug>

Settings::Settings()
{
    ensureDirectories();
}

void Settings::ensureDirectories()
{
    QDir dir;
    dir.mkpath(configDir());
    dir.mkpath(dataDir());
    dir.mkpath(defaultInstancesDir());
    dir.mkpath(javaDir());
    dir.mkpath(assetsDir());
    dir.mkpath(librariesDir());
}

void Settings::load()
{
    QFile file(configDir() + "/settings.json");
    if (!file.exists()) {
        // Set defaults
        m_instancesDir = defaultInstancesDir();
        m_defaultJvmArgs = {
            "-Xmx4G", "-Xms1G",
            "-XX:+UseG1GC", "-XX:+ParallelRefProcEnabled",
            "-XX:MaxGCPauseMillis=200", "-XX:+UnlockExperimentalVMOptions",
            "-XX:+DisableExplicitGC", "-XX:+AlwaysPreTouch"
        };
        save();
        return;
    }

    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject obj = doc.object();

        m_javaAutoDetect = obj.value("javaAutoDetect").toString();
        const auto readStringList = [](const QJsonValue& value) {
            QStringList result;
            for (const QJsonValue& item : value.toArray()) {
                if (item.isString()) {
                    result.append(item.toString());
                }
            }
            return result;
        };
        m_javaCustomPaths = readStringList(obj.value("javaCustomPaths"));
        m_defaultJvmArgs = readStringList(obj.value("defaultJvmArgs"));

        m_autoUpdate = obj.value("autoUpdate").toBool(true);
        m_closeOnLaunch = obj.value("closeOnLaunch").toBool(false);
        m_showConsole = obj.value("showConsole").toBool(false);
        m_language = obj.value("language").toString("en");
        m_instancesDir = obj.value("instancesDir").toString(defaultInstancesDir());

        m_proxyType = obj.value("proxyType").toString();
        m_proxyHost = obj.value("proxyHost").toString();
        m_proxyPort = obj.value("proxyPort").toInt(0);
        m_timeoutSeconds = obj.value("timeoutSeconds").toInt(30);
        m_maxConcurrentDownloads = obj.value("maxConcurrentDownloads").toInt(8);

        m_theme = obj.value("theme").toString("dark");
        m_accentColor = obj.value("accentColor").toString("#FF0033");
        m_animations = obj.value("animations").toBool(true);
        m_uiScale = obj.value("uiScale").toDouble(1.0);

        file.close();
    }

    emit settingsChanged();
}

void Settings::save()
{
    QJsonObject obj;
    obj["javaAutoDetect"] = m_javaAutoDetect;
    obj["javaCustomPaths"] = QJsonArray::fromStringList(m_javaCustomPaths);
    obj["defaultJvmArgs"] = QJsonArray::fromStringList(m_defaultJvmArgs);

    obj["autoUpdate"] = m_autoUpdate;
    obj["closeOnLaunch"] = m_closeOnLaunch;
    obj["showConsole"] = m_showConsole;
    obj["language"] = m_language;
    obj["instancesDir"] = m_instancesDir;

    obj["proxyType"] = m_proxyType;
    obj["proxyHost"] = m_proxyHost;
    obj["proxyPort"] = m_proxyPort;
    obj["timeoutSeconds"] = m_timeoutSeconds;
    obj["maxConcurrentDownloads"] = m_maxConcurrentDownloads;

    obj["theme"] = m_theme;
    obj["accentColor"] = m_accentColor;
    obj["animations"] = m_animations;
    obj["uiScale"] = m_uiScale;

    QJsonDocument doc(obj);
    QFile file(configDir() + "/settings.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    } else {
        qWarning() << "Failed to save settings to" << file.fileName() <<":" << file.errorString();
    }

    emit settingsChanged();
}