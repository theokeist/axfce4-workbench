/*
 * Copyright (C) 2026 XFCE4 MorphOS Workbench Plugin Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "morphos-workbench.h"

/* ------------------------------------------------------------------ *
 *  Menu item helper (icon + label, Ambient style)
 *  ------------------------------------------------------------------ */

static GtkWidget *
mwb_create_menu_item(const gchar *icon_name, const gchar *label_text)
{
    GtkWidget *item = gtk_menu_item_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(item), "mwb-appmenu-item");

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    if (icon_name) {
        GtkWidget *img = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_MENU);
        gtk_image_set_pixel_size(GTK_IMAGE(img), 16);
        gtk_box_pack_start(GTK_BOX(box), img, FALSE, FALSE, 0);
    }
    GtkWidget *lbl = gtk_label_new(label_text);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
    gtk_box_pack_start(GTK_BOX(box), lbl, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(item), box);
    gtk_widget_show_all(item);
    return item;
}

/* ------------------------------------------------------------------ *
 *  Menu title button
 *  ------------------------------------------------------------------ */

GtkWidget *
mwb_create_menu_title(MorphosWorkbenchPlugin *mwb G_GNUC_UNUSED, const gchar *text)
{
    GtkWidget *button = gtk_button_new_with_label(text);
    gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
    gtk_style_context_add_class(gtk_widget_get_style_context(button), "mwb-button");
    gtk_widget_set_can_focus(button, FALSE);
    return button;
}

/* ------------------------------------------------------------------ *
 *  Menu toggle (click)
 *  ------------------------------------------------------------------ */

static void
mwb_close_menus(MorphosWorkbenchPlugin *mwb)
{
    if (mwb->active_button) {
        gtk_style_context_remove_class(gtk_widget_get_style_context(mwb->active_button), "open");
        mwb->active_button = NULL;
    }
    gint i;
    for (i = 0; i < MWB_MENU_COUNT; i++) {
        if (mwb->menus[i])
            gtk_widget_hide(mwb->menus[i]);
    }
}

void
mwb_menu_toggle(GtkButton *button, MorphosWorkbenchPlugin *mwb)
{
    GtkWidget *menu = NULL;
    gint i;
    for (i = 0; i < MWB_MENU_COUNT; i++) {
        if (mwb->menu_buttons[i] == GTK_WIDGET(button)) {
            menu = mwb->menus[i];
            break;
        }
    }
    if (!menu)
        return;

    if (mwb->active_button == GTK_WIDGET(button) && gtk_widget_get_visible(menu)) {
        gtk_widget_hide(menu);
        mwb_close_menus(mwb);
        return;
    }

    mwb_close_menus(mwb);

    /* refresh the recently-used section before showing the Applications menu */
    if (i == MWB_MENU_APPLICATIONS && mwb->recent_apps != NULL) {
        mwb_rebuild_applications_menu(mwb);
        menu = mwb->menus[i];
    }

    gtk_menu_popup_at_widget(GTK_MENU(menu), GTK_WIDGET(button),
                             GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, NULL);
    mwb->active_button = GTK_WIDGET(button);
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->active_button), "open");
}

/* ------------------------------------------------------------------ *
 *  Menu builders
 *  ------------------------------------------------------------------ */

static GtkWidget *
mwb_build_ambient_menu(MorphosWorkbenchPlugin *mwb G_GNUC_UNUSED)
{
    GtkWidget *menu = gtk_menu_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(menu), "mwb-menu");

    GtkWidget *item;

    item = mwb_create_menu_item("computer", _("About This Computer"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch), "xfce4-about");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("preferences-system", _("Ambient Preferences…"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch), "xfce4-settings-manager");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("system-lock-screen", _("Lock Screen"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xflock4");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("system-log-out", _("Log Out…"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfce4-session-logout --logout");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("system-reboot", _("Restart…"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfce4-session-logout --reboot");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("system-shutdown", _("Shut Down…"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfce4-session-logout --halt");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    gtk_widget_show_all(menu);
    return menu;
}

static GtkWidget *
mwb_build_workbench_menu(MorphosWorkbenchPlugin *mwb G_GNUC_UNUSED)
{
    GtkWidget *menu = gtk_menu_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(menu), "mwb-menu");

    GtkWidget *item;

    item = mwb_create_menu_item("user-home", _("Open Home"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch), "exo-open --launch FileManager");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("folder-documents", _("Open Documents"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "exo-open ~/Dokumenty");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("utilities-terminal", _("Open Terminal"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "exo-open --launch TerminalEmulator");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("edit-find", _("Search Applications…"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfce4-appfinder");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("user-trash", _("Empty Trash"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "gio trash --empty");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("view-refresh", _("Redraw / Refresh"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfdesktop --reload");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    gtk_widget_show_all(menu);
    return menu;
}

static GtkWidget *
mwb_build_ambient_menu_menu(MorphosWorkbenchPlugin *mwb G_GNUC_UNUSED)
{
    GtkWidget *menu = gtk_menu_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(menu), "mwb-menu");

    GtkWidget *item;

    item = mwb_create_menu_item("computer", _("System Info"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "hardinfo || xfce4-about");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("utilities-system-monitor", _("Task Manager"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfce4-taskmanager");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("utilities-terminal", _("Terminal"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "exo-open --launch TerminalEmulator");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("system-run", _("Run…"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfce4-appfinder");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("system-file-manager", _("File Manager"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "exo-open --launch FileManager");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("applications-internet", _("Web Browser"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "exo-open --launch WebBrowser");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("preferences-system", _("Settings Manager"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfce4-settings-manager");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("video-display", _("Screen Settings"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfce4-display-settings");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("input-keyboard", _("Keyboard"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfce4-keyboard-settings");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("view-fullscreen", _("Open Application Menu"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfce4-popup-whiskermenu");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    gtk_widget_show_all(menu);
    return menu;
}

static GtkWidget *
mwb_build_icons_menu(MorphosWorkbenchPlugin *mwb G_GNUC_UNUSED)
{
    GtkWidget *menu = gtk_menu_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(menu), "mwb-menu");

    GtkWidget *item;

    item = mwb_create_menu_item("folder-new", _("New Folder"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_desktop_new_folder), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("view-grid", _("Clean Up Icons"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfdesktop --arrange");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("edit-select-all", _("Open Desktop Menu"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfdesktop --menu");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("preferences-system-windows", _("Window List"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfdesktop --windowlist");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("image-x-generic", _("Next Wallpaper"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfdesktop --next");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("view-refresh", _("Reload Desktop"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfdesktop --reload");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("preferences-desktop", _("Desktop Settings…"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfdesktop-settings");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    gtk_widget_show_all(menu);
    return menu;
}

static GtkWidget *
mwb_build_disk_menu(MorphosWorkbenchPlugin *mwb G_GNUC_UNUSED)
{
    GtkWidget *menu = gtk_menu_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(menu), "mwb-menu");

    GtkWidget *item;

    item = mwb_create_menu_item("system-file-manager", _("Open File Manager"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "exo-open --launch FileManager");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("drive-removable-media", _("Open Removable Media"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "exo-open /media");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("media-eject", _("Eject / Unmount…"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "udisksctl unmount -b /dev/sda1 || true");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("media-optical", _("Open Disk Utility"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "gparted");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    gtk_widget_show_all(menu);
    return menu;
}

#define MWB_RECENT_MAX 3
#define MWB_RECENT_CYCLES 3

typedef struct {
    MorphosWorkbenchPlugin *mwb;
    gchar *icon;
    gchar *label;
    gchar *cmd;
} MwbAppLaunch;

static void
mwb_recent_app_free(MwbRecentApp *app)
{
    if (!app)
        return;
    g_free(app->icon);
    g_free(app->label);
    g_free(app->cmd);
    g_free(app);
}

static void
mwb_app_launch_free(gpointer data, GClosure *closure G_GNUC_UNUSED)
{
    MwbAppLaunch *l = data;
    if (!l)
        return;
    g_free(l->icon);
    g_free(l->label);
    g_free(l->cmd);
    g_free(l);
}

static void
mwb_record_recent_app(MorphosWorkbenchPlugin *mwb, const gchar *icon,
                      const gchar *label, const gchar *cmd)
{
    GList *l;
    MwbRecentApp *app;

    /* drop an existing entry with the same command, then re-prepend it */
    for (l = mwb->recent_apps; l; l = l->next) {
        app = l->data;
        if (g_strcmp0(app->cmd, cmd) == 0) {
            mwb->recent_apps = g_list_delete_link(mwb->recent_apps, l);
            mwb_recent_app_free(app);
            break;
        }
    }

    app = g_new0(MwbRecentApp, 1);
    app->icon = g_strdup(icon);
    app->label = g_strdup(label);
    app->cmd = g_strdup(cmd);
    app->cycles = MWB_RECENT_CYCLES;
    mwb->recent_apps = g_list_prepend(mwb->recent_apps, app);

    while (g_list_length(mwb->recent_apps) > MWB_RECENT_MAX) {
        GList *last = g_list_last(mwb->recent_apps);
        app = last->data;
        mwb->recent_apps = g_list_delete_link(mwb->recent_apps, last);
        mwb_recent_app_free(app);
    }
}

static void
mwb_app_activate(MwbAppLaunch *l, GtkWidget *item G_GNUC_UNUSED)
{
    if (!l)
        return;
    mwb_record_recent_app(l->mwb, l->icon, l->label, l->cmd);
    mwb_launch(l->cmd);
}

static GtkWidget *
mwb_create_app_item(MorphosWorkbenchPlugin *mwb, const gchar *icon,
                    const gchar *label, const gchar *cmd)
{
    GtkWidget *item = mwb_create_menu_item(icon, label);
    MwbAppLaunch *l = g_new0(MwbAppLaunch, 1);
    l->mwb = mwb;
    l->icon = g_strdup(icon);
    l->label = g_strdup(label);
    l->cmd = g_strdup(cmd);
    g_signal_connect_data(item, "activate", G_CALLBACK(mwb_app_activate),
                          l, (GClosureNotify)mwb_app_launch_free, G_CONNECT_SWAPPED);
    return item;
}

static GtkWidget *
mwb_build_applications_menu(MorphosWorkbenchPlugin *mwb)
{
    GtkWidget *menu = gtk_menu_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(menu), "mwb-menu");

    GtkWidget *item;

    /* recently used apps on top, separated by a divider */
    if (mwb->recent_apps) {
        GList *l;
        for (l = mwb->recent_apps; l; l = l->next) {
            MwbRecentApp *app = l->data;
            item = mwb_create_app_item(mwb, app->icon, app->label, app->cmd);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        }
        item = gtk_separator_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    }

    struct { const gchar *icon; const gchar *label; const gchar *cmd; } apps[] = {
        { "utilities-terminal",   N_("Terminal"),        "exo-open --launch TerminalEmulator" },
        { "system-file-manager",  N_("File Manager"),     "exo-open --launch FileManager" },
        { "applications-internet", N_("Web Browser"),     "exo-open --launch WebBrowser" },
        { "applications-multimedia", N_("Multimedia"),    "exo-open --launch Multimedia" },
        { "applications-graphics", N_("Graphics"),        "exo-open --launch Graphics" },
        { "applications-office",  N_("Office"),           "exo-open --launch Office" },
        { "applications-science", N_("Education"),        "exo-open --launch Education" },
        { "applications-system",  N_("System Tools"),     "exo-open --launch SystemTools" },
    };
    guint i;
    for (i = 0; i < G_N_ELEMENTS(apps); i++) {
        item = mwb_create_app_item(mwb, apps[i].icon, _(apps[i].label), apps[i].cmd);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    }

    gtk_widget_show_all(menu);
    return menu;
}

void
mwb_rebuild_applications_menu(MorphosWorkbenchPlugin *mwb)
{
    GList *l = mwb->recent_apps;
    while (l) {
        MwbRecentApp *app = l->data;
        GList *next = l->next;
        if (--app->cycles <= 0) {
            mwb->recent_apps = g_list_delete_link(mwb->recent_apps, l);
            mwb_recent_app_free(app);
        }
        l = next;
    }

    if (mwb->menus[MWB_MENU_APPLICATIONS])
        gtk_widget_destroy(mwb->menus[MWB_MENU_APPLICATIONS]);

    mwb->menus[MWB_MENU_APPLICATIONS] = mwb_build_applications_menu(mwb);
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->menus[MWB_MENU_APPLICATIONS]), "mwb-menu-applications");
    mwb_theme_widget(mwb, mwb->menus[MWB_MENU_APPLICATIONS]);
}

void
mwb_recent_clear(MorphosWorkbenchPlugin *mwb)
{
    g_list_free_full(mwb->recent_apps, (GDestroyNotify)mwb_recent_app_free);
    mwb->recent_apps = NULL;
}

void
mwb_create_menus(MorphosWorkbenchPlugin *mwb)
{
    mwb->menus[MWB_MENU_AMBIENT] = mwb_build_ambient_menu(mwb);
    mwb->menus[MWB_MENU_WORKBENCH] = mwb_build_workbench_menu(mwb);
    mwb->menus[MWB_MENU_AMBIENT_MENU] = mwb_build_ambient_menu_menu(mwb);
    mwb->menus[MWB_MENU_ICONS] = mwb_build_icons_menu(mwb);
    mwb->menus[MWB_MENU_DISK] = mwb_build_disk_menu(mwb);
    mwb->menus[MWB_MENU_APPLICATIONS] = mwb_build_applications_menu(mwb);
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->menus[MWB_MENU_WORKBENCH]), "mwb-menu-workbench");
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->menus[MWB_MENU_AMBIENT_MENU]), "mwb-menu-ambient");
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->menus[MWB_MENU_ICONS]), "mwb-menu-icons");
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->menus[MWB_MENU_DISK]), "mwb-menu-disk");
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->menus[MWB_MENU_APPLICATIONS]), "mwb-menu-applications");
}
