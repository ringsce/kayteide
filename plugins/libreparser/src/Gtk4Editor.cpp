#include "Gtk4Editor.h"

#ifdef KAYTE_GTK4_ENABLED

#include <gtk/gtk.h>

#include <string>

namespace {

// Same starter document XmlEditorWindow.cpp's kDefaultTemplate loads for a
// new, unnamed document — kept in sync by hand since the two are built for
// different toolkits and don't share a translation unit.
constexpr char kDefaultTemplate[] = R"XML(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE window SYSTEM "ui.dtd">
<window id="main_win">
    <title>New Window</title>
    <content>
        <text>Enter your message here.</text>
    </content>
    <actions>
        <button id="btn_ok" onclick="onOk()">OK</button>
    </actions>
</window>
)XML";

struct ActivateData {
    std::string filePath;
};

void activate(GtkApplication *app, gpointer user_data)
{
    auto *data = static_cast<ActivateData *>(user_data);

    std::string xml;
    std::string title;

    if (!data->filePath.empty()) {
        gchar *contents = nullptr;
        gsize length = 0;
        GError *error = nullptr;
        if (g_file_get_contents(data->filePath.c_str(), &contents, &length, &error)) {
            xml.assign(contents, length);
            g_free(contents);
            title = data->filePath + " — libreparser (GTK4)";
        } else {
            g_printerr("Cannot open %s: %s\n", data->filePath.c_str(),
                       error ? error->message : "unknown error");
            if (error)
                g_error_free(error);
            xml = kDefaultTemplate;
            title = "untitled.xml — libreparser (GTK4)";
        }
    } else {
        xml = kDefaultTemplate;
        title = "untitled.xml — libreparser (GTK4)";
    }

    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), title.c_str());
    gtk_window_set_default_size(GTK_WINDOW(window), 1100, 720);

    GtkWidget *scroller = gtk_scrolled_window_new();
    gtk_widget_set_hexpand(scroller, TRUE);
    gtk_widget_set_vexpand(scroller, TRUE);

    GtkWidget *textView = gtk_text_view_new();
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(textView), TRUE);

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));
    gtk_text_buffer_set_text(buffer, xml.c_str(), static_cast<gint>(xml.size()));

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroller), textView);
    gtk_window_set_child(GTK_WINDOW(window), scroller);

    gtk_window_present(GTK_WINDOW(window));
}

} // namespace

int runGtk4XmlEditor(const char *filePath)
{
    ActivateData data{filePath ? filePath : ""};

    GtkApplication *app = gtk_application_new(
        "com.kayteide.libreparser", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), &data);

    // Args are handled by libreparser itself (the leading "--gtk4" switch
    // and optional file path), not by GApplication, so run with an empty
    // command line.
    int status = g_application_run(G_APPLICATION(app), 0, nullptr);
    g_object_unref(app);
    return status;
}

#else // !KAYTE_GTK4_ENABLED

int runGtk4XmlEditor(const char *)
{
    return 1;
}

#endif
