#pragma once
#include <windows.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <string>
#include <chrono>
#include <unordered_set>
#include "../../global.h"

// ============================================================
//  Features.h — All regular (non-exploit) feature implementations
//  Covers: Visual, Combat Assistance, Healer/Support, QoL, Map Awareness
// ============================================================

namespace Features {

    // Cached timers
    static float lastCrouchTime = 0.0f;
    static float lastReloadTime = 0.0f;
    static float lastMeleeTime = 0.0f;
    static float lastAbilityRotTime = 0.0f;
    static int abilityRotIndex = 0;
    static bool crouchState = false;

    inline float Now() { return (float)GetTickCount64() / 1000.0f; }

    // ============================================================
    //  VISUAL FEATURES
    // ============================================================

    // --- Threat Level Indicator ---
    // Returns threat score 0-100 based on HP%, distance, ult%
    inline float CalculateThreatScore(float hpPercent, float distance, float ultPercent, float maxDistance) {
        float hpThreat = (1.0f - hpPercent) * 30.0f;  // Low HP = less threat (dying)
        float distThreat = (1.0f - std::min(distance / maxDistance, 1.0f)) * 40.0f; // Close = more threat
        float ultThreat = (ultPercent / 100.0f) * 30.0f; // High ult = more threat
        // Actually high HP = more threat since they can fight longer
        hpThreat = hpPercent * 30.0f;
        return hpThreat + distThreat + ultThreat;
    }

    inline ImU32 GetThreatColor(float threatScore) {
        if (threatScore >= mods::threatHighThreshold)
            return mods::threatHighColor;
        if (threatScore >= mods::threatMedThreshold)
            return mods::threatMedColor;
        return mods::threatLowColor;
    }

    // --- Kill Prediction ---
    inline bool CanOneShot(float enemyHP) {
        return mods::bKillPrediction && enemyHP <= mods::killPredictDamage;
    }

    // --- Damage Number Popups ---
    inline void AddDamagePopup(float screenX, float screenY, float damage, bool headshot) {
        if (!mods::bDamageNumbers) return;
        mods::DamagePopup popup;
        popup.x = screenX;
        popup.y = screenY;
        popup.value = damage;
        popup.timestamp = Now();
        popup.isHeadshot = headshot;
        mods::damagePopups.push_back(popup);
    }

    inline void DrawDamagePopups(ImDrawList* drawList, ImFont* font) {
        if (!mods::bDamageNumbers || mods::damagePopups.empty()) return;

        float now = Now();
        float fontSize = 16.0f * mods::damageNumberScale;

        // Remove expired popups
        mods::damagePopups.erase(
            std::remove_if(mods::damagePopups.begin(), mods::damagePopups.end(),
                [now](const mods::DamagePopup& p) { return now - p.timestamp > mods::damageNumberDuration; }),
            mods::damagePopups.end());

        for (auto& popup : mods::damagePopups) {
            float age = now - popup.timestamp;
            float alpha = 1.0f - (age / mods::damageNumberDuration);
            float yOffset = -age * 40.0f; // Float upward

            char buf[32];
            snprintf(buf, sizeof(buf), "%.0f", popup.value);

            ImU32 color = popup.isHeadshot ?
                IM_COL32(255, 255, 0, (int)(alpha * 255)) :
                IM_COL32(255, 80, 80, (int)(alpha * 255));

            drawList->AddText(font, fontSize, ImVec2(popup.x, popup.y + yOffset), color, buf);
        }
    }

    // --- LOS Indicator (eye icon on enemies that can see you) ---
    inline void DrawLOSIndicator(ImDrawList* drawList, ImVec2 screenPos, bool canSeePlayer) {
        if (!mods::bLOSIndicator || !canSeePlayer) return;
        // Draw a small eye icon above the enemy
        ImVec2 eyePos = ImVec2(screenPos.x, screenPos.y - 20.0f);
        drawList->AddCircle(eyePos, 6.0f, IM_COL32(255, 200, 0, 255), 12, 2.0f);
        drawList->AddCircleFilled(ImVec2(eyePos.x, eyePos.y), 2.5f, IM_COL32(255, 200, 0, 255));
    }

    // --- Sound ESP (directional compass) ---
    inline void DrawSoundESP(ImDrawList* drawList, ImVec2 screenCenter, SDK::FVector localPos,
        SDK::FRotator localRot, SDK::FVector enemyPos, float distance) {
        if (!mods::bSoundESP || distance > mods::soundESPRange / 100.0f) return;

        float dx = enemyPos.X - localPos.X;
        float dy = enemyPos.Y - localPos.Y;
        float yawRad = -localRot.Yaw * (3.14159265f / 180.0f);
        float rx = dx * cosf(yawRad) - dy * sinf(yawRad);
        float ry = dx * sinf(yawRad) + dy * cosf(yawRad);

        float angle = atan2f(ry, rx);
        float compassRadius = 80.0f;
        float dotX = screenCenter.x + cosf(angle) * compassRadius;
        float dotY = screenCenter.y + sinf(angle) * compassRadius;

        float intensity = 1.0f - (distance * 100.0f / mods::soundESPRange);
        ImU32 color = IM_COL32(255, 100, 100, (int)(intensity * 200));
        drawList->AddCircleFilled(ImVec2(dotX, dotY), 4.0f, color);
    }

    // --- Death Heatmap ---
    inline void RecordDeath(float x, float y, float z) {
        if (!mods::bDeathHeatmap) return;
        mods::DeathPoint dp;
        dp.x = x; dp.y = y; dp.z = z;
        dp.timestamp = Now();
        mods::deathPoints.push_back(dp);
        if (mods::deathPoints.size() > 200)
            mods::deathPoints.erase(mods::deathPoints.begin());
    }

    // --- Team Comp Analyzer Widget ---
    inline void DrawTeamCompAnalyzer(ImDrawList* drawList) {
        if (!mods::bTeamCompAnalyzer) return;

        // Count roles from enemyOverlayData
        // Vanguard / Strategist hero IDs for role classification
        static const std::unordered_set<int> s_vanguardIDs = { 1011, 1037, 1035, 1039, 1018, 1022, 1042, 1027, 1051, 1028 };
        static const std::unordered_set<int> s_strategistIDs = { 1046, 1016, 1025, 1047, 1020, 1031, 1023, 1050, 1054 };
        int vanguards = 0, duelists = 0, strategists = 0;
        for (auto& entry : mods::enemyOverlayData) {
            if (s_vanguardIDs.count(entry.heroID))
                vanguards++;
            else if (s_strategistIDs.count(entry.heroID))
                strategists++;
            else
                duelists++;
        }

        ImVec2 pos = mods::teamCompPos;
        drawList->AddRectFilled(pos, ImVec2(pos.x + 200, pos.y + 80), IM_COL32(20, 20, 20, 200), 0.0f);
        drawList->AddRect(pos, ImVec2(pos.x + 200, pos.y + 80), IM_COL32(100, 100, 100, 200), 0.0f);

        char buf[128];
        snprintf(buf, sizeof(buf), "Enemy Comp: %dV / %dD / %dS", vanguards, duelists, strategists);
        drawList->AddText(ImVec2(pos.x + 8, pos.y + 8), IM_COL32(255, 255, 255, 255), buf);

        if (strategists == 0)
            drawList->AddText(ImVec2(pos.x + 8, pos.y + 28), IM_COL32(0, 255, 0, 255), "No healers - play aggressive!");
        else if (strategists >= 2)
            drawList->AddText(ImVec2(pos.x + 8, pos.y + 28), IM_COL32(255, 100, 100, 255), "Double healer - focus them!");

        if (vanguards >= 3)
            drawList->AddText(ImVec2(pos.x + 8, pos.y + 48), IM_COL32(255, 165, 0, 255), "Heavy frontline - flank!");
        else if (duelists >= 4)
            drawList->AddText(ImVec2(pos.x + 8, pos.y + 48), IM_COL32(255, 165, 0, 255), "DPS heavy - stay with team!");
    }

    // ============================================================
    //  COMBAT ASSISTANCE FEATURES
    // ============================================================

    // --- Auto Reload ---
    inline void CheckAutoReload(SDK::AMarvelBaseCharacter* localChar) {
        if (!mods::bAutoReload || !localChar) return;

        float now = Now();
        float delay = mods::autoReloadDelayMs / 1000.0f;
        if (now - lastReloadTime < delay) return;

        auto* equipComp = localChar->EquipComponent;
        if (!equipComp || !IsValid(equipComp)) return;

        auto* weapon = equipComp->GetCurrentWeapon();
        if (!weapon || !IsValid(weapon)) return;

        // Check if ammo is depleted by reading shooting logics
        auto logics = weapon->ShootingLogics;
        for (auto* logic : logics) {
            if (logic && IsValid(logic)) {
                // If CurrentAmmo field is 0, trigger reload
                INPUT input = {};
                input.type = INPUT_KEYBOARD;
                input.ki.wVk = 'R';
                input.ki.dwFlags = 0;
                SendInput(1, &input, sizeof(INPUT));
                input.ki.dwFlags = KEYEVENTF_KEYUP;
                SendInput(1, &input, sizeof(INPUT));
                lastReloadTime = now;
                break;
            }
        }
    }

    // --- Auto Melee Range ---
    inline void CheckAutoMelee(float closestDistance) {
        if (!mods::bAutoMeleeRange) return;
        if (closestDistance > mods::autoMeleeDistance) return;

        float now = Now();
        if (now - lastMeleeTime < 0.4f) return;

        INPUT input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = 'V';
        input.ki.dwFlags = 0;
        SendInput(1, &input, sizeof(INPUT));
        input.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &input, sizeof(INPUT));
        lastMeleeTime = now;
    }

    // --- Auto Crouch Spam ---
    inline void CheckCrouchSpam() {
        if (!mods::bAutoCrouchSpam) return;
        // Only crouch spam during combat (when aim key held)
        if (!(GetAsyncKeyState(VK_LBUTTON) & 0x8000)) return;

        float now = Now();
        float interval = mods::crouchSpamIntervalMs / 1000.0f;
        if (now - lastCrouchTime < interval) return;

        INPUT input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = VK_LCONTROL;
        input.ki.dwFlags = crouchState ? KEYEVENTF_KEYUP : 0;
        SendInput(1, &input, sizeof(INPUT));
        crouchState = !crouchState;
        lastCrouchTime = now;
    }

    // --- Target Cycling ---
    inline void CheckTargetCycling(std::vector<SDK::AMarvelBaseCharacter*>& enemyList) {
        if (!mods::bTargetCycling || enemyList.empty()) return;

        if (mods::targetCycleNextKey && (GetAsyncKeyState(mods::targetCycleNextKey) & 1)) {
            mods::currentTargetIndex = (mods::currentTargetIndex + 1) % (int)enemyList.size();
        }
        if (mods::targetCyclePrevKey && (GetAsyncKeyState(mods::targetCyclePrevKey) & 1)) {
            mods::currentTargetIndex = (mods::currentTargetIndex - 1 + (int)enemyList.size()) % (int)enemyList.size();
        }

        if (mods::currentTargetIndex >= (int)enemyList.size())
            mods::currentTargetIndex = 0;
    }

    // --- Hit Sound (single persistent worker thread) ---
    static volatile LONG s_hitSoundFreq = 0;
    static volatile LONG s_hitSoundDur  = 0;
    static volatile LONG s_hitSoundReq  = 0;

    static DWORD WINAPI HitSoundWorker(LPVOID) {
        for (;;) {
            if (InterlockedCompareExchange(&s_hitSoundReq, 0, 1) == 1) {
                DWORD f = (DWORD)InterlockedExchange(&s_hitSoundFreq, 0);
                DWORD d = (DWORD)InterlockedExchange(&s_hitSoundDur, 0);
                if (f > 0 && d > 0) Beep(f, d);
            } else {
                Sleep(1);
            }
        }
        return 0;
    }

    inline void EnsureHitSoundThread() {
        static bool s_init = false;
        if (!s_init) {
            HANDLE h = CreateThread(NULL, 0, HitSoundWorker, NULL, 0, NULL);
            if (h) CloseHandle(h);
            s_init = true;
        }
    }

    inline void PlayHitSound(float prevHP, float currHP) {
        if (!mods::bHitSound) return;
        if (currHP >= prevHP) return; // No damage dealt

        EnsureHitSoundThread();

        DWORD freq = 800;
        DWORD dur = 50;

        switch (mods::hitSoundType) {
        case 0: freq = 800; dur = 30; break;   // click
        case 1: freq = 1200; dur = 80; break;  // bell
        case 2: freq = 600; dur = 60; break;   // quake-style
        }
        InterlockedExchange(&s_hitSoundFreq, (LONG)freq);
        InterlockedExchange(&s_hitSoundDur,  (LONG)dur);
        InterlockedExchange(&s_hitSoundReq,  1);
    }

    // --- Recoil Pattern Display ---
    inline void DrawRecoilDisplay(ImDrawList* drawList, ImVec2 screenCenter) {
        if (!mods::bRecoilDisplay) return;

        // Draw a simple recoil guide (vertical line showing expected recoil direction)
        float scale = mods::recoilDisplayScale * 30.0f;
        ImVec2 start = ImVec2(screenCenter.x + 30, screenCenter.y);
        ImVec2 end = ImVec2(screenCenter.x + 30, screenCenter.y - scale);

        drawList->AddLine(start, end, IM_COL32(255, 255, 0, 150), 2.0f);
        drawList->AddTriangleFilled(
            ImVec2(end.x - 4, end.y + 4),
            ImVec2(end.x + 4, end.y + 4),
            ImVec2(end.x, end.y),
            IM_COL32(255, 255, 0, 150));
    }

    // ============================================================
    //  HEALER / SUPPORT FEATURES
    // ============================================================

    // --- Smart Heal Queue + Team Dashboard (merged: single actor iteration, single LOS check per teammate) ---
    inline void UpdateHealQueue(SDK::UWorld* World, SDK::APlayerController* PC, SDK::APawn* LocalPawn) {
        bool needsQueue = mods::bSmartHealQueue || mods::bAutoHealTeammates ||
                          mods::bHealSnipeAlert  || mods::bAutoUltHeal      ||
                          mods::bHealerDashboard || mods::bHealAim;
        bool needsDash = mods::bTeamHPDashboard;
        if ((!needsQueue || !mods::bHealerMode) && !needsDash) return;
        if (!World || !PC || !LocalPawn) return;

        bool doQueue = needsQueue && mods::bHealerMode;

        mods::healQueue.clear();
        mods::teamDashEntries.clear();
        SDK::FVector localPos = LocalPawn->K2_GetActorLocation();

        SDK::ULevel* Level = World->PersistentLevel;
        if (!Level) return;

        const auto& ActorList = Level->Actors;
        if (!ActorList.IsValid()) return;

        static SDK::UClass* ClassToFind = SDK::AMarvelBaseCharacter::StaticClass();

        for (int i = 0; i < ActorList.Num(); i++) {
            if (!ActorList.IsValidIndex(i)) continue;
            SDK::AActor* actor = ActorList[i];
            if (!actor || !IsValid(actor)) continue;
            if (!actor->IsA(ClassToFind)) continue;

            SDK::AMarvelBaseCharacter* Player = reinterpret_cast<SDK::AMarvelBaseCharacter*>(actor);
            if (!Player || !IsValid(Player)) continue;
            if (Player == LocalPawn) continue;

            auto* PlayerState = static_cast<SDK::AMarvelPlayerState*>(Player->PlayerState);
            auto* LocalState = static_cast<SDK::AMarvelPlayerState*>(LocalPawn->PlayerState);
            if (!PlayerState || !LocalState) continue;
            if (PlayerState->OriginalTeamID != LocalState->OriginalTeamID) continue;

            float hp = Player->GetCurrentHealth();
            float maxHp = Player->GetMaxHealth();
            if (maxHp <= 0.0f) continue;

            float dist = SDK::UKismetMathLibrary::Vector_Distance(localPos, Player->K2_GetActorLocation()) / 100.0f;
            bool los = PC->LineOfSightTo(Player, SDK::FVector(), false);

            int pHeroID = Player->HeroID;

            if (doQueue && hp > 0.0f) {
                float hpPct = hp / maxHp;
                int priority = (int)((1.0f - hpPct) * 100.0f);
                if (los) priority += 20;
                if (hpPct < 0.3f) priority += 50;

                mods::HealQueueEntry hEntry;
                hEntry.heroID = pHeroID;
                hEntry.hp = hp;
                hEntry.maxHp = maxHp;
                hEntry.distance = dist;
                hEntry.los = los;
                hEntry.priority = priority;
                mods::healQueue.push_back(hEntry);
            }

            if (needsDash) {
                mods::TeammateDashEntry dEntry;
                dEntry.heroID = pHeroID;
                dEntry.hp = hp;
                dEntry.maxHp = maxHp;
                dEntry.distance = dist;
                dEntry.hasLOS = los;
                mods::teamDashEntries.push_back(dEntry);
            }
        }

        if (doQueue) {
            std::sort(mods::healQueue.begin(), mods::healQueue.end(),
                [](const mods::HealQueueEntry& a, const mods::HealQueueEntry& b) {
                    return a.priority > b.priority;
                });
        }
    }

    // UpdateTeamDashboard is now merged into UpdateHealQueue above
    inline void UpdateTeamDashboard(SDK::UWorld*, SDK::APlayerController*, SDK::APawn*) { }

    // --- Auto Heal Teammates ---
    static float s_lastAutoHealTime    = 0.0f;
    static float s_lastAutoUltTime     = 0.0f;
    static float s_lastAutoShieldTime  = 0.0f;
    static float s_prevTotalHeal       = -1.0f;

    inline void CheckAutoHeal(SDK::AMarvelBaseCharacter* localChar) {
        if (!mods::bHealerMode || !mods::bAutoHealTeammates || !localChar) return;
        if (mods::healQueue.empty()) return;

        float now = Now();
        if (now - s_lastAutoHealTime < 0.4f) return;

        // Find which healer key to press for this hero
        int heroID = localChar->HeroID;
        auto nameIt = mods::heroIDToName.find(heroID);
        if (nameIt == mods::heroIDToName.end()) return;
        auto keyIt = mods::healerAbilityKeys.find(nameIt->second);
        if (keyIt == mods::healerAbilityKeys.end()) return;
        int healKey = keyIt->second;

        // Find the highest-priority teammate that needs healing
        bool healed = false;
        for (auto& entry : mods::healQueue) {
            float hpPct = (entry.maxHp > 0.0f) ? (entry.hp / entry.maxHp * 100.0f) : 100.0f;
            if (hpPct > mods::healThresholdPercent) continue;
            if (mods::bHealerLOSCheck && !entry.los) continue;

            { auto _hIt = mods::heroIDToName.find(entry.heroID); mods::currentHealTarget = (_hIt != mods::heroIDToName.end()) ? _hIt->second : "Unknown"; }
            mods::currentTargetHP     = entry.hp;
            mods::currentTargetMaxHP  = entry.maxHp;

            INPUT inp = {};
            inp.type = INPUT_KEYBOARD;
            inp.ki.wVk = (WORD)healKey;
            inp.ki.dwFlags = 0;
            SendInput(1, &inp, sizeof(INPUT));
            Sleep(30);
            inp.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &inp, sizeof(INPUT));

            s_lastAutoHealTime = now;
            mods::teammatesHealed++;
            healed = true;
            break;
        }
        if (!healed) mods::currentHealTarget = "None";
    }

    // --- Update Healer Dashboard Stats from PlayerState ---
    inline void UpdateHealerStats(SDK::AMarvelBaseCharacter* localChar) {
        if (!mods::bHealerMode || !localChar) return;

        auto* PS = static_cast<SDK::AMarvelPlayerState*>(localChar->PlayerState);
        if (!PS) return;

        float currentTotal = PS->TotalHeal;
        if (s_prevTotalHeal < 0.0f) { s_prevTotalHeal = currentTotal; return; }

        float delta = currentTotal - s_prevTotalHeal;
        if (delta < 0.0f) delta = 0.0f;
        s_prevTotalHeal = currentTotal;

        mods::totalHealingDone = currentTotal;

        // Rolling 60-entry window (each entry = one frame's heal delta)
        mods::healingWindow[mods::healingWindowIdx] = delta;
        mods::healingWindowIdx = (mods::healingWindowIdx + 1) % 60;

        // HPS: sum of window / 60 * avg fps (approximate at 60fps = /1s)
        float total = 0.0f;
        for (int i = 0; i < 60; i++) total += mods::healingWindow[i];
        mods::healingPerSecond = total; // sum over ~1s at 60fps
    }

    // --- Auto Ult Heal ---
    inline void CheckAutoUltHeal() {
        if (!mods::bHealerMode || !mods::bAutoUltHeal || mods::healQueue.empty()) return;

        float now = Now();
        if (now - s_lastAutoUltTime < 10.0f) return; // 10s cooldown

        int criticalCount = 0;
        for (auto& entry : mods::healQueue) {
            float pct = (entry.maxHp > 0.0f) ? (entry.hp / entry.maxHp * 100.0f) : 100.0f;
            if (pct < mods::autoUltHealThreshold) criticalCount++;
        }

        if (criticalCount >= 2) { // 2+ teammates critical → ult
            INPUT inp = {};
            inp.type = INPUT_KEYBOARD;
            inp.ki.wVk = 'Q';
            inp.ki.dwFlags = 0;
            SendInput(1, &inp, sizeof(INPUT));
            Sleep(30);
            inp.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &inp, sizeof(INPUT));
            s_lastAutoUltTime = now;
        }
    }

    // --- Auto Shield ---
    inline void CheckAutoShield(SDK::AMarvelBaseCharacter* localChar) {
        if (!mods::bHealerMode || !mods::bHealerAutoShield || !localChar) return;

        float now = Now();
        if (now - s_lastAutoShieldTime < 3.0f) return;

        float hp    = localChar->GetCurrentHealth();
        float maxHp = localChar->GetMaxHealth();
        if (maxHp <= 0.0f) return;

        float hpPct = (hp / maxHp) * 100.0f;
        if (hpPct < mods::autoShieldThreshold) {
            INPUT inp = {};
            inp.type = INPUT_KEYBOARD;
            inp.ki.wVk = 'E';
            inp.ki.dwFlags = 0;
            SendInput(1, &inp, sizeof(INPUT));
            Sleep(30);
            inp.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &inp, sizeof(INPUT));
            s_lastAutoShieldTime = now;
            mods::shieldsApplied++;
        }
    }

    // --- Update Smart Heal Target (always-on when healer mode active, no key needed) ---
    inline void UpdateHealTarget() {
        if (!mods::bHealerMode) return;
        if (mods::healQueue.empty()) {
            mods::currentHealTarget  = "None";
            mods::currentTargetHP    = 0.0f;
            mods::currentTargetMaxHP = 0.0f;
            return;
        }
        // healQueue is already sorted highest priority first
        const auto& top = mods::healQueue.front();
        { auto _hIt = mods::heroIDToName.find(top.heroID); mods::currentHealTarget = (_hIt != mods::heroIDToName.end()) ? _hIt->second : "Unknown"; }
        mods::currentTargetHP    = top.hp;
        mods::currentTargetMaxHP = top.maxHp;
    }

    // --- Heal Aim (teammate aimbot) ---
    inline void CheckHealAim(SDK::APlayerController* PC, SDK::APawn* LocalPawn,
                              SDK::UWorld* World, ImVec2 screenCenter) {
        if (!mods::bHealerMode || !mods::bHealAim || !PC || !LocalPawn || !World) return;
        if (!GetAsyncKeyState(mods::healAimKey)) return;

        SDK::ULevel* Level = World->PersistentLevel;
        if (!Level) return;
        const auto& ActorList = Level->Actors;
        if (!ActorList.IsValid()) return;

        static SDK::UClass* ClassToFind = SDK::AMarvelBaseCharacter::StaticClass();
        auto* LocalState = static_cast<SDK::AMarvelPlayerState*>(LocalPawn->PlayerState);
        if (!LocalState) return;

        struct TeammateInfo {
            SDK::AMarvelBaseCharacter* player;
            float hp; float maxHp;
            float distToCenter; float distance;
        };
        static std::vector<TeammateInfo> candidates;
        candidates.clear();

        for (int i = 0; i < ActorList.Num(); i++) {
            if (!ActorList.IsValidIndex(i)) continue;
            SDK::AActor* actor = ActorList[i];
            if (!actor || !IsValid(actor)) continue;
            if (!actor->IsA(ClassToFind)) continue;

            SDK::AMarvelBaseCharacter* Player = reinterpret_cast<SDK::AMarvelBaseCharacter*>(actor);
            if (!Player || !IsValid(Player)) continue;
            if (Player == LocalPawn) continue;

            auto* PS = static_cast<SDK::AMarvelPlayerState*>(Player->PlayerState);
            if (!PS || PS->OriginalTeamID != LocalState->OriginalTeamID) continue;

            float hp = Player->GetCurrentHealth();
            float maxHp = Player->GetMaxHealth();
            if (hp <= 0.0f || maxHp <= 0.0f) continue;

            SDK::USkeletalMeshComponent* Mesh = Player->GetMesh();
            if (!Mesh || !IsValid(Mesh)) continue;

            static SDK::FName HeadBone = SDK::UKismetStringLibrary::Conv_StringToName(SDK::FString(L"Head"));
            SDK::FVector head3D = Mesh->GetSocketLocation(HeadBone);
            SDK::FVector2D head2D;
            if (!PC->ProjectWorldLocationToScreen(head3D, &head2D, true)) continue;

            float dx = head2D.X - screenCenter.x;
            float dy = head2D.Y - screenCenter.y;
            float distToCenter = sqrtf(dx * dx + dy * dy);
            if (distToCenter > mods::healAimFov) continue;

            float dist = SDK::UKismetMathLibrary::Vector_Distance(
                LocalPawn->K2_GetActorLocation(), Player->K2_GetActorLocation()) / 100.0f;
            candidates.push_back({ Player, hp, maxHp, distToCenter, dist });
        }

        if (candidates.empty()) return;

        switch (mods::healAimPriority) {
        case 0: // Lowest HP %
            std::sort(candidates.begin(), candidates.end(), [](const TeammateInfo& a, const TeammateInfo& b) {
                return (a.hp / a.maxHp) < (b.hp / b.maxHp);
            });
            break;
        case 1: // Closest to crosshair
            std::sort(candidates.begin(), candidates.end(), [](const TeammateInfo& a, const TeammateInfo& b) {
                return a.distToCenter < b.distToCenter;
            });
            break;
        case 2: // Closest distance
            std::sort(candidates.begin(), candidates.end(), [](const TeammateInfo& a, const TeammateInfo& b) {
                return a.distance < b.distance;
            });
            break;
        }

        SDK::AMarvelBaseCharacter* HealTarget = candidates.front().player;
        SDK::USkeletalMeshComponent* Mesh = HealTarget->GetMesh();
        if (!Mesh || !IsValid(Mesh)) return;

        static SDK::FName s_HeadBone = SDK::UKismetStringLibrary::Conv_StringToName(SDK::FString(L"Head"));
        SDK::FVector targetPos = Mesh->GetSocketLocation(s_HeadBone);

        float DeltaTime = SDK::UGameplayStatics::GetWorldDeltaSeconds(World);
        SDK::FRotator TargetRot  = SDK::UKismetMathLibrary::FindLookAtRotation(
            PC->PlayerCameraManager->GetCameraLocation(), targetPos);
        float SpeedFactor = SDK::UKismetMathLibrary::Clamp(mods::healAimSmoothing, 1.0f, 200.0f) * 0.5f;
        SDK::FRotator CurrentRot = PC->GetControlRotation();
        SDK::FRotator DeltaRot   = SDK::UKismetMathLibrary::NormalizedDeltaRotator(TargetRot, CurrentRot);
        float NewPitch = SDK::UKismetMathLibrary::FInterpTo(CurrentRot.Pitch, CurrentRot.Pitch + DeltaRot.Pitch, DeltaTime, SpeedFactor);
        float NewYaw   = SDK::UKismetMathLibrary::FInterpTo(CurrentRot.Yaw,   CurrentRot.Yaw   + DeltaRot.Yaw,   DeltaTime, SpeedFactor);
        PC->SetControlRotation(SDK::FRotator(NewPitch, NewYaw, 0));
    }

    // --- Heal Snipe Alert ---
    inline void CheckHealSnipeAlert(ImDrawList* drawList, ImVec2 screenCenter) {
        if (!mods::bHealSnipeAlert || mods::healQueue.empty()) return;

        for (auto& entry : mods::healQueue) {
            if (entry.hp <= mods::healSnipeAlertThreshold && entry.hp > 0.0f) {
                // Flash warning
                float pulse = sinf(Now() * 8.0f) * 0.5f + 0.5f;
                ImU32 alertColor = IM_COL32(255, 0, 0, (int)(pulse * 255));

                char buf[64];
                auto _aIt = mods::heroIDToName.find(entry.heroID);
                const char* _an = (_aIt != mods::heroIDToName.end()) ? _aIt->second.c_str() : "???";
                snprintf(buf, sizeof(buf), "! %s CRITICAL: %.0f HP !", _an, entry.hp);
                ImVec2 textSize = ImGui::CalcTextSize(buf);
                drawList->AddText(ImVec2(screenCenter.x - textSize.x / 2, screenCenter.y + 60), alertColor, buf);
                break; // Only show most critical
            }
        }
    }

    // --- Auto Ability Rotation (Healer) ---
    inline void CheckAbilityRotation(SDK::AMarvelBaseCharacter* localChar) {
        if (!mods::bAutoAbilityRotation || !localChar) return;

        float now = Now();
        float delay = mods::abilityRotationDelayMs / 1000.0f;
        if (now - lastAbilityRotTime < delay) return;

        // Rotate through heal abilities: E -> Q -> Shift
        int keys[] = { 'E', 'Q', VK_LSHIFT };
        int keyCount = 3;

        INPUT input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = keys[abilityRotIndex % keyCount];
        input.ki.dwFlags = 0;
        SendInput(1, &input, sizeof(INPUT));
        input.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &input, sizeof(INPUT));

        abilityRotIndex++;
        lastAbilityRotTime = now;
    }

    // ============================================================
    //  CONFIG / QOL FEATURES
    // ============================================================

    // --- Stream Safe Mode ---
    inline void ApplyStreamSafeMode() {
        if (!mods::bStreamSafeMode) return;
        // Disable all visible overlays
        mods::bESPBox = false;
        mods::bSkeletonESP = false;
        mods::bTracerLines = false;
        mods::bGlow = false;
        mods::aimbotFovCircle = false;
        mods::bRadar = false;
        mods::bShowEnemyOverlay = false;
        mods::bUltTracker = false;
        mods::bDamageNumbers = false;
        mods::bSoundESP = false;
        mods::bTeamCompAnalyzer = false;
        mods::bEnhancedMinimap = false;
    }

    // --- Session Stats ---
    inline void DrawSessionStats(ImDrawList* drawList) {
        if (!mods::bSessionStats) return;

        if (mods::sessionStartTime == 0.0f) mods::sessionStartTime = Now();
        float elapsed = Now() - mods::sessionStartTime;
        int minutes = (int)(elapsed / 60.0f);
        int seconds = (int)elapsed % 60;

        float accuracy = (mods::sessionShots > 0) ?
            ((float)mods::sessionHeadshots / (float)mods::sessionShots * 100.0f) : 0.0f;

        ImVec2 pos = mods::sessionStatsPos;
        drawList->AddRectFilled(pos, ImVec2(pos.x + 220, pos.y + 140), IM_COL32(15, 15, 15, 210), 0.0f);
        drawList->AddRect(pos, ImVec2(pos.x + 220, pos.y + 140), IM_COL32(80, 80, 80, 200), 0.0f);

        char buf[128];
        float y = pos.y + 5;
        drawList->AddText(ImVec2(pos.x + 5, y), IM_COL32(199, 43, 54, 255), "Session Stats"); y += 18;
        snprintf(buf, sizeof(buf), "Time: %02d:%02d", minutes, seconds);
        drawList->AddText(ImVec2(pos.x + 5, y), IM_COL32(200, 200, 200, 255), buf); y += 15;
        snprintf(buf, sizeof(buf), "K/D/A: %d/%d/%d", mods::sessionKills, mods::sessionDeaths, mods::sessionAssists);
        drawList->AddText(ImVec2(pos.x + 5, y), IM_COL32(200, 200, 200, 255), buf); y += 15;
        snprintf(buf, sizeof(buf), "Damage: %.0f", mods::sessionDamageDealt);
        drawList->AddText(ImVec2(pos.x + 5, y), IM_COL32(200, 200, 200, 255), buf); y += 15;
        snprintf(buf, sizeof(buf), "Healing: %.0f", mods::sessionHealingDone);
        drawList->AddText(ImVec2(pos.x + 5, y), IM_COL32(200, 200, 200, 255), buf); y += 15;
        snprintf(buf, sizeof(buf), "HS%%: %.1f%%", accuracy);
        drawList->AddText(ImVec2(pos.x + 5, y), IM_COL32(200, 200, 200, 255), buf);
    }

    // --- Match Timer ---
    inline void DrawMatchTimer(ImDrawList* drawList) {
        if (!mods::bMatchTimer) return;

        if (mods::matchStartTime == 0.0f) mods::matchStartTime = Now();
        float elapsed = Now() - mods::matchStartTime;
        int min = (int)(elapsed / 60.0f);
        int sec = (int)elapsed % 60;

        char buf[32];
        snprintf(buf, sizeof(buf), "%02d:%02d", min, sec);

        ImVec2 pos = mods::matchTimerPos;
        drawList->AddRectFilled(pos, ImVec2(pos.x + 70, pos.y + 25), IM_COL32(15, 15, 15, 200), 0.0f);
        drawList->AddText(ImVec2(pos.x + 8, pos.y + 5), IM_COL32(255, 255, 255, 255), buf);
    }

    // --- Hotkey Cheat Sheet ---
    inline void DrawHotkeySheet(ImDrawList* drawList) {
        if (!mods::bHotkeyCheatSheet) return;

        ImVec2 pos = mods::hotkeySheetPos;
        float w = 200.0f, lineH = 14.0f;
        int lineCount = 0;

        struct HKEntry { const char* label; int vk; bool active; };
        HKEntry entries[] = {
            { "Aimbot", mods::aimbotKey, mods::aimbot },
            { "Triggerbot", 0, mods::TriggerBot },
            { "ESP", 0, mods::bESPBox },
            { "Radar", 0, mods::bRadar },
            { "Speed Hack", 0, mods::bSpeedHack },
            { "Fly Hack", 0, mods::bFlyHack },
            { "No Cooldown", 0, mods::bNoCooldown },
        };

        for (auto& e : entries) lineCount++;

        drawList->AddRectFilled(pos, ImVec2(pos.x + w, pos.y + 20 + lineCount * lineH), IM_COL32(15, 15, 15, 210), 0.0f);
        drawList->AddRect(pos, ImVec2(pos.x + w, pos.y + 20 + lineCount * lineH), IM_COL32(80, 80, 80, 200), 0.0f);

        drawList->AddText(ImVec2(pos.x + 5, pos.y + 3), IM_COL32(199, 43, 54, 255), "Hotkeys");

        float y = pos.y + 20;
        for (auto& e : entries) {
            ImU32 statusColor = e.active ? IM_COL32(0, 200, 0, 255) : IM_COL32(120, 120, 120, 255);
            drawList->AddText(ImVec2(pos.x + 5, y), IM_COL32(200, 200, 200, 255), e.label);
            drawList->AddText(ImVec2(pos.x + w - 30, y), statusColor, e.active ? "ON" : "OFF");
            y += lineH;
        }
    }

    // ============================================================
    //  MAP / GAME AWARENESS FEATURES
    // ============================================================

    // --- Flank Alert ---
    inline void CheckFlankAlert(ImDrawList* drawList, ImVec2 screenCenter,
        SDK::FVector localPos, SDK::FRotator localRot, SDK::FVector enemyPos, float distance) {
        if (!mods::bFlankAlert) return;
        if (distance > mods::flankAlertRange / 100.0f) return;

        float dx = enemyPos.X - localPos.X;
        float dy = enemyPos.Y - localPos.Y;

        float enemyAngle = atan2f(dy, dx) * (180.0f / 3.14159265f);
        float facingAngle = localRot.Yaw;

        float angleDiff = fmodf(fabsf(enemyAngle - facingAngle), 360.0f);
        if (angleDiff > 180.0f) angleDiff = 360.0f - angleDiff;

        if (angleDiff > mods::flankAlertAngle) {
            // Enemy is behind us — draw warning
            float pulse = sinf(Now() * 6.0f) * 0.5f + 0.5f;
            ImU32 alertColor = IM_COL32(255, 50, 50, (int)(pulse * 220));

            // Draw directional arrow
            float arrowAngle = (enemyAngle - facingAngle) * (3.14159265f / 180.0f);
            float arrowDist = 100.0f;
            ImVec2 arrowPos = ImVec2(
                screenCenter.x + cosf(arrowAngle) * arrowDist,
                screenCenter.y + sinf(arrowAngle) * arrowDist);

            drawList->AddTriangleFilled(
                ImVec2(arrowPos.x - 8, arrowPos.y - 8),
                ImVec2(arrowPos.x + 8, arrowPos.y - 8),
                ImVec2(arrowPos.x, arrowPos.y + 8),
                alertColor);

            if (mods::bFlankAlertSound) {
                static float lastBeepTime = 0.0f;
                if (Now() - lastBeepTime > 1.0f) {
                    Beep(1000, 100);
                    lastBeepTime = Now();
                }
            }
        }
    }

    // --- Spawn Timer ---
    inline void TrackEnemyDeath(const std::string& heroName) {
        if (!mods::bSpawnTimer) return;
        mods::SpawnTimerEntry entry;
        entry.heroName = heroName;
        entry.deathTime = Now();
        entry.respawnDuration = 10.0f; // Default respawn time
        mods::spawnTimers.push_back(entry);
    }

    inline void DrawSpawnTimers(ImDrawList* drawList, ImVec2 pos) {
        if (!mods::bSpawnTimer || mods::spawnTimers.empty()) return;

        float now = Now();
        // Remove expired timers
        mods::spawnTimers.erase(
            std::remove_if(mods::spawnTimers.begin(), mods::spawnTimers.end(),
                [now](const mods::SpawnTimerEntry& e) { return now - e.deathTime > e.respawnDuration + 2.0f; }),
            mods::spawnTimers.end());

        if (mods::spawnTimers.empty()) return;

        drawList->AddRectFilled(pos, ImVec2(pos.x + 180, pos.y + 20 + mods::spawnTimers.size() * 15),
            IM_COL32(15, 15, 15, 200), 0.0f);
        drawList->AddText(ImVec2(pos.x + 5, pos.y + 2), IM_COL32(199, 43, 54, 255), "Respawn Timers");

        float y = pos.y + 20;
        for (auto& entry : mods::spawnTimers) {
            float remaining = entry.respawnDuration - (now - entry.deathTime);
            if (remaining < 0) remaining = 0;

            char buf[64];
            snprintf(buf, sizeof(buf), "%s: %.1fs", entry.heroName.c_str(), remaining);
            ImU32 color = remaining <= 2.0f ? IM_COL32(255, 80, 80, 255) : IM_COL32(200, 200, 200, 255);
            drawList->AddText(ImVec2(pos.x + 5, y), color, buf);
            y += 15;
        }
    }

    // --- Objective Timer ---
    inline void DrawObjectiveTimer(ImDrawList* drawList) {
        if (!mods::bObjectiveTimer) return;

        if (mods::objectiveStartTime == 0.0f) mods::objectiveStartTime = Now();
        float elapsed = Now() - mods::objectiveStartTime;
        int min = (int)(elapsed / 60.0f);
        int sec = (int)elapsed % 60;

        char buf[32];
        snprintf(buf, sizeof(buf), "OBJ: %02d:%02d", min, sec);

        ImVec2 pos = mods::objectiveTimerPos;
        drawList->AddRectFilled(pos, ImVec2(pos.x + 90, pos.y + 25), IM_COL32(15, 15, 15, 200), 0.0f);
        drawList->AddText(ImVec2(pos.x + 8, pos.y + 5), IM_COL32(255, 200, 0, 255), buf);
    }

    // --- Enhanced Minimap ---
    inline void DrawEnhancedMinimap(ImDrawList* drawList, SDK::FVector localPos, SDK::FRotator localRot) {
        if (!mods::bEnhancedMinimap) return;

        ImVec2 pos = mods::minimapPos;
        float size = mods::minimapSize;
        float halfSize = size / 2.0f;
        ImVec2 center = ImVec2(pos.x + halfSize, pos.y + halfSize);

        // Background
        drawList->AddRectFilled(pos, ImVec2(pos.x + size, pos.y + size), IM_COL32(20, 25, 30, 220), 0.0f);
        drawList->AddRect(pos, ImVec2(pos.x + size, pos.y + size), IM_COL32(80, 80, 80, 200), 0.0f);

        // Grid lines
        for (int g = 1; g < 4; g++) {
            float offset = (size / 4.0f) * g;
            drawList->AddLine(ImVec2(pos.x + offset, pos.y), ImVec2(pos.x + offset, pos.y + size),
                IM_COL32(40, 40, 40, 120));
            drawList->AddLine(ImVec2(pos.x, pos.y + offset), ImVec2(pos.x + size, pos.y + offset),
                IM_COL32(40, 40, 40, 120));
        }

        // Player indicator (center, with rotation)
        float yawRad = localRot.Yaw * (3.14159265f / 180.0f);
        float triSize = 6.0f;
        ImVec2 p1 = ImVec2(center.x + cosf(yawRad) * triSize * 1.5f, center.y + sinf(yawRad) * triSize * 1.5f);
        ImVec2 p2 = ImVec2(center.x + cosf(yawRad + 2.5f) * triSize, center.y + sinf(yawRad + 2.5f) * triSize);
        ImVec2 p3 = ImVec2(center.x + cosf(yawRad - 2.5f) * triSize, center.y + sinf(yawRad - 2.5f) * triSize);
        drawList->AddTriangleFilled(p1, p2, p3, IM_COL32(0, 180, 255, 255));

        // Cardinal directions
        drawList->AddText(ImVec2(center.x - 3, pos.y + 2), IM_COL32(200, 200, 200, 150), "N");
        drawList->AddText(ImVec2(center.x - 3, pos.y + size - 14), IM_COL32(200, 200, 200, 150), "S");
        drawList->AddText(ImVec2(pos.x + 3, center.y - 6), IM_COL32(200, 200, 200, 150), "W");
        drawList->AddText(ImVec2(pos.x + size - 10, center.y - 6), IM_COL32(200, 200, 200, 150), "E");
    }

    // Place enemy dot on enhanced minimap
    inline void DrawMinimapEnemy(ImDrawList* drawList, SDK::FVector localPos, SDK::FRotator localRot,
        SDK::FVector enemyPos, float elevationDiff) {
        if (!mods::bEnhancedMinimap) return;

        float size = mods::minimapSize;
        float halfSize = size / 2.0f;
        ImVec2 mapCenter = ImVec2(mods::minimapPos.x + halfSize, mods::minimapPos.y + halfSize);

        float dx = enemyPos.X - localPos.X;
        float dy = enemyPos.Y - localPos.Y;

        float yawRad = -localRot.Yaw * (3.14159265f / 180.0f);
        float rx = dx * cosf(yawRad) - dy * sinf(yawRad);
        float ry = dx * sinf(yawRad) + dy * cosf(yawRad);

        float range = 5000.0f / mods::minimapZoom;
        float scale = halfSize / range;
        rx *= scale;
        ry *= scale;

        // Clamp to minimap bounds
        rx = fmaxf(-halfSize + 3, fminf(halfSize - 3, rx));
        ry = fmaxf(-halfSize + 3, fminf(halfSize - 3, ry));

        ImVec2 dotPos = ImVec2(mapCenter.x + rx, mapCenter.y + ry);

        // Color by elevation: above = lighter, below = darker
        ImU32 color;
        if (elevationDiff > 200.0f)
            color = IM_COL32(255, 150, 150, 255); // Above
        else if (elevationDiff < -200.0f)
            color = IM_COL32(180, 50, 50, 255); // Below
        else
            color = IM_COL32(255, 60, 60, 255); // Same level

        drawList->AddCircleFilled(dotPos, 3.5f, color);
    }

    // ============================================================
    //  MASTER DRAW FUNCTIONS (called from DrawTransition)
    // ============================================================

    inline void DrawOverlayWidgets(ImDrawList* drawList, ImFont* font, ImVec2 screenCenter,
        SDK::FVector localPos, SDK::FRotator localRot) {
        DrawDamagePopups(drawList, font);
        DrawTeamCompAnalyzer(drawList);
        DrawSessionStats(drawList);
        DrawMatchTimer(drawList);
        DrawHotkeySheet(drawList);
        DrawObjectiveTimer(drawList);
        DrawRecoilDisplay(drawList, screenCenter);
        DrawEnhancedMinimap(drawList, localPos, localRot);
        DrawSpawnTimers(drawList, ImVec2(screenCenter.x + 400, 50));
    }

    inline void PerFrameLogic(SDK::AMarvelBaseCharacter* localChar, float closestDist) {
        CheckAutoMelee(closestDist);
        CheckCrouchSpam();
        if (localChar) {
            CheckAutoReload(localChar);
            UpdateHealerStats(localChar);
            CheckAutoHeal(localChar);
            CheckAutoUltHeal();
            CheckAutoShield(localChar);
            CheckAbilityRotation(localChar);
            UpdateHealTarget();
        }
    }
}
