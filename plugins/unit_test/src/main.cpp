#include <QApplication>
#include "TestRunnerWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Qt6 Test Runner");
    app.setApplicationVersion("1.0.0");

    TestRunnerWindow win;

    // Allow passing the test binary directly on the command line:
    //   ./TestRunnerApp ./myTests
    if (argc >= 2)
        win.setBinaryPath(QString::fromLocal8Bit(argv[1]));

    win.show();
    return app.exec();
}
