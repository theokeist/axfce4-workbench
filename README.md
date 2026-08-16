# XFCE4 MorphOS Workbench Plugin

A panel plugin for XFCE4 that recreates the **MorphOS Ambient Workbench top bar** — the sleek screen-top menu bar of the Amiga-like MorphOS operating system — on your XFCE desktop.

## Features

### 🧭 Ambient menu bar
- Classic Workbench-style menu titles at the top of the screen
- **Ambient logo** button — system menu (About, Preferences, Lock, Log Out, Restart, Shut Down)
- **Workbench** menu — Home, Documents, Terminal, Search, Empty Trash, Redraw / Refresh
- **Ambient** menu — System Info, Task Manager, Terminal, Run…, File Manager, Web Browser, Settings
- **Icons** menu — New Folder, Clean Up, desktop actions, Desktop Settings
- **Disk** menu — File Manager, Removable Media, Eject / Unmount, Disk Utility
- **Applications** menu — all installed apps (categorized) plus recently closed apps

### 📊 Screenbar (right side)
- CPU, memory and disk **gauges** (Industrial / Glossy 3D / Plain styles)
- **Netlamps** — traffic-light diodes (red idle → yellow traffic → blue heavy)
- **Drivelamps** — amber disk-activity diodes
- **Wi-Fi** — embedded `xfce4-networkmanager` (connect / disconnect, Wi-Fi toggle)
- **Volume** — popup slider, mute toggle and mixer (PulseAudio)
- **Battery** indicator, **System info** button and **Clock** with calendar popup

### 🎨 Theming & appearance
- Three themes — Classic, Dark, Light — plus an **"Override GTK theme"** option
- Gauge style selection (Industrial / Glossy 3D / Plain)
- Ambient logo variants (Classic / Flat / Monochrome)
- **Dynamic title** — the title follows the foreground application
- Reorderable screenbar indicators

### ⚙️ Usability
- Tabbed settings dialog with sections and per-option explanations
- Internationalization (gettext)
- Unit-tested (`meson test`)

## Menu Structure (mirrors MorphOS Ambient)

| Menu | Items |
|---|---|
| **Ambient (logo)** | About This Computer · Ambient Preferences · Lock Screen · Log Out · Restart · Shut Down |
| **Workbench** | Open Home · Open Documents · Open Terminal · Search Applications · Empty Trash · Redraw / Refresh |
| **Ambient** | System Info · Task Manager · Terminal · Run… · File Manager · Web Browser · Settings Manager · Screen Settings · Keyboard · Open Application Menu |
| **Icons** | New Folder · Clean Up Icons · Open Desktop Menu · Window List · Next Wallpaper · Reload Desktop · Desktop Settings |
| **Disk** | Open File Manager · Open Removable Media · Eject / Unmount… · Open Disk Utility |
| **Applications** | Recently closed apps · All installed apps (Accessories, Development, Games, Graphics, Internet, Multimedia, Office, Science, Settings, System, Other) |

## Desktop Environment & Window Manager Compatibility

| Desktop / Session | Compatibility Status | Notes |
|---|---|---|
| **XFCE 4.20** | **Full (Supported)** | Tested on X11 (`xfwm4`) and Wayland (`labwc`/`wayfire`) via wrapper-2.0. |
| **XFCE 4.18 / 4.16** | **Compatible** | Built against `libxfce4panel-2.0` / GTK 3.24+. |

## Requirements & Package Dependencies

See [DEPENDENCIES.md](DEPENDENCIES.md) for full build/runtime requirements.

> **Runtime note:** the Wi-Fi section embeds the **xfce4-networkmanager** panel plugin
> (`libxfce4-networkmanager.so`). It is `dlopen()`ed at runtime (not linked at build time),
> so install that plugin for the Wi-Fi indicator to work.

### Quick Install Commands

- **Debian / Ubuntu / Linux Mint (`.deb`)**:
  ```bash
  sudo apt update && sudo apt install -y build-essential meson ninja-build pkg-config \
      libglib2.0-dev libgtk-3-dev libxfce4panel-2.0-dev libxfce4ui-2-dev libxfce4util-dev libexo-2-dev
  ```
- **Arch Linux (`.pkg.tar.zst`)**:
  ```bash
  sudo pacman -S --needed base-devel meson ninja pkgconf glib2 gtk3 libxfce4panel libxfce4ui libxfce4util exo
  ```
- **Fedora / RHEL (`.rpm`)**:
  ```bash
  sudo dnf install -y gcc meson ninja-build pkgconfig glib2-devel gtk3-devel \
      libxfce4panel-devel libxfce4ui-devel libxfce4util-devel exo-devel
  ```

## Installation

### From Source

```bash
git clone https://github.com/theokeist/axfce4-workbench.git
cd axfce4-workbench

meson setup build --prefix=/usr
ninja -C build
sudo ninja -C build install

# Restart XFCE Panel
xfce4-panel -r
```

### User-local install (no root)

```bash
meson setup build --prefix="$HOME/.local"
ninja -C build
ninja -C build install
gtk-update-icon-cache -f -t "$HOME/.local/share/icons/hicolor"
xfce4-panel -r
```

Then add it through **Panel Preferences → Items → Add → MorphOS Workbench**, or on the command line:

```bash
# add to the first panel
xfce4-panel -a morphos-workbench
```

## Configuration

Right-click the plugin and select **Properties** to open a tabbed settings dialog (**General**, **Workbench**, **Screenbar**) with sections and per-option explanations:

- **General**: theme (Classic / Dark / Light), Ambient logo, gauge style (Industrial / Glossy 3D / Plain), and "Override GTK theme"
- **Workbench**: Ambient logo, title, dynamic title, and the menu toggles
- **Screenbar**: enable/disable each meter and indicator, and reorder them with Up/Down arrows (applied immediately)

Settings are persisted to `~/.config/xfce4/panel/morphos-workbench-<id>.rc`.

## Testing

Run the unit tests with:

```bash
meson test -C build
```

## Theming

The plugin ships with an embedded CSS provider that gives the bar its Ambient look (dark blue gradient, inset highlights, dark dropdown menus with blue hover). To override, add rules to `~/.config/gtk-3.0/gtk.css`, e.g.:

```css
.mwb-bar {
    background-image: linear-gradient(to bottom, #101828 0%, #0b1120 100%);
}
.mwb-membar > trough > progress {
    background-image: linear-gradient(to bottom, #6ee7b7 0%, #059669 100%);
}
```

## Troubleshooting

### Plugin not appearing
1. Ensure the module was installed: `ls ~/.local/lib/xfce4/panel/plugins/libmorphos-workbench.so`
2. Restart the panel fully: `xfce4-panel -r` (or log out/in) so it re-scans plugin `.desktop` files.
3. Add it via Panel Preferences → Items → Add.

### Ambient logo missing
The icon is registered as `ambient-logo`. If missing, refresh the icon cache:
```bash
gtk-update-icon-cache -f -t "$HOME/.local/share/icons/hicolor"
```

### Menus not launching apps
The Applications menu uses `exo-open --launch <category>`. Install `exo-utils` and register your preferred apps in **Settings → Preferred Applications**.

## Author

theokeist
- GitHub: https://github.com/theokeist

## License

This project is licensed under the **GPL-2.0-or-later** License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- MorphOS & the Ambient open-source team — for the desktop design inspiration
- XFCE Development Team for the panel plugin framework

## Links

- [MorphOS](https://morphos-team.net)
- [Ambient (MorphOS desktop)](https://morphosambient.sf.net)
- [Report Issues](https://github.com/theokeist/axfce4-workbench/issues)
