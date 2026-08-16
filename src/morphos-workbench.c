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

/* Plugin lifecycle (static, file-local) */
static void     mwb_construct           (XfcePanelPlugin *plugin);
static void     mwb_free_data           (XfcePanelPlugin *plugin,
                                         MorphosWorkbenchPlugin *mwb);
static gboolean mwb_size_changed        (XfcePanelPlugin *plugin,
                                         guint size,
                                         MorphosWorkbenchPlugin *mwb);
static void     mwb_orientation_changed (XfcePanelPlugin *plugin,
                                         GtkOrientation orientation,
                                         MorphosWorkbenchPlugin *mwb);

/* ------------------------------------------------------------------ *
 *  Top bar (left side) — Ambient logo, Workbench title, menu titles
 *  ------------------------------------------------------------------ */

void
mwb_build_bar(MorphosWorkbenchPlugin *mwb)
{
    GtkWidget *sep;

    mwb->bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->bar), "mwb-bar");
    gtk_container_add(GTK_CONTAINER(mwb->plugin), mwb->bar);

    /* Ambient logo button */
    GtkWidget *logo = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(logo), GTK_RELIEF_NONE);
    gtk_style_context_add_class(gtk_widget_get_style_context(logo), "mwb-button");
    gtk_style_context_add_class(gtk_widget_get_style_context(logo), "mwb-logo");
    gtk_widget_set_tooltip_text(logo, _("Ambient"));
    GtkWidget *logo_img = gtk_image_new_from_icon_name(mwb_logo_icon_name(mwb->logo_variant), GTK_ICON_SIZE_BUTTON);
    if (!gtk_image_get_pixbuf(GTK_IMAGE(logo_img)))
        gtk_image_set_from_icon_name(GTK_IMAGE(logo_img), MWB_FALLBACK_ICON, GTK_ICON_SIZE_BUTTON);
    gtk_container_add(GTK_CONTAINER(logo), logo_img);
    gtk_widget_show(logo_img);
    gtk_box_pack_start(GTK_BOX(mwb->bar), logo, FALSE, FALSE, 0);
    mwb->logo_button = logo;
    mwb->menu_buttons[MWB_MENU_AMBIENT] = logo;
    g_signal_connect(logo, "clicked", G_CALLBACK(mwb_menu_toggle), mwb);

    /* Workbench title */
    mwb->title = gtk_label_new(_("Workbench"));
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->title), "mwb-title");
    gtk_box_pack_start(GTK_BOX(mwb->bar), mwb->title, FALSE, FALSE, 0);

    sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_style_context_add_class(gtk_widget_get_style_context(sep), "mwb-separator");
    gtk_box_pack_start(GTK_BOX(mwb->bar), sep, FALSE, FALSE, 0);

    /* Menu titles */
    struct { const gchar *label; MorphosWorkbenchMenuId id; } titles[] = {
        { N_("Workbench"), MWB_MENU_WORKBENCH },
        { N_("Ambient"),   MWB_MENU_AMBIENT_MENU },
        { N_("Icons"),     MWB_MENU_ICONS },
        { N_("Disk"),      MWB_MENU_DISK },
        { N_("Applications"), MWB_MENU_APPLICATIONS },
    };
    guint i;
    for (i = 0; i < G_N_ELEMENTS(titles); i++) {
        GtkWidget *b = mwb_create_menu_title(mwb, _(titles[i].label));
        gtk_box_pack_start(GTK_BOX(mwb->bar), b, FALSE, FALSE, 0);
        mwb->menu_buttons[titles[i].id] = b;
        g_signal_connect(b, "clicked", G_CALLBACK(mwb_menu_toggle), mwb);
    }

    gtk_widget_show_all(mwb->bar);
}

void
mwb_apply_left_visibility(MorphosWorkbenchPlugin *mwb)
{
    if (mwb->logo_button)
        gtk_widget_set_visible(mwb->logo_button, mwb->show_logo);
    if (mwb->title)
        gtk_widget_set_visible(mwb->title, mwb->show_title);
    if (mwb->menu_buttons[MWB_MENU_WORKBENCH])
        gtk_widget_set_visible(mwb->menu_buttons[MWB_MENU_WORKBENCH], mwb->show_workbench_menu);
    if (mwb->menu_buttons[MWB_MENU_AMBIENT_MENU])
        gtk_widget_set_visible(mwb->menu_buttons[MWB_MENU_AMBIENT_MENU], mwb->show_ambient_menu);
    if (mwb->menu_buttons[MWB_MENU_ICONS])
        gtk_widget_set_visible(mwb->menu_buttons[MWB_MENU_ICONS], mwb->show_icons_menu);
    if (mwb->menu_buttons[MWB_MENU_DISK])
        gtk_widget_set_visible(mwb->menu_buttons[MWB_MENU_DISK], mwb->show_disk_menu);
    if (mwb->menu_buttons[MWB_MENU_APPLICATIONS])
        gtk_widget_set_visible(mwb->menu_buttons[MWB_MENU_APPLICATIONS], mwb->show_applications_menu);
}

/* ------------------------------------------------------------------ *
 *  Plugin lifecycle
 *  ------------------------------------------------------------------ */

static void
mwb_construct(XfcePanelPlugin *plugin)
{
    MorphosWorkbenchPlugin *mwb = g_slice_new0(MorphosWorkbenchPlugin);
    mwb->plugin = plugin;
    mwb->theme = MWB_THEME_CLASSIC;
    mwb->logo_variant = MWB_LOGO_CLASSIC;
    mwb->gauge_style = MWB_GAUGE_STYLE_INDUSTRIAL;
    mwb->override_theme = TRUE;
    mwb->show_clock = TRUE;
    mwb->show_membar = TRUE;
    mwb->show_cpumbar = TRUE;
    mwb->show_diskgauge = TRUE;
    mwb->show_netlamps = TRUE;
    mwb->show_drivelamps = TRUE;
    mwb->show_volume = TRUE;
    mwb->show_wifi = TRUE;
    mwb->show_battery = TRUE;
    mwb->show_sysinfo = TRUE;
    mwb->show_logo = TRUE;
    mwb->show_title = TRUE;
    mwb->show_workbench_menu = TRUE;
    mwb->show_ambient_menu = TRUE;
    mwb->show_icons_menu = TRUE;
    mwb->show_disk_menu = TRUE;
    mwb->show_applications_menu = TRUE;
    mwb->vol_percent = 70;

    mwb_init_css();

    mwb_build_bar(mwb);
    mwb_create_menus(mwb);
    mwb_apply_theme(mwb);
    mwb_apply_logo(mwb);

    /* make the workbench fill the whole panel */
    xfce_panel_plugin_set_expand(plugin, TRUE);
    gtk_widget_set_hexpand(mwb->bar, TRUE);

    /* right side: screenbar (lamps, gauges, wifi, battery, volume, clock) */
    mwb_build_screenbar(mwb);

    mwb_load_config(mwb);

    g_signal_connect(plugin, "free-data", G_CALLBACK(mwb_free_data), mwb);
    g_signal_connect(plugin, "save", G_CALLBACK(mwb_save_config), mwb);
    g_signal_connect(plugin, "size-changed", G_CALLBACK(mwb_size_changed), mwb);
    g_signal_connect(plugin, "orientation-changed", G_CALLBACK(mwb_orientation_changed), mwb);
    g_signal_connect(plugin, "configure-plugin", G_CALLBACK(mwb_configure_plugin), mwb);

    xfce_panel_plugin_menu_show_configure(plugin);
    g_object_set_data(G_OBJECT(plugin), "mwb-data", mwb);
}

static void
mwb_free_data(XfcePanelPlugin *plugin G_GNUC_UNUSED, MorphosWorkbenchPlugin *mwb)
{
    if (mwb->clock_timeout)
        g_source_remove(mwb->clock_timeout);
    if (mwb->mem_timeout)
        g_source_remove(mwb->mem_timeout);
    if (mwb->cpu_timeout)
        g_source_remove(mwb->cpu_timeout);
    if (mwb->net_timeout)
        g_source_remove(mwb->net_timeout);
    if (mwb->disk_timeout)
        g_source_remove(mwb->disk_timeout);
    if (mwb->batt_timeout)
        g_source_remove(mwb->batt_timeout);
    if (mwb->sys_timeout)
        g_source_remove(mwb->sys_timeout);
    if (mwb->calendar_grab)
        g_source_remove(mwb->calendar_grab);
    if (mwb->volume_grab)
        g_source_remove(mwb->volume_grab);

    gint i;
    for (i = 0; i < MWB_MENU_COUNT; i++)
        if (mwb->menus[i])
            gtk_widget_destroy(mwb->menus[i]);

    if (mwb->calendar_popup)
        gtk_widget_destroy(mwb->calendar_popup);
    if (mwb->volume_popup)
        gtk_widget_destroy(mwb->volume_popup);

    if (mwb->bar)
        gtk_widget_destroy(mwb->bar);

    if (mwb->nm_module)
        g_module_close(mwb->nm_module);

    mwb_recent_clear(mwb);

    g_slice_free(MorphosWorkbenchPlugin, mwb);
}

static gboolean
mwb_size_changed(XfcePanelPlugin *plugin G_GNUC_UNUSED, guint size, MorphosWorkbenchPlugin *mwb)
{
    gint spacing, margin;

    gtk_widget_set_size_request(mwb->bar, -1, size);

    /* keep the right-side screenbar proportional at any panel size */
    if (mwb->screenbar) {
        spacing = CLAMP((gint)size / 6, 2, 12);
        margin  = CLAMP((gint)size / 10, 1, 8);
        gtk_box_set_spacing(GTK_BOX(mwb->screenbar), spacing);
        gtk_widget_set_margin_top(mwb->screenbar, margin);
        gtk_widget_set_margin_bottom(mwb->screenbar, margin);
    }
    return TRUE;
}

static void
mwb_orientation_changed(XfcePanelPlugin *plugin G_GNUC_UNUSED, GtkOrientation orientation, MorphosWorkbenchPlugin *mwb G_GNUC_UNUSED)
{
    if (orientation == GTK_ORIENTATION_HORIZONTAL)
        gtk_widget_show_all(mwb->bar);
}

XFCE_PANEL_PLUGIN_REGISTER(mwb_construct);
