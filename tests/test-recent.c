/*
 * Copyright (C) 2026 XFCE4 MorphOS Workbench Plugin Team
 * GPL-2.0-or-later
 */

#include "../src/morphos-workbench.h"

static guint
list_length(GList *l)
{
    guint n = 0;
    for (; l; l = l->next)
        n++;
    return n;
}

int
main(void)
{
    MorphosWorkbenchPlugin *mwb = g_new0(MorphosWorkbenchPlugin, 1);
    MwbRecentApp *first;

    /* record three distinct apps -> most recent first */
    mwb_recent_record(mwb, "a", "A", "cmd-a");
    mwb_recent_record(mwb, "b", "B", "cmd-b");
    mwb_recent_record(mwb, "c", "C", "cmd-c");
    g_assert_cmpuint(list_length(mwb->recent_apps), ==, 3);
    first = mwb->recent_apps->data;
    g_assert_cmpstr(first->cmd, ==, "cmd-c");

    /* duplicate re-record -> moves to front, no growth */
    mwb_recent_record(mwb, "a", "A", "cmd-a");
    g_assert_cmpuint(list_length(mwb->recent_apps), ==, 3);
    first = mwb->recent_apps->data;
    g_assert_cmpstr(first->cmd, ==, "cmd-a");

    /* cap at MWB_RECENT_MAX */
    mwb_recent_record(mwb, "d", "D", "cmd-d");
    g_assert_cmpuint(list_length(mwb->recent_apps), ==, MWB_RECENT_MAX);

    first = mwb->recent_apps->data;
    g_assert_cmpint(first->cycles, ==, MWB_RECENT_CYCLES);

    /* expire after MWB_RECENT_CYCLES ticks */
    mwb_recent_tick(mwb);
    mwb_recent_tick(mwb);
    g_assert_cmpuint(list_length(mwb->recent_apps), ==, MWB_RECENT_MAX);
    mwb_recent_tick(mwb);
    g_assert_cmpuint(list_length(mwb->recent_apps), ==, 0);

    mwb_recent_clear(mwb);
    g_assert_null(mwb->recent_apps);

    g_free(mwb);
    g_print("recent-apps: OK\n");
    return 0;
}
