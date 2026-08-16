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
 *  Recently-closed applications list (MRU)
 *  ------------------------------------------------------------------ */

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

void
mwb_recent_record(MorphosWorkbenchPlugin *mwb, const gchar *icon,
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

void
mwb_recent_tick(MorphosWorkbenchPlugin *mwb)
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
}

void
mwb_recent_clear(MorphosWorkbenchPlugin *mwb)
{
    g_list_free_full(mwb->recent_apps, (GDestroyNotify)mwb_recent_app_free);
    mwb->recent_apps = NULL;
}
