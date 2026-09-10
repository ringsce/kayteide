#include "Gtk4NewProjectDialog.h"

#ifdef KAYTE_GTK4_ENABLED

#include <gtk/gtk.h>

namespace {

// Kept in sync by hand with newprojectdialog.ui's projectTypeComboBox items
// — the two dialogs are built for different toolkits and don't share a
// translation unit.
constexpr const char *kProjectTypes[] = {
    "C++ Console Application",
    "Pascal Console Application",
    "Qt Widgets Application",
};

struct DialogState {
    GtkWidget *window = nullptr;
    GtkWidget *nameEntry = nullptr;
    GtkWidget *typeDropdown = nullptr;
    Gtk4NewProjectResult result;
};

// Reads the entry/dropdown into result, marks accepted, and closes the
// window — shared by the OK button and Enter-in-the-name-entry.
void accept(DialogState *state)
{
    state->result.accepted = true;
    state->result.projectName = gtk_editable_get_text(GTK_EDITABLE(state->nameEntry));
    state->result.projectTypeIndex =
        static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(state->typeDropdown)));
    gtk_window_destroy(GTK_WINDOW(state->window));
}

void onOkClicked(GtkButton *, gpointer user_data)
{
    accept(static_cast<DialogState *>(user_data));
}

void onCancelClicked(GtkButton *, gpointer user_data)
{
    auto *state = static_cast<DialogState *>(user_data);
    gtk_window_destroy(GTK_WINDOW(state->window));
}

void onNameEntryActivate(GtkEntry *, gpointer user_data)
{
    accept(static_cast<DialogState *>(user_data));
}

void activate(GtkApplication *app, gpointer user_data)
{
    auto *state = static_cast<DialogState *>(user_data);

    state->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(state->window), "New Project");
    gtk_window_set_default_size(GTK_WINDOW(state->window), 400, 300);
    gtk_window_set_modal(GTK_WINDOW(state->window), TRUE);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(box, 12);
    gtk_widget_set_margin_end(box, 12);
    gtk_widget_set_margin_top(box, 12);
    gtk_widget_set_margin_bottom(box, 12);
    gtk_window_set_child(GTK_WINDOW(state->window), box);

    GtkWidget *nameLabel = gtk_label_new("Project Name:");
    gtk_widget_set_halign(nameLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), nameLabel);

    state->nameEntry = gtk_entry_new();
    g_signal_connect(state->nameEntry, "activate", G_CALLBACK(onNameEntryActivate), state);
    gtk_box_append(GTK_BOX(box), state->nameEntry);

    GtkWidget *typeLabel = gtk_label_new("Project Type:");
    gtk_widget_set_halign(typeLabel, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), typeLabel);

    GtkStringList *types = gtk_string_list_new(nullptr);
    for (const char *type : kProjectTypes)
        gtk_string_list_append(types, type);
    state->typeDropdown = gtk_drop_down_new(G_LIST_MODEL(types), nullptr);
    gtk_drop_down_set_selected(GTK_DROP_DOWN(state->typeDropdown), 0);
    gtk_box_append(GTK_BOX(box), state->typeDropdown);

    // Spacer so the buttons sit at the bottom, same layout as
    // newprojectdialog.ui's verticalSpacer + buttonBox.
    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_vexpand(spacer, TRUE);
    gtk_box_append(GTK_BOX(box), spacer);

    GtkWidget *buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(buttonBox, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(box), buttonBox);

    GtkWidget *cancelButton = gtk_button_new_with_label("Cancel");
    g_signal_connect(cancelButton, "clicked", G_CALLBACK(onCancelClicked), state);
    gtk_box_append(GTK_BOX(buttonBox), cancelButton);

    GtkWidget *okButton = gtk_button_new_with_label("OK");
    g_signal_connect(okButton, "clicked", G_CALLBACK(onOkClicked), state);
    gtk_box_append(GTK_BOX(buttonBox), okButton);

    // Quit the application's main loop once the window is gone, so
    // g_application_run() below returns.
    g_signal_connect_swapped(state->window, "destroy", G_CALLBACK(g_application_quit), app);

    gtk_window_present(GTK_WINDOW(state->window));
}

} // namespace

Gtk4NewProjectResult runGtk4NewProjectDialog()
{
    DialogState state;

    GtkApplication *app = gtk_application_new(
        "com.kayteide.newprojectdialog", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), &state);

    g_application_run(G_APPLICATION(app), 0, nullptr);
    g_object_unref(app);

    return state.result;
}

#else // !KAYTE_GTK4_ENABLED

Gtk4NewProjectResult runGtk4NewProjectDialog()
{
    return {};
}

#endif
