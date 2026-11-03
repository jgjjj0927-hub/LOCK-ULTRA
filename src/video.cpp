// language: C++, file: src/video.cpp — Media Foundation: рендер в окно, конец по ENDED, BSTR, SetUnknown
#include "video.h"
#include "browser.h"
#include <mfapi.h>
#include <mfmediaengine.h>
#include <mfobjects.h>
#include <wrl/client.h>
#include <atomic>
#include <string>

using Microsoft::WRL::ComPtr;

static ComPtr<IMFMediaEngine>             g_engine;
static ComPtr<IMFMediaEngineClassFactory> g_factory;
static ComPtr<IMFAttributes>              g_attrs;
static ComPtr<IMFMediaEngineNotify>       g_notify;
static HWND                g_videoWnd  = nullptr;
static HWND                g_parentWnd = nullptr;
static std::atomic<bool>   g_ended{false};
static bool                g_mfUp = false;

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
        // фон под видео
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
        if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_FULL))) return false;
        g_mfUp = true;
    }

    HINSTANCE hInst = GetModuleHandleW(nullptr);
    WNDCLASSW wc = {};
    wc.lpfnWndProc   = VideoProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = L"LabVideoWnd";
    if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;

    g_parentWnd = GetBrowserWindow();
    if (!g_parentWnd) return false;

    int W = GetSystemMetrics(SM_CXSCREEN);
    int H = GetSystemMetrics(SM_CYSCREEN);
    g_videoWnd = CreateWindowExW(0, L"LabVideoWnd", L"",
        WS_CHILD | WS_VISIBLE, 0, 64, W, H - 64,
        g_parentWnd, nullptr, hInst, nullptr);
    if (!g_videoWnd) return false;

    if (FAILED(CoCreateInstance(CLSID_MFMediaEngineClassFactory, nullptr, CLSCTX_ALL,
                                IID_PPV_ARGS(g_factory.GetAddressOf())))) return false;

    if (FAILED(MFCreateAttributes(g_attrs.GetAddressOf(), 3))) return false;

    // HWND для рендера — атрибут, не второй аргумент CreateInstance
    g_attrs->SetUINT32(MF_MEDIA_ENGINE_VIDEO_OUTPUT_FORMAT, MFVideoFormat_RGB32);
    g_attrs->SetUINT64(MF_MEDIA_ENGINE_PLAYBACK_HWND, (UINT64)(uintptr_t)g_videoWnd);
    // колбэк — как IUnknown*, не UINT32
    g_notify.Attach(new EngineNotify());
    g_attrs->SetUnknown(MF_MEDIA_ENGINE_CALLBACK, g_notify.Get());

    // флаги создания — 0
    if (FAILED(g_factory->CreateInstance(0, g_attrs.Get(), g_engine.GetAddressOf()))) return false;

    // SetSource ждёт BSTR
    BSTR bstr = SysAllocString(path);
    if (!bstr) return false;
    HRESULT hr = g_engine->SetSource(bstr);
    SysFreeString(bstr);
    if (FAILED(hr)) return false;

    g_engine->SetLoop(FALSE);
    g_engine->Play();
    return true;
}

void StopVideoPlayer() {
    if (g_engine) { g_engine->Pause(); g_engine.Reset(); }
    g_notify.Reset();
    g_attrs.Reset();
    g_factory.Reset();
    if (g_videoWnd) { DestroyWindow(g_videoWnd); g_videoWnd = nullptr; }
}

bool VideoEnded() { return g_ended.load(); }
