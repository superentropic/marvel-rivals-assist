#pragma once
#include <windows.h>
#include <unordered_map>
#include "../../global.h"

namespace AutoKeySystem {

    // ---- Key helpers (no Sleep - instant press+release) ----
    inline void TapKey(WORD vk) {
        INPUT inputs[2] = {};
        inputs[0].type = INPUT_KEYBOARD; inputs[0].ki.wVk = vk;
        inputs[1].type = INPUT_KEYBOARD; inputs[1].ki.wVk = vk; inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(2, inputs, sizeof(INPUT));
    }

    // ---- State ----
    static uint64_t s_lastMeleeTime    = 0;
    static uint64_t s_lastShieldTime   = 0;
    static uint64_t s_lastImmuneTime   = 0;
    static uint64_t s_lastKillAbTime   = 0;
    static uint64_t s_lastBuffTime     = 0;
    static uint64_t s_lastHealTime     = 0;
    static uint64_t s_lastShiftTime    = 0;
    static uint64_t s_lastIWShieldTime = 0;
    static uint64_t s_lastCDShieldTime = 0;

    // Anim cancel: schedule a tap on the next frame after LMB down
    static bool     s_prevLMB              = false;
    static uint64_t s_animCancelScheduled  = 0; // time to fire, 0 = none

    // Dodge state
    static std::unordered_map<int, float> s_prevUltPct;
    static bool     s_dodgePending  = false;
    static uint64_t s_lastDodgeTime = 0;

    inline uint64_t Now() { return GetTickCount64(); }

    // ---- Called per enemy in the actor loop (Dodge detection) ----
    inline void NotifyEnemyUlt(int heroID, float ultPct, float distMeters) {
        if (!mods::bDodgeEnabled) {
            s_prevUltPct.clear();
            return;
        }

        auto it = s_prevUltPct.find(heroID);
        if (it != s_prevUltPct.end()) {
            float prev = it->second;
            // Ult fired: was >=80%, now <25%, and enemy within range
            if (prev >= 80.0f && ultPct < 25.0f && distMeters <= mods::dodgeMaxRange) {
                bool heroEnabled = false;
                switch (heroID) {
                    case 1050: heroEnabled = mods::bDodgeInvisibleWoman; break;
                    case 1053: heroEnabled = mods::bDodgeRogue;          break; // Rogue placeholder
                    case 1031: heroEnabled = mods::bDodgeLuna;           break;
                    case 1011: heroEnabled = mods::bDodgeHulk;           break;
                    case 1042: heroEnabled = mods::bDodgePeniParker;     break;
                    case 1036: heroEnabled = mods::bDodgeSpiderMan;      break;
                    case 1020: heroEnabled = mods::bDodgeMantis;         break;
                }
                if (heroEnabled) s_dodgePending = true;
            }
        }
        s_prevUltPct[heroID] = ultPct;
    }

    // ---- Called after the enemy loop to fire pending dodges ----
    inline void FlushDodge() {
        if (!s_dodgePending) return;
        s_dodgePending = false;
        uint64_t now = Now();
        if (now - s_lastDodgeTime < 2000) return; // 2 s cooldown
        s_lastDodgeTime = now;
        if (mods::dodgeUltCancelKey)
            TapKey((WORD)mods::dodgeUltCancelKey);
    }

    // ---- Main per-frame tick ----
    // localHeroID : local player's hero ID
    // localHP / localMaxHP : local player health
    // closestEnemyDistM : nearest enemy distance in metres
    inline void Tick(int localHeroID, float localHP, float localMaxHP, float closestEnemyDistM) {
        uint64_t now = Now();

        // --- Auto Melee ---
        if (mods::bAutoKeyEnabled && mods::bAutoMeleeKey) {
            if (closestEnemyDistM > 0.0f && closestEnemyDistM <= mods::autoMeleeRange) {
                if (now - s_lastMeleeTime >= 600) {
                    TapKey((WORD)mods::autoMeleeVK);
                    s_lastMeleeTime = now;
                }
            }
        }

        // --- Auto Shield Ability (generic) ---
        if (mods::bAutoKeyEnabled && mods::bAutoShieldAbility && localMaxHP > 0.0f) {
            float hpPct = (localHP / localMaxHP) * 100.0f;
            if (hpPct < mods::autoShieldAbilityHPPct && now - s_lastShieldTime >= 3000) {
                TapKey((WORD)mods::autoShieldAbilityVK);
                s_lastShieldTime = now;
            }
        }

        // --- Auto Immune ---
        if (mods::bAutoKeyEnabled && mods::bAutoImmune && localMaxHP > 0.0f) {
            float hpPct = (localHP / localMaxHP) * 100.0f;
            if (hpPct < mods::autoImmuneHPPct && now - s_lastImmuneTime >= 5000) {
                TapKey((WORD)mods::autoImmuneVK);
                s_lastImmuneTime = now;
            }
        }

        // --- Auto Kill Ability (triggered externally via bAutoKillAbTrigger) ---
        if (mods::bAutoKeyEnabled && mods::bAutoKillAbility && mods::bAutoKillAbTrigger) {
            mods::bAutoKillAbTrigger = false;
            if (now - s_lastKillAbTime >= 1000) {
                TapKey((WORD)mods::autoKillAbilityVK);
                s_lastKillAbTime = now;
            }
        }

        // --- Animation Cancel ---
        if (mods::bAutoKeyEnabled && mods::bAnimCancel) {
            bool lmbNow = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
            if (lmbNow && !s_prevLMB) {
                // Schedule cancel after configured delay
                s_animCancelScheduled = now + (uint64_t)mods::animCancelDelayMs;
            }
            if (s_animCancelScheduled > 0 && now >= s_animCancelScheduled) {
                TapKey((WORD)mods::animCancelVK);
                s_animCancelScheduled = 0;
            }
            s_prevLMB = lmbNow;
        }

        // --- Auto Buff ---
        if (mods::bAutoKeyEnabled && mods::bAutoBuff) {
            if (now - s_lastBuffTime >= (uint64_t)mods::autoBuffIntervalMs) {
                TapKey((WORD)mods::autoBuffVK);
                s_lastBuffTime = now;
            }
        }

        // --- Auto Heal (AutoKey version) ---
        if (mods::bAutoKeyEnabled && mods::bAutoKeyHeal && localMaxHP > 0.0f) {
            float hpPct = (localHP / localMaxHP) * 100.0f;
            if (hpPct < mods::autoKeyHealHPPct && now - s_lastHealTime >= 1500) {
                TapKey((WORD)mods::autoKeyHealVK);
                s_lastHealTime = now;
            }
        }

        // --- Auto Shift Ability ---
        if (mods::bAutoKeyEnabled && mods::bAutoShiftAbility) {
            if (now - s_lastShiftTime >= (uint64_t)mods::autoShiftIntervalMs) {
                TapKey(VK_LSHIFT);
                s_lastShiftTime = now;
            }
        }

        // --- Auto Shield: Invisible Woman (heroID 1050) ---
        if (mods::bAutoKeyEnabled && mods::bAutoShieldIW && localMaxHP > 0.0f) {
            if (localHeroID == 1050) {
                if (localHP < mods::autoShieldIWHP && now - s_lastIWShieldTime >= 3000) {
                    TapKey('E');
                    s_lastIWShieldTime = now;
                }
            }
        }

        // --- Auto Shield: Cloak & Dagger (heroID 1025) ---
        if (mods::bAutoKeyEnabled && mods::bAutoShieldCD && localMaxHP > 0.0f) {
            if (localHeroID == 1025) {
                if (localHP < mods::autoShieldCDHP && now - s_lastCDShieldTime >= 3000) {
                    TapKey('E');
                    s_lastCDShieldTime = now;
                }
            }
        }
    }

} // namespace AutoKeySystem
