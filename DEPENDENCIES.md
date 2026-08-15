# Dependencies & Package Requirements

This document lists build and runtime dependencies for **XFCE4 MorphOS Workbench Plugin** across major Linux distributions and package formats (`.deb`, `.rpm`, `.pkg.tar.zst`).

---

## 1. Arch Linux / Manjaro / EndeavourOS (`.pkg.tar.zst`)

### Package Names (Pacman)
- **Build Dependencies**:
  ```bash
  sudo pacman -S --needed \
      base-devel \
      meson \
      ninja \
      pkgconf \
      glib2 \
      gtk3 \
      libxfce4panel \
      libxfce4ui \
      libxfce4util \
      exo
  ```
- **Runtime Dependencies**:
  ```bash
  sudo pacman -S --needed \
      xfce4-panel \
      xfce4-session \
      exo
  ```

---

## 2. Debian / Ubuntu / Linux Mint / Pop!_OS (`.deb`)

### Package Names (APT)
- **Build Dependencies**:
  ```bash
  sudo apt update && sudo apt install -y \
      build-essential \
      meson \
      ninja-build \
      pkg-config \
      libglib2.0-dev \
      libgtk-3-dev \
      libxfce4panel-2.0-dev \
      libxfce4ui-2-dev \
      libxfce4util-dev \
      libexo-2-dev
  ```
- **Runtime Dependencies**:
  ```bash
  sudo apt install -y \
      xfce4-panel \
      xfce4-session \
      exo-utils
  ```

---

## 3. Fedora / RHEL / CentOS / Rocky Linux (`.rpm`)

### Package Names (DNF / YUM)
- **Build Dependencies**:
  ```bash
  sudo dnf install -y \
      gcc \
      meson \
      ninja-build \
      pkgconfig \
      glib2-devel \
      gtk3-devel \
      libxfce4panel-devel \
      libxfce4ui-devel \
      libxfce4util-devel \
      exo-devel
  ```
- **Runtime Dependencies**:
  ```bash
  sudo dnf install -y \
      xfce4-panel \
      xfce4-session \
      exo
  ```

---

## 4. openSUSE Tumbleweed / Leap (`.rpm`)

### Package Names (Zypper)
- **Build Dependencies**:
  ```bash
  sudo zypper install -y \
      gcc \
      meson \
      ninja \
      pkg-config \
      glib2-devel \
      gtk3-devel \
      libxfce4panel-devel \
      libxfce4ui-devel \
      libxfce4util-devel \
      libexo-devel
  ```
- **Runtime Dependencies**:
  ```bash
  sudo zypper install -y \
      xfce4-panel \
      xfce4-session \
      exo
  ```

---

## 5. Minimum Dependency Versions Matrix

| Dependency | Minimum Version | Purpose |
|---|---|---|
| **GTK+** | `>= 3.24.0` | Menu windows & UI widgets |
| **GLib / GIO** | `>= 2.66.0` | App launching, file/meminfo reading |
| **libxfce4panel** | `>= 4.16.0` | XFCE panel plugin interface (v2.0 API) |
| **libxfce4ui** | `>= 4.16.0` | Settings dialog & system actions |
| **libxfce4util** | `>= 4.16.0` | Utility helpers & RC-file config |
| **libexo** *(recommended)* | `>= 4.16.0` | `exo-open` preferred-application launching |
| **Meson** | `>= 0.49.0` | Build orchestration |
| **Ninja** | `>= 1.8.0` | Build executor |

> Note: this project uses `libxfce4panel-2.0` (XFCE 4.16+ panel API). Older `libxfce4panel-1.0` panels are **not** supported.

---

## 6. Build & Installation Quick-Start

```bash
# 1. Clone the repository
git clone https://github.com/theokeist/axfce4-workbench.git
cd axfce4-workbench

# 2. Setup build directory
meson setup build --prefix=/usr

# 3. Compile
ninja -C build

# 4. Install system-wide
sudo ninja -C build install

# 5. Reload XFCE Panel
xfce4-panel -r
```

### User-local (no sudo) build

```bash
meson setup build --prefix="$HOME/.local"
ninja -C build
ninja -C build install
gtk-update-icon-cache -f -t "$HOME/.local/share/icons/hicolor"
xfce4-panel -r
```
