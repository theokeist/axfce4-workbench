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
#include "mwb-apps.h"

/* ------------------------------------------------------------------ *
 *  Menu item helper (icon + label, Ambient style)
 *  ------------------------------------------------------------------ */

static gint mwb_menu_icon_px = 24;

void
mwb_set_icon_size(gint size)
{
    static const gint px[MWB_ICON_COUNT] = { 16, 24, 32 };
    if (size < 0)
        size = 0;
    if (size >= MWB_ICON_COUNT)
        size = MWB_ICON_COUNT - 1;
    mwb_menu_icon_px = px[size];
}

static void
mwb_menu_toplevel_setup(GtkWidget *menu, gpointer user_data G_GNUC_UNUSED)
{
    GtkWidget *toplevel = gtk_widget_get_toplevel(menu);
    if (toplevel && GTK_IS_WINDOW(toplevel)) {
        GdkScreen *screen = gtk_widget_get_screen(toplevel);
        GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
        if (visual) {
            gtk_widget_set_visual(toplevel, visual);
            gtk_widget_set_app_paintable(toplevel, TRUE);
        }
    }
}

static GtkWidget *
mwb_menu_new(void)
{
    GtkWidget *menu = gtk_menu_new();
    GdkScreen *screen = gdk_screen_get_default();
    GdkVisual *visual = screen ? gdk_screen_get_rgba_visual(screen) : NULL;

    if (visual != NULL) {
        gtk_widget_set_visual(menu, visual);
        gtk_widget_set_app_paintable(menu, TRUE);
    }

    g_signal_connect(menu, "realize", G_CALLBACK(mwb_menu_toplevel_setup), NULL);
    g_signal_connect(menu, "show", G_CALLBACK(mwb_menu_toplevel_setup), NULL);

    GtkWidget *toplevel = gtk_widget_get_toplevel(menu);
    if (toplevel && GTK_IS_WINDOW(toplevel) && visual != NULL) {
        gtk_widget_set_visual(toplevel, visual);
        gtk_widget_set_app_paintable(toplevel, TRUE);
    }

    gtk_style_context_add_class(gtk_widget_get_style_context(menu), "mwb-menu");
    return menu;
}

static GtkWidget *
mwb_create_menu_item(const gchar *icon_name, const gchar *label_text)
{
    GtkWidget *item = gtk_menu_item_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(item), "mwb-appmenu-item");

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    if (icon_name) {
        GtkWidget *img = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_MENU);
        gtk_image_set_pixel_size(GTK_IMAGE(img), mwb_menu_icon_px);
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

    /* ensure toplevel window has RGBA visual and app-paintable */
    GtkWidget *toplevel = gtk_widget_get_toplevel(menu);
    if (toplevel && GTK_IS_WINDOW(toplevel)) {
        GdkScreen *screen = gtk_widget_get_screen(toplevel);
        GdkVisual *visual = screen ? gdk_screen_get_rgba_visual(screen) : NULL;
        if (visual && !gtk_widget_get_realized(toplevel))
            gtk_widget_set_visual(toplevel, visual);
        gtk_widget_set_app_paintable(toplevel, TRUE);
    }

    gtk_menu_popup_at_widget(GTK_MENU(menu), GTK_WIDGET(button),
                             GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, NULL);
    mwb->active_button = GTK_WIDGET(button);
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->active_button), "open");
}

/* ------------------------------------------------------------------ *
 *  Menu builders
 *  ------------------------------------------------------------------ */

static GtkWidget *mwb_create_app_item(MorphosWorkbenchPlugin *mwb,
                                      const gchar *icon,
                                      const gchar *label,
                                      const gchar *cmd);

static GtkWidget *
mwb_build_ambient_menu(MorphosWorkbenchPlugin *mwb)
{
    GtkWidget *menu = mwb_menu_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(menu), "mwb-menu");

    GtkWidget *item;

    item = mwb_create_app_item(mwb, "computer", _("About This Computer"), "xfce4-about");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_app_item(mwb, "preferences-system", _("Ambient Preferences…"),
                               "xfce4-settings-manager");
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
mwb_build_workbench_menu(MorphosWorkbenchPlugin *mwb)
{
    GtkWidget *menu = mwb_menu_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(menu), "mwb-menu");

    GtkWidget *item;

    item = mwb_create_app_item(mwb, "user-home", _("Open Home"),
                               "exo-open --launch FileManager");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_app_item(mwb, "folder-documents", _("Open Documents"),
                               "exo-open ~/Dokumenty");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_app_item(mwb, "utilities-terminal", _("Open Terminal"),
                               "exo-open --launch TerminalEmulator");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_app_item(mwb, "edit-find", _("Search Applications…"),
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
mwb_build_ambient_menu_menu(MorphosWorkbenchPlugin *mwb)
{
    GtkWidget *menu = mwb_menu_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(menu), "mwb-menu");

    GtkWidget *item;

    item = mwb_create_app_item(mwb, "computer", _("System Info"),
                               "hardinfo || xfce4-about");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_app_item(mwb, "utilities-system-monitor", _("Task Manager"),
                               "xfce4-taskmanager");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_app_item(mwb, "utilities-terminal", _("Terminal"),
                               "exo-open --launch TerminalEmulator");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_app_item(mwb, "system-run", _("Run…"),
                               "xfce4-appfinder");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_app_item(mwb, "system-file-manager", _("File Manager"),
                               "exo-open --launch FileManager");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_app_item(mwb, "applications-internet", _("Web Browser"),
                               "exo-open --launch WebBrowser");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_app_item(mwb, "preferences-system", _("Settings Manager"),
                               "xfce4-settings-manager");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_app_item(mwb, "video-display", _("Screen Settings"),
                               "xfce4-display-settings");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_app_item(mwb, "input-keyboard", _("Keyboard"),
                               "xfce4-keyboard-settings");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_app_item(mwb, "view-fullscreen", _("Open Application Menu"),
                               "xfce4-popup-whiskermenu");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    gtk_widget_show_all(menu);
    return menu;
}

static GtkWidget *
mwb_build_icons_menu(MorphosWorkbenchPlugin *mwb G_GNUC_UNUSED)
{
    GtkWidget *menu = mwb_menu_new();
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
mwb_build_disk_menu(MorphosWorkbenchPlugin *mwb)
{
    GtkWidget *menu = mwb_menu_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(menu), "mwb-menu");

    GtkWidget *item;

    item = mwb_create_app_item(mwb, "system-file-manager", _("Open File Manager"),
                               "exo-open --launch FileManager");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_app_item(mwb, "drive-removable-media", _("Open Removable Media"),
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

    item = mwb_create_app_item(mwb, "media-optical", _("Open Disk Utility"),
                               "gparted");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    gtk_widget_show_all(menu);
    return menu;
}

typedef struct {
    MorphosWorkbenchPlugin *mwb;
    gchar *icon;
    gchar *label;
    gchar *cmd;
} MwbAppLaunch;

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

typedef struct {
    MorphosWorkbenchPlugin *mwb;
    guint   source_id;
    gchar  *icon;
    gchar  *label;
    gchar  *cmd;
} MwbTrackedLaunch;

static void
mwb_tracked_launch_free(gpointer data)
{
    MwbTrackedLaunch *t = data;
    if (!t)
        return;
    g_free(t->icon);
    g_free(t->label);
    g_free(t->cmd);
    g_free(t);
}

static void
mwb_app_closed(GPid pid G_GNUC_UNUSED, gint status G_GNUC_UNUSED, gpointer data)
{
    MwbTrackedLaunch *t = data;
    if (!t)
        return;
    if (t->mwb) {
        mwb_recent_record(t->mwb, t->icon, t->label, t->cmd);
        t->mwb->tracked_launches = g_list_remove(t->mwb->tracked_launches, t);
    }
    mwb_tracked_launch_free(t);
}

static void
mwb_app_activate(MwbAppLaunch *l, GtkWidget *item G_GNUC_UNUSED)
{
    MwbTrackedLaunch *t;

    if (!l)
        return;

    t = g_new0(MwbTrackedLaunch, 1);
    t->mwb = l->mwb;
    t->icon = g_strdup(l->icon);
    t->label = g_strdup(l->label);
    t->cmd = g_strdup(l->cmd);
    t->source_id = mwb_launch_tracked(l->cmd, mwb_app_closed, t);
    if (t->source_id == 0) {
        mwb_tracked_launch_free(t);
        return;
    }
    l->mwb->tracked_launches = g_list_prepend(l->mwb->tracked_launches, t);
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
    GtkWidget *menu = mwb_menu_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(menu), "mwb-menu");

    GtkWidget *item;
    gboolean has_items = FALSE;

    /* recently closed apps on top, separated by a divider */
    if (mwb->recent_apps && mwb->show_recent_apps) {
        GList *l;
        for (l = mwb->recent_apps; l; l = l->next) {
            MwbRecentApp *app = l->data;
            item = mwb_create_app_item(mwb, app->icon, app->label, app->cmd);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
            has_items = TRUE;
        }
        if (mwb->show_fav_apps || mwb->show_all_apps) {
            item = gtk_separator_menu_item_new();
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        }
    }

    /* hardcoded favorite applications */
    if (mwb->show_fav_apps) {
        struct { const gchar *icon; const gchar *label; const gchar *cmd; } apps[] = {
            { "utilities-terminal",      N_("Terminal"),        "exo-open --launch TerminalEmulator" },
            { "system-file-manager",     N_("File Manager"),    "exo-open --launch FileManager" },
            { "applications-internet",   N_("Web Browser"),     "exo-open --launch WebBrowser" },
            { "applications-multimedia", N_("Multimedia"),      "exo-open --launch Multimedia" },
            { "applications-graphics",   N_("Graphics"),        "exo-open --launch Graphics" },
            { "applications-office",     N_("Office"),          "exo-open --launch Office" },
            { "applications-science",    N_("Education"),       "exo-open --launch Education" },
            { "applications-system",     N_("System Tools"),    "exo-open --launch SystemTools" },
        };
        guint i;
        for (i = 0; i < G_N_ELEMENTS(apps); i++) {
            item = mwb_create_app_item(mwb, apps[i].icon, _(apps[i].label), apps[i].cmd);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
            has_items = TRUE;
        }
        if (mwb->show_all_apps) {
            item = gtk_separator_menu_item_new();
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        }
    }

    /* all installed applications, grouped into category submenus */
    if (mwb->show_all_apps) {
        if (mwb->installed_apps == NULL)
            mwb->installed_apps = mwb_apps_scan();

        if (mwb->installed_apps) {
            GtkWidget *submenu = NULL;
            GtkWidget *sub_item = NULL;
            guint cur_cat = G_MAXUINT;
            GList *l;

            for (l = mwb->installed_apps; l; l = l->next) {
                MwbDesktopApp *app = l->data;

                if (app->category != cur_cat) {
                    cur_cat = app->category;
                    sub_item = gtk_menu_item_new_with_label(_(mwb_app_category_label(cur_cat)));
                    submenu = mwb_menu_new();
                    gtk_style_context_add_class(gtk_widget_get_style_context(submenu), "mwb-menu");
                    gtk_menu_item_set_submenu(GTK_MENU_ITEM(sub_item), submenu);
                    gtk_menu_shell_append(GTK_MENU_SHELL(menu), sub_item);
                    has_items = TRUE;
                }

                item = mwb_create_app_item(mwb, app->icon, app->name, app->exec);
                gtk_menu_shell_append(GTK_MENU_SHELL(submenu), item);
            }
        }
    }

    if (!has_items) {
        item = gtk_menu_item_new_with_label(_("(No application sources selected in Settings)"));
        gtk_widget_set_sensitive(item, FALSE);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    }

    gtk_widget_show_all(menu);
    return menu;
}

void
mwb_rebuild_applications_menu(MorphosWorkbenchPlugin *mwb)
{
    mwb_recent_tick(mwb);

    GtkWidget *old_menu = mwb->menus[MWB_MENU_APPLICATIONS];
    mwb->menus[MWB_MENU_APPLICATIONS] = mwb_build_applications_menu(mwb);
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->menus[MWB_MENU_APPLICATIONS]), "mwb-menu-applications");
    mwb_theme_widget(mwb, mwb->menus[MWB_MENU_APPLICATIONS]);

    if (old_menu)
        gtk_widget_destroy(old_menu);
}

void
mwb_tracked_launches_clear(MorphosWorkbenchPlugin *mwb)
{
    GList *it;

    for (it = mwb->tracked_launches; it; it = it->next) {
        MwbTrackedLaunch *t = it->data;
        if (t->source_id)
            g_source_remove(t->source_id);
        t->mwb = NULL;
    }
    g_list_free_full(mwb->tracked_launches, mwb_tracked_launch_free);
    mwb->tracked_launches = NULL;
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

void
mwb_rebuild_menus(MorphosWorkbenchPlugin *mwb)
{
    gint i;
    for (i = 0; i < MWB_MENU_COUNT; i++) {
        if (mwb->menus[i])
            gtk_widget_destroy(mwb->menus[i]);
    }
    mwb_create_menus(mwb);
    mwb_apply_theme(mwb);
}
