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

#ifndef __MORPHOS_WORKBENCH_H__
#define __MORPHOS_WORKBENCH_H__

#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>

G_BEGIN_DECLS

/* Plugin definitions */
#define MWB_ICON_NAME "ambient-logo"
#define MWB_FALLBACK_ICON "distributor-logo"

/* Top bar menu titles (mirrors MorphOS Ambient menu bar) */
typedef enum {
    MWB_MENU_AMBIENT,    /* Ambient logo button */
    MWB_MENU_WORKBENCH,
    MWB_MENU_AMBIENT_MENU,
    MWB_MENU_ICONS,
    MWB_MENU_DISK,
    MWB_MENU_APPLICATIONS,
    MWB_MENU_COUNT
} MorphosWorkbenchMenuId;

/* Screenbar lamps (MorphOS Netlamps / Drivelamps) */
typedef enum {
    MWB_LAMP_NET_TX,
    MWB_LAMP_NET_RX,
    MWB_LAMP_DISK_0,
    MWB_LAMP_DISK_1,
    MWB_LAMP_COUNT
} MorphosWorkbenchLamp;

/* Screenbar vertical gauge kinds */
typedef enum {
    MWB_GAUGE_CPU,
    MWB_GAUGE_MEM,
    MWB_GAUGE_DISK,
    MWB_GAUGE_COUNT
} MorphosWorkbenchGauge;

/* Theme variants */
typedef enum {
    MWB_THEME_CLASSIC,   /* original Ambient dark-blue */
    MWB_THEME_DARK,      /* sleek near-black */
    MWB_THEME_LIGHT,     /* silver / light-grey */
    MWB_THEME_COUNT
} MorphosWorkbenchTheme;

typedef enum {
    MWB_LOGO_CLASSIC,
    MWB_LOGO_FLAT,
    MWB_LOGO_MONO,
    MWB_LOGO_COUNT
} MorphosWorkbenchLogo;

G_END_DECLS

#endif /* !__MORPHOS_WORKBENCH_H__ */
