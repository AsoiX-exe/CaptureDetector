# capscan

**Screen Capture Protection Scanner** — a tiny cross-platform CLI that lists your
visible windows and tells you which ones are *hidden from screenshots and screen
recording*.

![platform](https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-2b2b2b?style=flat-square)
![language](https://img.shields.io/badge/language-C-00599C?style=flat-square)
![deps](https://img.shields.io/badge/dependencies-none-2b2b2b?style=flat-square)
![license](https://img.shields.io/badge/license-MIT-2b2b2b?style=flat-square)

---

## What it does

Windows, macOS and some apps can flag a window as *"do not capture me"*. When you
take a screenshot or share your screen, that window shows up as a **black box** or
disappears entirely — you still see it with your eyes, but the camera doesn't.

This is used by DRM video players, secure document viewers, banking apps… and
sometimes by **malware** that hides its own window from being recorded.

`capscan` is an X-ray for that flag. It walks every open window, puts the hidden
ones front and center (in red), and lists the rest below — without changing
anything.

```
  capscan  screen capture protection scanner
  coded by AsoiX

  !!  1 window(s) HIDDEN from screen capture  !!
  won't show up in screenshots or screen recording

    HIDDEN   DoNotCapture.exe        MONITOR    pid 15940   <no title>

  other visible windows
             chrome.exe              NONE       pid 14088   GitHub - Chrome
             Яндекс Музыка.exe       NONE       pid 9228    Яндекс Музыка

  17 scanned   1 hidden
```

Hidden windows are shown with a red `HIDDEN` badge up top so they can't blend in
with everything else.

## The `AFFINITY` column

The underlying mechanism is different on every OS, so the column reports the native
state:

| OS | API | Values | Protected when |
|----|-----|--------|----------------|
| **Windows** | `GetWindowDisplayAffinity` | `NONE`, `MONITOR`, `EXCLUDE` | not `NONE` |
| **macOS** | `CGWindow` sharing state | `SHARING-NONE`, `READONLY`, `READWRITE` | `SHARING-NONE` |
| **Linux** | — | `N/A` | never* |

\* X11/Wayland has no per-window capture-exclusion flag; capture is handled at the
compositor level. On Linux `capscan` still lists managed windows (via
`_NET_CLIENT_LIST`) for parity, but affinity is always `N/A`.

## Build

No dependencies beyond the platform's system libraries.

```sh
# Windows (MinGW)
gcc capscan.c -o capscan.exe -O2

# Windows (MSVC)
cl capscan.c user32.lib

# macOS (clang)
clang capscan.c -o capscan -framework CoreGraphics -framework CoreFoundation

# Linux (gcc) — needs libX11 dev headers (e.g. apt install libx11-dev)
gcc capscan.c -o capscan -lX11
```

## Usage

```
capscan [options]
  -p, --protected-only   show only capture-protected windows
  -a, --all              include windows without a title
  -h, --help             show help
```

Protected windows are **always** shown (even untitled ones) so nothing hiding from
capture can slip past the filter.

On Windows, if you double-click `capscan.exe` from Explorer it keeps the window open
and waits for **Enter** so you can read the output. Run it from a terminal and it
just prints and exits.

## How it works

`capscan` enumerates top-level windows and, for each, reads the OS-native
"capture sharing" state plus the owning process id and executable name. It only
reads — it never sets affinity or touches any window.

- **Windows:** `EnumWindows` + `GetWindowDisplayAffinity`
- **macOS:** `CGWindowListCopyWindowInfo` → `kCGWindowSharingState`
- **Linux:** `_NET_CLIENT_LIST` / `_NET_WM_NAME` / `_NET_WM_PID`

## Caveats

- **macOS:** owner name and pid are always available; reading window *titles* may
  require granting the terminal the **Screen Recording** permission.
- **Linux:** requires an X11 session (or XWayland). Pure Wayland compositors do not
  expose a global window list.
- No elevated privileges are needed for windows in your own session.

## License

MIT

