#include <QApplication>

#include <cstring>
#include <iostream>

#include "Gtk4NewProjectDialog.h"
#include "newprojectdialog.h"

int main(int argc, char *argv[])
{
#ifdef KAYTE_GTK4_ENABLED
    // Explicit opt-in: `project_dialog --gtk4` runs the GTK4 "New Project"
    // dialog instead of the default Qt6 one below — same idea (collect a
    // project name and type, report accept/cancel) implemented with GTK4.
    // Only reachable on builds where GTK4 was actually found
    // (KAYTE_GTK4_ENABLED is set by CMake on Linux / macOS when pkg-config
    // locates gtk4 — see plugins/project/CMakeLists.txt), so the flag simply
    // doesn't exist on builds without it.
    if (argc > 1 && std::strcmp(argv[1], "--gtk4") == 0) {
        std::cout << "Dynamic check: GTK4 environment detected!" << std::endl;
        Gtk4NewProjectResult result = runGtk4NewProjectDialog();
        if (result.accepted) {
            std::cout << "Project accepted: " << result.projectName
                       << " (type " << result.projectTypeIndex << ")" << std::endl;
        } else {
            std::cout << "Project dialog cancelled." << std::endl;
        }
        return 0;
    }
#endif

    std::cout << "Dynamic check: Qt6 environment detected!" << std::endl;

    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("KayteIDE"));
    QCoreApplication::setApplicationName(QStringLiteral("project_dialog"));

    NewProjectDialog dialog;
    if (dialog.exec() == QDialog::Accepted) {
        std::cout << "Project dialog accepted (Qt6)." << std::endl;
    } else {
        std::cout << "Project dialog cancelled." << std::endl;
    }

    return 0;
}
