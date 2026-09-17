#pragma once
#include "../Custom/Custom.h"
#include <float.h>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <limits>
#include <cmath>
#include <xlocbuf>
#include <codecvt>
#include "../Custom/VTableHook.h"
#include "../Custom/ComboSystem.h"
#include "../Custom/AutoKeySystem.h"
#include "../Custom/SilentAim.h"
#include "../../global.h"
#include "../TriggerControlShared.h"

// Must be defined before Exploits.h / Features.h which depend on it
inline bool IsValid(const UObject* Test) {
    return Test != nullptr;
}

// SEH helper: isolated from C++ unwindable objects so __try is valid here
static bool SafeGetHP(SDK::AMarvelBaseCharacter* ch, float* outCur, float* outMax) {
    __try {
        *outCur = ch->GetCurrentHealth();
        *outMax = ch->GetMaxHealth();
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

#undef min
#undef max

#include "../Exploits/Exploits.h"
#include "../Features/Features.h"
# define M_PI           3.14159265358979323846  /* pi */
#define IsKeyHeld(key) (GetAsyncKeyState(key) & 0x8000)

bool debug = false;
Hook::VTableHook PRHook;    

void Log(const char* format, ...) {
    if (!debug) return;
    va_list args;
    va_start(args, format);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    printf("%s\n", buffer);
}

FVector GetBoneLocation(ACharacter* Player, SDK::FString BoneName) {
    if (!Player || !IsValid(Player) || !Player->GetMesh() || !IsValid(Player->GetMesh())) {
        Log("GetBoneLocation: Player or GetMesh() is invalid\n");
        return FVector();
    }

    USkeletalMeshComponent* mesh = Player->GetMesh();
    FName BoneFName = UKismetStringLibrary::Conv_StringToName(BoneName);
    FVector Location = mesh->GetSocketLocation(BoneFName);
    return Location;
}

FName GetAimBoneName() {
    // PUBG-style: use individual bone booleans (priority: Head > Neck > Chest > Pelvis > Hands)
    static SDK::FName s_cachedFName;
    static int s_lastBoneHash = -1;
    int boneHash = (mods::bBoneHead) | (mods::bBoneNeck << 1) | (mods::bBoneChest << 2) |
                   (mods::bBonePelvis << 3) | (mods::bBoneLeftHand << 4) | (mods::bBoneRightHand << 5);
    if (boneHash == s_lastBoneHash) return s_cachedFName;
    s_lastBoneHash = boneHash;

    if      (mods::bBoneHead)      s_cachedFName = UKismetStringLibrary::Conv_StringToName(SDK::FString(L"head"));
    else if (mods::bBoneNeck)      s_cachedFName = UKismetStringLibrary::Conv_StringToName(SDK::FString(L"neck_01"));
    else if (mods::bBoneChest)     s_cachedFName = UKismetStringLibrary::Conv_StringToName(SDK::FString(L"spine_01"));
    else if (mods::bBonePelvis)    s_cachedFName = UKismetStringLibrary::Conv_StringToName(SDK::FString(L"pelvis"));
    else if (mods::bBoneLeftHand)  s_cachedFName = UKismetStringLibrary::Conv_StringToName(SDK::FString(L"hand_l"));
    else if (mods::bBoneRightHand) s_cachedFName = UKismetStringLibrary::Conv_StringToName(SDK::FString(L"hand_r"));
    else                           s_cachedFName = UKismetStringLibrary::Conv_StringToName(SDK::FString(L"head")); // fallback
    return s_cachedFName;
}

AMarvelPlayerController* GetLocalPlayerController(UWorld* World)
{
    if (!World) return nullptr;

    if (AMarvelPlayerController* PlayerController = reinterpret_cast<AMarvelPlayerController*>(UGameplayStatics::GetPlayerController(World, 0)))
    {
        return PlayerController;
    }

    if (AMarvelPlayerController* PlayerController = reinterpret_cast<AMarvelPlayerController*>(UGameplayStatics::GetPlayerControllerFromID(World, 0)))
    {
        return PlayerController;
    }

    return nullptr;
}

namespace TriggerOnly {
    inline volatile LONG Status = 0;
    inline HANDLE SharedMapping = nullptr;
    inline TriggerControlShared::State* SharedState = nullptr;

    inline TriggerControlShared::State* GetSharedState() {
        if (SharedState) return SharedState;
        SharedMapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, TriggerControlShared::MappingName);
        if (!SharedMapping) return nullptr;
        SharedState = static_cast<TriggerControlShared::State*>(MapViewOfFile(
            SharedMapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(TriggerControlShared::State)));
        if (!SharedState || InterlockedCompareExchange(&SharedState->magic, 0, 0) != TriggerControlShared::Magic) {
            if (SharedState) UnmapViewOfFile(SharedState);
            SharedState = nullptr;
            CloseHandle(SharedMapping);
            SharedMapping = nullptr;
        }
        return SharedState;
    }

    inline void SetStatus(LONG value) {
        InterlockedExchange(&Status, value);
        if (auto* shared = GetSharedState()) InterlockedExchange(&shared->status, value);
    }

    inline const wchar_t* StatusText() {
        switch (InterlockedCompareExchange(&Status, 0, 0)) {
        case 0:  return L"Status: waiting";
        case 1:  return L"Status: triggerbot off";
        case 2:  return L"Status: engine unavailable";
        case 3:  return L"Status: viewport unavailable";
        case 4:  return L"Status: world unavailable";
        case 5:  return L"Status: controller unavailable";
        case 6:  return L"Status: camera or pawn unavailable";
        case 7:  return L"Status: character class unavailable";
        case 8:  return L"Status: trace missed";
        case 9:  return L"Status: trace hit non-character";
        case 10: return L"Status: local player, teammate, or dead target";
        case 11: return L"Status: click sent";
        case 12: return L"Status: pawn unavailable";
        case 13: return L"Status: input rejected";
        case 14: return L"Status: click requested externally";
        default: return L"Status: unknown";
        }
    }

    inline bool RequestExternalLeftClick() {
        auto* shared = GetSharedState();
        // Insert/AHK owns the master switch; no C++ path may request a click
        // while the assist is disabled.
        if (!shared || !InterlockedCompareExchange(&shared->triggerEnabled, 0, 0) ||
            !TriggerControlShared::RequestLeftClick(*shared)) {
            SetStatus(13);
            return false;
        }
        SetStatus(14);
        return true;
    }

    struct Frame {
        LONG status = 0;
        LONG x = 0, y = 0, aim = 0, fire = 0;
    };

    static void EaseControllerTowardHead(AMarvelPlayerController* controller,
                                         const FVector& cameraLocation,
                                         const FVector& headWorld,
                                         double screenDistancePx, double fovPx,
                                         LONG configuredSmoothing) {
        // The working source uses game control rotation. Apply only a bounded
        // fraction of the shortest pitch/yaw error per detector update.
        const FRotator current = controller->GetControlRotation();
        const FRotator desired = UKismetMathLibrary::FindLookAtRotation(cameraLocation, headWorld);
        const FRotator delta = UKismetMathLibrary::NormalizedDeltaRotator(desired, current);
        const double fraction = (std::min)(1.0, screenDistancePx / (std::max)(5.0, fovPx));
        const double smoothing = (std::max)(0L, (std::min)(100L, configuredSmoothing));
        const double speed = 1.35 - smoothing / 100.0;
        const double variation = 0.96 + (GetTickCount64() % 9) / 100.0;
        const double gain = (0.12 + 0.08 * fraction) * speed * variation;
        const double maxDegrees = (0.20 + 0.45 * fraction) * speed;
        const auto step = [gain, maxDegrees](double error) {
            if (std::abs(error) < 0.02) return 0.0;
            return (std::max)(-maxDegrees, (std::min)(maxDegrees, error * gain));
        };
        const double pitch = step(delta.Pitch), yaw = step(delta.Yaw);
        if (pitch != 0.0 || yaw != 0.0)
            controller->SetControlRotation(FRotator(current.Pitch + pitch,
                                                    current.Yaw + yaw, 0));
    }

    static bool IsReadableObjectPointer(const void* pointer) {
        MEMORY_BASIC_INFORMATION info{};
        return pointer && VirtualQuery(pointer, &info, sizeof(info)) == sizeof(info) &&
            info.State == MEM_COMMIT && !(info.Protect & PAGE_NOACCESS) &&
            !(info.Protect & PAGE_GUARD);
    }

    static bool ResolveMeshHead(USkeletalMeshComponent* mesh, const FName& headName,
                                FVector* outHead, LONG* failure) {
        __try {
            // Match the known-working source's path: GetMesh() already yields
            // the character's skeletal mesh. Extra reflection checks here had
            // been rejecting that valid object before the Head lookup.
            if (!mesh)
                return false;
            *failure = 22;
            // Unreal distinguishes named sockets from bones. "Head" may be
            // a socket on one hero and only the exact skeletal bone on another.
            // Both branches use the same FName and world-space point.
            FVector head{};
            if (mesh->DoesSocketExist(headName)) {
                head = mesh->GetSocketLocation(headName);
            } else {
                if (mesh->GetBoneIndex(headName) < 0) return false;
                head = mesh->GetBoneTransform(
                    headName, ERelativeTransformSpace::RTS_World).Translation;
            }
            if (!std::isfinite(head.X) || !std::isfinite(head.Y) || !std::isfinite(head.Z))
                return false;
            *outHead = head;
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            *failure = 32;
            return false;
        }
    }

    static bool ResolveInheritedHead(AMarvelBaseCharacter* target, const FName& headName,
                                     FVector* outHead, LONG* failure) {
        __try {
            return ResolveMeshHead(target->GetMesh(), headName, outHead, failure);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            *failure = 32;
            return false;
        }
    }

    static bool ResolveOffsetHead(AMarvelBaseCharacter* target, const FName& headName,
                                  FVector* outHead, LONG* failure) {
        // The current Marvel SDK places ACharacter::DefaultMeshComponent at
        // 0x1D40. Keep this fallback isolated because older dumps used 0x1C08.
        constexpr uintptr_t offsets[] = { 0x1D40, 0x1C08 };
        __try {
            for (const auto offset : offsets) {
                auto* mesh = *reinterpret_cast<USkeletalMeshComponent**>(
                    reinterpret_cast<uintptr_t>(target) + offset);
                if (ResolveMeshHead(mesh, headName, outHead, failure)) return true;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            *failure = 32;
        }
        return false;
    }

    static bool ResolveExactHeadSocket(AMarvelBaseCharacter* target,
                                       const FName& headName,
                                       FVector* outHead, LONG* failure) {
        __try {
            if (!target || !outHead) return false;
            *failure = 31;
            auto* meshClass = USkeletalMeshComponent::StaticClass();
            if (!meshClass || !target->Class ||
                !target->Class->GetFunction("Actor", "K2_GetComponentsByClass")) return false;
            // Retain the engine-owned output allocation between queries.
            struct ComponentParams {
                TSubclassOf<UActorComponent> componentClass;
                TArray<UActorComponent*> components;
            };
            static_assert(sizeof(ComponentParams) == 0x18);
            static ComponentParams params{};
            params.componentClass = meshClass;
            auto* function = target->Class->GetFunction("Actor", "K2_GetComponentsByClass");
            const auto flags = function->FunctionFlags;
            function->FunctionFlags |= 0x400;
            __try {
                target->ProcessEvent(function, &params);
            } __finally {
                function->FunctionFlags = flags;
            }
            *failure = 30;
            const auto& components = params.components;
            if (!components.IsValid() || components.Num() <= 0 || components.Num() > 128) {
                if (ResolveInheritedHead(target, headName, outHead, failure)) return true;
                return ResolveOffsetHead(target, headName, outHead, failure);
            }
            *failure = 22;
            for (int i = 0; i < components.Num(); ++i) {
                auto* mesh = static_cast<USkeletalMeshComponent*>(components[i]);
                if (ResolveMeshHead(mesh, headName, outHead, failure)) return true;
            }
            if (ResolveInheritedHead(target, headName, outHead, failure)) return true;
            return ResolveOffsetHead(target, headName, outHead, failure);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            *failure = 32;
            return ResolveOffsetHead(target, headName, outHead, failure);
        }
    }

    inline const TArray<AActor*>* QueryCharacters(UWorld* world, UClass* characterClass) {
        // Layout verified against Engine_parameters.hpp. Retain the engine's
        // output buffer between calls: the generated wrapper discards it.
        struct QueryParams {
            const UObject* world;
            TSubclassOf<AActor> actorClass;
            TArray<AActor*> actors;
        };
        static_assert(sizeof(QueryParams) == 0x20);
        static QueryParams params{};
        auto* klass = UGameplayStatics::StaticClass();
        if (!klass) return nullptr;
        auto* function = klass->GetFunction("GameplayStatics", "GetAllActorsOfClass");
        auto* object = UGameplayStatics::GetDefaultObj();
        if (!function || !object) return nullptr;
        params.world = world;
        params.actorClass = characterClass;
        const auto flags = function->FunctionFlags;
        function->FunctionFlags |= 0x400;
        object->ProcessEvent(function, &params);
        function->FunctionFlags = flags;
        return &params.actors;
    }

    inline bool ProjectWithCamera(const FVector& worldPosition,
                                  const FVector& cameraPosition,
                                  const FRotator& cameraRotation,
                                  double horizontalFovDegrees,
                                  int32 width,
                                  int32 height,
                                  SDK::FVector2D& screen) {
        if (width <= 0 || height <= 0 || !std::isfinite(horizontalFovDegrees) ||
            horizontalFovDegrees <= 1.0 || horizontalFovDegrees >= 179.0)
            return false;

        constexpr double degreesToRadians = 3.14159265358979323846 / 180.0;
        const double pitch = cameraRotation.Pitch * degreesToRadians;
        const double yaw = cameraRotation.Yaw * degreesToRadians;
        const double roll = cameraRotation.Roll * degreesToRadians;
        const double sp = std::sin(pitch), cp = std::cos(pitch);
        const double sy = std::sin(yaw), cy = std::cos(yaw);
        const double sr = std::sin(roll), cr = std::cos(roll);

        // Unreal's rotation matrix axes: X forward, Y right, Z up.
        const FVector forward(cp * cy, cp * sy, sp);
        const FVector right(sr * sp * cy - cr * sy,
                            sr * sp * sy + cr * cy,
                            -sr * cp);
        const FVector up(-(cr * sp * cy + sr * sy),
                         cy * sr - cr * sp * sy,
                         cr * cp);
        const FVector delta = worldPosition - cameraPosition;
        const auto dot = [](const FVector& a, const FVector& b) {
            return a.X * b.X + a.Y * b.Y + a.Z * b.Z;
        };
        const double depth = dot(delta, forward);
        if (!std::isfinite(depth) || depth <= 1.0) return false;
        const double focal = (static_cast<double>(width) * 0.5) /
            std::tan(horizontalFovDegrees * degreesToRadians * 0.5);
        screen.X = static_cast<double>(width) * 0.5 + dot(delta, right) * focal / depth;
        screen.Y = static_cast<double>(height) * 0.5 - dot(delta, up) * focal / depth;
        return std::isfinite(screen.X) && std::isfinite(screen.Y);
    }

    inline void DetectFrame(TriggerControlShared::State& shared, Frame& frame) {
        if (!InterlockedCompareExchange(&shared.triggerEnabled, 0, 0)) {
            frame.status = 1;
            return;
        }
        UEngine* engine = UEngine::GetEngine();
        if (!engine) { frame.status = 2; return; }
        if (!engine->GameViewport) { frame.status = 3; return; }
        UWorld* world = engine->GameViewport->World;
        if (!world || !IsValid(world)) { frame.status = 4; return; }
        auto* controller = GetLocalPlayerController(world);
        if (!controller || !IsValid(controller)) { frame.status = 5; return; }
        auto* camera = controller->PlayerCameraManager;
        auto* pawn = controller->AcknowledgedPawn;
        if (!camera || !IsValid(camera)) { frame.status = 6; return; }
        if (!pawn || !IsValid(pawn)) { frame.status = 12; return; }
        auto* characterClass = AMarvelBaseCharacter::StaticClass();
        if (!characterClass) { frame.status = 7; return; }
        auto* localState = static_cast<AMarvelPlayerState*>(pawn->PlayerState);
        const FVector cameraLocation = camera->GetCameraLocation();

        // Firing is its own detector. A blocking camera trace must resolve to
        // a valid, live enemy; aim acquisition and aim FOV do not gate it.
        const FRotator cameraRotation = camera->GetCameraRotation();
        const FVector traceEnd = cameraLocation +
            UKismetMathLibrary::GetForwardVector(cameraRotation) *
            (mods::TriggerBotDistance > 0.f ? mods::TriggerBotDistance : 999999.f);
        FHitResult hit{};
        static TArray<AActor*> ignoredActors;
        if (UKismetSystemLibrary::LineTraceSingle(world, cameraLocation, traceEnd,
            ETraceTypeQuery::TraceTypeQuery3, false, ignoredActors,
            EDrawDebugTrace::None, &hit, true, FLinearColor(), FLinearColor(), 0.f)
            && hit.bBlockingHit) {
            auto* hitComponent = hit.Component.Get();
            auto* hitActor = hitComponent ? hitComponent->GetOwner() : nullptr;
            if (hitActor && hitActor != pawn && hitActor->IsA(characterClass)) {
                auto* hitTarget = reinterpret_cast<AMarvelBaseCharacter*>(hitActor);
                auto* hitState = static_cast<AMarvelPlayerState*>(hitTarget->PlayerState);
                const bool enemy = hitState && localState &&
                    hitState->OriginalTeamID != localState->OriginalTeamID;
                if (enemy && !hitTarget->IsLocallyControlled() && hitTarget->GetCurrentHealth() > 0.f)
                    frame.fire = 1;
            }
        }

        int32 width = 0, height = 0;
        controller->GetViewportSize(&width, &height);
        if (width <= 0 || height <= 0) { frame.status = 3; return; }

        // The active characters are distributed across streamed levels in the
        // trigger-only build, so query them globally rather than relying on
        // PersistentLevel->Actors.
        const auto* actors = QueryCharacters(world, characterClass);
        if (!actors) { frame.status = 27; return; }
        if (actors->Num() == 0) { frame.status = 20; return; }
        if (!actors->IsValid() || actors->Num() > 4096) { frame.status = 28; return; }
        struct Candidate {
            AMarvelBaseCharacter* player;
            SDK::FVector2D headScreen;
            FVector headWorld;
            double distance;
            bool visible;
        };
        std::vector<Candidate> candidates;
        candidates.reserve(static_cast<size_t>(actors->Num()));
        static FName HeadBoneName = UKismetStringLibrary::Conv_StringToName(SDK::FString(L"Head"));
        const LONG configuredFov = InterlockedCompareExchange(&shared.aimFovPx, 0, 0);
        const double fov = (std::max)(5L, (std::min)(300L, configuredFov));
        const LONG configuredHeight = InterlockedCompareExchange(&shared.aimHeightPct, 0, 0);
        const double heightFraction = (std::max)(75L, (std::min)(100L, configuredHeight)) / 100.0;
        const LONG configuredSmoothing = InterlockedCompareExchange(&shared.aimSmoothing, 0, 0);
        const double cx = width / 2.0, cy = height / 2.0;
        const FVector localPosition = pawn->K2_GetActorLocation();
        int characters = 0, enemies = 0, heads = 0, projected = 0, visible = 0, inFov = 0;
        LONG headFailure = 22;
        for (int i = 0; i < actors->Num(); ++i) {
            auto* actor = (*actors)[i];
            if (!actor || actor == pawn || !actor->IsA(characterClass)) continue;
            ++characters;
            auto* target = reinterpret_cast<AMarvelBaseCharacter*>(actor);
            if (target->IsLocallyControlled() || target->GetCurrentHealth() <= 0.f) continue;
            auto* state = static_cast<AMarvelPlayerState*>(target->PlayerState);
            if (!state || !localState ||
                state->OriginalTeamID == localState->OriginalTeamID) continue;
            ++enemies;
            FVector headWorld{};
            if (!ResolveExactHeadSocket(target, HeadBoneName, &headWorld, &headFailure)) {
                // Fallback for heroes whose runtime mesh does not expose the
                // Head name: retain the working actor-location reference and
                // move upward using the character capsule height. Render
                // bounds can include offsets/child meshes and project away
                // from the actual target; the capsule is centered on the pawn.
                const FVector actorLocation = target->K2_GetActorLocation();
                float halfHeight = 88.0f;
                if (target->CapsuleComponent)
                    halfHeight = target->CapsuleComponent->GetScaledCapsuleHalfHeight();
                if (!std::isfinite(halfHeight) || halfHeight < 1.0f || halfHeight > 500.0f)
                    halfHeight = 88.0f;
                const double verticalOffset = (heightFraction - 0.5) * 2.0 * halfHeight;
                headWorld = actorLocation + FVector(0.0, 0.0, verticalOffset);
                if (!std::isfinite(headWorld.X) || !std::isfinite(headWorld.Y) ||
                    !std::isfinite(headWorld.Z)) continue;
                headFailure = 33;
            }
            ++heads;
            SDK::FVector2D screen{};
            // This is the game's viewport/client coordinate system. AHK turns
            // this into a relative offset from the viewport centre.
            if (!controller->ProjectWorldLocationToScreen(headWorld, &screen, true) ||
                !std::isfinite(screen.X) || !std::isfinite(screen.Y) ||
                screen.X < 0 || screen.Y < 0 || screen.X >= width || screen.Y >= height) continue;
            ++projected;
            const bool isVisible = controller->LineOfSightTo(target, cameraLocation, false);
            if (isVisible) ++visible;
            const double dx = screen.X - cx, dy = screen.Y - cy;
            if (dx * dx + dy * dy > fov * fov) continue;
            ++inFov;
            const double distance = UKismetMathLibrary::Vector_Distance(localPosition, target->K2_GetActorLocation());
            candidates.push_back({ target, screen, headWorld, distance, isVisible });
        }
        // Stable ordering gives a deterministic result for equal distances:
        // visible targets first, then closest world distance. Health and
        // screen/crosshair distance deliberately play no part in ranking.
        std::stable_sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
            if (a.visible != b.visible) return a.visible > b.visible;
            return a.distance < b.distance;
        });
        if (!candidates.empty()) {
            const Candidate& selected = candidates.front();
            const double dx = selected.headScreen.X - cx;
            const double dy = selected.headScreen.Y - cy;
            if (selected.visible && dx * dx + dy * dy <= fov * fov) {
                frame.x = static_cast<LONG>(std::lround(selected.headScreen.X));
                frame.y = static_cast<LONG>(std::lround(selected.headScreen.Y));
                frame.aim = 1;
#ifdef INTERNAL_AIM_ONLY
                EaseControllerTowardHead(controller, cameraLocation, selected.headWorld,
                                         std::sqrt(dx * dx + dy * dy), fov,
                                         configuredSmoothing);
#endif
            }
        }
        frame.status = frame.aim ? 25 : !characters ? 20 : !enemies ? 21 :
            !heads ? headFailure : !projected ? 26 : !visible ? 23 : !inFov ? 24 : 24;
    }

    inline void Tick() {
        auto* shared = GetSharedState();
        if (!shared) return;
        static ULONGLONG lastTick = 0;
        const ULONGLONG now = GetTickCount64();
        if (now - lastTick < 8) return;
        lastTick = now;
        Frame frame{};
        try {
            DetectFrame(*shared, frame);
        } catch (...) {
            frame = Frame{};
            frame.status = 29;
        }

        // The two channels are independent: fire advances clickRequest, while
        // a complete selected-head publication advances aimUpdate.
        if (frame.aim) {
#ifdef INTERNAL_AIM_ONLY
            // Internal variant consumes the selected world-space Head above;
            // do not also send coordinates to AHK for a second correction.
            InterlockedExchange(&shared->aimActive, 0);
#else
            InterlockedExchange(&shared->aimX, frame.x);
            InterlockedExchange(&shared->aimY, frame.y);
            InterlockedExchange(&shared->aimActive, 1);
            InterlockedIncrement(&shared->aimUpdate);
#endif
        } else {
            InterlockedExchange(&shared->aimActive, 0);
        }
        InterlockedExchange(&shared->status, frame.status);
        InterlockedExchange(&Status, frame.status);
        if (frame.fire) TriggerControlShared::RequestLeftClick(*shared);
    }
}

FVector PredictTargetPosition(AMarvelBaseCharacter* Target, float ProjectileSpeed) {
    if (!Target || !IsValid(Target) || !Target->GetMesh() || !IsValid(Target->GetMesh())) {
        Log("PredictTargetPosition: Target or Mesh is invalid\n");
        return FVector();
    }

    FVector TargetVelocity = Target->GetVelocity();
    FName BoneFName = GetAimBoneName();
    if (!Target->GetMesh()->DoesSocketExist(BoneFName)) {
        Log("PredictTargetPosition: Selected bone does not exist\n");
        return FVector();
    }

    FVector TargetPosition = Target->GetMesh()->GetSocketLocation(BoneFName);
    FVector ShooterPosition = Variables::CameraLocation;

    float distance = UKismetMathLibrary::Vector_Distance(ShooterPosition, TargetPosition);
    float timeToHit = distance / ProjectileSpeed;

    FVector PredictedPosition = TargetPosition + (TargetVelocity * timeToHit);
    return PredictedPosition;
}

bool CheckIfPlayer(AActor* Player, UClass* PlayerClass) {
    if (Player && PlayerClass) {
        if (Player->IsA(PlayerClass)) return true;
    }
    return false;
}

bool IsPointInAimbotCircle(float radius, SDK::FVector2D point) {
    ImVec2 screenCenter = ImVec2(ImGui::GetIO().DisplaySize.x / 2, ImGui::GetIO().DisplaySize.y / 2);
    float dx = point.X - screenCenter.x;
    float dy = point.Y - screenCenter.y;
    return (dx * dx + dy * dy) <= (radius * radius);
}

ImVec2 NormalizeImVec2(const ImVec2& vec) {
    float mag = sqrt(vec.x * vec.x + vec.y * vec.y);
    if (mag > 0.0f) {
        return ImVec2(vec.x / mag, vec.y / mag);
    }
    return ImVec2(0.0f, 0.0f);
}
inline UObject* (*StaticLoadObject)(UClass* ObjectClass, UObject* Outer, const wchar_t* Name, const wchar_t* FileName, uint32_t LoadFlags, uintptr_t Sandbox, bool bAllowObjectReconciliation, const uintptr_t* InstancingContex);

static const std::vector<std::pair<std::wstring, std::wstring>> boneConnections = {
    { L"spine_01", L"pelvis" },
    { L"neck_01", L"spine_01" },
    { L"head", L"neck_01" },
    { L"upperarm_l", L"spine_01" },
    { L"lowerarm_l", L"upperarm_l" },
    { L"hand_l", L"lowerarm_l" },
    { L"upperarm_r", L"spine_01" },
    { L"lowerarm_r", L"upperarm_r" },
    { L"hand_r", L"lowerarm_r" },
    { L"thigh_l", L"pelvis" },
    { L"calf_l", L"thigh_l" },
    { L"foot_l", L"calf_l" },
    { L"thigh_r", L"pelvis" },
    { L"calf_r", L"thigh_r" },
    { L"foot_r", L"calf_r" }
};

// Cache FName pairs once — avoids FString heap allocations every frame
struct BonePair { SDK::FName bone; SDK::FName parent; };
static std::vector<BonePair> s_bonePairs;
void DrawSkeleton(ImDrawList* drawList, SDK::AMarvelBaseCharacter* marvelChar, APlayerController* controller) {
    if (!marvelChar || !controller) return;

    SDK::USkeletalMeshComponent* mesh = marvelChar->GetMesh();
    if (!mesh) return;

    if (s_bonePairs.empty()) {
        for (const auto& [b, p] : boneConnections) {
            BonePair bp;
            bp.bone   = UKismetStringLibrary::Conv_StringToName(SDK::FString(b.c_str()));
            bp.parent = p.empty() ? SDK::FName() : UKismetStringLibrary::Conv_StringToName(SDK::FString(p.c_str()));
            s_bonePairs.push_back(bp);
        }
    }

    for (const auto& bp : s_bonePairs) {
        if (!mesh->DoesSocketExist(bp.bone)) continue;

        SDK::FVector boneWorldPos = mesh->GetSocketLocation(bp.bone);
        SDK::FVector2D boneScreenPos;
        if (!controller->ProjectWorldLocationToScreen(boneWorldPos, &boneScreenPos, true)) continue;

        if (mesh->DoesSocketExist(bp.parent)) {
            SDK::FVector parentBoneWorldPos = mesh->GetSocketLocation(bp.parent);
            SDK::FVector2D parentBoneScreenPos;
            if (controller->ProjectWorldLocationToScreen(parentBoneWorldPos, &parentBoneScreenPos, true)) {
                drawList->AddLine(
                    ImVec2(boneScreenPos.X, boneScreenPos.Y),
                    ImVec2(parentBoneScreenPos.X, parentBoneScreenPos.Y),
                    mods::skeletonESPColor);
            }
        }
    }
}

FVector GetVectorForward(const FRotator& angles)
{
    float sp, sy, cp, cy;
    float angle;

    angle = angles.Yaw * (M_PI / 180.0f);
    sy = sinf(angle);
    cy = cosf(angle);
    angle = -angles.Pitch * (M_PI / 180.0f);
    sp = sinf(angle);
    cp = cosf(angle);

    return { cp * cy, cp * sy, -sp };
}

void DrawCrosshair(ImDrawList* drawList, ImVec2 center) {
    if (mods::crosshairType == mods::NONE) return;

    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    center = ImVec2(displaySize.x / 2, displaySize.y / 2);
    ImU32 outlineCol = IM_COL32(0, 0, 0, 200);

    switch (mods::crosshairType) {
    case mods::DOT:
        if (mods::bCrosshairOutline)
            drawList->AddCircleFilled(center, mods::crosshairSize + 1.0f, outlineCol);
        drawList->AddCircleFilled(center, mods::crosshairSize, mods::crosshairColor);
        break;
    case mods::CROSS: {
        float s = mods::crosshairSize;
        float g = mods::crosshairGap;
        float t = mods::crosshairThickness;
        if (mods::bCrosshairOutline) {
            drawList->AddLine(ImVec2(center.x - s, center.y), ImVec2(center.x - g, center.y), outlineCol, t + 2.0f);
            drawList->AddLine(ImVec2(center.x + g, center.y), ImVec2(center.x + s, center.y), outlineCol, t + 2.0f);
            drawList->AddLine(ImVec2(center.x, center.y - s), ImVec2(center.x, center.y - g), outlineCol, t + 2.0f);
            drawList->AddLine(ImVec2(center.x, center.y + g), ImVec2(center.x, center.y + s), outlineCol, t + 2.0f);
        }
        drawList->AddLine(ImVec2(center.x - s, center.y), ImVec2(center.x - g, center.y), mods::crosshairColor, t);
        drawList->AddLine(ImVec2(center.x + g, center.y), ImVec2(center.x + s, center.y), mods::crosshairColor, t);
        drawList->AddLine(ImVec2(center.x, center.y - s), ImVec2(center.x, center.y - g), mods::crosshairColor, t);
        drawList->AddLine(ImVec2(center.x, center.y + g), ImVec2(center.x, center.y + s), mods::crosshairColor, t);
        break;
    }
    case mods::CIRCLE:
        if (mods::bCrosshairOutline)
            drawList->AddCircle(center, mods::crosshairSize, outlineCol, 0, mods::crosshairThickness + 2.0f);
        drawList->AddCircle(center, mods::crosshairSize, mods::crosshairColor, 0, mods::crosshairThickness);
        break;
    }
}

// Radar helper: plots enemy dots onto the radar widget
void UpdateRadarDots(ImDrawList* drawList, FVector localPos, FRotator localRot, FVector enemyPos) {
    if (!mods::bRadar) return;

    float dx = enemyPos.X - localPos.X;
    float dy = enemyPos.Y - localPos.Y;

    float yawRad = -localRot.Yaw * (M_PI / 180.0f);
    float rx = dx * cosf(yawRad) - dy * sinf(yawRad);
    float ry = dx * sinf(yawRad) + dy * cosf(yawRad);

    float scale = (mods::radarSize / 2.0f) / (mods::radarRange / mods::radarZoom);
    rx *= scale;
    ry *= scale;

    float halfSize = mods::radarSize / 2.0f;
    rx = fmaxf(-halfSize + 3, fminf(halfSize - 3, rx));
    ry = fmaxf(-halfSize + 3, fminf(halfSize - 3, ry));

    ImVec2 radarCenter(mods::radarPos.x + 10 + halfSize, mods::radarPos.y + 35 + halfSize);
    ImVec2 dotPos(radarCenter.x + rx, radarCenter.y + ry);

    if (mods::radarDesign == mods::CIRCULAR) {
        float dist = sqrtf(rx * rx + ry * ry);
        if (dist > halfSize - 3) return;
    }

    drawList->AddCircleFilled(dotPos, 3.0f, IM_COL32(255, 60, 60, 255));
}

// Damage log helper
static std::unordered_map<AMarvelBaseCharacter*, float> previousHealthMap;

void TrackDamage(AMarvelBaseCharacter* player, float currentHealth, const std::string& name) {
    if (!mods::bDamageLog) return;

    auto it = previousHealthMap.find(player);
    if (it != previousHealthMap.end()) {
        float prevHP = it->second;
        if (currentHealth < prevHP) {
            float dmg = prevHP - currentHealth;
            char buf[128];
            snprintf(buf, sizeof(buf), "%s  -%.0f HP", name.c_str(), dmg);
            mods::DamageLogEntry entry;
            entry.text = buf;
            entry.timestamp = (float)ImGui::GetTime();
            entry.color = ImColor(255, 80, 80, 255);
            mods::damageLogEntries.push_back(entry);
            while ((int)mods::damageLogEntries.size() > mods::damageLogMaxEntries) {
                mods::damageLogEntries.pop_front();
            }
        }
    }
    previousHealthMap[player] = currentHealth;
}

void AddTextWithOutline(ImDrawList* drawList, ImFont* font, float fontSize, ImVec2 pos, ImU32 textColor, ImU32 outlineColor, const char* text) {
    drawList->AddText(font, fontSize, ImVec2(pos.x - 1, pos.y), outlineColor, text);
    drawList->AddText(font, fontSize, ImVec2(pos.x + 1, pos.y), outlineColor, text);
    drawList->AddText(font, fontSize, ImVec2(pos.x, pos.y - 1), outlineColor, text);
    drawList->AddText(font, fontSize, ImVec2(pos.x, pos.y + 1), outlineColor, text);
    drawList->AddText(font, fontSize, pos, textColor, text);
}

int EATshit = 0;
static bool LilXanxPerkies = false;
bool DONTSCAN = false;
void DrawTransition(ImDrawList* BackgroundList, ImDrawList* ForegroundList) {
    try {
        UEngine* engine = UEngine::GetEngine();
        if (!engine) {
            Log("Invalid Engine");
            return;
        }

        if (!engine->GameViewport) {
            Log("Invalid GameViewport");
            return;
        }

        UWorld* aWorld = engine->GameViewport->World;
        if (!aWorld) {
            Log("Invalid UWorld");
            return;
        }

        if (!aWorld || !IsValid(aWorld) || !BackgroundList || !ForegroundList) {
            Log("DrawTransition: Invalid input parameters\n");
            return;
        }

        auto gameplayStatics = (SDK::UGameplayStatics*)SDK::UGameplayStatics::StaticClass();
        if (gameplayStatics && aWorld) {
            if (gameplayStatics->GetTimeSeconds(aWorld) <= 10) {
                return;
            }
        }

        Variables::World = aWorld;

        ULevel* PersistentLevel = aWorld->PersistentLevel;
        if (!PersistentLevel || !IsValid(PersistentLevel)) {
            Log("DrawTransition: PersistentLevel is invalid\n");
            return;
        }

        APlayerController* PlayerController = GetLocalPlayerController(aWorld);
        if (!PlayerController || !IsValid(PlayerController)) {
            Log("DrawTransition: PlayerController is invalid\n");
            return;
        }
        Variables::PlayerController = PlayerController;

        APlayerCameraManager* PlayerCameraManager = UGameplayStatics::GetPlayerCameraManager(aWorld, 0);
        if (!PlayerCameraManager || !IsValid(PlayerCameraManager)) {
            Log("DrawTransition: PlayerCameraManager is invalid\n");
            return;
        }

        APawn* AcknowledgedPawn = PlayerController->AcknowledgedPawn;
        if (!AcknowledgedPawn || !IsValid(AcknowledgedPawn)) {
            return;
        }
        Variables::AcknowledgedPawn = AcknowledgedPawn;
        Variables::CameraLocation = PlayerCameraManager->GetCameraLocation();
        Variables::CameraRotation = PlayerCameraManager->GetCameraRotation();

        // ===== Local movement correction (rubberband) detector =====
        // Writes to a log file when enabled via mods::bRubberbandLog.
        static bool s_hasLastLocalPos = false;
        static FVector s_lastLocalPos;
        static uint64_t s_lastLocalPosTickMs = 0;
        if (mods::bRubberbandLog) {
            // Initialize log path once
            if (mods::rubberbandLogPath.empty()) {
                char dllPath[MAX_PATH];
                GetModuleFileNameA(NULL, dllPath, MAX_PATH);
                std::string dir = std::string(dllPath);
                size_t lastSlash = dir.find_last_of("\\/");
                if (lastSlash != std::string::npos) dir = dir.substr(0, lastSlash);
                mods::rubberbandLogPath = dir + "\\rubberband_log.txt";
                // Write header
                FILE* f = nullptr;
                fopen_s(&f, mods::rubberbandLogPath.c_str(), "w");
                if (f) {
                    fprintf(f, "=== Rubberband Log Started ===\n");
                    fprintf(f, "Format: [time_ms] dt=<seconds> delta=<meters> pos(x,y,z) last(x,y,z) vel(x,y,z) falling=<0/1> CTD=<value> mods(speed=<0/1> jump=<0/1> fly=<0/1> wall=<0/1>)\n\n");
                    fclose(f);
                }
                mods::rubberbandLogCount = 0;
            }

            FVector curPos = AcknowledgedPawn->K2_GetActorLocation();
            uint64_t nowMs = GetTickCount64();

            if (s_hasLastLocalPos) {
                float dt = (nowMs > s_lastLocalPosTickMs) ? (float)(nowMs - s_lastLocalPosTickMs) / 1000.0f : 0.0f;
                float deltaCm = UKismetMathLibrary::Vector_Distance(curPos, s_lastLocalPos);
                float deltaM = deltaCm / 100.0f;

                if (dt > 0.0f && deltaM > 3.0f) {
                    float timeDil = 1.0f;
                    FVector vel = FVector(0, 0, 0);
                    bool isFalling = false;

                    if (PlayerController->Character && IsValid(PlayerController->Character)) {
                        auto* localChar = reinterpret_cast<AMarvelBaseCharacter*>(PlayerController->Character);
                        if (localChar && IsValid(localChar)) {
                            timeDil = localChar->CustomTimeDilation;
                            if (localChar->CharacterMovement && IsValid(localChar->CharacterMovement)) {
                                vel = localChar->CharacterMovement->Velocity;
                                isFalling = localChar->CharacterMovement->IsFalling();
                            }
                        }
                    }

                    FILE* f = nullptr;
                    fopen_s(&f, mods::rubberbandLogPath.c_str(), "a");
                    if (f) {
                        fprintf(f, "[%llu] dt=%.3f delta=%.2fm pos(%.1f,%.1f,%.1f) last(%.1f,%.1f,%.1f) vel(%.1f,%.1f,%.1f) falling=%d CTD=%.2f mods(speed=%d jump=%d fly=%d wall=%d)\n",
                            nowMs,
                            dt,
                            deltaM,
                            curPos.X, curPos.Y, curPos.Z,
                            s_lastLocalPos.X, s_lastLocalPos.Y, s_lastLocalPos.Z,
                            vel.X, vel.Y, vel.Z,
                            isFalling ? 1 : 0,
                            timeDil,
                            mods::bSpeedHack ? 1 : 0,
                            mods::bSuperJump ? 1 : 0,
                            mods::bFlyHack ? 1 : 0,
                            mods::bWallClimbAnywhere ? 1 : 0);
                        fclose(f);
                        mods::rubberbandLogCount++;
                    }
                }
            }

            s_lastLocalPos = curPos;
            s_lastLocalPosTickMs = nowMs;
            s_hasLastLocalPos = true;
        } else {
            // Reset state when logging is disabled
            s_hasLastLocalPos = false;
            if (!mods::rubberbandLogPath.empty()) {
                mods::rubberbandLogPath = "";
            }
        }

        ////AMarvelAbilityTargetActor_Projectile
        //AMarvelAbilityTargetActor_Trace* TargetActor = reinterpret_cast<AMarvelAbilityTargetActor_Trace*>(PlayerController);
        //if (!TargetActor) return;

        //if (TargetActor) {
        //    TargetActor->AimingSpreadMod = 0.F;
        //    TargetActor->BaseSpread = 0.0f;
        //    TargetActor->TargetingSpreadIncrement = 0.f;
        //    TargetActor->TargetingSpreadMax = 0.0f;
        //    
        //}

        if (PlayerController->Character || IsValid(PlayerController->Character)) {
            if (GetAsyncKeyState(VK_LCONTROL) && mods::SelfCustomTimeDilationBool) {
                PlayerController->Character->CustomTimeDilation = mods::SelfCustomTimeDilationFloat;
            }
            else if (!mods::bSpeedHack) {
                // Only reset to 1 if speed hack isn't active (speed hack sets its own value)
                PlayerController->Character->CustomTimeDilation = 1;
            }
        }

        // ===== Exploits & Features: per-frame init =====
        Exploits::BeginFrame();

        // ===== Combo System Tick =====
        ComboSystem::Tick();

        // ===== AutoKey System Tick =====
        if (PlayerController->Character && IsValid(PlayerController->Character)) {
            AMarvelBaseCharacter* akLocal = reinterpret_cast<AMarvelBaseCharacter*>(PlayerController->Character);
            if (akLocal && IsValid(akLocal)) {
                AutoKeySystem::Tick(
                    akLocal->HeroID,
                    akLocal->GetCurrentHealth(),
                    akLocal->GetMaxHealth(),
                    mods::closestDistance);
            }
        }

        // ===== pSilent Hook: install once, sync state each frame =====
        static bool bPSilentInstalled = false;
        static void* s_lastPawn = nullptr;
        if (!bPSilentInstalled) {
            bPSilentInstalled = SilentAim::Install();
        }
        // Reset UFunction cache when local pawn changes (respawn / round change)
        void* curPawn = reinterpret_cast<void*>(Variables::AcknowledgedPawn);
        if (curPawn != s_lastPawn) {
            SilentAim::ResetCache();
            s_lastPawn = curPawn;
        }
        SilentAim::UpdateHookState();

        struct ChamsMeshCache {
            uint8 bRenderCustomDepth;
            int32 CustomDepthStencilValue;
            uint8 bForceWireframe;
            std::vector<class UMaterialInterface*> Materials;
        };

        static std::unordered_map<SDK::USkeletalMeshComponent*, ChamsMeshCache> s_chamsCache;
        static std::unordered_map<SDK::USkeletalMeshComponent*, ChamsMeshCache> s_selfChamsCache;
        static bool s_lastChamsEnabled = false;
        static bool s_lastSelfChamsEnabled = false;
        static bool s_lastSelfWireframeEnabled = false;

        auto RestoreChamsOnMesh = [](SDK::USkeletalMeshComponent* Mesh, const ChamsMeshCache& cache) {
            if (!Mesh || !IsValid(Mesh)) return;
            Mesh->bRenderCustomDepth = cache.bRenderCustomDepth;
            Mesh->CustomDepthStencilValue = cache.CustomDepthStencilValue;
            Mesh->bForceWireframe = cache.bForceWireframe;
            int32 num = Mesh->GetNumMaterials();
            int32 restoreCount = (int32)cache.Materials.size();
            if (restoreCount > num) restoreCount = num;
            for (int32 i = 0; i < restoreCount; ++i) {
                if (cache.Materials[i]) {
                    Mesh->SetMaterial(i, cache.Materials[i]);
                }
            }
        };

        // Helper: apply chams effect to a mesh with full MID glow
        // Reuses existing MIDs on subsequent frames to avoid memory churn
        static SDK::UClass* s_midClass = SDK::UMaterialInstanceDynamic::StaticClass();
        static SDK::FName n_BaseColor = UKismetStringLibrary::Conv_StringToName(SDK::FString(L"BaseColor"));
        static SDK::FName n_Emissive  = UKismetStringLibrary::Conv_StringToName(SDK::FString(L"EmissiveColor"));
        static SDK::FName n_Opacity   = UKismetStringLibrary::Conv_StringToName(SDK::FString(L"Opacity"));

        auto ApplyChamsToMesh = [](SDK::USkeletalMeshComponent* Mesh, ImVec4 color, float glowIntensity, bool wireframe, bool throughWalls) {
            if (!Mesh || !IsValid(Mesh)) return;

            if (throughWalls) {
                Mesh->bRenderCustomDepth = 1;
                Mesh->CustomDepthStencilValue = 255;
            }

            Mesh->bForceWireframe = wireframe ? 1 : 0;

            int32 numMats = Mesh->GetNumMaterials();
            for (int32 i = 0; i < numMats; ++i) {
                SDK::UMaterialInterface* curMat = Mesh->GetMaterial(i);
                if (!curMat || !IsValid(curMat)) continue;

                // Reuse existing MID if already applied, otherwise create new one
                SDK::UMaterialInstanceDynamic* mid = nullptr;
                if (s_midClass && curMat->IsA(s_midClass)) {
                    mid = reinterpret_cast<SDK::UMaterialInstanceDynamic*>(curMat);
                } else {
                    mid = Mesh->CreateDynamicMaterialInstance(i, curMat, SDK::FName());
                }
                if (!mid || !IsValid(mid)) continue;

                SDK::FLinearColor baseCol;
                baseCol.R = color.x; baseCol.G = color.y; baseCol.B = color.z; baseCol.A = color.w;
                mid->SetVectorParameterValue(n_BaseColor, baseCol);

                SDK::FLinearColor emissiveCol;
                emissiveCol.R = color.x * glowIntensity;
                emissiveCol.G = color.y * glowIntensity;
                emissiveCol.B = color.z * glowIntensity;
                emissiveCol.A = 1.0f;
                mid->SetVectorParameterValue(n_Emissive, emissiveCol);

                mid->SetScalarParameterValue(n_Opacity, color.w);
            }
        };

        // Restore enemy chams when toggled off
        if (!mods::bChams && s_lastChamsEnabled) {
            for (auto it = s_chamsCache.begin(); it != s_chamsCache.end(); ++it) {
                RestoreChamsOnMesh(it->first, it->second);
            }
            s_chamsCache.clear();
        }
        s_lastChamsEnabled = mods::bChams;

        // Restore self chams when toggled off
        if ((!mods::bSelfChams && !mods::bSelfWireframe) && (s_lastSelfChamsEnabled || s_lastSelfWireframeEnabled)) {
            for (auto it = s_selfChamsCache.begin(); it != s_selfChamsCache.end(); ++it) {
                RestoreChamsOnMesh(it->first, it->second);
            }
            s_selfChamsCache.clear();
        }
        s_lastSelfChamsEnabled = mods::bSelfChams;
        s_lastSelfWireframeEnabled = mods::bSelfWireframe;

        // Apply local-character exploits (no cooldown, speed hack, fly, etc.)
        if (PlayerController->Character && IsValid(PlayerController->Character)) {
            AMarvelBaseCharacter* localChar = reinterpret_cast<AMarvelBaseCharacter*>(PlayerController->Character);
            Exploits::ApplyLocalExploits(localChar, aWorld);
            Features::PerFrameLogic(localChar, mods::closestDistance);

            // Healer features: smart heal queue + team dashboard
            Features::UpdateHealQueue(aWorld, PlayerController, AcknowledgedPawn);
            Features::UpdateTeamDashboard(aWorld, PlayerController, AcknowledgedPawn);
        }

        if (mods::Experimental::HideLocalPlayer) {
            if (PlayerController->Character) {
                SDK::USkeletalMeshComponent* MeshComponent = PlayerController->Character->GetMesh();
                if (MeshComponent) {
                    if (mods::Experimental::HideLocalPlayer) {
                        MeshComponent->SetHiddenInGame(true, true);
                    }
                    else if (!mods::Experimental::HideLocalPlayer && MeshComponent->bHiddenInGame) {
                        MeshComponent->SetHiddenInGame(false, false);
                    }
                }
            }
        }
        if (mods::Experimental::SmallPerson) {
            if (PlayerController->Character) {
                SDK::USkeletalMeshComponent* MeshComponent = PlayerController->Character->GetMesh();
                if (MeshComponent) {
                    if (mods::Experimental::SmallPerson) {
                        FVector desiredScale(mods::Experimental::SmallPersonScale,
                            mods::Experimental::SmallPersonScale,
                            mods::Experimental::SmallPersonScale);
                        if (MeshComponent->RelativeScale3D != desiredScale) {
                            MeshComponent->SetWorldScale3D(desiredScale);
                        }
                    }
                }
            }
        }

        // === Self Chams & Self Wireframe ===
        if ((mods::bSelfChams || mods::bSelfWireframe) && PlayerController->Character && IsValid(PlayerController->Character)) {
            SDK::USkeletalMeshComponent* selfMesh = PlayerController->Character->GetMesh();
            if (selfMesh && IsValid(selfMesh)) {
                // Cache original state
                auto selfCacheIt = s_selfChamsCache.find(selfMesh);
                if (selfCacheIt == s_selfChamsCache.end()) {
                    ChamsMeshCache cache;
                    cache.bRenderCustomDepth = selfMesh->bRenderCustomDepth;
                    cache.CustomDepthStencilValue = selfMesh->CustomDepthStencilValue;
                    cache.bForceWireframe = selfMesh->bForceWireframe;
                    int32 numMats = selfMesh->GetNumMaterials();
                    cache.Materials.reserve((size_t)numMats);
                    for (int32 i = 0; i < numMats; ++i) {
                        cache.Materials.push_back(selfMesh->GetMaterial(i));
                    }
                    selfCacheIt = s_selfChamsCache.emplace(selfMesh, std::move(cache)).first;
                }

                bool doWireframe = mods::bSelfWireframe || (mods::bSelfChams && mods::selfChamsStyle == mods::SELF_CHAMS_WIREFRAME);

                if (mods::bSelfChams) {
                    ImVec4 selfCol = mods::selfChamsColor.Value;
                    selfCol.w = mods::selfChamsOpacity;
                    float intensity = (mods::selfChamsStyle == mods::SELF_CHAMS_GLOW) ? mods::selfChamsGlowIntensity : 1.0f;
                    ApplyChamsToMesh(selfMesh, selfCol, intensity, doWireframe, false);
                } else if (mods::bSelfWireframe) {
                    selfMesh->bForceWireframe = 1;
                }
            }
        }

        // Skin Changer - apply skin overrides to local player
        if (mods::bSkinChanger && PlayerController->Character) {
            AMarvelBaseCharacter* localChar = reinterpret_cast<AMarvelBaseCharacter*>(PlayerController->Character);
            if (localChar && IsValid(localChar)) {
                int32_t heroId = localChar->HeroID;
                auto enableIt = mods::skinEnabled.find(heroId);
                if (enableIt != mods::skinEnabled.end() && enableIt->second) {
                    auto skinIt = mods::skinOverrides.find(heroId);
                    if (skinIt != mods::skinOverrides.end() && localChar->SkinID != skinIt->second) {
                        localChar->SkinID = skinIt->second;
                    }
                }
            }
        }

        if (mods::bSpinbot && PlayerController->Character) {
            if (PlayerController->Character) {
                static TArray<AActor*> components;
                components.Clear(); // reuse allocation, just reset count
                PlayerController->Character->GetAllChildActors(&components, true);
                static float Yaw, pitch, roll = 1.0f;
                Yaw += mods::SpiningSpeedX;
                pitch += mods::SpiningSpeedY;
                roll += mods::SpiningSpeedZ;
                if (Yaw >= 360.0f) Yaw = 1.0f;
                if (pitch >= 360.0f) pitch = 1.0f;
                if (roll >= 360.0f) roll = 1.0f;
                SDK::FRotator NewRotation(mods::bSpinbotX ? Yaw : 0.0f, mods::bSpinbotY ? pitch : 0.0f, mods::bSpinbotZ ? roll : 0.0f);
                SDK::FHitResult fhit;
                for (AActor* comp : components) {
                    comp->K2_SetActorRelativeRotation(NewRotation, false, &fhit, false);
                }
            }
        }

        if (mods::fov_changer && PlayerController) {
            PlayerController->FOV(mods::fov_changer_amount);
        }

        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        ImVec2 screenCenter = ImVec2(displaySize.x / 2, displaySize.y / 2);

        DrawCrosshair(ForegroundList, screenCenter);

        ImFont* font = nullptr;
        if (!mods::availableFonts.empty() && mods::selectedESPFontIndex >= 0 && mods::selectedESPFontIndex < (int)mods::availableFonts.size()) {
            font = mods::availableFonts[mods::selectedESPFontIndex];
        }
        if (!font) {
            font = ImGui::GetFont();
        }
        float fontSize = mods::baseFontSize * mods::textScale;

        // Compute base aimbot FOV radius in pixels BEFORE enemy collection
        // This ensures actualfovcircle is always valid. Adaptive FOV may override it later.
        {
            float baseFovRadius = static_cast<float>(mods::fov) * displaySize.x / static_cast<float>(mods::fov_changer_amount > 0 ? mods::fov_changer_amount : 115) / 2.0f;
            mods::actualfovcircle = baseFovRadius;
        }

        // Draw aimbot FOV circle
        if (mods::aimbotFovCircle && mods::actualfovcircle > 0.0f) {
            BackgroundList->AddCircle(screenCenter, mods::actualfovcircle, mods::aimbotFovCircleColor, 64, 1.0f);
        }

        // Heal Aim — runs after screenCenter is defined
        if (PlayerController->Character && IsValid(PlayerController->Character)) {
            Features::CheckHealAim(PlayerController, AcknowledgedPawn, aWorld, screenCenter);
        }
        if (mods::bHealAimFovCircle && mods::healAimFov > 0.0f) {
            BackgroundList->AddCircle(screenCenter, mods::healAimFov, mods::healAimFovColor, 64, 1.5f);
        }

        // Draw pSilent FOV circle
        if (mods::bPSilentEnabled && mods::bPSilentShowFov) {
            float pSilentRadius = mods::pSilentFov * displaySize.x / static_cast<float>(mods::fov_changer_amount > 0 ? mods::fov_changer_amount : 115) / 2.0f;
            if (pSilentRadius > 0.0f) {
                BackgroundList->AddCircle(screenCenter, pSilentRadius, mods::pSilentFovColor, 64, 1.5f);
            }
        }

        struct Candidate {
            AMarvelBaseCharacter* player;
            float distance;
            bool visible;
        };
        static std::vector<Candidate> candidates;
        candidates.clear();
        mods::closestDistance = FLT_MAX;

        // Clear per-frame overlay data
        mods::enemyOverlayData.clear();
        mods::ultTrackerEntries.clear();

        // Time-prune popup/timer vectors to prevent unbounded growth
        {
            float now = (float)GetTickCount64() / 1000.0f;
            mods::damagePopups.erase(
                std::remove_if(mods::damagePopups.begin(), mods::damagePopups.end(),
                    [now](const mods::DamagePopup& p) { return now - p.timestamp > mods::damageNumberDuration + 0.5f; }),
                mods::damagePopups.end());
            mods::spawnTimers.erase(
                std::remove_if(mods::spawnTimers.begin(), mods::spawnTimers.end(),
                    [now](const mods::SpawnTimerEntry& e) { return now - e.deathTime > e.respawnDuration + 2.0f; }),
                mods::spawnTimers.end());
            mods::healthPackTimers.erase(
                std::remove_if(mods::healthPackTimers.begin(), mods::healthPackTimers.end(),
                    [now](const mods::HealthPackInfo& h) { return h.isAvailable && now - h.lastPickupTime > h.respawnTime + 5.0f; }),
                mods::healthPackTimers.end());
        }

        // Seen-this-frame sets used to evict stale map entries after the loop
        static std::unordered_set<SDK::USkeletalMeshComponent*> s_seenMeshes;
        static std::unordered_set<AMarvelBaseCharacter*>         s_seenPlayers;
        static std::unordered_map<AMarvelBaseCharacter*, bool>   s_losCache;
        static int s_drawFrame = 0;
        ++s_drawFrame;
        s_seenMeshes.clear();
        s_seenPlayers.clear();

        // pSilent target tracked independently of aimbot toggle
        AMarvelBaseCharacter* pSilentCandidate = nullptr;
        float pSilentBestFovDist = FLT_MAX;

        // ===== Cache ThreatValueAdmin data once per frame =====
        struct CachedThreatInfo { float UltimatePercentage; float CurrentHP; float MaxHP; };
        static std::unordered_map<AMarvelBaseCharacter*, CachedThreatInfo> s_threatCache;
        s_threatCache.clear();
        {
            AThreatValueAdmin* TVA = SDK::UMarvelAudioLibrary::GetThreatValueAdmin(aWorld);
            if (TVA && IsValid(TVA)) {
                auto ThreatInfoArray = TVA->GetPlayerThreatInfo();
                for (const auto& TI : ThreatInfoArray) {
                    AMarvelBaseCharacter* ch = TI.Character.Get();
                    if (!ch || !IsValid(ch)) continue;
                    CachedThreatInfo cti;
                    cti.UltimatePercentage = TI.UltimatePercentage;
                    if (!SafeGetHP(ch, &cti.CurrentHP, &cti.MaxHP)) continue;
                    s_threatCache[ch] = cti;
                }
            }
        }

        const TArray<AActor*>& ActorList = PersistentLevel->Actors;
        if (!ActorList.IsValid()) {
            Log("DrawTransition: ActorList is invalid\n");
            return;
        }

        static UClass* BulletClass = AGameplayAbilityTargetActor::StaticClass();
        if (!BulletClass) {
            BulletClass = AGameplayAbilityTargetActor::StaticClass();
            if (!BulletClass) { Log("FUCK YOU BULLET CLASS"); return; }
        }

        static UClass* ClassToFind = AMarvelBaseCharacter::StaticClass();
        if (!ClassToFind) {
            ClassToFind = AMarvelBaseCharacter::StaticClass();
            if (!ClassToFind) { Log("DrawTransition: ClassToFind is null\n"); return; }
        }

        for (int i = 0; i < ActorList.Num(); i++) {
            if (!ActorList.IsValidIndex(i)) continue;
            AActor* actor = ActorList[i];
            if (!actor || !IsValid(actor)) continue;

            if (CheckIfPlayer(actor, ClassToFind)) {

                AMarvelBaseCharacter* Player = reinterpret_cast<AMarvelBaseCharacter*>(actor);
                if (!Player || !IsValid(Player)) continue;

                USkeletalMeshComponent* Mesh = Player->GetMesh();
                if (!Mesh || !IsValid(Mesh)) continue;

                float currentHealth = Player->GetCurrentHealth();
                if (currentHealth <= 0.f) continue;

                static FName HeadBoneName = UKismetStringLibrary::Conv_StringToName(SDK::FString(L"Head"));
                if (!Mesh->DoesSocketExist(HeadBoneName)) continue;

                FVector TargetHead3D = Mesh->GetSocketLocation(HeadBoneName);
                SDK::FVector2D TargetHead2D;
                if (!PlayerController->ProjectWorldLocationToScreen(TargetHead3D, &TargetHead2D, true)) continue;

                if (mods::LocalCheck && Player->IsLocallyControlled()) continue;

                float distance = UKismetMathLibrary::Vector_Distance(Variables::CameraLocation, Player->K2_GetActorLocation()) / 100.0f;

                bool bIsEnemy = false;
                if (mods::bESPTeamCheck || mods::bAimbotTeamCheck) {
                    AMarvelPlayerState* PlayerState = static_cast<AMarvelPlayerState*>(Player->PlayerState);
                    AMarvelPlayerState* LocalState = static_cast<AMarvelPlayerState*>(AcknowledgedPawn->PlayerState);

                    if (!PlayerState || !IsValid(PlayerState) || !LocalState || !IsValid(LocalState)) continue;
                    if (PlayerState->OriginalTeamID == LocalState->OriginalTeamID) continue;
                    bIsEnemy = true;
                    mods::closestDistance = std::min(mods::closestDistance, distance);
                }

                if (GetAsyncKeyState(mods::aimbotKey) && mods::CustomTimeDilationBool) {
                    Player->CustomTimeDilation = mods::CustomTimeDilationFloat;
                }
                else {
                    Player->CustomTimeDilation = 1;
                }

                // Rate-limited LOS: recalculate on alternating frames per player to halve raycast cost
                // Visibility is part of target ranking, so collect LOS for
                // every aimbot candidate even when the optional VisCheck
                // filter is disabled.
                bool bNeedsLOS = mods::bESPBox || mods::bLOSIndicator || mods::aimbot;
                bool bIsVisible = false;
                if (bNeedsLOS) {
                    bool doLOS = (s_drawFrame & 1) == (reinterpret_cast<uintptr_t>(Player) & 1);
                    if (doLOS) {
                        bIsVisible = PlayerController->LineOfSightTo(Player, Variables::CameraLocation, false);
                        s_losCache[Player] = bIsVisible;
                    } else {
                        auto losIt = s_losCache.find(Player);
                        if (losIt != s_losCache.end()) bIsVisible = losIt->second;
                    }
                }

                // Radar dot rendering
                if (bIsEnemy) {
                    UpdateRadarDots(ForegroundList, AcknowledgedPawn->K2_GetActorLocation(), Variables::CameraRotation, Player->K2_GetActorLocation());
                }

                // Damage log tracking
                if (bIsEnemy) {
                    // PERF: Use heroIDToName instead of GetPlayerName() to avoid FString + std::string heap allocs
                    int eHeroID = Player->HeroID;
                    static const std::string s_unknown = "Unknown";
                    auto eNameIt = mods::heroIDToName.find(eHeroID);
                    const std::string& enemyHeroName = (eNameIt != mods::heroIDToName.end()) ? eNameIt->second : s_unknown;

                    TrackDamage(Player, currentHealth, enemyHeroName);

                    // === Enemy Exploits ===
                    Exploits::ApplyEnemyExploits(Player, enemyHeroName);

                    // === Visual Features on enemies ===
                    FVector enemyPos = Player->K2_GetActorLocation();
                    FVector localPos = AcknowledgedPawn->K2_GetActorLocation();

                    float maxHP_feat = Player->GetMaxHealth();
                    float hpPct_feat = (maxHP_feat > 0.0f) ? (currentHealth / maxHP_feat) : 1.0f;

                    // Sound ESP compass
                    Features::DrawSoundESP(ForegroundList, screenCenter, localPos, Variables::CameraRotation, enemyPos, distance);

                    // Flank alert
                    Features::CheckFlankAlert(ForegroundList, screenCenter, localPos, Variables::CameraRotation, enemyPos, distance);

                    // Enhanced minimap enemy dot
                    float elevDiff = enemyPos.Z - localPos.Z;
                    Features::DrawMinimapEnemy(ForegroundList, localPos, Variables::CameraRotation, enemyPos, elevDiff);

                    // LOS indicator
                    Features::DrawLOSIndicator(ForegroundList, ImVec2(TargetHead2D.X, TargetHead2D.Y), bIsVisible);

                    // Kill prediction marker
                    if (Features::CanOneShot(currentHealth)) {
                        ForegroundList->AddText(ImVec2(TargetHead2D.X - 6, TargetHead2D.Y - 25), IM_COL32(255, 0, 0, 255), "X");
                    }

                    // Hit sound + damage popup tracking
                    {
                        s_seenPlayers.insert(Player);
                        auto hpIt = previousHealthMap.find(Player);
                        if (hpIt != previousHealthMap.end()) {
                            float prevHP = hpIt->second;
                            Features::PlayHitSound(prevHP, currentHealth);
                            if (currentHealth < prevHP) {
                                float dmg = prevHP - currentHealth;
                                Features::AddDamagePopup(TargetHead2D.X, TargetHead2D.Y, dmg, false);
                                mods::sessionDamageDealt += dmg;
                            }
                        }
                        previousHealthMap[Player] = currentHealth;
                    }

                    // Track enemy death for spawn timer
                    if (currentHealth <= 1.0f) {
                        Features::TrackEnemyDeath(enemyHeroName);
                    }

                    // === Chams Application ===
                    if (mods::bChams && Mesh) {
                        s_seenMeshes.insert(Mesh);
                        auto cacheIt = s_chamsCache.find(Mesh);
                        if (cacheIt == s_chamsCache.end()) {
                            ChamsMeshCache cache;
                            cache.bRenderCustomDepth = Mesh->bRenderCustomDepth;
                            cache.CustomDepthStencilValue = Mesh->CustomDepthStencilValue;
                            cache.bForceWireframe = Mesh->bForceWireframe;
                            int32 numMats = Mesh->GetNumMaterials();
                            cache.Materials.reserve((size_t)numMats);
                            for (int32 i = 0; i < numMats; ++i) {
                                cache.Materials.push_back(Mesh->GetMaterial(i));
                            }
                            cacheIt = s_chamsCache.emplace(Mesh, std::move(cache)).first;
                        }

                        ImVec4 chamsColor;
                        if (mods::bChamsHealthBased) {
                            chamsColor = ImVec4(1.0f - hpPct_feat, hpPct_feat, 0.0f, mods::chamsOpacity);
                        } else {
                            chamsColor = bIsVisible ? mods::chamsVisibleColor.Value : mods::chamsNotVisibleColor.Value;
                            chamsColor.w = mods::chamsOpacity;
                        }

                        bool doWireframe = (mods::chamsStyle == mods::CHAMS_WIREFRAME);
                        bool doThroughWalls = mods::bChamsThroughWalls;

                        float glowIntensity = 1.0f;
                        if (mods::chamsStyle == mods::CHAMS_GLOW) {
                            glowIntensity = mods::chamsGlowIntensity;
                        } else if (mods::chamsStyle == mods::CHAMS_PULSE) {
                            float t = (float)GetTickCount64() / 1000.0f;
                            float pulse = (sinf(t * 3.0f) + 1.0f) * 0.5f;
                            glowIntensity = 2.0f + pulse * (mods::chamsGlowIntensity - 2.0f);
                        }

                        ApplyChamsToMesh(Mesh, chamsColor, glowIntensity, doWireframe, doThroughWalls);
                    }

                }

                if (distance <= mods::espMaxDistance && distance <= mods::distanceFilter) {
                    {
// ... (rest of the code remains the same)
                        static SDK::FName s_pelvisBone = UKismetStringLibrary::Conv_StringToName(SDK::FString(L"pelvis"));
                    FVector Origin = Mesh->GetSocketLocation(s_pelvisBone);
                        FVector BoxExtent = FVector(50.0f, 50.0f, 100.0f);

                        FVector Corners[8] = {
                            Origin + FVector(-BoxExtent.X, -BoxExtent.Y, -BoxExtent.Z),
                            Origin + FVector(BoxExtent.X, -BoxExtent.Y, -BoxExtent.Z),
                            Origin + FVector(BoxExtent.X, BoxExtent.Y, -BoxExtent.Z),
                            Origin + FVector(-BoxExtent.X, BoxExtent.Y, -BoxExtent.Z),
                            Origin + FVector(-BoxExtent.X, -BoxExtent.Y, BoxExtent.Z),
                            Origin + FVector(BoxExtent.X, -BoxExtent.Y, BoxExtent.Z),
                            Origin + FVector(BoxExtent.X, BoxExtent.Y, BoxExtent.Z),
                            Origin + FVector(-BoxExtent.X, BoxExtent.Y, BoxExtent.Z)
                        };
                        bool bOnScreen = false;
                        SDK::FVector2D ScreenMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
                        SDK::FVector2D ScreenMax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
                        SDK::FVector2D screenCorners[8];

                        for (int j = 0; j < 8; j++) {
                            if (PlayerController->ProjectWorldLocationToScreen(Corners[j], &screenCorners[j], true)) {
                                if (!bOnScreen) {
                                    ScreenMin = screenCorners[j];
                                    ScreenMax = screenCorners[j];
                                    bOnScreen = true;
                                }
                                else {
                                    ScreenMin.X = std::min(ScreenMin.X, screenCorners[j].X);
                                    ScreenMin.Y = std::min(ScreenMin.Y, screenCorners[j].Y);
                                    ScreenMax.X = std::max(ScreenMax.X, screenCorners[j].X);
                                    ScreenMax.Y = std::max(ScreenMax.Y, screenCorners[j].Y);
                                }
                            }
                        }

                        if (bOnScreen) {
                            ImU32 color = bIsVisible ? mods::visibleColor : mods::nonVisibleColor;

                            if (mods::bESPBox) {
                                if (mods::espBoxType == mods::ESP_BOX_2D) {
                                    if (mods::bESPBoxOutline) {
                                        BackgroundList->AddRect(ImVec2(ScreenMin.X, ScreenMin.Y), ImVec2(ScreenMax.X, ScreenMax.Y), mods::espBoxOutlineColor, 0.0f, ImDrawFlags_None, mods::espBoxThickness + 2.0f);
                                    }
                                    BackgroundList->AddRect(ImVec2(ScreenMin.X, ScreenMin.Y), ImVec2(ScreenMax.X, ScreenMax.Y), color, 0.0f, ImDrawFlags_None, mods::espBoxThickness);
                                }
                                else if (mods::espBoxType == mods::ESP_BOX_3D) {
                                    static constexpr int edges[][2] = {
                                        {0,1}, {1,2}, {2,3}, {3,0},
                                        {4,5}, {5,6}, {6,7}, {7,4},
                                        {0,4}, {1,5}, {2,6}, {3,7}
                                    };
                                    for (const auto& edge : edges) {
                                        ImVec2 p1 = ImVec2(screenCorners[edge[0]].X, screenCorners[edge[0]].Y);
                                        ImVec2 p2 = ImVec2(screenCorners[edge[1]].X, screenCorners[edge[1]].Y);
                                        if (mods::bESPBoxOutline) {
                                            BackgroundList->AddLine(p1, p2, mods::espBoxOutlineColor, mods::espBoxThickness + 2.0f);
                                        }
                                        BackgroundList->AddLine(p1, p2, color, mods::espBoxThickness);
                                    }
                                }
                            }

                            if (mods::bHealthBar) {
                                float maxHealth = Player->GetMaxHealth();
                                float healthPercent = currentHealth / maxHealth;
                                ImVec2 barStart, barEnd;
                                if (mods::healthBarPosition == mods::LEFT) {
                                    barStart = ImVec2(ScreenMin.X - 10.0f, ScreenMin.Y);
                                    barEnd = ImVec2(ScreenMin.X - 5.0f, ScreenMax.Y);
                                }
                                else if (mods::healthBarPosition == mods::RIGHT) {
                                    barStart = ImVec2(ScreenMax.X + 5.0f, ScreenMin.Y);
                                    barEnd = ImVec2(ScreenMax.X + 10.0f, ScreenMax.Y);
                                }
                                else if (mods::healthBarPosition == mods::TOP) {
                                    barStart = ImVec2(ScreenMin.X, ScreenMin.Y - 10.0f);
                                    barEnd = ImVec2(ScreenMax.X, ScreenMin.Y - 5.0f);
                                }
                                else if (mods::healthBarPosition == mods::BOTTOM) {
                                    barStart = ImVec2(ScreenMin.X, ScreenMax.Y + 5.0f);
                                    barEnd = ImVec2(ScreenMax.X, ScreenMax.Y + 10.0f);
                                }
                                ImU32 healthColor = (healthPercent >= 0.7f) ? mods::healthHighColor :
                                    (healthPercent >= 0.3f) ? mods::healthMidColor : mods::healthLowColor;
                                if (mods::healthBarPosition == mods::LEFT || mods::healthBarPosition == mods::RIGHT) {
                                    float barHeight = barEnd.y - barStart.y;
                                    ImVec2 fillTL(barStart.x, barEnd.y - (healthPercent * barHeight));
                                    BackgroundList->AddRectFilled(barStart, barEnd, mods::barBackgroundColor);
                                    BackgroundList->AddRectFilled(fillTL, barEnd, healthColor);
                                    BackgroundList->AddRect(barStart, barEnd, mods::healthBarOutlineColor);
                                }
                                else {
                                    float barWidth = barEnd.x - barStart.x;
                                    ImVec2 fillBR(barStart.x + (healthPercent * barWidth), barEnd.y);
                                    BackgroundList->AddRectFilled(barStart, barEnd, mods::barBackgroundColor);
                                    BackgroundList->AddRectFilled(barStart, fillBR, healthColor);
                                    BackgroundList->AddRect(barStart, barEnd, mods::healthBarOutlineColor);
                                }
                            }

                            if (mods::bUltimatePercentage) {
                                auto threatIt = s_threatCache.find(Player);
                                if (threatIt != s_threatCache.end()) {
                                    float ultPercent = threatIt->second.UltimatePercentage / 100.0f;
                                    ImVec2 uStart, uEnd;
                                    if (mods::ultBarPosition == mods::LEFT) {
                                        uStart = ImVec2(ScreenMin.X - 17.0f, ScreenMin.Y);
                                        uEnd   = ImVec2(ScreenMin.X - 12.0f, ScreenMax.Y);
                                    }
                                    else if (mods::ultBarPosition == mods::RIGHT) {
                                        uStart = ImVec2(ScreenMax.X + 12.0f, ScreenMin.Y);
                                        uEnd   = ImVec2(ScreenMax.X + 17.0f, ScreenMax.Y);
                                    }
                                    else if (mods::ultBarPosition == mods::TOP) {
                                        uStart = ImVec2(ScreenMin.X, ScreenMin.Y - 17.0f);
                                        uEnd   = ImVec2(ScreenMax.X, ScreenMin.Y - 12.0f);
                                    }
                                    else {
                                        uStart = ImVec2(ScreenMin.X, ScreenMax.Y + 12.0f);
                                        uEnd   = ImVec2(ScreenMax.X, ScreenMax.Y + 17.0f);
                                    }
                                    if (mods::ultBarPosition == mods::LEFT || mods::ultBarPosition == mods::RIGHT) {
                                        float h = uEnd.y - uStart.y;
                                        ImVec2 fillTL(uStart.x, uEnd.y - (ultPercent * h));
                                        BackgroundList->AddRectFilled(uStart, uEnd, mods::barBackgroundColor);
                                        BackgroundList->AddRectFilled(fillTL, uEnd, mods::ultBarColor);
                                        BackgroundList->AddRect(uStart, uEnd, mods::ultBarOutlineColor);
                                    }
                                    else {
                                        float w = uEnd.x - uStart.x;
                                        ImVec2 fillBR(uStart.x + (ultPercent * w), uEnd.y);
                                        BackgroundList->AddRectFilled(uStart, uEnd, mods::barBackgroundColor);
                                        BackgroundList->AddRectFilled(uStart, fillBR, mods::ultBarColor);
                                        BackgroundList->AddRect(uStart, uEnd, mods::ultBarOutlineColor);
                                    }
                                }
                            }

                            // Stacking offsets to prevent ESP text overlap per position
                            float stackTop = 5.0f, stackBottom = 5.0f, stackLeft = 5.0f, stackRight = 5.0f;

                            if (mods::bShowDistance) {
                                char distStr[16];
                                snprintf(distStr, sizeof(distStr), "%dm", static_cast<int>(distance));
                                ImVec2 ts = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, distStr);
                                ImVec2 tp;
                                switch (mods::distancePosition) {
                                case mods::TOP:    tp = ImVec2((ScreenMin.X+ScreenMax.X)/2.0f - ts.x/2, ScreenMin.Y - ts.y - stackTop); stackTop += ts.y + 3; break;
                                case mods::BOTTOM: tp = ImVec2((ScreenMin.X+ScreenMax.X)/2.0f - ts.x/2, ScreenMax.Y + stackBottom); stackBottom += ts.y + 3; break;
                                case mods::LEFT:   tp = ImVec2(ScreenMin.X - ts.x - stackLeft, (ScreenMin.Y+ScreenMax.Y)/2.0f - ts.y/2); stackLeft += ts.x + 5; break;
                                default:           tp = ImVec2(ScreenMax.X + stackRight, (ScreenMin.Y+ScreenMax.Y)/2.0f - ts.y/2); stackRight += ts.x + 5; break;
                                }
                                AddTextWithOutline(BackgroundList, font, fontSize, tp, mods::distanceTextColor, mods::distanceTextOutlineColor, distStr);
                            }

                            if (mods::bShowHeroNames) {
                                // PERF: use heroIDToName instead of GetPlayerName() to avoid FString+std::string heap allocs
                                char PlayerName[32];
                                auto hnIt = mods::heroIDToName.find(Player->HeroID);
                                snprintf(PlayerName, sizeof(PlayerName), "%s", (hnIt != mods::heroIDToName.end()) ? hnIt->second.c_str() : "Unknown");
                                ImVec2 ts = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, PlayerName);
                                ImVec2 tp;
                                switch (mods::heroNamePosition) {
                                case mods::TOP:    tp = ImVec2((ScreenMin.X+ScreenMax.X)/2.0f - ts.x/2, ScreenMin.Y - ts.y - stackTop); stackTop += ts.y + 3; break;
                                case mods::BOTTOM: tp = ImVec2((ScreenMin.X+ScreenMax.X)/2.0f - ts.x/2, ScreenMax.Y + stackBottom); stackBottom += ts.y + 3; break;
                                case mods::LEFT:   tp = ImVec2(ScreenMin.X - ts.x - stackLeft, (ScreenMin.Y+ScreenMax.Y)/2.0f - ts.y/2); stackLeft += ts.x + 5; break;
                                default:           tp = ImVec2(ScreenMax.X + stackRight, (ScreenMin.Y+ScreenMax.Y)/2.0f - ts.y/2); stackRight += ts.x + 5; break;
                                }
                                if (mods::bTextBackground)
                                    BackgroundList->AddRectFilled(ImVec2(tp.x-5,tp.y-2), ImVec2(tp.x+ts.x+5,tp.y+ts.y+2), mods::heroNameBgColor);
                                AddTextWithOutline(BackgroundList, font, fontSize, tp, mods::heroNameTextColor, mods::heroNameTextOutlineColor, PlayerName);
                            }

                            if (mods::bShowHealthText) {
                                char hStr[16];
                                snprintf(hStr, sizeof(hStr), "%d", static_cast<int>(currentHealth));
                                ImVec2 ts = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, hStr);
                                ImVec2 tp;
                                switch (mods::healthBarPosition) {
                                case mods::TOP:    tp = ImVec2((ScreenMin.X+ScreenMax.X)/2.0f - ts.x/2, ScreenMin.Y - ts.y - stackTop); stackTop += ts.y + 3; break;
                                case mods::BOTTOM: tp = ImVec2((ScreenMin.X+ScreenMax.X)/2.0f - ts.x/2, ScreenMax.Y + stackBottom); stackBottom += ts.y + 3; break;
                                case mods::LEFT:   tp = ImVec2(ScreenMin.X - ts.x - stackLeft, (ScreenMin.Y+ScreenMax.Y)/2.0f - ts.y/2); stackLeft += ts.x + 5; break;
                                default:           tp = ImVec2(ScreenMax.X + stackRight, (ScreenMin.Y+ScreenMax.Y)/2.0f - ts.y/2); stackRight += ts.x + 5; break;
                                }
                                AddTextWithOutline(BackgroundList, font, fontSize, tp, mods::healthTextColor, mods::healthTextOutlineColor, hStr);
                            }

                            if (mods::bShowUltPercentageText) {
                                auto threatIt2 = s_threatCache.find(Player);
                                if (threatIt2 != s_threatCache.end()) {
                                    char uStr[16];
                                    snprintf(uStr, sizeof(uStr), "%d%%", static_cast<int>(threatIt2->second.UltimatePercentage));
                                    ImVec2 ts = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, uStr);
                                    ImVec2 tp;
                                    switch (mods::ultBarPosition) {
                                    case mods::TOP:    tp = ImVec2((ScreenMin.X+ScreenMax.X)/2.0f - ts.x/2, ScreenMin.Y - ts.y - stackTop); stackTop += ts.y + 3; break;
                                    case mods::BOTTOM: tp = ImVec2((ScreenMin.X+ScreenMax.X)/2.0f - ts.x/2, ScreenMax.Y + stackBottom); stackBottom += ts.y + 3; break;
                                    case mods::LEFT:   tp = ImVec2(ScreenMin.X - ts.x - stackLeft, (ScreenMin.Y+ScreenMax.Y)/2.0f - ts.y/2); stackLeft += ts.x + 5; break;
                                    default:           tp = ImVec2(ScreenMax.X + stackRight, (ScreenMin.Y+ScreenMax.Y)/2.0f - ts.y/2); stackRight += ts.x + 5; break;
                                    }
                                    AddTextWithOutline(BackgroundList, font, fontSize, tp, mods::ultTextColor, mods::ultTextOutlineColor, uStr);
                                }
                            }
                            // ---- Extended Player Info Stack (above box) ----
                            if (mods::bShowHeroNameESP || mods::bShowHeroIcons ||
                                mods::bShowKillsESP || mods::bShowKDRESP ||
                                mods::bShowHealingESP  || mods::bShowKillStreakESP || mods::bShowPlatformESP) {

                                AMarvelPlayerState* PS = static_cast<AMarvelPlayerState*>(Player->PlayerState);
                                if (PS && IsValid(PS)) {
                                    const float lineH = fontSize + 3.0f;
                                    const float cx    = (ScreenMin.X + ScreenMax.X) / 2.0f;

                                    // Check if we should draw an icon for this hero
                                    bool drawIcon = false;
                                    const std::string* iconHeroName = nullptr;
                                    float iconSz = mods::heroIconSize;
                                    if (mods::bShowHeroIcons && HeroIcons::bInitialized) {
                                        auto it = mods::heroIDToName.find(PS->HeroID_Controlled);
                                        if (it != mods::heroIDToName.end()) {
                                            iconHeroName = &it->second;
                                            drawIcon = (HeroIcons::GetIcon(it->second) != nullptr);
                                        }
                                    }

                                    // Build all text lines first so lineCount is known before placing them
                                    struct StatLine { char text[64]; ImColor col; ImColor outline; };
                                    StatLine lines[6];
                                    int lineCount = 0;

                                    if (mods::bShowHeroNameESP && !drawIcon) {
                                        auto it = mods::heroIDToName.find(PS->HeroID_Controlled);
                                        if (it != mods::heroIDToName.end())
                                            snprintf(lines[lineCount++].text, 64, "[%s]", it->second.c_str());
                                        if (lineCount > 0) {
                                            lines[lineCount-1].col     = mods::heroNameESPColor;
                                            lines[lineCount-1].outline = mods::heroNameESPOutline;
                                        }
                                    }

                                    if (mods::bShowKillsESP) {
                                        snprintf(lines[lineCount].text, 64, "K %d  D %d  A %d",
                                            PS->KillScore, PS->DeathScore, PS->AssistScore);
                                        lines[lineCount].col     = mods::killsESPColor;
                                        lines[lineCount].outline = mods::killsESPOutline;
                                        lineCount++;
                                    }

                                    if (mods::bShowKDRESP) {
                                        float kdr = (PS->DeathScore > 0)
                                            ? (float)PS->KillScore / (float)PS->DeathScore
                                            : (float)PS->KillScore;
                                        snprintf(lines[lineCount].text, 64, "KDR %.2f", kdr);
                                        lines[lineCount].col     = mods::kdrESPColor;
                                        lines[lineCount].outline = mods::kdrESPOutline;
                                        lineCount++;
                                    }

                                    if (mods::bShowHealingESP && PS->TotalHeal > 0.0f) {
                                        snprintf(lines[lineCount].text, 64, "Heal %.0f", PS->TotalHeal);
                                        lines[lineCount].col     = mods::healingESPColor;
                                        lines[lineCount].outline = mods::healingESPOutline;
                                        lineCount++;
                                    }

                                    if (mods::bShowKillStreakESP && PS->KillStreak > 1) {
                                        snprintf(lines[lineCount].text, 64, "Streak %d", PS->KillStreak);
                                        lines[lineCount].col     = mods::killStreakESPColor;
                                        lines[lineCount].outline = mods::killStreakESPOutline;
                                        lineCount++;
                                    }

                                    if (mods::bShowPlatformESP) {
                                        bool isController = (PS->ControllerLSScore != 0.0f ||
                                                             PS->ControllerRSScore != 0.0f ||
                                                             PS->ControllerLTScore != 0.0f ||
                                                             PS->ControllerRTScore != 0.0f);
                                        snprintf(lines[lineCount].text, 64, isController ? "Console" : "Windows");
                                        lines[lineCount].col     = isController
                                            ? ImColor(50, 200, 100, 255)
                                            : ImColor(100, 150, 255, 255);
                                        lines[lineCount].outline = ImColor(0, 0, 0, 200);
                                        lineCount++;
                                    }

                                    // Calculate total height: icon row + text rows
                                    float iconRowH = drawIcon ? (iconSz + 4.0f) : 0.0f;
                                    float totalH = iconRowH + lineH * (float)lineCount + 4.0f;
                                    float startY = ScreenMin.Y - totalH;

                                    // Draw hero icon centered above the box
                                    if (drawIcon) {
                                        ImVec2 iconPos = ImVec2(cx - iconSz * 0.5f, startY);
                                        HeroIcons::DrawIcon(BackgroundList, *iconHeroName, iconPos, iconSz,
                                                            mods::bTextBackground, IM_COL32(0, 0, 0, 160));
                                    }

                                    // Draw text lines below the icon (or from the top if no icon)
                                    if (lineCount > 0) {
                                        float textStartY = startY + iconRowH;
                                        for (int li = 0; li < lineCount; li++) {
                                            float rowY = textStartY + li * lineH;
                                            ImVec2 ts = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, lines[li].text);
                                            ImVec2 tp = ImVec2(cx - ts.x * 0.5f, rowY);
                                            if (mods::bTextBackground)
                                                BackgroundList->AddRectFilled(
                                                    ImVec2(tp.x - 4, tp.y - 2),
                                                    ImVec2(tp.x + ts.x + 4, tp.y + ts.y + 2),
                                                    IM_COL32(0, 0, 0, 160));
                                            AddTextWithOutline(BackgroundList, font, fontSize, tp,
                                                lines[li].col, lines[li].outline, lines[li].text);
                                        }
                                    }
                                }
                            }
                        }
                    }

                    if (mods::bSkeletonESP) {
                        DrawSkeleton(BackgroundList, Player, PlayerController);
                    }

                    if (mods::bTracerLines) {
                        ImVec2 startPos;
                        if (mods::tracerStartPos == mods::TRACER_TOP) startPos = ImVec2(displaySize.x / 2, 0);
                        else if (mods::tracerStartPos == mods::TRACER_CENTER) startPos = ImVec2(displaySize.x / 2, displaySize.y / 2);
                        else if (mods::tracerStartPos == mods::TRACER_BOTTOM) startPos = ImVec2(displaySize.x / 2, displaySize.y);
                        BackgroundList->AddLine(startPos, ImVec2(TargetHead2D.X, TargetHead2D.Y), mods::tracerColor);
                    }
                    

                    if (mods::bGlow) {
                        if (mods::bGlowIgnoreTeammates && !bIsEnemy) continue;

                        auto Mesh = Player->GetMesh();
                        if (Mesh && IsValid(Mesh)) {
                            auto status = mods::bGlowThroughWalls ?
                                SDK::ETeamOutlineShowStatus::ETOSS_Always :
                                SDK::ETeamOutlineShowStatus::ETOSS_Unoccluded;
                            Mesh->SetTeamOutlineShowStatus(status);
                        }
                    }
                }

                // pSilent target tracking — runs regardless of aimbot toggle
                if (mods::bPSilentEnabled && bIsEnemy) {
                    float fovDist = sqrtf((TargetHead2D.X - screenCenter.x) * (TargetHead2D.X - screenCenter.x) +
                                         (TargetHead2D.Y - screenCenter.y) * (TargetHead2D.Y - screenCenter.y));
                    if (fovDist < pSilentBestFovDist) {
                        pSilentBestFovDist = fovDist;
                        pSilentCandidate = Player;
                    }
                }

                // Aimbot target collection — only when aimbot is enabled
                if (mods::aimbot && bIsVisible && distance <= mods::aimbotMaxDistance && IsPointInAimbotCircle(mods::actualfovcircle, TargetHead2D)) {
                    if (mods::bAimbotTeamCheck && !bIsEnemy) continue;
                    candidates.push_back({ Player, distance, bIsVisible });
                }

                if (bIsEnemy) {
                    int heroID = Player->HeroID;
                    auto heroNameIt = mods::heroIDToName.find(heroID);
                    if (heroNameIt != mods::heroIDToName.end()) {
                        const std::string& heroName = heroNameIt->second;
                        float ultPercentage = 0.0f;
                        float CurrentHPercent = 0.0f;
                        auto threatIt3 = s_threatCache.find(Player);
                        if (threatIt3 != s_threatCache.end()) {
                            float MaxHP = threatIt3->second.MaxHP;
                            if (MaxHP > 0.0f)
                                CurrentHPercent = (threatIt3->second.CurrentHP / MaxHP) * 100.0f;
                            ultPercentage = threatIt3->second.UltimatePercentage;
                        }
                        mods::enemyOverlayData.push_back({ heroID, ultPercentage, CurrentHPercent });

                        // Populate ult tracker widget data
                        if (mods::bUltTracker && bIsEnemy) {
                            mods::ultTrackerEntries.push_back({ heroID, ultPercentage, ultPercentage >= 100.0f });
                        }

                        // Dodge: notify ult state per enemy
                        AutoKeySystem::NotifyEnemyUlt(heroID, ultPercentage, distance);

                        // Auto Kill Ability: trigger when enemy HP is below threshold and in FOV
                        if (mods::bAutoKeyEnabled && mods::bAutoKillAbility && currentHealth <= mods::autoKillAbilityEnemyHP) {
                            if (IsPointInAimbotCircle(mods::actualfovcircle, TargetHead2D)) {
                                mods::bAutoKillAbTrigger = true;
                            }
                        }
                    }
                }
            }
        }

        // Evict stale chamsCache entries (meshes from dead/disconnected enemies)
        if (mods::bChams) {
            for (auto it = s_chamsCache.begin(); it != s_chamsCache.end(); ) {
                if (s_seenMeshes.find(it->first) == s_seenMeshes.end())
                    it = s_chamsCache.erase(it);
                else
                    ++it;
            }
        }
        // Only run eviction every 120 frames (~2 seconds) to avoid overhead
        if ((s_drawFrame % 120) == 0) {
            // Evict stale previousHealthMap entries (dead/disconnected players)
            for (auto it = previousHealthMap.begin(); it != previousHealthMap.end(); ) {
                if (s_seenPlayers.find(it->first) == s_seenPlayers.end())
                    it = previousHealthMap.erase(it);
                else
                    ++it;
            }
            // Evict stale LOS cache entries
            for (auto it = s_losCache.begin(); it != s_losCache.end(); ) {
                if (s_seenPlayers.find(it->first) == s_seenPlayers.end())
                    it = s_losCache.erase(it);
                else
                    ++it;
            }
        }

        // Flush pending dodge key presses
        AutoKeySystem::FlushDodge();

        // Adaptive FOV logic
        if (mods::bAdaptiveFov && mods::closestDistance < mods::adaptiveFarDistance) {
            float baseRadius = static_cast<float>(mods::fov) * Variables::ScreenSize.X / static_cast<float>(mods::fov_changer_amount) / 2.0f;
            float t = 0.0f;
            if (mods::adaptiveFarDistance > mods::adaptiveCloseDistance) {
                t = (mods::closestDistance - mods::adaptiveCloseDistance) / (mods::adaptiveFarDistance - mods::adaptiveCloseDistance);
                if (t < 0.0f) t = 0.0f;
                if (t > 1.0f) t = 1.0f;
            }
            float scale = mods::adaptiveFovScaleClose + t * (mods::adaptiveFovScaleFar - mods::adaptiveFovScaleClose);
            mods::actualfovcircle = baseRadius * scale;
            if (mods::bShowAdaptiveFov) {
                ImVec2 sc = ImVec2(ImGui::GetIO().DisplaySize.x / 2, ImGui::GetIO().DisplaySize.y / 2);
                BackgroundList->AddCircle(sc, mods::actualfovcircle, IM_COL32(199, 43, 54, 80), 64, 1.0f);
            }
        }

        static AMarvelBaseCharacter* s_hardLockTarget = nullptr;

        AMarvelBaseCharacter* TargetPlayer = nullptr;
        
        if (!candidates.empty()) {
            // Fixed target priority: visible first, then nearest world
            // distance. The selected aim point below is always the head.
            std::stable_sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
                if (a.visible != b.visible)
                    return a.visible > b.visible;
                return a.distance < b.distance;
            });
            TargetPlayer = candidates.front().player;

            // Hard Lock: update lock whenever we have a valid in-FOV target
            if (mods::bHardLock)
                s_hardLockTarget = TargetPlayer;

            Variables::TargetPlayerPTR = TargetPlayer;

            if (mods::bAimbotSnapLine) {
                FName BoneFName = GetAimBoneName();
                if (TargetPlayer && TargetPlayer->GetMesh() && TargetPlayer->GetMesh()->DoesSocketExist(BoneFName)) {
                    FVector TargetBone3D = TargetPlayer->GetMesh()->GetSocketLocation(BoneFName);// TargetPlayer->GetMesh()->GetSocketLocation(BoneFName);
                    SDK::FVector2D TargetBone2D;
                    if (PlayerController->ProjectWorldLocationToScreen(TargetBone3D, &TargetBone2D, true)) {
                        ImVec2 targetPos = ImVec2(TargetBone2D.X, TargetBone2D.Y);
                        BackgroundList->AddLine(screenCenter, targetPos, mods::snapLineColor, mods::snapLineThickness);
                    }
                }
            }
        }
        else {
            // Do not revive a stale hard-lock target. A selection only exists
            // when it was ranked this frame by visible -> world distance.
            s_hardLockTarget = nullptr;
            Variables::TargetPlayerPTR = (mods::bPSilentEnabled && pSilentCandidate) ? pSilentCandidate : nullptr;
        }


        if (mods::bRapidFire && PlayerController && AcknowledgedPawn) {
            if (AMarvelBaseCharacter* Character = reinterpret_cast<AMarvelBaseCharacter*>(AcknowledgedPawn)) {
                if (UEquipComponent* EquipComponent = Character->EquipComponent) {
                    if (EquipComponent && UKismetSystemLibrary::IsValid(EquipComponent)) {
                        if (AShootingWeapon* Weapon = EquipComponent->GetCurrentWeapon()) {
                            const auto& LogiBase = Weapon->ShootingLogics; if (LogiBase.Num() > 0) {
                                for (UShootingLogic_Base* logic : LogiBase) {
                                    logic->ThisFireTime = mods::rapidFireRate;
                                    logic->NextFireTime = mods::rapidFireRate;
                                    logic->TimeBetweenShots = mods::rapidFireRate;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (mods::TriggerBot) {
            static uint64_t s_lastTriggerMs = 0;
            uint64_t nowMs = GetTickCount64();

            // Rate-limit automatic trigger input to one click every 20ms.
            if (nowMs - s_lastTriggerMs >= 20) {
                FVector traceStart = PlayerCameraManager->GetCameraLocation();
                // Use the rendered camera orientation, not the camera actor's
                // transform, which can differ during third-person offsets,
                // camera lag, or view-target transitions.
                FVector fwd = UKismetMathLibrary::GetForwardVector(
                    PlayerCameraManager->GetCameraRotation());
                FVector traceEnd = traceStart + fwd * (mods::TriggerBotDistance > 0.f ? mods::TriggerBotDistance : 999999.f);

                FHitResult HitResult;
                static TArray<AActor*> ignoreActors; // static: reuse allocation, always empty
                bool bHit = UKismetSystemLibrary::LineTraceSingle(
                    aWorld,
                    traceStart,
                    traceEnd,
                    ETraceTypeQuery::TraceTypeQuery3,  // Pawn channel
                    false,
                    ignoreActors,
                    EDrawDebugTrace::None,
                    &HitResult,
                    true,
                    FLinearColor(1.f, 0.f, 0.f, 1.f),
                    FLinearColor(0.f, 1.f, 0.f, 1.f),
                    0.f);

                if (bHit && HitResult.bBlockingHit) {
                    UPrimitiveComponent* comp = HitResult.Component.Get();
                    AActor* hitActor = comp ? comp->GetOwner() : nullptr;

                    if (hitActor && IsValid(hitActor) && hitActor->IsA(ClassToFind)) {
                        AMarvelBaseCharacter* hitChar = reinterpret_cast<AMarvelBaseCharacter*>(hitActor);

                        // Keep triggerbot off the local player and teammates. The S10
                        // dump keeps Pawn::PlayerState at 0x728 and
                        // MarvelPlayerState::OriginalTeamID at 0x838.
                        bool bIsEnemy = true;
                        AMarvelPlayerState* hitPS   = static_cast<AMarvelPlayerState*>(hitChar->PlayerState);
                        AMarvelPlayerState* localPS = static_cast<AMarvelPlayerState*>(AcknowledgedPawn->PlayerState);
                        if (hitPS && localPS && hitPS->OriginalTeamID == localPS->OriginalTeamID)
                            bIsEnemy = false;

                        if (!hitChar->IsLocallyControlled() && bIsEnemy && hitChar->GetCurrentHealth() > 0.f) {
                            // Trigger tracing may request fire, but it never
                            // writes aim coordinates. The periodic ranked-head
                            // selector is the only AHK coordinate publisher.
                            TriggerOnly::RequestExternalLeftClick();
                            s_lastTriggerMs = nowMs;
                        }
                    }
                }
            }
        }

        if (GetAsyncKeyState(mods::aimbotKey) && TargetPlayer && IsValid(TargetPlayer) && TargetPlayer->GetMesh() && IsValid(TargetPlayer->GetMesh())) {
            if (mods::aimbot) {
                FName BoneFName = GetAimBoneName();
                if (!TargetPlayer->GetMesh()->DoesSocketExist(BoneFName)) {
                    Log("DrawTransition: Aimbot - Target bone does not exist\n");
                    return;
                }

                FVector TargetBone3D = mods::bAimPrediction ?
                    PredictTargetPosition(TargetPlayer, mods::projectileSpeed) :
                    TargetPlayer->GetMesh()->GetSocketLocation(BoneFName);
                TargetBone3D.Z += mods::aimOffset;

                if (mods::bAimHumanizer) {
                    float offset = mods::humanizerLevel;
                    FVector randomOffset = FVector(
                        (float)rand() / RAND_MAX * offset - offset / 2,
                        (float)rand() / RAND_MAX * offset - offset / 2,
                        (float)rand() / RAND_MAX * offset - offset / 2
                    );
                    TargetBone3D += randomOffset;
                }

                float DeltaTime = UGameplayStatics::GetWorldDeltaSeconds(aWorld);

                FRotator TargetRotation = UKismetMathLibrary::FindLookAtRotation(PlayerCameraManager->GetCameraLocation(), TargetBone3D);

                float SpeedFactor = UKismetMathLibrary::Clamp(mods::smoothing, 1.0f, 200.0f) * 0.5f;

                FRotator CurrentRotation = PlayerController->GetControlRotation();

                FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(TargetRotation, CurrentRotation);

                float NewPitch = UKismetMathLibrary::FInterpTo(CurrentRotation.Pitch, CurrentRotation.Pitch + DeltaRot.Pitch, DeltaTime, SpeedFactor);
                float NewYaw = UKismetMathLibrary::FInterpTo(CurrentRotation.Yaw, CurrentRotation.Yaw + DeltaRot.Yaw, DeltaTime, SpeedFactor);

                PlayerController->SetControlRotation(FRotator(NewPitch, NewYaw, 0));

            }
        }

        // Silent Aim (Rage Mode) - snap rotation for one frame then restore
        if (mods::bRageMode && mods::bSilentAim && TargetPlayer && IsValid(TargetPlayer) && TargetPlayer->GetMesh() && IsValid(TargetPlayer->GetMesh())) {
            bool shouldSilent = true;
            if (mods::bSilentHealingOnly) {
                // Only activate if target is a healer (check heroIDToName)
                int tHeroID = TargetPlayer->HeroID;
                auto it = mods::heroIDToName.find(tHeroID);
                if (it != mods::heroIDToName.end()) {
                    const std::string& n = it->second;
                    if (n != "Adam Warlock" && n != "Loki" && n != "Cloak & Dagger" && n != "Jeff" && n != "Ultron" && n != "Mantis" && n != "Luna Snow" && n != "Rocket Raccoon") {
                        shouldSilent = false;
                    }
                }
            }
            if (shouldSilent) {
                // Check silent FOV
                SDK::FVector2D tHead2D;
                static FName HeadBoneName = UKismetStringLibrary::Conv_StringToName(SDK::FString(L"Head"));
                FVector tHead3D = TargetPlayer->GetMesh()->GetSocketLocation(HeadBoneName);
                if (PlayerController->ProjectWorldLocationToScreen(tHead3D, &tHead2D, true)) {
                    float dx = tHead2D.X - screenCenter.x;
                    float dy = tHead2D.Y - screenCenter.y;
                    float distPx = sqrtf(dx * dx + dy * dy);
                    float silentRadius = mods::silentMagicFov * Variables::ScreenSize.X / (float)mods::fov_changer_amount / 2.0f;
                    if (distPx <= silentRadius) {
                        FRotator savedRot = PlayerController->GetControlRotation();
                        FRotator silentRot = UKismetMathLibrary::FindLookAtRotation(PlayerCameraManager->GetCameraLocation(), tHead3D);
                        PlayerController->SetControlRotation(silentRot);
                        // Fire
                        TriggerOnly::RequestExternalLeftClick();
                        // Restore rotation
                        PlayerController->SetControlRotation(savedRot);
                    }
                }
            }
        }

        // Show silent FOV circle
        if (mods::bRageMode && mods::bShowFovSilent) {
            float silentRadius = mods::silentMagicFov * Variables::ScreenSize.X / (float)mods::fov_changer_amount / 2.0f;
            BackgroundList->AddCircle(screenCenter, silentRadius, IM_COL32(255, 165, 0, 120), 64, 1.0f);
        }

        // Distance filter for ESP (apply after drawing)
        // Already handled by espMaxDistance, but distanceFilter provides additional UI-level filtering

        // ===== Projectile exploits (needs nearest enemy) =====
        if (TargetPlayer && IsValid(TargetPlayer) && PlayerController->Character && IsValid(PlayerController->Character)) {
            AMarvelBaseCharacter* localChar = reinterpret_cast<AMarvelBaseCharacter*>(PlayerController->Character);
            Exploits::ApplyProjectileExploits(aWorld, localChar, TargetPlayer);
        }

        // ===== Draw all overlay widgets =====
        FVector localDrawPos = AcknowledgedPawn->K2_GetActorLocation();
        Features::DrawOverlayWidgets(ForegroundList, font, screenCenter, localDrawPos, Variables::CameraRotation);
        Features::CheckHealSnipeAlert(ForegroundList, screenCenter);

    }
    catch (const std::exception& e) {
        Log("DrawTransition: Exception caught - %s\n", e.what());
    }
}
