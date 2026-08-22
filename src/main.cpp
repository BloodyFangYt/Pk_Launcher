#include <QApplication>
#include <QStyleFactory>
#include <QFontDatabase>
#include <QDir>
#include <QStandardPaths>
#include <QMessageBox>
#include <QSplashScreen>
#include <QPixmap>
#include <QTimer>

#include "core/Application.h"
#include "core/Settings.h"
#include "ui/MainWindow.h"
#include "launcher/LauncherCore.h"

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);

    QApplication app(argc, argv);
    app.setApplicationName("PkLauncher");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("PKLauncher");
    app.setOrganizationDomain("pklauncher.dev");

    // Set style
    app.setStyle(QStyleFactory::create("Fusion"));

    // Load custom font
    int fontId = QFontDatabase::addApplicationFont(":/fonts/JetBrainsMono-Regular.ttf");
    if (fontId != -1) {
        QString family = QFontDatabase::applicationFontFamilies(fontId).at(0);
        QFont font(family, 10);
        app.setFont(font);
    }

    // Initialize settings
    Settings::instance().load();

    // Create and initialize application core
    Application launcherApp;
    if (!launcherApp.initialize()) {
        QMessageBox::critical(nullptr, "Initialization Error",
                              "Failed to initialize application.\n"
                              "Check logs for details.");
        return 1;
    }

    // Show splash screen
    QSplashScreen splash(QPixmap(":/images/applogo.png"));
    splash.show();
    app.processEvents();

    // Create and show main window
    MainWindow mainWindow(launcherApp.launcherCore(), launcherApp.instanceManager(), launcherApp.authManager());
    mainWindow.show();

    splash.finish(&mainWindow);

    return app.exec();
}