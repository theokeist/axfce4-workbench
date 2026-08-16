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

#include <string.h>
#include <time.h>
#include <unistd.h>

/* ------------------------------------------------------------------ *
 *  Popup window helper (proper seat grab, outside-click dismiss)
 *  ------------------------------------------------------------------ */

static void
mwb_popup_hide(MorphosWorkbenchPlugin *mwb, GtkWidget *win)
{
    if (!win || !gtk_widget_get_visible(win))
        return;

    GdkDisplay *display = gtk_widget_get_display(win);
    GdkSeat *seat = gdk_display_get_default_seat(display);
    if (seat)
        gdk_seat_ungrab(seat);

    if (win == mwb->calendar_popup && mwb->calendar_grab) {
        g_source_remove(mwb->calendar_grab);
        mwb->calendar_grab = 0;
    }
    if (win == mwb->volume_popup && mwb->volume_grab) {
        g_source_remove(mwb->volume_grab);
        mwb->volume_grab = 0;
    }

    gtk_widget_hide(win);
}

static gboolean
mwb_popup_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    MorphosWorkbenchPlugin *mwb = data;
    GtkWidget *win = widget;

    if (g_get_monotonic_time() - mwb->popup_open_time < 150000)
        return FALSE;

    gint win_w, win_h, win_x, win_y;
    gtk_window_get_size(GTK_WINDOW(win), &win_w, &win_h);
    gtk_window_get_position(GTK_WINDOW(win), &win_x, &win_y);

    if (event->x_root < win_x || event->x_root > (win_x + win_w) ||
        event->y_root < win_y || event->y_root > (win_y + win_h)) {
        mwb_popup_hide(mwb, win);
        return TRUE;
    }
    return FALSE;
}

static gboolean
mwb_popup_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    MorphosWorkbenchPlugin *mwb = data;
    if (event->keyval == GDK_KEY_Escape) {
        mwb_popup_hide(mwb, widget);
        return TRUE;
    }
    return FALSE;
}

static void
mwb_popup_show(MorphosWorkbenchPlugin *mwb, GtkWidget *win, GtkWidget *anchor)
{
    GtkAllocation alloc;
    GtkRequisition req;
    gint x = 0, y = 0;

    if (mwb->calendar_popup && gtk_widget_get_visible(mwb->calendar_popup))
        mwb_popup_hide(mwb, mwb->calendar_popup);
    if (mwb->volume_popup && gtk_widget_get_visible(mwb->volume_popup))
        mwb_popup_hide(mwb, mwb->volume_popup);

    GdkWindow *btn_win = gtk_widget_get_window(anchor);
    if (btn_win)
        gdk_window_get_origin(btn_win, &x, &y);
    gtk_widget_get_allocation(anchor, &alloc);
    x += alloc.x;
    y += alloc.y;

    gtk_widget_get_preferred_size(win, NULL, &req);

    GdkScreen *screen = gtk_widget_get_screen(win);
    GdkDisplay *display = gdk_screen_get_display(screen);
    GdkMonitor *monitor = gdk_display_get_monitor_at_window(display,
                                                            gdk_screen_get_root_window(screen));
    GdkRectangle monitor_geom = { 0, 0, 1920, 1080 };
    if (monitor)
        gdk_monitor_get_geometry(monitor, &monitor_geom);

    gint popup_x = x + (alloc.width / 2) - (req.width / 2);
    gint popup_y = y + alloc.height + 4;

    if (popup_x + req.width > monitor_geom.x + monitor_geom.width - 6)
        popup_x = monitor_geom.x + monitor_geom.width - req.width - 6;
    if (popup_x < monitor_geom.x + 6)
        popup_x = monitor_geom.x + 6;

    gtk_window_move(GTK_WINDOW(win), popup_x, popup_y);
    mwb->popup_open_time = g_get_monotonic_time();
    /* The popup children are created lazily; show the complete tree. */
    gtk_widget_show_all(win);
    gtk_window_present(GTK_WINDOW(win));
    if (win == mwb->calendar_popup)
        gtk_widget_grab_focus(mwb->calendar);
    else if (win == mwb->volume_popup)
        gtk_widget_grab_focus(mwb->vol_scale);
}

/* ------------------------------------------------------------------ *
 *  Clock / calendar + volume popups (MorphOS Calendar & Volume)
 *  ------------------------------------------------------------------ */

static void
mwb_clock_clicked(GtkButton *button G_GNUC_UNUSED, MorphosWorkbenchPlugin *mwb)
{
    if (mwb->calendar_popup && gtk_widget_get_visible(mwb->calendar_popup)) {
        mwb_popup_hide(mwb, mwb->calendar_popup);
        return;
    }

    if (!mwb->calendar_popup) {
        mwb->calendar_popup = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_decorated(GTK_WINDOW(mwb->calendar_popup), FALSE);
        gtk_window_set_skip_taskbar_hint(GTK_WINDOW(mwb->calendar_popup), TRUE);
        gtk_window_set_skip_pager_hint(GTK_WINDOW(mwb->calendar_popup), TRUE);
        gtk_window_set_type_hint(GTK_WINDOW(mwb->calendar_popup), GDK_WINDOW_TYPE_HINT_POPUP_MENU);
        gtk_window_set_resizable(GTK_WINDOW(mwb->calendar_popup), FALSE);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->calendar_popup), "mwb-popup");
        gtk_widget_set_name(mwb->calendar_popup, "mwb-calendar");
        mwb_theme_widget(mwb, mwb->calendar_popup);

        mwb->calendar = gtk_calendar_new();
        gtk_widget_set_margin_top(mwb->calendar, 8);
        gtk_widget_set_margin_bottom(mwb->calendar, 8);
        gtk_widget_set_margin_start(mwb->calendar, 8);
        gtk_widget_set_margin_end(mwb->calendar, 8);
        gtk_container_add(GTK_CONTAINER(mwb->calendar_popup), mwb->calendar);

        g_signal_connect(mwb->calendar_popup, "button-press-event",
                         G_CALLBACK(mwb_popup_button_press), mwb);
        g_signal_connect(mwb->calendar_popup, "key-press-event",
                         G_CALLBACK(mwb_popup_key_press), mwb);
        g_signal_connect(mwb->calendar_popup, "destroy",
                         G_CALLBACK(gtk_widget_destroyed), &mwb->calendar_popup);
    }

    /* select today's date */
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    if (tm) {
        gtk_calendar_select_month(GTK_CALENDAR(mwb->calendar), tm->tm_mon, 1900 + tm->tm_year);
        gtk_calendar_select_day(GTK_CALENDAR(mwb->calendar), tm->tm_mday);
        gtk_calendar_mark_day(GTK_CALENDAR(mwb->calendar), tm->tm_mday);
    }

    mwb_popup_show(mwb, mwb->calendar_popup, mwb->clock_button);
}

static gboolean
mwb_volume_get_percent(MorphosWorkbenchPlugin *mwb)
{
    gchar *out = NULL;
    guint sum = 0, n = 0;
    gchar *p, *q;

    if (!g_spawn_command_line_sync("pactl get-sink-volume @DEFAULT_SINK@",
                                   &out, NULL, NULL, NULL) || !out)
        return FALSE;

    /* average every channel percentage ("... / 86% / ...") in the output */
    for (p = out; (p = strchr(p, '%')) != NULL; p++) {
        q = p;
        while (q > out && g_ascii_isdigit(*(q - 1)))
            q--;
        if (q < p) {
            sum += (guint)atoi(q);
            n++;
        }
    }
    g_free(out);

    if (n == 0)
        return FALSE;

    mwb->vol_percent = MIN(sum / n, 100u);
    return TRUE;
}

static gboolean
mwb_volume_get_muted(MorphosWorkbenchPlugin *mwb)
{
    gchar *out = NULL;

    if (!g_spawn_command_line_sync("pactl get-sink-mute @DEFAULT_SINK@",
                                   &out, NULL, NULL, NULL) || !out)
        return FALSE;

    mwb->vol_muted = (strstr(out, "yes") != NULL);
    g_free(out);
    return TRUE;
}

static void
mwb_volume_icon_update(MorphosWorkbenchPlugin *mwb)
{
    const gchar *icon;

    if (mwb->vol_muted || mwb->vol_percent == 0)
        icon = "audio-volume-muted";
    else if (mwb->vol_percent < 33)
        icon = "audio-volume-low";
    else if (mwb->vol_percent < 66)
        icon = "audio-volume-medium";
    else
        icon = "audio-volume-high";

    if (mwb->vol_icon)
        gtk_image_set_from_icon_name(GTK_IMAGE(mwb->vol_icon), icon, GTK_ICON_SIZE_MENU);
    if (mwb->vol_mute_icon)
        gtk_image_set_from_icon_name(GTK_IMAGE(mwb->vol_mute_icon), icon, GTK_ICON_SIZE_MENU);
    if (mwb->vol_percent_label) {
        gchar *txt = g_strdup_printf("%d%%", mwb->vol_percent);
        gtk_label_set_text(GTK_LABEL(mwb->vol_percent_label), txt);
        g_free(txt);
    }
}

static void
mwb_volume_mute_clicked(GtkButton *button G_GNUC_UNUSED, MorphosWorkbenchPlugin *mwb)
{
    mwb_launch("pactl set-sink-mute @DEFAULT_SINK@ toggle");
    mwb->vol_muted = !mwb->vol_muted;
    mwb_volume_icon_update(mwb);
}

static void
mwb_volume_changed(GtkRange *range, MorphosWorkbenchPlugin *mwb)
{
    guint pct = (guint)gtk_range_get_value(range);
    mwb->vol_percent = MIN(pct, 100u);
    mwb->vol_muted = FALSE;
    mwb_volume_icon_update(mwb);
    gchar *cmd = g_strdup_printf("pactl set-sink-volume @DEFAULT_SINK@ %u%%", mwb->vol_percent);
    mwb_launch(cmd);
    g_free(cmd);
}

static void
mwb_volume_clicked(GtkButton *button G_GNUC_UNUSED, MorphosWorkbenchPlugin *mwb)
{
    if (mwb->volume_popup && gtk_widget_get_visible(mwb->volume_popup)) {
        mwb_popup_hide(mwb, mwb->volume_popup);
        return;
    }

    if (!mwb->volume_popup) {
        mwb->volume_popup = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_decorated(GTK_WINDOW(mwb->volume_popup), FALSE);
        gtk_window_set_skip_taskbar_hint(GTK_WINDOW(mwb->volume_popup), TRUE);
        gtk_window_set_skip_pager_hint(GTK_WINDOW(mwb->volume_popup), TRUE);
        gtk_window_set_type_hint(GTK_WINDOW(mwb->volume_popup), GDK_WINDOW_TYPE_HINT_POPUP_MENU);
        gtk_window_set_resizable(GTK_WINDOW(mwb->volume_popup), FALSE);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->volume_popup), "mwb-popup");
        mwb_theme_widget(mwb, mwb->volume_popup);

        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_container_set_border_width(GTK_CONTAINER(box), 10);

        /* header: title + live percentage */
        GtkWidget *hdr = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        GtkWidget *lbl = gtk_label_new(_("Volume"));
        gtk_box_pack_start(GTK_BOX(hdr), lbl, TRUE, TRUE, 0);
        mwb->vol_percent_label = gtk_label_new("0%");
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->vol_percent_label), "mwb-gauge-value");
        gtk_box_pack_start(GTK_BOX(hdr), mwb->vol_percent_label, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), hdr, FALSE, FALSE, 0);

        /* controls: mute toggle + slider */
        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        mwb->vol_mute_button = gtk_button_new();
        gtk_button_set_relief(GTK_BUTTON(mwb->vol_mute_button), GTK_RELIEF_NONE);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->vol_mute_button), "mwb-volbutton");
        gtk_widget_set_tooltip_text(mwb->vol_mute_button, _("Mute"));
        mwb->vol_mute_icon = gtk_image_new_from_icon_name("audio-volume-high", GTK_ICON_SIZE_MENU);
        gtk_container_add(GTK_CONTAINER(mwb->vol_mute_button), mwb->vol_mute_icon);
        gtk_widget_show(mwb->vol_mute_icon);
        g_signal_connect(mwb->vol_mute_button, "clicked", G_CALLBACK(mwb_volume_mute_clicked), mwb);
        gtk_box_pack_start(GTK_BOX(row), mwb->vol_mute_button, FALSE, FALSE, 0);

        mwb->vol_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
        gtk_scale_set_draw_value(GTK_SCALE(mwb->vol_scale), FALSE);
        gtk_widget_set_size_request(mwb->vol_scale, 180, -1);
        gtk_box_pack_start(GTK_BOX(row), mwb->vol_scale, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(box), row, FALSE, FALSE, 0);

        /* mixer */
        GtkWidget *mixer = gtk_button_new_with_label(_("Mixer…"));
        gtk_button_set_relief(GTK_BUTTON(mixer), GTK_RELIEF_NONE);
        gtk_style_context_add_class(gtk_widget_get_style_context(mixer), "mwb-volbutton");
        g_signal_connect_swapped(mixer, "clicked", G_CALLBACK(mwb_launch), "pavucontrol");
        gtk_box_pack_start(GTK_BOX(box), mixer, FALSE, FALSE, 0);

        gtk_container_add(GTK_CONTAINER(mwb->volume_popup), box);

        g_signal_connect(mwb->vol_scale, "value-changed",
                         G_CALLBACK(mwb_volume_changed), mwb);
        g_signal_connect(mwb->volume_popup, "button-press-event",
                         G_CALLBACK(mwb_popup_button_press), mwb);
        g_signal_connect(mwb->volume_popup, "key-press-event",
                         G_CALLBACK(mwb_popup_key_press), mwb);
        g_signal_connect(mwb->volume_popup, "destroy",
                         G_CALLBACK(gtk_widget_destroyed), &mwb->volume_popup);
    }

    mwb_volume_get_percent(mwb);
    mwb_volume_get_muted(mwb);
    g_signal_handlers_block_by_func(mwb->vol_scale, mwb_volume_changed, mwb);
    gtk_range_set_value(GTK_RANGE(mwb->vol_scale), mwb->vol_percent);
    g_signal_handlers_unblock_by_func(mwb->vol_scale, mwb_volume_changed, mwb);
    mwb_volume_icon_update(mwb);

    mwb_popup_show(mwb, mwb->volume_popup, mwb->vol_button);
}

/* Wifi part — embed the xfce4-networkmanager panel plugin widget. */
static GtkWidget *
mwb_embed_networkmanager(MorphosWorkbenchPlugin *mwb)
{
    typedef XfcePanelPlugin *(*ModuleConstructFunc) (const gchar *, gint,
                                                      const gchar *, const gchar *,
                                                      gchar **, GdkScreen *);

    gchar *home_path = g_build_filename(g_get_home_dir(), ".local", "lib",
                                        "xfce4", "panel", "plugins",
                                        "libxfce4-networkmanager.so", NULL);
    const gchar *paths[] = {
        home_path,
        "/usr/local/lib/xfce4/panel/plugins/libxfce4-networkmanager.so",
        "/usr/local/lib64/xfce4/panel/plugins/libxfce4-networkmanager.so",
        "/usr/lib/xfce4/panel/plugins/libxfce4-networkmanager.so",
        "/usr/lib64/xfce4/panel/plugins/libxfce4-networkmanager.so",
        "libxfce4-networkmanager.so",
        NULL
    };
    guint i;

    for (i = 0; paths[i] != NULL; i++) {
        mwb->nm_module = g_module_open(paths[i],
                                       G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);
        if (mwb->nm_module != NULL)
            break;
    }
    g_free(home_path);

    if (mwb->nm_module == NULL) {
        g_warning("MorphOS Workbench: could not load xfce4-networkmanager plugin");
        return NULL;
    }

    ModuleConstructFunc construct = NULL;
    if (!g_module_symbol(mwb->nm_module, "xfce_panel_module_construct",
                         (gpointer *)&construct) || construct == NULL) {
        g_warning("MorphOS Workbench: xfce_panel_module_construct missing in "
                  "xfce4-networkmanager");
        g_module_close(mwb->nm_module);
        mwb->nm_module = NULL;
        return NULL;
    }

    XfcePanelPlugin *nm = construct("xfce4-networkmanager", 999999,
                                    "Network Manager",
                                    "Manage network connections",
                                    NULL, gdk_screen_get_default());
    if (nm == NULL) {
        g_warning("MorphOS Workbench: failed to construct xfce4-networkmanager");
        g_module_close(mwb->nm_module);
        mwb->nm_module = NULL;
        return NULL;
    }

    return GTK_WIDGET(nm);
}

/* Battery click — open power settings */
static void
mwb_batt_clicked(GtkButton *button G_GNUC_UNUSED, MorphosWorkbenchPlugin *mwb G_GNUC_UNUSED)
{
    mwb_launch("xfce4-power-manager-settings");
}

/* System info click — open hardinfo/xfce4-about */
static void
mwb_sys_clicked(GtkButton *button G_GNUC_UNUSED, MorphosWorkbenchPlugin *mwb G_GNUC_UNUSED)
{
    if (g_file_test("/usr/bin/hardinfo", G_FILE_TEST_EXISTS))
        mwb_launch("hardinfo");
    else
        mwb_launch("xfce4-about");
}

/* ------------------------------------------------------------------ *
 *  Timers
 *  ------------------------------------------------------------------ */

static gboolean
mwb_tick_clock(MorphosWorkbenchPlugin *mwb)
{
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    gchar buf[128];

    if (tm) {
        strftime(buf, sizeof(buf), "%H:%M", tm);
        gtk_label_set_text(GTK_LABEL(mwb->clock_label), buf);

        gchar tip[160];
        strftime(tip, sizeof(tip), "%A, %d %B %Y", tm);
        gtk_widget_set_tooltip_text(mwb->clock_button ? mwb->clock_button : mwb->clock_label, tip);
    }
    return G_SOURCE_CONTINUE;
}

static gboolean
mwb_tick_memory(MorphosWorkbenchPlugin *mwb)
{
    gchar *content = NULL;
    gsize len = 0;
    guint64 total = 0, available = 0;

    if (g_file_get_contents("/proc/meminfo", &content, &len, NULL) && content) {
        gchar *tok = content;
        while (tok && *tok) {
            gchar *line = strchr(tok, '\n');
            if (line)
                *line = '\0';
            if (g_str_has_prefix(tok, "MemTotal:")) {
                sscanf(tok + 9, "%" G_GUINT64_FORMAT, &total);
                total *= 1024;
            } else if (g_str_has_prefix(tok, "MemAvailable:")) {
                sscanf(tok + 13, "%" G_GUINT64_FORMAT, &available);
                available *= 1024;
            }
            tok = line ? line + 1 : NULL;
        }
        g_free(content);
    }

    if (total > 0 && available <= total) {
        gdouble frac = 1.0 - (gdouble)available / (gdouble)total;
        mwb_gauge_set(mwb->mem_gauge, frac);

        guint64 used = total - available;
        gchar *tip = g_strdup_printf(_("RAM used: %d%%\nUsed: %.1f GiB\nAvailable: %.1f GiB\nTotal: %.1f GiB"),
                                     (gint)(frac * 100.0),
                                     (gdouble)used / (1024.0 * 1024.0 * 1024.0),
                                     (gdouble)available / (1024.0 * 1024.0 * 1024.0),
                                     (gdouble)total / (1024.0 * 1024.0 * 1024.0));
        gtk_widget_set_tooltip_text(mwb->mem_gauge, tip);
        g_free(tip);
    }
    return G_SOURCE_CONTINUE;
}

static gboolean
mwb_tick_cpu(MorphosWorkbenchPlugin *mwb)
{
    gchar *content = NULL;
    gdouble total_frac = 0.0;

    if (g_file_get_contents("/proc/stat", &content, NULL, NULL) && content) {
        gchar *tok = content;
        gint core = -1;

        while (tok && *tok) {
            gchar *line = strchr(tok, '\n');
            if (line)
                *line = '\0';

            if (g_str_has_prefix(tok, "cpu")) {
                const gchar *stats = tok + 3;
                if (*stats == ' ' || *stats == '\0')
                    core = -1;
                else {
                    core = atoi(stats);
                    while (g_ascii_isdigit(*stats))
                        stats++;
                }

                guint64 user = 0, nice = 0, sys = 0, idle = 0, iowait = 0,
                        irq = 0, softirq = 0, steal = 0;
                if (core >= 0)
                    stats = tok + 4;
                else
                    stats = tok + 3;
                sscanf(stats,
                       "%" G_GUINT64_FORMAT " %" G_GUINT64_FORMAT " %" G_GUINT64_FORMAT
                       " %" G_GUINT64_FORMAT " %" G_GUINT64_FORMAT " %" G_GUINT64_FORMAT
                       " %" G_GUINT64_FORMAT " %" G_GUINT64_FORMAT,
                       &user, &nice, &sys, &idle, &iowait, &irq, &softirq, &steal);

                guint64 total = user + nice + sys + idle + iowait + irq + softirq + steal;
                guint64 idl   = idle + iowait;

                if (core == -1) {
                    if (mwb->cpu_prev_total > 0 && total >= mwb->cpu_prev_total) {
                        guint64 dtotal = total - mwb->cpu_prev_total;
                        guint64 didle  = idl - mwb->cpu_prev_idle;
                        if (dtotal > 0)
                            total_frac = CLAMP((gdouble)(dtotal - didle) / (gdouble)dtotal, 0.0, 1.0);
                    }
                    mwb->cpu_prev_total = total;
                    mwb->cpu_prev_idle = idl;
                    if (mwb->cpu_gauges[0])
                        mwb_gauge_set(mwb->cpu_gauges[0], total_frac);
                } else if (core < 16) {
                    if (mwb->cpu_prev_core_total[core] > 0 && total >= mwb->cpu_prev_core_total[core]) {
                        guint64 dtotal = total - mwb->cpu_prev_core_total[core];
                        guint64 didle  = idl - mwb->cpu_prev_core_idle[core];
                        if (dtotal > 0 && core < mwb->cpu_ncores && mwb->cpu_gauges[core]) {
                            gdouble f = CLAMP((gdouble)(dtotal - didle) / (gdouble)dtotal, 0.0, 1.0);
                            mwb_gauge_set(mwb->cpu_gauges[core], f);
                            gchar *tip = g_strdup_printf(_("CPU %d load: %d%%"),
                                                         core, (gint)(f * 100.0));
                            gtk_widget_set_tooltip_text(mwb->cpu_gauges[core], tip);
                            g_free(tip);
                        }
                    }
                    mwb->cpu_prev_core_total[core] = total;
                    mwb->cpu_prev_core_idle[core] = idl;
                }
            }
            tok = line ? line + 1 : NULL;
        }
        g_free(content);
    }

    if (mwb->cpu_ncores > 0 && mwb->cpu_gauges[0]) {
        mwb->cpu_load = total_frac;
        gchar *tip = g_strdup_printf(_("CPU load: %d%% (%d cores)"),
                                     (gint)(total_frac * 100.0), mwb->cpu_ncores);
        gtk_widget_set_tooltip_text(mwb->cpu_gauges[0], tip);
        g_free(tip);
    }
    return G_SOURCE_CONTINUE;
}

#define MWB_NET_HIGH_THRESHOLD (512 * 1024)  /* bytes/s: blue diode above this */

static void
mwb_net_lamp_update(MorphosWorkbenchPlugin *mwb, gint idx, gdouble rate)
{
    GtkWidget *lamp = mwb->net_lamps[idx];
    GtkStyleContext *ctx;

    if (!lamp)
        return;
    ctx = gtk_widget_get_style_context(lamp);
    gtk_style_context_remove_class(ctx, "active-net");
    gtk_style_context_remove_class(ctx, "active-net-high");
    if (rate > 0.0) {
        if (rate >= MWB_NET_HIGH_THRESHOLD)
            gtk_style_context_add_class(ctx, "active-net-high");
        else
            gtk_style_context_add_class(ctx, "active-net");
    }
}

static gboolean
mwb_tick_net(MorphosWorkbenchPlugin *mwb)
{
    gchar *content = NULL;
    guint64 tx = 0, rx = 0;
    gdouble elapsed = 1.0;
    gdouble tx_rate = 0.0, rx_rate = 0.0;
    gint64 now = g_get_monotonic_time();

    if (g_file_get_contents("/proc/net/dev", &content, NULL, NULL) && content) {
        gchar *tok = content;
        while (tok && *tok) {
            gchar *line = strchr(tok, '\n');
            if (line)
                *line = '\0';
            gchar *colon = strchr(tok, ':');
            if (colon) {
                guint64 if_rx = 0, if_tx = 0;
                if (sscanf(colon + 1,
                           "%" G_GUINT64_FORMAT " %*u %*u %*u %*u %*u %*u %*u"
                           " %" G_GUINT64_FORMAT,
                           &if_rx, &if_tx) >= 2) {
                    rx += if_rx;
                    tx += if_tx;
                }
            }
            tok = line ? line + 1 : NULL;
        }
        g_free(content);
    }

    if (mwb->net_prev_time > 0 && now > mwb->net_prev_time)
        elapsed = (gdouble)(now - mwb->net_prev_time) / 1000000.0;

    if (mwb->net_prev_tx > 0 && tx >= mwb->net_prev_tx)
        tx_rate = (gdouble)(tx - mwb->net_prev_tx) / elapsed;
    if (mwb->net_prev_rx > 0 && rx >= mwb->net_prev_rx)
        rx_rate = (gdouble)(rx - mwb->net_prev_rx) / elapsed;

    mwb_net_lamp_update(mwb, MWB_LAMP_NET_TX, tx_rate);
    mwb_net_lamp_update(mwb, MWB_LAMP_NET_RX, rx_rate);

    {
        gdouble rate = (tx_rate + rx_rate) / 1024.0;
        gchar *tip = g_strdup_printf(_("Network traffic: %.1f KiB/s"), rate);
        if (mwb->net_lamps[MWB_LAMP_NET_TX])
            gtk_widget_set_tooltip_text(mwb->net_lamps[MWB_LAMP_NET_TX], tip);
        if (mwb->net_lamps[MWB_LAMP_NET_RX])
            gtk_widget_set_tooltip_text(mwb->net_lamps[MWB_LAMP_NET_RX], tip);
        g_free(tip);
    }

    mwb->net_prev_tx = tx;
    mwb->net_prev_rx = rx;
    mwb->net_prev_time = now;
    return G_SOURCE_CONTINUE;
}

static gboolean
mwb_tick_disk(MorphosWorkbenchPlugin *mwb)
{
    gchar *content = NULL;
    guint64 sects[2] = { 0, 0 };
    gint n = 0;

    if (g_file_get_contents("/proc/diskstats", &content, NULL, NULL) && content) {
        gchar *tok = content;
        while (tok && *tok) {
            gchar *line = strchr(tok, '\n');
            if (line)
                *line = '\0';
            guint major = 0, minor = 0;
            gchar dev[64] = "";
            guint64 rsect = 0, wsect = 0;
            if (sscanf(tok, "%u %u %63s %*u %*u %" G_GUINT64_FORMAT
                       " %*u %*u %*u %" G_GUINT64_FORMAT,
                       &major, &minor, dev, &rsect, &wsect) >= 5) {
                if (major > 0 && n < 2)
                    sects[n++] = rsect + wsect;
            }
            tok = line ? line + 1 : NULL;
        }
        g_free(content);
    }

    guint i;
    for (i = 0; i < 2 && i < (guint)n; i++) {
        gboolean active = (mwb->disk_prev_sects[i] > 0 && sects[i] > mwb->disk_prev_sects[i]);
        if (mwb->disk_lamps[i]) {
            GtkStyleContext *ctx = gtk_widget_get_style_context(mwb->disk_lamps[i]);
            gtk_style_context_remove_class(ctx, "active-disk");
            if (active)
                gtk_style_context_add_class(ctx, "active-disk");
        }
        mwb->disk_prev_sects[i] = sects[i];
    }
    return G_SOURCE_CONTINUE;
}

/* Battery — reads /sys/class/power_supply/BAT0 */
static gboolean
mwb_tick_battery(MorphosWorkbenchPlugin *mwb)
{
    gchar *content = NULL;
    gint cap = -1;
    gchar status[32] = "";

    if (g_file_get_contents("/sys/class/power_supply/BAT0/capacity", &content, NULL, NULL) && content) {
        cap = atoi(content);
        g_free(content);
    }
    if (g_file_get_contents("/sys/class/power_supply/BAT0/status", &content, NULL, NULL) && content) {
        g_strstrip(content);
        g_strlcpy(status, content, sizeof(status));
        g_free(content);
    }

    if (cap < 0 || !mwb->batt_icon)
        return G_SOURCE_CONTINUE;

    const gchar *icon;
    gboolean charging = (g_strcmp0(status, "Charging") == 0);

    if (cap >= 80)          icon = charging ? "battery-full-charging-symbolic" : "battery-full-symbolic";
    else if (cap >= 60)     icon = charging ? "battery-good-charging-symbolic" : "battery-good-symbolic";
    else if (cap >= 40)     icon = charging ? "battery-good-charging-symbolic" : "battery-good-symbolic";
    else if (cap >= 20)     icon = charging ? "battery-low-charging-symbolic" : "battery-low-symbolic";
    else if (cap >= 5)      icon = charging ? "battery-caution-charging-symbolic" : "battery-caution-symbolic";
    else                    icon = charging ? "battery-empty-charging-symbolic" : "battery-empty-symbolic";

    gtk_image_set_from_icon_name(GTK_IMAGE(mwb->batt_icon), icon, GTK_ICON_SIZE_MENU);

    if (mwb->batt_label) {
        gchar *txt = g_strdup_printf("%d%%", cap);
        gtk_label_set_text(GTK_LABEL(mwb->batt_label), txt);
        g_free(txt);
    }

    gchar *tip = g_strdup_printf(_("Battery: %d%% (%s)"), cap, status[0] ? status : _("Unknown"));
    gtk_widget_set_tooltip_text(mwb->batt_button ? mwb->batt_button : mwb->batt_icon, tip);
    g_free(tip);
    return G_SOURCE_CONTINUE;
}

/* System info — update tooltip with CPU/mem/disk/uptime */
static gboolean
mwb_tick_sysinfo(MorphosWorkbenchPlugin *mwb)
{
    if (!mwb->sys_button)
        return G_SOURCE_CONTINUE;

    gchar *content = NULL;
    guint64 total = 0, available = 0;
    if (g_file_get_contents("/proc/meminfo", &content, NULL, NULL) && content) {
        gchar *tok = content;
        while (tok && *tok) {
            gchar *line = strchr(tok, '\n');
            if (line)
                *line = '\0';
            if (g_str_has_prefix(tok, "MemTotal:"))
                sscanf(tok + 9, "%" G_GUINT64_FORMAT, &total);
            else if (g_str_has_prefix(tok, "MemAvailable:"))
                sscanf(tok + 13, "%" G_GUINT64_FORMAT, &available);
            tok = line ? line + 1 : NULL;
        }
        g_free(content);
    }

    gchar *dfout = NULL;
    gdouble disk_used_pct = -1.0;
    if (g_spawn_command_line_sync("df -P /", &dfout, NULL, NULL, NULL) && dfout) {
        gchar *line = strchr(dfout, '\n');
        if (line) {
            guint64 blocks = 0, used = 0;
            if (sscanf(line + 1, "%*s %" G_GUINT64_FORMAT " %" G_GUINT64_FORMAT, &blocks, &used) >= 2)
                disk_used_pct = blocks > 0 ? (gdouble)used / (gdouble)blocks : 0.0;
        }
        g_free(dfout);
    }

    if (mwb->disk_gauge && disk_used_pct >= 0.0)
        mwb_gauge_set(mwb->disk_gauge, disk_used_pct);

    GError *err = NULL;
    gchar *uname_out = NULL;
    g_spawn_command_line_sync("uname -sr", &uname_out, NULL, NULL, &err);
    if (err) {
        g_error_free(err);
        uname_out = g_strdup(_("unknown"));
    }

    gchar *tip = g_strdup_printf(
        _("System\nKernel: %s\nCPU: %d%%\nMemory: %d%%\nDisk: %d%%\nUptime: see clock"),
        uname_out ? g_strstrip(uname_out) : "?",
        (gint)(mwb->cpu_load * 100.0),
        total > 0 ? (gint)(100 - available * 100 / total) : 0,
        disk_used_pct >= 0 ? (gint)(disk_used_pct * 100.0) : 0);
    gtk_widget_set_tooltip_text(mwb->sys_button, tip);
    g_free(tip);
    g_free(uname_out);
    return G_SOURCE_CONTINUE;
}

/* ------------------------------------------------------------------ *
 *  Screenbar (right side) — mirrors MorphOS Ambient screenbar:
 *  Drivelamps, Netlamps, Wifi, Battery, CPU, Mem, Disk, Volume, Sys, Time
 *  ------------------------------------------------------------------ */

/* A single round activity diode (Netlamp / Drivelamp). */
static GtkWidget *
mwb_lamp_new(const gchar *tooltip, const gchar *kind_class)
{
    GtkWidget *lamp = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(lamp), "mwb-lamp");
    if (kind_class != NULL)
        gtk_style_context_add_class(gtk_widget_get_style_context(lamp), kind_class);
    gtk_widget_set_valign(lamp, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(lamp, GTK_ALIGN_CENTER);
    if (tooltip != NULL)
        gtk_widget_set_tooltip_text(lamp, tooltip);
    return lamp;
}

/* Volume button (screenbar). Popup built lazily in mwb_volume_clicked(). */
static void
mwb_build_volume(MorphosWorkbenchPlugin *mwb, GtkWidget *status_group)
{
    mwb->vol_button = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(mwb->vol_button), GTK_RELIEF_NONE);
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->vol_button), "mwb-volbutton");
    gtk_widget_set_tooltip_text(mwb->vol_button, _("Volume"));
    mwb->vol_icon = gtk_image_new_from_icon_name("audio-volume-high", GTK_ICON_SIZE_MENU);
    gtk_container_add(GTK_CONTAINER(mwb->vol_button), mwb->vol_icon);
    gtk_widget_show(mwb->vol_icon);
    g_signal_connect(mwb->vol_button, "clicked", G_CALLBACK(mwb_volume_clicked), mwb);
    gtk_box_pack_start(GTK_BOX(status_group), mwb->vol_button, FALSE, FALSE, 0);
}

void
mwb_build_screenbar(MorphosWorkbenchPlugin *mwb)
{
    GtkWidget *right = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_style_context_add_class(gtk_widget_get_style_context(right), "mwb-hbox");
    mwb->screenbar = right;
    GtkWidget *status_group = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_style_context_add_class(gtk_widget_get_style_context(status_group), "mwb-status-group");

    /* ---- Drivelamps ---- */
    if (mwb->show_drivelamps) {
        GtkWidget *diskbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
        gtk_style_context_add_class(gtk_widget_get_style_context(diskbox), "mwb-island");
        guint d;
        for (d = 0; d < 2; d++) {
            mwb->disk_lamps[d] = mwb_lamp_new(_("Disk activity"), "disk");
            gtk_box_pack_start(GTK_BOX(diskbox), mwb->disk_lamps[d], FALSE, FALSE, 0);
        }
        gtk_box_pack_start(GTK_BOX(right), diskbox, FALSE, FALSE, 0);
    }

    gtk_box_pack_start(GTK_BOX(right), mwb_screenbar_divider(), FALSE, FALSE, 0);

    /* ---- Netlamps ---- */
    if (mwb->show_netlamps) {
        GtkWidget *netbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
        gtk_style_context_add_class(gtk_widget_get_style_context(netbox), "mwb-island");
        mwb->net_lamps[MWB_LAMP_NET_TX] = mwb_lamp_new(_("Network transmit"), "net");
        gtk_box_pack_start(GTK_BOX(netbox), mwb->net_lamps[MWB_LAMP_NET_TX], FALSE, FALSE, 0);

        mwb->net_lamps[MWB_LAMP_NET_RX] = mwb_lamp_new(_("Network receive"), "net");
        gtk_box_pack_start(GTK_BOX(netbox), mwb->net_lamps[MWB_LAMP_NET_RX], FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(right), netbox, FALSE, FALSE, 0);
    }

    gtk_box_pack_start(GTK_BOX(right), mwb_screenbar_divider(), FALSE, FALSE, 0);

    /* ---- Wi-Fi indicator (embedded xfce4-networkmanager) ---- */
    if (mwb->show_wifi) {
        mwb->nm_plugin = mwb_embed_networkmanager(mwb);
        if (mwb->nm_plugin != NULL) {
            gtk_box_pack_start(GTK_BOX(status_group), mwb->nm_plugin, FALSE, FALSE, 0);
            gtk_widget_show(mwb->nm_plugin);
        }
    }

    /* ---- Battery ---- */
    if (mwb->show_battery) {
        mwb->batt_button = gtk_button_new();
        gtk_button_set_relief(GTK_BUTTON(mwb->batt_button), GTK_RELIEF_NONE);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->batt_button), "mwb-volbutton");
        GtkWidget *batt_content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
        mwb->batt_icon = gtk_image_new_from_icon_name("battery-full-symbolic", GTK_ICON_SIZE_MENU);
        gtk_box_pack_start(GTK_BOX(batt_content), mwb->batt_icon, FALSE, FALSE, 0);
        gtk_widget_show(mwb->batt_icon);
        mwb->batt_label = gtk_label_new("");
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->batt_label), "mwb-screenbar-item");
        gtk_box_pack_start(GTK_BOX(batt_content), mwb->batt_label, FALSE, FALSE, 0);
        gtk_widget_show(mwb->batt_label);
        gtk_container_add(GTK_CONTAINER(mwb->batt_button), batt_content);
        gtk_widget_show(batt_content);
        g_signal_connect(mwb->batt_button, "clicked", G_CALLBACK(mwb_batt_clicked), mwb);
        gtk_box_pack_start(GTK_BOX(status_group), mwb->batt_button, FALSE, FALSE, 0);
    }

    gtk_box_pack_start(GTK_BOX(right), status_group, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(right), mwb_screenbar_divider(), FALSE, FALSE, 0);

    /* ---- CPU per-core vertical gauges ---- */
    if (mwb->show_cpumbar) {
        gint nc = sysconf(_SC_NPROCESSORS_ONLN);
        if (nc > 16)
            nc = 16;
        if (nc < 1)
            nc = 1;
        mwb->cpu_ncores = nc;

        GtkWidget *cpurow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
        mwb->cpu_gauges[0] = mwb_gauge_new(MWB_GAUGE_CPU, _("CPU"), mwb->theme, mwb->gauge_style);
        gtk_box_pack_start(GTK_BOX(cpurow), mwb->cpu_gauges[0], FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(right), cpurow, FALSE, FALSE, 0);
    }

    gtk_box_pack_start(GTK_BOX(right), mwb_screenbar_divider(), FALSE, FALSE, 0);

    /* ---- Memory vertical gauge ---- */
    if (mwb->show_membar) {
        GtkWidget *memrow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
        mwb->mem_gauge = mwb_gauge_new(MWB_GAUGE_MEM, _("RAM"), mwb->theme, mwb->gauge_style);
        gtk_box_pack_start(GTK_BOX(memrow), mwb->mem_gauge, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(right), memrow, FALSE, FALSE, 0);
    }

    /* ---- Disk vertical gauge ---- */
    if (mwb->show_diskgauge) {
        GtkWidget *diskrow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
        mwb->disk_gauge = mwb_gauge_new(MWB_GAUGE_DISK, _("DISK"), mwb->theme, mwb->gauge_style);
        gtk_box_pack_start(GTK_BOX(diskrow), mwb->disk_gauge, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(right), diskrow, FALSE, FALSE, 0);
    }

    gtk_box_pack_start(GTK_BOX(right), mwb_screenbar_divider(), FALSE, FALSE, 0);

    /* ---- System info ---- */
    if (mwb->show_sysinfo) {
        mwb->sys_button = gtk_button_new();
        gtk_button_set_relief(GTK_BUTTON(mwb->sys_button), GTK_RELIEF_NONE);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->sys_button), "mwb-volbutton");
        gtk_widget_set_tooltip_text(mwb->sys_button, _("System information"));
        GtkWidget *sys_img = gtk_image_new_from_icon_name("computer", GTK_ICON_SIZE_MENU);
        gtk_container_add(GTK_CONTAINER(mwb->sys_button), sys_img);
        gtk_widget_show(sys_img);
        g_signal_connect(mwb->sys_button, "clicked", G_CALLBACK(mwb_sys_clicked), mwb);
        gtk_box_pack_start(GTK_BOX(right), mwb->sys_button, FALSE, FALSE, 0);
    }

    /* ---- Volume ---- */
    if (mwb->show_volume)
        mwb_build_volume(mwb, status_group);

    /* ---- Clock (clickable -> calendar) ---- */
    if (mwb->show_clock) {
        mwb->clock_button = gtk_button_new();
        gtk_button_set_relief(GTK_BUTTON(mwb->clock_button), GTK_RELIEF_NONE);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->clock_button), "mwb-clockbtn");
        mwb->clock_label = gtk_label_new("");
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->clock_label), "mwb-clocks");
        gtk_container_add(GTK_CONTAINER(mwb->clock_button), mwb->clock_label);
        gtk_widget_show(mwb->clock_label);
        g_signal_connect(mwb->clock_button, "clicked", G_CALLBACK(mwb_clock_clicked), mwb);
        gtk_box_pack_start(GTK_BOX(right), mwb->clock_button, FALSE, FALSE, 0);
    }

    gtk_box_pack_end(GTK_BOX(mwb->bar), right, FALSE, FALSE, 0);
    gtk_widget_show_all(right);

    /* Timers */
    if (mwb->show_clock) {
        mwb_tick_clock(mwb);
        mwb->clock_timeout = g_timeout_add_seconds(1, (GSourceFunc)mwb_tick_clock, mwb);
    }
    if (mwb->show_membar) {
        mwb_tick_memory(mwb);
        mwb->mem_timeout = g_timeout_add_seconds(2, (GSourceFunc)mwb_tick_memory, mwb);
    }
    if (mwb->show_cpumbar) {
        mwb_tick_cpu(mwb);
        mwb->cpu_timeout = g_timeout_add_seconds(1, (GSourceFunc)mwb_tick_cpu, mwb);
    }
    if (mwb->show_netlamps) {
        mwb_tick_net(mwb);
        mwb->net_timeout = g_timeout_add(100, (GSourceFunc)mwb_tick_net, mwb);
    }
    if (mwb->show_drivelamps) {
        mwb_tick_disk(mwb);
        mwb->disk_timeout = g_timeout_add_seconds(1, (GSourceFunc)mwb_tick_disk, mwb);
    }
    if (mwb->show_battery) {
        mwb_tick_battery(mwb);
        mwb->batt_timeout = g_timeout_add_seconds(5, (GSourceFunc)mwb_tick_battery, mwb);
    }
    if (mwb->show_sysinfo || mwb->show_diskgauge) {
        mwb_tick_sysinfo(mwb);
        mwb->sys_timeout = g_timeout_add_seconds(5, (GSourceFunc)mwb_tick_sysinfo, mwb);
    }
}
