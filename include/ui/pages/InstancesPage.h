#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QTextEdit>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QFrame>
#include <QGridLayout>
#include <QGraphicsDropShadowEffect>
#include "launcher/InstanceManager.h"

class LauncherCore;

class InstancesPage : public QWidget
{
    Q_OBJECT

public:
    explicit InstancesPage(LauncherCore* launcherCore, InstanceManager* instanceManager, QWidget* parent = nullptr);

private slots:
    void onInstanceListChanged();
    void onCreateInstance();
    void onEditInstance(const QString& instanceId);
    void onRefreshClicked();
    void onPlayInstance(const QString& instanceId);
    void onOpenInstanceFolder(const QString& instanceId);

private:
    void refreshInstanceGrid();
    QFrame* createInstanceCard(const InstanceInfo& instance);
    void setupUI();
    void showCreateDialog();
    void showEditDialog(const QString& instanceId);
    bool confirmDelete(const QString& name);

    LauncherCore* m_launcherCore;
    InstanceManager* m_instanceManager;

    // UI elements
    QWidget* m_gridWidget = nullptr;
    QGridLayout* m_gridLayout = nullptr;
    QPushButton* m_createButton = nullptr;
    QPushButton* m_refreshButton = nullptr;
    QLabel* m_instanceCountLabel = nullptr;
    QLabel* m_emptyStateLabel = nullptr;
};