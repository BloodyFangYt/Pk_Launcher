#include "ui/DownloadManager.h"
#include "launcher/LauncherCore.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QFrame>

DownloadManager::DownloadManager(LauncherCore* launcherCore, QWidget* parent)
    : QWidget(parent), m_launcherCore(launcherCore)
{
    setupUI();
}

void DownloadManager::setupUI()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(32, 32, 32, 32);
    layout->setSpacing(16);

    // Header
    QHBoxLayout* header = new QHBoxLayout();
    QLabel* title = new QLabel("📥  Downloads");
    title->setStyleSheet("color: #e5e2e1; font-size: 20px; font-weight: bold;");
    header->addWidget(title);
    header->addStretch();

    QPushButton* clearBtn = new QPushButton("Clear Completed");
    clearBtn->setStyleSheet("background: transparent; color: #c8c6c5; border: 1px solid #353534; padding: 8px 16px; border-radius: 4px;");
    connect(clearBtn, &QPushButton::clicked, this, &DownloadManager::clearCompleted);
    header->addWidget(clearBtn);

    layout->addLayout(header);

    // Overall progress
    m_overallProgress = new QProgressBar();
    m_overallProgress->setTextVisible(false);
    m_overallProgress->setFixedHeight(6);
    m_overallProgress->setStyleSheet(R"(
        QProgressBar { background: #353534; border: none; border-radius: 3px; }
        QProgressBar::chunk { background: #FF0033; border-radius: 3px; }
    )");
    m_overallProgress->hide();
    layout->addWidget(m_overallProgress);

    // Download list
    m_downloadList = new QListWidget();
    m_downloadList->setStyleSheet(R"(
        QListWidget { background: #1c1b1b; border: 1px solid #262626; border-radius: 8px; }
        QListWidget::item { padding: 16px; border-bottom: 1px solid #262626; }
        QListWidget::item:last { border-bottom: none; }
    )");
    layout->addWidget(m_downloadList, 1);

    // Status
    m_statusLabel = new QLabel("No active downloads");
    m_statusLabel->setStyleSheet("color: #c8c6c5; font-family: 'JetBrains Mono'; font-size: 11px;");
    layout->addWidget(m_statusLabel);

    connect(m_launcherCore, &LauncherCore::downloadProgress, this, &DownloadManager::onDownloadProgress);
}

void DownloadManager::addDownload(const DownloadItem& item)
{
    QListWidgetItem* listItem = new QListWidgetItem();
    m_downloadList->addItem(listItem);

    QWidget* itemWidget = new QWidget();
    QVBoxLayout* itemLayout = new QVBoxLayout(itemWidget);
    itemLayout->setContentsMargins(0, 0, 0, 0);
    itemLayout->setSpacing(8);

    QHBoxLayout* topLayout = new QHBoxLayout();
    QLabel* nameLabel = new QLabel(item.name);
    nameLabel->setStyleSheet("color: #e5e2e1; font-weight: bold; font-size: 13px;");
    topLayout->addWidget(nameLabel);
    topLayout->addStretch();
    QLabel* statusLabel = new QLabel("Pending");
    statusLabel->setStyleSheet("color: #c8c6c5; font-family: 'JetBrains Mono'; font-size: 11px;");
    statusLabel->setProperty("name", item.name);
    topLayout->addWidget(statusLabel);
    itemLayout->addLayout(topLayout);

    QProgressBar* progressBar = new QProgressBar();
    progressBar->setTextVisible(false);
    progressBar->setFixedHeight(4);
    progressBar->setStyleSheet(R"(
        QProgressBar { background: #353534; border: none; border-radius: 2px; }
        QProgressBar::chunk { background: #FF0033; border-radius: 2px; }
    )");
    m_itemProgressBars[item.name] = progressBar;
    itemLayout->addWidget(progressBar);

    listItem->setSizeHint(itemWidget->sizeHint());
    m_downloadList->setItemWidget(listItem, itemWidget);

    m_overallProgress->show();
    m_statusLabel->setText(QString("Downloading %1 file(s)...").arg(m_downloadList->count()));
}

void DownloadManager::clearCompleted()
{
    for (int i = m_downloadList->count() - 1; i >= 0; --i) {
        QListWidgetItem* item = m_downloadList->item(i);
        QWidget* widget = m_downloadList->itemWidget(item);
        QProgressBar* progress = widget->findChild<QProgressBar*>();
        if (progress && progress->value() == progress->maximum() && progress->maximum() > 0) {
            m_itemProgressBars.remove(widget->property("name").toString());
            delete m_downloadList->takeItem(i);
        }
    }

    if (m_downloadList->count() == 0) {
        m_overallProgress->hide();
        m_statusLabel->setText("No active downloads");
    }
}

void DownloadManager::onDownloadProgress(const DownloadProgress& progress)
{
    const QString& name = progress.currentFile;
    const qint64 current = progress.bytesDownloaded;
    const qint64 total = progress.totalBytes;
    if (m_itemProgressBars.contains(name)) {
        QProgressBar* progress = m_itemProgressBars[name];
        progress->setMaximum(total);
        progress->setValue(current);

        QWidget* widget = progress->parentWidget();
        QLabel* status = widget->findChild<QLabel*>("status");
        if (status) {
            status->setText(QString("%1%").arg((current * 100) / qMax(total, qint64(1))));
        }
    }

    // Update overall progress
    qint64 totalAll = 0, currentAll = 0;
    for (auto it = m_itemProgressBars.constBegin(); it != m_itemProgressBars.constEnd(); ++it) {
        totalAll += it.value()->maximum();
        currentAll += it.value()->value();
    }
    if (totalAll > 0) {
        m_overallProgress->setMaximum(totalAll);
        m_overallProgress->setValue(currentAll);
    }
}

void DownloadManager::onDownloadComplete(const QString& name)
{
    if (m_itemProgressBars.contains(name)) {
        QProgressBar* progress = m_itemProgressBars[name];
        progress->setValue(progress->maximum());
        progress->setStyleSheet(R"(
            QProgressBar { background: #353534; border: none; border-radius: 2px; }
            QProgressBar::chunk { background: #00FF66; border-radius: 2px; }
        )");

        QWidget* widget = progress->parentWidget();
        QLabel* status = widget->findChild<QLabel*>("status");
        if (status) status->setText("Complete");
    }

    // Check if all complete
    bool allDone = true;
    for (auto it = m_itemProgressBars.constBegin(); it != m_itemProgressBars.constEnd(); ++it) {
        if (it.value()->value() < it.value()->maximum()) {
            allDone = false;
            break;
        }
    }
    if (allDone && !m_itemProgressBars.isEmpty()) {
        m_statusLabel->setText("All downloads complete!");
        emit allDownloadsComplete();
    }
}

void DownloadManager::onDownloadError(const QString& name, const QString& error)
{
    if (m_itemProgressBars.contains(name)) {
        QProgressBar* progress = m_itemProgressBars[name];
        progress->setStyleSheet(R"(
            QProgressBar { background: #353534; border: none; border-radius: 2px; }
            QProgressBar::chunk { background: #FF0033; border-radius: 2px; }
        )");

        QWidget* widget = progress->parentWidget();
        QLabel* status = widget->findChild<QLabel*>("status");
        if (status) {
            status->setText("Failed: " + error);
            status->setStyleSheet("color: #FF0033; font-family: 'JetBrains Mono'; font-size: 11px;");
        }
    }
    m_statusLabel->setText("Download failed: " + error);
}