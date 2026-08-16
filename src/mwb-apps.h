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

#ifndef __MWB_APPS_H__
#define __MWB_APPS_H__

#include <glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS

typedef enum {
    MWB_CAT_ACCESSORIES,
    MWB_CAT_DEVELOPMENT,
    MWB_CAT_GAMES,
    MWB_CAT_GRAPHICS,
    MWB_CAT_INTERNET,
    MWB_CAT_MULTIMEDIA,
    MWB_CAT_OFFICE,
    MWB_CAT_SCIENCE,
    MWB_CAT_SETTINGS,
    MWB_CAT_SYSTEM,
    MWB_CAT_OTHER,
    MWB_CAT_COUNT
} MwbAppCategory;

typedef struct {
    gchar *name;
    gchar *icon;
    gchar *exec;   /* field codes stripped, ready for sh -c */
    guint  category;
} MwbDesktopApp;

GList       *mwb_apps_scan            (void);
void         mwb_apps_free            (GList *apps);
const gchar *mwb_app_category_label   (guint category);
gchar       *mwb_app_exec_clean       (const gchar *exec);
guint        mwb_app_categorize       (const gchar *categories);

G_END_DECLS

#endif /* !__MWB_APPS_H__ */
