# Icon installation
icon_sizes = ['16x16', '22x22', '24x24']

foreach size : icon_sizes
  install_data(
    size / 'ambient-logo.svg',
    install_dir: get_option('datadir') / 'icons' / 'hicolor' / size / 'apps'
  )
endforeach

# Compact logo variants used by the Workbench root preferences.
install_data(
  '16x16/ambient-logo-flat.svg',
  '16x16/ambient-logo-mono.svg',
  '16x16/ambient-logo-leaf.svg',
  '16x16/ambient-logo-f.svg',
  install_dir: get_option('datadir') / 'icons' / 'hicolor' / '16x16' / 'apps'
)

# Update icon cache after installation
meson.add_install_script('gtk-update-icon-cache',
  join_paths(get_option('prefix'), get_option('datadir'), 'icons', 'hicolor'),
  skip_if_destdir: true
)
