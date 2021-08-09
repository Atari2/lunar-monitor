// lunar-monitor-prototype-window.cpp : Defines the entry point for the application.
//

#define DIRECTORY_TO_WATCH ".\\"
#define ROM_NAME_TO_WATCH "c.smc"
#define LEVEL_DIRECTORY ".\\levels\\"
#define LUNAR_MAGIC_PATH ".\\lunar_magic.exe"
#define MWL_FILE_PREFIX "level "

#define DEBUG

#include "framework.h"

#include <windows.h>
#include <shellapi.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <strsafe.h>

#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <sstream>

#ifndef WINVER                // Allow use of features specific to Windows XP or later.
#define WINVER 0x0501        // Change this to the appropriate value to target other versions of Windows.
#endif
#include <vector>

#ifndef _WIN32_WINNT        // Allow use of features specific to Windows XP or later.                  
#define _WIN32_WINNT 0x0501    // Change this to the appropriate value to target other versions of Windows.
#endif                       

#ifndef _WIN32_WINDOWS        // Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#define WIN32_LEAN_AND_MEAN        // Exclude rarely-used stuff from Windows headers

#define SZ_WND_CLASS L"MONITOR"

#define FULL_ROM_PATH DIRECTORY_TO_WATCH ROM_NAME_TO_WATCH

unsigned int lvlNum{ 0x105 };
std::time_t previousLastModifiedTime = NULL;

LPCWSTR szWndClass = SZ_WND_CLASS;

#ifdef DEBUG
bool RedirectConsoleIO();
bool CreateNewConsole(int16_t minLength);
void AdjustConsoleBuffer(int16_t minLength);
#endif

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

std::time_t getLastModifiedTime(const std::string romPath);
bool romWasModified(const std::string romPath, std::time_t& previousLastChangedTime);
void handlePotentialROMChange(const std::string romPath, std::time_t& previousLastChangedTime);

int APIENTRY _tWinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR    lpCmdLine,
    int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

#ifdef DEBUG
    CreateNewConsole(500);
#endif

    MSG msg;

    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = NULL;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = NULL;
    wcex.hCursor = NULL;
    wcex.hbrBackground = NULL;
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWndClass;
    wcex.hIconSm = NULL;
    RegisterClassEx(&wcex);

    auto window = CreateWindowExW(0, szWndClass, szWndClass, 0, 0, 0, 0, 0, 0, 0, hInstance, 0);

    if (!window)
    {
        return 1;
    }

    ShowWindow(window, SW_HIDE);

    HANDLE romChangeHandle = FindFirstChangeNotification(
        TEXT(DIRECTORY_TO_WATCH), false, FILE_NOTIFY_CHANGE_LAST_WRITE
    );

    previousLastModifiedTime = getLastModifiedTime(FULL_ROM_PATH);

    while (true)
    {
        DWORD result = WaitForSingleObject(romChangeHandle, 20);

        if (result == WAIT_OBJECT_0) 
        {
#ifdef DEBUG
            std::cout << "Change in ROM dir detected" << std::endl;
#endif
            handlePotentialROMChange(FULL_ROM_PATH, previousLastModifiedTime);
        }

        FindNextChangeNotification(romChangeHandle);

        BOOL msgAvailable = PeekMessage(&msg, NULL, 0xBECA, 0xBECA, PM_REMOVE);
        if (msgAvailable)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        break;
    case WM_COMMAND:
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        if (lParam >> 16 == 0x6942)
        {
            unsigned int newLvlNum = ((lParam >> 6) & 0x3FF);
#ifdef DEBUG
            std::cout << "LM now in level " << std::hex << std::uppercase << newLvlNum << std::endl;
#endif
            lvlNum = newLvlNum;
        }
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

bool exportCurrLvl()
{
#ifdef DEBUG
    std::cout << "Attempting to export level " << std::hex << std::uppercase << lvlNum << std::endl;
#endif

    std::wstringstream ws;

    ws << " -ExportLevel \"" << FULL_ROM_PATH << "\" \"" 
        << LEVEL_DIRECTORY MWL_FILE_PREFIX << std::hex << std::uppercase << lvlNum << std::nouppercase << ".mwl\" " << lvlNum;

#ifdef DEBUG
    std::wcout << "Command:\n" << LUNAR_MAGIC_PATH << " " << ws.str() << std::endl;
#endif

    std::wstring command = ws.str();
    std::vector<wchar_t> buf(command.begin(), command.end());
    buf.push_back(0);

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcess(TEXT(LUNAR_MAGIC_PATH), buf.data(), NULL, NULL, false, 0, NULL, NULL, &si, &pi))
    {
#ifdef DEBUG
        std::cerr << "Creating level export process failed: " << GetLastError() << std::endl;
#endif // DEBUG
        return false;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode;

    GetExitCodeProcess(pi.hProcess, &exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

#ifdef DEBUG
    if (exitCode == 0)
    {
        std::cout << "Successfully exported level " << std::hex << lvlNum << " as "
            << LEVEL_DIRECTORY MWL_FILE_PREFIX << std::hex << std::uppercase << lvlNum << std::nouppercase << ".mwl" << std::endl;
        return true;
    }
    else
    {

        std::cerr << "Level export failed!" << std::endl;
        return false;
    }
#else
    return exitCode;
#endif
}

std::time_t getLastModifiedTime(const std::string romPath)
{
    struct _stat result;
    _stat(romPath.c_str(), &result);

    return result.st_mtime;
}

bool romWasModified(const std::string romPath, std::time_t& previousLastChangedTime)
{
    std::time_t newLastChangedTime = getLastModifiedTime(romPath);

    if (newLastChangedTime == previousLastChangedTime)
        return false;

#ifdef DEBUG
    std::cout << "Last modified time has changed from " << previousLastChangedTime << " to " << newLastChangedTime << std::endl;
#endif

    previousLastChangedTime = newLastChangedTime;
    return true;
}

void handlePotentialROMChange(const std::string romPath, std::time_t& previousLastChangedTime)
{
    if (!romWasModified(romPath, previousLastChangedTime))
    {
#ifdef DEBUG
        std::cout << "Change in directory was not on ROM" << std::endl;
#endif
        return;
    }

#ifdef DEBUG
    std::cout << "ROM modification detected" << std::endl;
#endif

    HWND foregroundWindowHandle = GetForegroundWindow();
    std::wstring windowTitle(GetWindowTextLength(foregroundWindowHandle) + 1, L'\0');
    GetWindowText(foregroundWindowHandle, &windowTitle[0], windowTitle.size());

#ifdef DEBUG
    std::wcout << "Foreground window title on ROM change was: " << windowTitle << std::endl;
#endif

    if (windowTitle.find(L"Lunar Magic") != std::string::npos)
    {
#ifdef DEBUG
        std::cout << "Foreground window was main level editor, user likely just saved a level, attempting to export it now" << std::endl;
#endif
        exportCurrLvl();
    }
    else
    {
#ifdef DEBUG
        std::wcout << "There is currently no action defined when the foreground window during a ROM write is " << windowTitle << ", ignoring write" << std::endl;
#endif
    }
}

#ifdef DEBUG
void AdjustConsoleBuffer(int16_t minLength)
{
    // Set the screen buffer to be big enough to scroll some text
    CONSOLE_SCREEN_BUFFER_INFO conInfo;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &conInfo);
    if (conInfo.dwSize.Y < minLength)
        conInfo.dwSize.Y = minLength;
    SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), conInfo.dwSize);
}

bool CreateNewConsole(int16_t minLength)
{
    bool result = false;

    // Attempt to create new console
    if (AllocConsole())
    {
        AdjustConsoleBuffer(minLength);
        result = RedirectConsoleIO();
    }

    return result;
}

bool RedirectConsoleIO()
{
    bool result = true;
    FILE* fp;

    // Redirect STDIN if the console has an input handle
    if (GetStdHandle(STD_INPUT_HANDLE) != INVALID_HANDLE_VALUE)
        if (freopen_s(&fp, "CONIN$", "r", stdin) != 0)
            result = false;
        else
            setvbuf(stdin, NULL, _IONBF, 0);

    // Redirect STDOUT if the console has an output handle
    if (GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE)
        if (freopen_s(&fp, "CONOUT$", "w", stdout) != 0)
            result = false;
        else
            setvbuf(stdout, NULL, _IONBF, 0);

    // Redirect STDERR if the console has an error handle
    if (GetStdHandle(STD_ERROR_HANDLE) != INVALID_HANDLE_VALUE)
        if (freopen_s(&fp, "CONOUT$", "w", stderr) != 0)
            result = false;
        else
            setvbuf(stderr, NULL, _IONBF, 0);

    // Make C++ standard streams point to console as well.
    std::ios::sync_with_stdio(true);

    // Clear the error state for each of the C++ standard streams.
    std::wcout.clear();
    std::cout.clear();
    std::wcerr.clear();
    std::cerr.clear();
    std::wcin.clear();
    std::cin.clear();

    return result;
}
#endif
