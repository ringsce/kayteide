#include <QApplication>
#include <QCommandLineParser>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("KayteIDEUpdater");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("ringsce");
    app.setQuitOnLastWindowClosed(false); // keep alive in system tray

    QCommandLineParser parser;
    parser.setApplicationDescription("Auto-updater and build tool for KayteIDE");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({{"m", "minimized"}, "Start minimized to system tray"});
    parser.process(app);

    MainWindow w;
    if (!parser.isSet("minimized"))
        w.show();

    return app.exec();
}
