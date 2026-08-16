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
    xfce_rc_write_bool_entry(rc, "show_workbench_menu", mwb->show_workbench_menu);
    xfce_rc_write_bool_entry(rc, "show_ambient_menu", mwb->show_ambient_menu);
    xfce_rc_write_bool_entry(rc, "show_icons_menu", mwb->show_icons_menu);
    xfce_rc_write_bool_entry(rc, "show_disk_menu", mwb->show_disk_menu);
    xfce_rc_write_bool_entry(rc, "show_applications_menu", mwb->show_applications_menu);
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
    mwb->show_workbench_menu = xfce_rc_read_bool_entry(rc, "show_workbench_menu", TRUE);
    mwb->show_ambient_menu = xfce_rc_read_bool_entry(rc, "show_ambient_menu", TRUE);
    mwb->show_icons_menu = xfce_rc_read_bool_entry(rc, "show_icons_menu", TRUE);
    mwb->show_disk_menu = xfce_rc_read_bool_entry(rc, "show_disk_menu", TRUE);
    mwb->show_applications_menu = xfce_rc_read_bool_entry(rc, "show_applications_menu", TRUE);
    xfce_rc_close(rc);

    mwb_apply_theme(mwb);
    mwb_apply_logo(mwb);
    mwb_apply_left_visibility(mwb);

    if (mwb->clock_button)
        gtk_widget_set_visible(mwb->clock_button, mwb->show_clock);
    if (mwb->mem_gauge)
        gtk_widget_set_visible(mwb->mem_gauge, mwb->show_membar);
    if (mwb->disk_gauge)
        gtk_widget_set_visible(mwb->disk_gauge, mwb->show_diskgauge);
    if (mwb->vol_button)
        gtk_widget_set_visible(mwb->vol_button, mwb->show_volume);
    if (mwb->nm_plugin)
        gtk_widget_set_visible(mwb->nm_plugin, mwb->show_wifi);
    if (mwb->batt_button)
        gtk_widget_set_visible(mwb->batt_button, mwb->show_battery);
    if (mwb->sys_button)
        gtk_widget_set_visible(mwb->sys_button, mwb->show_sysinfo);
}

void
mwb_configure_plugin(XfcePanelPlugin *plugin, MorphosWorkbenchPlugin *mwb)
{
    GtkWidget *dialog, *content_area, *grid;
    GtkWidget *ck_clock, *ck_mem, *ck_cpu, *ck_net, *ck_disk, *ck_vol, *ck_wifi, *ck_batt, *ck_sys, *ck_diskgauge;
    GtkWidget *ck_logo, *ck_title, *ck_wbmenu, *ck_ambmenu, *ck_iconsmenu, *ck_diskmenu, *ck_appsmenu;
    GtkWidget *ck_override;
    GtkWidget *theme_combo;
    GtkWidget *logo_combo;
    GtkWidget *gauge_combo;
    gint row;

    xfce_panel_plugin_block_menu(plugin);

    dialog = xfce_titled_dialog_new_with_mixed_buttons(_("MorphOS Workbench Settings"),
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

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 14);
    gtk_box_pack_start(GTK_BOX(content_area), grid, TRUE, TRUE, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(dialog), "mwb-settings");

    row = 0;
    GtkWidget *label = gtk_label_new(_("Theme:"));
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);

    theme_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(theme_combo), NULL, _("Classic (Ambient dark-blue)"));
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(theme_combo), NULL, _("Dark (sleek near-black)"));
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(theme_combo), NULL, _("Light (silver metal)"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(theme_combo), mwb->theme);
    gtk_grid_attach(GTK_GRID(grid), theme_combo, 1, row++, 1, 1);

    GtkWidget *logo_label = gtk_label_new(_("Ambient logo:"));
    gtk_label_set_xalign(GTK_LABEL(logo_label), 0.0);
    gtk_grid_attach(GTK_GRID(grid), logo_label, 0, row, 1, 1);
    logo_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(logo_combo), NULL, _("Classic sphere"));
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(logo_combo), NULL, _("Flat sphere"));
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(logo_combo), NULL, _("Monochrome sphere"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(logo_combo), mwb->logo_variant);
    gtk_grid_attach(GTK_GRID(grid), logo_combo, 1, row++, 1, 1);

    GtkWidget *gauge_label = gtk_label_new(_("Gauge style:"));
    gtk_label_set_xalign(GTK_LABEL(gauge_label), 0.0);
    gtk_grid_attach(GTK_GRID(grid), gauge_label, 0, row, 1, 1);
    gauge_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(gauge_combo), NULL, _("Industrial (segmented blocks)"));
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(gauge_combo), NULL, _("Glossy 3D (chrome)"));
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(gauge_combo), NULL, _("Plain (flat square)"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(gauge_combo), mwb->gauge_style);
    gtk_grid_attach(GTK_GRID(grid), gauge_combo, 1, row++, 1, 1);

    ck_override = gtk_check_button_new_with_label(_("Override GTK theme (apply Workbench style to everything)"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ck_override), mwb->override_theme);
    gtk_grid_attach(GTK_GRID(grid), ck_override, 0, row++, 2, 1);

    ck_logo = gtk_check_button_new_with_label(_("Ambient logo"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ck_logo), mwb->show_logo);
    gtk_grid_attach(GTK_GRID(grid), ck_logo, 0, row, 1, 1);
    ck_title = gtk_check_button_new_with_label(_("Workbench title"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ck_title), mwb->show_title);
    gtk_grid_attach(GTK_GRID(grid), ck_title, 1, row++, 1, 1);

    ck_wbmenu = gtk_check_button_new_with_label(_("Workbench menu"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ck_wbmenu), mwb->show_workbench_menu);
    gtk_grid_attach(GTK_GRID(grid), ck_wbmenu, 0, row, 1, 1);
    ck_ambmenu = gtk_check_button_new_with_label(_("Ambient menu"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ck_ambmenu), mwb->show_ambient_menu);
    gtk_grid_attach(GTK_GRID(grid), ck_ambmenu, 1, row++, 1, 1);

    ck_iconsmenu = gtk_check_button_new_with_label(_("Icons menu"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ck_iconsmenu), mwb->show_icons_menu);
    gtk_grid_attach(GTK_GRID(grid), ck_iconsmenu, 0, row, 1, 1);
    ck_diskmenu = gtk_check_button_new_with_label(_("Disk menu"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ck_diskmenu), mwb->show_disk_menu);
    gtk_grid_attach(GTK_GRID(grid), ck_diskmenu, 1, row++, 1, 1);

    ck_appsmenu = gtk_check_button_new_with_label(_("Applications menu"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ck_appsmenu), mwb->show_applications_menu);
    gtk_grid_attach(GTK_GRID(grid), ck_appsmenu, 0, row++, 2, 1);

    ck_cpu = gtk_check_button_new_with_label(_("Show CPU gauges (per core)"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ck_cpu), mwb->show_cpumbar);
    gtk_grid_attach(GTK_GRID(grid), ck_cpu, 0, row++, 2, 1);

    ck_mem = gtk_check_button_new_with_label(_("Show memory gauge"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ck_mem), mwb->show_membar);
    gtk_grid_attach(GTK_GRID(grid), ck_mem, 0, row++, 2, 1);

    ck_diskgauge = gtk_check_button_new_with_label(_("Show disk gauge"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ck_diskgauge), mwb->show_diskgauge);
    gtk_grid_attach(GTK_GRID(grid), ck_diskgauge, 0, row++, 2, 1);

    ck_net = gtk_check_button_new_with_label(_("Show network lamps"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ck_net), mwb->show_netlamps);
    gtk_grid_attach(GTK_GRID(grid), ck_net, 0, row++, 2, 1);

    ck_disk = gtk_check_button_new_with_label(_("Show disk lamps"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ck_disk), mwb->show_drivelamps);
    gtk_grid_attach(GTK_GRID(grid), ck_disk, 0, row++, 2, 1);

    ck_wifi = gtk_check_button_new_with_label(_("Show Wi-Fi indicator"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ck_wifi), mwb->show_wifi);
    gtk_grid_attach(GTK_GRID(grid), ck_wifi, 0, row++, 2, 1);

    ck_batt = gtk_check_button_new_with_label(_("Show battery indicator"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ck_batt), mwb->show_battery);
    gtk_grid_attach(GTK_GRID(grid), ck_batt, 0, row++, 2, 1);

    ck_sys = gtk_check_button_new_with_label(_("Show system info"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ck_sys), mwb->show_sysinfo);
    gtk_grid_attach(GTK_GRID(grid), ck_sys, 0, row++, 2, 1);

    ck_vol = gtk_check_button_new_with_label(_("Show volume control"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ck_vol), mwb->show_volume);
    gtk_grid_attach(GTK_GRID(grid), ck_vol, 0, row++, 2, 1);

    ck_clock = gtk_check_button_new_with_label(_("Show clock (click for calendar)"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ck_clock), mwb->show_clock);
    gtk_grid_attach(GTK_GRID(grid), ck_clock, 0, row++, 2, 1);

    gtk_widget_show_all(grid);

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
        mwb->show_workbench_menu = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_wbmenu));
        mwb->show_ambient_menu = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_ambmenu));
        mwb->show_icons_menu = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_iconsmenu));
        mwb->show_disk_menu = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_diskmenu));
        mwb->show_applications_menu = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_appsmenu));

        mwb_apply_theme(mwb);
        mwb_apply_logo(mwb);
        mwb_apply_left_visibility(mwb);

        if (mwb->clock_button)
            gtk_widget_set_visible(mwb->clock_button, mwb->show_clock);
        if (mwb->nm_plugin)
            gtk_widget_set_visible(mwb->nm_plugin, mwb->show_wifi);
        if (mwb->batt_button)
            gtk_widget_set_visible(mwb->batt_button, mwb->show_battery);
        if (mwb->sys_button)
            gtk_widget_set_visible(mwb->sys_button, mwb->show_sysinfo);
        if (mwb->vol_button)
            gtk_widget_set_visible(mwb->vol_button, mwb->show_volume);

        mwb_save_config(plugin, mwb);

        if (response == GTK_RESPONSE_OK)
            done = TRUE;
    }
    gtk_widget_destroy(dialog);

    xfce_panel_plugin_unblock_menu(plugin);
}
