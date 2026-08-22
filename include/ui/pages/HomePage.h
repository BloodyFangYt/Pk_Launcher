#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QScrollArea>
#include <QLinearGradient>
#include <QPainter>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

class LauncherCore;

class HomePage : public QWidget
{
    Q_OBJECT

public:
    explicit HomePage(LauncherCore* launcherCore, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void setupUI();
    void createHeroBackground();
    void updateAnimations();

    LauncherCore* m_launcherCore;
    QLabel* m_statusLabel;
    QPushButton* m_playButton;
    QWidget* m_heroWidget = nullptr;
    QWidget* m_playCard = nullptr;
    QPropertyAnimation* m_glowAnimation = nullptr;
    QGraphicsOpacityEffect* m_glowEffect = nullptr;
    QPixmap m_heroBackground;
    bool m_hasHeroBackground = false;
};