#pragma once
#include <windows.h>
#include <vector>
#include "../../global.h"

namespace ComboSystem {

    inline void PressKey(WORD vk) {
        INPUT input = {}; input.type = INPUT_KEYBOARD; input.ki.wVk = vk;
        SendInput(1, &input, sizeof(INPUT));
    }
    inline void ReleaseKey(WORD vk) {
        INPUT input = {}; input.type = INPUT_KEYBOARD; input.ki.wVk = vk; input.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &input, sizeof(INPUT));
    }
    inline void TapKey(WORD vk, int ms = 30) { PressKey(vk); Sleep(ms); ReleaseKey(vk); }
    inline void TapLMB(int ms = 30) {
        INPUT i = {}; i.type = INPUT_MOUSE; i.mi.dwFlags = MOUSEEVENTF_LEFTDOWN; SendInput(1, &i, sizeof(INPUT));
        Sleep(ms);
        i.mi.dwFlags = MOUSEEVENTF_LEFTUP; SendInput(1, &i, sizeof(INPUT));
    }
    inline void TapRMB(int ms = 30) {
        INPUT i = {}; i.type = INPUT_MOUSE; i.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN; SendInput(1, &i, sizeof(INPUT));
        Sleep(ms);
        i.mi.dwFlags = MOUSEEVENTF_RIGHTUP; SendInput(1, &i, sizeof(INPUT));
    }
    inline void HoldRMB() { INPUT i = {}; i.type = INPUT_MOUSE; i.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN; SendInput(1, &i, sizeof(INPUT)); }
    inline void ReleaseRMB() { INPUT i = {}; i.type = INPUT_MOUSE; i.mi.dwFlags = MOUSEEVENTF_RIGHTUP; SendInput(1, &i, sizeof(INPUT)); }
    inline void HoldLMB() { INPUT i = {}; i.type = INPUT_MOUSE; i.mi.dwFlags = MOUSEEVENTF_LEFTDOWN; SendInput(1, &i, sizeof(INPUT)); }
    inline void ReleaseLMB() { INPUT i = {}; i.type = INPUT_MOUSE; i.mi.dwFlags = MOUSEEVENTF_LEFTUP; SendInput(1, &i, sizeof(INPUT)); }

    inline bool IsHeld(int key) { return key && (GetAsyncKeyState(key) & 0x8000); }

    // Each combo is a sequence of steps run in a thread
    struct ComboStep { WORD key; int preDelay; int holdMs; bool isLMB; bool isRMB; };

    // Minimum delay after each action so the game can process the input
    // before the next step fires. Abilities need animation startup time.
    constexpr int POST_ACTION_DELAY_MS = 150;

    inline void RunSequence(const std::vector<ComboStep>& steps, int comboKey) {
        mods::bComboRunning = true;
        bool wasTB = mods::TriggerBot;
        if (mods::bComboDisableTriggerbot) mods::TriggerBot = false;

        for (int i = 0; i < (int)steps.size(); i++) {
            const auto& s = steps[i];
            // Always execute step 0 — key was pressed to trigger this thread.
            // From step 1 onward, check if key must still be held.
            if (i > 0 && mods::bComboHoldKey && !IsHeld(comboKey)) break;
            if (!mods::bCombosEnabled) break;
            Sleep(s.preDelay);
            if (s.isLMB) TapLMB(s.holdMs);
            else if (s.isRMB) TapRMB(s.holdMs);
            else TapKey(s.key, s.holdMs);
            // Wait after each action so the game registers the input
            // before we fire the next step
            if (i < (int)steps.size() - 1)
                Sleep(POST_ACTION_DELAY_MS);
        }

        if (mods::bComboDisableTriggerbot) mods::TriggerBot = wasTB;
        mods::bComboRunning = false;
    }

    // ===== Hero Combo Definitions =====
    // Each returns a vector of steps. Key codes: E=0x45, Q=0x51, Shift=VK_LSHIFT, LMB/RMB via flags

    inline void ExecRogue(int ck)       { RunSequence({{0x45,0,30},{0,80,30,true},{0x45,100,30},{0,80,30,true},{VK_LSHIFT,100,30},{0,80,30,true}}, ck); }
    inline void ExecAngela(int ck)      { RunSequence({{0x45,0,30},{0,50,30,true},{0x51,100,30},{0,50,30,true},{VK_LSHIFT,100,30}}, ck); }
    inline void ExecGambit(int ck)      { RunSequence({{0x45,0,30},{0,80,30,true},{0,80,30,true},{VK_LSHIFT,100,30},{0,80,30,true}}, ck); }
    inline void ExecDaredevil(int ck)   { RunSequence({{VK_LSHIFT,0,30},{0,80,30,true},{0x45,100,30},{0,80,30,true},{0,80,30,true}}, ck); }
    inline void ExecBlackPanther(int ck){ RunSequence({{VK_LSHIFT,0,30},{0,50,30,true},{0x45,80,30},{0,50,30,true},{0,50,30,true}}, ck); }
    inline void ExecDrStrange(int ck)   { RunSequence({{0x45,0,200},{0x51,100,30},{0,50,30,true},{0,50,30,true}}, ck); }
    inline void ExecMagic1_1(int ck)    { RunSequence({{0x45,0,30},{0,50,30,true},{VK_LSHIFT,80,30},{0,50,30,true}}, ck); }
    inline void ExecMagic1_2(int ck)    { RunSequence({{0,0,30,false,true},{0x45,80,30},{0,50,30,true},{VK_LSHIFT,80,30}}, ck); }
    inline void ExecGroot(int ck)       { RunSequence({{0x45,0,200},{0x51,100,30},{0,80,30,true},{VK_LSHIFT,100,30}}, ck); }
    inline void ExecVenom(int ck)       { RunSequence({{0x45,0,30},{0,50,30,true},{VK_LSHIFT,80,30},{0,50,30,true},{0x51,100,30}}, ck); }
    inline void ExecJeff(int ck)        { RunSequence({{0x45,0,200},{0,80,30,true},{0x51,100,30},{0,80,30,true}}, ck); }
    inline void ExecBlackWidow(int ck)  { RunSequence({{0x45,0,30},{0,50,30,true},{0,50,30,true},{VK_LSHIFT,80,30},{0,50,30,true}}, ck); }
    inline void ExecCaptAmerica(int ck) { RunSequence({{VK_LSHIFT,0,30},{0x45,80,30},{0,50,30,true},{0,50,30,true},{0x51,100,30}}, ck); }
    inline void ExecPsylocke1(int ck)   { RunSequence({{0x45,0,30},{0,50,30,true},{VK_LSHIFT,80,30},{0,50,30,true},{0,50,30,true}}, ck); }
    inline void ExecPsylocke1_2(int ck) { RunSequence({{VK_LSHIFT,0,30},{0x45,80,30},{0,50,30,true},{0,50,30,true},{0x51,100,30}}, ck); }
    inline void ExecMagneto(int ck)     { RunSequence({{0x45,0,200},{0,80,30,true},{VK_LSHIFT,100,30},{0,80,30,true}}, ck); }
    inline void ExecSpiderMan1_1(int ck){ RunSequence({{0x45,0,30},{0,50,30,true},{VK_LSHIFT,80,30},{0,50,30,true}}, ck); }
    inline void ExecSpiderMan1_2(int ck){ RunSequence({{VK_LSHIFT,0,30},{0x45,80,30},{0,50,30,true},{0,50,30,true}}, ck); }
    inline void ExecSpiderMan1_3(int ck){ RunSequence({{0x45,0,30},{0,30,30,true},{VK_LSHIFT,50,30},{0,30,30,true}}, ck); }
    inline void ExecMrFantastic(int ck) { RunSequence({{0x45,0,30},{0,50,30,true},{VK_LSHIFT,80,30},{0x51,100,30},{0,50,30,true}}, ck); }
    inline void ExecWinterSoldier1_1(int ck){ RunSequence({{0x52,0,30},{0x45,80,30},{0,50,30,true},{0,50,30,true}}, ck); } // R=reload
    inline void ExecWinterSoldier1_2(int ck){ RunSequence({{0x52,0,30},{VK_LSHIFT,80,30},{0x45,80,30},{0,50,30,true}}, ck); }
    inline void ExecWolverine(int ck)   { RunSequence({{0x45,0,30},{0,50,30,true},{VK_LSHIFT,80,30},{0,50,30,true},{0,50,30,true}}, ck); }
    inline void ExecPhoenix(int ck)     { RunSequence({{0x45,0,30},{0,50,30,true},{0x51,100,30},{VK_LSHIFT,80,30}}, ck); }
    inline void ExecThor(int ck)        { RunSequence({{0x45,0,200},{0,80,30,true},{VK_LSHIFT,100,30},{0,80,30,true}}, ck); }
    inline void ExecEmmaFrost(int ck)   { RunSequence({{0x45,0,30},{0,50,30,true},{VK_LSHIFT,80,30},{0x51,100,30}}, ck); }
    inline void ExecHawkeye(int ck)     { RunSequence({{0x45,0,30},{0,50,30,true},{0,50,30,true},{VK_LSHIFT,80,30}}, ck); }

    // Combo 2 heroes
    inline void ExecSpiderMan2_1(int ck){ RunSequence({{0,0,30,true},{0x45,80,30},{0,50,30,true},{VK_LSHIFT,80,30}}, ck); }
    inline void ExecNamor(int ck)       { RunSequence({{0x45,0,200},{VK_LSHIFT,100,30},{0,80,30,true},{0x51,100,30}}, ck); }
    inline void ExecMantisHeal(int ck)  { RunSequence({{0x45,0,200},{0x51,200,30}}, ck); }
    inline void ExecPhoenixBey(int ck)  { RunSequence({{VK_LSHIFT,0,30},{0x45,50,30},{0,30,30,true},{0x45,50,30},{0,30,30,true},{VK_LSHIFT,50,30}}, ck); }
    inline void ExecHumanTorch(int ck)  { RunSequence({{0x45,0,30},{0,50,30,true},{VK_LSHIFT,80,30},{0,50,30,true},{0x51,100,30}}, ck); }

    inline void ExecBlackPantherMark(int ck) {
        if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) return; // already marking
        TapRMB(30); Sleep(50); // mark with RMB
    }

    inline void ExecDrStrangeShield(int ck) {
        PressKey(0x45); Sleep(500); ReleaseKey(0x45); // hold E for shield
    }

    // Thread proc for combo execution
    static DWORD WINAPI ComboThread1(LPVOID) {
        int ck = mods::comboKey1;
        if (mods::bComboRogue) ExecRogue(ck);
        if (mods::bComboAngela) ExecAngela(ck);
        if (mods::bComboGambit) ExecGambit(ck);
        if (mods::bComboDaredevil) ExecDaredevil(ck);
        if (mods::bComboBlackPanther) ExecBlackPanther(ck);
        if (mods::bComboBlackPantherAutoMark) ExecBlackPantherMark(ck);
        if (mods::bComboDoctorStrange) ExecDrStrange(ck);
        if (mods::bComboDoctorStrangeHoldShield) ExecDrStrangeShield(ck);
        if (mods::bComboMagic1_1) ExecMagic1_1(ck);
        if (mods::bComboMagic1_2) ExecMagic1_2(ck);
        if (mods::bComboGroot) ExecGroot(ck);
        if (mods::bComboVenom) ExecVenom(ck);
        if (mods::bComboJeff) ExecJeff(ck);
        if (mods::bComboBlackWidow) ExecBlackWidow(ck);
        if (mods::bComboCaptainAmerica) ExecCaptAmerica(ck);
        if (mods::bComboPsylocke1) ExecPsylocke1(ck);
        if (mods::bComboPsylocke1_2) ExecPsylocke1_2(ck);
        if (mods::bComboMagneto) ExecMagneto(ck);
        if (mods::bComboSpiderMan1_1) ExecSpiderMan1_1(ck);
        if (mods::bComboSpiderMan1_2) ExecSpiderMan1_2(ck);
        if (mods::bComboSpiderMan1_3) ExecSpiderMan1_3(ck);
        if (mods::bComboMisterFantastic) ExecMrFantastic(ck);
        if (mods::bComboWinterSoldier1_1) ExecWinterSoldier1_1(ck);
        if (mods::bComboWinterSoldier1_2) ExecWinterSoldier1_2(ck);
        if (mods::bComboWolverine) ExecWolverine(ck);
        if (mods::bComboPhoenix) ExecPhoenix(ck);
        if (mods::bComboThor) ExecThor(ck);
        if (mods::bComboEmmaFrost) ExecEmmaFrost(ck);
        if (mods::bComboHawkeye) ExecHawkeye(ck);
        return 0;
    }

    static DWORD WINAPI ComboThread2(LPVOID) {
        int ck = mods::comboKey2;
        if (mods::bComboSpiderMan2_1) ExecSpiderMan2_1(ck);
        if (mods::bComboNamor) ExecNamor(ck);
        if (mods::bComboMantisAutoHeal) ExecMantisHeal(ck);
        if (mods::bComboPhoenixBeyblade) ExecPhoenixBey(ck);
        if (mods::bComboHumanTorch) ExecHumanTorch(ck);
        return 0;
    }

    // Call this every frame from DrawTransition
    inline void Tick() {
        if (!mods::bCombosEnabled) return;

        // Safety reset: if bComboRunning has been stuck for > 5 seconds, clear it
        static uint64_t s_comboStartMs = 0;
        if (mods::bComboRunning) {
            if (s_comboStartMs == 0) s_comboStartMs = GetTickCount64();
            if (GetTickCount64() - s_comboStartMs > 5000) {
                mods::bComboRunning = false;
                s_comboStartMs = 0;
            }
            return;
        }
        s_comboStartMs = 0;

        if (mods::comboKey1 && (GetAsyncKeyState(mods::comboKey1) & 1)) {
            HANDLE h = CreateThread(NULL, 0, ComboThread1, NULL, 0, NULL);
            if (h) CloseHandle(h);
        }
        if (mods::comboKey2 && (GetAsyncKeyState(mods::comboKey2) & 1)) {
            HANDLE h = CreateThread(NULL, 0, ComboThread2, NULL, 0, NULL);
            if (h) CloseHandle(h);
        }
    }
}
