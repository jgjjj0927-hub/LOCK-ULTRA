// language: C++, file: src/browser.h
#pragma once
#include <windows.h>

bool CreateBrowserWindow(HINSTANCE hInst);
void DestroyBrowserWindow();
HWND GetBrowserWindow();
