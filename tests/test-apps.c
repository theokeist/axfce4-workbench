/*
 * Copyright (C) 2026 XFCE4 MorphOS Workbench Plugin Team
 * GPL-2.0-or-later
 */

#include "../src/mwb-apps.h"

static void
check_clean(const gchar *in, const gchar *expected)
{
    gchar *out = mwb_app_exec_clean(in);
    g_assert_cmpstr(out, ==, expected);
    g_free(out);
}

int
main(void)
{
    check_clean("firefox %U", "firefox");
    check_clean("env FOO=bar myapp %F", "env FOO=bar myapp");
    check_clean("app 100%%", "app 100%");
    check_clean("/usr/bin/app", "/usr/bin/app");

    g_assert_cmpuint(mwb_app_categorize("Game;ActionGame"), ==, MWB_CAT_GAMES);
    g_assert_cmpuint(mwb_app_categorize("Network;WebBrowser"), ==, MWB_CAT_INTERNET);
    g_assert_cmpuint(mwb_app_categorize("Utility"), ==, MWB_CAT_ACCESSORIES);
    g_assert_cmpuint(mwb_app_categorize("TerminalEmulator;System"), ==, MWB_CAT_SYSTEM);
    g_assert_cmpuint(mwb_app_categorize(NULL), ==, MWB_CAT_OTHER);
    g_assert_cmpuint(mwb_app_categorize("FooBar"), ==, MWB_CAT_OTHER);

    g_print("apps: OK\n");
    return 0;
}
