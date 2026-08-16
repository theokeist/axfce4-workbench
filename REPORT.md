# Performance & Concurrency Report — XFCE4 MorphOS Workbench Plugin

This report reviews the plugin for performance bottlenecks, race conditions,
and GUI re-entrancy hazards. It is a read-only analysis; no code was changed.

---

## 1. Execution model

- **Single-threaded GTK main loop.** Every timer callback, signal handler, and
  child-watch callback runs on the one GLib main loop. GTK is not thread-safe;
  all widget access is (correctly) confined to the main thread.
- **No worker threads.** Metrics are read by polling `/proc` from timer
  callbacks. External programs are spawned with `g_spawn_*`.
- **Timers** (in `src/mwb-screenbar.c`, `mwb_build_screenbar`):
  | Source | Interval | Reads / does |
  |---|---|---|
  | clock | 1 s | `localtime()` |
  | cpu | 1 s | `/proc/stat` |
  | mem | 2 s | `/proc/meminfo` |
  | **net** | **100 ms** | `/proc/net/dev` + CSS class toggles + tooltip |
  | disk | 1 s | `/proc/diskstats` |
  | battery | 5 s | `/sys/class/power_supply/BAT0/*` |
  | sysinfo | 5 s | `/proc/meminfo` + **`df -P /`** + **`uname -sr`** (subprocesses) |

Because everything is on the main thread, **any blocking work stalls the entire
UI**, and **any per-tick allocation adds GC pressure** to the loop.

---

## 2. Performance bottlenecks

### P1 — Blocking synchronous subprocesses on the main thread (High)

- `mwb_tick_sysinfo()` runs `df -P /` and `uname -sr` via
  `g_spawn_command_line_sync()` every 5 s (`src/mwb-screenbar.c:765,780`).
- `mwb_volume_get_percent()` / `mwb_volume_get_muted()` run
  `pactl get-sink-volume` / `pactl get-sink-mute` synchronously when the volume
  popup opens (`src/mwb-screenbar.c:193,221`).

`g_spawn_command_line_sync` **blocks the main loop** until the child exits.
`pactl` can stall waiting on PulseAudio, and `df` can hang on a dead NFS/network
mount — each stall freezes the whole panel.

**Fix:** switch these to `g_spawn_async` + `GAsyncQueue`/callback, or
`gio`/`GSubprocess` with a completion callback; cache `uname -sr` once at
startup instead of re-spawning every 5 s.

### P2 — Netlamps poll at 100 ms with allocation churn (Medium)

`mwb_tick_net()` (`src/mwb-screenbar.c`) runs every 100 ms and, on each tick:

1. opens/reads/parses `/proc/net/dev`,
2. adds/removes CSS classes (`active-net` / `active-net-high`) on two widgets,
3. `g_strdup_printf` a tooltip string,
4. sets the tooltip on both lamps.

At 10 Hz this is constant file I/O + string churn. Most ticks are identical
(idle). The tooltip is rebuilt even when the rate is unchanged.

**Fix:** cache the previous per-direction state and only touch CSS/tooltip when
it actually changes; throttle the tooltip update to ~1 s while keeping the
flicker at a faster rate, or lower the poll to ~250 ms.

### P3 — Volume slider spawn storm + zombie leak (High)

`mwb_volume_changed()` fires on **every** `value-changed` during a drag and
spawns `pactl set-sink-volume …` each time (`mwb_launch`). Dragging the slider
rapidly spawns dozens of `pactl` processes. Combined with **R1** (unreaped
children) this leaks a zombie process per event.

**Fix:** debounce/coalesce the `pactl` set (e.g., send on `change-value`/release,
or a 150–250 ms `g_timeout` that collapses intermediate values).

### P4 — Applications menu destroyed/rebuilt on every open (Low)

`mwb_rebuild_applications_menu()` destroys the whole `GtkMenu` and rebuilds it
(static items + recent section) each time the Applications menu is opened. It
is small (< 15 items) so the cost is negligible, but the destroy/recreate churn
also feeds **R4**.

**Fix:** rebuild in place (clear + repopulate), or only rebuild when the recent
list actually changed.

### P5 — Minor

- `g_list_length()` inside `mwb_recent_record()` is O(n), but n ≤
  `MWB_RECENT_MAX` (3) — negligible.
- `mwb_app_activate()` duplicates `icon`/`label`/`cmd` strings per launch — tiny.
- `mwb_init_css()` is guarded by a `static gboolean loaded`, so the provider is
  registered once — good. It is only added to the **default** screen, so a
  multi-screen setup would not get the plugin CSS on secondary screens.

---

## 3. Race conditions & concurrency hazards

### R1 — Unreaped children → zombie processes (High)

`mwb_launch()` (`src/mwb-utils.c`) spawns `sh -c "…"` with
`G_SPAWN_DO_NOT_REAP_CHILD` and **never reaps** the child (no
`g_child_watch_add`, no `waitpid`). Every untracked launch (volume slider,
`xfdesktop --reload`, `gio trash --empty`, lock/logout, etc.) leaves a zombie
`sh` process until the panel process exits. Under heavy use this accumulates.

**Fix:** drop `G_SPAWN_DO_NOT_REAP_CHILD` (let GLib reap), or always attach a
`g_child_watch_add`.

### R2 — Tracked-launch lifecycle (Safe, but fragile)

`mwb_app_closed()` (`src/mwb-menus.c`) frees its `MwbTrackedLaunch` and removes
it from `mwb->tracked_launches`; `mwb_tracked_launches_clear()` cancels sources
and frees on teardown. On the single-threaded loop this ordering is correct and
the unit test passes. Remaining fragility:

- The `t->mwb = NULL` guard in `mwb_tracked_launches_clear()` is defensive but
  relies on the fact that the child-watch callback cannot run once
  `g_source_remove()` has returned. That invariant holds only because everything
  is single-threaded; if a worker thread is ever introduced this becomes a real
  use-after-free.

**Fix:** keep the watch source ids explicitly and ensure clear runs before the
plugin struct is freed (already done); do not add threads without a lock.

### R3 — Popup seat grab / outside-click dismiss is broken (GUI bug)

`mwb_popup_show()` (`src/mwb-screenbar.c:88`) **never calls `gdk_seat_grab`**,
but `mwb_popup_hide()` **always calls `gdk_seat_ungrab`**. The
`calendar_grab`/`volume_grab` fields are dead code (cleared, never set).

Consequence: the calendar/volume popups rely on `button-press-event` to dismiss
on outside clicks, but that event only reaches the popup window. Without a grab,
clicks outside the popup go to other windows, so the popup does **not** reliably
dismiss on outside click (Escape still works via `key-press-event`).

**Fix:** implement the seat grab the same way the embedded
`xfce4-networkmanager` plugin does (`gdk_seat_grab` on show, retry on failure,
ungrab on hide), and wire up the grab timers.

### R4 — Applications menu destroyed while possibly referenced

`mwb_rebuild_applications_menu()` calls `gtk_widget_destroy()` on the current
Applications menu and replaces `mwb->menus[MWB_MENU_APPLICATIONS]`. If any
callback still holds a reference to the old menu (a queued popup/position
callback, or the `open` class on `active_button`), that is a latent
use-after-free. The toggle-off path avoids destroying the open menu in the
common case, but the pattern is fragile.

**Fix:** rebuild in place, or `g_object_ref`/`g_object_unref` the old menu
around the swap.

### R5 — `mwb->active_button` dangling pointer

`mwb->active_button` is a raw pointer to the currently-open menu's button. In
`mwb_free_data()` the menus are destroyed, then the bar (which owns the
buttons) is destroyed, but `active_button` is **not** cleared first. It is only
safe because `mwb` is freed immediately after with no further use. Any future
code that touches `active_button` after bar teardown would dereference freed
memory.

**Fix:** set `mwb->active_button = NULL` before destroying the bar (or call
`mwb_close_menus()` first).

### R6 — Static CSS provider scope

`mwb_init_css()` registers the provider for `gdk_screen_get_default()` only.
Not a race, but on multi-screen/multi-display setups the CSS does not apply to
secondary screens. Consider registering per-screen or using the application
provider.

---

## 4. GUI race-condition specifics

1. **Popup dismissal** (R3): the "outside click to close" mechanism is
   effectively a GUI race — the dismiss only works if the click happens to land
   on the popup, because the seat is never grabbed.
2. **Menu swap** (R4): destroying a `GtkMenu` during its own button handler is
   re-entrant; if a nested event (grab, timer) fires in between, the old menu is
   dereferenced after free.
3. **Volume drag** (P3): a burst of `pactl` children is spawned from
   `value-changed`; each is un-reaped (R1), so a single drag can briefly spawn a
   storm of zombies and momentarily starve the main loop.
4. **Blocking popup open** (P1): opening the volume popup blocks the main loop
   on two synchronous `pactl` calls, which is visible as a UI freeze on slow
   PulseAudio.

---

## 5. Recommended fix order

| # | Issue | Severity | Effort | Suggested fix |
|---|---|---|---|---|
| 1 | R1 zombie leak | High | Low | Remove `G_SPAWN_DO_NOT_REAP_CHILD` from `mwb_launch` |
| 2 | P3 volume spawn storm | High | Low | Debounce `pactl set-sink-volume` |
| 3 | P1 blocking `pactl`/`df`/`uname` | High | Med | Async `GSubprocess`; cache `uname` |
| 4 | R3 popup seat grab | Medium | Med | Implement `gdk_seat_grab` on show |
| 5 | P2 net tick churn | Medium | Low | State-diff before CSS/tooltip updates |
| 6 | R5 active_button teardown | Low | Low | Clear `active_button` in `free_data` |
| 7 | R4/P4 menu rebuild | Low | Med | Rebuild in place / rebuild on change only |
| 8 | R6 multi-screen CSS | Low | Low | Register provider per screen |
