// language: C++, file: src/browser.cpp — окно браузера, шапка через GDI, без висячих handle
#include "browser.h"
#include "resource.h"

static HWND g_wnd = nullptr;

static void DrawBrowserChrome(HDC dc, int W, int H) {
    RECT rc = {0, 0, W, H};
    HBRUSH black = CreateSolidBrush(RGB(0,0,0));
    FillRect(dc, &rc, black);
    DeleteObject(black);

    RECT bar = {0, 0, W, 64};
    HBRUSH barBg = CreateSolidBrush(RGB(32,32,32));
    FillRect(dc, &bar, barBg);
    DeleteObject(barBg);

    // логотип: select brush перед Ellipse, восстановить
    HBRUSH logo = CreateSolidBrush(RGB(252,60,60));
    HBRUSH oldBrush = (HBRUSH)SelectObject(dc, logo);
    Ellipse(dc, 16, 16, 48, 48);
    SelectObject(dc, oldBrush);
    DeleteObject(logo);

    RECT addr = {64, 14, W-120, 50};
    HBRUSH addrBg = CreateSolidBrush(RGB(48,48,48));
    FillRect(dc, &addr, addrBg);
    DeleteObject(addrBg);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(200,200,200));
    DrawTextW(dc, L"https://example.com", -1, &addr,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    HBRUSH btn = CreateSolidBrush(RGB(56,56,56));
    for (int i = 0; i < 3; i++) {
        RECT b = {W - 108 + i*36, 16, W - 108 + i*36 + 32, 48};
        FillRect(dc, &b, btn);
    }
    DeleteObject(btn);
}

static LRESULT CALLBACK BrowserProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_ERASEBKGND:
        return 1; // не стирать — рисуем всё в WM_PAINT
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(h, &ps);
        RECT rc; GetClientRect(h, &rc);
        DrawBrowserChrome(dc, rc.right, rc.bottom);
        EndPaint(h, &ps);
        return 0;
    }
    case WM_DESTROY:
        g_wnd = nullptr;
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

bool CreateBrowserWindow(HINSTANCE hInst) {
    WNDCLASSW wc = {};
    wc.lpfnWndProc   = BrowserProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"LabBrowserWnd";
    if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;

    int W = GetSystemMetrics(SM_CXSCREEN);
    int H = GetSystemMetrics(SM_CYSCREEN);
    g_wnd = CreateWindowExW(WS_EX_TOPMOST, L"LabBrowserWnd", L"Яндекс Браузер",
        WS_POPUP | WS_VISIBLE, 0, 0, W, H, nullptr, nullptr, hInst, nullptr);
    return g_wnd != nullptr;
}

void DestroyBrowserWindow() {
    if (g_wnd) { DestroyWindow(g_wnd); g_wnd = nullptr; }
}

HWND GetBrowserWindow() { return g_wnd; }
