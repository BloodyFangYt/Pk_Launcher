#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QSlider>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QFrame>

class LauncherCore;
class UpdateManager;

class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(LauncherCore* launcherCore, QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* event) override;

private:
    void setupUI();
    void loadSettings();
    void saveSettings();

    void onBrowseJava();
    void onDetectJava();
    void onAccentPick();
    void onCheckUpdates();

    void updateAccentButton(const QString& color);
    QStringList splitJvmArgs(const QString& text) const;

    QFrame* createCard(const QString& title);

    LauncherCore* m_launcherCore;
    UpdateManager* m_updateManager = nullptr;

    // Java section
    QLineEdit* m_javaPath = nullptr;
    QTextEdit* m_customJvmArgs = nullptr;
    QTextEdit* m_defaultJvmArgs = nullptr;

    // Launcher section
    QSlider* m_ramSlider = nullptr;
    QLabel* m_ramValue = nullptr;
    QCheckBox* m_closeOnLaunch = nullptr;
    QCheckBox* m_showConsole = nullptr;

    // Network section
    QComboBox* m_proxyType = nullptr;
    QLineEdit* m_proxyHost = nullptr;
    QSpinBox* m_proxyPort = nullptr;
    QSpinBox* m_timeout = nullptr;
    QSpinBox* m_maxConcurrent = nullptr;

    // Appearance section
    QComboBox* m_theme = nullptr;
    QPushButton* m_accentIndicator = nullptr;
    QCheckBox* m_animations = nullptr;
    QSlider* m_uiScale = nullptr;
    QLabel* m_uiScaleValue = nullptr;

    // About section
    QLabel* m_updateStatus = nullptr;

    bool m_loading = false;
};