#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QTextEdit>
#include <QGroupBox>

class LauncherCore;
class InstanceManager;

class PlayPage : public QWidget
{
    Q_OBJECT

public:
    explicit PlayPage(LauncherCore* launcherCore, InstanceManager* instanceManager, QWidget* parent = nullptr);

private slots:
    void onInstanceListChanged();
    void onLaunchClicked();
    void onLaunchStarted();
    void onLaunchFinished(bool success, const QString& error);
    void onLogMessage(const QString& message);

private:
    void refreshInstanceList();
    void setupUI();

    LauncherCore* m_launcherCore;
    InstanceManager* m_instanceManager;

    // UI elements
    QComboBox* m_instanceCombo = nullptr;
    QLabel* m_versionLabel = nullptr;
    QPushButton* m_launchButton = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QTextEdit* m_logOutput = nullptr;
    QGroupBox* m_instanceGroup = nullptr;
};