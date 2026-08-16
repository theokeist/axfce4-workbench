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

#include <math.h>

/* ------------------------------------------------------------------ *
 *  Horizontal gauge widget (MorphOS Ambient "spacegauge" style)
 *  ------------------------------------------------------------------ */

typedef struct {
    gdouble    frac;
    gint       kind;   /* MWB_GAUGE_* */
    gint       theme;  /* MorphosWorkbenchTheme */
    gint       style;  /* MorphosWorkbenchGaugeStyle */
    GtkWidget *value_label;
} MwbGaugeData;

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

static void
mwb_gauge_color(gint kind G_GNUC_UNUSED, gdouble t, gdouble frac, gdouble *r, gdouble *g, gdouble *b)
{
    /* single chromatic scheme shared by CPU / RAM / DISK: blue -> amber -> red */
    gdouble load = CLAMP(frac, 0.0, 1.0);
    gdouble sr, sg, sb, er, eg, eb;

    if (load < 0.5) {
        gdouble k = load / 0.5;
        sr = 0.20 + 0.70 * k; sg = 0.50 + 0.25 * k; sb = 0.85 - 0.65 * k;
        er = 0.45 + 0.50 * k; eg = 0.60 + 0.25 * k; eb = 0.70 - 0.55 * k;
    } else {
        gdouble k = (load - 0.5) / 0.5;
        sr = 0.90 + 0.10 * k; sg = 0.75 - 0.70 * k; sb = 0.20 - 0.15 * k;
        er = 0.95;            eg = 0.85 - 0.80 * k; eb = 0.15 - 0.10 * k;
    }

    *r = MIN(sr + (er - sr) * t, 1.0);
    *g = MIN(sg + (eg - sg) * t, 1.0);
    *b = MIN(sb + (eb - sb) * t, 1.0);
}

static void
mwb_gauge_draw_industrial(cairo_t *cr, MwbGaugeData *gd, gdouble w, gdouble h, gboolean light)
{
    gdouble bw, bh, radius, inner_radius, frac;
    gint i;

    radius = MIN(3.0, h / 2.0);

    /* flat industrial recess */
    if (light)
        cairo_set_source_rgb(cr, 0.66, 0.69, 0.74);
    else
        cairo_set_source_rgb(cr, 0.02, 0.04, 0.08);
    cairo_rounded_rectangle(cr, 0.5, 0.5, w - 1, h - 1, radius);
    cairo_fill(cr);

    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, light ? 0.20 : 0.60);
    cairo_rounded_rectangle(cr, 0.5, 0.5, w - 1, h - 1, radius);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);

    /* flat inner well */
    inner_radius = MAX(radius - 1.5, 1.0);
    if (light)
        cairo_set_source_rgb(cr, 0.93, 0.94, 0.96);
    else
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_rounded_rectangle(cr, 2.5, 2.5, w - 5, h - 5, inner_radius);
    cairo_fill(cr);

    bw = w - 6;
    bh = h - 6;
    if (bw < 3 || bh < 3)
        return;

    frac = CLAMP(gd->frac, 0.0, 1.0);

    /* classic segmented LED blocks */
    gdouble pitch = 8.0;
    gint nsegs = MAX((gint)(bw / pitch), 1);
    gint filled = (gint)(frac * nsegs + 0.5);

    for (i = 0; i < filled && i < nsegs; i++) {
        gdouble t = nsegs > 1 ? (gdouble)i / (nsegs - 1) : 0.0;
        gdouble r, g, b;
        mwb_gauge_color(gd->kind, t, frac, &r, &g, &b);
        cairo_set_source_rgb(cr, r, g, b);
        gdouble xx = 3.0 + i * pitch;
        cairo_rounded_rectangle(cr, xx, 3.0, pitch - 2.0, bh, 1.5);
        cairo_fill(cr);
    }
}

static void
mwb_gauge_draw_3d(cairo_t *cr, MwbGaugeData *gd, gdouble w, gdouble h, gboolean light)
{
    gdouble bw, bh, radius, inner_radius, frac;
    gdouble fillw, fx, fy;
    gint i;

    radius = h / 2.0;

    /* outer frame (raised chrome bezel) */
    if (light)
        cairo_set_source_rgb(cr, 0.58, 0.61, 0.66);
    else
        cairo_set_source_rgb(cr, 0.03, 0.055, 0.10);
    cairo_rounded_rectangle(cr, 0.5, 0.5, w - 1, h - 1, radius);
    cairo_fill(cr);

    cairo_set_line_width(cr, 1.0);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, light ? 0.20 : 0.60);
    cairo_rounded_rectangle(cr, 0.5, 0.5, w - 1, h - 1, radius);
    cairo_stroke(cr);

    /* sunken, vertically shaded inner well */
    inner_radius = (h - 5) / 2.0;
    if (inner_radius < 1.0)
        inner_radius = 1.0;
    {
        cairo_pattern_t *well = cairo_pattern_create_linear(0, 2.5, 0, h - 2.5);
        if (light) {
            cairo_pattern_add_color_stop_rgba(well, 0.0, 0, 0, 0, 0.32);
            cairo_pattern_add_color_stop_rgba(well, 1.0, 1, 1, 1, 0.55);
        } else {
            cairo_pattern_add_color_stop_rgba(well, 0.0, 0, 0, 0, 0.85);
            cairo_pattern_add_color_stop_rgba(well, 1.0, 1, 1, 1, 0.14);
        }
        cairo_set_source(cr, well);
        cairo_rounded_rectangle(cr, 2.5, 2.5, w - 5, h - 5, inner_radius);
        cairo_fill(cr);
        cairo_pattern_destroy(well);
    }

    bw = w - 6;
    bh = h - 6;
    if (bw < 3 || bh < 3)
        return;

    frac = CLAMP(gd->frac, 0.0, 1.0);
    fillw = bw * frac;
    fx = 3.0;
    fy = 3.0;

    if (fillw >= 1.0) {
        gdouble r0, g0, b0, r1, g1, b1;
        mwb_gauge_color(gd->kind, 0.0, frac, &r0, &g0, &b0);
        mwb_gauge_color(gd->kind, 1.0, frac, &r1, &g1, &b1);

        gdouble use_r = MIN(bh / 2.0, fillw / 2.0);

        cairo_pattern_t *pat = cairo_pattern_create_linear(fx, fy, fx + fillw, fy);
        cairo_pattern_add_color_stop_rgb(pat, 0.0, r0, g0, b0);
        cairo_pattern_add_color_stop_rgb(pat, 1.0, r1, g1, b1);
        cairo_set_source(cr, pat);
        cairo_rounded_rectangle(cr, fx, fy, fillw, bh, use_r);
        cairo_fill(cr);
        cairo_pattern_destroy(pat);

        cairo_save(cr);
        cairo_rounded_rectangle(cr, fx, fy, fillw, bh, use_r);
        cairo_clip(cr);
        cairo_pattern_t *cyl = cairo_pattern_create_linear(fx, fy, fx, fy + bh);
        cairo_pattern_add_color_stop_rgba(cyl, 0.0, 1, 1, 1, 0.55);
        cairo_pattern_add_color_stop_rgba(cyl, 0.28, 1, 1, 1, 0.05);
        cairo_pattern_add_color_stop_rgba(cyl, 0.55, 0, 0, 0, 0.05);
        cairo_pattern_add_color_stop_rgba(cyl, 1.0, 0, 0, 0, 0.55);
        cairo_set_source(cr, cyl);
        cairo_paint(cr);
        cairo_pattern_destroy(cyl);

        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, light ? 0.75 : 0.5);
        cairo_rounded_rectangle(cr, fx + 1.0, fy + 1.0, fillw - 2.0, bh * 0.16, use_r);
        cairo_fill(cr);
        cairo_restore(cr);
    }

    /* segment breaks over the full track */
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, light ? 0.22 : 0.48);
    for (i = 8; i < (gint)bw; i += 8) {
        gdouble xx = 3.0 + i;
        cairo_rounded_rectangle(cr, xx, 3.0 + bh * 0.18, 1.0, bh * 0.64, 0.5);
        cairo_fill(cr);
    }
}

static void
mwb_gauge_draw_plain(cairo_t *cr, MwbGaugeData *gd, gdouble w, gdouble h, gboolean light)
{
    gdouble bw, bh, frac;
    gint i;

    /* flat well, no rounding */
    if (light)
        cairo_set_source_rgb(cr, 0.95, 0.96, 0.97);
    else
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_rectangle(cr, 1.0, 1.0, w - 2, h - 2);
    cairo_fill(cr);

    /* thin 1px frame, no rounding */
    cairo_set_line_width(cr, 1.0);
    if (light)
        cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.45);
    else
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.25);
    cairo_rectangle(cr, 0.5, 0.5, w - 1, h - 1);
    cairo_stroke(cr);

    bw = w - 4;
    bh = h - 4;
    if (bw < 3 || bh < 3)
        return;

    frac = CLAMP(gd->frac, 0.0, 1.0);

    /* square segments */
    gdouble pitch = 8.0;
    gint nsegs = MAX((gint)(bw / pitch), 1);
    gint filled = (gint)(frac * nsegs + 0.5);

    for (i = 0; i < filled && i < nsegs; i++) {
        gdouble t = nsegs > 1 ? (gdouble)i / (nsegs - 1) : 0.0;
        gdouble r, g, b;
        mwb_gauge_color(gd->kind, t, frac, &r, &g, &b);
        cairo_set_source_rgb(cr, r, g, b);
        gdouble xx = 2.0 + i * pitch;
        cairo_rectangle(cr, xx, 2.0, pitch - 1.0, bh);
        cairo_fill(cr);
    }
}

static gboolean
mwb_check_is_light(GtkWidget *widget, MorphosWorkbenchTheme theme)
{
    if (theme == MWB_THEME_LIGHT)
        return TRUE;
    if (theme == MWB_THEME_SYSTEM && widget != NULL) {
        GtkStyleContext *ctx = gtk_widget_get_style_context(widget);
        GdkRGBA bg;
        if (gtk_style_context_lookup_color(ctx, "theme_bg_color", &bg)) {
            gdouble lum = 0.299 * bg.red + 0.587 * bg.green + 0.114 * bg.blue;
            return lum > 0.5;
        }
    }
    return FALSE;
}

static gboolean
mwb_gauge_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    MwbGaugeData *gd = data;
    GtkAllocation alloc;
    gdouble w, h;
    gboolean light;

    gtk_widget_get_allocation(widget, &alloc);
    w = alloc.width;
    h = alloc.height;
    light = mwb_check_is_light(widget, gd->theme);

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);

    if (gd->style == MWB_GAUGE_STYLE_3D)
        mwb_gauge_draw_3d(cr, gd, w, h, light);
    else if (gd->style == MWB_GAUGE_STYLE_PLAIN)
        mwb_gauge_draw_plain(cr, gd, w, h, light);
    else
        mwb_gauge_draw_industrial(cr, gd, w, h, light);

    return FALSE;
}

static MwbGaugeData *
mwb_gauge_find(GtkWidget *gauge, GtkWidget **draw_widget)
{
    MwbGaugeData *gd = g_object_get_data(G_OBJECT(gauge), "mwb-gauge-data");
    GtkWidget *dw = gauge;

    if (!gd && GTK_IS_CONTAINER(gauge)) {
        GList *children = gtk_container_get_children(GTK_CONTAINER(gauge));
        for (GList *item = children; item; item = item->next) {
            gd = g_object_get_data(G_OBJECT(item->data), "mwb-gauge-data");
            if (gd) {
                dw = item->data;
                break;
            }
        }
        g_list_free(children);
    }
    if (draw_widget)
        *draw_widget = dw;
    return gd;
}

void
mwb_gauge_set(GtkWidget *gauge, gdouble frac)
{
    GtkWidget *draw_widget = NULL;
    MwbGaugeData *gd = mwb_gauge_find(gauge, &draw_widget);
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

void
mwb_gauge_set_theme(GtkWidget *gauge, MorphosWorkbenchTheme theme)
{
    GtkWidget *draw_widget = NULL;
    MwbGaugeData *gd = mwb_gauge_find(gauge, &draw_widget);
    if (!gd)
        return;
    gd->theme = theme;
    gtk_widget_queue_draw(draw_widget);
}

void
mwb_gauge_set_style(GtkWidget *gauge, MorphosWorkbenchGaugeStyle style)
{
    GtkWidget *draw_widget = NULL;
    MwbGaugeData *gd = mwb_gauge_find(gauge, &draw_widget);
    if (!gd)
        return;
    gd->style = style;
    gtk_widget_queue_draw(draw_widget);
}

GtkWidget *
mwb_gauge_new(gint kind, const gchar *label_text, MorphosWorkbenchTheme theme,
              MorphosWorkbenchGaugeStyle style)
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
    gtk_widget_set_size_request(da, 76, 14);

    MwbGaugeData *gd = g_new0(MwbGaugeData, 1);
    gd->frac = 0.0;
    gd->kind = kind;
    gd->theme = theme;
    gd->style = style;
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
