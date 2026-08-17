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
#include <math.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>

#ifdef HAVE_SQLITE3
#include <sqlite3.h>
#endif

static void
cairo_rounded_rectangle(cairo_t *cr, gdouble x, gdouble y, gdouble w, gdouble h, gdouble r)
{
    cairo_new_path(cr);
    if (r < 1.0) {
        cairo_rectangle(cr, x, y, w, h);
        return;
    }
    r = MIN(r, MIN(w / 2.0, h / 2.0));
    cairo_move_to(cr, x + r, y);
    cairo_line_to(cr, x + w - r, y);
    cairo_arc(cr, x + w - r, y + r, r, -G_PI_2, 0);
    cairo_line_to(cr, x + w, y + h - r);
    cairo_arc(cr, x + w - r, y + h - r, r, 0, G_PI_2);
    cairo_line_to(cr, x + r, y + h);
    cairo_arc(cr, x + r, y + h - r, r, G_PI_2, G_PI);
    cairo_line_to(cr, x, y + r);
    cairo_arc(cr, x + r, y + r, r, G_PI, -G_PI_2);
    cairo_close_path(cr);
}

/* ------------------------------------------------------------------ *
 *  Popup window helper (proper seat grab, outside-click dismiss)
 *  ------------------------------------------------------------------ */

static void
mwb_popup_hide(MorphosWorkbenchPlugin *mwb G_GNUC_UNUSED, GtkWidget *win)
{
    if (!win || !gtk_widget_get_visible(win))
        return;

    gtk_widget_hide(win);
}

static gboolean
mwb_popup_focus_out(GtkWidget *widget, GdkEventFocus *event G_GNUC_UNUSED, gpointer data)
{
    MorphosWorkbenchPlugin *mwb = data;
    mwb_popup_hide(mwb, widget);
    return FALSE;
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
mwb_popup_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data G_GNUC_UNUSED)
{
    GtkStyleContext *context = gtk_widget_get_style_context(widget);
    gint width = gtk_widget_get_allocated_width(widget);
    gint height = gtk_widget_get_allocated_height(widget);

    gtk_render_background(context, cr, 0, 0, width, height);
    gtk_render_frame(context, cr, 0, 0, width, height);

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

    if (mwb->calendar_popup && gtk_widget_get_visible(mwb->calendar_popup) && mwb->calendar_popup != win)
        mwb_popup_hide(mwb, mwb->calendar_popup);
    if (mwb->volume_popup && gtk_widget_get_visible(mwb->volume_popup) && mwb->volume_popup != win)
        mwb_popup_hide(mwb, mwb->volume_popup);
    if (mwb->batt_popup && gtk_widget_get_visible(mwb->batt_popup) && mwb->batt_popup != win)
        mwb_popup_hide(mwb, mwb->batt_popup);
    if (mwb->wifi_popup && gtk_widget_get_visible(mwb->wifi_popup) && mwb->wifi_popup != win)
        mwb_popup_hide(mwb, mwb->wifi_popup);
    if (mwb->notify_popup && gtk_widget_get_visible(mwb->notify_popup) && mwb->notify_popup != win)
        mwb_popup_hide(mwb, mwb->notify_popup);

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
    if (win == mwb->calendar_popup && mwb->calendar)
        gtk_widget_grab_focus(mwb->calendar);
    else if (win == mwb->volume_popup && mwb->vol_scale)
        gtk_widget_grab_focus(mwb->vol_scale);
}

/* ------------------------------------------------------------------ *
 *  Clock / calendar + volume popups (MorphOS Calendar & Volume)
 *  ------------------------------------------------------------------ */

static void
mwb_datetime_settings_clicked(GtkButton *button G_GNUC_UNUSED, MorphosWorkbenchPlugin *mwb)
{
    mwb_popup_hide(mwb, mwb->calendar_popup);
    mwb_launch("xfce4-datetime-settings || time-admin || gnome-control-center datetime");
}

static void
mwb_clock_clicked(GtkButton *button G_GNUC_UNUSED, MorphosWorkbenchPlugin *mwb)
{
    if (mwb->calendar_popup && gtk_widget_get_visible(mwb->calendar_popup)) {
        mwb_popup_hide(mwb, mwb->calendar_popup);
        return;
    }

    if (!mwb->calendar_popup) {
        mwb->calendar_popup = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        GdkScreen *screen = gdk_screen_get_default();
        GdkVisual *visual = screen ? gdk_screen_get_rgba_visual(screen) : NULL;
        if (visual) {
            gtk_widget_set_visual(mwb->calendar_popup, visual);
            gtk_widget_set_app_paintable(mwb->calendar_popup, TRUE);
        }
        gtk_window_set_decorated(GTK_WINDOW(mwb->calendar_popup), FALSE);
        gtk_window_set_skip_taskbar_hint(GTK_WINDOW(mwb->calendar_popup), TRUE);
        gtk_window_set_skip_pager_hint(GTK_WINDOW(mwb->calendar_popup), TRUE);
        gtk_window_set_type_hint(GTK_WINDOW(mwb->calendar_popup), GDK_WINDOW_TYPE_HINT_POPUP_MENU);
        gtk_window_set_resizable(GTK_WINDOW(mwb->calendar_popup), FALSE);
        gtk_widget_set_size_request(mwb->calendar_popup, 320, -1);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->calendar_popup), "mwb-popup");
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->calendar_popup), "mwb-vol-popup");
        gtk_widget_set_name(mwb->calendar_popup, "mwb-calendar");
        mwb_theme_widget(mwb, mwb->calendar_popup);

        GtkWidget *root_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_container_set_border_width(GTK_CONTAINER(root_box), 12);

        /* Header: Icon + Title + Live Time badge */
        GtkWidget *hdr = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        GtkWidget *hdr_icon = gtk_image_new_from_icon_name("x-office-calendar-symbolic", GTK_ICON_SIZE_MENU);
        gtk_style_context_add_class(gtk_widget_get_style_context(hdr_icon), "mwb-pop-icon");
        gtk_box_pack_start(GTK_BOX(hdr), hdr_icon, FALSE, FALSE, 0);

        GtkWidget *lbl = gtk_label_new(_("Date & Time"));
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl), "mwb-pop-title");
        gtk_box_pack_start(GTK_BOX(hdr), lbl, TRUE, TRUE, 0);

        time_t now = time(NULL);
        struct tm *tm = localtime(&now);
        gchar time_str[64] = "";
        if (tm)
            strftime(time_str, sizeof(time_str), "%H:%M:%S", tm);
        GtkWidget *badge = gtk_label_new(time_str);
        gtk_style_context_add_class(gtk_widget_get_style_context(badge), "mwb-vol-badge");
        gtk_box_pack_start(GTK_BOX(hdr), badge, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(root_box), hdr, FALSE, FALSE, 0);

        /* Card 1: Calendar Widget */
        GtkWidget *cal_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_style_context_add_class(gtk_widget_get_style_context(cal_card), "mwb-vol-card");
        gtk_container_set_border_width(GTK_CONTAINER(cal_card), 8);

        mwb->calendar = gtk_calendar_new();
        gtk_calendar_set_display_options(GTK_CALENDAR(mwb->calendar),
                                         GTK_CALENDAR_SHOW_HEADING |
                                         GTK_CALENDAR_SHOW_DAY_NAMES |
                                         GTK_CALENDAR_SHOW_WEEK_NUMBERS);
        gtk_box_pack_start(GTK_BOX(cal_card), mwb->calendar, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(root_box), cal_card, TRUE, TRUE, 0);

        /* Card 2: Date Info & Uptime */
        GtkWidget *info_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_style_context_add_class(gtk_widget_get_style_context(info_card), "mwb-vol-card");
        gtk_container_set_border_width(GTK_CONTAINER(info_card), 10);

        gchar date_full[128] = "";
        if (tm)
            strftime(date_full, sizeof(date_full), "%A, %B %e, %Y", tm);
        GtkWidget *date_lbl = gtk_label_new(date_full);
        gtk_label_set_xalign(GTK_LABEL(date_lbl), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(date_lbl), "mwb-pop-title");
        gtk_box_pack_start(GTK_BOX(info_card), date_lbl, FALSE, FALSE, 0);

        /* System uptime */
        FILE *f = fopen("/proc/uptime", "r");
        gdouble up_sec = 0.0;
        if (f) {
            if (fscanf(f, "%lf", &up_sec) == 1) {
                guint hrs = (guint)(up_sec / 3600);
                guint mins = (guint)(((guint)up_sec % 3600) / 60);
                gchar *up_txt = g_strdup_printf(_("System Uptime: %uh %02um"), hrs, mins);
                GtkWidget *up_lbl = gtk_label_new(up_txt);
                gtk_label_set_xalign(GTK_LABEL(up_lbl), 0.0);
                gtk_style_context_add_class(gtk_widget_get_style_context(up_lbl), "mwb-vol-title");
                gtk_box_pack_start(GTK_BOX(info_card), up_lbl, FALSE, FALSE, 0);
                g_free(up_txt);
            }
            fclose(f);
        }
        gtk_box_pack_start(GTK_BOX(root_box), info_card, FALSE, FALSE, 0);

        /* Quick Settings button */
        GtkWidget *btn = gtk_button_new_with_label(_("Date & Time Settings..."));
        gtk_style_context_add_class(gtk_widget_get_style_context(btn), "mwb-mixer-btn");
        g_signal_connect(btn, "clicked", G_CALLBACK(mwb_datetime_settings_clicked), mwb);
        gtk_box_pack_start(GTK_BOX(root_box), btn, FALSE, FALSE, 0);

        gtk_container_add(GTK_CONTAINER(mwb->calendar_popup), root_box);

        g_signal_connect(mwb->calendar_popup, "draw",
                         G_CALLBACK(mwb_popup_draw), NULL);
        g_signal_connect(mwb->calendar_popup, "button-press-event",
                         G_CALLBACK(mwb_popup_button_press), mwb);
        g_signal_connect(mwb->calendar_popup, "focus-out-event",
                         G_CALLBACK(mwb_popup_focus_out), mwb);
        g_signal_connect(mwb->calendar_popup, "key-press-event",
                         G_CALLBACK(mwb_popup_key_press), mwb);
        g_signal_connect(mwb->calendar_popup, "destroy",
                         G_CALLBACK(gtk_widget_destroyed), &mwb->calendar_popup);
    }

    /* select today's date */
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    if (tm && mwb->calendar) {
        gtk_calendar_select_month(GTK_CALENDAR(mwb->calendar), tm->tm_mon, 1900 + tm->tm_year);
        gtk_calendar_select_day(GTK_CALENDAR(mwb->calendar), tm->tm_mday);
        gtk_calendar_mark_day(GTK_CALENDAR(mwb->calendar), tm->tm_mday);
    }

    mwb_popup_show(mwb, mwb->calendar_popup, mwb->clock_button);
}

static void mwb_volume_icon_update(MorphosWorkbenchPlugin *mwb);
static void mwb_mic_refresh(MorphosWorkbenchPlugin *mwb);
static void mwb_mic_icon_update(MorphosWorkbenchPlugin *mwb);
static void mwb_mic_changed(GtkRange *range, MorphosWorkbenchPlugin *mwb);
static void mwb_volume_update_playing(MorphosWorkbenchPlugin *mwb);
static void mwb_streams_refresh(MorphosWorkbenchPlugin *mwb);
static gboolean mwb_tick_battery(MorphosWorkbenchPlugin *mwb);

static void
mwb_volume_parse_percent(MorphosWorkbenchPlugin *mwb, const gchar *out)
{
    guint sum = 0, n = 0;
    const gchar *p;

    for (p = out; (p = strchr(p, '%')) != NULL; p++) {
        const gchar *q = p;
        while (q > out && g_ascii_isdigit(*(q - 1)))
            q--;
        if (q < p) {
            sum += (guint)atoi(q);
            n++;
        }
    }
    if (n > 0)
        mwb->vol_percent = MIN(sum / n, 100u);
}

static void
mwb_volume_percent_cb(GObject *src, GAsyncResult *res, gpointer data)
{
    MorphosWorkbenchPlugin *mwb = data;
    GError *err = NULL;
    gchar *out = NULL;

    if (g_subprocess_communicate_utf8_finish(G_SUBPROCESS(src), res, &out, NULL, &err) && out) {
        mwb_volume_parse_percent(mwb, out);
        mwb_volume_icon_update(mwb);
        g_free(out);
    }
    if (err)
        g_error_free(err);
    g_object_unref(src);
}

static void
mwb_volume_mute_cb(GObject *src, GAsyncResult *res, gpointer data)
{
    MorphosWorkbenchPlugin *mwb = data;
    GError *err = NULL;
    gchar *out = NULL;

    if (g_subprocess_communicate_utf8_finish(G_SUBPROCESS(src), res, &out, NULL, &err) && out) {
        mwb->vol_muted = (strstr(out, "yes") != NULL);
        mwb_volume_icon_update(mwb);
        g_free(out);
    }
    if (err)
        g_error_free(err);
    g_object_unref(src);
}

static void
mwb_volume_refresh(MorphosWorkbenchPlugin *mwb)
{
    GError *err = NULL;
    GSubprocess *proc;

    proc = g_subprocess_new(G_SUBPROCESS_FLAGS_STDOUT_PIPE, &err,
                            "pactl", "get-sink-volume", "@DEFAULT_SINK@", NULL);
    if (proc)
        g_subprocess_communicate_utf8_async(proc, NULL, NULL, mwb_volume_percent_cb, mwb);
    else if (err) {
        g_error_free(err);
        err = NULL;
    }

    proc = g_subprocess_new(G_SUBPROCESS_FLAGS_STDOUT_PIPE, &err,
                            "pactl", "get-sink-mute", "@DEFAULT_SINK@", NULL);
    if (proc)
        g_subprocess_communicate_utf8_async(proc, NULL, NULL, mwb_volume_mute_cb, mwb);
    else if (err)
        g_error_free(err);
}

static gboolean
mwb_tick_volume(MorphosWorkbenchPlugin *mwb)
{
    mwb_volume_refresh(mwb);
    mwb_mic_refresh(mwb);
    mwb_volume_update_playing(mwb);
    mwb_streams_refresh(mwb);
    return G_SOURCE_CONTINUE;
}

/* ---- microphone ---- */

static void
mwb_mic_icon_update(MorphosWorkbenchPlugin *mwb)
{
    if (mwb->vol_mic_icon)
        gtk_image_set_from_icon_name(GTK_IMAGE(mwb->vol_mic_icon),
            mwb->vol_mic_muted ? "microphone-sensitivity-muted-symbolic"
                               : "audio-input-microphone-symbolic",
            GTK_ICON_SIZE_MENU);
}

static void
mwb_mic_mute_cb(GObject *src, GAsyncResult *res, gpointer data)
{
    MorphosWorkbenchPlugin *mwb = data;
    GError *err = NULL;
    gchar *out = NULL;

    if (g_subprocess_communicate_utf8_finish(G_SUBPROCESS(src), res, &out, NULL, &err) && out) {
        mwb->vol_mic_muted = (strstr(out, "yes") != NULL);
        mwb_mic_icon_update(mwb);
        g_free(out);
    }
    if (err)
        g_error_free(err);
    g_object_unref(src);
}

static void
mwb_mic_volume_cb(GObject *src, GAsyncResult *res, gpointer data)
{
    MorphosWorkbenchPlugin *mwb = data;
    GError *err = NULL;
    gchar *out = NULL;
    guint sum = 0, n = 0;
    const gchar *p;

    if (g_subprocess_communicate_utf8_finish(G_SUBPROCESS(src), res, &out, NULL, &err) && out) {
        for (p = out; (p = strchr(p, '%')) != NULL; p++) {
            const gchar *q = p;
            while (q > out && g_ascii_isdigit(*(q - 1)))
                q--;
            if (q < p) {
                sum += (guint)atoi(q);
                n++;
            }
        }
        if (n > 0)
            mwb->mic_percent = MIN(sum / n, 100u);
        if (mwb->vol_mic_scale) {
            g_signal_handlers_block_by_func(mwb->vol_mic_scale, mwb_mic_changed, mwb);
            gtk_range_set_value(GTK_RANGE(mwb->vol_mic_scale), mwb->mic_percent);
            g_signal_handlers_unblock_by_func(mwb->vol_mic_scale, mwb_mic_changed, mwb);
        }
        g_free(out);
    }
    if (err)
        g_error_free(err);
    g_object_unref(src);
}

static void
mwb_mic_refresh(MorphosWorkbenchPlugin *mwb)
{
    GError *err = NULL;
    GSubprocess *proc;

    proc = g_subprocess_new(G_SUBPROCESS_FLAGS_STDOUT_PIPE, &err,
                            "pactl", "get-source-mute", "@DEFAULT_SOURCE@", NULL);
    if (proc)
        g_subprocess_communicate_utf8_async(proc, NULL, NULL, mwb_mic_mute_cb, mwb);
    else if (err) {
        g_error_free(err);
        err = NULL;
    }

    proc = g_subprocess_new(G_SUBPROCESS_FLAGS_STDOUT_PIPE, &err,
                            "pactl", "get-source-volume", "@DEFAULT_SOURCE@", NULL);
    if (proc)
        g_subprocess_communicate_utf8_async(proc, NULL, NULL, mwb_mic_volume_cb, mwb);
    else if (err)
        g_error_free(err);
}

static void
mwb_mic_mute_clicked(GtkButton *button G_GNUC_UNUSED, MorphosWorkbenchPlugin *mwb)
{
    mwb_launch("pactl set-source-mute @DEFAULT_SOURCE@ toggle");
    mwb->vol_mic_muted = !mwb->vol_mic_muted;
    mwb_mic_icon_update(mwb);
}

static gboolean
mwb_mic_apply_debounced(gpointer data)
{
    MorphosWorkbenchPlugin *mwb = data;
    mwb->vol_mic_debounce_id = 0;
    gchar *cmd = g_strdup_printf("pactl set-source-volume @DEFAULT_SOURCE@ %u%%", mwb->mic_percent);
    mwb_launch(cmd);
    g_free(cmd);
    return G_SOURCE_REMOVE;
}

static void
mwb_mic_changed(GtkRange *range, MorphosWorkbenchPlugin *mwb)
{
    guint pct = (guint)gtk_range_get_value(range);
    mwb->mic_percent = MIN(pct, 100u);
    mwb->vol_mic_muted = FALSE;
    mwb_mic_icon_update(mwb);

    if (mwb->vol_mic_debounce_id)
        g_source_remove(mwb->vol_mic_debounce_id);
    mwb->vol_mic_debounce_id = g_timeout_add(50, mwb_mic_apply_debounced, mwb);
}

/* ---- now playing (MPRIS over D-Bus) ---- */

typedef struct {
    gchar    *bus_name;
    gchar    *player_name;
    gchar    *status;
    gchar    *title;
    gchar    *artist;
    gchar    *album;
    gchar    *art_url;
} MwbMediaInfo;

static void
mwb_media_info_free(MwbMediaInfo *info)
{
    if (!info)
        return;
    g_free(info->bus_name);
    g_free(info->player_name);
    g_free(info->status);
    g_free(info->title);
    g_free(info->artist);
    g_free(info->album);
    g_free(info->art_url);
    g_free(info);
}

static MwbMediaInfo *
mwb_media_info(void)
{
    GDBusConnection *conn;
    GVariant *result, *names_v;
    gchar **names = NULL;
    MwbMediaInfo *best_info = NULL;
    guint i;

    conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
    if (!conn)
        return NULL;

    result = g_dbus_connection_call_sync(conn,
        "org.freedesktop.DBus", "/org/freedesktop/DBus",
        "org.freedesktop.DBus", "ListNames",
        NULL, G_VARIANT_TYPE("(as)"), G_DBUS_CALL_FLAGS_NONE, 150, NULL, NULL);
    if (result) {
        names_v = g_variant_get_child_value(result, 0);
        names = g_variant_dup_strv(names_v, NULL);
        g_variant_unref(names_v);
        g_variant_unref(result);
    }

    if (!names) {
        g_object_unref(conn);
        return NULL;
    }

    for (i = 0; names[i]; i++) {
        if (!g_str_has_prefix(names[i], "org.mpris.MediaPlayer2.") ||
            g_strcmp0(names[i], "org.mpris.MediaPlayer2") == 0)
            continue;

        const gchar *player_bus = names[i];

        /* Get Identity */
        gchar *player_name = NULL;
        GVariant *id_v = g_dbus_connection_call_sync(conn, player_bus, "/org/mpris/MediaPlayer2",
            "org.freedesktop.DBus.Properties", "Get",
            g_variant_new("(ss)", "org.mpris.MediaPlayer2", "Identity"),
            G_VARIANT_TYPE("(v)"), G_DBUS_CALL_FLAGS_NONE, 150, NULL, NULL);
        if (id_v) {
            GVariant *v = g_variant_get_child_value(id_v, 0);
            if (g_variant_is_of_type(v, G_VARIANT_TYPE_STRING))
                player_name = g_variant_dup_string(v, NULL);
            g_variant_unref(v);
            g_variant_unref(id_v);
        }
        if (!player_name) {
            const gchar *dot = strrchr(player_bus, '.');
            if (dot && *(dot + 1)) {
                player_name = g_strdup(dot + 1);
                player_name[0] = g_ascii_toupper(player_name[0]);
            } else {
                player_name = g_strdup("Media Player");
            }
        }

        /* Get PlaybackStatus */
        gchar *status = NULL;
        GVariant *status_v = g_dbus_connection_call_sync(conn, player_bus, "/org/mpris/MediaPlayer2",
            "org.freedesktop.DBus.Properties", "Get",
            g_variant_new("(ss)", "org.mpris.MediaPlayer2.Player", "PlaybackStatus"),
            G_VARIANT_TYPE("(v)"), G_DBUS_CALL_FLAGS_NONE, 150, NULL, NULL);
        if (status_v) {
            GVariant *v = g_variant_get_child_value(status_v, 0);
            if (g_variant_is_of_type(v, G_VARIANT_TYPE_STRING))
                status = g_variant_dup_string(v, NULL);
            g_variant_unref(v);
            g_variant_unref(status_v);
        }
        if (!status)
            status = g_strdup("Stopped");

        /* Get Metadata */
        GVariant *meta = g_dbus_connection_call_sync(conn, player_bus, "/org/mpris/MediaPlayer2",
            "org.freedesktop.DBus.Properties", "Get",
            g_variant_new("(ss)", "org.mpris.MediaPlayer2.Player", "Metadata"),
            G_VARIANT_TYPE("(v)"), G_DBUS_CALL_FLAGS_NONE, 150, NULL, NULL);

        MwbMediaInfo *info = NULL;
        if (meta) {
            GVariant *md = g_variant_get_child_value(meta, 0);
            if (g_variant_is_of_type(md, G_VARIANT_TYPE("a{sv}"))) {
                info = g_new0(MwbMediaInfo, 1);
                info->bus_name = g_strdup(player_bus);
                info->player_name = player_name;
                player_name = NULL;
                info->status = status;
                status = NULL;

                GVariantIter iter;
                const gchar *key;
                GVariant *val;
                g_variant_iter_init(&iter, md);
                while (g_variant_iter_next(&iter, "{sv}", &key, &val)) {
                    if (g_strcmp0(key, "xesam:title") == 0 &&
                        g_variant_is_of_type(val, G_VARIANT_TYPE_STRING)) {
                        info->title = g_variant_dup_string(val, NULL);
                    } else if (g_strcmp0(key, "xesam:artist") == 0) {
                        if (g_variant_is_of_type(val, G_VARIANT_TYPE_STRING_ARRAY)) {
                            gsize n_artists = 0;
                            const gchar **artists = g_variant_get_strv(val, &n_artists);
                            if (artists && n_artists > 0)
                                info->artist = g_strjoinv(", ", (gchar **)artists);
                            g_free(artists);
                        } else if (g_variant_is_of_type(val, G_VARIANT_TYPE_STRING)) {
                            info->artist = g_variant_dup_string(val, NULL);
                        }
                    } else if (g_strcmp0(key, "xesam:album") == 0 &&
                               g_variant_is_of_type(val, G_VARIANT_TYPE_STRING)) {
                        info->album = g_variant_dup_string(val, NULL);
                    } else if (g_strcmp0(key, "mpris:artUrl") == 0 &&
                               g_variant_is_of_type(val, G_VARIANT_TYPE_STRING)) {
                        info->art_url = g_variant_dup_string(val, NULL);
                    }
                    g_variant_unref(val);
                }

                if (!info->title || !*info->title) {
                    mwb_media_info_free(info);
                    info = NULL;
                }
            }
            g_variant_unref(md);
            g_variant_unref(meta);
        }

        g_free(player_name);
        g_free(status);

        if (info) {
            if (g_ascii_strcasecmp(info->status, "Playing") == 0) {
                if (best_info)
                    mwb_media_info_free(best_info);
                best_info = info;
                break;
            } else if (!best_info) {
                best_info = info;
            } else if (g_ascii_strcasecmp(best_info->status, "Playing") != 0 &&
                       g_ascii_strcasecmp(info->status, "Paused") == 0) {
                mwb_media_info_free(best_info);
                best_info = info;
            } else {
                mwb_media_info_free(info);
            }
        }
    }

    g_strfreev(names);
    g_object_unref(conn);
    return best_info;
}

static void
mwb_media_control(MorphosWorkbenchPlugin *mwb, const gchar *method)
{
    if (!mwb->vol_playing_bus || !*mwb->vol_playing_bus)
        return;

    GDBusConnection *conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
    if (!conn)
        return;

    g_dbus_connection_call(conn, mwb->vol_playing_bus, "/org/mpris/MediaPlayer2",
                           "org.mpris.MediaPlayer2.Player", method,
                           NULL, NULL, G_DBUS_CALL_FLAGS_NONE, 500, NULL, NULL, NULL);
    g_object_unref(conn);

    mwb_volume_update_playing(mwb);
}

static void
mwb_media_prev_clicked(GtkButton *btn G_GNUC_UNUSED, MorphosWorkbenchPlugin *mwb)
{
    mwb_media_control(mwb, "Previous");
}

static void
mwb_media_play_clicked(GtkButton *btn G_GNUC_UNUSED, MorphosWorkbenchPlugin *mwb)
{
    mwb_media_control(mwb, "PlayPause");
}

static void
mwb_media_next_clicked(GtkButton *btn G_GNUC_UNUSED, MorphosWorkbenchPlugin *mwb)
{
    mwb_media_control(mwb, "Next");
}

static void
mwb_playing_set_art(MorphosWorkbenchPlugin *mwb, const gchar *art_url)
{
    if (!mwb->vol_playing_art)
        return;

    gchar *path = NULL;
    if (art_url && *art_url) {
        if (g_str_has_prefix(art_url, "file://"))
            path = g_filename_from_uri(art_url, NULL, NULL);
        else if (art_url[0] == '/')
            path = g_strdup(art_url);
    }

    if (path) {
        GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_size(path, 64, 64, NULL);
        if (pb) {
            gtk_image_set_from_pixbuf(GTK_IMAGE(mwb->vol_playing_art), pb);
            g_object_unref(pb);
            g_free(path);
            return;
        }
        g_free(path);
    }

    gtk_image_set_from_icon_name(GTK_IMAGE(mwb->vol_playing_art),
                                 "audio-x-generic", GTK_ICON_SIZE_DIALOG);
    gtk_image_set_pixel_size(GTK_IMAGE(mwb->vol_playing_art), 56);
}

static void
mwb_volume_update_playing(MorphosWorkbenchPlugin *mwb)
{
    MwbMediaInfo *info;

    if (!mwb->vol_playing_card)
        return;

    info = mwb_media_info();
    if (info) {
        g_free(mwb->vol_playing_bus);
        mwb->vol_playing_bus = g_strdup(info->bus_name);

        if (mwb->vol_playing_title) {
            gtk_label_set_text(GTK_LABEL(mwb->vol_playing_title), info->title);
            gtk_widget_set_tooltip_text(mwb->vol_playing_title, info->title);
        }

        if (mwb->vol_playing_artist) {
            if (info->artist && *info->artist) {
                if (info->album && *info->album) {
                    gchar *line = g_strdup_printf("%s • %s", info->artist, info->album);
                    gtk_label_set_text(GTK_LABEL(mwb->vol_playing_artist), line);
                    gtk_widget_set_tooltip_text(mwb->vol_playing_artist, line);
                    g_free(line);
                } else {
                    gtk_label_set_text(GTK_LABEL(mwb->vol_playing_artist), info->artist);
                    gtk_widget_set_tooltip_text(mwb->vol_playing_artist, info->artist);
                }
            } else if (info->album && *info->album) {
                gtk_label_set_text(GTK_LABEL(mwb->vol_playing_artist), info->album);
                gtk_widget_set_tooltip_text(mwb->vol_playing_artist, info->album);
            } else {
                gtk_label_set_text(GTK_LABEL(mwb->vol_playing_artist), _("Unknown artist"));
                gtk_widget_set_tooltip_text(mwb->vol_playing_artist, NULL);
            }
        }

        if (mwb->vol_playing_status) {
            gboolean is_playing = (g_ascii_strcasecmp(info->status, "Playing") == 0);
            gchar *badge_txt = g_strdup_printf("%s%s%s",
                info->player_name ? info->player_name : _("Media"),
                info->player_name ? " • " : "",
                is_playing ? _("Playing") : _("Paused"));
            gtk_label_set_text(GTK_LABEL(mwb->vol_playing_status), badge_txt);
            g_free(badge_txt);

            GtkStyleContext *bctx = gtk_widget_get_style_context(mwb->vol_playing_status);
            if (is_playing) {
                gtk_style_context_remove_class(bctx, "mwb-badge-paused");
                gtk_style_context_add_class(bctx, "mwb-badge-playing");
            } else {
                gtk_style_context_remove_class(bctx, "mwb-badge-playing");
                gtk_style_context_add_class(bctx, "mwb-badge-paused");
            }
            gtk_widget_show(mwb->vol_playing_status);
        }

        if (mwb->vol_playing_play_icon) {
            gboolean is_playing = (g_ascii_strcasecmp(info->status, "Playing") == 0);
            gtk_image_set_from_icon_name(GTK_IMAGE(mwb->vol_playing_play_icon),
                is_playing ? "media-playback-pause-symbolic" : "media-playback-start-symbolic",
                GTK_ICON_SIZE_MENU);
        }

        if (mwb->vol_playing_controls)
            gtk_widget_show(mwb->vol_playing_controls);

        mwb_playing_set_art(mwb, info->art_url);
        mwb_media_info_free(info);
    } else {
        g_free(mwb->vol_playing_bus);
        mwb->vol_playing_bus = NULL;

        if (mwb->vol_playing_title) {
            gtk_label_set_text(GTK_LABEL(mwb->vol_playing_title), _("No media playing"));
            gtk_widget_set_tooltip_text(mwb->vol_playing_title, NULL);
        }
        if (mwb->vol_playing_artist) {
            gtk_label_set_text(GTK_LABEL(mwb->vol_playing_artist), _("Play audio in any media player or browser"));
            gtk_widget_set_tooltip_text(mwb->vol_playing_artist, NULL);
        }
        if (mwb->vol_playing_status)
            gtk_widget_hide(mwb->vol_playing_status);
        if (mwb->vol_playing_controls)
            gtk_widget_hide(mwb->vol_playing_controls);

        mwb_playing_set_art(mwb, NULL);
    }
}

/* ---- active playback streams (per-app volume) ---- */

typedef struct {
    guint  index;
    gchar *app;
    gchar *media;
    guint  volume;
} MwbStream;

static void
mwb_stream_free(MwbStream *s)
{
    if (!s)
        return;
    g_free(s->app);
    g_free(s->media);
    g_free(s);
}

static void
mwb_stream_add(GList **list, guint index, const gchar *app, const gchar *media, guint volume)
{
    MwbStream *s = g_new0(MwbStream, 1);
    s->index = index;
    s->app = g_strdup(app && *app ? app : _("Unknown"));
    s->media = g_strdup(media);
    s->volume = volume;
    *list = g_list_append(*list, s);
}

static gchar *
mwb_stream_prop(const gchar *line)
{
    gchar *p = strchr(line, '=');
    gchar *val;
    if (!p)
        return NULL;
    val = g_strdup(p + 1);
    g_strstrip(val);
    if (val[0] == '"') {
        gchar *q = val + 1;
        gchar *end = strchr(q, '"');
        if (end)
            *end = '\0';
        memmove(val, q, strlen(q) + 1);
    }
    return val;
}

static GList *
mwb_streams_parse(const gchar *out)
{
    GList *streams = NULL;
    gchar **lines = g_strsplit(out, "\n", -1);
    guint index = 0, volume = 0;
    gchar *app = NULL, *media = NULL;
    gboolean have = FALSE;
    gint i;

    for (i = 0; lines[i]; i++) {
        gchar *trimmed = g_strdup(lines[i]);
        g_strstrip(trimmed);

        if (g_str_has_prefix(trimmed, "Sink Input #")) {
            if (have)
                mwb_stream_add(&streams, index, app, media, volume);
            have = TRUE;
            index = (guint)atoi(trimmed + strlen("Sink Input #"));
            volume = 0;
            g_free(app); app = NULL;
            g_free(media); media = NULL;
        } else if (g_str_has_prefix(trimmed, "application.name")) {
            g_free(app);
            app = mwb_stream_prop(trimmed);
        } else if (g_str_has_prefix(trimmed, "media.name")) {
            g_free(media);
            media = mwb_stream_prop(trimmed);
        } else if (g_str_has_prefix(trimmed, "Volume:")) {
            gchar *p = strchr(trimmed, '%');
            if (p) {
                gchar *q = p;
                while (q > trimmed && g_ascii_isdigit(*(q - 1)))
                    q--;
                volume = (guint)atoi(q);
            }
        }

        g_free(trimmed);
    }
    if (have)
        mwb_stream_add(&streams, index, app, media, volume);

    g_free(app);
    g_free(media);
    g_strfreev(lines);
    return streams;
}

static void
mwb_stream_volume_changed(GtkRange *range, MorphosWorkbenchPlugin *mwb G_GNUC_UNUSED)
{
    guint index = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(range), "mwb-stream-index"));
    guint pct = (guint)gtk_range_get_value(range);
    gchar *cmd = g_strdup_printf("pactl set-sink-input-volume %u %u%%", index, pct);
    mwb_launch(cmd);
    g_free(cmd);
}

static const gchar *
mwb_stream_app_icon(const gchar *app_name)
{
    if (!app_name || !*app_name)
        return "applications-multimedia-symbolic";
    gchar *lower = g_ascii_strdown(app_name, -1);
    const gchar *icon = "audio-card-symbolic";
    if (strstr(lower, "firefox"))        icon = "firefox";
    else if (strstr(lower, "chrome"))    icon = "google-chrome";
    else if (strstr(lower, "chromium"))  icon = "chromium";
    else if (strstr(lower, "spotify"))   icon = "spotify";
    else if (strstr(lower, "vlc"))       icon = "vlc";
    else if (strstr(lower, "mpv"))       icon = "mpv";
    else if (strstr(lower, "rhythmbox")) icon = "rhythmbox";
    else if (strstr(lower, "audacious")) icon = "audacious";
    else if (strstr(lower, "discord"))   icon = "discord";
    else if (strstr(lower, "telegram"))  icon = "telegram";
    else if (strstr(lower, "steam"))     icon = "steam";
    else                                 icon = "applications-multimedia-symbolic";
    g_free(lower);
    return icon;
}

static void
mwb_streams_rebuild(MorphosWorkbenchPlugin *mwb, GList *streams)
{
    GList *children, *c;

    if (!mwb->vol_streams_box)
        return;

    children = gtk_container_get_children(GTK_CONTAINER(mwb->vol_streams_box));
    for (c = children; c; c = c->next)
        gtk_widget_destroy(GTK_WIDGET(c->data));
    g_list_free(children);

    for (GList *l = streams; l; l = l->next) {
        MwbStream *s = l->data;
        GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_style_context_add_class(gtk_widget_get_style_context(card), "mwb-vol-card");
        gtk_container_set_border_width(GTK_CONTAINER(card), 8);

        /* Top row: App icon + App Name + Media Name + Vol % badge */
        GtkWidget *top_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        const gchar *icon_name = mwb_stream_app_icon(s->app);
        GtkWidget *icon = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_MENU);
        gtk_style_context_add_class(gtk_widget_get_style_context(icon), "mwb-pop-icon");
        gtk_box_pack_start(GTK_BOX(top_row), icon, FALSE, FALSE, 0);

        GtkWidget *title_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
        GtkWidget *app_lbl = gtk_label_new(s->app && *s->app ? s->app : _("Application"));
        gtk_label_set_xalign(GTK_LABEL(app_lbl), 0.0);
        gtk_label_set_ellipsize(GTK_LABEL(app_lbl), PANGO_ELLIPSIZE_END);
        gtk_style_context_add_class(gtk_widget_get_style_context(app_lbl), "mwb-pop-title");
        gtk_box_pack_start(GTK_BOX(title_box), app_lbl, FALSE, FALSE, 0);

        if (s->media && *s->media && g_strcmp0(s->media, s->app) != 0) {
            GtkWidget *med_lbl = gtk_label_new(s->media);
            gtk_label_set_xalign(GTK_LABEL(med_lbl), 0.0);
            gtk_label_set_ellipsize(GTK_LABEL(med_lbl), PANGO_ELLIPSIZE_END);
            gtk_style_context_add_class(gtk_widget_get_style_context(med_lbl), "mwb-vol-title");
            gtk_box_pack_start(GTK_BOX(title_box), med_lbl, FALSE, FALSE, 0);
        }
        gtk_box_pack_start(GTK_BOX(top_row), title_box, TRUE, TRUE, 0);

        gchar *vol_str = g_strdup_printf("%u%%", s->volume);
        GtkWidget *vol_badge = gtk_label_new(vol_str);
        gtk_style_context_add_class(gtk_widget_get_style_context(vol_badge), "mwb-vol-badge");
        gtk_box_pack_start(GTK_BOX(top_row), vol_badge, FALSE, FALSE, 0);
        g_free(vol_str);

        gtk_box_pack_start(GTK_BOX(card), top_row, FALSE, FALSE, 0);

        /* Bottom row: Volume Scale */
        GtkWidget *scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
        gtk_scale_set_draw_value(GTK_SCALE(scale), FALSE);
        gtk_style_context_add_class(gtk_widget_get_style_context(scale), "mwb-vol-scale");
        g_object_set_data(G_OBJECT(scale), "mwb-stream-index", GUINT_TO_POINTER(s->index));
        g_signal_connect(scale, "value-changed", G_CALLBACK(mwb_stream_volume_changed), mwb);
        g_signal_handlers_block_by_func(scale, mwb_stream_volume_changed, mwb);
        gtk_range_set_value(GTK_RANGE(scale), s->volume);
        g_signal_handlers_unblock_by_func(scale, mwb_stream_volume_changed, mwb);
        gtk_box_pack_start(GTK_BOX(card), scale, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(mwb->vol_streams_box), card, FALSE, FALSE, 0);
        gtk_widget_show_all(card);
    }

    if (!streams) {
        GtkWidget *lbl = gtk_label_new(_("No active playback streams"));
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.5);
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl), "mwb-stream-empty");
        gtk_box_pack_start(GTK_BOX(mwb->vol_streams_box), lbl, FALSE, FALSE, 0);
        gtk_widget_show_all(lbl);
    }
}

static void
mwb_streams_cb(GObject *src, GAsyncResult *res, gpointer data)
{
    MorphosWorkbenchPlugin *mwb = data;
    GError *err = NULL;
    gchar *out = NULL;

    if (g_subprocess_communicate_utf8_finish(G_SUBPROCESS(src), res, &out, NULL, &err) && out) {
        GList *streams = mwb_streams_parse(out);
        mwb_streams_rebuild(mwb, streams);
        g_list_free_full(streams, (GDestroyNotify)mwb_stream_free);
        g_free(out);
    }
    if (err)
        g_error_free(err);
    g_object_unref(src);
}

static void
mwb_streams_refresh(MorphosWorkbenchPlugin *mwb)
{
    GError *err = NULL;
    GSubprocess *proc = g_subprocess_new(G_SUBPROCESS_FLAGS_STDOUT_PIPE, &err,
                                        "pactl", "list", "sink-inputs", NULL);
    if (proc)
        g_subprocess_communicate_utf8_async(proc, NULL, NULL, mwb_streams_cb, mwb);
    else if (err)
        g_error_free(err);
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
    if (mwb->vol_mute_button)
        gtk_widget_set_tooltip_text(mwb->vol_mute_button, mwb->vol_muted ? _("Unmute Output") : _("Mute Output"));
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

static gboolean
mwb_volume_apply_debounced(gpointer data)
{
    MorphosWorkbenchPlugin *mwb = data;
    mwb->vol_debounce_id = 0;
    gchar *cmd = g_strdup_printf("pactl set-sink-volume @DEFAULT_SINK@ %u%%", mwb->vol_percent);
    mwb_launch(cmd);
    g_free(cmd);
    return G_SOURCE_REMOVE;
}

static void
mwb_volume_changed(GtkRange *range, MorphosWorkbenchPlugin *mwb)
{
    guint pct = (guint)gtk_range_get_value(range);
    mwb->vol_percent = MIN(pct, 100u);
    mwb->vol_muted = FALSE;
    mwb_volume_icon_update(mwb);

    if (mwb->vol_debounce_id)
        g_source_remove(mwb->vol_debounce_id);
    mwb->vol_debounce_id = g_timeout_add(50, mwb_volume_apply_debounced, mwb);
}

static void
mwb_volume_popup_destroyed(GtkWidget *widget G_GNUC_UNUSED, MorphosWorkbenchPlugin *mwb)
{
    mwb->volume_popup = NULL;
    mwb->vol_scale = NULL;
    mwb->vol_mute_button = NULL;
    mwb->vol_mute_icon = NULL;
    mwb->vol_percent_label = NULL;
    mwb->vol_mic_button = NULL;
    mwb->vol_mic_icon = NULL;
    mwb->vol_mic_scale = NULL;
    mwb->vol_playing_card = NULL;
    mwb->vol_playing_art = NULL;
    mwb->vol_playing_title = NULL;
    mwb->vol_playing_artist = NULL;
    mwb->vol_playing_status = NULL;
    mwb->vol_playing_controls = NULL;
    mwb->vol_playing_prev_btn = NULL;
    mwb->vol_playing_play_btn = NULL;
    mwb->vol_playing_next_btn = NULL;
    mwb->vol_playing_play_icon = NULL;
    mwb->vol_streams_box = NULL;
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
        GdkScreen *screen = gdk_screen_get_default();
        GdkVisual *visual = screen ? gdk_screen_get_rgba_visual(screen) : NULL;
        if (visual) {
            gtk_widget_set_visual(mwb->volume_popup, visual);
            gtk_widget_set_app_paintable(mwb->volume_popup, TRUE);
        }
        gtk_window_set_decorated(GTK_WINDOW(mwb->volume_popup), FALSE);
        gtk_window_set_skip_taskbar_hint(GTK_WINDOW(mwb->volume_popup), TRUE);
        gtk_window_set_skip_pager_hint(GTK_WINDOW(mwb->volume_popup), TRUE);
        gtk_window_set_type_hint(GTK_WINDOW(mwb->volume_popup), GDK_WINDOW_TYPE_HINT_POPUP_MENU);
        gtk_window_set_resizable(GTK_WINDOW(mwb->volume_popup), FALSE);
        gtk_widget_set_size_request(mwb->volume_popup, 340, -1);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->volume_popup), "mwb-popup");
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->volume_popup), "mwb-vol-popup");
        gtk_widget_set_name(mwb->volume_popup, "mwb-volume-pop");
        mwb_theme_widget(mwb, mwb->volume_popup);

        GtkWidget *root_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_container_set_border_width(GTK_CONTAINER(root_box), 12);

        /* Header: Icon + Title + Master Volume Badge */
        GtkWidget *hdr = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        GtkWidget *hdr_icon = gtk_image_new_from_icon_name("audio-speakers-symbolic", GTK_ICON_SIZE_MENU);
        gtk_style_context_add_class(gtk_widget_get_style_context(hdr_icon), "mwb-pop-icon");
        gtk_box_pack_start(GTK_BOX(hdr), hdr_icon, FALSE, FALSE, 0);

        GtkWidget *lbl = gtk_label_new(_("Sound & Media"));
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl), "mwb-pop-title");
        gtk_box_pack_start(GTK_BOX(hdr), lbl, TRUE, TRUE, 0);

        mwb->vol_percent_label = gtk_label_new("0%");
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->vol_percent_label), "mwb-badge");
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->vol_percent_label), "mwb-badge-vol");
        gtk_box_pack_start(GTK_BOX(hdr), mwb->vol_percent_label, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(root_box), hdr, FALSE, FALSE, 0);

        /* Volume & Mic Control Card */
        GtkWidget *vol_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_style_context_add_class(gtk_widget_get_style_context(vol_card), "mwb-vol-card");

        /* Master output row */
        GtkWidget *out_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        mwb->vol_mute_button = gtk_button_new();
        gtk_button_set_relief(GTK_BUTTON(mwb->vol_mute_button), GTK_RELIEF_NONE);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->vol_mute_button), "mwb-volbutton");
        gtk_widget_set_tooltip_text(mwb->vol_mute_button, _("Mute Output"));
        mwb->vol_mute_icon = gtk_image_new_from_icon_name("audio-volume-high", GTK_ICON_SIZE_MENU);
        gtk_container_add(GTK_CONTAINER(mwb->vol_mute_button), mwb->vol_mute_icon);
        gtk_widget_show(mwb->vol_mute_icon);
        g_signal_connect(mwb->vol_mute_button, "clicked", G_CALLBACK(mwb_volume_mute_clicked), mwb);
        gtk_box_pack_start(GTK_BOX(out_row), mwb->vol_mute_button, FALSE, FALSE, 0);

        mwb->vol_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
        gtk_scale_set_draw_value(GTK_SCALE(mwb->vol_scale), FALSE);
        gtk_widget_set_hexpand(mwb->vol_scale, TRUE);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->vol_scale), "mwb-vol-scale");
        gtk_box_pack_start(GTK_BOX(out_row), mwb->vol_scale, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(vol_card), out_row, FALSE, FALSE, 0);

        /* Microphone input row */
        GtkWidget *mic_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        mwb->vol_mic_button = gtk_button_new();
        gtk_button_set_relief(GTK_BUTTON(mwb->vol_mic_button), GTK_RELIEF_NONE);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->vol_mic_button), "mwb-volbutton");
        gtk_widget_set_tooltip_text(mwb->vol_mic_button, _("Mute Microphone"));
        mwb->vol_mic_icon = gtk_image_new_from_icon_name("audio-input-microphone-symbolic", GTK_ICON_SIZE_MENU);
        gtk_container_add(GTK_CONTAINER(mwb->vol_mic_button), mwb->vol_mic_icon);
        gtk_widget_show(mwb->vol_mic_icon);
        g_signal_connect(mwb->vol_mic_button, "clicked", G_CALLBACK(mwb_mic_mute_clicked), mwb);
        gtk_box_pack_start(GTK_BOX(mic_row), mwb->vol_mic_button, FALSE, FALSE, 0);

        mwb->vol_mic_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
        gtk_scale_set_draw_value(GTK_SCALE(mwb->vol_mic_scale), FALSE);
        gtk_widget_set_hexpand(mwb->vol_mic_scale, TRUE);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->vol_mic_scale), "mwb-vol-scale");
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->vol_mic_scale), "mwb-mic-scale");
        gtk_box_pack_start(GTK_BOX(mic_row), mwb->vol_mic_scale, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(vol_card), mic_row, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(root_box), vol_card, FALSE, FALSE, 0);

        /* Currently Playing Section Header */
        GtkWidget *np_hdr_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        GtkWidget *np_hdr = gtk_label_new(_("Now Playing"));
        gtk_label_set_xalign(GTK_LABEL(np_hdr), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(np_hdr), "mwb-pop-section-hdr");
        gtk_box_pack_start(GTK_BOX(np_hdr_box), np_hdr, TRUE, TRUE, 0);

        mwb->vol_playing_status = gtk_label_new("");
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->vol_playing_status), "mwb-badge");
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->vol_playing_status), "mwb-badge-playing");
        gtk_box_pack_start(GTK_BOX(np_hdr_box), mwb->vol_playing_status, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(root_box), np_hdr_box, FALSE, FALSE, 0);

        /* Beautiful Currently Playing Card */
        mwb->vol_playing_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->vol_playing_card), "mwb-playing-card");

        /* Card Top: Album Art + Track Information */
        GtkWidget *np_top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

        GtkWidget *art_frame = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_style_context_add_class(gtk_widget_get_style_context(art_frame), "mwb-playing-art-frame");
        mwb->vol_playing_art = gtk_image_new();
        gtk_image_set_pixel_size(GTK_IMAGE(mwb->vol_playing_art), 64);
        gtk_box_pack_start(GTK_BOX(art_frame), mwb->vol_playing_art, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(np_top), art_frame, FALSE, FALSE, 0);

        GtkWidget *ptxt = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        gtk_widget_set_valign(ptxt, GTK_ALIGN_CENTER);

        mwb->vol_playing_title = gtk_label_new(_("No media playing"));
        gtk_label_set_ellipsize(GTK_LABEL(mwb->vol_playing_title), PANGO_ELLIPSIZE_END);
        gtk_label_set_xalign(GTK_LABEL(mwb->vol_playing_title), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->vol_playing_title), "mwb-playing-title");
        gtk_box_pack_start(GTK_BOX(ptxt), mwb->vol_playing_title, FALSE, FALSE, 0);

        mwb->vol_playing_artist = gtk_label_new("");
        gtk_label_set_ellipsize(GTK_LABEL(mwb->vol_playing_artist), PANGO_ELLIPSIZE_END);
        gtk_label_set_xalign(GTK_LABEL(mwb->vol_playing_artist), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->vol_playing_artist), "mwb-playing-artist");
        gtk_box_pack_start(GTK_BOX(ptxt), mwb->vol_playing_artist, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(np_top), ptxt, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(mwb->vol_playing_card), np_top, FALSE, FALSE, 0);

        /* Playback Controls Toolbar */
        mwb->vol_playing_controls = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_widget_set_halign(mwb->vol_playing_controls, GTK_ALIGN_CENTER);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->vol_playing_controls), "mwb-media-toolbar");

        mwb->vol_playing_prev_btn = gtk_button_new();
        gtk_button_set_relief(GTK_BUTTON(mwb->vol_playing_prev_btn), GTK_RELIEF_NONE);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->vol_playing_prev_btn), "mwb-media-btn");
        GtkWidget *prev_icon = gtk_image_new_from_icon_name("media-skip-backward-symbolic", GTK_ICON_SIZE_MENU);
        gtk_container_add(GTK_CONTAINER(mwb->vol_playing_prev_btn), prev_icon);
        gtk_widget_set_tooltip_text(mwb->vol_playing_prev_btn, _("Previous Track"));
        g_signal_connect(mwb->vol_playing_prev_btn, "clicked", G_CALLBACK(mwb_media_prev_clicked), mwb);
        gtk_box_pack_start(GTK_BOX(mwb->vol_playing_controls), mwb->vol_playing_prev_btn, FALSE, FALSE, 0);

        mwb->vol_playing_play_btn = gtk_button_new();
        gtk_button_set_relief(GTK_BUTTON(mwb->vol_playing_play_btn), GTK_RELIEF_NONE);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->vol_playing_play_btn), "mwb-media-btn");
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->vol_playing_play_btn), "mwb-media-play-btn");
        mwb->vol_playing_play_icon = gtk_image_new_from_icon_name("media-playback-start-symbolic", GTK_ICON_SIZE_MENU);
        gtk_container_add(GTK_CONTAINER(mwb->vol_playing_play_btn), mwb->vol_playing_play_icon);
        gtk_widget_set_tooltip_text(mwb->vol_playing_play_btn, _("Play / Pause"));
        g_signal_connect(mwb->vol_playing_play_btn, "clicked", G_CALLBACK(mwb_media_play_clicked), mwb);
        gtk_box_pack_start(GTK_BOX(mwb->vol_playing_controls), mwb->vol_playing_play_btn, FALSE, FALSE, 0);

        mwb->vol_playing_next_btn = gtk_button_new();
        gtk_button_set_relief(GTK_BUTTON(mwb->vol_playing_next_btn), GTK_RELIEF_NONE);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->vol_playing_next_btn), "mwb-media-btn");
        GtkWidget *next_icon = gtk_image_new_from_icon_name("media-skip-forward-symbolic", GTK_ICON_SIZE_MENU);
        gtk_container_add(GTK_CONTAINER(mwb->vol_playing_next_btn), next_icon);
        gtk_widget_set_tooltip_text(mwb->vol_playing_next_btn, _("Next Track"));
        g_signal_connect(mwb->vol_playing_next_btn, "clicked", G_CALLBACK(mwb_media_next_clicked), mwb);
        gtk_box_pack_start(GTK_BOX(mwb->vol_playing_controls), mwb->vol_playing_next_btn, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(mwb->vol_playing_card), mwb->vol_playing_controls, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(root_box), mwb->vol_playing_card, FALSE, FALSE, 0);

        /* Active Playback Streams Section */
        GtkWidget *st_hdr = gtk_label_new(_("Applications"));
        gtk_label_set_xalign(GTK_LABEL(st_hdr), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(st_hdr), "mwb-pop-section-hdr");
        gtk_box_pack_start(GTK_BOX(root_box), st_hdr, FALSE, FALSE, 0);

        GtkWidget *streams_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
        gtk_style_context_add_class(gtk_widget_get_style_context(streams_card), "mwb-streams-card");

        mwb->vol_streams_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
        gtk_box_pack_start(GTK_BOX(streams_card), mwb->vol_streams_box, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(root_box), streams_card, FALSE, FALSE, 0);

        /* Audio Settings / Mixer Button */
        GtkWidget *mixer = gtk_button_new();
        gtk_button_set_relief(GTK_BUTTON(mixer), GTK_RELIEF_NONE);
        gtk_style_context_add_class(gtk_widget_get_style_context(mixer), "mwb-mixer-btn");
        GtkWidget *m_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_widget_set_halign(m_box, GTK_ALIGN_CENTER);
        GtkWidget *m_icon = gtk_image_new_from_icon_name("audio-card-symbolic", GTK_ICON_SIZE_MENU);
        GtkWidget *m_lbl = gtk_label_new(_("Audio Settings…"));
        gtk_box_pack_start(GTK_BOX(m_box), m_icon, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(m_box), m_lbl, FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(mixer), m_box);
        g_signal_connect_swapped(mixer, "clicked", G_CALLBACK(mwb_launch), "pavucontrol");
        gtk_box_pack_start(GTK_BOX(root_box), mixer, FALSE, FALSE, 0);

        gtk_container_add(GTK_CONTAINER(mwb->volume_popup), root_box);

        g_signal_connect(mwb->vol_scale, "value-changed",
                         G_CALLBACK(mwb_volume_changed), mwb);
        g_signal_connect(mwb->vol_mic_scale, "value-changed",
                         G_CALLBACK(mwb_mic_changed), mwb);
        g_signal_connect(mwb->volume_popup, "draw",
                         G_CALLBACK(mwb_popup_draw), NULL);
        g_signal_connect(mwb->volume_popup, "button-press-event",
                         G_CALLBACK(mwb_popup_button_press), mwb);
        g_signal_connect(mwb->volume_popup, "focus-out-event",
                         G_CALLBACK(mwb_popup_focus_out), mwb);
        g_signal_connect(mwb->volume_popup, "key-press-event",
                         G_CALLBACK(mwb_popup_key_press), mwb);
        g_signal_connect(mwb->volume_popup, "destroy",
                         G_CALLBACK(mwb_volume_popup_destroyed), mwb);
    }

    g_signal_handlers_block_by_func(mwb->vol_scale, mwb_volume_changed, mwb);
    gtk_range_set_value(GTK_RANGE(mwb->vol_scale), mwb->vol_percent);
    g_signal_handlers_unblock_by_func(mwb->vol_scale, mwb_volume_changed, mwb);
    mwb_volume_icon_update(mwb);

    mwb_popup_show(mwb, mwb->volume_popup, mwb->vol_button);

    /* refresh asynchronously so the popup opens instantly */
    mwb_volume_refresh(mwb);
    mwb_mic_refresh(mwb);
    mwb_volume_update_playing(mwb);
    mwb_streams_refresh(mwb);
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

/* Battery click — open rich Power & Battery Popover */
static void
mwb_batt_settings_clicked(GtkButton *button G_GNUC_UNUSED, MorphosWorkbenchPlugin *mwb)
{
    mwb_popup_hide(mwb, mwb->batt_popup);
    mwb_launch("xfce4-power-manager-settings || mate-power-preferences || gnome-control-center power");
}

static gboolean
mwb_batt_bar_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    MorphosWorkbenchPlugin *mwb = (MorphosWorkbenchPlugin *)data;
    gint w = gtk_widget_get_allocated_width(widget);
    gint h = gtk_widget_get_allocated_height(widget);
    if (w < 12 || h < 8)
        return FALSE;

    gdouble radius = 4.0;
    gdouble bw = w - 9.0; /* leave room for battery terminal tip */
    gdouble bh = h - 2.0;
    gdouble bx = 1.0;
    gdouble by = 1.0;

    /* Sunken battery casing / frame */
    cairo_pattern_t *casing = cairo_pattern_create_linear(bx, by, bx, by + bh);
    cairo_pattern_add_color_stop_rgba(casing, 0.0, 0.02, 0.04, 0.08, 0.95);
    cairo_pattern_add_color_stop_rgba(casing, 1.0, 0.08, 0.12, 0.18, 0.95);
    cairo_set_source(cr, casing);
    cairo_rounded_rectangle(cr, bx, by, bw, bh, radius);
    cairo_fill(cr);
    cairo_pattern_destroy(casing);

    /* Battery positive terminal nub on right */
    gdouble tip_w = 4.0;
    gdouble tip_h = bh * 0.45;
    gdouble tip_x = bx + bw;
    gdouble tip_y = by + (bh - tip_h) / 2.0;
    cairo_rounded_rectangle(cr, tip_x, tip_y, tip_w, tip_h, 1.5);
    cairo_set_source_rgba(cr, 0.45, 0.55, 0.68, 0.85);
    cairo_fill(cr);

    /* Inner level fill */
    gdouble pad = 2.0;
    gdouble max_fill_w = bw - (pad * 2.0);
    gdouble fill_h = bh - (pad * 2.0);
    gdouble pct = CLAMP(mwb->batt_percent, 0, 100) / 100.0;
    gdouble fill_w = max_fill_w * pct;

    if (fill_w > 1.0) {
        cairo_pattern_t *fill_pat = cairo_pattern_create_linear(bx + pad, by + pad, bx + pad + fill_w, by + pad);
        if (mwb->batt_charging) {
            /* Neon Electric Cyan -> Sapphire Blue */
            cairo_pattern_add_color_stop_rgb(fill_pat, 0.0, 0.0, 0.95, 0.99);
            cairo_pattern_add_color_stop_rgb(fill_pat, 1.0, 0.31, 0.67, 0.99);
        } else if (mwb->batt_percent >= 40) {
            /* Vibrant Emerald Green */
            cairo_pattern_add_color_stop_rgb(fill_pat, 0.0, 0.29, 0.87, 0.50);
            cairo_pattern_add_color_stop_rgb(fill_pat, 1.0, 0.09, 0.64, 0.29);
        } else if (mwb->batt_percent >= 15) {
            /* Amber Gold */
            cairo_pattern_add_color_stop_rgb(fill_pat, 0.0, 0.98, 0.75, 0.14);
            cairo_pattern_add_color_stop_rgb(fill_pat, 1.0, 0.85, 0.47, 0.02);
        } else {
            /* Ruby Red */
            cairo_pattern_add_color_stop_rgb(fill_pat, 0.0, 0.97, 0.44, 0.44);
            cairo_pattern_add_color_stop_rgb(fill_pat, 1.0, 0.86, 0.15, 0.15);
        }

        cairo_rounded_rectangle(cr, bx + pad, by + pad, fill_w, fill_h, 2.0);
        cairo_set_source(cr, fill_pat);
        cairo_fill(cr);
        cairo_pattern_destroy(fill_pat);

        /* Cylindrical reflection gloss */
        cairo_save(cr);
        cairo_rounded_rectangle(cr, bx + pad, by + pad, fill_w, fill_h, 2.0);
        cairo_clip(cr);
        cairo_pattern_t *gloss = cairo_pattern_create_linear(0, by + pad, 0, by + pad + fill_h);
        cairo_pattern_add_color_stop_rgba(gloss, 0.0, 1.0, 1.0, 1.0, 0.45);
        cairo_pattern_add_color_stop_rgba(gloss, 0.35, 1.0, 1.0, 1.0, 0.05);
        cairo_pattern_add_color_stop_rgba(gloss, 0.65, 0.0, 0.0, 0.0, 0.05);
        cairo_pattern_add_color_stop_rgba(gloss, 1.0, 0.0, 0.0, 0.0, 0.35);
        cairo_set_source(cr, gloss);
        cairo_paint(cr);
        cairo_pattern_destroy(gloss);
        cairo_restore(cr);
    }

    /* Metallic bevel outline */
    cairo_rounded_rectangle(cr, bx + 0.5, by + 0.5, bw - 1.0, bh - 1.0, radius);
    cairo_set_source_rgba(cr, 0.45, 0.58, 0.75, 0.55);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);

    /* Electric Gold Lightning bolt if charging */
    if (mwb->batt_charging) {
        gdouble cx = bx + (bw / 2.0);
        gdouble cy = by + (bh / 2.0);
        cairo_save(cr);
        cairo_set_line_width(cr, 1.2);
        cairo_move_to(cr, cx + 1.5, cy - 5.0);
        cairo_line_to(cr, cx - 3.0, cy + 0.5);
        cairo_line_to(cr, cx + 0.5, cy + 0.5);
        cairo_line_to(cr, cx - 1.5, cy + 5.0);
        cairo_line_to(cr, cx + 3.0, cy - 0.5);
        cairo_line_to(cr, cx - 0.5, cy - 0.5);
        cairo_close_path(cr);

        cairo_set_source_rgba(cr, 1.0, 0.90, 0.15, 1.0);
        cairo_fill_preserve(cr);
        cairo_set_source_rgba(cr, 0.05, 0.08, 0.15, 0.90);
        cairo_stroke(cr);
        cairo_restore(cr);
    }

    return FALSE;
}

static void
mwb_set_power_profile(MorphosWorkbenchPlugin *mwb, const gchar *profile)
{
    if (!profile || !*profile)
        return;

    gchar *cmd = g_strdup_printf("powerprofilesctl set %s", profile);
    mwb_launch(cmd);
    g_free(cmd);

    g_strlcpy(mwb->batt_profile_str, profile, sizeof(mwb->batt_profile_str));

    if (mwb->batt_prof_saver_btn && mwb->batt_prof_bal_btn && mwb->batt_prof_perf_btn) {
        GtkStyleContext *sc_sav = gtk_widget_get_style_context(mwb->batt_prof_saver_btn);
        GtkStyleContext *sc_bal = gtk_widget_get_style_context(mwb->batt_prof_bal_btn);
        GtkStyleContext *sc_prf = gtk_widget_get_style_context(mwb->batt_prof_perf_btn);

        gtk_style_context_remove_class(sc_sav, "mwb-media-play-btn");
        gtk_style_context_remove_class(sc_bal, "mwb-media-play-btn");
        gtk_style_context_remove_class(sc_prf, "mwb-media-play-btn");

        if (g_strcmp0(profile, "power-saver") == 0)
            gtk_style_context_add_class(sc_sav, "mwb-media-play-btn");
        else if (g_strcmp0(profile, "performance") == 0)
            gtk_style_context_add_class(sc_prf, "mwb-media-play-btn");
        else
            gtk_style_context_add_class(sc_bal, "mwb-media-play-btn");
    }
}

static void
mwb_batt_prof_saver_clicked(GtkButton *b G_GNUC_UNUSED, MorphosWorkbenchPlugin *mwb)
{
    mwb_set_power_profile(mwb, "power-saver");
}

static void
mwb_batt_prof_bal_clicked(GtkButton *b G_GNUC_UNUSED, MorphosWorkbenchPlugin *mwb)
{
    mwb_set_power_profile(mwb, "balanced");
}

static void
mwb_batt_prof_perf_clicked(GtkButton *b G_GNUC_UNUSED, MorphosWorkbenchPlugin *mwb)
{
    mwb_set_power_profile(mwb, "performance");
}

static void
mwb_batt_clicked(GtkButton *button G_GNUC_UNUSED, MorphosWorkbenchPlugin *mwb)
{
    if (mwb->batt_popup && gtk_widget_get_visible(mwb->batt_popup)) {
        mwb_popup_hide(mwb, mwb->batt_popup);
        return;
    }

    if (!mwb->batt_popup) {
        mwb->batt_popup = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        GdkScreen *screen = gdk_screen_get_default();
        GdkVisual *visual = screen ? gdk_screen_get_rgba_visual(screen) : NULL;
        if (visual) {
            gtk_widget_set_visual(mwb->batt_popup, visual);
            gtk_widget_set_app_paintable(mwb->batt_popup, TRUE);
        }
        gtk_window_set_decorated(GTK_WINDOW(mwb->batt_popup), FALSE);
        gtk_window_set_skip_taskbar_hint(GTK_WINDOW(mwb->batt_popup), TRUE);
        gtk_window_set_skip_pager_hint(GTK_WINDOW(mwb->batt_popup), TRUE);
        gtk_window_set_type_hint(GTK_WINDOW(mwb->batt_popup), GDK_WINDOW_TYPE_HINT_POPUP_MENU);
        gtk_window_set_resizable(GTK_WINDOW(mwb->batt_popup), FALSE);
        gtk_widget_set_size_request(mwb->batt_popup, 320, -1);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->batt_popup), "mwb-popup");
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->batt_popup), "mwb-vol-popup");
        gtk_widget_set_name(mwb->batt_popup, "mwb-batt-pop");
        mwb_theme_widget(mwb, mwb->batt_popup);

        GtkWidget *root_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_container_set_border_width(GTK_CONTAINER(root_box), 10);

        /* Header: Icon + Title + State Badge + Small Settings Button */
        GtkWidget *hdr = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        GtkWidget *hdr_icon = gtk_image_new_from_icon_name(
            mwb->batt_charging ? "battery-good-charging-symbolic" : "battery-full-symbolic",
            GTK_ICON_SIZE_MENU);
        gtk_image_set_pixel_size(GTK_IMAGE(hdr_icon), 16);
        gtk_style_context_add_class(gtk_widget_get_style_context(hdr_icon), "mwb-pop-icon");
        gtk_box_pack_start(GTK_BOX(hdr), hdr_icon, FALSE, FALSE, 0);

        GtkWidget *lbl = gtk_label_new(_("Power & Battery"));
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl), "mwb-pop-title");
        gtk_box_pack_start(GTK_BOX(hdr), lbl, TRUE, TRUE, 0);

        const gchar *status_str = mwb->batt_charging ? _("Charging") : (mwb->batt_percent >= 95 ? _("Fully Charged") : _("Discharging"));
        mwb->batt_pop_badge = gtk_label_new(status_str);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->batt_pop_badge), "mwb-vol-badge");
        if (mwb->batt_charging)
            gtk_style_context_add_class(gtk_widget_get_style_context(mwb->batt_pop_badge), "mwb-badge-playing");
        gtk_box_pack_start(GTK_BOX(hdr), mwb->batt_pop_badge, FALSE, FALSE, 0);

        /* Small MorphOS Settings Button */
        GtkWidget *settings_btn = gtk_button_new();
        gtk_button_set_relief(GTK_BUTTON(settings_btn), GTK_RELIEF_NONE);
        gtk_style_context_add_class(gtk_widget_get_style_context(settings_btn), "mwb-media-btn");
        gtk_style_context_add_class(gtk_widget_get_style_context(settings_btn), "mwb-gear-btn");
        GtkWidget *gear_img = gtk_image_new_from_icon_name("preferences-system-symbolic", GTK_ICON_SIZE_MENU);
        gtk_image_set_pixel_size(GTK_IMAGE(gear_img), 14);
        gtk_container_add(GTK_CONTAINER(settings_btn), gear_img);
        gtk_widget_set_tooltip_text(settings_btn, _("Power Management Settings..."));
        g_signal_connect(settings_btn, "clicked", G_CALLBACK(mwb_batt_settings_clicked), mwb);
        gtk_box_pack_start(GTK_BOX(hdr), settings_btn, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(root_box), hdr, FALSE, FALSE, 0);

        /* Card 1: Main Battery Level & 3D Gauge Card */
        GtkWidget *card1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_style_context_add_class(gtk_widget_get_style_context(card1), "mwb-vol-card");
        gtk_container_set_border_width(GTK_CONTAINER(card1), 8);

        GtkWidget *stat_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        gchar *pct_txt = mwb->batt_charging ?
            g_strdup_printf("⚡ %d%%", CLAMP(mwb->batt_percent, 0, 100)) :
            g_strdup_printf("%d%%", CLAMP(mwb->batt_percent, 0, 100));
        mwb->batt_pop_pct_lbl = gtk_label_new(pct_txt);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->batt_pop_pct_lbl), "mwb-pop-title");
        gtk_box_pack_start(GTK_BOX(stat_row), mwb->batt_pop_pct_lbl, FALSE, FALSE, 0);
        g_free(pct_txt);

        mwb->batt_pop_state_lbl = gtk_label_new(mwb->batt_charging ? _("AC Connected (Charging)") : _("Running on Battery"));
        gtk_label_set_xalign(GTK_LABEL(mwb->batt_pop_state_lbl), 1.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->batt_pop_state_lbl), "mwb-vol-title");
        gtk_box_pack_start(GTK_BOX(stat_row), mwb->batt_pop_state_lbl, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(card1), stat_row, FALSE, FALSE, 0);

        /* Large 3D Battery Bar */
        mwb->batt_pop_gauge = gtk_drawing_area_new();
        gtk_widget_set_size_request(mwb->batt_pop_gauge, -1, 16);
        g_signal_connect(mwb->batt_pop_gauge, "draw", G_CALLBACK(mwb_batt_bar_draw), mwb);
        gtk_box_pack_start(GTK_BOX(card1), mwb->batt_pop_gauge, FALSE, FALSE, 0);

        /* 2-Column MorphOS Details Grid */
        GtkWidget *grid = gtk_grid_new();
        gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
        gtk_grid_set_row_spacing(GTK_GRID(grid), 3);

        GtkWidget *l_src = gtk_label_new(_("Source:"));
        gtk_label_set_xalign(GTK_LABEL(l_src), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(l_src), "mwb-batt-grid-lbl");
        gtk_grid_attach(GTK_GRID(grid), l_src, 0, 0, 1, 1);

        GtkWidget *v_src = gtk_label_new(mwb->batt_charging ? _("AC Adapter") : _("Battery"));
        gtk_label_set_xalign(GTK_LABEL(v_src), 1.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(v_src), "mwb-batt-grid-val");
        gtk_grid_attach(GTK_GRID(grid), v_src, 1, 0, 1, 1);

        GtkWidget *l_tech = gtk_label_new(_("Technology:"));
        gtk_label_set_xalign(GTK_LABEL(l_tech), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(l_tech), "mwb-batt-grid-lbl");
        gtk_grid_attach(GTK_GRID(grid), l_tech, 0, 1, 1, 1);

        mwb->batt_pop_tech_lbl = gtk_label_new(mwb->batt_tech_str[0] ? mwb->batt_tech_str : "Li-ion");
        gtk_label_set_xalign(GTK_LABEL(mwb->batt_pop_tech_lbl), 1.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->batt_pop_tech_lbl), "mwb-batt-grid-val");
        gtk_grid_attach(GTK_GRID(grid), mwb->batt_pop_tech_lbl, 1, 1, 1, 1);

        GtkWidget *l_volt = gtk_label_new(_("Voltage:"));
        gtk_label_set_xalign(GTK_LABEL(l_volt), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(l_volt), "mwb-batt-grid-lbl");
        gtk_grid_attach(GTK_GRID(grid), l_volt, 2, 0, 1, 1);

        gchar *volt_txt = (mwb->batt_voltage_val > 0.1) ?
            g_strdup_printf("%.2f V", mwb->batt_voltage_val) : g_strdup("11.40 V");
        mwb->batt_pop_volt_lbl = gtk_label_new(volt_txt);
        gtk_label_set_xalign(GTK_LABEL(mwb->batt_pop_volt_lbl), 1.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->batt_pop_volt_lbl), "mwb-batt-grid-val");
        gtk_grid_attach(GTK_GRID(grid), mwb->batt_pop_volt_lbl, 3, 0, 1, 1);
        g_free(volt_txt);

        GtkWidget *l_cyc = gtk_label_new(_("Cycles:"));
        gtk_label_set_xalign(GTK_LABEL(l_cyc), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(l_cyc), "mwb-batt-grid-lbl");
        gtk_grid_attach(GTK_GRID(grid), l_cyc, 2, 1, 1, 1);

        gchar *cyc_txt = (mwb->batt_cycle_val > 0) ?
            g_strdup_printf("%d", mwb->batt_cycle_val) : g_strdup("N/A");
        mwb->batt_pop_cycle_lbl = gtk_label_new(cyc_txt);
        gtk_label_set_xalign(GTK_LABEL(mwb->batt_pop_cycle_lbl), 1.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->batt_pop_cycle_lbl), "mwb-batt-grid-val");
        gtk_grid_attach(GTK_GRID(grid), mwb->batt_pop_cycle_lbl, 3, 1, 1, 1);
        g_free(cyc_txt);

        gtk_box_pack_start(GTK_BOX(card1), grid, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(root_box), card1, TRUE, TRUE, 0);

        /* Card 2: Power Profiles Segmented Controls */
        GtkWidget *card2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_style_context_add_class(gtk_widget_get_style_context(card2), "mwb-vol-card");
        gtk_container_set_border_width(GTK_CONTAINER(card2), 8);

        GtkWidget *prof_hdr = gtk_label_new(_("Energy Mode"));
        gtk_label_set_xalign(GTK_LABEL(prof_hdr), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(prof_hdr), "mwb-pop-section-hdr");
        gtk_box_pack_start(GTK_BOX(card2), prof_hdr, FALSE, FALSE, 0);

        GtkWidget *prof_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
        gtk_widget_set_hexpand(prof_box, TRUE);

        mwb->batt_prof_saver_btn = gtk_button_new_with_label(_("🍃 Power Saver"));
        gtk_widget_set_hexpand(mwb->batt_prof_saver_btn, TRUE);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->batt_prof_saver_btn), "mwb-media-btn");
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->batt_prof_saver_btn), "mwb-prof-btn");
        g_signal_connect(mwb->batt_prof_saver_btn, "clicked", G_CALLBACK(mwb_batt_prof_saver_clicked), mwb);
        gtk_box_pack_start(GTK_BOX(prof_box), mwb->batt_prof_saver_btn, TRUE, TRUE, 0);

        mwb->batt_prof_bal_btn = gtk_button_new_with_label(_("⚖ Balanced"));
        gtk_widget_set_hexpand(mwb->batt_prof_bal_btn, TRUE);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->batt_prof_bal_btn), "mwb-media-btn");
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->batt_prof_bal_btn), "mwb-prof-btn");
        g_signal_connect(mwb->batt_prof_bal_btn, "clicked", G_CALLBACK(mwb_batt_prof_bal_clicked), mwb);
        gtk_box_pack_start(GTK_BOX(prof_box), mwb->batt_prof_bal_btn, TRUE, TRUE, 0);

        mwb->batt_prof_perf_btn = gtk_button_new_with_label(_("⚡ Performance"));
        gtk_widget_set_hexpand(mwb->batt_prof_perf_btn, TRUE);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->batt_prof_perf_btn), "mwb-media-btn");
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->batt_prof_perf_btn), "mwb-prof-btn");
        g_signal_connect(mwb->batt_prof_perf_btn, "clicked", G_CALLBACK(mwb_batt_prof_perf_clicked), mwb);
        gtk_box_pack_start(GTK_BOX(prof_box), mwb->batt_prof_perf_btn, TRUE, TRUE, 0);

        gtk_box_pack_start(GTK_BOX(card2), prof_box, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(root_box), card2, FALSE, FALSE, 0);

        gtk_container_add(GTK_CONTAINER(mwb->batt_popup), root_box);

        g_signal_connect(mwb->batt_popup, "draw",
                         G_CALLBACK(mwb_popup_draw), NULL);
        g_signal_connect(mwb->batt_popup, "button-press-event",
                         G_CALLBACK(mwb_popup_button_press), mwb);
        g_signal_connect(mwb->batt_popup, "focus-out-event",
                         G_CALLBACK(mwb_popup_focus_out), mwb);
        g_signal_connect(mwb->batt_popup, "key-press-event",
                         G_CALLBACK(mwb_popup_key_press), mwb);
        g_signal_connect(mwb->batt_popup, "destroy",
                         G_CALLBACK(gtk_widget_destroyed), &mwb->batt_popup);
    }

    /* Query and highlight active power profile */
    gchar *prof_out = NULL;
    if (g_spawn_command_line_sync("powerprofilesctl get", &prof_out, NULL, NULL, NULL) && prof_out) {
        g_strstrip(prof_out);
        g_strlcpy(mwb->batt_profile_str, prof_out, sizeof(mwb->batt_profile_str));
        g_free(prof_out);
    }
    if (mwb->batt_prof_saver_btn && mwb->batt_prof_bal_btn && mwb->batt_prof_perf_btn) {
        GtkStyleContext *sc_sav = gtk_widget_get_style_context(mwb->batt_prof_saver_btn);
        GtkStyleContext *sc_bal = gtk_widget_get_style_context(mwb->batt_prof_bal_btn);
        GtkStyleContext *sc_prf = gtk_widget_get_style_context(mwb->batt_prof_perf_btn);

        gtk_style_context_remove_class(sc_sav, "mwb-media-play-btn");
        gtk_style_context_remove_class(sc_bal, "mwb-media-play-btn");
        gtk_style_context_remove_class(sc_prf, "mwb-media-play-btn");

        if (g_strcmp0(mwb->batt_profile_str, "power-saver") == 0)
            gtk_style_context_add_class(sc_sav, "mwb-media-play-btn");
        else if (g_strcmp0(mwb->batt_profile_str, "performance") == 0)
            gtk_style_context_add_class(sc_prf, "mwb-media-play-btn");
        else
            gtk_style_context_add_class(sc_bal, "mwb-media-play-btn");
    }

    mwb_tick_battery(mwb);
    mwb_popup_show(mwb, mwb->batt_popup, mwb->batt_button);
}

/* Wi-Fi & Network Popover */
static void
mwb_wifi_settings_clicked(GtkButton *button G_GNUC_UNUSED, MorphosWorkbenchPlugin *mwb)
{
    mwb_popup_hide(mwb, mwb->wifi_popup);
    mwb_launch("nm-connection-editor || xfce4-settings-manager");
}

static void
mwb_wifi_clicked(GtkButton *button G_GNUC_UNUSED, MorphosWorkbenchPlugin *mwb)
{
    if (mwb->wifi_popup && gtk_widget_get_visible(mwb->wifi_popup)) {
        mwb_popup_hide(mwb, mwb->wifi_popup);
        return;
    }

    if (!mwb->wifi_popup) {
        mwb->wifi_popup = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        GdkScreen *screen = gdk_screen_get_default();
        GdkVisual *visual = screen ? gdk_screen_get_rgba_visual(screen) : NULL;
        if (visual) {
            gtk_widget_set_visual(mwb->wifi_popup, visual);
            gtk_widget_set_app_paintable(mwb->wifi_popup, TRUE);
        }
        gtk_window_set_decorated(GTK_WINDOW(mwb->wifi_popup), FALSE);
        gtk_window_set_skip_taskbar_hint(GTK_WINDOW(mwb->wifi_popup), TRUE);
        gtk_window_set_skip_pager_hint(GTK_WINDOW(mwb->wifi_popup), TRUE);
        gtk_window_set_type_hint(GTK_WINDOW(mwb->wifi_popup), GDK_WINDOW_TYPE_HINT_POPUP_MENU);
        gtk_window_set_resizable(GTK_WINDOW(mwb->wifi_popup), FALSE);
        gtk_widget_set_size_request(mwb->wifi_popup, 320, -1);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->wifi_popup), "mwb-popup");
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->wifi_popup), "mwb-vol-popup");
        mwb_theme_widget(mwb, mwb->wifi_popup);

        GtkWidget *root_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_container_set_border_width(GTK_CONTAINER(root_box), 12);

        /* Header: Icon + Title + Status Badge */
        GtkWidget *hdr = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        GtkWidget *hdr_icon = gtk_image_new_from_icon_name("network-wireless-signal-good-symbolic", GTK_ICON_SIZE_MENU);
        gtk_style_context_add_class(gtk_widget_get_style_context(hdr_icon), "mwb-pop-icon");
        gtk_box_pack_start(GTK_BOX(hdr), hdr_icon, FALSE, FALSE, 0);

        GtkWidget *lbl = gtk_label_new(_("Network & Wi-Fi"));
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl), "mwb-pop-title");
        gtk_box_pack_start(GTK_BOX(hdr), lbl, TRUE, TRUE, 0);

        GtkWidget *badge = gtk_label_new(_("Connected"));
        gtk_style_context_add_class(gtk_widget_get_style_context(badge), "mwb-vol-badge");
        gtk_box_pack_start(GTK_BOX(hdr), badge, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(root_box), hdr, FALSE, FALSE, 0);

        /* Card 1: Active Connection Info */
        GtkWidget *card1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_style_context_add_class(gtk_widget_get_style_context(card1), "mwb-vol-card");
        gtk_container_set_border_width(GTK_CONTAINER(card1), 10);

        /* Fast, non-blocking check of network state */
        gchar *conn_name = NULL;
        if (g_file_test("/sys/class/net/wlan0/operstate", G_FILE_TEST_EXISTS)) {
            gchar *state = NULL;
            if (g_file_get_contents("/sys/class/net/wlan0/operstate", &state, NULL, NULL) && state) {
                g_strstrip(state);
                if (g_ascii_strcasecmp(state, "up") == 0)
                    conn_name = g_strdup(_("Wi-Fi Connected"));
                g_free(state);
            }
        }
        if (!conn_name)
            conn_name = g_strdup(_("Network Connected"));

        GtkWidget *row1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        GtkWidget *ssid_lbl = gtk_label_new(conn_name ? conn_name : _("Active Connection"));
        gtk_style_context_add_class(gtk_widget_get_style_context(ssid_lbl), "mwb-pop-title");
        gtk_box_pack_start(GTK_BOX(row1), ssid_lbl, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(card1), row1, FALSE, FALSE, 0);
        g_free(conn_name);

        GtkWidget *row2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        GtkWidget *type_lbl = gtk_label_new(_("Interface: Wireless (wlan0)"));
        gtk_style_context_add_class(gtk_widget_get_style_context(type_lbl), "mwb-vol-title");
        gtk_box_pack_start(GTK_BOX(row2), type_lbl, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(card1), row2, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(root_box), card1, TRUE, TRUE, 0);

        /* Card 2: Actions */
        GtkWidget *btn = gtk_button_new_with_label(_("Network Connections..."));
        gtk_style_context_add_class(gtk_widget_get_style_context(btn), "mwb-mixer-btn");
        g_signal_connect(btn, "clicked", G_CALLBACK(mwb_wifi_settings_clicked), mwb);
        gtk_box_pack_start(GTK_BOX(root_box), btn, FALSE, FALSE, 0);

        gtk_container_add(GTK_CONTAINER(mwb->wifi_popup), root_box);

        g_signal_connect(mwb->wifi_popup, "draw",
                         G_CALLBACK(mwb_popup_draw), NULL);
        g_signal_connect(mwb->wifi_popup, "button-press-event",
                         G_CALLBACK(mwb_popup_button_press), mwb);
        g_signal_connect(mwb->wifi_popup, "focus-out-event",
                         G_CALLBACK(mwb_popup_focus_out), mwb);
        g_signal_connect(mwb->wifi_popup, "key-press-event",
                         G_CALLBACK(mwb_popup_key_press), mwb);
        g_signal_connect(mwb->wifi_popup, "destroy",
                         G_CALLBACK(gtk_widget_destroyed), &mwb->wifi_popup);
    }

    GtkWidget *anchor = mwb->wifi_button ? mwb->wifi_button : (mwb->nm_plugin ? mwb->nm_plugin : mwb->bar);
    mwb_popup_show(mwb, mwb->wifi_popup, anchor);
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
    if (!lamp)
        return;

    gint state = 0;
    if (rate > 0.0)
        state = (rate >= MWB_NET_HIGH_THRESHOLD) ? 2 : 1;

    if (state == mwb->net_prev_state[idx])
        return;

    mwb->net_prev_state[idx] = state;
    GtkStyleContext *ctx = gtk_widget_get_style_context(lamp);
    gtk_style_context_remove_class(ctx, "active-net");
    gtk_style_context_remove_class(ctx, "active-net-high");
    if (state == 2)
        gtk_style_context_add_class(ctx, "active-net-high");
    else if (state == 1)
        gtk_style_context_add_class(ctx, "active-net");
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

    /* Throttle tooltip string allocations (~1s interval or on first run) */
    mwb->net_tip_ticks++;
    if (mwb->net_tip_ticks >= 10 || mwb->net_prev_tip == NULL) {
        mwb->net_tip_ticks = 0;
        gdouble rate = (tx_rate + rx_rate) / 1024.0;
        gchar *tip = g_strdup_printf(_("Network traffic: %.1f KiB/s"), rate);
        if (g_strcmp0(mwb->net_prev_tip, tip) != 0) {
            g_free(mwb->net_prev_tip);
            mwb->net_prev_tip = tip;
            if (mwb->net_lamps[MWB_LAMP_NET_TX])
                gtk_widget_set_tooltip_text(mwb->net_lamps[MWB_LAMP_NET_TX], mwb->net_prev_tip);
            if (mwb->net_lamps[MWB_LAMP_NET_RX])
                gtk_widget_set_tooltip_text(mwb->net_lamps[MWB_LAMP_NET_RX], mwb->net_prev_tip);
        } else {
            g_free(tip);
        }
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

/* Horizontal battery renderer */
static gboolean
mwb_batt_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    MorphosWorkbenchPlugin *mwb = data;
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);

    gdouble w = alloc.width;
    gdouble h = alloc.height;
    if (w < 10 || h < 8)
        return FALSE;

    /* Horizontal battery geometry */
    gdouble batt_w = 22.0;
    gdouble batt_h = 12.0;
    gdouble nib_w = 2.0;
    gdouble nib_h = 5.0;
    gdouble total_w = batt_w + nib_w + 1.0;

    gdouble bx = floor((w - total_w) / 2.0) + 0.5;
    gdouble by = floor((h - batt_h) / 2.0) + 0.5;

    gboolean light = (mwb->theme == MWB_THEME_LIGHT);
    if (mwb->theme == MWB_THEME_SYSTEM && widget != NULL) {
        GtkStyleContext *ctx = gtk_widget_get_style_context(widget);
        GdkRGBA bg;
        if (gtk_style_context_lookup_color(ctx, "theme_bg_color", &bg)) {
            gdouble lum = 0.299 * bg.red + 0.587 * bg.green + 0.114 * bg.blue;
            light = (lum > 0.5);
        }
    }

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);

    /* Battery body outline */
    cairo_rounded_rectangle(cr, bx, by, batt_w, batt_h, 2.5);
    if (light)
        cairo_set_source_rgba(cr, 0.25, 0.32, 0.42, 0.85);
    else
        cairo_set_source_rgba(cr, 0.65, 0.78, 0.92, 0.85);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);

    /* Battery positive terminal nib on the right */
    gdouble nx = bx + batt_w + 0.5;
    gdouble ny = by + (batt_h - nib_h) / 2.0;
    cairo_rounded_rectangle(cr, nx, ny, nib_w, nib_h, 1.0);
    if (light)
        cairo_set_source_rgba(cr, 0.25, 0.32, 0.42, 0.85);
    else
        cairo_set_source_rgba(cr, 0.65, 0.78, 0.92, 0.85);
    cairo_fill(cr);

    /* Inner trough */
    gdouble pad = 2.0;
    gdouble in_x = bx + pad;
    gdouble in_y = by + pad;
    gdouble in_w = batt_w - (pad * 2.0);
    gdouble in_h = batt_h - (pad * 2.0);

    cairo_rounded_rectangle(cr, in_x, in_y, in_w, in_h, 1.5);
    if (light)
        cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.12);
    else
        cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.50);
    cairo_fill(cr);

    /* Charge level horizontal fill */
    gint pct = CLAMP(mwb->batt_percent, 0, 100);
    gdouble fill_w = (in_w * pct) / 100.0;

    if (fill_w > 0.5) {
        cairo_save(cr);
        cairo_rounded_rectangle(cr, in_x, in_y, in_w, in_h, 1.5);
        cairo_clip(cr);

        cairo_pattern_t *pat = cairo_pattern_create_linear(in_x, in_y, in_x, in_y + in_h);
        if (mwb->batt_charging) {
            /* Charging cyan / electric blue */
            cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.30, 0.85, 1.0, 0.95);
            cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.05, 0.55, 0.85, 0.95);
        } else if (pct >= 40) {
            /* Healthy emerald green */
            cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.35, 0.88, 0.45, 0.95);
            cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.12, 0.65, 0.25, 0.95);
        } else if (pct >= 15) {
            /* Warning amber */
            cairo_pattern_add_color_stop_rgba(pat, 0.0, 1.00, 0.85, 0.20, 0.95);
            cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.85, 0.60, 0.05, 0.95);
        } else {
            /* Critical red */
            cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.98, 0.45, 0.45, 0.95);
            cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.85, 0.18, 0.18, 0.95);
        }

        cairo_rectangle(cr, in_x, in_y, fill_w, in_h);
        cairo_set_source(cr, pat);
        cairo_fill(cr);
        cairo_pattern_destroy(pat);

        /* Subtle top sheen */
        cairo_rectangle(cr, in_x, in_y, fill_w, 1.0);
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.35);
        cairo_fill(cr);

        cairo_restore(cr);
    }

    /* If charging, draw a sharp lightning bolt glyph on the battery body */
    if (mwb->batt_charging) {
        gdouble cx = bx + (batt_w / 2.0);
        gdouble cy = by + (batt_h / 2.0);

        cairo_save(cr);
        cairo_set_line_width(cr, 1.2);
        cairo_move_to(cr, cx + 1.0, cy - 4.5);
        cairo_line_to(cr, cx - 2.5, cy + 0.5);
        cairo_line_to(cr, cx + 0.5, cy + 0.5);
        cairo_line_to(cr, cx - 1.0, cy + 4.5);
        cairo_line_to(cr, cx + 2.5, cy - 0.5);
        cairo_line_to(cr, cx - 0.5, cy - 0.5);
        cairo_close_path(cr);

        cairo_set_source_rgba(cr, 1.0, 0.88, 0.12, 1.0); /* Vibrant electric gold */
        cairo_fill_preserve(cr);
        cairo_set_source_rgba(cr, 0.1, 0.1, 0.15, 0.85);  /* High-contrast dark outline */
        cairo_stroke(cr);
        cairo_restore(cr);
    }

    return FALSE;
}

/* Battery — reads sysfs power_supply */
static gboolean
mwb_tick_battery(MorphosWorkbenchPlugin *mwb)
{
    gchar *content = NULL;
    gint cap = -1;
    gchar status[32] = "";

    const gchar *bat_paths[] = {
        "/sys/class/power_supply/BAT0",
        "/sys/class/power_supply/BAT1",
        "/sys/class/power_supply/battery"
    };
    const gchar *bat_dir = NULL;
    guint i;
    for (i = 0; i < G_N_ELEMENTS(bat_paths); i++) {
        if (g_file_test(bat_paths[i], G_FILE_TEST_IS_DIR)) {
            bat_dir = bat_paths[i];
            break;
        }
    }

    if (bat_dir) {
        gchar *cap_file = g_build_filename(bat_dir, "capacity", NULL);
        if (g_file_get_contents(cap_file, &content, NULL, NULL) && content) {
            cap = atoi(content);
            g_free(content);
            content = NULL;
        }
        g_free(cap_file);

        gchar *stat_file = g_build_filename(bat_dir, "status", NULL);
        if (g_file_get_contents(stat_file, &content, NULL, NULL) && content) {
            g_strstrip(content);
            g_strlcpy(status, content, sizeof(status));
            g_strlcpy(mwb->batt_status_str, content, sizeof(mwb->batt_status_str));
            g_free(content);
            content = NULL;
        }
        g_free(stat_file);

        gchar *tech_file = g_build_filename(bat_dir, "technology", NULL);
        if (g_file_get_contents(tech_file, &content, NULL, NULL) && content) {
            g_strstrip(content);
            g_strlcpy(mwb->batt_tech_str, content, sizeof(mwb->batt_tech_str));
            g_free(content);
            content = NULL;
        }
        g_free(tech_file);

        gchar *volt_file = g_build_filename(bat_dir, "voltage_now", NULL);
        if (g_file_get_contents(volt_file, &content, NULL, NULL) && content) {
            mwb->batt_voltage_val = (gdouble)g_ascii_strtoll(content, NULL, 10) / 1000000.0;
            g_free(content);
            content = NULL;
        }
        g_free(volt_file);

        gchar *cyc_file = g_build_filename(bat_dir, "cycle_count", NULL);
        if (g_file_get_contents(cyc_file, &content, NULL, NULL) && content) {
            mwb->batt_cycle_val = atoi(content);
            g_free(content);
            content = NULL;
        }
        g_free(cyc_file);
    }

    /* Check AC online status */
    gboolean ac_online = FALSE;
    const gchar *ac_paths[] = {
        "/sys/class/power_supply/AC",
        "/sys/class/power_supply/AC0",
        "/sys/class/power_supply/AC1",
        "/sys/class/power_supply/ACAD",
        "/sys/class/power_supply/ADP1",
        "/sys/class/power_supply/ADP0"
    };
    for (i = 0; i < G_N_ELEMENTS(ac_paths); i++) {
        if (g_file_test(ac_paths[i], G_FILE_TEST_IS_DIR)) {
            gchar *online_file = g_build_filename(ac_paths[i], "online", NULL);
            if (g_file_get_contents(online_file, &content, NULL, NULL) && content) {
                if (atoi(content) == 1)
                    ac_online = TRUE;
                g_free(content);
                content = NULL;
            }
            g_free(online_file);
            if (ac_online)
                break;
        }
    }

    if (cap < 0 || !mwb->batt_icon)
        return G_SOURCE_CONTINUE;

    gboolean charging = (g_ascii_strcasecmp(status, "Charging") == 0) ||
                        (ac_online && g_ascii_strcasecmp(status, "Discharging") != 0);
    mwb->batt_percent = cap;
    mwb->batt_charging = charging;

    gtk_widget_queue_draw(mwb->batt_icon);

    if (mwb->batt_label) {
        gchar *txt = charging ?
            g_strdup_printf("⚡ %d%%", cap) :
            g_strdup_printf("%d%%", cap);
        gtk_label_set_text(GTK_LABEL(mwb->batt_label), txt);
        g_free(txt);
    }

    gchar *tip = g_strdup_printf(_("Battery: %d%% (%s%s)"),
                                 cap,
                                 charging ? _("Charging") : (status[0] ? status : _("Discharging")),
                                 ac_online ? _(" - AC Connected") : "");
    gtk_widget_set_tooltip_text(mwb->batt_button ? mwb->batt_button : mwb->batt_icon, tip);
    g_free(tip);

    /* Dynamically refresh open battery popover */
    if (mwb->batt_popup && gtk_widget_get_visible(mwb->batt_popup)) {
        if (mwb->batt_pop_pct_lbl) {
            gchar *pct_txt = charging ?
                g_strdup_printf("⚡ %d%%", cap) :
                g_strdup_printf("%d%%", cap);
            gtk_label_set_text(GTK_LABEL(mwb->batt_pop_pct_lbl), pct_txt);
            g_free(pct_txt);
        }
        if (mwb->batt_pop_state_lbl) {
            gtk_label_set_text(GTK_LABEL(mwb->batt_pop_state_lbl),
                               charging ? _("AC Connected (Charging)") : _("Running on Battery"));
        }
        if (mwb->batt_pop_badge) {
            const gchar *s_str = charging ? _("Charging") : (cap >= 95 ? _("Fully Charged") : _("Discharging"));
            gtk_label_set_text(GTK_LABEL(mwb->batt_pop_badge), s_str);
            GtkStyleContext *sc_b = gtk_widget_get_style_context(mwb->batt_pop_badge);
            if (charging)
                gtk_style_context_add_class(sc_b, "mwb-badge-playing");
            else
                gtk_style_context_remove_class(sc_b, "mwb-badge-playing");
        }
        if (mwb->batt_pop_tech_lbl) {
            gtk_label_set_text(GTK_LABEL(mwb->batt_pop_tech_lbl),
                               mwb->batt_tech_str[0] ? mwb->batt_tech_str : "Li-ion");
        }
        if (mwb->batt_pop_volt_lbl) {
            gchar *v_txt = (mwb->batt_voltage_val > 0.1) ?
                g_strdup_printf("%.2f V", mwb->batt_voltage_val) : g_strdup("11.40 V");
            gtk_label_set_text(GTK_LABEL(mwb->batt_pop_volt_lbl), v_txt);
            g_free(v_txt);
        }
        if (mwb->batt_pop_cycle_lbl) {
            gchar *c_txt = (mwb->batt_cycle_val > 0) ?
                g_strdup_printf("%d", mwb->batt_cycle_val) : g_strdup("N/A");
            gtk_label_set_text(GTK_LABEL(mwb->batt_pop_cycle_lbl), c_txt);
            g_free(c_txt);
        }
        if (mwb->batt_pop_gauge)
            gtk_widget_queue_draw(mwb->batt_pop_gauge);
    }

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

    /* Fast, non-blocking POSIX statvfs for disk space */
    struct statvfs st;
    gdouble disk_used_pct = -1.0;
    if (statvfs("/", &st) == 0 && st.f_blocks > 0) {
        guint64 total_blocks = st.f_blocks;
        guint64 free_blocks = st.f_bavail;
        guint64 used_blocks = (total_blocks > free_blocks) ? (total_blocks - free_blocks) : 0;
        disk_used_pct = (gdouble)used_blocks / (gdouble)total_blocks;
    }

    if (mwb->disk_gauge && disk_used_pct >= 0.0)
        mwb_gauge_set(mwb->disk_gauge, disk_used_pct);

    if (mwb->sys_button) {
        /* POSIX uname with zero subprocess overhead */
        struct utsname uts;
        gchar kernel_buf[128];
        if (uname(&uts) == 0)
            g_snprintf(kernel_buf, sizeof(kernel_buf), "%s %s", uts.sysname, uts.release);
        else
            g_strlcpy(kernel_buf, "?", sizeof(kernel_buf));

        gchar *tip = g_strdup_printf(
            _("System\nKernel: %s\nCPU: %d%%\nMemory: %d%%\nDisk: %d%%\nUptime: see clock"),
            kernel_buf,
            (gint)(mwb->cpu_load * 100.0),
            total > 0 ? (gint)(100 - available * 100 / total) : 0,
            disk_used_pct >= 0 ? (gint)(disk_used_pct * 100.0) : 0);
        gtk_widget_set_tooltip_text(mwb->sys_button, tip);
        g_free(tip);
    }
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

/* Per-widget builders — each returns the widget to pack into the screenbar. */

const gchar *
mwb_widget_name(guint widget)
{
    static const gchar *names[MWB_WIDGET_COUNT] = {
        N_("Disk lamps"), N_("Network lamps"), N_("Wi-Fi"), N_("Battery"),
        N_("CPU gauges"), N_("Memory gauge"), N_("Disk gauge"),
        N_("System info"), N_("Notifications"), N_("Volume"), N_("Clock"),
    };
    if (widget >= MWB_WIDGET_COUNT)
        return "?";
    return names[widget];
}

static GtkWidget *
mwb_build_drivelamps(MorphosWorkbenchPlugin *mwb)
{
    GtkWidget *diskbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
    guint d;
    gtk_style_context_add_class(gtk_widget_get_style_context(diskbox), "mwb-island");
    gtk_widget_set_valign(diskbox, GTK_ALIGN_CENTER);
    for (d = 0; d < 2; d++) {
        mwb->disk_lamps[d] = mwb_lamp_new(_("Disk activity"), "disk");
        gtk_box_pack_start(GTK_BOX(diskbox), mwb->disk_lamps[d], FALSE, FALSE, 0);
    }
    return diskbox;
}

static GtkWidget *
mwb_build_netlamps(MorphosWorkbenchPlugin *mwb)
{
    GtkWidget *netbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
    gtk_style_context_add_class(gtk_widget_get_style_context(netbox), "mwb-island");
    gtk_widget_set_valign(netbox, GTK_ALIGN_CENTER);
    mwb->net_lamps[MWB_LAMP_NET_TX] = mwb_lamp_new(_("Network transmit"), "net");
    gtk_box_pack_start(GTK_BOX(netbox), mwb->net_lamps[MWB_LAMP_NET_TX], FALSE, FALSE, 0);
    mwb->net_lamps[MWB_LAMP_NET_RX] = mwb_lamp_new(_("Network receive"), "net");
    gtk_box_pack_start(GTK_BOX(netbox), mwb->net_lamps[MWB_LAMP_NET_RX], FALSE, FALSE, 0);
    return netbox;
}

static GtkWidget *
mwb_build_wifi(MorphosWorkbenchPlugin *mwb)
{
    mwb->nm_plugin = mwb_embed_networkmanager(mwb);
    if (mwb->nm_plugin) {
        gtk_widget_show(mwb->nm_plugin);
        return mwb->nm_plugin;
    }

    /* Native MorphOS Wi-Fi screenbar button */
    mwb->wifi_button = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(mwb->wifi_button), GTK_RELIEF_NONE);
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->wifi_button), "mwb-volbutton");
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->wifi_button), "mwb-wifi-button");
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->wifi_button), "mwb-screenbar-pill");
    gtk_widget_set_valign(mwb->wifi_button, GTK_ALIGN_CENTER);

    GtkWidget *icon = gtk_image_new_from_icon_name("network-wireless-signal-good-symbolic", GTK_ICON_SIZE_MENU);
    gtk_image_set_pixel_size(GTK_IMAGE(icon), 16);
    gtk_container_add(GTK_CONTAINER(mwb->wifi_button), icon);
    gtk_widget_set_tooltip_text(mwb->wifi_button, _("Network & Wi-Fi"));
    gtk_widget_show_all(mwb->wifi_button);
    g_signal_connect(mwb->wifi_button, "clicked", G_CALLBACK(mwb_wifi_clicked), mwb);
    return mwb->wifi_button;
}

static GtkWidget *
mwb_build_battery(MorphosWorkbenchPlugin *mwb)
{
    mwb->batt_button = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(mwb->batt_button), GTK_RELIEF_NONE);
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->batt_button), "mwb-volbutton");
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->batt_button), "mwb-batt-button");
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->batt_button), "mwb-screenbar-pill");
    gtk_widget_set_valign(mwb->batt_button, GTK_ALIGN_CENTER);

    GtkWidget *batt_content = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    mwb->batt_icon = gtk_drawing_area_new();
    gtk_widget_set_size_request(mwb->batt_icon, 24, 14);
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->batt_icon), "mwb-batt-gauge");
    g_signal_connect(mwb->batt_icon, "draw", G_CALLBACK(mwb_batt_draw), mwb);
    gtk_box_pack_start(GTK_BOX(batt_content), mwb->batt_icon, FALSE, FALSE, 0);
    gtk_widget_show(mwb->batt_icon);

    mwb->batt_label = gtk_label_new("");
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->batt_label), "mwb-screenbar-item");
    gtk_box_pack_start(GTK_BOX(batt_content), mwb->batt_label, FALSE, FALSE, 0);
    gtk_widget_show(mwb->batt_label);

    gtk_container_add(GTK_CONTAINER(mwb->batt_button), batt_content);
    gtk_widget_show(batt_content);
    g_signal_connect(mwb->batt_button, "clicked", G_CALLBACK(mwb_batt_clicked), mwb);

    mwb_tick_battery(mwb);
    return mwb->batt_button;
}

static GtkWidget *
mwb_build_cpu(MorphosWorkbenchPlugin *mwb)
{
    gint nc = sysconf(_SC_NPROCESSORS_ONLN);
    if (nc > 16)
        nc = 16;
    if (nc < 1)
        nc = 1;
    mwb->cpu_ncores = nc;

    mwb->cpu_gauges[0] = mwb_gauge_new(MWB_GAUGE_CPU, _("CPU"), mwb->theme, mwb->gauge_style);
    return mwb->cpu_gauges[0];
}

static GtkWidget *
mwb_build_mem(MorphosWorkbenchPlugin *mwb)
{
    mwb->mem_gauge = mwb_gauge_new(MWB_GAUGE_MEM, _("RAM"), mwb->theme, mwb->gauge_style);
    return mwb->mem_gauge;
}

static GtkWidget *
mwb_build_diskgauge(MorphosWorkbenchPlugin *mwb)
{
    mwb->disk_gauge = mwb_gauge_new(MWB_GAUGE_DISK, _("DISK"), mwb->theme, mwb->gauge_style);
    return mwb->disk_gauge;
}

static GtkWidget *
mwb_build_sysinfo(MorphosWorkbenchPlugin *mwb)
{
    mwb->sys_button = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(mwb->sys_button), GTK_RELIEF_NONE);
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->sys_button), "mwb-volbutton");
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->sys_button), "mwb-screenbar-pill");
    gtk_widget_set_valign(mwb->sys_button, GTK_ALIGN_CENTER);
    gtk_widget_set_tooltip_text(mwb->sys_button, _("System information"));
    GtkWidget *sys_img = gtk_image_new_from_icon_name("computer", GTK_ICON_SIZE_MENU);
    gtk_image_set_pixel_size(GTK_IMAGE(sys_img), 16);
    gtk_container_add(GTK_CONTAINER(mwb->sys_button), sys_img);
    gtk_widget_show(sys_img);
    g_signal_connect(mwb->sys_button, "clicked", G_CALLBACK(mwb_sys_clicked), mwb);
    return mwb->sys_button;
}

static GtkWidget *
mwb_build_volume(MorphosWorkbenchPlugin *mwb)
{
    mwb->vol_button = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(mwb->vol_button), GTK_RELIEF_NONE);
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->vol_button), "mwb-volbutton");
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->vol_button), "mwb-screenbar-pill");
    gtk_widget_set_valign(mwb->vol_button, GTK_ALIGN_CENTER);
    gtk_widget_set_tooltip_text(mwb->vol_button, _("Volume"));
    mwb->vol_icon = gtk_image_new_from_icon_name("audio-volume-high", GTK_ICON_SIZE_MENU);
    gtk_image_set_pixel_size(GTK_IMAGE(mwb->vol_icon), 16);
    gtk_container_add(GTK_CONTAINER(mwb->vol_button), mwb->vol_icon);
    gtk_widget_show(mwb->vol_icon);
    g_signal_connect(mwb->vol_button, "clicked", G_CALLBACK(mwb_volume_clicked), mwb);
    return mwb->vol_button;
}

/* ------------------------------------------------------------------ *
 *  Notification Center & Screenbar Widget
 *  ------------------------------------------------------------------ */

void
mwb_notification_free(MwbNotification *n)
{
    if (!n)
        return;
    g_free(n->id);
    g_free(n->app_name);
    g_free(n->icon_name);
    g_free(n->summary);
    g_free(n->body);
    g_free(n);
}

static gchar *
mwb_format_time_ago(gint64 ts)
{
    gint64 now = g_get_real_time() / G_USEC_PER_SEC;
    gint64 diff = now - ts;
    if (diff < 0)
        diff = 0;

    if (diff < 60)
        return g_strdup(_("Just now"));
    if (diff < 3600)
        return g_strdup_printf(_("%dm ago"), (gint)(diff / 60));
    if (diff < 86400)
        return g_strdup_printf(_("%dh ago"), (gint)(diff / 3600));
    if (diff < 172800)
        return g_strdup(_("Yesterday"));

    GDateTime *dt = g_date_time_new_from_unix_local(ts);
    if (dt) {
        gchar *res = g_date_time_format(dt, "%b %d");
        g_date_time_unref(dt);
        return res;
    }
    return g_strdup_printf(_("%dd ago"), (gint)(diff / 86400));
}

static void
mwb_notifications_load_sqlite(MorphosWorkbenchPlugin *mwb)
{
#ifdef HAVE_SQLITE3
    gchar *dir = g_build_filename(g_get_user_cache_dir(), "xfce4", "notifyd", NULL);
    g_mkdir_with_parents(dir, 0755);
    gchar *db_path = g_build_filename(dir, "log.sqlite", NULL);
    g_free(dir);

    sqlite3 *db = NULL;
    if (sqlite3_open(db_path, &db) != SQLITE_OK) {
        if (db)
            sqlite3_close(db);
        g_free(db_path);
        return;
    }

    sqlite3_exec(db,
        "CREATE TABLE IF NOT EXISTS notifications ("
        "id TEXT PRIMARY KEY NOT NULL,"
        "timestamp INTEGER NOT NULL,"
        "tz_identifier TEXT NOT NULL,"
        "app_id TEXT,"
        "app_name TEXT,"
        "icon_id TEXT,"
        "summary TEXT,"
        "body TEXT,"
        "actions BLOB,"
        "expire_timeout INTEGER,"
        "is_read INTEGER NOT NULL DEFAULT FALSE"
        ");", NULL, NULL, NULL);

    sqlite3_stmt *stmt = NULL;
    const gchar *sql = "SELECT id, timestamp, app_name, icon_id, summary, body, is_read FROM notifications ORDER BY timestamp DESC LIMIT 40;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        GList *new_list = NULL;

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            MwbNotification *n = g_new0(MwbNotification, 1);
            const gchar *c_id = (const gchar *)sqlite3_column_text(stmt, 0);
            gint64 ts = sqlite3_column_int64(stmt, 1);
            const gchar *c_app = (const gchar *)sqlite3_column_text(stmt, 2);
            const gchar *c_icon = (const gchar *)sqlite3_column_text(stmt, 3);
            const gchar *c_summary = (const gchar *)sqlite3_column_text(stmt, 4);
            const gchar *c_body = (const gchar *)sqlite3_column_text(stmt, 5);
            gint is_read = sqlite3_column_int(stmt, 6);

            n->id = g_strdup(c_id ? c_id : "");
            n->timestamp = ts;
            n->app_name = g_strdup(c_app ? c_app : _("System"));
            n->icon_name = g_strdup(c_icon ? c_icon : "dialog-information");
            n->summary = g_strdup(c_summary ? c_summary : "");
            n->body = g_strdup(c_body ? c_body : "");
            n->is_read = is_read ? TRUE : FALSE;

            new_list = g_list_append(new_list, n);
        }
        sqlite3_finalize(stmt);

        g_list_free_full(mwb->notifications, (GDestroyNotify)mwb_notification_free);
        mwb->notifications = new_list;
    }
    sqlite3_close(db);
    g_free(db_path);
#endif
}

void
mwb_notification_add(MorphosWorkbenchPlugin *mwb,
                     const gchar *app_name,
                     const gchar *icon_name,
                     const gchar *summary,
                     const gchar *body_text)
{
    if (!mwb || !summary || !*summary)
        return;

    gint64 now = g_get_real_time() / G_USEC_PER_SEC;
    gchar *uid = g_strdup_printf("%" G_GINT64_FORMAT "-%u", now, g_random_int_range(1000, 9999));

#ifdef HAVE_SQLITE3
    gchar *dir = g_build_filename(g_get_user_cache_dir(), "xfce4", "notifyd", NULL);
    g_mkdir_with_parents(dir, 0755);
    gchar *db_path = g_build_filename(dir, "log.sqlite", NULL);
    g_free(dir);

    sqlite3 *db = NULL;
    if (sqlite3_open(db_path, &db) == SQLITE_OK) {
        sqlite3_exec(db,
            "CREATE TABLE IF NOT EXISTS notifications ("
            "id TEXT PRIMARY KEY NOT NULL,"
            "timestamp INTEGER NOT NULL,"
            "tz_identifier TEXT NOT NULL,"
            "app_id TEXT,"
            "app_name TEXT,"
            "icon_id TEXT,"
            "summary TEXT,"
            "body TEXT,"
            "actions BLOB,"
            "expire_timeout INTEGER,"
            "is_read INTEGER NOT NULL DEFAULT FALSE"
            ");", NULL, NULL, NULL);

        sqlite3_stmt *stmt = NULL;
        const gchar *sql = "INSERT OR REPLACE INTO notifications (id, timestamp, tz_identifier, app_id, app_name, icon_id, summary, body, expire_timeout, is_read) VALUES (?, ?, 'UTC', ?, ?, ?, ?, ?, 5000, 0);";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
            const gchar *app = (app_name && *app_name) ? app_name : "System";
            const gchar *icon = (icon_name && *icon_name) ? icon_name : "dialog-information";
            sqlite3_bind_text(stmt, 1, uid, -1, SQLITE_TRANSIENT);
            sqlite3_bind_int64(stmt, 2, now);
            sqlite3_bind_text(stmt, 3, app, -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 4, app, -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 5, icon, -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 6, summary, -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 7, body_text ? body_text : "", -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        sqlite3_close(db);
    }
    g_free(db_path);
#endif
    g_free(uid);

    mwb_notifications_refresh(mwb);
}

static void
mwb_notification_dismiss_clicked(GtkWidget *btn, gpointer data)
{
    MwbNotification *target = data;
    MorphosWorkbenchPlugin *mwb = g_object_get_data(G_OBJECT(btn), "mwb-plugin");
    if (!mwb || !target)
        return;

#ifdef HAVE_SQLITE3
    gchar *db_path = g_build_filename(g_get_user_cache_dir(), "xfce4", "notifyd", "log.sqlite", NULL);
    if (g_file_test(db_path, G_FILE_TEST_EXISTS)) {
        sqlite3 *db = NULL;
        if (sqlite3_open(db_path, &db) == SQLITE_OK) {
            sqlite3_stmt *stmt = NULL;
            const gchar *sql = "DELETE FROM notifications WHERE id = ?;";
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, target->id, -1, SQLITE_STATIC);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);
            }
            sqlite3_close(db);
        }
    }
    g_free(db_path);
#endif

    mwb->notifications = g_list_remove(mwb->notifications, target);
    mwb_notification_free(target);
    mwb_notifications_refresh(mwb);
}

static void
mwb_notifications_clear_all(GtkWidget *btn G_GNUC_UNUSED, gpointer data)
{
    MorphosWorkbenchPlugin *mwb = data;
    if (!mwb)
        return;

#ifdef HAVE_SQLITE3
    gchar *db_path = g_build_filename(g_get_user_cache_dir(), "xfce4", "notifyd", "log.sqlite", NULL);
    if (g_file_test(db_path, G_FILE_TEST_EXISTS)) {
        sqlite3 *db = NULL;
        if (sqlite3_open(db_path, &db) == SQLITE_OK) {
            sqlite3_exec(db, "DELETE FROM notifications;", NULL, NULL, NULL);
            sqlite3_close(db);
        }
    }
    g_free(db_path);
#endif

    g_list_free_full(mwb->notifications, (GDestroyNotify)mwb_notification_free);
    mwb->notifications = NULL;
    mwb_notifications_refresh(mwb);
}

static void
mwb_notifications_launch_settings(GtkWidget *btn G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED)
{
    if (!g_spawn_command_line_async("xfce4-notifyd-config", NULL))
        g_spawn_command_line_async("xfce4-settings-manager", NULL);
}

static const gchar *
mwb_resolve_notification_icon(const gchar *app_name, const gchar *icon_name)
{
    GtkIconTheme *theme = gtk_icon_theme_get_default();
    if (!theme)
        return "notification-symbolic";

    if (icon_name && *icon_name && gtk_icon_theme_has_icon(theme, icon_name))
        return icon_name;

    if (app_name && *app_name) {
        gchar *lower = g_ascii_strdown(app_name, -1);
        if (gtk_icon_theme_has_icon(theme, lower)) {
            static gchar cached[64];
            g_strlcpy(cached, lower, sizeof(cached));
            g_free(lower);
            return cached;
        }
        g_free(lower);
    }

    if (gtk_icon_theme_has_icon(theme, "notification-symbolic"))
        return "notification-symbolic";
    if (gtk_icon_theme_has_icon(theme, "org.xfce.notification"))
        return "org.xfce.notification";
    return "dialog-information";
}

void
mwb_notifications_refresh(MorphosWorkbenchPlugin *mwb)
{
    if (!mwb)
        return;

    mwb_notifications_load_sqlite(mwb);

    guint count = g_list_length(mwb->notifications);

    if (mwb->notify_badge) {
        if (count > 0) {
            gchar *cnt_str = g_strdup_printf("%u", count);
            gtk_label_set_text(GTK_LABEL(mwb->notify_badge), cnt_str);
            g_free(cnt_str);
            gtk_widget_show(mwb->notify_badge);
        } else {
            gtk_widget_hide(mwb->notify_badge);
        }
    }

    if (mwb->notify_button) {
        gchar *tip = count > 0 ?
            g_strdup_printf(_("Notifications (%u items)"), count) :
            g_strdup(_("Notifications (none)"));
        gtk_widget_set_tooltip_text(mwb->notify_button, tip);
        g_free(tip);
    }

    if (mwb->notify_pop_badge) {
        gchar *cnt_str = (count > 0) ? g_strdup_printf(_("%u items"), count) : g_strdup(_("Empty"));
        gtk_label_set_text(GTK_LABEL(mwb->notify_pop_badge), cnt_str);
        g_free(cnt_str);
        GtkStyleContext *sc_b = gtk_widget_get_style_context(mwb->notify_pop_badge);
        if (count > 0)
            gtk_style_context_add_class(sc_b, "mwb-badge-playing");
        else
            gtk_style_context_remove_class(sc_b, "mwb-badge-playing");
    }

    if (!mwb->notify_list_box)
        return;

    /* Rebuild cards in the notification popover */
    GList *children = gtk_container_get_children(GTK_CONTAINER(mwb->notify_list_box));
    g_list_free_full(children, (GDestroyNotify)gtk_widget_destroy);

    if (!mwb->notifications) {
        GtkWidget *empty_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_widget_set_margin_top(empty_box, 30);
        gtk_widget_set_margin_bottom(empty_box, 30);
        gtk_widget_set_halign(empty_box, GTK_ALIGN_CENTER);

        GtkWidget *empty_img = gtk_image_new_from_icon_name("notification-symbolic", GTK_ICON_SIZE_DIALOG);
        gtk_image_set_pixel_size(GTK_IMAGE(empty_img), 36);
        gtk_widget_set_opacity(empty_img, 0.45);
        gtk_box_pack_start(GTK_BOX(empty_box), empty_img, FALSE, FALSE, 0);

        GtkWidget *empty_title = gtk_label_new(_("No Notifications"));
        gtk_style_context_add_class(gtk_widget_get_style_context(empty_title), "mwb-notify-summary");
        gtk_widget_set_opacity(empty_title, 0.65);
        gtk_box_pack_start(GTK_BOX(empty_box), empty_title, FALSE, FALSE, 0);

        GtkWidget *empty_sub = gtk_label_new(_("You're all caught up."));
        gtk_style_context_add_class(gtk_widget_get_style_context(empty_sub), "mwb-notify-time");
        gtk_widget_set_opacity(empty_sub, 0.5);
        gtk_box_pack_start(GTK_BOX(empty_box), empty_sub, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(mwb->notify_list_box), empty_box, TRUE, TRUE, 0);
        gtk_widget_show_all(mwb->notify_list_box);
        if (mwb->notify_popup && gtk_widget_get_visible(mwb->notify_popup))
            gtk_window_resize(GTK_WINDOW(mwb->notify_popup), 1, 1);
        return;
    }

    GList *l;
    for (l = mwb->notifications; l != NULL; l = l->next) {
        MwbNotification *n = l->data;

        GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
        gtk_style_context_add_class(gtk_widget_get_style_context(card), "mwb-vol-card");
        gtk_style_context_add_class(gtk_widget_get_style_context(card), "mwb-notify-card");
        gtk_container_set_border_width(GTK_CONTAINER(card), 8);

        /* Top header row: App Icon, App Name, Time ago, Dismiss button */
        GtkWidget *top_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        const gchar *icon_name = mwb_resolve_notification_icon(n->app_name, n->icon_name);
        GtkWidget *icon = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_MENU);
        gtk_image_set_pixel_size(GTK_IMAGE(icon), 18);
        gtk_box_pack_start(GTK_BOX(top_row), icon, FALSE, FALSE, 0);

        GtkWidget *app_lbl = gtk_label_new(n->app_name ? n->app_name : _("System"));
        gtk_style_context_add_class(gtk_widget_get_style_context(app_lbl), "mwb-notify-app-name");
        gtk_label_set_xalign(GTK_LABEL(app_lbl), 0.0);
        gtk_box_pack_start(GTK_BOX(top_row), app_lbl, TRUE, TRUE, 0);

        gchar *time_str = mwb_format_time_ago(n->timestamp);
        GtkWidget *time_lbl = gtk_label_new(time_str);
        g_free(time_str);
        gtk_style_context_add_class(gtk_widget_get_style_context(time_lbl), "mwb-notify-time");
        gtk_box_pack_start(GTK_BOX(top_row), time_lbl, FALSE, FALSE, 0);

        GtkWidget *dismiss_btn = gtk_button_new();
        gtk_button_set_relief(GTK_BUTTON(dismiss_btn), GTK_RELIEF_NONE);
        GtkWidget *close_img = gtk_image_new_from_icon_name("window-close-symbolic", GTK_ICON_SIZE_MENU);
        gtk_image_set_pixel_size(GTK_IMAGE(close_img), 12);
        gtk_container_add(GTK_CONTAINER(dismiss_btn), close_img);
        gtk_style_context_add_class(gtk_widget_get_style_context(dismiss_btn), "mwb-notify-dismiss");
        gtk_widget_set_tooltip_text(dismiss_btn, _("Dismiss"));
        g_object_set_data(G_OBJECT(dismiss_btn), "mwb-plugin", mwb);
        g_signal_connect(dismiss_btn, "clicked", G_CALLBACK(mwb_notification_dismiss_clicked), n);
        gtk_box_pack_start(GTK_BOX(top_row), dismiss_btn, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(card), top_row, FALSE, FALSE, 0);

        /* Summary */
        if (n->summary && *n->summary) {
            GtkWidget *sum_lbl = gtk_label_new(n->summary);
            gtk_style_context_add_class(gtk_widget_get_style_context(sum_lbl), "mwb-notify-summary");
            gtk_label_set_xalign(GTK_LABEL(sum_lbl), 0.0);
            gtk_label_set_line_wrap(GTK_LABEL(sum_lbl), TRUE);
            gtk_box_pack_start(GTK_BOX(card), sum_lbl, FALSE, FALSE, 0);
        }

        /* Body */
        if (n->body && *n->body) {
            GtkWidget *body_lbl = gtk_label_new(n->body);
            gtk_style_context_add_class(gtk_widget_get_style_context(body_lbl), "mwb-notify-body");
            gtk_label_set_xalign(GTK_LABEL(body_lbl), 0.0);
            gtk_label_set_line_wrap(GTK_LABEL(body_lbl), TRUE);
            gtk_box_pack_start(GTK_BOX(card), body_lbl, FALSE, FALSE, 0);
        }

        gtk_box_pack_start(GTK_BOX(mwb->notify_list_box), card, FALSE, FALSE, 0);
    }

    gtk_widget_show_all(mwb->notify_list_box);
}

typedef struct {
    MorphosWorkbenchPlugin *mwb;
    gchar *app_name;
    gchar *icon_name;
    gchar *summary;
    gchar *body;
} MwbNotifyData;

static gboolean
mwb_notification_add_idle(gpointer data)
{
    MwbNotifyData *nd = data;
    if (nd) {
        mwb_notification_add(nd->mwb, nd->app_name, nd->icon_name, nd->summary, nd->body);
        g_free(nd->app_name);
        g_free(nd->icon_name);
        g_free(nd->summary);
        g_free(nd->body);
        g_free(nd);
    }
    return G_SOURCE_REMOVE;
}

static GDBusMessage *
mwb_notify_dbus_filter(GDBusConnection *connection G_GNUC_UNUSED,
                       GDBusMessage *message,
                       gboolean incoming G_GNUC_UNUSED,
                       gpointer user_data)
{
    MorphosWorkbenchPlugin *mwb = user_data;
    if (!mwb)
        return message;

    if (g_dbus_message_get_message_type(message) == G_DBUS_MESSAGE_TYPE_METHOD_CALL) {
        const gchar *member = g_dbus_message_get_member(message);
        const gchar *interface = g_dbus_message_get_interface(message);
        if (g_strcmp0(member, "Notify") == 0 &&
            (interface == NULL || g_strcmp0(interface, "org.freedesktop.Notifications") == 0)) {
            GVariant *body = g_dbus_message_get_body(message);
            if (body && g_variant_n_children(body) >= 5) {
                GVariant *v_app = g_variant_get_child_value(body, 0);
                GVariant *v_icon = g_variant_get_child_value(body, 2);
                GVariant *v_sum = g_variant_get_child_value(body, 3);
                GVariant *v_body = g_variant_get_child_value(body, 4);

                const gchar *app_name = (v_app && g_variant_is_of_type(v_app, G_VARIANT_TYPE_STRING)) ? g_variant_get_string(v_app, NULL) : NULL;
                const gchar *app_icon = (v_icon && g_variant_is_of_type(v_icon, G_VARIANT_TYPE_STRING)) ? g_variant_get_string(v_icon, NULL) : NULL;
                const gchar *summary = (v_sum && g_variant_is_of_type(v_sum, G_VARIANT_TYPE_STRING)) ? g_variant_get_string(v_sum, NULL) : NULL;
                const gchar *body_text = (v_body && g_variant_is_of_type(v_body, G_VARIANT_TYPE_STRING)) ? g_variant_get_string(v_body, NULL) : NULL;

                if (summary && *summary) {
                    MwbNotifyData *nd = g_new0(MwbNotifyData, 1);
                    nd->mwb = mwb;
                    nd->app_name = g_strdup(app_name);
                    nd->icon_name = g_strdup(app_icon);
                    nd->summary = g_strdup(summary);
                    nd->body = g_strdup(body_text);
                    g_idle_add(mwb_notification_add_idle, nd);
                }

                if (v_app) g_variant_unref(v_app);
                if (v_icon) g_variant_unref(v_icon);
                if (v_sum) g_variant_unref(v_sum);
                if (v_body) g_variant_unref(v_body);
            }
        }
    }
    return message;
}

static void
mwb_notify_dbus_signal(GDBusConnection *conn G_GNUC_UNUSED,
                       const gchar *sender_name G_GNUC_UNUSED,
                       const gchar *object_path G_GNUC_UNUSED,
                       const gchar *interface_name G_GNUC_UNUSED,
                       const gchar *signal_name G_GNUC_UNUSED,
                       GVariant *parameters G_GNUC_UNUSED,
                       gpointer user_data)
{
    MorphosWorkbenchPlugin *mwb = user_data;
    mwb_notifications_refresh(mwb);
}

static gboolean
mwb_tick_notifications(MorphosWorkbenchPlugin *mwb)
{
    mwb_notifications_refresh(mwb);
    return G_SOURCE_CONTINUE;
}

static void
mwb_notify_clicked(GtkWidget *widget G_GNUC_UNUSED, gpointer data)
{
    MorphosWorkbenchPlugin *mwb = data;

    if (mwb->notify_popup && gtk_widget_get_visible(mwb->notify_popup)) {
        mwb_popup_hide(mwb, mwb->notify_popup);
        return;
    }

    if (!mwb->notify_popup) {
        GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_decorated(GTK_WINDOW(win), FALSE);
        gtk_window_set_skip_taskbar_hint(GTK_WINDOW(win), TRUE);
        gtk_window_set_skip_pager_hint(GTK_WINDOW(win), TRUE);
        gtk_window_set_type_hint(GTK_WINDOW(win), GDK_WINDOW_TYPE_HINT_POPUP_MENU);
        gtk_window_set_resizable(GTK_WINDOW(win), FALSE);
        gtk_widget_set_size_request(win, 320, -1);
        GdkScreen *screen = gtk_widget_get_screen(win);
        GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
        if (visual)
            gtk_widget_set_visual(win, visual);
        gtk_widget_set_app_paintable(win, TRUE);

        gtk_style_context_add_class(gtk_widget_get_style_context(win), "mwb-popup");
        gtk_style_context_add_class(gtk_widget_get_style_context(win), "mwb-vol-popup");
        gtk_widget_set_name(win, "mwb-notify-pop");

        g_signal_connect(win, "draw", G_CALLBACK(mwb_popup_draw), NULL);
        g_signal_connect(win, "button-press-event", G_CALLBACK(mwb_popup_button_press), mwb);
        g_signal_connect(win, "focus-out-event", G_CALLBACK(mwb_popup_focus_out), mwb);
        g_signal_connect(win, "key-press-event", G_CALLBACK(mwb_popup_key_press), mwb);
        g_signal_connect(win, "delete-event", G_CALLBACK(gtk_widget_hide_on_delete), NULL);
        g_signal_connect(win, "destroy", G_CALLBACK(gtk_widget_destroyed), &mwb->notify_popup);

        GtkWidget *root_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_container_set_border_width(GTK_CONTAINER(root_box), 10);
        gtk_container_add(GTK_CONTAINER(win), root_box);

        /* Header: Icon + Title + State Badge + Small Clear All Button + Small Settings Button */
        GtkWidget *hdr = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        GtkWidget *bell_img = gtk_image_new_from_icon_name("notification-symbolic", GTK_ICON_SIZE_MENU);
        gtk_image_set_pixel_size(GTK_IMAGE(bell_img), 16);
        gtk_style_context_add_class(gtk_widget_get_style_context(bell_img), "mwb-pop-icon");
        gtk_box_pack_start(GTK_BOX(hdr), bell_img, FALSE, FALSE, 0);

        GtkWidget *title_lbl = gtk_label_new(_("Notifications"));
        gtk_label_set_xalign(GTK_LABEL(title_lbl), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(title_lbl), "mwb-pop-title");
        gtk_box_pack_start(GTK_BOX(hdr), title_lbl, TRUE, TRUE, 0);

        guint count = g_list_length(mwb->notifications);
        gchar *cnt_str = (count > 0) ? g_strdup_printf(_("%u items"), count) : g_strdup(_("Empty"));
        mwb->notify_pop_badge = gtk_label_new(cnt_str);
        g_free(cnt_str);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->notify_pop_badge), "mwb-vol-badge");
        if (count > 0)
            gtk_style_context_add_class(gtk_widget_get_style_context(mwb->notify_pop_badge), "mwb-badge-playing");
        gtk_box_pack_start(GTK_BOX(hdr), mwb->notify_pop_badge, FALSE, FALSE, 0);

        /* Small MorphOS Clear All Button */
        GtkWidget *clear_btn = gtk_button_new();
        gtk_button_set_relief(GTK_BUTTON(clear_btn), GTK_RELIEF_NONE);
        gtk_style_context_add_class(gtk_widget_get_style_context(clear_btn), "mwb-media-btn");
        gtk_style_context_add_class(gtk_widget_get_style_context(clear_btn), "mwb-gear-btn");
        GtkWidget *clear_img = gtk_image_new_from_icon_name("edit-clear-all-symbolic", GTK_ICON_SIZE_MENU);
        gtk_image_set_pixel_size(GTK_IMAGE(clear_img), 14);
        gtk_container_add(GTK_CONTAINER(clear_btn), clear_img);
        gtk_widget_set_tooltip_text(clear_btn, _("Clear all notifications"));
        g_signal_connect(clear_btn, "clicked", G_CALLBACK(mwb_notifications_clear_all), mwb);
        gtk_box_pack_start(GTK_BOX(hdr), clear_btn, FALSE, FALSE, 0);

        /* Small MorphOS Settings Button */
        GtkWidget *settings_btn = gtk_button_new();
        gtk_button_set_relief(GTK_BUTTON(settings_btn), GTK_RELIEF_NONE);
        gtk_style_context_add_class(gtk_widget_get_style_context(settings_btn), "mwb-media-btn");
        gtk_style_context_add_class(gtk_widget_get_style_context(settings_btn), "mwb-gear-btn");
        GtkWidget *gear_img = gtk_image_new_from_icon_name("preferences-system-symbolic", GTK_ICON_SIZE_MENU);
        gtk_image_set_pixel_size(GTK_IMAGE(gear_img), 14);
        gtk_container_add(GTK_CONTAINER(settings_btn), gear_img);
        gtk_widget_set_tooltip_text(settings_btn, _("Notification Settings..."));
        g_signal_connect(settings_btn, "clicked", G_CALLBACK(mwb_notifications_launch_settings), mwb);
        gtk_box_pack_start(GTK_BOX(hdr), settings_btn, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(root_box), hdr, FALSE, FALSE, 0);

        /* Scrolled notification list */
        GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
        gtk_style_context_add_class(gtk_widget_get_style_context(scrolled), "mwb-notify-scroll");
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                       GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_propagate_natural_height(GTK_SCROLLED_WINDOW(scrolled), TRUE);
        gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrolled), 90);
        gtk_scrolled_window_set_max_content_height(GTK_SCROLLED_WINDOW(scrolled), 380);

        mwb->notify_list_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_container_add(GTK_CONTAINER(scrolled), mwb->notify_list_box);
        gtk_box_pack_start(GTK_BOX(root_box), scrolled, TRUE, TRUE, 0);

        mwb->notify_popup = win;
        mwb_theme_widget(mwb, win);
    }

    mwb_notifications_refresh(mwb);
    mwb_popup_show(mwb, mwb->notify_popup, mwb->notify_button);
}

void
mwb_init_notification_monitor(MorphosWorkbenchPlugin *mwb)
{
    if (!mwb || mwb->notify_mon_conn)
        return;

    gchar *address = g_dbus_address_get_for_bus_sync(G_BUS_TYPE_SESSION, NULL, NULL);
    if (address) {
        GDBusConnection *mon = g_dbus_connection_new_for_address_sync(
            address,
            G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT | G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
            NULL,
            NULL,
            NULL);
        g_free(address);

        if (mon) {
            mwb->notify_mon_conn = mon;
            mwb->notify_filter_id = g_dbus_connection_add_filter(
                mon,
                mwb_notify_dbus_filter,
                mwb,
                NULL);

            GVariantBuilder b;
            g_variant_builder_init(&b, G_VARIANT_TYPE("as"));
            g_variant_builder_add(&b, "s", "type='method_call',member='Notify'");
            g_variant_builder_add(&b, "s", "type='method_call',interface='org.freedesktop.Notifications'");
            g_variant_builder_add(&b, "s", "type='method_call',path='/org/freedesktop/Notifications'");

            GVariant *res = g_dbus_connection_call_sync(
                mon,
                "org.freedesktop.DBus",
                "/org/freedesktop/DBus",
                "org.freedesktop.DBus.Monitoring",
                "BecomeMonitor",
                g_variant_new("(@asu)", g_variant_builder_end(&b), (guint32)0),
                NULL,
                G_DBUS_CALL_FLAGS_NONE,
                -1,
                NULL,
                NULL);
            if (res)
                g_variant_unref(res);
        }
    }
}

static GtkWidget *
mwb_build_notifications(MorphosWorkbenchPlugin *mwb)
{
    mwb->notify_button = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(mwb->notify_button), GTK_RELIEF_NONE);
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->notify_button), "mwb-volbutton");
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->notify_button), "mwb-notify-btn");
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->notify_button), "mwb-screenbar-pill");
    gtk_widget_set_valign(mwb->notify_button, GTK_ALIGN_CENTER);

    GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
    mwb->notify_icon = gtk_image_new_from_icon_name("notification-symbolic", GTK_ICON_SIZE_MENU);
    gtk_image_set_pixel_size(GTK_IMAGE(mwb->notify_icon), 16);
    gtk_box_pack_start(GTK_BOX(btn_box), mwb->notify_icon, FALSE, FALSE, 0);

    mwb->notify_badge = gtk_label_new("");
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->notify_badge), "mwb-badge");
    gtk_widget_set_no_show_all(mwb->notify_badge, TRUE);
    gtk_box_pack_start(GTK_BOX(btn_box), mwb->notify_badge, FALSE, FALSE, 0);

    gtk_container_add(GTK_CONTAINER(mwb->notify_button), btn_box);
    g_signal_connect(mwb->notify_button, "clicked", G_CALLBACK(mwb_notify_clicked), mwb);

    /* Setup D-Bus notification monitor */
    mwb_init_notification_monitor(mwb);

    /* Also subscribe to D-Bus notification signals on main session bus */
    GDBusConnection *conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
    if (conn) {
        mwb->notify_dbus_id = g_dbus_connection_signal_subscribe(
            conn,
            NULL,
            "org.freedesktop.Notifications",
            NULL,
            NULL,
            NULL,
            G_DBUS_SIGNAL_FLAGS_NONE,
            mwb_notify_dbus_signal,
            mwb,
            NULL);
    }

    mwb->notify_poll_id = g_timeout_add_seconds(3, (GSourceFunc)mwb_tick_notifications, mwb);
    mwb_notifications_refresh(mwb);

    return mwb->notify_button;
}

static GtkWidget *
mwb_build_clock(MorphosWorkbenchPlugin *mwb)
{
    mwb->clock_button = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(mwb->clock_button), GTK_RELIEF_NONE);
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->clock_button), "mwb-volbutton");
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->clock_button), "mwb-clockbtn");
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->clock_button), "mwb-screenbar-pill");
    gtk_widget_set_valign(mwb->clock_button, GTK_ALIGN_CENTER);

    mwb->clock_label = gtk_label_new("");
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->clock_label), "mwb-clocks");
    gtk_container_add(GTK_CONTAINER(mwb->clock_button), mwb->clock_label);
    gtk_widget_show(mwb->clock_label);
    g_signal_connect(mwb->clock_button, "clicked", G_CALLBACK(mwb_clock_clicked), mwb);
    return mwb->clock_button;
}

static gboolean
mwb_widget_enabled(MorphosWorkbenchPlugin *mwb, MorphosWorkbenchWidget w)
{
    switch (w) {
    case MWB_WIDGET_DRIVELAMPS:   return mwb->show_drivelamps;
    case MWB_WIDGET_NETLAMPS:     return mwb->show_netlamps;
    case MWB_WIDGET_WIFI:         return mwb->show_wifi;
    case MWB_WIDGET_BATTERY:      return mwb->show_battery;
    case MWB_WIDGET_CPU:          return mwb->show_cpumbar;
    case MWB_WIDGET_MEM:          return mwb->show_membar;
    case MWB_WIDGET_DISK:         return mwb->show_diskgauge;
    case MWB_WIDGET_SYSINFO:      return mwb->show_sysinfo;
    case MWB_WIDGET_NOTIFICATIONS:return mwb->show_notifications;
    case MWB_WIDGET_VOLUME:       return mwb->show_volume;
    case MWB_WIDGET_CLOCK:        return mwb->show_clock;
    default:                      return FALSE;
    }
}

static GtkWidget *
mwb_build_widget(MorphosWorkbenchPlugin *mwb, MorphosWorkbenchWidget w)
{
    switch (w) {
    case MWB_WIDGET_DRIVELAMPS:   return mwb_build_drivelamps(mwb);
    case MWB_WIDGET_NETLAMPS:     return mwb_build_netlamps(mwb);
    case MWB_WIDGET_WIFI:         return mwb_build_wifi(mwb);
    case MWB_WIDGET_BATTERY:      return mwb_build_battery(mwb);
    case MWB_WIDGET_CPU:          return mwb_build_cpu(mwb);
    case MWB_WIDGET_MEM:          return mwb_build_mem(mwb);
    case MWB_WIDGET_DISK:         return mwb_build_diskgauge(mwb);
    case MWB_WIDGET_SYSINFO:      return mwb_build_sysinfo(mwb);
    case MWB_WIDGET_NOTIFICATIONS:return mwb_build_notifications(mwb);
    case MWB_WIDGET_VOLUME:       return mwb_build_volume(mwb);
    case MWB_WIDGET_CLOCK:        return mwb_build_clock(mwb);
    default:                      return NULL;
    }
}

void
mwb_build_screenbar(MorphosWorkbenchPlugin *mwb)
{
    GtkWidget *right = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_style_context_add_class(gtk_widget_get_style_context(right), "mwb-hbox");
    mwb->screenbar = right;

    gint i;
    for (i = 0; i < MWB_WIDGET_COUNT; i++) {
        MorphosWorkbenchWidget w = (MorphosWorkbenchWidget)mwb->widget_order[i];
        GtkWidget *wgt = mwb_build_widget(mwb, w);
        if (wgt) {
            GtkWidget *slot = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
            gtk_box_pack_start(GTK_BOX(slot), wgt, FALSE, FALSE, 0);
            gtk_box_pack_start(GTK_BOX(slot), mwb_screenbar_divider(), FALSE, FALSE, 0);
            gtk_box_pack_start(GTK_BOX(right), slot, FALSE, FALSE, 0);
            gtk_widget_show_all(slot);
            gtk_widget_set_visible(slot, mwb_widget_enabled(mwb, w));
            mwb->widget_widgets[w] = slot;
        }
    }

    gtk_box_pack_end(GTK_BOX(mwb->bar), right, FALSE, FALSE, 0);
    gtk_widget_show_all(right);
    mwb_apply_widget_order(mwb);

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
    if (mwb->show_volume) {
        mwb_volume_refresh(mwb);
        mwb->vol_timeout = g_timeout_add_seconds(5, (GSourceFunc)mwb_tick_volume, mwb);
    }
}

void
mwb_apply_widget_order(MorphosWorkbenchPlugin *mwb)
{
    GtkWidget *right = mwb->screenbar;
    gint i, pos = 0;

    if (!right)
        return;

    for (i = 0; i < MWB_WIDGET_COUNT; i++) {
        MorphosWorkbenchWidget w = (MorphosWorkbenchWidget)mwb->widget_order[i];
        GtkWidget *slot = mwb->widget_widgets[w];

        if (!slot)
            continue;

        gtk_widget_show_all(slot);
        gtk_widget_set_visible(slot, mwb_widget_enabled(mwb, w));
        gtk_box_reorder_child(GTK_BOX(right), slot, pos);
        pos++;
    }
}
