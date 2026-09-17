#pragma once
#include <atomic>
#include <cmath>
#include <unordered_set>
#include <string>

namespace SilentAim {

    typedef void(*ProcessEvent_t)(void*, void*, void*);

    inline ProcessEvent_t oProcessEvent        = nullptr;
    inline void*          hookTargetAddr       = nullptr;
    inline bool           bHookEnabled         = false;
    inline bool           bHookInstalled       = false;

    inline void* cachedFireFunc       = nullptr;
    inline void* cachedFireServerFunc = nullptr;
    inline bool  bBothCached          = false;

    inline std::unordered_set<void*> checkedFunctions;

    inline std::atomic<int>      warmupBudget     { 0 };
    inline std::atomic<uint64_t> warmupWindowStart{ 0 };
    inline uint64_t              warmupCooldownUntil = 0;

    constexpr int      WARMUP_BUDGET       = 10;
    constexpr uint64_t WARMUP_WINDOW_MS    = 16;
    constexpr uint64_t WARMUP_COOLDOWN_MS  = 500;

    inline bool bProcessingFire = false;
    inline bool bWarmingUp      = false;

    // ---- Helper: safe pointer probe via SEH ----
    static bool ProbePointer(const void* ptr) {
        __try {
            volatile char c = *reinterpret_cast<const volatile char*>(ptr);
            (void)c;
            return true;
        }
        __except (1) {
            return false;
        }
    }

    // ---- Get name of a UFunction object ----
    static std::string GetFunctionName(void* func) {
        if (!ProbePointer(func)) return "";
        SDK::UObject* obj = reinterpret_cast<SDK::UObject*>(func);
        if (!ProbePointer(obj)) return "";
        return obj->GetName();
    }

    // ---- Aim bone matching local aimbot hitbox setting (cached) ----
    static FName GetAimBone() {
        static std::string s_lastHitbox;
        static SDK::FName  s_cachedBone;
        if (mods::aimHitbox == s_lastHitbox) return s_cachedBone;
        s_lastHitbox = mods::aimHitbox;
        const std::string& hb = mods::aimHitbox;
        if (hb == "Pelvis")     s_cachedBone = UKismetStringLibrary::Conv_StringToName(SDK::FString(L"pelvis"));
        else if (hb == "Neck")       s_cachedBone = UKismetStringLibrary::Conv_StringToName(SDK::FString(L"neck_01"));
        else if (hb == "Chest")      s_cachedBone = UKismetStringLibrary::Conv_StringToName(SDK::FString(L"spine_01"));
        else if (hb == "Left Hand")  s_cachedBone = UKismetStringLibrary::Conv_StringToName(SDK::FString(L"hand_l"));
        else if (hb == "Right Hand") s_cachedBone = UKismetStringLibrary::Conv_StringToName(SDK::FString(L"hand_r"));
        else                          s_cachedBone = UKismetStringLibrary::Conv_StringToName(SDK::FString(L"head"));
        return s_cachedBone;
    }

    // ---- Resolve target world position from cached TargetPlayerPTR ----
    static bool GetSilentAimTarget(FVector& outPos) {
        AMarvelBaseCharacter* target = Variables::TargetPlayerPTR;
        if (!target || !ProbePointer(target)) return false;

        USkeletalMeshComponent* mesh = reinterpret_cast<ACharacter*>(target)->GetMesh();
        if (!mesh || !ProbePointer(mesh)) return false;

        FVector pos = mesh->GetSocketLocation(GetAimBone());
        pos.Z += mods::aimOffset;

        if (!std::isfinite(pos.X) || !std::isfinite(pos.Y) || !std::isfinite(pos.Z))
            return false;

        FVector camLoc = Variables::CameraLocation;
        float dx = pos.X - camLoc.X;
        float dy = pos.Y - camLoc.Y;
        float dz = pos.Z - camLoc.Z;
        if (sqrtf(dx*dx + dy*dy + dz*dz) / 100.0f > mods::pSilentMaxDistance) return false;

        outPos = pos;
        return true;
    }

    // ---- Cached local player controller/pawn accessors ----
    static void* GetLocalPlayerController() {
        return reinterpret_cast<void*>(Variables::PlayerController);
    }

    static void* GetLocalPawn() {
        return reinterpret_cast<void*>(Variables::AcknowledgedPawn);
    }

    // ---- Camera view from cached Variables ----
    static void GetCameraView(FVector& outLoc, FRotator& outRot) {
        outLoc = Variables::CameraLocation;
        outRot = Variables::CameraRotation;
    }

    // ---- PlayerController rotation accessors ----
    static FRotator GetControlRotation(void* pc) {
        return reinterpret_cast<APlayerController*>(pc)->GetControlRotation();
    }

    static void SetControlRotation(void* pc, const FRotator& rot) {
        reinterpret_cast<APlayerController*>(pc)->SetControlRotation(rot);
    }

    // ---- Is the configured aim key held? ----
    static bool IsAimKeyHeld() {
        if (!mods::bPSilentEnabled) return false;
        int key = mods::bPSilentUseAimbotKey ? mods::aimbotKey : mods::pSilentKey;
        return (GetAsyncKeyState(key) & 0x8000) != 0;
    }

    // ---- Angular distance check (degrees) ----
    static bool IsWithinFOV(const FVector& camLoc, const FRotator& camRot,
                             const FVector& targetPos, float maxDeg) {
        FRotator aimRot = UKismetMathLibrary::FindLookAtRotation(camLoc, targetPos);
        FRotator delta  = UKismetMathLibrary::NormalizedDeltaRotator(aimRot, camRot);
        float dist = sqrtf(delta.Pitch * delta.Pitch + delta.Yaw * delta.Yaw);
        return dist <= maxDeg;
    }

    // ---- Is this object owned by the local player's pawn? ----
    static bool IsLocalPlayerFire(void* object) {
        __try {
            void* localPawn = GetLocalPawn();
            void* localPC   = GetLocalPlayerController();
            if (!object || (!localPawn && !localPC)) return false;
            SDK::UObject* cur = reinterpret_cast<SDK::UObject*>(object);
            for (int i = 0; i < 6; i++) {
                if (!cur || !ProbePointer(cur)) return false;
                if (localPawn && cur == reinterpret_cast<SDK::UObject*>(localPawn)) return true;
                if (localPC   && cur == reinterpret_cast<SDK::UObject*>(localPC))   return true;
                cur = cur->Outer;
            }
            return false;
        }
        __except (1) {
            return false;
        }
    }

    // ---- Try to identify and cache a function pointer ----
    //  0 = not a fire func (add to negative cache)
    //  1 = HandleFireWithParams
    //  2 = HandleFireWithParamsServerOnly
    // -1 = bad pointer (skip)
    static int TryResolveFunc(void* func) {
        if (!ProbePointer(func)) return -1;
        std::string name = GetFunctionName(func);
        if (name == "HandleFireWithParams")            return 1;
        if (name == "HandleFireWithParamsServerOnly")  return 2;
        return 0;
    }

    // ---- Main hook logic (C++ objects OK here, no __try) ----
    static void hkProcessEvent_impl(void* object, void* function, void* parms) {
        if (!GetLocalPawn()) {
            oProcessEvent(object, function, parms);
            return;
        }

        bool isFireFunc = (function == cachedFireFunc || function == cachedFireServerFunc);

        if (isFireFunc) {
            if (bProcessingFire) {
                oProcessEvent(object, function, parms);
                return;
            }

            if (!parms || !IsLocalPlayerFire(object)) {
                oProcessEvent(object, function, parms);
                return;
            }

            if (IsAimKeyHeld()) {
                FVector targetPos;
                if (GetSilentAimTarget(targetPos)) {
                    FVector camLoc;
                    FRotator camRot;
                    GetCameraView(camLoc, camRot);

                    if (IsWithinFOV(camLoc, camRot, targetPos, mods::pSilentFov)) {
                        void* pc = GetLocalPlayerController();
                        if (pc) {
                            FRotator aimRot = UKismetMathLibrary::FindLookAtRotation(camLoc, targetPos);

                            if (!std::isfinite(aimRot.Pitch) || !std::isfinite(aimRot.Yaw) || !std::isfinite(aimRot.Roll)) {
                                oProcessEvent(object, function, parms);
                                return;
                            }

                            uintptr_t base = reinterpret_cast<uintptr_t>(parms);
                            *reinterpret_cast<bool*>    (base + (uintptr_t)mods::pSilentOffsetUseCustomTarget) = true;
                            *reinterpret_cast<FVector*> (base + (uintptr_t)mods::pSilentOffsetViewLocation)    = camLoc;
                            *reinterpret_cast<FRotator*>(base + (uintptr_t)mods::pSilentOffsetViewRotation)    = aimRot;
                            *reinterpret_cast<FVector*> (base + (uintptr_t)mods::pSilentOffsetTargetLocation)  = targetPos;

                            FRotator origRot = GetControlRotation(pc);
                            SetControlRotation(pc, aimRot);

                            bProcessingFire = true;
                            oProcessEvent(object, function, parms);
                            bProcessingFire = false;

                            SetControlRotation(pc, origRot);
                            return;
                        }
                    }
                }
            }
        }
        else if (!bBothCached && function) {
            uint64_t now = GetTickCount64();
            if (now >= warmupCooldownUntil) {
                if (checkedFunctions.find(function) == checkedFunctions.end()) {
                    uint64_t wStart = warmupWindowStart.load(std::memory_order_relaxed);
                    if (now - wStart > WARMUP_WINDOW_MS) {
                        warmupWindowStart.store(now, std::memory_order_relaxed);
                        warmupBudget.store(WARMUP_BUDGET, std::memory_order_relaxed);
                    }
                    if (warmupBudget.fetch_sub(1, std::memory_order_relaxed) > 0) {
                        int result = TryResolveFunc(function);
                        if (result == 1) {
                            cachedFireFunc = function;
                        }
                        else if (result == 2) {
                            cachedFireServerFunc = function;
                        }
                        else if (result == 0) {
                            checkedFunctions.insert(function);
                            if (checkedFunctions.size() > 10000) checkedFunctions.clear();
                        }
                        if (cachedFireFunc && cachedFireServerFunc) {
                            bBothCached = true;
                            bWarmingUp  = false;
                        }
                    }
                }
            }
            bWarmingUp = !bBothCached;
        }

        oProcessEvent(object, function, parms);
    }

    // ---- SEH crash wrapper — no C++ objects, safe for __try ----
    static void hkProcessEvent(void* object, void* function, void* parms) {
        __try {
            hkProcessEvent_impl(object, function, parms);
        }
        __except (1) {
            oProcessEvent(object, function, parms);
        }
    }

    // ---- Install hook (call once) ----
    inline bool Install() {
        uintptr_t addr = InSDKUtils::GetImageBase() + SDK::Offsets::ProcessEvent;
        hookTargetAddr = reinterpret_cast<void*>(addr);
        if (MH_CreateHook(hookTargetAddr, reinterpret_cast<void*>(&hkProcessEvent),
                          reinterpret_cast<void**>(&oProcessEvent)) != MH_OK)
            return false;
        bHookInstalled = true;
        bHookEnabled   = false;
        return true;
    }

    inline void EnableHook() {
        if (!bHookInstalled || bHookEnabled || !hookTargetAddr) return;
        MH_EnableHook(hookTargetAddr);
        bHookEnabled = true;
        bWarmingUp   = !bBothCached;
    }

    inline void DisableHook() {
        if (!bHookInstalled || !bHookEnabled || !hookTargetAddr) return;
        MH_DisableHook(hookTargetAddr);
        bHookEnabled = false;
        bWarmingUp   = false;
    }

    // ---- Call once per frame to sync enabled state ----
    inline void UpdateHookState() {
        if (!bHookInstalled) return;
        if (mods::bPSilentEnabled && !bHookEnabled) EnableHook();
        else if (!mods::bPSilentEnabled && bHookEnabled) DisableHook();
    }

    // ---- Call when world/game changes to invalidate cached UFunction ptrs ----
    inline void ResetCache() {
        cachedFireFunc       = nullptr;
        cachedFireServerFunc = nullptr;
        bBothCached          = false;
        checkedFunctions.clear();
        warmupCooldownUntil = GetTickCount64() + WARMUP_COOLDOWN_MS;
        warmupBudget.store(0, std::memory_order_relaxed);
        bWarmingUp = bHookEnabled;
    }

} // namespace SilentAim
