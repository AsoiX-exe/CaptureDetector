// capscan - finds windows that are hidden from screen capture
// (screenshots / screen recording), e.g. DRM overlays or sneaky malware.
// coded by AsoiX

#include <stdio.h>
#include <string.h>

#define C_RESET "\x1b[0m"
#define C_BOLD  "\x1b[1m"
#define C_DIM   "\x1b[90m"
#define C_RED   "\x1b[91m"
#define C_GRN   "\x1b[92m"
#define C_YEL   "\x1b[93m"
#define C_CYA   "\x1b[96m"

typedef struct {
    int prot;
    unsigned long pid;
    char aff[16];
    char proc[160];
    char title[512];
} win_t;

static win_t rows[4096];
static int nrows = 0;
static int nprot = 0;
static int only_protected = 0;
static int include_untitled = 0;

static void add_win(int prot, const char *aff, unsigned long pid,
                    const char *proc, const char *title, int has_title) {
    if (only_protected && !prot) return;
    if (!has_title && !include_untitled && !prot) return;
    if (nrows >= 4096) return;

    win_t *w = &rows[nrows++];
    w->prot = prot;
    w->pid = pid;
    snprintf(w->aff, sizeof w->aff, "%s", aff);
    snprintf(w->proc, sizeof w->proc, "%s", (proc && proc[0]) ? proc : "?");
    snprintf(w->title, sizeof w->title, "%s", has_title ? title : "<no title>");
    if (prot) nprot++;
}

// pad a utf-8 string to `width` visible columns (counts code points, not bytes)
static void field(const char *s, int width) {
    int total = 0;
    for (const char *q = s; *q; ++q)
        if ((*q & 0xC0) != 0x80) total++;

    if (total <= width) {
        fputs(s, stdout);
        for (int i = total; i < width; ++i) putchar(' ');
    } else {
        int lim = width - 2, c = 0;
        for (const char *p = s; *p; ++p) {
            if ((*p & 0xC0) != 0x80) { if (c >= lim) break; c++; }
            putchar((unsigned char)*p);
        }
        fputs("..", stdout);
    }
}

static void enumerate(void);

#if defined(_WIN32)
#include <windows.h>

#ifndef WDA_MONITOR
#define WDA_MONITOR 0x00000001
#endif
#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif

static void to_utf8(const wchar_t *in, char *out, int cap) {
    if (WideCharToMultiByte(CP_UTF8, 0, in, -1, out, cap, NULL, NULL) == 0)
        out[0] = 0;
    out[cap - 1] = 0;
}

static void proc_name(DWORD pid, char *out, int cap) {
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

    DWORD aff = 0;
    GetWindowDisplayAffinity(hwnd, &aff);
    const char *name = aff == 0 ? "NONE"
                     : aff == WDA_MONITOR ? "MONITOR"
                     : aff == WDA_EXCLUDEFROMCAPTURE ? "EXCLUDE" : "UNKNOWN";

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    char exe[320];
    proc_name(pid, exe, sizeof exe);

    add_win(aff != 0, name, (unsigned long)pid, exe, title, len > 0);
    return TRUE;
}

static void enumerate(void) { EnumWindows(on_window, 0); }

static void enable_console(void) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD m = 0;
    if (GetConsoleMode(h, &m))
        SetConsoleMode(h, m | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    SetConsoleOutputCP(CP_UTF8);
}

// true when double-clicked from explorer (we own the console alone)
static int own_console(void) {
    DWORD pids[2];
    return GetConsoleProcessList(pids, 2) <= 1;
}

#elif defined(__APPLE__)
#include <CoreGraphics/CoreGraphics.h>
#include <CoreFoundation/CoreFoundation.h>

static void cfstr(CFStringRef s, char *out, int cap) {
    out[0] = 0;
    if (s) CFStringGetCString(s, out, cap, kCFStringEncodingUTF8);
}

static long dict_int(CFDictionaryRef d, CFStringRef key) {
    long v = 0;
    CFNumberRef n = (CFNumberRef)CFDictionaryGetValue(d, key);
    if (n) CFNumberGetValue(n, kCFNumberLongType, &v);
    return v;
}

// on mac, sharing state 0 (none) means the window is kept out of captures
static void enumerate(void) {
    CFArrayRef list = CGWindowListCopyWindowInfo(
        kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements,
        kCGNullWindowID);
    if (!list) return;
    for (CFIndex i = 0, n = CFArrayGetCount(list); i < n; ++i) {
        CFDictionaryRef w = (CFDictionaryRef)CFArrayGetValueAtIndex(list, i);
        long sharing = dict_int(w, kCGWindowSharingState);
        long pid = dict_int(w, kCGWindowOwnerPID);
        char proc[160], title[512];
        cfstr((CFStringRef)CFDictionaryGetValue(w, kCGWindowOwnerName), proc, sizeof proc);
        title[0] = 0;
        CFStringRef nm = (CFStringRef)CFDictionaryGetValue(w, kCGWindowName);
        int has_title = nm && CFStringGetLength(nm) > 0;
        if (has_title) cfstr(nm, title, sizeof title);
        const char *aff = sharing == 0 ? "NO-SHARE" : sharing == 1 ? "READONLY" : "READWRITE";
        add_win(sharing == 0, aff, (unsigned long)pid, proc, title, has_title);
    }
    CFRelease(list);
}

static void enable_console(void) {}
static int own_console(void) { return 0; }

#elif defined(__linux__)
#include <X11/Xlib.h>
#include <X11/Xatom.h>

static unsigned char *xprop(Display *d, Window w, Atom prop, Atom type, unsigned long *n) {
    Atom rt; int rf; unsigned long rb; unsigned char *data = NULL;
    if (XGetWindowProperty(d, w, prop, 0, (~0L), False, type, &rt, &rf, n, &rb, &data) != Success)
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

// x11 has no per-window capture flag, so affinity is always N/A here
static void enumerate(void) {
    Display *d = XOpenDisplay(NULL);
    if (!d) { fprintf(stderr, "no X display\n"); return; }
    Window root = DefaultRootWindow(d);
    Atom cl = XInternAtom(d, "_NET_CLIENT_LIST", False);
    Atom nm = XInternAtom(d, "_NET_WM_NAME", False);
    Atom u8 = XInternAtom(d, "UTF8_STRING", False);
    Atom pa = XInternAtom(d, "_NET_WM_PID", False);
    unsigned long n = 0;
    Window *wins = (Window *)xprop(d, root, cl, XA_WINDOW, &n);
    if (!wins) { XCloseDisplay(d); return; }
    for (unsigned long i = 0; i < n; ++i) {
        char title[512]; title[0] = 0; int has_title = 0;
        unsigned long tn = 0;
        unsigned char *t = xprop(d, wins[i], nm, u8, &tn);
        if (t) { snprintf(title, sizeof title, "%s", (char *)t); has_title = title[0] != 0; XFree(t); }
        long pid = 0; unsigned long pn = 0;
        unsigned char *p = xprop(d, wins[i], pa, XA_CARDINAL, &pn);
        if (p) { pid = (long)*(unsigned long *)p; XFree(p); }
        char proc[160]; proc_name(pid, proc, sizeof proc);
        add_win(0, "N/A", (unsigned long)pid, proc, title, has_title);
    }
    XFree(wins);
    XCloseDisplay(d);
}

static void enable_console(void) {}
static int own_console(void) { return 0; }

#else
static void enumerate(void) { fprintf(stderr, "unsupported platform\n"); }
static void enable_console(void) {}
static int own_console(void) { return 0; }
#endif

#define C_ONRED "\x1b[41m\x1b[97m\x1b[1m"   // bold white on red

static void print_row(const win_t *w, int prot) {
    if (prot) {
        printf("   " C_ONRED " HIDDEN " C_RESET "  " C_RED C_BOLD);
        field(w->proc, 22);
        printf(C_RESET "  " C_YEL "%-9s" C_RESET "  " C_DIM "pid %-6lu" C_RESET "  %s\n",
               w->aff, w->pid, w->title);
    } else {
        printf("             " C_DIM);
        field(w->proc, 22);
        printf("  %-9s  pid %-6lu  %s" C_RESET "\n", w->aff, w->pid, w->title);
    }
}

int main(int argc, char **argv) {
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "--protected-only"))
            only_protected = 1;
        else if (!strcmp(argv[i], "-a") || !strcmp(argv[i], "--all"))
            include_untitled = 1;
        else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            printf("capscan [-p only-protected] [-a include-untitled]\n");
            return 0;
        }
    }

    enable_console();
    enumerate();

    printf("\n  " C_CYA C_BOLD "capscan" C_RESET C_CYA
           "  screen capture protection scanner" C_RESET "\n");
    printf("  " C_DIM "coded by AsoiX" C_RESET "\n\n");

    if (nprot > 0) {
        printf("  " C_RED C_BOLD "!!  %d window(s) HIDDEN from screen capture  !!"
               C_RESET "\n", nprot);
        printf("  " C_DIM "won't show up in screenshots or screen recording" C_RESET "\n\n");
        for (int i = 0; i < nrows; ++i)
            if (rows[i].prot) print_row(&rows[i], 1);
        printf("\n");
    } else {
        printf("  " C_GRN C_BOLD "OK  nothing is hidden - every window is capturable"
               C_RESET "\n\n");
    }

    if (!only_protected) {
        printf("  " C_DIM "other visible windows" C_RESET "\n");
        for (int i = 0; i < nrows; ++i)
            if (!rows[i].prot) print_row(&rows[i], 0);
    }

    printf("\n  " C_DIM "%d scanned" C_RESET "   ", nrows);
    if (nprot > 0) printf(C_RED C_BOLD "%d hidden" C_RESET "\n", nprot);
    else printf(C_GRN "0 hidden" C_RESET "\n");

    if (own_console()) {
        printf("\n  " C_DIM "press Enter to exit..." C_RESET);
        fflush(stdout);
        getchar();
    }
    return 0;
}

