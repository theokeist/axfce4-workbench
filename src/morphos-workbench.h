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
    MWB_WIDGET_NOTIFICATIONS,
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
    MWB_THEME_SYSTEM,    /* follow desktop system GTK theme */
    MWB_THEME_COUNT
} MorphosWorkbenchTheme;

typedef enum {
    MWB_LOGO_CLASSIC,
    MWB_LOGO_FLAT,
    MWB_LOGO_MONO,
    MWB_LOGO_LEAF,
    MWB_LOGO_F,
    MWB_LOGO_COUNT
} MorphosWorkbenchLogo;

/* Menu icon size */
typedef enum {
    MWB_ICON_SMALL,
    MWB_ICON_MEDIUM,
    MWB_ICON_BIG,
    MWB_ICON_COUNT
} MorphosWorkbenchIconSize;

/* Recently-used application entry (shown at the top of the Applications menu) */
#define MWB_RECENT_MAX 3
#define MWB_RECENT_CYCLES 3

typedef struct {
    gchar   *id;
    gchar   *app_name;
    gchar   *icon_name;
    gchar   *summary;
    gchar   *body;
    gint64   timestamp;
    gboolean is_read;
} MwbNotification;

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
    GtkWidget       *batt_icon;       /* battery custom drawing area */
    GtkWidget       *batt_label;      /* battery percentage label */
    gint             batt_percent;    /* 0-100 */
    gboolean         batt_charging;   /* AC connected / charging */
    gchar            batt_status_str[32]; /* status: Charging, Discharging, Full */
    gchar            batt_tech_str[32];   /* technology: Li-ion */
    gdouble          batt_voltage_val;    /* voltage in Volts */
    gint             batt_cycle_val;      /* cycle count */
    gchar            batt_profile_str[32];/* current power profile */
    GtkWidget       *batt_pop_gauge;      /* large horizontal battery bar inside popover */
    GtkWidget       *batt_pop_pct_lbl;    /* percentage label inside popover */
    GtkWidget       *batt_pop_state_lbl;  /* state / power source label */
    GtkWidget       *batt_pop_badge;      /* status badge inside popover header */
    GtkWidget       *batt_prof_saver_btn; /* power saver button */
    GtkWidget       *batt_prof_bal_btn;   /* balanced button */
    GtkWidget       *batt_prof_perf_btn;  /* performance button */
    GtkWidget       *batt_pop_tech_lbl;   /* technology label */
    GtkWidget       *batt_pop_volt_lbl;   /* voltage label */
    GtkWidget       *batt_pop_cycle_lbl;  /* cycles label */
    GtkWidget       *sys_button;      /* system info button */
    GtkWidget       *vol_button;      /* volume button on screenbar */
    GtkWidget       *vol_icon;        /* volume icon image */
    GtkWidget       *vol_scale;       /* main volume scale */
    GtkWidget       *vol_mute_button; /* volume mute button */
    GtkWidget       *vol_mute_icon;   /* mute button icon */
    GtkWidget       *vol_percent_label; /* volume % label */
    GtkWidget       *vol_mic_button;  /* microphone mute button */
    GtkWidget       *vol_mic_icon;    /* mic icon image */
    GtkWidget       *vol_mic_scale;   /* mic level scale */
    GtkWidget       *vol_mic_percent_label; /* mic % label */
    GtkWidget       *vol_playing_card;  /* now-playing media card */
    GtkWidget       *vol_playing_art;   /* now-playing album art */
    GtkWidget       *vol_playing_title; /* now-playing title label */
    GtkWidget       *vol_playing_artist; /* now-playing artist label */
    GtkWidget       *vol_playing_status; /* now-playing status / player badge */
    GtkWidget       *vol_playing_controls; /* playback controls container */
    GtkWidget       *vol_playing_prev_btn; /* previous track button */
    GtkWidget       *vol_playing_play_btn; /* play/pause button */
    GtkWidget       *vol_playing_next_btn; /* next track button */
    GtkWidget       *vol_playing_play_icon; /* play/pause icon */
    gchar           *vol_playing_bus;   /* current MPRIS player D-Bus destination */
    gchar           *vol_last_art_url;  /* cached now-playing art URL (skip re-decode) */
    gchar           *vol_streams_sig;   /* signature of current playback streams set */
    guint            vol_media_task;    /* in-flight now-playing async task guard */
    GtkWidget       *vol_streams_box;   /* active playback streams container */
    gboolean         vol_muted;       /* mute state */
    gboolean         vol_mic_muted;   /* microphone mute state */
    guint            mic_percent;     /* microphone level */
    GtkWidget       *screenbar;       /* right-side screenbar container */
    GtkWidget       *calendar_popup;  /* calendar popup window */
    GtkWidget       *volume_popup;    /* volume popup window */
    GtkWidget       *batt_popup;      /* battery popup window */
    GtkWidget       *wifi_popup;      /* wifi/network popup window */
    GtkWidget       *notify_popup;    /* notification center popup */
    GtkWidget       *notify_button;   /* notification button on screenbar */
    GtkWidget       *notify_icon;     /* notification icon */
    GtkWidget       *notify_badge;    /* unread count badge on screenbar */
    GtkWidget       *notify_pop_badge;/* header status badge in notification popover */
    GtkWidget       *notify_list_box; /* notification items container */
    GList           *notifications;   /* list of MwbNotification */
    GtkWidget       *calendar;        /* the GtkCalendar itself */
    GtkWidget       *wifi_button;     /* wifi button on screenbar */
    guint            calendar_grab;   /* seat grab timer id */
    guint            volume_grab;     /* seat grab timer id */
    guint            batt_grab;       /* seat grab timer id */
    guint            wifi_grab;       /* seat grab timer id */
    guint            notify_grab;     /* seat grab timer id */
    GDBusConnection *notify_mon_conn; /* private D-Bus monitor connection */
    guint            notify_dbus_id;  /* D-Bus signal subscription id */
    guint            notify_filter_id;/* D-Bus message filter id */
    guint            notify_poll_id;  /* notification poll timer id */
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
    guint            vol_debounce_id;
    guint            vol_mic_debounce_id;
    gint             net_prev_state[2];
    gchar           *net_prev_tip;
    gint             net_tip_ticks;
    MorphosWorkbenchTheme theme;
    MorphosWorkbenchLogo logo_variant;
    gint             gauge_style;   /* MorphosWorkbenchGaugeStyle */
    gint             icon_size;     /* MorphosWorkbenchIconSize */
    gint             menu_opacity;  /* 0-100, 100 = opaque menus */
    gboolean         clear_bar_bg;  /* clear/transparent background for the bar */
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
    gboolean         show_notifications;
    gboolean         show_logo;
    gboolean         show_title;
    gboolean         show_dynamic_title; /* title follows the foreground app */
    gboolean         show_workbench_menu;
    gboolean         show_ambient_menu;
    gboolean         show_icons_menu;
    gboolean         show_disk_menu;
    gboolean         show_applications_menu;
    gboolean         show_recent_apps;  /* Applications: recently closed */
    gboolean         show_fav_apps;     /* Applications: hardcoded favorites */
    gboolean         show_all_apps;     /* Applications: categorized installed */
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
    GtkWidget       *widget_widgets[MWB_WIDGET_COUNT]; /* per-widget slot containers */
} MorphosWorkbenchPlugin;

/* theme.c */
void        mwb_init_css                (void);
void        mwb_apply_theme             (MorphosWorkbenchPlugin *mwb);
void        mwb_apply_logo              (MorphosWorkbenchPlugin *mwb);
void        mwb_apply_menu_opacity      (MorphosWorkbenchPlugin *mwb);
void        mwb_theme_widget            (MorphosWorkbenchPlugin *mwb,
                                         GtkWidget *widget);
const gchar *mwb_logo_icon_name         (MorphosWorkbenchLogo variant);

/* utils.c */
void        mwb_launch                  (const gchar *command);
guint       mwb_launch_tracked          (const gchar *command,
                                         GChildWatchFunc func,
                                         gpointer data);
void        mwb_desktop_new_folder      (void);
void        mwb_next_wallpaper          (void);
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
void        mwb_rebuild_menus           (MorphosWorkbenchPlugin *mwb);
void        mwb_set_icon_size           (gint size);
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
void        mwb_apply_widget_order      (MorphosWorkbenchPlugin *mwb);
const gchar *mwb_widget_name            (guint widget);
void        mwb_notifications_refresh   (MorphosWorkbenchPlugin *mwb);
void        mwb_init_notification_monitor (MorphosWorkbenchPlugin *mwb);
void        mwb_notification_add        (MorphosWorkbenchPlugin *mwb,
                                         const gchar *app_name,
                                         const gchar *icon_name,
                                         const gchar *summary,
                                         const gchar *body_text);
void        mwb_notification_free       (MwbNotification *n);

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
