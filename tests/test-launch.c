/*
 * Copyright (C) 2026 XFCE4 MorphOS Workbench Plugin Team
 * GPL-2.0-or-later
 */

#include "../src/morphos-workbench.h"

static gboolean fired = FALSE;

static void
on_child_exit(GPid pid G_GNUC_UNUSED, gint status G_GNUC_UNUSED, gpointer data)
{
    GMainLoop *loop = data;
    fired = TRUE;
    g_main_loop_quit(loop);
}

static gboolean
on_timeout(gpointer data)
{
    g_main_loop_quit((GMainLoop *)data);
    return G_SOURCE_REMOVE;
}

int
main(void)
{
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    guint id = mwb_launch_tracked("true", on_child_exit, loop);
    guint timer;

    g_assert_cmpuint(id, !=, 0);

    /* safety: quit after 5s if the watch never fires */
    timer = g_timeout_add(5000, on_timeout, loop);
    g_main_loop_run(loop);
    g_source_remove(timer);
    g_main_loop_unref(loop);

    g_assert_true(fired);
    g_print("launch-tracked: OK\n");
    return 0;
}
