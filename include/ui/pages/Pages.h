#ifndef PAGES_H
#define PAGES_H

#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>

class LauncherCore;

class ModsPage : public QWidget {
    Q_OBJECT
public:
    explicit ModsPage(LauncherCore*, QWidget* parent = nullptr);
};

class ServersPage : public QWidget {
    Q_OBJECT
public:
    explicit ServersPage(LauncherCore*, QWidget* parent = nullptr);
};

class WorldsPage : public QWidget {
    Q_OBJECT
public:
    explicit WorldsPage(LauncherCore*, QWidget* parent = nullptr);
};

class CosmeticsPage : public QWidget {
    Q_OBJECT
public:
    explicit CosmeticsPage(LauncherCore*, QWidget* parent = nullptr);
};

class NewsPage : public QWidget {
    Q_OBJECT
public:
    explicit NewsPage(LauncherCore*, QWidget* parent = nullptr);
};

#endif // PAGES_H
