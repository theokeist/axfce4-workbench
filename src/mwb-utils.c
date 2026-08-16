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
                  G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                  NULL, NULL, NULL, NULL);
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

GtkWidget *
mwb_screenbar_divider(void)
{
    GtkWidget *divider = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_style_context_add_class(gtk_widget_get_style_context(divider), "mwb-screenbar-divider");
    return divider;
}
