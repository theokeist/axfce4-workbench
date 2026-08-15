project('xfce4-morphos-workbench', 'c',
  version: '0.1.0',
  license: 'GPL-2.0-or-later',
  default_options: [
    'c_std=gnu11',
    'warning_level=2',
    'buildtype=debugoptimized',
  ],
  meson_version: '>= 0.49.0'
)

# Add project arguments
cc = meson.get_compiler('c')
add_project_arguments([
  '-DHAVE_CONFIG_H',
  '-DPACKAGE_LOCALE_DIR="@0@"'.format(get_option('prefix') / get_option('localedir')),
], language: 'c')

# Dependencies
glib_dep = dependency('glib-2.0', version: '>= 2.66')
gtk_dep = dependency('gtk+-3.0', version: '>= 3.24')
libxfce4panel_dep = dependency('libxfce4panel-2.0', version: '>= 4.16')
libxfce4ui_dep = dependency('libxfce4ui-2', version: '>= 4.16')
libxfce4util_dep = dependency('libxfce4util-1.0', version: '>= 4.16')
exo_dep = dependency('exo-2', version: '>= 4.16', required: false)

# Create config.h
config_h = configuration_data()
config_h.set_quoted('PACKAGE_NAME', meson.project_name())
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set_quoted('VERSION', meson.project_version())
config_h.set_quoted('GETTEXT_PACKAGE', meson.project_name())
config_h.set('ENABLE_NLS', true)

configure_file(
  output: 'config.h',
  configuration: config_h
)

# Include directories
inc = include_directories('.')

# i18n
i18n = import('i18n')

# Subdirectories
subdir('src')
subdir('data')
subdir('po')

# Summary
summary({
  'prefix': get_option('prefix'),
  'libdir': get_option('prefix') / get_option('libdir'),
  'datadir': get_option('prefix') / get_option('datadir'),
}, section: 'Directories')
