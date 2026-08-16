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
 *  Config
 *  ------------------------------------------------------------------ */

static gchar *
mwb_widget_order_to_string(MorphosWorkbenchPlugin *mwb)
{
    GString *s = g_string_new(NULL);
    gint i;
    for (i = 0; i < MWB_WIDGET_COUNT; i++) {
        if (i)
            g_string_append_c(s, ',');
        g_string_append_printf(s, "%d", mwb->widget_order[i]);
    }
    return g_string_free(s, FALSE);
}

static void
mwb_widget_order_from_string(MorphosWorkbenchPlugin *mwb, const gchar *str)
{
    gchar **toks = g_strsplit(str ? str : "", ",", -1);
    gboolean seen[MWB_WIDGET_COUNT] = { FALSE };
    gint i, w, n = 0;

    for (i = 0; toks[i] && n < MWB_WIDGET_COUNT; i++) {
        gint v = atoi(toks[i]);
        if (v >= 0 && v < MWB_WIDGET_COUNT && !seen[v]) {
            mwb->widget_order[n++] = v;
            seen[v] = TRUE;
        }
    }
    for (w = 0; w < MWB_WIDGET_COUNT && n < MWB_WIDGET_COUNT; w++) {
        if (!seen[w])
            mwb->widget_order[n++] = w;
    }
    g_strfreev(toks);
}

void
mwb_save_config(XfcePanelPlugin *plugin, MorphosWorkbenchPlugin *mwb)
{
    gchar *file = xfce_panel_plugin_save_location(plugin, TRUE);
    if (!file)
        return;
    XfceRc *rc = xfce_rc_simple_open(file, FALSE);
    g_free(file);
    if (!rc)
        return;
    xfce_rc_write_int_entry(rc, "theme", mwb->theme);
    xfce_rc_write_int_entry(rc, "logo_variant", mwb->logo_variant);
    xfce_rc_write_int_entry(rc, "gauge_style", mwb->gauge_style);
    xfce_rc_write_bool_entry(rc, "override_theme", mwb->override_theme);
    xfce_rc_write_bool_entry(rc, "show_clock", mwb->show_clock);
    xfce_rc_write_bool_entry(rc, "show_membar", mwb->show_membar);
    xfce_rc_write_bool_entry(rc, "show_cpumbar", mwb->show_cpumbar);
    xfce_rc_write_bool_entry(rc, "show_diskgauge", mwb->show_diskgauge);
    xfce_rc_write_bool_entry(rc, "show_netlamps", mwb->show_netlamps);
    xfce_rc_write_bool_entry(rc, "show_drivelamps", mwb->show_drivelamps);
    xfce_rc_write_bool_entry(rc, "show_volume", mwb->show_volume);
    xfce_rc_write_bool_entry(rc, "show_wifi", mwb->show_wifi);
    xfce_rc_write_bool_entry(rc, "show_battery", mwb->show_battery);
    xfce_rc_write_bool_entry(rc, "show_sysinfo", mwb->show_sysinfo);
    xfce_rc_write_bool_entry(rc, "show_logo", mwb->show_logo);
    xfce_rc_write_bool_entry(rc, "show_title", mwb->show_title);
    xfce_rc_write_bool_entry(rc, "show_dynamic_title", mwb->show_dynamic_title);
    xfce_rc_write_bool_entry(rc, "show_workbench_menu", mwb->show_workbench_menu);
    xfce_rc_write_bool_entry(rc, "show_ambient_menu", mwb->show_ambient_menu);
    xfce_rc_write_bool_entry(rc, "show_icons_menu", mwb->show_icons_menu);
    xfce_rc_write_bool_entry(rc, "show_disk_menu", mwb->show_disk_menu);
    xfce_rc_write_bool_entry(rc, "show_applications_menu", mwb->show_applications_menu);
    {
        gchar *order = mwb_widget_order_to_string(mwb);
        xfce_rc_write_entry(rc, "widget_order", order);
        g_free(order);
    }
    xfce_rc_close(rc);
}

void
mwb_load_config(MorphosWorkbenchPlugin *mwb)
{
    gchar *file = xfce_panel_plugin_lookup_rc_file(mwb->plugin);
    if (!file)
        return;
    XfceRc *rc = xfce_rc_simple_open(file, TRUE);
    g_free(file);
    if (!rc)
        return;
    mwb->theme = xfce_rc_read_int_entry(rc, "theme", MWB_THEME_CLASSIC);
    if (mwb->theme < 0 || mwb->theme >= MWB_THEME_COUNT)
        mwb->theme = MWB_THEME_CLASSIC;
    mwb->logo_variant = xfce_rc_read_int_entry(rc, "logo_variant", MWB_LOGO_CLASSIC);
    if (mwb->logo_variant < 0 || mwb->logo_variant >= MWB_LOGO_COUNT)
        mwb->logo_variant = MWB_LOGO_CLASSIC;
    mwb->gauge_style = xfce_rc_read_int_entry(rc, "gauge_style", MWB_GAUGE_STYLE_INDUSTRIAL);
    if (mwb->gauge_style < 0 || mwb->gauge_style >= MWB_GAUGE_STYLE_COUNT)
        mwb->gauge_style = MWB_GAUGE_STYLE_INDUSTRIAL;
    mwb->override_theme = xfce_rc_read_bool_entry(rc, "override_theme", TRUE);
    mwb->show_clock = xfce_rc_read_bool_entry(rc, "show_clock", TRUE);
    mwb->show_membar = xfce_rc_read_bool_entry(rc, "show_membar", TRUE);
    mwb->show_cpumbar = xfce_rc_read_bool_entry(rc, "show_cpumbar", TRUE);
    mwb->show_diskgauge = xfce_rc_read_bool_entry(rc, "show_diskgauge", TRUE);
    mwb->show_netlamps = xfce_rc_read_bool_entry(rc, "show_netlamps", TRUE);
    mwb->show_drivelamps = xfce_rc_read_bool_entry(rc, "show_drivelamps", TRUE);
    mwb->show_volume = xfce_rc_read_bool_entry(rc, "show_volume", TRUE);
    mwb->show_wifi = xfce_rc_read_bool_entry(rc, "show_wifi", TRUE);
    mwb->show_battery = xfce_rc_read_bool_entry(rc, "show_battery", TRUE);
    mwb->show_sysinfo = xfce_rc_read_bool_entry(rc, "show_sysinfo", TRUE);
    mwb->show_logo = xfce_rc_read_bool_entry(rc, "show_logo", TRUE);
    mwb->show_title = xfce_rc_read_bool_entry(rc, "show_title", TRUE);
    mwb->show_dynamic_title = xfce_rc_read_bool_entry(rc, "show_dynamic_title", FALSE);
    mwb->show_workbench_menu = xfce_rc_read_bool_entry(rc, "show_workbench_menu", TRUE);
    mwb->show_ambient_menu = xfce_rc_read_bool_entry(rc, "show_ambient_menu", TRUE);
    mwb->show_icons_menu = xfce_rc_read_bool_entry(rc, "show_icons_menu", TRUE);
    mwb->show_disk_menu = xfce_rc_read_bool_entry(rc, "show_disk_menu", TRUE);
    mwb->show_applications_menu = xfce_rc_read_bool_entry(rc, "show_applications_menu", TRUE);
    {
        const gchar *order = xfce_rc_read_entry(rc, "widget_order", NULL);
        if (order)
            mwb_widget_order_from_string(mwb, order);
    }
    xfce_rc_close(rc);

    mwb_apply_theme(mwb);
    mwb_apply_logo(mwb);
    mwb_apply_left_visibility(mwb);
    mwb_apply_widget_order(mwb);
}

/* ------------------------------------------------------------------ *
 *  Settings dialog helpers (tabs, sections, descriptions)
 *  ------------------------------------------------------------------ */

static GtkWidget *
mwb_settings_section(GtkWidget *box, const gchar *title)
{
    GtkWidget *lbl = gtk_label_new(NULL);
    gchar *markup = g_markup_printf_escaped("<b>%s</b>", title);
    gtk_label_set_markup(GTK_LABEL(lbl), markup);
    g_free(markup);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
    gtk_widget_set_margin_top(lbl, 12);
    gtk_widget_set_margin_bottom(lbl, 2);
    gtk_box_pack_start(GTK_BOX(box), lbl, FALSE, FALSE, 0);
    return lbl;
}

static GtkWidget *
mwb_settings_desc(GtkWidget *box, const gchar *text)
{
    GtkWidget *lbl = gtk_label_new(text);
    PangoAttrList *attrs;

    gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
    gtk_label_set_line_wrap(GTK_LABEL(lbl), TRUE);
    gtk_widget_set_margin_start(lbl, 24);

    attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_scale_new(0.9));
    pango_attr_list_insert(attrs, pango_attr_foreground_new(0x8080, 0x8080, 0x8080));
    gtk_label_set_attributes(GTK_LABEL(lbl), attrs);
    pango_attr_list_unref(attrs);

    gtk_box_pack_start(GTK_BOX(box), lbl, FALSE, FALSE, 0);
    return lbl;
}

static GtkWidget *
mwb_settings_check(GtkWidget *box, const gchar *label, const gchar *desc, gboolean active)
{
    GtkWidget *check = gtk_check_button_new_with_label(label);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), active);
    gtk_box_pack_start(GTK_BOX(box), check, FALSE, FALSE, 0);
    if (desc)
        mwb_settings_desc(box, desc);
    return check;
}

static GtkWidget *
mwb_settings_combo(GtkWidget *box, const gchar *label, const gchar *desc)
{
    GtkWidget *lbl = gtk_label_new(label);
    GtkWidget *combo;

    gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
    gtk_box_pack_start(GTK_BOX(box), lbl, FALSE, FALSE, 0);

    combo = gtk_combo_box_text_new();
    gtk_widget_set_margin_start(combo, 24);
    gtk_box_pack_start(GTK_BOX(box), combo, FALSE, FALSE, 0);

    if (desc)
        mwb_settings_desc(box, desc);
    return combo;
}

static GtkWidget *
mwb_settings_page(GtkNotebook *notebook, const gchar *title)
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *sc;

    gtk_container_set_border_width(GTK_CONTAINER(box), 12);

    sc = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sc), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(sc), box);

    gtk_notebook_append_page(notebook, sc, gtk_label_new(title));
    return box;
}

/* Reorderable widget list (Up/Down buttons). */
typedef struct {
    GtkWidget *box;
    gint       order[MWB_WIDGET_COUNT];
} MwbOrderList;

static void mwb_order_list_rebuild(MwbOrderList *ol);

static void
mwb_order_move(MwbOrderList *ol, gint pos, gint delta)
{
    gint newpos = pos + delta;
    gint tmp;

    if (newpos < 0 || newpos >= MWB_WIDGET_COUNT)
        return;
    tmp = ol->order[pos];
    ol->order[pos] = ol->order[newpos];
    ol->order[newpos] = tmp;
    mwb_order_list_rebuild(ol);
}

static void
mwb_order_up(GtkButton *btn, MwbOrderList *ol)
{
    mwb_order_move(ol, GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "mwb-pos")), -1);
}

static void
mwb_order_down(GtkButton *btn, MwbOrderList *ol)
{
    mwb_order_move(ol, GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "mwb-pos")), +1);
}

static void
mwb_order_list_rebuild(MwbOrderList *ol)
{
    GList *children = gtk_container_get_children(GTK_CONTAINER(ol->box));
    GList *c;
    gint i;

    for (c = children; c; c = c->next)
        gtk_widget_destroy(GTK_WIDGET(c->data));
    g_list_free(children);

    for (i = 0; i < MWB_WIDGET_COUNT; i++) {
        guint id = ol->order[i];
        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        GtkWidget *lbl = gtk_label_new(_(mwb_widget_name(id)));
        GtkWidget *up, *down;

        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
        gtk_box_pack_start(GTK_BOX(row), lbl, TRUE, TRUE, 0);

        up = gtk_button_new_from_icon_name("go-up-symbolic", GTK_ICON_SIZE_MENU);
        gtk_button_set_relief(GTK_BUTTON(up), GTK_RELIEF_NONE);
        gtk_widget_set_sensitive(up, i > 0);
        g_object_set_data(G_OBJECT(up), "mwb-pos", GINT_TO_POINTER(i));
        g_signal_connect(up, "clicked", G_CALLBACK(mwb_order_up), ol);
        gtk_box_pack_start(GTK_BOX(row), up, FALSE, FALSE, 0);

        down = gtk_button_new_from_icon_name("go-down-symbolic", GTK_ICON_SIZE_MENU);
        gtk_button_set_relief(GTK_BUTTON(down), GTK_RELIEF_NONE);
        gtk_widget_set_sensitive(down, i < MWB_WIDGET_COUNT - 1);
        g_object_set_data(G_OBJECT(down), "mwb-pos", GINT_TO_POINTER(i));
        g_signal_connect(down, "clicked", G_CALLBACK(mwb_order_down), ol);
        gtk_box_pack_start(GTK_BOX(row), down, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(ol->box), row, FALSE, FALSE, 0);
        gtk_widget_show_all(row);
    }
}

void
mwb_configure_plugin(XfcePanelPlugin *plugin, MorphosWorkbenchPlugin *mwb)
{
    GtkWidget *dialog, *content_area, *notebook, *box;
    GtkWidget *theme_combo, *logo_combo, *gauge_combo, *ck_override;
    GtkWidget *ck_clock, *ck_mem, *ck_cpu, *ck_net, *ck_disk, *ck_vol, *ck_wifi, *ck_batt, *ck_sys, *ck_diskgauge;
    GtkWidget *ck_logo, *ck_title, *ck_dyntitle, *ck_wbmenu, *ck_ambmenu, *ck_iconsmenu, *ck_diskmenu, *ck_appsmenu;
    MwbOrderList order_list;
    gint i;

    xfce_panel_plugin_block_menu(plugin);

    dialog = xfce_titled_dialog_new_with_mixed_buttons(_("Workbench Settings"),
                                                 GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(plugin))),
                                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                                 "window-close",
                                                 _("_Close"), GTK_RESPONSE_OK,
                                                 "document-save",
                                                 _("_Apply"), GTK_RESPONSE_APPLY,
                                                 "process-stop",
                                                 _("_Cancel"), GTK_RESPONSE_CANCEL,
                                                 NULL);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_icon_name(GTK_WINDOW(dialog), MWB_ICON_NAME);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 580, 520);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    notebook = gtk_notebook_new();
    gtk_container_set_border_width(GTK_CONTAINER(notebook), 6);
    gtk_box_pack_start(GTK_BOX(content_area), notebook, TRUE, TRUE, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(dialog), "mwb-settings");

    /* ---- General ---- */
    box = mwb_settings_page(GTK_NOTEBOOK(notebook), _("General"));
    mwb_settings_section(box, _("Appearance"));
    theme_combo = mwb_settings_combo(box, _("Theme:"), _("Color scheme used across the Workbench."));
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(theme_combo), NULL, _("Classic (Ambient dark-blue)"));
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(theme_combo), NULL, _("Dark (sleek near-black)"));
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(theme_combo), NULL, _("Light (silver metal)"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(theme_combo), mwb->theme);

    logo_combo = mwb_settings_combo(box, _("Ambient logo:"), _("Shape of the Ambient emblem on the left."));
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(logo_combo), NULL, _("Classic sphere"));
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(logo_combo), NULL, _("Flat sphere"));
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(logo_combo), NULL, _("Monochrome sphere"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(logo_combo), mwb->logo_variant);

    gauge_combo = mwb_settings_combo(box, _("Gauge style:"), _("Visual style of the CPU, memory and disk meters."));
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(gauge_combo), NULL, _("Industrial (segmented blocks)"));
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(gauge_combo), NULL, _("Glossy 3D (chrome)"));
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(gauge_combo), NULL, _("Plain (flat square)"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(gauge_combo), mwb->gauge_style);

    mwb_settings_section(box, _("Behavior"));
    ck_override = mwb_settings_check(box, _("Override GTK theme"),
                                     _("Apply the Workbench's own style to every widget, independent of the desktop theme."),
                                     mwb->override_theme);

    /* ---- Workbench ---- */
    box = mwb_settings_page(GTK_NOTEBOOK(notebook), _("Workbench"));
    mwb_settings_section(box, _("Workbench"));
    ck_logo = mwb_settings_check(box, _("Ambient logo"),
                                 _("Show the Ambient emblem button."), mwb->show_logo);
    ck_title = mwb_settings_check(box, _("Workbench title"),
                                  _("Show the title label next to the emblem."), mwb->show_title);
    ck_dyntitle = mwb_settings_check(box, _("Dynamic title"),
                                     _("Show the foreground application's name instead of a static title."),
                                     mwb->show_dynamic_title);

    mwb_settings_section(box, _("Menus"));
    ck_wbmenu = mwb_settings_check(box, _("Workbench menu"),
                                   _("Home, documents, terminal, trash and refresh."), mwb->show_workbench_menu);
    ck_ambmenu = mwb_settings_check(box, _("Ambient menu"),
                                    _("System, task manager, settings and shortcuts."), mwb->show_ambient_menu);
    ck_iconsmenu = mwb_settings_check(box, _("Icons menu"),
                                      _("Desktop icon actions and folder creation."), mwb->show_icons_menu);
    ck_diskmenu = mwb_settings_check(box, _("Disk menu"),
                                     _("File manager, removable media and disk tools."), mwb->show_disk_menu);
    ck_appsmenu = mwb_settings_check(box, _("Applications menu"),
                                     _("All installed applications, categorized."), mwb->show_applications_menu);

    /* ---- Screenbar ---- */
    box = mwb_settings_page(GTK_NOTEBOOK(notebook), _("Screenbar"));
    mwb_settings_section(box, _("Meters"));
    ck_cpu = mwb_settings_check(box, _("CPU gauges"),
                                _("Per-core processor load meters."), mwb->show_cpumbar);
    ck_mem = mwb_settings_check(box, _("Memory gauge"),
                                _("Used RAM meter."), mwb->show_membar);
    ck_diskgauge = mwb_settings_check(box, _("Disk gauge"),
                                      _("Used disk-space meter."), mwb->show_diskgauge);

    mwb_settings_section(box, _("Indicators"));
    ck_net = mwb_settings_check(box, _("Network lamps"),
                                _("Traffic diodes (red idle, yellow traffic, blue high)."), mwb->show_netlamps);
    ck_disk = mwb_settings_check(box, _("Disk lamps"),
                                 _("Disk-activity diodes."), mwb->show_drivelamps);
    ck_wifi = mwb_settings_check(box, _("Wi-Fi indicator"),
                                 _("NetworkManager based Wi-Fi control."), mwb->show_wifi);
    ck_batt = mwb_settings_check(box, _("Battery indicator"),
                                 _("Charge level and status."), mwb->show_battery);
    ck_sys = mwb_settings_check(box, _("System info"),
                                _("System summary button."), mwb->show_sysinfo);
    ck_vol = mwb_settings_check(box, _("Volume control"),
                                _("Volume slider, mute and mixer."), mwb->show_volume);
    ck_clock = mwb_settings_check(box, _("Clock"),
                                  _("Time, with a click for the calendar."), mwb->show_clock);

    mwb_settings_section(box, _("Layout order"));
    mwb_settings_desc(box, _("Rearrange the indicators with the arrows (takes effect after applying and reloading the panel)."));
    order_list.box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_box_pack_start(GTK_BOX(box), order_list.box, FALSE, FALSE, 0);
    for (i = 0; i < MWB_WIDGET_COUNT; i++)
        order_list.order[i] = mwb->widget_order[i];
    mwb_order_list_rebuild(&order_list);

    gtk_widget_show_all(notebook);

    gboolean done = FALSE;
    while (!done) {
        gint response = gtk_dialog_run(GTK_DIALOG(dialog));
        if (response == GTK_RESPONSE_CANCEL || response == GTK_RESPONSE_DELETE_EVENT) {
            done = TRUE;
            continue;
        }
        if (response != GTK_RESPONSE_APPLY && response != GTK_RESPONSE_OK)
            continue;

        mwb->theme = gtk_combo_box_get_active(GTK_COMBO_BOX(theme_combo));
        if (mwb->theme < 0 || mwb->theme >= MWB_THEME_COUNT)
            mwb->theme = MWB_THEME_CLASSIC;
        mwb->logo_variant = gtk_combo_box_get_active(GTK_COMBO_BOX(logo_combo));
        if (mwb->logo_variant < 0 || mwb->logo_variant >= MWB_LOGO_COUNT)
            mwb->logo_variant = MWB_LOGO_CLASSIC;
        mwb->gauge_style = gtk_combo_box_get_active(GTK_COMBO_BOX(gauge_combo));
        if (mwb->gauge_style < 0 || mwb->gauge_style >= MWB_GAUGE_STYLE_COUNT)
            mwb->gauge_style = MWB_GAUGE_STYLE_INDUSTRIAL;
        mwb->override_theme = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_override));
        mwb->show_clock = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_clock));
        mwb->show_cpumbar = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_cpu));
        mwb->show_membar = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_mem));
        mwb->show_diskgauge = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_diskgauge));
        mwb->show_netlamps = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_net));
        mwb->show_drivelamps = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_disk));
        mwb->show_wifi = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_wifi));
        mwb->show_battery = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_batt));
        mwb->show_sysinfo = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_sys));
        mwb->show_volume = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_vol));
        mwb->show_logo = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_logo));
        mwb->show_title = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_title));
        mwb->show_dynamic_title = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_dyntitle));
        mwb->show_workbench_menu = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_wbmenu));
        mwb->show_ambient_menu = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_ambmenu));
        mwb->show_icons_menu = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_iconsmenu));
        mwb->show_disk_menu = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_diskmenu));
        mwb->show_applications_menu = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_appsmenu));
        for (i = 0; i < MWB_WIDGET_COUNT; i++)
            mwb->widget_order[i] = order_list.order[i];

        mwb_apply_theme(mwb);
        mwb_apply_logo(mwb);
        mwb_apply_left_visibility(mwb);
        mwb_update_title(mwb);
        mwb_apply_widget_order(mwb);

        mwb_save_config(plugin, mwb);

        if (response == GTK_RESPONSE_OK)
            done = TRUE;
    }
    gtk_widget_destroy(dialog);

    xfce_panel_plugin_unblock_menu(plugin);
}
