#include "mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("waynaptics-config");
    app.setApplicationDisplayName("Waynaptics Configuration");

    QCommandLineParser parser;
    parser.setApplicationDescription("Configuration utility for waynaptics touchpad daemon");
    parser.addHelpOption();

    QCommandLineOption socketOption(
        QStringList() << "socket",
        "Unix socket path for waynaptics daemon.",
        "path",
        "/var/run/waynaptics.sock");
    parser.addOption(socketOption);

    QCommandLineOption ignoreCapsOption(
        QStringList() << "ignore-capabilities",
        "Ignore device capabilities (enable all controls for testing).");
    parser.addOption(ignoreCapsOption);

    parser.process(app);

    QString socketPath = parser.value(socketOption);
    bool ignoreCaps = parser.isSet(ignoreCapsOption);

    MainWindow w(socketPath, ignoreCaps);
    if (!w.isConnected()) {
        return 1;
    }
    w.show();

    return app.exec();
}
