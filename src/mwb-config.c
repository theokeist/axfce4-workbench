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

#include <gio/gio.h>

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
    xfce_rc_write_int_entry(rc, "icon_size", mwb->icon_size);
    xfce_rc_write_int_entry(rc, "menu_opacity", mwb->menu_opacity);
    xfce_rc_write_bool_entry(rc, "clear_bar_bg", mwb->clear_bar_bg);
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
    xfce_rc_write_bool_entry(rc, "show_notifications", mwb->show_notifications);
    xfce_rc_write_bool_entry(rc, "show_logo", mwb->show_logo);
    xfce_rc_write_bool_entry(rc, "show_title", mwb->show_title);
    xfce_rc_write_bool_entry(rc, "show_dynamic_title", mwb->show_dynamic_title);
    xfce_rc_write_bool_entry(rc, "show_workbench_menu", mwb->show_workbench_menu);
    xfce_rc_write_bool_entry(rc, "show_ambient_menu", mwb->show_ambient_menu);
    xfce_rc_write_bool_entry(rc, "show_icons_menu", mwb->show_icons_menu);
    xfce_rc_write_bool_entry(rc, "show_disk_menu", mwb->show_disk_menu);
    xfce_rc_write_bool_entry(rc, "show_applications_menu", mwb->show_applications_menu);
    xfce_rc_write_bool_entry(rc, "show_recent_apps", mwb->show_recent_apps);
    xfce_rc_write_bool_entry(rc, "show_fav_apps", mwb->show_fav_apps);
    xfce_rc_write_bool_entry(rc, "show_all_apps", mwb->show_all_apps);
    {
        gchar *order = mwb_widget_order_to_string(mwb);
        xfce_rc_write_entry(rc, "widget_order", order);
        g_free(order);
    }
    xfce_rc_flush(rc);
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
    mwb->icon_size = xfce_rc_read_int_entry(rc, "icon_size", MWB_ICON_MEDIUM);
    if (mwb->icon_size < 0 || mwb->icon_size >= MWB_ICON_COUNT)
        mwb->icon_size = MWB_ICON_MEDIUM;
    mwb->menu_opacity = xfce_rc_read_int_entry(rc, "menu_opacity", 90);
    if (mwb->menu_opacity < 0)
        mwb->menu_opacity = 0;
    if (mwb->menu_opacity > 100)
        mwb->menu_opacity = 100;
    mwb->clear_bar_bg = xfce_rc_read_bool_entry(rc, "clear_bar_bg", FALSE);
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
    mwb->show_notifications = xfce_rc_read_bool_entry(rc, "show_notifications", TRUE);
    mwb->show_logo = xfce_rc_read_bool_entry(rc, "show_logo", TRUE);
    mwb->show_title = xfce_rc_read_bool_entry(rc, "show_title", TRUE);
    mwb->show_dynamic_title = xfce_rc_read_bool_entry(rc, "show_dynamic_title", FALSE);
    mwb->show_workbench_menu = xfce_rc_read_bool_entry(rc, "show_workbench_menu", TRUE);
    mwb->show_ambient_menu = xfce_rc_read_bool_entry(rc, "show_ambient_menu", TRUE);
    mwb->show_icons_menu = xfce_rc_read_bool_entry(rc, "show_icons_menu", TRUE);
    mwb->show_disk_menu = xfce_rc_read_bool_entry(rc, "show_disk_menu", TRUE);
    mwb->show_applications_menu = xfce_rc_read_bool_entry(rc, "show_applications_menu", TRUE);
    mwb->show_recent_apps = xfce_rc_read_bool_entry(rc, "show_recent_apps", TRUE);
    mwb->show_fav_apps = xfce_rc_read_bool_entry(rc, "show_fav_apps", TRUE);
    mwb->show_all_apps = xfce_rc_read_bool_entry(rc, "show_all_apps", TRUE);
    {
        const gchar *order = xfce_rc_read_entry(rc, "widget_order", NULL);
        if (order)
            mwb_widget_order_from_string(mwb, order);
    }
    xfce_rc_close(rc);

    mwb_apply_theme(mwb);
    mwb_apply_logo(mwb);
    mwb_apply_menu_opacity(mwb);
    mwb_apply_left_visibility(mwb);
    mwb_apply_widget_order(mwb);
    mwb_set_icon_size(mwb->icon_size);
    mwb_rebuild_menus(mwb);
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
mwb_settings_scale(GtkWidget *box, const gchar *label, const gchar *desc, gdouble value)
{
    GtkWidget *lbl = gtk_label_new(label);
    GtkWidget *scale;

    gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
    gtk_box_pack_start(GTK_BOX(box), lbl, FALSE, FALSE, 0);

    scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 5);
    gtk_scale_set_draw_value(GTK_SCALE(scale), TRUE);
    gtk_range_set_value(GTK_RANGE(scale), value);
    gtk_widget_set_margin_start(scale, 24);
    gtk_widget_set_margin_end(scale, 12);
    gtk_box_pack_start(GTK_BOX(box), scale, FALSE, FALSE, 0);

    if (desc)
        mwb_settings_desc(box, desc);
    return scale;
}

static GtkWidget *
mwb_settings_page(GtkStack *stack, const gchar *title)
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

    gtk_container_set_border_width(GTK_CONTAINER(box), 12);
    gtk_stack_add_titled(stack, box, title, title);
    return box;
}

static void
mwb_open_url(GtkButton *button G_GNUC_UNUSED, const gchar *url)
{
    g_app_info_launch_default_for_uri(url, NULL, NULL);
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

static void
mwb_settings_opacity_changed(GtkRange *range, gpointer data)
{
    MorphosWorkbenchPlugin *mwb = data;
    mwb->menu_opacity = (gint)gtk_range_get_value(range);
    mwb_apply_menu_opacity(mwb);
}

static void
mwb_settings_clear_bg_toggled(GtkToggleButton *btn, gpointer data)
{
    MorphosWorkbenchPlugin *mwb = data;
    mwb->clear_bar_bg = gtk_toggle_button_get_active(btn);
    mwb_apply_theme(mwb);
}

static void
mwb_settings_theme_changed(GtkComboBox *combo, gpointer data)
{
    MorphosWorkbenchPlugin *mwb = data;
    mwb->theme = gtk_combo_box_get_active(combo);
    if (mwb->theme < 0 || mwb->theme >= MWB_THEME_COUNT)
        mwb->theme = MWB_THEME_CLASSIC;
    mwb_apply_theme(mwb);
    mwb_apply_menu_opacity(mwb);
}

void
mwb_configure_plugin(XfcePanelPlugin *plugin, MorphosWorkbenchPlugin *mwb)
{
    GtkWidget *dialog, *content_area, *stack, *switcher, *vbox, *box;
    GtkWidget *theme_combo, *logo_combo, *gauge_combo, *icon_combo, *ck_clear_bg, *ck_override, *opacity_scale;
    GtkWidget *ck_clock, *ck_mem, *ck_cpu, *ck_net, *ck_disk, *ck_vol, *ck_wifi, *ck_batt, *ck_sys, *ck_diskgauge, *ck_notify;
    GtkWidget *ck_logo, *ck_title, *ck_dyntitle, *ck_wbmenu, *ck_ambmenu, *ck_iconsmenu, *ck_diskmenu, *ck_appsmenu;
    GtkWidget *ck_recent, *ck_favs, *ck_allapps;
    MwbOrderList order_list;
    gint i;

    xfce_panel_plugin_block_menu(plugin);

    dialog = xfce_titled_dialog_new_with_mixed_buttons(_("Workbench Settings"),
                                                 GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(plugin))),
                                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                                 "document-save",
                                                 _("_Save"), GTK_RESPONSE_OK,
                                                 "emblem-default",
                                                 _("_Apply"), GTK_RESPONSE_APPLY,
                                                 "window-close",
                                                 _("_Close"), GTK_RESPONSE_CLOSE,
                                                 "process-stop",
                                                 _("_Cancel"), GTK_RESPONSE_CANCEL,
                                                 NULL);
    gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
    gtk_window_set_icon_name(GTK_WINDOW(dialog), MWB_ICON_NAME);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 580, 520);

    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    stack = gtk_stack_new();
    switcher = gtk_stack_switcher_new();
    gtk_stack_switcher_set_stack(GTK_STACK_SWITCHER(switcher), GTK_STACK(stack));
    gtk_widget_set_halign(switcher, GTK_ALIGN_CENTER);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), switcher, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), stack, TRUE, TRUE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);
    gtk_box_pack_start(GTK_BOX(content_area), vbox, TRUE, TRUE, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(dialog), "mwb-settings");

    /* ---- General ---- */
    box = mwb_settings_page(GTK_STACK(stack), _("General"));
    mwb_settings_section(box, _("Appearance"));
    theme_combo = mwb_settings_combo(box, _("Theme:"), _("Color scheme used across the Workbench."));
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(theme_combo), NULL, _("Classic (Ambient dark-blue)"));
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(theme_combo), NULL, _("Dark (sleek near-black)"));
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(theme_combo), NULL, _("Light (silver metal)"));
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(theme_combo), NULL, _("System (follow desktop theme)"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(theme_combo), mwb->theme);
    g_signal_connect(theme_combo, "changed", G_CALLBACK(mwb_settings_theme_changed), mwb);

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

    icon_combo = mwb_settings_combo(box, _("Menu icon size:"), _("Size of the icons in the menus."));
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(icon_combo), NULL, _("Small"));
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(icon_combo), NULL, _("Medium"));
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(icon_combo), NULL, _("Big"));
    gtk_combo_box_set_active(GTK_COMBO_BOX(icon_combo), mwb->icon_size);

    opacity_scale = mwb_settings_scale(box, _("Menu opacity:"), _("Transparency of the dropdown menus and popups (100 = fully opaque)."), mwb->menu_opacity);
    g_signal_connect(opacity_scale, "value-changed", G_CALLBACK(mwb_settings_opacity_changed), mwb);

    mwb_settings_section(box, _("Behavior"));
    ck_clear_bg = mwb_settings_check(box, _("Clear background"),
                                     _("Make the Workbench bar transparent so only menu text and screenbar items are visible over transparent panels or desktop backgrounds."),
                                     mwb->clear_bar_bg);
    g_signal_connect(ck_clear_bg, "toggled", G_CALLBACK(mwb_settings_clear_bg_toggled), mwb);
    ck_override = mwb_settings_check(box, _("Override GTK theme"),
                                     _("Apply the Workbench's own style to every widget, independent of the desktop theme."),
                                     mwb->override_theme);

    /* ---- Workbench ---- */
    box = mwb_settings_page(GTK_STACK(stack), _("Workbench"));
    mwb_settings_section(box, _("Workbench"));
    ck_logo = mwb_settings_check(box, _("Ambient logo"),
                                 _("Show the Ambient emblem button."), mwb->show_logo);
    ck_title = mwb_settings_check(box, _("Workbench title"),
                                  _("Show the title label next to the emblem."), mwb->show_title);
    ck_dyntitle = mwb_settings_check(box, _("Dynamic title"),
                                     _("Show the foreground application's name instead of a static title."),
                                     mwb->show_dynamic_title);

    mwb_settings_section(box, _("Bar Menus"));
    ck_wbmenu = mwb_settings_check(box, _("Workbench menu"),
                                   _("Home, documents, terminal, trash and refresh."), mwb->show_workbench_menu);
    ck_ambmenu = mwb_settings_check(box, _("Ambient menu"),
                                    _("System, task manager, settings and shortcuts."), mwb->show_ambient_menu);
    ck_iconsmenu = mwb_settings_check(box, _("Icons menu"),
                                      _("Desktop cleanup, snapshot and arrangements."), mwb->show_icons_menu);
    ck_diskmenu = mwb_settings_check(box, _("Disk menu"),
                                     _("Mounts, unmounts, eject and format."), mwb->show_disk_menu);
    ck_appsmenu = mwb_settings_check(box, _("Applications menu"),
                                     _("Favorite, categorized, and recently closed applications."), mwb->show_applications_menu);

    mwb_settings_section(box, _("Applications Menu Contents"));
    ck_favs = mwb_settings_check(box, _("Include favorite applications"),
                                 _("Show quick links to terminal, file manager, browser, and settings."), mwb->show_fav_apps);
    ck_allapps = mwb_settings_check(box, _("Include categorized applications"),
                                   _("Show full system applications categorized by group."), mwb->show_all_apps);
    ck_recent = mwb_settings_check(box, _("Include recently closed applications"),
                                   _("Show applications closed during this session."), mwb->show_recent_apps);

    /* ---- Screenbar (right side) ---- */
    box = mwb_settings_page(GTK_STACK(stack), _("Screenbar"));
    mwb_settings_section(box, _("Screenbar Items"));
    ck_clock = mwb_settings_check(box, _("Clock"),
                                  _("Live digital clock and date in the screenbar."), mwb->show_clock);
    ck_cpu = mwb_settings_check(box, _("CPU gauge"),
                                _("Activity meter for processor cores."), mwb->show_cpumbar);
    ck_mem = mwb_settings_check(box, _("Memory gauge"),
                                _("RAM usage meter."), mwb->show_membar);
    ck_diskgauge = mwb_settings_check(box, _("Disk gauge"),
                                      _("Storage usage indicator."), mwb->show_diskgauge);
    ck_net = mwb_settings_check(box, _("Network lamps"),
                                _("TX and RX activity indicator LEDs."), mwb->show_netlamps);
    ck_disk = mwb_settings_check(box, _("Drive lamps"),
                                 _("Disk read and write activity LEDs."), mwb->show_drivelamps);
    ck_wifi = mwb_settings_check(box, _("Wi-Fi"),
                                 _("NetworkManager Wi-Fi / connectivity indicator."), mwb->show_wifi);
    ck_batt = mwb_settings_check(box, _("Battery"),
                                 _("Laptop battery meter and power status."), mwb->show_battery);
    ck_sys = mwb_settings_check(box, _("System info"),
                                _("Quick kernel and machine info."), mwb->show_sysinfo);
    ck_vol = mwb_settings_check(box, _("Volume & Media"),
                                _("Master volume, mic and application streams popover."), mwb->show_volume);
    ck_notify = mwb_settings_check(box, _("Notifications"),
                                   _("Notification center and unread alerts indicator."), mwb->show_notifications);

    /* ---- Layout (screenbar item reordering) ---- */
    box = mwb_settings_page(GTK_STACK(stack), _("Layout"));
    mwb_settings_section(box, _("Screenbar Item Order"));
    mwb_settings_desc(box, _("Use the Up and Down buttons to reorder the items on the right side of the Workbench screenbar (from left to right)."));
    {
        for (i = 0; i < MWB_WIDGET_COUNT; i++)
            order_list.order[i] = mwb->widget_order[i];
        order_list.box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        gtk_widget_set_margin_start(order_list.box, 12);
        gtk_widget_set_margin_top(order_list.box, 6);
        mwb_order_list_rebuild(&order_list);
        gtk_box_pack_start(GTK_BOX(box), order_list.box, TRUE, TRUE, 0);
    }

    /* ---- About ---- */
    box = mwb_settings_page(GTK_STACK(stack), _("About"));
    {
        GtkWidget *title = gtk_label_new(NULL);
        gchar *markup = g_markup_printf_escaped("<b>%s %s</b>", _("MorphOS Workbench"), PACKAGE_VERSION);
        gtk_label_set_markup(GTK_LABEL(title), markup);
        g_free(markup);
        gtk_label_set_xalign(GTK_LABEL(title), 0.0);
        gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);

        GtkWidget *desc = gtk_label_new(_("A MorphOS Ambient style top bar for XFCE4."));
        gtk_label_set_xalign(GTK_LABEL(desc), 0.0);
        gtk_label_set_line_wrap(GTK_LABEL(desc), TRUE);
        gtk_box_pack_start(GTK_BOX(box), desc, FALSE, FALSE, 0);

        mwb_settings_section(box, _("Author"));
        GtkWidget *author = gtk_label_new(_("theokeist"));
        gtk_label_set_xalign(GTK_LABEL(author), 0.0);
        gtk_box_pack_start(GTK_BOX(box), author, FALSE, FALSE, 0);
        GtkWidget *github = gtk_label_new("https://github.com/theokeist");
        gtk_label_set_xalign(GTK_LABEL(github), 0.0);
        gtk_box_pack_start(GTK_BOX(box), github, FALSE, FALSE, 0);

        mwb_settings_section(box, _("Support"));
        GtkWidget *sponsor = gtk_button_new_with_label(_("Donate via GitHub Sponsors"));
        gtk_button_set_relief(GTK_BUTTON(sponsor), GTK_RELIEF_NONE);
        gtk_style_context_add_class(gtk_widget_get_style_context(sponsor), "mwb-volbutton");
        g_signal_connect(sponsor, "clicked", G_CALLBACK(mwb_open_url),
                         "https://github.com/sponsors/theokeist");
        gtk_box_pack_start(GTK_BOX(box), sponsor, FALSE, FALSE, 0);

        GtkWidget *issue = gtk_button_new_with_label(_("Report an Issue"));
        gtk_button_set_relief(GTK_BUTTON(issue), GTK_RELIEF_NONE);
        gtk_style_context_add_class(gtk_widget_get_style_context(issue), "mwb-volbutton");
        g_signal_connect(issue, "clicked", G_CALLBACK(mwb_open_url),
                         "https://github.com/theokeist/axfce4-workbench/issues");
        gtk_box_pack_start(GTK_BOX(box), issue, FALSE, FALSE, 0);
    }

    gtk_widget_show_all(dialog);

    gboolean done = FALSE;
    while (!done) {
        gint response = gtk_dialog_run(GTK_DIALOG(dialog));
        if (response == GTK_RESPONSE_CANCEL || response == GTK_RESPONSE_DELETE_EVENT) {
            /* Discard unapplied live preview changes and restore saved configuration */
            mwb_load_config(mwb);
            done = TRUE;
            continue;
        }
        if (response != GTK_RESPONSE_APPLY && response != GTK_RESPONSE_OK && response != GTK_RESPONSE_CLOSE)
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
        mwb->icon_size = gtk_combo_box_get_active(GTK_COMBO_BOX(icon_combo));
        if (mwb->icon_size < 0 || mwb->icon_size >= MWB_ICON_COUNT)
            mwb->icon_size = MWB_ICON_MEDIUM;
        mwb->menu_opacity = (gint)gtk_range_get_value(GTK_RANGE(opacity_scale));
        mwb->clear_bar_bg = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_clear_bg));
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
        mwb->show_notifications = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_notify));
        mwb->show_logo = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_logo));
        mwb->show_title = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_title));
        mwb->show_dynamic_title = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_dyntitle));
        mwb->show_workbench_menu = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_wbmenu));
        mwb->show_ambient_menu = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_ambmenu));
        mwb->show_icons_menu = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_iconsmenu));
        mwb->show_disk_menu = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_diskmenu));
        mwb->show_applications_menu = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_appsmenu));
        mwb->show_recent_apps = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_recent));
        mwb->show_fav_apps = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_favs));
        mwb->show_all_apps = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ck_allapps));
        for (i = 0; i < MWB_WIDGET_COUNT; i++)
            mwb->widget_order[i] = order_list.order[i];

        /* Update plugin styling & layout */
        mwb_apply_theme(mwb);
        mwb_apply_logo(mwb);
        mwb_apply_menu_opacity(mwb);
        mwb_apply_left_visibility(mwb);
        mwb_update_title(mwb);
        mwb_apply_widget_order(mwb);
        mwb_set_icon_size(mwb->icon_size);
        mwb_rebuild_menus(mwb);

        /* Save directly to the file */
        mwb_save_config(plugin, mwb);

        if (response == GTK_RESPONSE_OK || response == GTK_RESPONSE_CLOSE)
            done = TRUE;
    }
    gtk_widget_destroy(dialog);

    xfce_panel_plugin_unblock_menu(plugin);
}
