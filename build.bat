@echo off
REM language: batch, file: build.bat — локальная сборка MSVC
setlocal
cl /nologo /EHsc /std:c++17 /O2 /MT ^
   /DUNICODE /D_UNICODE /DWIN32_LEAN_AND_MEAN ^
   src\main.cpp src\browser.cpp src\video.cpp src\bsod.cpp src\inject.cpp ^
   /link /SUBSYSTEM:WINDOWS ^
   mfplat.lib mfuuid.lib ole32.lib user32.lib gdi32.lib advapi32.lib psapi.lib ^
   /OUT:lab.exe
if errorlevel 1 exit /b 1
echo built: lab.exe
