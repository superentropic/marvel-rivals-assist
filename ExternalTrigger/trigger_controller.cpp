#include <Windows.h>
#include "../Source/TriggerControlShared.h"

#pragma comment(lib, "winmm.lib")

namespace {
    constexpr int kToggleId = 1001;
    constexpr UINT kAhkTriggerMessage = WM_APP + 0x219;
    constexpr wchar_t kAhkWindowClass[] = L"AutoHotkeyGUI";
    constexpr wchar_t kAhkWindowTitle[] = L"Marvel Trigger AHK";
    HWND gToggle = nullptr;
    HWND gStatus = nullptr;
    HANDLE gMapping = nullptr;
    TriggerControlShared::State* gState = nullptr;
    LONG gLastStatus = -1;
    HANDLE gWorker = nullptr;
    volatile LONG gRunning = 1;

    bool RequestAhkClick() {
        const HWND ahkWindow = FindWindowW(kAhkWindowClass, kAhkWindowTitle);
        return ahkWindow && PostMessageW(ahkWindow, kAhkTriggerMessage, 0, 0) != FALSE;
    }

    DWORD WINAPI RequestWorker(void*) {
        LONG acknowledged = InterlockedCompareExchange(&gState->clickAcknowledged, 0, 0);

        while (InterlockedCompareExchange(&gRunning, 0, 0) != 0) {
            const LONG requested = InterlockedCompareExchange(&gState->clickRequest, 0, 0);
            if (requested == acknowledged) {
                Sleep(1);
                continue;
            }

            if (!RequestAhkClick()) {
                // Do not acknowledge a failed dispatch. Retry without losing
                // the request. A missing AHK bridge does not need a hot loop.
                InterlockedExchange(&gState->status, 16);
                Sleep(25);
                continue;
            }

            ++acknowledged;
            InterlockedExchange(&gState->clickAcknowledged, acknowledged);
            InterlockedExchange(&gState->status, 15);
        }

        return 0;
    }

    const wchar_t* StatusText(LONG status) {
        switch (status) {
        case 0:  return L"Status: waiting for DLL";
        case 1:  return L"Status: triggerbot off";
        case 2:  return L"Status: engine unavailable";
        case 3:  return L"Status: viewport unavailable";
        case 4:  return L"Status: world unavailable";
        case 5:  return L"Status: controller unavailable";
        case 6:  return L"Status: camera or pawn unavailable";
        case 7:  return L"Status: character class unavailable";
        case 8:  return L"Status: trace missed";
        case 9:  return L"Status: trace hit non-character";
        case 10: return L"Status: target rejected";
        case 11: return L"Status: legacy input sent";
        case 12: return L"Status: pawn unavailable";
        case 13: return L"Status: input rejected";
        case 14: return L"Status: click requested externally";
        case 15: return L"Status: AHK click dispatched";
        case 16: return L"Status: AHK bridge unavailable";
        default: return L"Status: unknown";
        }
    }

    void Refresh() {
        if (!gState) return;
        const bool enabled = InterlockedCompareExchange(&gState->triggerEnabled, 0, 0) != 0;
        const LONG status = InterlockedCompareExchange(&gState->status, 0, 0);
        SetWindowTextW(gToggle, enabled ? L"Triggerbot: ON" : L"Triggerbot: OFF");
        SetWindowTextW(gStatus, StatusText(status));

        gLastStatus = status;
    }

    LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
        if (message == WM_COMMAND && LOWORD(wParam) == kToggleId) {
            const LONG enabled = InterlockedCompareExchange(&gState->triggerEnabled, 0, 0);
            InterlockedExchange(&gState->triggerEnabled, enabled ? 0 : 1);
            Refresh();
            return 0;
        }
        if (message == WM_TIMER) {
            Refresh();
            return 0;
        }
        if (message == WM_DESTROY) {
            InterlockedExchange(&gRunning, 0);
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand) {
    timeBeginPeriod(1);
    gMapping = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
        sizeof(TriggerControlShared::State), TriggerControlShared::MappingName);
    if (!gMapping) {
        timeEndPeriod(1);
        return 1;
    }

    gState = static_cast<TriggerControlShared::State*>(MapViewOfFile(gMapping, FILE_MAP_ALL_ACCESS, 0, 0,
        sizeof(TriggerControlShared::State)));
    if (!gState) {
        CloseHandle(gMapping);
        timeEndPeriod(1);
        return 1;
    }

    if (InterlockedCompareExchange(&gState->magic, 0, 0) != TriggerControlShared::Magic) {
        InterlockedExchange(&gState->magic, TriggerControlShared::Magic);
        InterlockedExchange(&gState->triggerEnabled, 0);
        InterlockedExchange(&gState->status, 0);
        InterlockedExchange(&gState->clickRequest, 0);
        InterlockedExchange(&gState->clickAcknowledged, 0);
    }

    const wchar_t* className = L"MarvelExternalTriggerControl";
    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = className;
    RegisterClassW(&wc);

    HWND window = CreateWindowExW(WS_EX_TOPMOST, className, L"Marvel Trigger Controller",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, 80, 80, 320, 170,
        nullptr, nullptr, instance, nullptr);
    if (!window) {
        UnmapViewOfFile(gState);
        CloseHandle(gMapping);
        timeEndPeriod(1);
        return 1;
    }

    CreateWindowW(L"STATIC", L"Practice Range only", WS_CHILD | WS_VISIBLE,
        18, 18, 260, 20, window, nullptr, instance, nullptr);
    gToggle = CreateWindowW(L"BUTTON", L"Triggerbot: OFF", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        18, 48, 260, 30, window, reinterpret_cast<HMENU>(kToggleId), instance, nullptr);
    gStatus = CreateWindowW(L"STATIC", L"Status: waiting for DLL", WS_CHILD | WS_VISIBLE,
        18, 92, 270, 20, window, nullptr, instance, nullptr);
    SetTimer(window, 1, 10, nullptr);
    gWorker = CreateThread(nullptr, 0, RequestWorker, nullptr, 0, nullptr);
    Refresh();
    ShowWindow(window, showCommand);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (gWorker) {
        WaitForSingleObject(gWorker, INFINITE);
        CloseHandle(gWorker);
    }

    UnmapViewOfFile(gState);
    CloseHandle(gMapping);
    timeEndPeriod(1);
    return 0;
}
