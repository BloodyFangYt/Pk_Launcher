#include "launcher/InstanceManager.h"
#include "core/Settings.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <QUuid>
#include <QDateTime>
#include <QDebug>

InstanceManager::InstanceManager(QObject* parent) : QObject(parent)
{
    initDatabase();
}

InstanceManager::~InstanceManager()
{
    if (m_db.isOpen()) m_db.close();
}

void InstanceManager::initDatabase()
{
    m_db = QSqlDatabase::addDatabase("QSQLITE", "instances");
    QString dbPath = Settings::dataDir() + "/instances.db";
    QDir().mkpath(QFileInfo(dbPath).absolutePath());
    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        qWarning() << "Failed to open instances database:" << m_db.lastError();
        return;
    }

    QSqlQuery query(m_db);
    query.exec(R"(
        CREATE TABLE IF NOT EXISTS instances (
            id TEXT PRIMARY KEY,
            name TEXT NOT NULL,
            version TEXT NOT NULL,
            loader TEXT DEFAULT 'vanilla',
            loader_version TEXT,
            java_version INTEGER DEFAULT 17,
            ram_mb INTEGER DEFAULT 2048,
            jvm_args TEXT,
            game_dir TEXT NOT NULL,
            created_at TEXT NOT NULL,
            updated_at TEXT NOT NULL,
            last_played TEXT,
            play_time INTEGER DEFAULT 0
        )
    )");
}

InstanceInfo InstanceManager::rowToInstance(const QSqlQuery& query)
{
    InstanceInfo info;
    info.id = query.value("id").toString();
    info.name = query.value("name").toString();
    info.version = query.value("version").toString();
    info.loader = query.value("loader").toString();
    info.loaderVersion = query.value("loader_version").toString();
    info.javaVersion = query.value("java_version").toInt();
    info.ramMb = query.value("ram_mb").toInt();
    info.jvmArgs = query.value("jvm_args").toString();
    info.gameDir = query.value("game_dir").toString();
    info.createdAt = QDateTime::fromString(query.value("created_at").toString(), Qt::ISODate);
    info.updatedAt = QDateTime::fromString(query.value("updated_at").toString(), Qt::ISODate);
    info.lastPlayed = QDateTime::fromString(query.value("last_played").toString(), Qt::ISODate);
    info.playTime = query.value("play_time").toLongLong();
    return info;
}

QList<InstanceInfo> InstanceManager::listInstances()
{
    QList<InstanceInfo> list;
    QSqlQuery query("SELECT * FROM instances ORDER BY created_at DESC", m_db);
    while (query.next()) {
        list.append(rowToInstance(query));
    }
    return list;
}

InstanceInfo InstanceManager::createInstance(const QString& name, const QString& version)
{
    // Input validation
    if (name.trimmed().isEmpty()) {
        qWarning() << "createInstance: name must not be empty";
        return InstanceInfo();
    }
    if (name.contains('/') || name.contains('\\')) {
        qWarning() << "createInstance: name must not contain path separators";
        return InstanceInfo();
    }
    if (name.length() > 64) {
        qWarning() << "createInstance: name must not exceed 64 characters";
        return InstanceInfo();
    }
    if (version.trimmed().isEmpty()) {
        qWarning() << "createInstance: version must not be empty";
        return InstanceInfo();
    }

    InstanceInfo info;
    info.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    info.name = name.trimmed();
    info.version = version.trimmed();
    info.loader = "vanilla";
    info.javaVersion = 17;
    info.ramMb = 2048;
    info.gameDir = Settings::defaultInstancesDir() + "/" + info.id;
    info.createdAt = QDateTime::currentDateTime();
    info.updatedAt = info.createdAt;

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO instances (id, name, version, loader, loader_version, java_version, ram_mb, jvm_args, game_dir, created_at, updated_at)
        VALUES (:id, :name, :version, :loader, :loader_version, :java_version, :ram_mb, :jvm_args, :game_dir, :created_at, :updated_at)
    )");
    query.bindValue(":id", info.id);
    query.bindValue(":name", info.name);
    query.bindValue(":version", info.version);
    query.bindValue(":loader", info.loader);
    query.bindValue(":loader_version", info.loaderVersion);
    query.bindValue(":java_version", info.javaVersion);
    query.bindValue(":ram_mb", info.ramMb);
    query.bindValue(":jvm_args", info.jvmArgs);
    query.bindValue(":game_dir", info.gameDir);
    query.bindValue(":created_at", info.createdAt.toString(Qt::ISODate));
    query.bindValue(":updated_at", info.updatedAt.toString(Qt::ISODate));

    if (!query.exec()) {
        qWarning() << "Failed to create instance:" << query.lastError();
        return InstanceInfo();
    }

    QDir().mkpath(info.gameDir);
    emit instanceCreated(info);
    return info;
}

bool InstanceManager::deleteInstance(const QString& id)
{
    QSqlQuery query(m_db);
    query.prepare("DELETE FROM instances WHERE id = :id");
    query.bindValue(":id", id);

    bool success = query.exec();
    if (success) {
        emit instanceDeleted(id);
    }
    return success;
}

InstanceInfo InstanceManager::getInstance(const QString& id)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM instances WHERE id = :id");
    query.bindValue(":id", id);

    if (query.exec() && query.next()) {
        return rowToInstance(query);
    }
    return InstanceInfo();
}

bool InstanceManager::updateInstance(const InstanceInfo& info)
{
    // Input validation
    if (info.name.trimmed().isEmpty()) {
        qWarning() << "updateInstance: name must not be empty";
        return false;
    }
    if (info.name.contains('/') || info.name.contains('\\')) {
        qWarning() << "updateInstance: name must not contain path separators";
        return false;
    }
    if (info.name.length() > 64) {
        qWarning() << "updateInstance: name must not exceed 64 characters";
        return false;
    }
    if (info.version.trimmed().isEmpty()) {
        qWarning() << "updateInstance: version must not be empty";
        return false;
    }

    QSqlQuery query(m_db);
    query.prepare(R"(
        UPDATE instances SET
            name = :name, version = :version, loader = :loader,
            loader_version = :loader_version, java_version = :java_version,
            ram_mb = :ram_mb, jvm_args = :jvm_args, game_dir = :game_dir,
            updated_at = :updated_at, last_played = :last_played, play_time = :play_time
        WHERE id = :id
    )");
    InstanceInfo updated = info;
    updated.updatedAt = QDateTime::currentDateTime();

    query.bindValue(":id", updated.id);
    query.bindValue(":name", updated.name);
    query.bindValue(":version", updated.version);
    query.bindValue(":loader", updated.loader);
    query.bindValue(":loader_version", updated.loaderVersion);
    query.bindValue(":java_version", updated.javaVersion);
    query.bindValue(":ram_mb", updated.ramMb);
    query.bindValue(":jvm_args", updated.jvmArgs);
    query.bindValue(":game_dir", updated.gameDir);
    query.bindValue(":updated_at", updated.updatedAt.toString(Qt::ISODate));
    query.bindValue(":last_played", updated.lastPlayed.toString(Qt::ISODate));
    query.bindValue(":play_time", updated.playTime);

    bool success = query.exec();
    if (success) emit instanceUpdated(updated);
    return success;
}

QString InstanceManager::getInstanceGameDir(const QString& id)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT game_dir FROM instances WHERE id = :id");
    query.bindValue(":id", id);

    if (query.exec() && query.next()) {
        return query.value("game_dir").toString();
    }
    return QString();
}