#include "ui/pages/Pages.h"
#include "launcher/LauncherCore.h"

ModsPage::ModsPage(LauncherCore*, QWidget* p) : QWidget(p) {
    QVBoxLayout* l = new QVBoxLayout(this);
    QLabel* label = new QLabel("Mods - Mod Management");
    label->setStyleSheet("color: #e5e2e1; font-size: 24px; font-weight: bold;");
    l->addWidget(label);
    l->addStretch();
}

ServersPage::ServersPage(LauncherCore*, QWidget* p) : QWidget(p) {
    QVBoxLayout* l = new QVBoxLayout(this);
    QLabel* label = new QLabel("Servers - Server List & Management");
    label->setStyleSheet("color: #e5e2e1; font-size: 24px; font-weight: bold;");
    l->addWidget(label);
    l->addStretch();
}

WorldsPage::WorldsPage(LauncherCore*, QWidget* p) : QWidget(p) {
    QVBoxLayout* l = new QVBoxLayout(this);
    QLabel* label = new QLabel("Worlds - World Management & Backups");
    label->setStyleSheet("color: #e5e2e1; font-size: 24px; font-weight: bold;");
    l->addWidget(label);
    l->addStretch();
}

CosmeticsPage::CosmeticsPage(LauncherCore*, QWidget* p) : QWidget(p) {
    QVBoxLayout* l = new QVBoxLayout(this);
    QLabel* label = new QLabel("Cosmetics - Skins, Capes & Customization");
    label->setStyleSheet("color: #e5e2e1; font-size: 24px; font-weight: bold;");
    l->addWidget(label);
    l->addStretch();
}

NewsPage::NewsPage(LauncherCore*, QWidget* p) : QWidget(p) {
    QVBoxLayout* l = new QVBoxLayout(this);
    QLabel* label = new QLabel("News - Updates & Announcements");
    label->setStyleSheet("color: #e5e2e1; font-size: 24px; font-weight: bold;");
    l->addWidget(label);
    l->addStretch();
}