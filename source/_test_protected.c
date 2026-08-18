/* test helper: creates a capture-excluded window so capscan can flag it.
 * build: gcc _test_protected.c -o _test_protected.exe -O2
 * run  : _test_protected.exe   (self-closes after ~6s)
 */
#include <windows.h>

int main(void) {
    HWND w = CreateWindowExW(0, L"STATIC", L"CAPSCAN_TEST_PROTECTED",
                             WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                             100, 100, 300, 120, NULL, NULL, NULL, NULL);
    if (!w) return 1;
    SetWindowDisplayAffinity(w, WDA_EXCLUDEFROMCAPTURE);
    for (int i = 0; i < 120; ++i) {   /* pump messages ~6s */
        MSG m;
        while (PeekMessage(&m, NULL, 0, 0, PM_REMOVE)) DispatchMessage(&m);
        Sleep(50);
    }
    return 0;
}
