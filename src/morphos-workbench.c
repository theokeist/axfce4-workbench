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

#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <glib/gstdio.h>

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
    cairo_arc(cr, x + r, y + r, r, G_PI, 3 * G_PI_2);
    cairo_close_path(cr);
}

/* ------------------------------------------------------------------ *
 *  CSS - three Ambient themes (Classic / Dark / Light)
 *  ------------------------------------------------------------------ */

static const gchar *MWB_CSS =
  /* ---- structural base (theme independent) ---- */
  ".mwb-bar {"
  "  padding: 0;"
  "}"
  ".mwb-button {"
  "  background: transparent;"
  "  border: none;"
  "  border-radius: 0;"
  "  padding: 0 9px;"
  "  font-weight: 600;"
  "}"
  ".mwb-button:hover {"
  "  color: #ffffff;"
  "}"
  ".mwb-button:active, .mwb-button.open {"
  "  color: #ffffff;"
  "}"
  ".mwb-logo {"
  "  padding: 0 7px 0 6px;"
  "}"
  ".mwb-clocks {"
  "  background: transparent;"
  "  padding: 0 7px;"
  "  font-size: 12px;"
  "  font-weight: 600;"
  "}"
  ".mwb-separator {"
  "  min-width: 1px;"
  "  margin: 4px 3px;"
  "}"
  ".mwb-screenbar-divider {"
  "  min-width: 1px;"
  "  margin: 5px 3px;"
  "}"
  ".mwb-popup {"
  "  background-color: #18263a;"
  "  color: #dbe6f2;"
  "  border: 1px solid #0a1526;"
  "  box-shadow: 0 8px 22px alpha(#000000, 0.60);"
  "}"
  ".mwb-popup label, .mwb-popup calendar {"
  "  color: #dbe6f2;"
  "}"
  ".mwb-settings-section {"
  "  font-weight: 700;"
  "  padding: 4px 6px;"
  "  border-top: 1px solid alpha(#ffffff, 0.28);"
  "  border-bottom: 1px solid alpha(#000000, 0.35);"
  "}"
  ".mwb-gaugebox {"
  "  background: transparent;"
  "  padding: 2px 2px;"
  "  border: 1px solid alpha(#000000, 0.55);"
  "  box-shadow: inset 1px 1px 0 alpha(#ffffff, 0.16), inset -1px -1px 0 alpha(#000000, 0.55);"
  "}"
  ".mwb-vgauge {"
  "  min-width: 76px;"
  "  min-height: 10px;"
  "  background: transparent;"
  "}"
  ".mwb-vgauge-label {"
  "  font-size: 8px;"
  "  font-weight: 700;"
  "  padding: 0 1px;"
  "}"
  ".mwb-gauge-value {"
  "  min-width: 30px;"
  "  font-family: monospace;"
  "  font-size: 9px;"
  "  font-weight: 700;"
  "  padding: 0 2px;"
  "}"
  ".mwb-lamp {"
  "  min-width: 10px;"
  "  min-height: 10px;"
  "  border-radius: 6px;"
  "  margin: 0 1px;"
  "}"
  ".mwb-screenbar-item {"
  "  background: transparent;"
  "  font-size: 10px;"
  "  font-weight: 600;"
  "}"
  ".mwb-volbutton {"
  "  background: transparent;"
  "  border: 1px solid transparent;"
  "  border-radius: 0;"
  "  padding: 0 5px;"
  "  min-height: 24px;"
  "}"
  ".mwb-volbutton:hover {"
  "  color: #ffffff;"
  "}"
  ".mwb-clockbtn {"
  "  background: transparent;"
  "  border: 1px solid transparent;"
  "  border-radius: 0;"
  "  padding: 0 4px;"
  "}"
  ".mwb-clockbtn:hover {"
  "  color: #ffffff;"
  "}"
  ".mwb-theme-classic .mwb-volbutton, .mwb-theme-classic .mwb-clockbtn,"
  ".mwb-theme-dark .mwb-volbutton, .mwb-theme-dark .mwb-clockbtn {"
  "  border-top-color: alpha(#ffffff, 0.12);"
  "  border-left-color: alpha(#ffffff, 0.08);"
  "  border-right-color: alpha(#000000, 0.45);"
  "  border-bottom-color: alpha(#000000, 0.45);"
  "}"
  ".mwb-theme-light .mwb-volbutton, .mwb-theme-light .mwb-clockbtn {"
  "  border-top-color: alpha(#ffffff, 0.72);"
  "  border-left-color: alpha(#ffffff, 0.60);"
  "  border-right-color: alpha(#5b6672, 0.42);"
  "  border-bottom-color: alpha(#5b6672, 0.42);"
  "}"
  ".mwb-clockbtn .mwb-clocks {"
  "  color: inherit;"
  "}"
  ".mwb-hbox {"
  "  background: transparent;"
  "}"
  ".mwb-status-group {"
  "  padding: 0 3px;"
  "  border: 1px solid alpha(#000000, 0.45);"
  "  box-shadow: inset 1px 1px 0 alpha(#ffffff, 0.12), inset -1px -1px 0 alpha(#000000, 0.45);"
  "}"
  ".mwb-island {"
  "  margin: 4px 1px;"
  "  padding: 0 4px;"
  "  min-height: 18px;"
  "  border: 1px solid alpha(#000000, 0.55);"
  "  box-shadow: inset 1px 1px 0 alpha(#ffffff, 0.16), inset -1px -1px 0 alpha(#000000, 0.55);"
  "}"
  ".mwb-title {"
  "  font-weight: 700;"
  "  padding: 0 6px;"
  "}"
  ".mwb-title.mwb-title-active {"
  "  text-shadow: 0 0 6px alpha(#6ba3d9, 0.8);"
  "}"
  ".mwb-appmenu-item {"
  "  padding: 6px 22px 6px 12px;"
  "}"
  ".mwb-appmenu-item:hover {"
  "  color: #ffffff;"
  "}"
  ".mwb-menu-workbench { min-width: 220px; }"
  ".mwb-menu-ambient { min-width: 250px; }"
  ".mwb-menu-icons { min-width: 210px; }"
  ".mwb-menu-disk { min-width: 230px; }"
  ".mwb-menu-applications { min-width: 280px; }"
  /* ---- CLASSIC theme (original Ambient dark-blue) ---- */
  ".mwb-theme-classic .mwb-bar {"
  "  background-image: linear-gradient(to bottom,"
  "    #2c3f5c 0%, #253650 35%, #1c2a40 100%);"
  "  background-color: #1c2a40;"
  "  border-top: 1px solid alpha(#ffffff, 0.18);"
  "  border-bottom: 1px solid alpha(#000000, 0.45);"
  "  box-shadow: inset 0 1px 0 alpha(#ffffff, 0.10), inset 0 -1px 0 alpha(#000000, 0.35);"
  "}"
  ".mwb-theme-classic .mwb-button,"
  ".mwb-theme-classic .mwb-volbutton,"
  ".mwb-theme-classic .mwb-clockbtn {"
  "  color: #e6edf5;"
  "  text-shadow: 0 1px 1px alpha(#000000, 0.6);"
  "}"
  ".mwb-theme-classic .mwb-button:hover,"
  ".mwb-theme-classic .mwb-volbutton:hover,"
  ".mwb-theme-classic .mwb-clockbtn:hover {"
  "  background-image: linear-gradient(to bottom,"
  "    alpha(#7fb2e6, 0.55) 0%, alpha(#4a7fb8, 0.45) 100%);"
  "}"
  ".mwb-theme-classic .mwb-button:active, .mwb-theme-classic .mwb-button.open {"
  "  background-image: linear-gradient(to bottom,"
  "    alpha(#17325a, 0.9) 0%, alpha(#2b5588, 0.7) 100%);"
  "}"
  ".mwb-theme-classic .mwb-clocks {"
  "  color: #dbe6f2;"
  "  text-shadow: 0 1px 1px alpha(#000000, 0.6);"
  "}"
  ".mwb-theme-classic .mwb-screenbar-item,"
  ".mwb-theme-classic .mwb-title {"
  "  color: #aebfd4;"
  "  text-shadow: 0 1px 1px alpha(#000000, 0.6);"
  "}"
  ".mwb-theme-classic .mwb-title-active {"
  "  color: #eaf3ff;"
  "}"
  ".mwb-theme-classic .mwb-vgauge-label {"
  "  color: #7e8fa6;"
  "  text-shadow: 0 1px 1px alpha(#000000, 0.7);"
  "}"
  ".mwb-theme-classic .mwb-lamp {"
  "  background-image: linear-gradient(to bottom, #0a1526 0%, #13203a 100%);"
  "  border: 1px solid alpha(#000000, 0.8);"
  "  box-shadow: inset 0 1px 1px alpha(#000000, 0.7), 0 0 0 1px alpha(#ffffff, 0.05);"
  "}"
  ".mwb-theme-classic .mwb-separator {"
  "  background-image: linear-gradient(to bottom,"
  "    alpha(#ffffff, 0.0) 0%, alpha(#ffffff, 0.35) 50%, alpha(#ffffff, 0.0) 100%);"
  "}"
  ".mwb-theme-classic .mwb-screenbar-divider,"
  ".mwb-theme-dark .mwb-screenbar-divider {"
  "  background: alpha(#ffffff, 0.20);"
  "}"
  ".mwb-theme-classic .mwb-menu {"
  "  background-color: rgba(28, 42, 64, 0.97);"
  "  color: #dbe6f2;"
  "  border: 1px solid #0a1526;"
  "  box-shadow: 0 6px 18px alpha(#000000, 0.55);"
  "}"
  ".mwb-theme-classic .mwb-menu > menuitem {"
  "  color: #dbe6f2;"
  "}"
  ".mwb-theme-classic .mwb-menu > menuitem:hover {"
  "  background-image: linear-gradient(to bottom,"
  "    alpha(#6ba3d9, 0.75) 0%, alpha(#3a72ad, 0.75) 100%);"
  "  color: #ffffff;"
  "}"
  ".mwb-theme-classic .mwb-menu > menuitem:disabled {"
  "  color: alpha(#dbe6f2, 0.4);"
  "}"
  ".mwb-theme-classic .mwb-menu > separator {"
  "  background: alpha(#ffffff, 0.10);"
  "}"
  ".mwb-theme-classic .mwb-popup {"
  "  background-color: rgba(24, 38, 58, 0.98);"
  "  color: #dbe6f2;"
  "  border: 1px solid #0a1526;"
  "  box-shadow: 0 8px 22px alpha(#000000, 0.6);"
  "}"
  ".mwb-theme-classic .mwb-popup label,"
  ".mwb-theme-classic .mwb-popup calendar {"
  "  color: #dbe6f2;"
  "}"
  /* ---- DARK theme (sleek near-black) ---- */
  ".mwb-theme-dark .mwb-bar {"
  "  background-image: linear-gradient(to bottom,"
  "    #1c2430 0%, #141a24 45%, #0d121a 100%);"
  "  background-color: #0d121a;"
  "  border-top: 1px solid alpha(#ffffff, 0.10);"
  "  border-bottom: 1px solid alpha(#000000, 0.6);"
  "  box-shadow: inset 0 1px 0 alpha(#ffffff, 0.06), inset 0 -1px 0 alpha(#000000, 0.5);"
  "}"
  ".mwb-theme-dark .mwb-button,"
  ".mwb-theme-dark .mwb-volbutton,"
  ".mwb-theme-dark .mwb-clockbtn {"
  "  color: #cfd8e3;"
  "  text-shadow: 0 1px 1px alpha(#000000, 0.8);"
  "}"
  ".mwb-theme-dark .mwb-button:hover,"
  ".mwb-theme-dark .mwb-volbutton:hover,"
  ".mwb-theme-dark .mwb-clockbtn:hover {"
  "  background-image: linear-gradient(to bottom,"
  "    alpha(#4a8fd4, 0.5) 0%, alpha(#2c5f96, 0.4) 100%);"
  "  color: #ffffff;"
  "}"
  ".mwb-theme-dark .mwb-button:active, .mwb-theme-dark .mwb-button.open {"
  "  background-image: linear-gradient(to bottom,"
  "    alpha(#0d1f33, 0.95) 0%, alpha(#1a3a5c, 0.8) 100%);"
  "  color: #ffffff;"
  "}"
  ".mwb-theme-dark .mwb-clocks {"
  "  color: #c9d4e0;"
  "  text-shadow: 0 1px 1px alpha(#000000, 0.8);"
  "}"
  ".mwb-theme-dark .mwb-screenbar-item,"
  ".mwb-theme-dark .mwb-title {"
  "  color: #97a6b8;"
  "  text-shadow: 0 1px 1px alpha(#000000, 0.7);"
  "}"
  ".mwb-theme-dark .mwb-title-active {"
  "  color: #e4eef8;"
  "}"
  ".mwb-theme-dark .mwb-vgauge-label {"
  "  color: #6f7e92;"
  "  text-shadow: 0 1px 1px alpha(#000000, 0.8);"
  "}"
  ".mwb-theme-dark .mwb-lamp {"
  "  background-image: linear-gradient(to bottom, #05080d 0%, #0b121c 100%);"
  "  border: 1px solid alpha(#000000, 0.9);"
  "  box-shadow: inset 0 1px 1px alpha(#000000, 0.8), 0 0 0 1px alpha(#ffffff, 0.04);"
  "}"
  ".mwb-theme-dark .mwb-separator {"
  "  background-image: linear-gradient(to bottom,"
  "    alpha(#ffffff, 0.0) 0%, alpha(#ffffff, 0.22) 50%, alpha(#ffffff, 0.0) 100%);"
  "}"
  ".mwb-theme-dark .mwb-menu {"
  "  background-color: rgba(12, 17, 25, 0.98);"
  "  color: #c9d4e0;"
  "  border: 1px solid #000000;"
  "  box-shadow: 0 6px 18px alpha(#000000, 0.7);"
  "}"
  ".mwb-theme-dark .mwb-menu > menuitem {"
  "  color: #c9d4e0;"
  "}"
  ".mwb-theme-dark .mwb-menu > menuitem:hover {"
  "  background-image: linear-gradient(to bottom,"
  "    alpha(#4a8fd4, 0.8) 0%, alpha(#2c5f96, 0.8) 100%);"
  "  color: #ffffff;"
  "}"
  ".mwb-theme-dark .mwb-menu > separator {"
  "  background: alpha(#ffffff, 0.08);"
  "}"
  ".mwb-theme-dark .mwb-popup {"
  "  background-color: rgba(10, 15, 22, 0.99);"
  "  color: #c9d4e0;"
  "  border: 1px solid #000000;"
  "  box-shadow: 0 8px 22px alpha(#000000, 0.7);"
  "}"
  ".mwb-theme-dark .mwb-popup label,"
  ".mwb-theme-dark .mwb-popup calendar {"
  "  color: #c9d4e0;"
  "}"
  /* ---- LIGHT theme (silver / classic Workbench metal) ---- */
  ".mwb-theme-light .mwb-bar {"
  "  background-image: linear-gradient(to bottom,"
  "    #f2f4f7 0%, #d9dee5 45%, #b8c0ca 100%);"
  "  background-color: #cdd4dc;"
  "  border-top: 1px solid alpha(#ffffff, 0.85);"
  "  border-bottom: 1px solid alpha(#6a7280, 0.7);"
  "  box-shadow: inset 0 1px 0 alpha(#ffffff, 0.9), inset 0 -1px 0 alpha(#87909b, 0.5);"
  "}"
  ".mwb-theme-light .mwb-button,"
  ".mwb-theme-light .mwb-volbutton,"
  ".mwb-theme-light .mwb-clockbtn {"
  "  color: #2b3440;"
  "  text-shadow: 0 1px 0 alpha(#ffffff, 0.6);"
  "}"
  ".mwb-theme-light .mwb-button:hover,"
  ".mwb-theme-light .mwb-volbutton:hover,"
  ".mwb-theme-light .mwb-clockbtn:hover {"
  "  background-image: linear-gradient(to bottom,"
  "    alpha(#ffffff, 0.75) 0%, alpha(#b9c4d0, 0.65) 100%);"
  "  color: #101722;"
  "}"
  ".mwb-theme-light .mwb-button:active, .mwb-theme-light .mwb-button.open {"
  "  background-image: linear-gradient(to bottom,"
  "    alpha(#a9b6c4, 0.9) 0%, alpha(#c9d3dd, 0.8) 100%);"
  "  color: #101722;"
  "}"
  ".mwb-theme-light .mwb-clocks {"
  "  color: #2b3440;"
  "  text-shadow: 0 1px 0 alpha(#ffffff, 0.6);"
  "}"
  ".mwb-theme-light .mwb-screenbar-item,"
  ".mwb-theme-light .mwb-title {"
  "  color: #5b6672;"
  "  text-shadow: 0 1px 0 alpha(#ffffff, 0.6);"
  "}"
  ".mwb-theme-light .mwb-title-active {"
  "  color: #101722;"
  "}"
  ".mwb-theme-light .mwb-vgauge-label {"
  "  color: #6a7480;"
  "  text-shadow: 0 1px 0 alpha(#ffffff, 0.5);"
  "}"
  ".mwb-theme-light .mwb-lamp {"
  "  background-image: linear-gradient(to bottom, #9aa4b0 0%, #c3ccd5 100%);"
  "  border: 1px solid alpha(#5b6672, 0.7);"
  "  box-shadow: inset 0 1px 1px alpha(#ffffff, 0.8), 0 0 0 1px alpha(#6a7480, 0.3);"
  "}"
  ".mwb-theme-light .mwb-separator {"
  "  background-image: linear-gradient(to bottom,"
  "    alpha(#000000, 0.0) 0%, alpha(#5b6672, 0.45) 50%, alpha(#000000, 0.0) 100%);"
  "}"
  ".mwb-theme-light .mwb-screenbar-divider {"
  "  background: alpha(#5b6672, 0.38);"
  "}"
  ".mwb-theme-classic .mwb-island, .mwb-theme-dark .mwb-island {"
  "  background: alpha(#07111e, 0.42);"
  "  border-top-color: alpha(#ffffff, 0.18);"
  "  border-left-color: alpha(#ffffff, 0.10);"
  "  border-right-color: alpha(#000000, 0.65);"
  "  border-bottom-color: alpha(#000000, 0.65);"
  "}"
  ".mwb-theme-light .mwb-island {"
  "  background: alpha(#f7f9fb, 0.42);"
  "  border-top-color: alpha(#ffffff, 0.85);"
  "  border-left-color: alpha(#ffffff, 0.65);"
  "  border-right-color: alpha(#5b6672, 0.45);"
  "  border-bottom-color: alpha(#5b6672, 0.45);"
  "}"
  ".mwb-theme-light .mwb-menu {"
  "  background-color: rgba(240, 243, 247, 0.99);"
  "  color: #2b3440;"
  "  border: 1px solid #87909b;"
  "  box-shadow: 0 6px 18px alpha(#000000, 0.25);"
  "}"
  ".mwb-theme-light .mwb-menu > menuitem {"
  "  color: #2b3440;"
  "}"
  ".mwb-theme-light .mwb-menu > menuitem:hover {"
  "  background-image: linear-gradient(to bottom,"
  "    #5a87c4 0%, #3f6da8 100%);"
  "  color: #ffffff;"
  "}"
  ".mwb-theme-light .mwb-menu > separator {"
  "  background: alpha(#2b3440, 0.12);"
  "}"
  ".mwb-theme-light .mwb-popup {"
  "  background-color: rgba(240, 243, 247, 0.99);"
  "  color: #2b3440;"
  "  border: 1px solid #87909b;"
  "  box-shadow: 0 8px 22px alpha(#000000, 0.3);"
  "}"
  ".mwb-theme-light .mwb-popup label,"
  ".mwb-theme-light .mwb-popup calendar {"
  "  color: #2b3440;"
  "}"
  /* lamp active colors (shared) */
  ".mwb-lamp.active-net {"
  "  background-image: linear-gradient(to bottom,"
  "    #9fe3ff 0%, #37b0f0 60%, #1570b8 100%);"
  "  box-shadow: inset 0 1px 1px alpha(#ffffff, 0.5),"
  "    0 0 6px alpha(#37b0f0, 0.9), 0 0 12px alpha(#37b0f0, 0.4);"
  "}"
  ".mwb-lamp.active-disk {"
  "  background-image: linear-gradient(to bottom,"
  "    #ffe0a0 0%, #f0a32d 60%, #b87215 100%);"
  "  box-shadow: inset 0 1px 1px alpha(#ffffff, 0.5),"
  "    0 0 6px alpha(#f0a32d, 0.9), 0 0 12px alpha(#f0a32d, 0.4);"
  "}"
  ".mwb-popup scale trough {"
  "  background-image: linear-gradient(to bottom,"
  "    #0f1c30 0%, #16253d 100%);"
  "  border: 1px solid alpha(#0a1526, 0.9);"
  "  border-radius: 4px;"
  "  min-height: 8px;"
  "}"
  ".mwb-popup scale highlight {"
  "  background-image: linear-gradient(to bottom,"
  "    #8fd0ff 0%, #3f8fe0 60%, #2b6cb0 100%);"
  "  border: none;"
  "  border-radius: 4px;"
  "}"
  ".mwb-popup scale slider {"
  "  background-image: linear-gradient(to bottom,"
  "    #eaf4ff 0%, #b8d4f0 100%);"
  "  border: 1px solid #0a1526;"
  "  border-radius: 8px;"
  "  min-width: 14px;"
  "  min-height: 14px;"
  "}"
  "";

/* ------------------------------------------------------------------ *
 *  Plugin structure
 *  ------------------------------------------------------------------ */

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
    GtkWidget       *wifi_button;     /* wifi indicator button */
    GtkWidget       *wifi_icon;       /* wifi image inside button */
    GtkWidget       *batt_button;     /* battery button */
    GtkWidget       *batt_icon;       /* battery image */
    GtkWidget       *batt_label;      /* battery % label */
    GtkWidget       *sys_button;      /* system info button */
    GtkWidget       *vol_button;      /* volume button */
    GtkWidget       *vol_scale;       /* volume slider in popup */
    GtkWidget       *vol_icon;        /* volume icon */
    GtkWidget       *calendar_popup;  /* calendar popup window */
    GtkWidget       *volume_popup;    /* volume popup window */
    GtkWidget       *calendar;        /* the GtkCalendar itself */
    guint            calendar_grab;   /* seat grab timer id */
    guint            volume_grab;     /* seat grab timer id */
    gint64           popup_open_time;
    GtkWidget       *menus[MWB_MENU_COUNT];
    GtkWidget       *menu_buttons[MWB_MENU_COUNT];
    GtkWidget       *active_button;   /* currently open menu button */
    guint            clock_timeout;
    guint            mem_timeout;
    guint            cpu_timeout;
    guint            net_timeout;
    guint            disk_timeout;
    guint            wifi_timeout;
    guint            batt_timeout;
    guint            sys_timeout;
    MorphosWorkbenchTheme theme;
    MorphosWorkbenchLogo logo_variant;
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
    guint64          disk_prev_sects[2];
    gint             wifi_signal;     /* dBm */
    guint            vol_percent;
} MorphosWorkbenchPlugin;

/* Prototypes */
static void mwb_construct(XfcePanelPlugin *plugin);
static void mwb_free_data(XfcePanelPlugin *plugin, MorphosWorkbenchPlugin *mwb);
static gboolean mwb_size_changed(XfcePanelPlugin *plugin, guint size, MorphosWorkbenchPlugin *mwb);
static void mwb_orientation_changed(XfcePanelPlugin *plugin, GtkOrientation orientation, MorphosWorkbenchPlugin *mwb);
static void mwb_configure_plugin(XfcePanelPlugin *plugin, MorphosWorkbenchPlugin *mwb);
static void mwb_save_config(XfcePanelPlugin *plugin, MorphosWorkbenchPlugin *mwb);
static void mwb_load_config(MorphosWorkbenchPlugin *mwb);

static void mwb_build_bar(MorphosWorkbenchPlugin *mwb);
static GtkWidget *mwb_create_menu_title(MorphosWorkbenchPlugin *mwb, const gchar *text);
static void mwb_create_menus(MorphosWorkbenchPlugin *mwb);

static gboolean mwb_tick_clock(MorphosWorkbenchPlugin *mwb);
static gboolean mwb_tick_memory(MorphosWorkbenchPlugin *mwb);
static gboolean mwb_tick_cpu(MorphosWorkbenchPlugin *mwb);
static gboolean mwb_tick_net(MorphosWorkbenchPlugin *mwb);
static gboolean mwb_tick_disk(MorphosWorkbenchPlugin *mwb);
static gboolean mwb_tick_wifi(MorphosWorkbenchPlugin *mwb);
static gboolean mwb_tick_battery(MorphosWorkbenchPlugin *mwb);
static gboolean mwb_tick_sysinfo(MorphosWorkbenchPlugin *mwb);

static gboolean mwb_gauge_draw(GtkWidget *widget, cairo_t *cr, gpointer data);
static void mwb_gauge_set(GtkWidget *gauge, gdouble frac);
static GtkWidget *mwb_gauge_new(gint kind, const gchar *label_text);

static void mwb_menu_toggle(GtkButton *button, MorphosWorkbenchPlugin *mwb);
static void mwb_clock_clicked(GtkButton *button, MorphosWorkbenchPlugin *mwb);
static void mwb_volume_clicked(GtkButton *button, MorphosWorkbenchPlugin *mwb);
static void mwb_volume_changed(GtkRange *range, MorphosWorkbenchPlugin *mwb);
static void mwb_wifi_clicked(GtkButton *button, MorphosWorkbenchPlugin *mwb);
static void mwb_batt_clicked(GtkButton *button, MorphosWorkbenchPlugin *mwb);
static void mwb_sys_clicked(GtkButton *button, MorphosWorkbenchPlugin *mwb);

static void mwb_popup_show(MorphosWorkbenchPlugin *mwb, GtkWidget *win, GtkWidget *anchor);
static void mwb_popup_hide(MorphosWorkbenchPlugin *mwb, GtkWidget *win);
static gboolean mwb_popup_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data);
static gboolean mwb_popup_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data);

/* Launch helper */
static void mwb_launch(const gchar *command);

/* Register plugin */
XFCE_PANEL_PLUGIN_REGISTER(mwb_construct);

/* ------------------------------------------------------------------ *
 *  CSS init
 *  ------------------------------------------------------------------ */

static void
mwb_init_css(void)
{
    static gboolean loaded = FALSE;
    if (loaded)
        return;
    loaded = TRUE;

    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, MWB_CSS, -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                              GTK_STYLE_PROVIDER(provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(provider);
}

static void
mwb_apply_theme(MorphosWorkbenchPlugin *mwb)
{
    static const gchar *classes[MWB_THEME_COUNT] = {
        "mwb-theme-classic", "mwb-theme-dark", "mwb-theme-light"
    };
    GtkStyleContext *ctx = gtk_widget_get_style_context(mwb->bar);
    gint i;
    for (i = 0; i < MWB_THEME_COUNT; i++)
        gtk_style_context_remove_class(ctx, classes[i]);
    gtk_style_context_add_class(ctx, classes[mwb->theme]);
}

static void
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

static const gchar *
mwb_logo_icon_name(MorphosWorkbenchLogo variant)
{
    switch (variant) {
    case MWB_LOGO_FLAT:
        return "ambient-logo-flat";
    case MWB_LOGO_MONO:
        return "ambient-logo-mono";
    case MWB_LOGO_CLASSIC:
    default:
        return MWB_ICON_NAME;
    }
}

static void
mwb_apply_logo(MorphosWorkbenchPlugin *mwb)
{
    if (!mwb->logo_button)
        return;
    GtkWidget *child = gtk_bin_get_child(GTK_BIN(mwb->logo_button));
    if (child && GTK_IS_IMAGE(child))
        gtk_image_set_from_icon_name(GTK_IMAGE(child), mwb_logo_icon_name(mwb->logo_variant), GTK_ICON_SIZE_BUTTON);
}

/* ------------------------------------------------------------------ *
 *  Launch / actions
 *  ------------------------------------------------------------------ */

static void
mwb_launch(const gchar *command)
{
    if (!command || !*command)
        return;
    gchar *argv[4] = { (gchar *)"sh", (gchar *)"-c", (gchar *)command, NULL };
    g_spawn_async(NULL, argv, NULL,
                  G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                  NULL, NULL, NULL, NULL);
}

/* ------------------------------------------------------------------ *
 *  Menu item helper (icon + label, Ambient style)
 *  ------------------------------------------------------------------ */

static GtkWidget *
mwb_create_menu_item(const gchar *icon_name, const gchar *label_text)
{
    GtkWidget *item = gtk_menu_item_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(item), "mwb-appmenu-item");

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    if (icon_name) {
        GtkWidget *img = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_MENU);
        gtk_image_set_pixel_size(GTK_IMAGE(img), 16);
        gtk_box_pack_start(GTK_BOX(box), img, FALSE, FALSE, 0);
    }
    GtkWidget *lbl = gtk_label_new(label_text);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
    gtk_box_pack_start(GTK_BOX(box), lbl, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(item), box);
    gtk_widget_show_all(item);
    return item;
}

/* ------------------------------------------------------------------ *
 *  Menu title button
 *  ------------------------------------------------------------------ */

static GtkWidget *
mwb_create_menu_title(MorphosWorkbenchPlugin *mwb G_GNUC_UNUSED, const gchar *text)
{
    GtkWidget *button = gtk_button_new_with_label(text);
    gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
    gtk_style_context_add_class(gtk_widget_get_style_context(button), "mwb-button");
    gtk_widget_set_can_focus(button, FALSE);
    return button;
}

/* ------------------------------------------------------------------ *
 *  Menu toggle (click)
 *  ------------------------------------------------------------------ */

static void
mwb_close_menus(MorphosWorkbenchPlugin *mwb)
{
    if (mwb->active_button) {
        gtk_style_context_remove_class(gtk_widget_get_style_context(mwb->active_button), "open");
        mwb->active_button = NULL;
    }
    gint i;
    for (i = 0; i < MWB_MENU_COUNT; i++) {
        if (mwb->menus[i])
            gtk_widget_hide(mwb->menus[i]);
    }
}

static void
mwb_menu_toggle(GtkButton *button, MorphosWorkbenchPlugin *mwb)
{
    GtkWidget *menu = NULL;
    gint i;
    for (i = 0; i < MWB_MENU_COUNT; i++) {
        if (mwb->menu_buttons[i] == GTK_WIDGET(button)) {
            menu = mwb->menus[i];
            break;
        }
    }
    if (!menu)
        return;

    if (mwb->active_button == GTK_WIDGET(button) && gtk_widget_get_visible(menu)) {
        gtk_widget_hide(menu);
        mwb_close_menus(mwb);
        return;
    }

    mwb_close_menus(mwb);

    gtk_menu_popup_at_widget(GTK_MENU(menu), GTK_WIDGET(button),
                             GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, NULL);
    mwb->active_button = GTK_WIDGET(button);
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->active_button), "open");
}

/* ------------------------------------------------------------------ *
 *  Horizontal gauge widget (MorphOS Ambient "spacegauge" style)
 *  ------------------------------------------------------------------ */

typedef struct {
    gdouble  frac;
    gint     kind;   /* MWB_GAUGE_* */
    GtkWidget *value_label;
} MwbGaugeData;

static gboolean
mwb_gauge_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    MwbGaugeData *gd = data;
    GtkAllocation alloc;
    gdouble w, h, bw, bh;
    gint i;

    gtk_widget_get_allocation(widget, &alloc);
    w = alloc.width;
    h = alloc.height;

    /* groove */
    cairo_set_source_rgb(cr, 0.035, 0.075, 0.14);
    cairo_rounded_rectangle(cr, 0.5, 0.5, w - 1, h - 1, 3);
    cairo_fill(cr);

    cairo_set_source_rgba(cr, 0.85, 0.90, 0.95, 0.35);
    cairo_rounded_rectangle(cr, 0.5, 0.5, w - 1, h - 1, 3);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_rounded_rectangle(cr, 2.5, 2.5, w - 5, h - 5, 2);
    cairo_fill(cr);

    bw = w - 6;
    bh = h - 6;
    if (bw < 3 || bh < 3)
        return FALSE;

    gdouble fillw = bw * CLAMP(gd->frac, 0.0, 1.0);
    gdouble fx = 3.0, fy = 3.0;

    for (i = 0; i < (gint)fillw; i++) {
        gdouble xx = fx + i;
        gdouble t = (gdouble)i / MAX(fillw - 1.0, 1.0);
        gdouble r, g, b;

        if (gd->kind == MWB_GAUGE_MEM) {
            r = 0.20 + 0.55 * CLAMP(gd->frac * 1.6 - 0.5, 0.0, 1.0);
            g = 0.45 + 0.25 * t;
            b = 0.85 + 0.10 * t;
            r += 0.75 * MAX(0.0, gd->frac - 0.80) * 2.0;
        } else if (gd->kind == MWB_GAUGE_DISK) {
            r = 0.55 + 0.20 * t;
            g = 0.35 + 0.20 * t;
            b = 0.65 + 0.10 * t;
            r += 0.7 * MAX(0.0, gd->frac - 0.85) * 2.0;
        } else {
            r = 0.15 + 0.75 * t;
            g = 0.80 - 0.55 * t;
            b = 0.15 + 0.20 * t;
            r += 0.6 * MAX(0.0, gd->frac - 0.85) * 2.0;
        }
        cairo_set_source_rgb(cr, MIN(r, 1.0), MIN(g, 1.0), MIN(b, 1.0));
        cairo_rectangle(cr, xx, fy, 1.0, bh);
        cairo_fill(cr);
    }

    /* Classical screenbar meters use short dark segment breaks. */
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.48);
    for (i = 8; i < (gint)bw; i += 8) {
        gdouble xx = 3.0 + i;
        cairo_rectangle(cr, xx, 3.0, 1.0, bh);
        cairo_fill(cr);
    }

    return FALSE;
}

static void
mwb_gauge_set(GtkWidget *gauge, gdouble frac)
{
    MwbGaugeData *gd = g_object_get_data(G_OBJECT(gauge), "mwb-gauge-data");
    GtkWidget *draw_widget = gauge;
    if (!gd && GTK_IS_CONTAINER(gauge)) {
        GList *children = gtk_container_get_children(GTK_CONTAINER(gauge));
        for (GList *item = children; item; item = item->next) {
            gd = g_object_get_data(G_OBJECT(item->data), "mwb-gauge-data");
            if (gd) {
                draw_widget = item->data;
                break;
            }
        }
        g_list_free(children);
    }
    if (!gd)
        return;
    gd->frac = frac;
    if (gd->value_label) {
        gchar *value = g_strdup_printf("%d%%", (gint)(CLAMP(frac, 0.0, 1.0) * 100.0));
        gtk_label_set_text(GTK_LABEL(gd->value_label), value);
        g_free(value);
    }
    gtk_widget_queue_draw(draw_widget);
}

static GtkWidget *
mwb_gauge_new(gint kind, const gchar *label_text)
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_style_context_add_class(gtk_widget_get_style_context(box), "mwb-gaugebox");

    if (label_text) {
        GtkWidget *lbl = gtk_label_new(label_text);
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl), "mwb-vgauge-label");
        gtk_box_pack_start(GTK_BOX(box), lbl, FALSE, FALSE, 0);
        gtk_widget_show(lbl);
    }

    GtkWidget *da = gtk_drawing_area_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(da), "mwb-vgauge");
    gtk_widget_set_size_request(da, 76, 10);

    MwbGaugeData *gd = g_new0(MwbGaugeData, 1);
    gd->frac = 0.0;
    gd->kind = kind;
    g_object_set_data_full(G_OBJECT(da), "mwb-gauge-data", gd, g_free);
    g_signal_connect(da, "draw", G_CALLBACK(mwb_gauge_draw), gd);

    gtk_box_pack_start(GTK_BOX(box), da, TRUE, TRUE, 0);
    gtk_widget_show(da);

    GtkWidget *value = gtk_label_new("0%");
    gtk_style_context_add_class(gtk_widget_get_style_context(value), "mwb-gauge-value");
    gtk_box_pack_start(GTK_BOX(box), value, FALSE, FALSE, 0);
    gtk_widget_show(value);
    gd->value_label = value;
    return box;
}

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
    if (g_spawn_command_line_sync("pactl get-sink-volume @DEFAULT_SINK@", &out, NULL, NULL, NULL) && out) {
        guint pct = 0;
        gchar *p = strstr(out, "/");
        if (p && sscanf(p + 1, "%u%%", &pct) == 1) {
            g_free(out);
            mwb->vol_percent = MIN(pct, 100u);
            return TRUE;
        }
        g_free(out);
    }
    return FALSE;
}

static void
mwb_volume_icon_update(MorphosWorkbenchPlugin *mwb)
{
    if (!mwb->vol_icon)
        return;
    const gchar *icon;
    if (mwb->vol_percent == 0)
        icon = "audio-volume-muted";
    else if (mwb->vol_percent < 33)
        icon = "audio-volume-low";
    else if (mwb->vol_percent < 66)
        icon = "audio-volume-medium";
    else
        icon = "audio-volume-high";
    gtk_image_set_from_icon_name(GTK_IMAGE(mwb->vol_icon), icon, GTK_ICON_SIZE_MENU);
}

static void
mwb_volume_changed(GtkRange *range, MorphosWorkbenchPlugin *mwb)
{
    guint pct = (guint)gtk_range_get_value(range);
    mwb->vol_percent = MIN(pct, 100u);
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

        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_container_set_border_width(GTK_CONTAINER(box), 10);

        GtkWidget *lbl = gtk_label_new(_("Volume"));
        gtk_box_pack_start(GTK_BOX(box), lbl, FALSE, FALSE, 0);

        mwb->vol_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
        gtk_scale_set_draw_value(GTK_SCALE(mwb->vol_scale), FALSE);
        gtk_widget_set_size_request(mwb->vol_scale, 160, -1);
        gtk_box_pack_start(GTK_BOX(box), mwb->vol_scale, TRUE, TRUE, 0);

        GtkWidget *mixer = gtk_button_new_with_label(_("Mixer"));
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
    g_signal_handlers_block_by_func(mwb->vol_scale, mwb_volume_changed, mwb);
    gtk_range_set_value(GTK_RANGE(mwb->vol_scale), mwb->vol_percent);
    g_signal_handlers_unblock_by_func(mwb->vol_scale, mwb_volume_changed, mwb);
    mwb_volume_icon_update(mwb);

    mwb_popup_show(mwb, mwb->volume_popup, mwb->vol_button);
}

/* Wifi click — open NetworkManager connection editor */
static void
mwb_wifi_clicked(GtkButton *button G_GNUC_UNUSED, MorphosWorkbenchPlugin *mwb G_GNUC_UNUSED)
{
    if (g_file_test("/usr/bin/nm-connection-editor", G_FILE_TEST_EXISTS))
        mwb_launch("nm-connection-editor");
    else
        mwb_launch("nmcli device wifi list");
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
 *  Menu builders
 *  ------------------------------------------------------------------ */

static GtkWidget *
mwb_build_ambient_menu(MorphosWorkbenchPlugin *mwb G_GNUC_UNUSED)
{
    GtkWidget *menu = gtk_menu_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(menu), "mwb-menu");

    GtkWidget *item;

    item = mwb_create_menu_item("computer", _("About This Computer"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch), "xfce4-about");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("preferences-system", _("Ambient Preferences…"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch), "xfce4-settings-manager");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("system-lock-screen", _("Lock Screen"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xflock4");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("system-log-out", _("Log Out…"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfce4-session-logout --logout");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("system-reboot", _("Restart…"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfce4-session-logout --reboot");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("system-shutdown", _("Shut Down…"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfce4-session-logout --halt");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    gtk_widget_show_all(menu);
    return menu;
}

static GtkWidget *
mwb_build_workbench_menu(MorphosWorkbenchPlugin *mwb G_GNUC_UNUSED)
{
    GtkWidget *menu = gtk_menu_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(menu), "mwb-menu");

    GtkWidget *item;

    item = mwb_create_menu_item("user-home", _("Open Home"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch), "exo-open --launch FileManager");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("folder-documents", _("Open Documents"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "exo-open ~/Dokumenty");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("user-trash", _("Empty Trash"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "gio trash --empty");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("view-refresh", _("Redraw / Refresh"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfdesktop --reload");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    gtk_widget_show_all(menu);
    return menu;
}

static GtkWidget *
mwb_build_ambient_menu_menu(MorphosWorkbenchPlugin *mwb G_GNUC_UNUSED)
{
    GtkWidget *menu = gtk_menu_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(menu), "mwb-menu");

    GtkWidget *item;

    item = mwb_create_menu_item("computer", _("System Info"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch), "hardinfo");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("utilities-terminal", _("Terminal"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "exo-open --launch TerminalEmulator");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("system-run", _("Run…"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfce4-appfinder");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("preferences-system", _("Settings Manager"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfce4-settings-manager");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("video-display", _("Screen Settings"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfce4-display-settings");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("input-keyboard", _("Keyboard"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfce4-keyboard-settings");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("view-fullscreen", _("Open Application Menu"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfce4-popup-whiskermenu");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    gtk_widget_show_all(menu);
    return menu;
}

static GtkWidget *
mwb_build_icons_menu(MorphosWorkbenchPlugin *mwb G_GNUC_UNUSED)
{
    GtkWidget *menu = gtk_menu_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(menu), "mwb-menu");

    GtkWidget *item;

    item = mwb_create_menu_item("view-grid", _("Clean Up Icons"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfdesktop --arrange-icons --all-workspaces");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("view-sort-ascending", _("Sort Icons by Name"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfdesktop --arrange-icons --sort-by name");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("view-sort-descending", _("Sort Icons by Type"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfdesktop --arrange-icons --sort-by type");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("edit-select-all", _("Select All Icons"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "xfdesktop --select-all");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    gtk_widget_show_all(menu);
    return menu;
}

static GtkWidget *
mwb_build_disk_menu(MorphosWorkbenchPlugin *mwb G_GNUC_UNUSED)
{
    GtkWidget *menu = gtk_menu_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(menu), "mwb-menu");

    GtkWidget *item;

    item = mwb_create_menu_item("drive-harddisk", _("Disk Usage"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "exo-open --launch FileManager");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("media-eject", _("Eject / Unmount…"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "udisksctl mount -b /dev/sda1 || true");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("drive-removable-media", _("Mount Volumes"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "exo-open --launch FileManager");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    item = mwb_create_menu_item("media-optical", _("Open Disk Utility"));
    g_signal_connect_swapped(item, "activate", G_CALLBACK(mwb_launch),
                             "gparted");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

    gtk_widget_show_all(menu);
    return menu;
}

static void
mwb_closure_free(gpointer data, GClosure *closure G_GNUC_UNUSED)
{
    g_free(data);
}

static GtkWidget *
mwb_build_applications_menu(MorphosWorkbenchPlugin *mwb G_GNUC_UNUSED)
{
    GtkWidget *menu = gtk_menu_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(menu), "mwb-menu");

    struct { const gchar *icon; const gchar *label; const gchar *cmd; } apps[] = {
        { "utilities-terminal",   N_("Terminal"),        "exo-open --launch TerminalEmulator" },
        { "system-file-manager",  N_("File Manager"),     "exo-open --launch FileManager" },
        { "applications-internet", N_("Web Browser"),     "exo-open --launch WebBrowser" },
        { "applications-multimedia", N_("Multimedia"),    "exo-open --launch Multimedia" },
        { "applications-graphics", N_("Graphics"),        "exo-open --launch Graphics" },
        { "applications-office",  N_("Office"),           "exo-open --launch Office" },
        { "applications-science", N_("Education"),        "exo-open --launch Education" },
        { "applications-system",  N_("System Tools"),     "exo-open --launch SystemTools" },
    };
    guint i;
    for (i = 0; i < G_N_ELEMENTS(apps); i++) {
        GtkWidget *item = mwb_create_menu_item(apps[i].icon, _(apps[i].label));
        gchar *cmd = g_strdup(apps[i].cmd);
        g_signal_connect_data(item, "activate", G_CALLBACK(mwb_launch), cmd,
                              (GClosureNotify)mwb_closure_free, G_CONNECT_SWAPPED);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    }

    gtk_widget_show_all(menu);
    return menu;
}

/* ------------------------------------------------------------------ *
 *  Build the top bar
 *  ------------------------------------------------------------------ */

static void
mwb_create_menus(MorphosWorkbenchPlugin *mwb)
{
    mwb->menus[MWB_MENU_AMBIENT] = mwb_build_ambient_menu(mwb);
    mwb->menus[MWB_MENU_WORKBENCH] = mwb_build_workbench_menu(mwb);
    mwb->menus[MWB_MENU_AMBIENT_MENU] = mwb_build_ambient_menu_menu(mwb);
    mwb->menus[MWB_MENU_ICONS] = mwb_build_icons_menu(mwb);
    mwb->menus[MWB_MENU_DISK] = mwb_build_disk_menu(mwb);
    mwb->menus[MWB_MENU_APPLICATIONS] = mwb_build_applications_menu(mwb);
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->menus[MWB_MENU_WORKBENCH]), "mwb-menu-workbench");
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->menus[MWB_MENU_AMBIENT_MENU]), "mwb-menu-ambient");
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->menus[MWB_MENU_ICONS]), "mwb-menu-icons");
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->menus[MWB_MENU_DISK]), "mwb-menu-disk");
    gtk_style_context_add_class(gtk_widget_get_style_context(mwb->menus[MWB_MENU_APPLICATIONS]), "mwb-menu-applications");
}

static GtkWidget *
mwb_screenbar_divider(void)
{
    GtkWidget *divider = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_style_context_add_class(gtk_widget_get_style_context(divider), "mwb-screenbar-divider");
    return divider;
}

static void
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

static gboolean
mwb_tick_net(MorphosWorkbenchPlugin *mwb)
{
    gchar *content = NULL;
    guint64 tx = 0, rx = 0;

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

    guint64 bytes = tx + rx;
    guint64 delta = mwb->net_prev_bytes > 0 && bytes >= mwb->net_prev_bytes
                  ? bytes - mwb->net_prev_bytes : 0;
    gboolean tx_on = (mwb->net_prev_tx > 0 && tx > mwb->net_prev_tx);
    gboolean rx_on = (mwb->net_prev_rx > 0 && rx > mwb->net_prev_rx);

    GtkStyleContext *ctx;
    if (mwb->net_lamps[MWB_LAMP_NET_TX]) {
        ctx = gtk_widget_get_style_context(mwb->net_lamps[MWB_LAMP_NET_TX]);
        gtk_style_context_remove_class(ctx, "active-net");
        if (tx_on)
            gtk_style_context_add_class(ctx, "active-net");
    }
    if (mwb->net_lamps[MWB_LAMP_NET_RX]) {
        ctx = gtk_widget_get_style_context(mwb->net_lamps[MWB_LAMP_NET_RX]);
        gtk_style_context_remove_class(ctx, "active-net");
        if (rx_on)
            gtk_style_context_add_class(ctx, "active-net");
    }

    gchar *tip = g_strdup_printf(_("Network traffic: %.1f KiB/s"),
                                 (gdouble)delta / 1024.0);
    if (mwb->net_lamps[MWB_LAMP_NET_TX])
        gtk_widget_set_tooltip_text(mwb->net_lamps[MWB_LAMP_NET_TX], tip);
    if (mwb->net_lamps[MWB_LAMP_NET_RX])
        gtk_widget_set_tooltip_text(mwb->net_lamps[MWB_LAMP_NET_RX], tip);
    g_free(tip);

    mwb->net_prev_tx = tx;
    mwb->net_prev_rx = rx;
    mwb->net_prev_bytes = bytes;
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

/* WiFi indicator — reads /proc/net/wireless */
static gboolean
mwb_tick_wifi(MorphosWorkbenchPlugin *mwb)
{
    gchar *content = NULL;
    gint signal = G_MININT;

    if (g_file_get_contents("/proc/net/wireless", &content, NULL, NULL) && content) {
        gchar *tok = content;
        while (tok && *tok) {
            gchar *line = strchr(tok, '\n');
            if (line)
                *line = '\0';
            gchar iface[64] = "";
            guint link = 0;
            gint level = 0, noise = 0;
            if (sscanf(tok, " %63[^:]: %u. %d. %d", iface, &link, &level, &noise) >= 2) {
                if (level != 0)
                    signal = level;
            }
            tok = line ? line + 1 : NULL;
        }
        g_free(content);
    }

    if (!mwb->wifi_icon)
        return G_SOURCE_CONTINUE;

    mwb->wifi_signal = signal;

    const gchar *icon;
    gchar *tip;

    if (signal == G_MININT) {
        icon = "network-wireless-offline-symbolic";
        tip = g_strdup(_("No wireless interface"));
    } else {
        if (signal >= -50)      icon = "network-wireless-signal-excellent-symbolic";
        else if (signal >= -60) icon = "network-wireless-signal-good-symbolic";
        else if (signal >= -67) icon = "network-wireless-signal-ok-symbolic";
        else                    icon = "network-wireless-signal-weak-symbolic";
        tip = g_strdup_printf(_("Wi-Fi signal: %d dBm"), signal);
    }

    gtk_image_set_from_icon_name(GTK_IMAGE(mwb->wifi_icon), icon, GTK_ICON_SIZE_MENU);
    gtk_widget_set_tooltip_text(mwb->wifi_button ? mwb->wifi_button : mwb->wifi_icon, tip);
    g_free(tip);
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
 *  Plugin lifecycle
 *  ------------------------------------------------------------------ */

static void
mwb_construct(XfcePanelPlugin *plugin)
{
    MorphosWorkbenchPlugin *mwb = g_slice_new0(MorphosWorkbenchPlugin);
    mwb->plugin = plugin;
    mwb->theme = MWB_THEME_CLASSIC;
    mwb->logo_variant = MWB_LOGO_CLASSIC;
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

    /* ------------------------------------------------------------------ *
     *  Screenbar (right side) — mirrors MorphOS Ambient screenbar:
     *  Drivelamps, Netlamps, Wifi, Battery, CPU, Mem, Disk, Volume, Sys, Time
     *  ------------------------------------------------------------------ */
    GtkWidget *right = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_style_context_add_class(gtk_widget_get_style_context(right), "mwb-hbox");
    GtkWidget *status_group = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_style_context_add_class(gtk_widget_get_style_context(status_group), "mwb-status-group");

    /* ---- Drivelamps ---- */
    if (mwb->show_drivelamps) {
        GtkWidget *diskbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
        gtk_style_context_add_class(gtk_widget_get_style_context(diskbox), "mwb-island");
        guint d;
        for (d = 0; d < 2; d++) {
            mwb->disk_lamps[d] = gtk_label_new(" ");
            gtk_style_context_add_class(gtk_widget_get_style_context(mwb->disk_lamps[d]), "mwb-lamp");
            gtk_widget_set_tooltip_text(mwb->disk_lamps[d], _("Disk activity"));
            gtk_box_pack_start(GTK_BOX(diskbox), mwb->disk_lamps[d], FALSE, FALSE, 0);
        }
        gtk_box_pack_start(GTK_BOX(right), diskbox, FALSE, FALSE, 0);
    }

    gtk_box_pack_start(GTK_BOX(right), mwb_screenbar_divider(), FALSE, FALSE, 0);

    /* ---- Netlamps ---- */
    if (mwb->show_netlamps) {
        GtkWidget *netbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
        gtk_style_context_add_class(gtk_widget_get_style_context(netbox), "mwb-island");
        mwb->net_lamps[MWB_LAMP_NET_TX] = gtk_label_new(" ");
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->net_lamps[MWB_LAMP_NET_TX]), "mwb-lamp");
        gtk_widget_set_tooltip_text(mwb->net_lamps[MWB_LAMP_NET_TX], _("Network transmit"));
        gtk_box_pack_start(GTK_BOX(netbox), mwb->net_lamps[MWB_LAMP_NET_TX], FALSE, FALSE, 0);

        mwb->net_lamps[MWB_LAMP_NET_RX] = gtk_label_new(" ");
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->net_lamps[MWB_LAMP_NET_RX]), "mwb-lamp");
        gtk_widget_set_tooltip_text(mwb->net_lamps[MWB_LAMP_NET_RX], _("Network receive"));
        gtk_box_pack_start(GTK_BOX(netbox), mwb->net_lamps[MWB_LAMP_NET_RX], FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(right), netbox, FALSE, FALSE, 0);
    }

    gtk_box_pack_start(GTK_BOX(right), mwb_screenbar_divider(), FALSE, FALSE, 0);

    /* ---- WiFi indicator ---- */
    if (mwb->show_wifi) {
        mwb->wifi_button = gtk_button_new();
        gtk_button_set_relief(GTK_BUTTON(mwb->wifi_button), GTK_RELIEF_NONE);
        gtk_style_context_add_class(gtk_widget_get_style_context(mwb->wifi_button), "mwb-volbutton");
        mwb->wifi_icon = gtk_image_new_from_icon_name("network-wireless-signal-weak-symbolic",
                                                      GTK_ICON_SIZE_MENU);
        gtk_container_add(GTK_CONTAINER(mwb->wifi_button), mwb->wifi_icon);
        gtk_widget_show(mwb->wifi_icon);
        g_signal_connect(mwb->wifi_button, "clicked", G_CALLBACK(mwb_wifi_clicked), mwb);
        gtk_box_pack_start(GTK_BOX(status_group), mwb->wifi_button, FALSE, FALSE, 0);
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
        mwb->cpu_gauges[0] = mwb_gauge_new(MWB_GAUGE_CPU, _("CPU"));
        gtk_box_pack_start(GTK_BOX(cpurow), mwb->cpu_gauges[0], FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(right), cpurow, FALSE, FALSE, 0);
    }

    gtk_box_pack_start(GTK_BOX(right), mwb_screenbar_divider(), FALSE, FALSE, 0);

    /* ---- Memory vertical gauge ---- */
    if (mwb->show_membar) {
        GtkWidget *memrow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
        mwb->mem_gauge = mwb_gauge_new(MWB_GAUGE_MEM, _("RAM"));
        gtk_box_pack_start(GTK_BOX(memrow), mwb->mem_gauge, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(right), memrow, FALSE, FALSE, 0);
    }

    /* ---- Disk vertical gauge ---- */
    if (mwb->show_diskgauge) {
        GtkWidget *diskrow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
        mwb->disk_gauge = mwb_gauge_new(MWB_GAUGE_DISK, _("DISK"));
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
    if (mwb->show_volume) {
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
        mwb->net_timeout = g_timeout_add_seconds(1, (GSourceFunc)mwb_tick_net, mwb);
    }
    if (mwb->show_drivelamps) {
        mwb_tick_disk(mwb);
        mwb->disk_timeout = g_timeout_add_seconds(1, (GSourceFunc)mwb_tick_disk, mwb);
    }
    if (mwb->show_wifi) {
        mwb_tick_wifi(mwb);
        mwb->wifi_timeout = g_timeout_add_seconds(3, (GSourceFunc)mwb_tick_wifi, mwb);
    }
    if (mwb->show_battery) {
        mwb_tick_battery(mwb);
        mwb->batt_timeout = g_timeout_add_seconds(5, (GSourceFunc)mwb_tick_battery, mwb);
    }
    if (mwb->show_sysinfo || mwb->show_diskgauge) {
        mwb_tick_sysinfo(mwb);
        mwb->sys_timeout = g_timeout_add_seconds(5, (GSourceFunc)mwb_tick_sysinfo, mwb);
    }

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
    if (mwb->wifi_timeout)
        g_source_remove(mwb->wifi_timeout);
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

    g_slice_free(MorphosWorkbenchPlugin, mwb);
}

static gboolean
mwb_size_changed(XfcePanelPlugin *plugin G_GNUC_UNUSED, guint size, MorphosWorkbenchPlugin *mwb)
{
    gtk_widget_set_size_request(mwb->bar, -1, size);
    return TRUE;
}

static void
mwb_orientation_changed(XfcePanelPlugin *plugin G_GNUC_UNUSED, GtkOrientation orientation, MorphosWorkbenchPlugin *mwb G_GNUC_UNUSED)
{
    if (orientation == GTK_ORIENTATION_HORIZONTAL)
        gtk_widget_show_all(mwb->bar);
}

/* ------------------------------------------------------------------ *
 *  Config
 *  ------------------------------------------------------------------ */

static void
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

static void
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
    if (mwb->wifi_button)
        gtk_widget_set_visible(mwb->wifi_button, mwb->show_wifi);
    if (mwb->batt_button)
        gtk_widget_set_visible(mwb->batt_button, mwb->show_battery);
    if (mwb->sys_button)
        gtk_widget_set_visible(mwb->sys_button, mwb->show_sysinfo);
}

static void
mwb_configure_plugin(XfcePanelPlugin *plugin, MorphosWorkbenchPlugin *mwb)
{
    GtkWidget *dialog, *content_area, *grid;
    GtkWidget *ck_clock, *ck_mem, *ck_cpu, *ck_net, *ck_disk, *ck_vol, *ck_wifi, *ck_batt, *ck_sys, *ck_diskgauge;
    GtkWidget *ck_logo, *ck_title, *ck_wbmenu, *ck_ambmenu, *ck_iconsmenu, *ck_diskmenu, *ck_appsmenu;
    GtkWidget *theme_combo;
    GtkWidget *logo_combo;
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

    row++;
    GtkWidget *left_section = gtk_label_new(_("Left side / Workbench root:"));
    gtk_label_set_xalign(GTK_LABEL(left_section), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(left_section), "mwb-settings-section");
    gtk_grid_attach(GTK_GRID(grid), left_section, 0, row++, 2, 1);

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

    GtkWidget *lbl2 = gtk_label_new(_("Screenbar:"));
    gtk_label_set_xalign(GTK_LABEL(lbl2), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl2), "mwb-settings-section");
    gtk_grid_attach(GTK_GRID(grid), lbl2, 0, row++, 2, 1);

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
        if (mwb->wifi_button)
            gtk_widget_set_visible(mwb->wifi_button, mwb->show_wifi);
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
