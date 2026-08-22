#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QProgressBar>
#include <QHeaderView>
#include <QStackedWidget>
#include <QButtonGroup>
#include <QFrame>
#include <QGridLayout>
#include <QGraphicsDropShadowEffect>
#include <QGroupBox>
#include "launcher/LauncherCore.h"

class LauncherCore;

class VersionsPage : public QWidget
{
    Q_OBJECT

public:
    explicit VersionsPage(LauncherCore* launcherCore, QWidget* parent = nullptr);

private slots:
    void onVersionsFetched(const QList<VersionEntry>& versions);
    void onDownloadProgress(const DownloadProgress& progress);
    void onDownloadComplete(const QString& versionId);
    void onDownloadError(const QString& error);
    void onFilterChanged();
    void onSearchChanged(const QString& text);
    void onDownloadClicked();
    void onTabChanged(int index);

private:
    void setupUI();
    void populateVersionGrid(const QList<VersionEntry>& versions);
    void populateInstanceGrid();
    QFrame* createVersionCard(const VersionEntry& version);
    void filterVersions();
    void showVersionsEmptyState();
    void showInstancesEmptyState();

    LauncherCore* m_launcherCore;

    // UI elements
    QLineEdit* m_searchEdit = nullptr;
    QComboBox* m_typeFilter = nullptr;
    QStackedWidget* m_contentStack = nullptr;
    QWidget* m_instancesWidget = nullptr;
    QWidget* m_versionsWidget = nullptr;
    QWidget* m_versionsGrid = nullptr;
    QGridLayout* m_versionsGridLayout = nullptr;
    QVBoxLayout* m_versionsLayout = nullptr;
    QVBoxLayout* m_instancesLayout = nullptr;
    QWidget* m_instancesGrid = nullptr;
    QGridLayout* m_instancesGridLayout = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QLabel* m_statusLabel = nullptr;
    QPushButton* m_refreshButton = nullptr;
    QButtonGroup* m_tabGroup = nullptr;

    QList<VersionEntry> m_allVersions;
    QString m_currentFilter = "All";
    QString m_currentSearch;
    int m_currentTab = 0; // 0 = Instances, 1 = Versions
};