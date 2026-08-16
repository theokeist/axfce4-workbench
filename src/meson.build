# Plugin sources
morphos_workbench_sources = [
  'morphos-workbench.c',
  'mwb-theme.c',
  'mwb-utils.c',
  'mwb-gauges.c',
  'mwb-menus.c',
  'mwb-screenbar.c',
  'mwb-config.c'
]

# Dependencies for the plugin
morphos_workbench_deps = [
  glib_dep,
  gtk_dep,
  libxfce4panel_dep,
  libxfce4ui_dep,
  libxfce4util_dep,
]

if exo_dep.found()
  morphos_workbench_deps += exo_dep
endif

# Build the plugin as a shared module
morphos_workbench_lib = shared_module('morphos-workbench',
  morphos_workbench_sources,
  dependencies: morphos_workbench_deps,
  include_directories: inc,
  install: true,
  install_dir: get_option('libdir') / 'xfce4' / 'panel' / 'plugins',
  name_prefix: 'lib',
  c_args: [
    '-DG_LOG_DOMAIN="xfce4-morphos-workbench"',
  ]
)

# Desktop file
desktop_file = i18n.merge_file(
  input: 'morphos-workbench.desktop.in',
  output: 'morphos-workbench.desktop',
  type: 'desktop',
  po_dir: '../po',
  install: true,
  install_dir: get_option('datadir') / 'xfce4' / 'panel' / 'plugins'
)
