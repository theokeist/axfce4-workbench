# Unit tests

test_deps = [
  glib_dep,
  gtk_dep,
  libxfce4panel_dep,
  libxfce4ui_dep,
  libxfce4util_dep,
]

test('recent-apps',
  executable('test-recent',
    'test-recent.c',
    '../src/mwb-recent.c',
    dependencies: test_deps,
    include_directories: inc
  )
)

test('launch-tracked',
  executable('test-launch',
    'test-launch.c',
    '../src/mwb-utils.c',
    dependencies: test_deps,
    include_directories: inc
  )
)

test('apps',
  executable('test-apps',
    'test-apps.c',
    '../src/mwb-apps.c',
    dependencies: test_deps,
    include_directories: inc
  )
)
