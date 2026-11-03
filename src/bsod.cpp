// language: C++, file: src/bsod.cpp — красный BSOD, корректное восстановление GDI-объектов
#include "bsod.h"
#include <windows.h>

static LRESULT CALLBACK BsodProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_ERASEBKGND) return 1;
    if (m == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(h, &ps);
        RECT rc; GetClientRect(h, &rc);
        int W = rc.right, H = rc.bottom;

        HBRUSH red = CreateSolidBrush(RGB(160, 0, 0));
        FillRect(dc, &rc, red);
        DeleteObject(red);

        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(255,255,255));

        HFONT fBig = CreateFontW(64, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
                                 0, 0, 0, 0, L"Segoe UI");
        HFONT fMed = CreateFontW(28, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
                                 0, 0, 0, 0, L"Segoe UI");

        HFONT old1 = (HFONT)SelectObject(dc, fBig);
        RECT t1 = {80, H/2 - 200, W-80, H/2 - 60};
        DrawTextW(dc, L":(", -1, &t1, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);

        HFONT old2 = (HFONT)SelectObject(dc, fMed);
        RECT t2 = {80, H/2 - 100, W-80, H/2 - 40};
        DrawTextW(dc, L"Your PC ran into a problem and needs to restart.",
                  -1, &t2, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
        RECT t3 = {80, H/2 - 50, W-80, H/2};
        DrawTextW(dc, L"Stop code: LABVM_CRITICAL_FAILURE",
                  -1, &t3, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);

        SetTextColor(dc, RGB(255,220,220));
        RECT t4 = {80, H/2 + 60, W-80, H/2 + 120};
        DrawTextW(dc, L"Recovery partition not found.",
                  -1, &t4, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);
        RECT t5 = {80, H/2 + 120, W-80, H/2 + 180};
        DrawTextW(dc, L"Safe Mode unavailable. Automatic Repair failed.",
                  -1, &t5, DT_LEFT | DT_SINGLELINE | DT_NOPREFIX);

        SelectObject(dc, old2);
        SelectObject(dc, old1);
        DeleteObject(fMed);
        DeleteObject(fBig);
        EndPaint(h, &ps);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

void ShowRedBSOD() {
    HINSTANCE hInst = GetModuleHandleW(nullptr);
    WNDCLASSW wc = {};
    wc.lpfnWndProc   = BsodProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = L"LabBsodWnd";
    if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return;

    int W = GetSystemMetrics(SM_CXSCREEN);
    int H = GetSystemMetrics(SM_CYSCREEN);
    HWND h = CreateWindowExW(WS_EX_TOPMOST, L"LabBsodWnd", L"",
        WS_POPUP | WS_VISIBLE, 0, 0, W, H, nullptr, nullptr, hInst, nullptr);
    if (!h) return;
    SetWindowPos(h, HWND_TOPMOST, 0, 0, W, H, SWP_SHOWWINDOW);
    UpdateWindow(h);
}
