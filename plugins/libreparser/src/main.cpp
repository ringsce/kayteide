#include <QApplication>

#include <cstring>
#include <iostream>

#include "Gtk4Editor.h"
#include "src/XmlEditorWindow.h"

int main(int argc, char *argv[])
{
#ifdef KAYTE_GTK4_ENABLED
    // Explicit opt-in: `libreparser --gtk4 [file.xml]` runs the GTK4 UI
    // instead of the default Qt6 one below — same idea (create the window,
    // show it, open the given file if any) implemented with GTK4. Only
    // reachable on builds where GTK4 was actually found (KAYTE_GTK4_ENABLED
    // is set by CMake on Linux / macOS when pkg-config locates gtk4 — see
    // plugins/libreparser/CMakeLists.txt), so the flag simply doesn't exist
    // on builds without it.
    if (argc > 1 && std::strcmp(argv[1], "--gtk4") == 0) {
        std::cout << "Dynamic check: GTK4 environment detected!" << std::endl;
        return runGtk4XmlEditor(argc > 2 ? argv[2] : nullptr);
    }
#endif

    std::cout << "Dynamic check: Qt6 environment detected!" << std::endl;

    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("KayteIDE"));
    QCoreApplication::setApplicationName(QStringLiteral("libreparser"));

    XmlEditorWindow window;
    window.show();

    if (argc > 1)
        window.openFile(QString::fromLocal8Bit(argv[1]));

    return app.exec();
}
