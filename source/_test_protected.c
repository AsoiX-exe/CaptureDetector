// little test window that hides itself from capture, to check the scanner catches it
// coded by AsoiX
#include <windows.h>

int main(void) {
    HWND w = CreateWindowExW(0, L"STATIC", L"hidden test window",
                             WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                             100, 100, 300, 120, NULL, NULL, NULL, NULL);
    if (!w) return 1;

    SetWindowDisplayAffinity(w, WDA_EXCLUDEFROMCAPTURE);

    // stay up for a few seconds then die on its own
    for (int i = 0; i < 120; ++i) {
        MSG m;
        while (PeekMessage(&m, NULL, 0, 0, PM_REMOVE)) DispatchMessage(&m);
        Sleep(50);
    }
    return 0;
}
