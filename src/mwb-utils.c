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

#include <glib/gstdio.h>
#include <string.h>

/* ------------------------------------------------------------------ *
 *  Launch / actions
 *  ------------------------------------------------------------------ */

void
mwb_launch(const gchar *command)
{
    if (!command || !*command)
        return;
    gchar *argv[4] = { (gchar *)"sh", (gchar *)"-c", (gchar *)command, NULL };
    g_spawn_async(NULL, argv, NULL,
                  G_SPAWN_SEARCH_PATH,
                  NULL, NULL, NULL, NULL);
}

guint
mwb_launch_tracked(const gchar *command, GChildWatchFunc func, gpointer data)
{
    gchar *argv[4];
    GPid pid;
    GError *err = NULL;

    if (!command || !*command)
        return 0;

    argv[0] = (gchar *)"sh";
    argv[1] = (gchar *)"-c";
    argv[2] = (gchar *)command;
    argv[3] = NULL;

    if (!g_spawn_async(NULL, argv, NULL,
                       G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                       NULL, NULL, &pid, &err)) {
        if (err)
            g_error_free(err);
        return 0;
    }

    return g_child_watch_add(pid, func, data);
}

/* Create a new folder on the actual desktop (xfdesktop's icon area). */
void
mwb_desktop_new_folder(void)
{
    const gchar *desktop_dir = g_get_user_special_dir(G_USER_DIRECTORY_DESKTOP);
    gchar *fallback = NULL;
    gchar *path = NULL;
    const gchar *base = _("New Folder");
    gint n = 0;

    if (desktop_dir == NULL || *desktop_dir == '\0') {
        fallback = g_build_filename(g_get_home_dir(), "Desktop", NULL);
        desktop_dir = fallback;
    }

    if (!g_file_test(desktop_dir, G_FILE_TEST_IS_DIR))
        g_mkdir_with_parents(desktop_dir, 0755);

    do {
        g_free(path);
        if (n == 0)
            path = g_build_filename(desktop_dir, base, NULL);
        else
            path = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s %d", desktop_dir, base, n);
        n++;
    } while (g_file_test(path, G_FILE_TEST_EXISTS));

    if (g_mkdir(path, 0755) == 0)
        mwb_launch("xfdesktop --reload");

    g_free(path);
    g_free(fallback);
}

static gboolean
mwb_wallpaper_is_image(const gchar *name)
{
    const gchar *ext;

    if (!name)
        return FALSE;
    ext = strrchr(name, '.');
    if (!ext)
        return FALSE;
    return g_str_has_suffix(name, ".jpg") || g_str_has_suffix(name, ".jpeg") ||
           g_str_has_suffix(name, ".png") || g_str_has_suffix(name, ".gif") ||
           g_str_has_suffix(name, ".webp") || g_str_has_suffix(name, ".bmp") ||
           g_str_has_suffix(name, ".svg");
}

/* Cycle the XFCE desktop to the next image in the current backdrop folder. */
void
mwb_next_wallpaper(void)
{
    gchar *props_out = NULL;
    gchar *val_out = NULL;
    gchar *cmd = NULL;
    gchar **lines = NULL;
    gchar *prop = NULL;
    gchar *current = NULL;
    gchar *current_base = NULL;
    gchar *dirpath = NULL;
    GDir *dir = NULL;
    GList *images = NULL;
    GList *l;
    const gchar *name;
    const gchar *next_name = NULL;
    gint i;

    if (!g_spawn_command_line_sync("xfconf-query -c xfce4-desktop -l",
                                   &props_out, NULL, NULL, NULL) || !props_out)
        return;

    lines = g_strsplit(props_out, "\n", -1);
    for (i = 0; lines[i]; i++) {
        g_strstrip(lines[i]);
        if (g_str_has_suffix(lines[i], "/last-image")) {
            prop = g_strdup(lines[i]);
            break;
        }
    }
    g_strfreev(lines);
    g_free(props_out);

    if (!prop)
        return;

    cmd = g_strdup_printf("xfconf-query -c xfce4-desktop -p '%s'", prop);
    if (g_spawn_command_line_sync(cmd, &val_out, NULL, NULL, NULL) && val_out)
        current = g_strdup(g_strstrip(val_out));
    g_free(cmd);
    g_free(val_out);

    if (!current || !*current)
        goto out;

    dirpath = g_path_get_dirname(current);
    current_base = g_path_get_basename(current);

    dir = g_dir_open(dirpath, 0, NULL);
    if (!dir)
        goto out;

    while ((name = g_dir_read_name(dir)) != NULL) {
        if (mwb_wallpaper_is_image(name))
            images = g_list_insert_sorted(images, g_strdup(name),
                                          (GCompareFunc)g_utf8_collate);
    }

    for (l = images; l; l = l->next) {
        if (g_strcmp0((const gchar *)l->data, current_base) == 0) {
            next_name = l->next ? (const gchar *)l->next->data
                                : (const gchar *)images->data;
            break;
        }
    }
    if (!next_name && images)
        next_name = (const gchar *)images->data;

    if (next_name) {
        gchar *next_path = g_build_filename(dirpath, next_name, NULL);
        cmd = g_strdup_printf("xfconf-query -c xfce4-desktop -p '%s' -s '%s'",
                              prop, next_path);
        g_spawn_command_line_sync(cmd, NULL, NULL, NULL, NULL);
        g_free(next_path);
        g_free(cmd);
    }

out:
    if (dir)
        g_dir_close(dir);
    g_list_free_full(images, g_free);
    g_free(current_base);
    g_free(dirpath);
    g_free(current);
    g_free(prop);
}

GtkWidget *
mwb_screenbar_divider(void)
{
    GtkWidget *divider = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_style_context_add_class(gtk_widget_get_style_context(divider), "mwb-screenbar-divider");
    return divider;
}
