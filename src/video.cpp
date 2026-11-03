// language: C++, file: src/video.cpp — MF Media Engine: HWND-рендер, конец по ENDED, без утечек
#include "video.h"
#include "browser.h"
#include <mfapi.h>
#include <mfmediaengine.h>
#include <mfobjects.h>
#include <wrl/client.h>
#include <atomic>
#include <cstdio>

using Microsoft::WRL::ComPtr;

static ComPtr<IMFMediaEngine>             g_engine;
static ComPtr<IMFMediaEngineClassFactory> g_factory;
static ComPtr<IMFAttributes>              g_attrs;
static ComPtr<IMFMediaEngineNotify>       g_notify;
static HWND                g_videoWnd  = nullptr;
static HWND                g_parentWnd = nullptr;
static std::atomic<bool>   g_ended{false};
static bool                g_mfUp = false;

static void log_hr(const wchar_t* what, HRESULT hr) {
    wchar_t buf[256];
    swprintf_s(buf, L"[lab-video] %s failed: 0x%08X\n", what, (unsigned)hr);
    OutputDebugStringW(buf);
}

class EngineNotify : public IMFMediaEngineNotify {
    std::atomic<ULONG> ref{1};
public:
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) return E_POINTER;
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IMFMediaEngineNotify)) {
            *ppv = static_cast<IMFMediaEngineNotify*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return ++ref; }
    STDMETHODIMP_(ULONG) Release() override {
        ULONG r = --ref;
        if (!r) delete this;
        return r;
    }
    STDMETHODIMP EventNotify(DWORD event, DWORD_PTR, DWORD) override {
        if (event == MF_MEDIA_ENGINE_EVENT_ENDED) g_ended = true;
        return S_OK;
    }
};

static LRESULT CALLBACK VideoProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_ERASEBKGND) return 1;
    if (m == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(h, &ps);
        RECT rc; GetClientRect(h, &rc);
        HBRUSH b = CreateSolidBrush(RGB(0,0,0));
        FillRect(dc, &rc, b);
        DeleteObject(b);
        EndPaint(h, &ps);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

bool StartVideoPlayer(const wchar_t* path) {
    if (!g_mfUp) {
        HRESULT hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
        if (FAILED(hr)) { log_hr(L"MFStartup", hr); return false; }
        g_mfUp = true;
    }

    HINSTANCE hInst = GetModuleHandleW(nullptr);
    WNDCLASSW wc = {};
    wc.lpfnWndProc   = VideoProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = L"LabVideoWnd";
    if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;

    g_parentWnd = GetBrowserWindow();
    if (!g_parentWnd) { OutputDebugStringW(L"[lab-video] no browser window\n"); return false; }

    int W = GetSystemMetrics(SM_CXSCREEN);
    int H = GetSystemMetrics(SM_CYSCREEN);
    g_videoWnd = CreateWindowExW(0, L"LabVideoWnd", L"",
        WS_CHILD | WS_VISIBLE, 0, 64, W, H - 64,
        g_parentWnd, nullptr, hInst, nullptr);
    if (!g_videoWnd) { OutputDebugStringW(L"[lab-video] CreateWindow failed\n"); return false; }

    HRESULT hr = CoCreateInstance(CLSID_MFMediaEngineClassFactory, nullptr, CLSCTX_ALL,
                                  IID_PPV_ARGS(g_factory.GetAddressOf()));
    if (FAILED(hr)) { log_hr(L"CoCreateInstance", hr); return false; }

    hr = MFCreateAttributes(g_attrs.GetAddressOf(), 2);
    if (FAILED(hr)) { log_hr(L"MFCreateAttributes", hr); return false; }

    hr = g_attrs->SetUINT64(MF_MEDIA_ENGINE_PLAYBACK_HWND, (UINT64)(uintptr_t)g_videoWnd);
    if (FAILED(hr)) { log_hr(L"SetUINT64(HWND)", hr); return false; }

    // колбэк: ComPtr забирает владение, SetUnknown делает AddRef — баланс без утечки
    g_notify = new EngineNotify();
    hr = g_attrs->SetUnknown(MF_MEDIA_ENGINE_CALLBACK, g_notify.Get());
    if (FAILED(hr)) { log_hr(L"SetUnknown(callback)", hr); return false; }

    hr = g_factory->CreateInstance(0, g_attrs.Get(), g_engine.GetAddressOf());
    if (FAILED(hr)) { log_hr(L"CreateInstance", hr); return false; }

    BSTR bstr = SysAllocString(path);
    if (!bstr) return false;
    hr = g_engine->SetSource(bstr);
    SysFreeString(bstr);
    if (FAILED(hr)) { log_hr(L"SetSource", hr); return false; }

    g_engine->SetLoop(FALSE);
    hr = g_engine->Play();
    if (FAILED(hr)) { log_hr(L"Play", hr); return false; }
    return true;
}

void StopVideoPlayer() {
    if (g_engine) {
        g_engine->Pause();
        g_engine->Shutdown();
        g_engine.Reset();
    }
    g_notify.Reset();
    g_attrs.Reset();
    g_factory.Reset();
    if (g_videoWnd) { DestroyWindow(g_videoWnd); g_videoWnd = nullptr; }
}

bool VideoEnded() { return g_ended.load(); }
