#pragma once
#include <windows.h>
#include <thread>
#include <atomic>
#include "../../ThirdParty/Cumhook/Cumhook.h"

// WDA_EXCLUDEFROMCAPTURE: Win10 2004+ (build 19041)
// Blocks: Windows Graphics Capture API, Desktop Duplication API, PrintWindow
#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif

namespace AntiScreenshot {

    static std::atomic<bool> g_running{ false };
    static std::atomic<bool> g_enabled{ false };
    static HWND              g_hwnd   = nullptr;
    static HANDLE            g_thread = nullptr;

    // ── GDI hook typedefs ─────────────────────────────────────────────
    typedef BOOL(WINAPI* tBitBlt)(HDC, int, int, int, int, HDC, int, int, DWORD);
    typedef BOOL(WINAPI* tStretchBlt)(HDC, int, int, int, int, HDC, int, int, int, int, DWORD);
    typedef BOOL(WINAPI* tPrintWindow)(HWND, HDC, UINT);

    static tBitBlt      oBitBlt      = nullptr;
    static tStretchBlt  oStretchBlt  = nullptr;
    static tPrintWindow oPrintWindow = nullptr;

    // Returns true when the source DC is the screen or desktop (i.e. a capture attempt)
    static inline bool IsScreenCaptureDC(HDC hdcSrc) {
        if (!hdcSrc) return false;
        HWND wnd = WindowFromDC(hdcSrc);
        return (wnd == nullptr || wnd == GetDesktopWindow());
    }

    // ── Hook: BitBlt ──────────────────────────────────────────────────
    BOOL WINAPI hkBitBlt(HDC hdcDest, int x, int y, int cx, int cy,
                         HDC hdcSrc, int x1, int y1, DWORD rop)
    {
        if (g_enabled && IsScreenCaptureDC(hdcSrc)) {
            // Fill destination with black instead of exposing screen content
            RECT rc = { x, y, x + cx, y + cy };
            FillRect(hdcDest, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
            return TRUE;
        }
        return oBitBlt(hdcDest, x, y, cx, cy, hdcSrc, x1, y1, rop);
    }

    // ── Hook: StretchBlt ──────────────────────────────────────────────
    BOOL WINAPI hkStretchBlt(HDC hdcDest, int xD, int yD, int wD, int hD,
                              HDC hdcSrc, int xS, int yS, int wS, int hS, DWORD rop)
    {
        if (g_enabled && IsScreenCaptureDC(hdcSrc)) {
            RECT rc = { xD, yD, xD + wD, yD + hD };
            FillRect(hdcDest, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
            return TRUE;
        }
        return oStretchBlt(hdcDest, xD, yD, wD, hD, hdcSrc, xS, yS, wS, hS, rop);
    }

    // ── Hook: PrintWindow ─────────────────────────────────────────────
    BOOL WINAPI hkPrintWindow(HWND hwnd, HDC hdcBlt, UINT nFlags) {
        if (g_enabled && (hwnd == g_hwnd || hwnd == GetDesktopWindow()))
            return FALSE;
        return oPrintWindow(hwnd, hdcBlt, nFlags);
    }

    // ── Watchdog: re-applies display affinity every 2 seconds ─────────
    static DWORD WINAPI WatchdogProc(LPVOID) {
        while (g_running) {
            if (g_hwnd && IsWindow(g_hwnd)) {
                if (g_enabled) {
                    if (!SetWindowDisplayAffinity(g_hwnd, WDA_EXCLUDEFROMCAPTURE))
                        SetWindowDisplayAffinity(g_hwnd, WDA_MONITOR);
                } else {
                    SetWindowDisplayAffinity(g_hwnd, WDA_NONE);
                }
            }
            Sleep(2000);
        }
        return 0;
    }

    // ── Enable / Disable at runtime ─────────────────────────────────
    static void Enable() {
        g_enabled = true;
        if (g_hwnd && IsWindow(g_hwnd)) {
            if (!SetWindowDisplayAffinity(g_hwnd, WDA_EXCLUDEFROMCAPTURE))
                SetWindowDisplayAffinity(g_hwnd, WDA_MONITOR);
        }
    }

    static void Disable() {
        g_enabled = false;
        if (g_hwnd && IsWindow(g_hwnd))
            SetWindowDisplayAffinity(g_hwnd, WDA_NONE);
    }

    // ── Init: call once after MH_Initialize() and Process::Hwnd is set ─
    static void Init(HWND hwnd, bool enabled)
    {
        g_hwnd = hwnd;
        g_enabled = enabled;

        // Layer 1: SetWindowDisplayAffinity – blocks WGC, DDA, PrintWindow
        if (enabled) {
            if (!SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE))
                SetWindowDisplayAffinity(hwnd, WDA_MONITOR);
        }

        // Layer 2: GDI hooks – blocks legacy BitBlt-based capture tools
        HMODULE hGDI    = GetModuleHandleW(L"gdi32.dll");
        HMODULE hUser32 = GetModuleHandleW(L"user32.dll");

        if (hGDI) {
            void* pBitBlt = GetProcAddress(hGDI, "BitBlt");
            if (pBitBlt &&
                MH_CreateHookApi(L"gdi32.dll", "BitBlt",
                    (LPVOID)hkBitBlt, (LPVOID*)&oBitBlt) == MH_OK)
                MH_EnableHook(pBitBlt);

            void* pSBlt = GetProcAddress(hGDI, "StretchBlt");
            if (pSBlt &&
                MH_CreateHookApi(L"gdi32.dll", "StretchBlt",
                    (LPVOID)hkStretchBlt, (LPVOID*)&oStretchBlt) == MH_OK)
                MH_EnableHook(pSBlt);
        }

        if (hUser32) {
            void* pPW = GetProcAddress(hUser32, "PrintWindow");
            if (pPW &&
                MH_CreateHookApi(L"user32.dll", "PrintWindow",
                    (LPVOID)hkPrintWindow, (LPVOID*)&oPrintWindow) == MH_OK)
                MH_EnableHook(pPW);
        }

        // Layer 3: Watchdog thread – keeps display affinity enforced
        g_running = true;
        g_thread = CreateThread(nullptr, 0, WatchdogProc, nullptr, 0, nullptr);
    }

    // ── Shutdown: unhook and stop watchdog ────────────────────────────
    static void Shutdown()
    {
        g_running = false;
        if (g_thread) {
            WaitForSingleObject(g_thread, 3000);
            CloseHandle(g_thread);
            g_thread = nullptr;
        }

        HMODULE hGDI    = GetModuleHandleW(L"gdi32.dll");
        HMODULE hUser32 = GetModuleHandleW(L"user32.dll");

        if (hGDI && oBitBlt)
            MH_RemoveHook(GetProcAddress(hGDI, "BitBlt"));
        if (hGDI && oStretchBlt)
            MH_RemoveHook(GetProcAddress(hGDI, "StretchBlt"));
        if (hUser32 && oPrintWindow)
            MH_RemoveHook(GetProcAddress(hUser32, "PrintWindow"));

        if (g_hwnd && IsWindow(g_hwnd))
            SetWindowDisplayAffinity(g_hwnd, WDA_NONE);
    }
}
