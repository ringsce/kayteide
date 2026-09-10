#pragma once
// Gtk4NewProjectDialog.h – plugins/project
//
// GTK4 counterpart to NewProjectDialog (Qt6): collects a project name and
// project type and reports whether the user accepted or cancelled. Mirrors
// libreparser's Gtk4Editor.h split — built only when KAYTE_GTK4_ENABLED is
// defined (see plugins/project/CMakeLists.txt); on other builds
// runGtk4NewProjectDialog() is a stub that reports "not accepted" without
// opening any window.
//
// Deliberately Qt-free: this header (and its .cpp) must never be included
// together with a Qt header in the same translation unit. Qt's meta-object
// macros redefine "signals" to Q_SIGNALS, which collides with GLib headers
// that use "signals" as a plain struct member name.

#include <string>

struct Gtk4NewProjectResult {
    bool accepted = false;
    std::string projectName;
    // Index into the same list as NewProjectDialog's projectTypeComboBox
    // (newprojectdialog.ui): 0 = C++ Console Application,
    // 1 = Pascal Console Application, 2 = Qt Widgets Application.
    // -1 when the dialog was cancelled.
    int projectTypeIndex = -1;
};

// Shows the GTK4 "New Project" dialog and runs its own event loop until the
// user accepts or cancels. Returns the user's choices, or accepted=false
// with an empty result if GTK4 support isn't built in.
Gtk4NewProjectResult runGtk4NewProjectDialog();
