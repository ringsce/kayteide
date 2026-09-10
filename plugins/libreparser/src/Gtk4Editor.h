#pragma once
// Gtk4Editor.h – libreparser
//
// Minimal GTK4 counterpart to XmlEditorWindow's bootstrap sequence in
// main.cpp (create window, show it, open the given file if any, run the
// event loop). Not a full port of XmlEditorWindow — no tree/diagnostics
// panes — just enough to mirror what the Qt6 branch does when built with
// KAYTE_GTK4_ENABLED (see plugins/libreparser/CMakeLists.txt).
//
// Deliberately Qt-free: this header (and its .cpp) must never be included
// together with a Qt header in the same translation unit. Qt's meta-object
// macros redefine "signals" to Q_SIGNALS, which collides with GLib headers
// that use "signals" as a plain struct member name.

// Shows the GTK4 editor window and runs its event loop.
// filePath may be null, in which case the same built-in template used by
// XmlEditorWindow's default document is shown instead.
// Returns the process exit status.
int runGtk4XmlEditor(const char *filePath);
