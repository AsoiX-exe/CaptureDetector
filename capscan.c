/*
 * capscan - Screen Capture Protection Scanner (cross-platform)
 * ---------------------------------------------------------------
 * Lists visible top-level windows and reports whether each one is
 * hidden from screenshots / screen recording ("capture protection").
 *
 * The mechanism differs per OS, so the AFFINITY column means:
 *
 *   Windows : SetWindowDisplayAffinity state
 *               NONE / MONITOR / EXCLUDEFROMCAP
 *   macOS   : CGWindow sharing state
 *               SHARING-NONE (protected) / READONLY / READWRITE
 *   Linux   : N/A - X11/Wayland has no per-window capture-exclusion
 *
 * Windows marked [!] are protected (won't appear in captures).
 *
 * Build:
 *   Windows (MinGW) : gcc capscan.c -o capscan.exe -O2
 *   Windows (MSVC)  : cl capscan.c user32.lib
 *   macOS (clang)   : clang capscan.c -o capscan \
 *                       -framework CoreGraphics -framework CoreFoundation
 *   Linux (gcc)     : gcc capscan.c -o capscan -lX11
 *
 * No elevated privileges required for windows in your own session.
 */
#include <stdio.h>
#include <string.h>

static int g_protected_only = 0;
static int g_include_untitled = 0;
static int g_count = 0;
static int g_protected = 0;

static void print_field(const char *s, int width); /* defined below */

/* Central reporting + filtering, shared by every platform backend.
 * Rule: protected windows are ALWAYS shown (even without a title);
 * --protected-only hides unprotected ones; untitled unprotected
 * windows are hidden unless --all is given. */
static void report(int is_protected, const char *affinity, unsigned long pid,
                   const char *proc, const char *title, int has_title) {
    if (g_protected_only && !is_protected) return;
    if (!has_title && !g_include_untitled && !is_protected) return;

    printf("  %-10s %-9s %-7lu ",
           is_protected ? "[DETECTED]" : "",
           affinity,
           pid);
    print_field((proc && proc[0]) ? proc : "?", 24);
    printf(" %s\n", has_title ? title : "<no title>");

    g_count++;
    if (is_protected) g_protected++;
}

static void enumerate(void); /* implemented per platform below */

/* Print a UTF-8 string in exactly `width` visible columns: pad with spaces
 * if short, truncate with ".." if long. Counts Unicode code points (not
 * bytes) so Cyrillic / accented names line up in the table. */
static void print_field(const char *s, int width) {
    int total = 0;
    for (const char *q = s; *q; ++q)
        if ((*q & 0xC0) != 0x80) total++; /* count code-point starts */

    if (total <= width) {
        fputs(s, stdout);
        for (int i = total; i < width; ++i) putchar(' ');
    } else {
        int lim = width - 2, c = 0;
        for (const char *p = s; *p; ++p) {
            if ((*p & 0xC0) != 0x80) {
                if (c >= lim) break;
                c++;
            }
            putchar((unsigned char)*p);
        }
        fputs("..", stdout);
    }
}


/* ======================== Windows backend ======================== */
#if defined(_WIN32)
#include <windows.h>
#include <wchar.h>

#ifndef WDA_NONE
#define WDA_NONE 0x00000000
#endif
#ifndef WDA_MONITOR
#define WDA_MONITOR 0x00000001
#endif
#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif

static const char *affinity_name(DWORD a) {
    switch (a) {
        case WDA_NONE:               return "NONE";
        case WDA_MONITOR:            return "MONITOR";
        case WDA_EXCLUDEFROMCAPTURE: return "EXCLUDE";
        default:                     return "UNKNOWN";
    }
}

static void to_utf8(const wchar_t *in, char *out, int cap) {
    if (WideCharToMultiByte(CP_UTF8, 0, in, -1, out, cap, NULL, NULL) == 0)
        out[0] = 0;
    out[cap - 1] = 0;
}

static void process_image(DWORD pid, char *out, int cap) {
    out[0] = 0;
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return;
    wchar_t path[MAX_PATH];
    DWORD sz = MAX_PATH;
    if (QueryFullProcessImageNameW(h, 0, path, &sz)) {
        wchar_t *name = path;
        for (wchar_t *p = path; *p; ++p)
            if (*p == L'\\' || *p == L'/') name = p + 1;
        to_utf8(name, out, cap);
    }
    CloseHandle(h);
}

static BOOL CALLBACK on_window(HWND hwnd, LPARAM lp) {
    (void)lp;
    if (!IsWindowVisible(hwnd)) return TRUE;

    wchar_t wtitle[256];
    int len = GetWindowTextW(hwnd, wtitle, 256);
    char title[512];
    title[0] = 0;
    if (len > 0) to_utf8(wtitle, title, sizeof title);

    DWORD aff = WDA_NONE;
    GetWindowDisplayAffinity(hwnd, &aff);

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    char exe[MAX_PATH * 2];
    process_image(pid, exe, sizeof exe);

    report(aff != WDA_NONE, affinity_name(aff), (unsigned long)pid,
           exe, title, len > 0);
    return TRUE;
}

static void enumerate(void) {
    SetConsoleOutputCP(CP_UTF8);
    EnumWindows(on_window, 0);
}


/* ======================== macOS backend ========================= */
#elif defined(__APPLE__)
#include <CoreGraphics/CoreGraphics.h>
#include <CoreFoundation/CoreFoundation.h>

static void cfstr_utf8(CFStringRef s, char *out, int cap) {
    out[0] = 0;
    if (s) CFStringGetCString(s, out, cap, kCFStringEncodingUTF8);
}

static long dict_int(CFDictionaryRef d, CFStringRef key) {
    long v = 0;
    CFNumberRef n = (CFNumberRef)CFDictionaryGetValue(d, key);
    if (n) CFNumberGetValue(n, kCFNumberLongType, &v);
    return v;
}

/* macOS: kCGWindowSharingNone(0) means the window is NOT shared to other
 * apps, i.e. excluded from capture. Reading window titles may require the
 * Screen Recording permission; owner name and pid do not. */
static void enumerate(void) {
    CFArrayRef list = CGWindowListCopyWindowInfo(
        kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements,
        kCGNullWindowID);
    if (!list) { fprintf(stderr, "CGWindowListCopyWindowInfo failed\n"); return; }

    for (CFIndex i = 0, n = CFArrayGetCount(list); i < n; ++i) {
        CFDictionaryRef w = (CFDictionaryRef)CFArrayGetValueAtIndex(list, i);
        long sharing = dict_int(w, kCGWindowSharingState);
        long pid = dict_int(w, kCGWindowOwnerPID);

        char proc[256], title[512];
        cfstr_utf8((CFStringRef)CFDictionaryGetValue(w, kCGWindowOwnerName),
                   proc, sizeof proc);
        title[0] = 0;
        CFStringRef nm = (CFStringRef)CFDictionaryGetValue(w, kCGWindowName);
        int has_title = nm && CFStringGetLength(nm) > 0;
        if (has_title) cfstr_utf8(nm, title, sizeof title);

        const char *aff = sharing == 0 ? "SHARING-NONE"
                        : sharing == 1 ? "READONLY" : "READWRITE";
        report(sharing == 0, aff, (unsigned long)pid, proc, title, has_title);
    }
    CFRelease(list);
}

/* ======================== Linux (X11) backend =================== */
#elif defined(__linux__)
#include <X11/Xlib.h>
#include <X11/Xatom.h>

static unsigned char *get_prop(Display *d, Window w, Atom prop, Atom type,
                               unsigned long *nitems) {
    Atom rtype; int rfmt; unsigned long bytes; unsigned char *data = NULL;
    if (XGetWindowProperty(d, w, prop, 0, (~0L), False, type,
                           &rtype, &rfmt, nitems, &bytes, &data) != Success)
        return NULL;
    return data;
}

static void proc_name(long pid, char *out, int cap) {
    out[0] = 0;
    if (pid <= 0) return;
    char path[64];
    snprintf(path, sizeof path, "/proc/%ld/comm", pid);
    FILE *f = fopen(path, "r");
    if (!f) return;
    if (fgets(out, cap, f)) out[strcspn(out, "\n")] = 0;
    fclose(f);
}

/* Linux: X11/Wayland has no per-window "exclude from capture" flag, so
 * affinity is reported as N/A. We still enumerate managed top-level
 * windows via _NET_CLIENT_LIST for parity with the other platforms. */
static void enumerate(void) {
    Display *d = XOpenDisplay(NULL);
    if (!d) { fprintf(stderr, "cannot open X display (Wayland? run under Xorg or set DISPLAY)\n"); return; }

    Window root = DefaultRootWindow(d);
    Atom net_client = XInternAtom(d, "_NET_CLIENT_LIST", False);
    Atom net_name   = XInternAtom(d, "_NET_WM_NAME", False);
    Atom utf8       = XInternAtom(d, "UTF8_STRING", False);
    Atom net_pid    = XInternAtom(d, "_NET_WM_PID", False);

    unsigned long n = 0;
    Window *wins = (Window *)get_prop(d, root, net_client, XA_WINDOW, &n);
    if (!wins) { fprintf(stderr, "WM does not expose _NET_CLIENT_LIST\n"); XCloseDisplay(d); return; }

    for (unsigned long i = 0; i < n; ++i) {
        Window w = wins[i];
        char title[512]; title[0] = 0; int has_title = 0;
        unsigned long tn = 0;
        unsigned char *t = get_prop(d, w, net_name, utf8, &tn);
        if (t) { strncpy(title, (char *)t, sizeof title - 1); has_title = title[0] != 0; XFree(t); }
        if (!has_title) {
            char *wn = NULL;
            if (XFetchName(d, w, &wn) && wn) { strncpy(title, wn, sizeof title - 1); has_title = title[0] != 0; XFree(wn); }
        }
        long pid = 0; unsigned long pn = 0;
        unsigned char *p = get_prop(d, w, net_pid, XA_CARDINAL, &pn);
        if (p) { pid = (long)*(unsigned long *)p; XFree(p); }
        char proc[256]; proc_name(pid, proc, sizeof proc);
        report(0, "N/A", (unsigned long)pid, proc, title, has_title);
    }
    XFree(wins);
    XCloseDisplay(d);
}

/* ======================== unsupported =========================== */
#else
static void enumerate(void) { fprintf(stderr, "capscan: unsupported platform\n"); }
#endif

/* ============================ main ============================== */
int main(int argc, char **argv) {
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--protected-only") || !strcmp(argv[i], "-p"))
            g_protected_only = 1;
        else if (!strcmp(argv[i], "--all") || !strcmp(argv[i], "-a"))
            g_include_untitled = 1;
        else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            printf("capscan - screen capture protection scanner\n\n"
                   "usage: capscan [options]\n"
                   "  -p, --protected-only  show only capture-protected windows\n"
                   "  -a, --all             include windows without a title\n"
                   "  -h, --help            this help\n");
            return 0;
        } else {
            fprintf(stderr, "unknown option: %s (try --help)\n", argv[i]);
            return 2;
        }
    }

    printf("\n  capscan - screen capture protection scan\n");
    printf("  ====================================================================\n");
    printf("  %-10s %-9s %-7s ", "STATUS", "AFFINITY", "PID");
    print_field("PROCESS", 24);
    printf(" %s\n", "TITLE");
    printf("  --------------------------------------------------------------------\n");
    enumerate();
    printf("  ====================================================================\n");
    if (g_protected > 0)
        printf("  RESULT: %d window(s) HIDDEN from screenshots / screen recording.\n"
               "          Rows marked [DETECTED] will NOT show up in a capture.\n",
               g_protected);
    else
        printf("  RESULT: nothing hidden - all %d window(s) are capturable.\n", g_count);
    return 0;
}



