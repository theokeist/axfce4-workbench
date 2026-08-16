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

#include "mwb-apps.h"

#include <string.h>
#include <glib/gi18n-lib.h>
#include <gio/gdesktopappinfo.h>

static const gchar *MWB_CATEGORY_LABELS[MWB_CAT_COUNT] = {
    N_("Accessories"),
    N_("Development"),
    N_("Games"),
    N_("Graphics"),
    N_("Internet"),
    N_("Multimedia"),
    N_("Office"),
    N_("Science"),
    N_("Settings"),
    N_("System"),
    N_("Other"),
};

typedef struct {
    const gchar *token;
    guint        category;
} MwbCatToken;

static const MwbCatToken MWB_CAT_TOKENS[] = {
    { "Game",              MWB_CAT_GAMES },
    { "Development",       MWB_CAT_DEVELOPMENT },
    { "Graphics",          MWB_CAT_GRAPHICS },
    { "Photography",       MWB_CAT_GRAPHICS },
    { "Office",            MWB_CAT_OFFICE },
    { "Science",           MWB_CAT_SCIENCE },
    { "Education",         MWB_CAT_SCIENCE },
    { "AudioVideo",        MWB_CAT_MULTIMEDIA },
    { "Audio",             MWB_CAT_MULTIMEDIA },
    { "Video",             MWB_CAT_MULTIMEDIA },
    { "Network",           MWB_CAT_INTERNET },
    { "WebBrowser",        MWB_CAT_INTERNET },
    { "Email",             MWB_CAT_INTERNET },
    { "Chat",              MWB_CAT_INTERNET },
    { "FileTransfer",      MWB_CAT_INTERNET },
    { "Settings",          MWB_CAT_SETTINGS },
    { "System",            MWB_CAT_SYSTEM },
    { "TerminalEmulator",  MWB_CAT_SYSTEM },
    { "FileManager",       MWB_CAT_SYSTEM },
    { "Monitor",           MWB_CAT_SYSTEM },
    { "Utility",           MWB_CAT_ACCESSORIES },
    { "Accessibility",     MWB_CAT_ACCESSORIES },
};

const gchar *
mwb_app_category_label(guint category)
{
    if (category >= MWB_CAT_COUNT)
        return MWB_CATEGORY_LABELS[MWB_CAT_OTHER];
    return MWB_CATEGORY_LABELS[category];
}

guint
mwb_app_categorize(const gchar *categories)
{
    gchar **toks;
    guint i, j;
    guint result = MWB_CAT_OTHER;

    if (!categories || !*categories)
        return MWB_CAT_OTHER;

    toks = g_strsplit_set(categories, "; ", -1);
    for (i = 0; toks[i]; i++) {
        for (j = 0; j < G_N_ELEMENTS(MWB_CAT_TOKENS); j++) {
            if (g_strcmp0(toks[i], MWB_CAT_TOKENS[j].token) == 0) {
                result = MWB_CAT_TOKENS[j].category;
                g_strfreev(toks);
                return result;
            }
        }
    }
    g_strfreev(toks);
    return result;
}

gchar *
mwb_app_exec_clean(const gchar *exec)
{
    GString *out;
    const gchar *p;
    gchar *result;

    if (!exec)
        return NULL;

    out = g_string_new(NULL);
    p = exec;
    while (*p) {
        if (*p == '%' && p[1] != '\0') {
            gchar c = p[1];
            if (c == '%') {
                g_string_append_c(out, '%');
                p += 2;
                continue;
            }
            /* field-code arguments: strip them (launched without file/URL args) */
            if (strchr("fFuUdDnNickvm", c) != NULL) {
                p += 2;
                continue;
            }
        }
        g_string_append_c(out, *p++);
    }

    result = g_strdup(g_strstrip(out->str));
    g_string_free(out, TRUE);
    return result;
}

static gint
mwb_app_compare(gconstpointer a, gconstpointer b)
{
    const MwbDesktopApp *aa = a;
    const MwbDesktopApp *bb = b;

    if (aa->category != bb->category)
        return (gint)aa->category - (gint)bb->category;
    return g_utf8_collate(aa->name, bb->name);
}

void
mwb_apps_free(GList *apps)
{
    GList *l;
    for (l = apps; l; l = l->next) {
        MwbDesktopApp *app = l->data;
        g_free(app->name);
        g_free(app->icon);
        g_free(app->exec);
        g_free(app);
    }
    g_list_free(apps);
}

GList *
mwb_apps_scan(void)
{
    GList *infos = g_app_info_get_all();
    GList *apps = NULL;
    GList *l;

    for (l = infos; l; l = l->next) {
        GAppInfo *info = G_APP_INFO(l->data);
        const gchar *name, *cmdline, *cats;
        GIcon *gicon;
        gchar *icon = NULL;
        MwbDesktopApp *app;

        if (!g_app_info_should_show(info))
            continue;

        name = g_app_info_get_name(info);
        cmdline = g_app_info_get_commandline(info);
        if (!name || !*name || !cmdline || !*cmdline)
            continue;

        gicon = g_app_info_get_icon(info);
        if (G_IS_THEMED_ICON(gicon)) {
            const gchar * const *names = g_themed_icon_get_names(G_THEMED_ICON(gicon));
            if (names && names[0])
                icon = g_strdup(names[0]);
        }
        if (!icon)
            icon = g_strdup("application-x-executable");

        cats = NULL;
        if (G_IS_DESKTOP_APP_INFO(info))
            cats = g_desktop_app_info_get_categories(G_DESKTOP_APP_INFO(info));

        app = g_new0(MwbDesktopApp, 1);
        app->name = g_strdup(name);
        app->icon = icon;
        app->exec = mwb_app_exec_clean(cmdline);
        app->category = mwb_app_categorize(cats);
        apps = g_list_prepend(apps, app);
    }
    g_list_free_full(infos, g_object_unref);

    return g_list_sort(apps, mwb_app_compare);
}
