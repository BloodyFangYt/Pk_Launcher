#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QListWidget>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QHash>

class LauncherCore;
struct DownloadProgress;

struct DownloadItem {
    QString name;
    QString url;
    QString destPath;
    QString expectedSha1;
    qint64 totalBytes = 0;
    qint64 downloadedBytes = 0;
    bool completed = false;
    bool failed = false;
    QString error;
};

class DownloadManager : public QWidget
{
    Q_OBJECT

public:
    explicit DownloadManager(LauncherCore* launcherCore, QWidget* parent = nullptr);

    void addDownload(const DownloadItem& item);
    void clearCompleted();

signals:
    void allDownloadsComplete();

private slots:
    void onDownloadProgress(const DownloadProgress& progress);
    void onDownloadComplete(const QString& name);
    void onDownloadError(const QString& name, const QString& error);

private:
    void setupUI();

    LauncherCore* m_launcherCore;
    QListWidget* m_downloadList;
    QLabel* m_statusLabel;
    QProgressBar* m_overallProgress;
    QHash<QString, QProgressBar*> m_itemProgressBars;
};