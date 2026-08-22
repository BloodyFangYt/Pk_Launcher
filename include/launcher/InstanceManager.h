#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QJsonArray>

struct InstanceInfo {
    QString id;
    QString name;
    QString version;
    QString loader;
    QString loaderVersion;
    int javaVersion = 17;
    int ramMb = 2048;
    QString jvmArgs;
    QString gameDir;
    QDateTime createdAt;
    QDateTime updatedAt;
    QDateTime lastPlayed;
    qint64 playTime = 0;
};

class InstanceManager : public QObject
{
    Q_OBJECT

public:
    explicit InstanceManager(QObject* parent = nullptr);
    ~InstanceManager();

    QList<InstanceInfo> listInstances();
    InstanceInfo createInstance(const QString& name, const QString& version);
    bool deleteInstance(const QString& id);
    InstanceInfo getInstance(const QString& id);
    bool updateInstance(const InstanceInfo& info);
    QString getInstanceGameDir(const QString& id);

signals:
    void instanceCreated(const InstanceInfo& info);
    void instanceDeleted(const QString& id);
    void instanceUpdated(const InstanceInfo& info);

private:
    QSqlDatabase m_db;
    void initDatabase();
    InstanceInfo rowToInstance(const QSqlQuery& query);
};