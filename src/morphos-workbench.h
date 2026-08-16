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
#include <gmodule.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>

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

/* Screenbar widgets (reorderable) */
typedef enum {
    MWB_WIDGET_DRIVELAMPS,
    MWB_WIDGET_NETLAMPS,
    MWB_WIDGET_WIFI,
    MWB_WIDGET_BATTERY,
    MWB_WIDGET_CPU,
    MWB_WIDGET_MEM,
    MWB_WIDGET_DISK,
    MWB_WIDGET_SYSINFO,
    MWB_WIDGET_VOLUME,
    MWB_WIDGET_CLOCK,
    MWB_WIDGET_COUNT
} MorphosWorkbenchWidget;

/* Screenbar gauge visual styles */
typedef enum {
    MWB_GAUGE_STYLE_INDUSTRIAL, /* classic segmented LED blocks */
    MWB_GAUGE_STYLE_3D,         /* glossy chrome cylinder */
    MWB_GAUGE_STYLE_PLAIN,      /* flat square, thin frame */
    MWB_GAUGE_STYLE_COUNT
} MorphosWorkbenchGaugeStyle;

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

/* Recently-used application entry (shown at the top of the Applications menu) */
#define MWB_RECENT_MAX 3
#define MWB_RECENT_CYCLES 3

typedef struct {
    gchar *icon;
    gchar *label;
    gchar *cmd;
    gint   cycles;
} MwbRecentApp;

/* Plugin state */
typedef struct {
    XfcePanelPlugin *plugin;
    GtkWidget       *bar;             /* top bar container */
    GtkWidget       *logo_button;     /* Ambient Workbench identity */
    GtkWidget       *title;           /* workbench title label */
    GtkWidget       *clock_label;     /* date/time label */
    GtkWidget       *clock_button;    /* clickable time (opens calendar) */
    GtkWidget       *mem_gauge;       /* vertical memory gauge */
    GtkWidget       *cpu_gauges[16];  /* per-core vertical CPU gauges */
    GtkWidget       *disk_gauge;      /* vertical disk gauge */
    GtkWidget       *net_lamps[2];    /* Netlamps: TX, RX */
    GtkWidget       *disk_lamps[2];   /* Drivelamps: disk 0, disk 1 */
    GModule         *nm_module;       /* dlopen'd xfce4-networkmanager module */
    GtkWidget       *nm_plugin;       /* embedded xfce4-networkmanager widget */
    GtkWidget       *batt_button;     /* battery button */
    GtkWidget       *batt_icon;       /* battery image */
    GtkWidget       *batt_label;      /* battery % label */
    GtkWidget       *sys_button;      /* system info button */
    GtkWidget       *vol_button;      /* volume button */
    GtkWidget       *vol_scale;       /* volume slider in popup */
    GtkWidget       *vol_icon;        /* volume icon */
    GtkWidget       *vol_mute_button; /* volume mute toggle in popup */
    GtkWidget       *vol_mute_icon;   /* mute button icon */
    GtkWidget       *vol_percent_label; /* volume % label in popup */
    gboolean         vol_muted;       /* mute state */
    GtkWidget       *screenbar;       /* right-side screenbar container */
    GtkWidget       *calendar_popup;  /* calendar popup window */
    GtkWidget       *volume_popup;    /* volume popup window */
    GtkWidget       *calendar;        /* the GtkCalendar itself */
    guint            calendar_grab;   /* seat grab timer id */
    guint            volume_grab;     /* seat grab timer id */
    gint64           popup_open_time;
    GtkWidget       *menus[MWB_MENU_COUNT];
    GtkWidget       *menu_buttons[MWB_MENU_COUNT];
    GtkWidget       *active_button;   /* currently open menu button */
    GList           *recent_apps;     /* recently closed apps (MRU) */
    GList           *tracked_launches; /* pending app-close watches */
    GList           *installed_apps;   /* cached system application list */
    guint            clock_timeout;
    guint            mem_timeout;
    guint            cpu_timeout;
    guint            net_timeout;
    guint            disk_timeout;
    guint            batt_timeout;
    guint            sys_timeout;
    guint            title_timeout;
    guint            vol_timeout;
    MorphosWorkbenchTheme theme;
    MorphosWorkbenchLogo logo_variant;
    gint             gauge_style;   /* MorphosWorkbenchGaugeStyle */
    gboolean         override_theme;  /* force Workbench theming on all widgets */
    gboolean         show_clock;
    gboolean         show_membar;
    gboolean         show_cpumbar;
    gboolean         show_diskgauge;
    gboolean         show_netlamps;
    gboolean         show_drivelamps;
    gboolean         show_volume;
    gboolean         show_wifi;
    gboolean         show_battery;
    gboolean         show_sysinfo;
    gboolean         show_logo;
    gboolean         show_title;
    gboolean         show_dynamic_title; /* title follows the foreground app */
    gboolean         show_workbench_menu;
    gboolean         show_ambient_menu;
    gboolean         show_icons_menu;
    gboolean         show_disk_menu;
    gboolean         show_applications_menu;
    guint64          cpu_prev_total;
    guint64          cpu_prev_idle;
    guint64          cpu_prev_core_total[16];
    guint64          cpu_prev_core_idle[16];
    gint             cpu_ncores;
    gdouble          cpu_load;
    guint64          net_prev_tx;
    guint64          net_prev_rx;
    guint64          net_prev_bytes;
    gint64           net_prev_time;
    guint64          disk_prev_sects[2];
    guint            vol_percent;
    gint             widget_order[MWB_WIDGET_COUNT]; /* screenbar display order */
} MorphosWorkbenchPlugin;

/* theme.c */
void        mwb_init_css                (void);
void        mwb_apply_theme             (MorphosWorkbenchPlugin *mwb);
void        mwb_apply_logo              (MorphosWorkbenchPlugin *mwb);
void        mwb_theme_widget            (MorphosWorkbenchPlugin *mwb,
                                         GtkWidget *widget);
const gchar *mwb_logo_icon_name         (MorphosWorkbenchLogo variant);

/* utils.c */
void        mwb_launch                  (const gchar *command);
guint       mwb_launch_tracked          (const gchar *command,
                                         GChildWatchFunc func,
                                         gpointer data);
void        mwb_desktop_new_folder      (void);
GtkWidget  *mwb_screenbar_divider       (void);

/* gauges.c */
GtkWidget  *mwb_gauge_new               (gint kind, const gchar *label_text,
                                         MorphosWorkbenchTheme theme,
                                         MorphosWorkbenchGaugeStyle style);
void        mwb_gauge_set               (GtkWidget *gauge, gdouble frac);
void        mwb_gauge_set_theme         (GtkWidget *gauge,
                                         MorphosWorkbenchTheme theme);
void        mwb_gauge_set_style         (GtkWidget *gauge,
                                         MorphosWorkbenchGaugeStyle style);

/* menus.c */
GtkWidget  *mwb_create_menu_title       (MorphosWorkbenchPlugin *mwb,
                                         const gchar *text);
void        mwb_menu_toggle             (GtkButton *button,
                                         MorphosWorkbenchPlugin *mwb);
void        mwb_create_menus            (MorphosWorkbenchPlugin *mwb);
void        mwb_rebuild_applications_menu (MorphosWorkbenchPlugin *mwb);
void        mwb_recent_record           (MorphosWorkbenchPlugin *mwb,
                                         const gchar *icon,
                                         const gchar *label,
                                         const gchar *cmd);
void        mwb_recent_tick             (MorphosWorkbenchPlugin *mwb);
void        mwb_recent_clear            (MorphosWorkbenchPlugin *mwb);
void        mwb_tracked_launches_clear  (MorphosWorkbenchPlugin *mwb);

/* screenbar.c */
void        mwb_build_screenbar         (MorphosWorkbenchPlugin *mwb);

/* config.c */
void        mwb_save_config             (XfcePanelPlugin *plugin,
                                         MorphosWorkbenchPlugin *mwb);
void        mwb_load_config             (MorphosWorkbenchPlugin *mwb);
void        mwb_configure_plugin        (XfcePanelPlugin *plugin,
                                         MorphosWorkbenchPlugin *mwb);

/* bar (morphos-workbench.c) */
void        mwb_build_bar               (MorphosWorkbenchPlugin *mwb);
void        mwb_apply_left_visibility   (MorphosWorkbenchPlugin *mwb);
void        mwb_update_title            (MorphosWorkbenchPlugin *mwb);

G_END_DECLS

#endif /* !__MORPHOS_WORKBENCH_H__ */
