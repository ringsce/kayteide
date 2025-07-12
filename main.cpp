#include <QApplication> // Required for all Qt GUI applications
#include "mainwindow.h"   // Include the header for your MainWindow class
#include <QFile>
#include <QTextStream>
#include <QDebug> // For qDebug(), equivalent to std::cout

void readLinesWithQt(const QString& filename) {
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Error: Could not open file" << filename << ":" << file.errorString();
        return;
    }

    QTextStream in(&file);
    QString line;
    int lineNumber = 0;

    while (!in.atEnd()) { // Check if we are at the end of the stream
        line = in.readLine(); // Read a line
        lineNumber++;
        qDebug() << "Line" << lineNumber << ":" << line;
    }

    file.close(); // Close the file
}

int main(int argc, char *argv[])
{
    // 1. Create a QApplication object.
    // This object manages the GUI application's control flow and main settings.
    // It also handles command-line arguments specific to Qt.
    QApplication a(argc, argv);

    // 2. Create an instance of your MainWindow.
    // This will load the UI defined in mainwindow.ui via ui_mainwindow.h.
    MainWindow w;

    // 3. Show the main window.
    // By default, widgets are created hidden. show() makes them visible.
    w.show();

    // 4. Enter the main event loop and hand over control to Qt.
    // This is where the application waits for user input and system events.
    // The application exits when exec() receives a signal to quit.
    return a.exec();
}