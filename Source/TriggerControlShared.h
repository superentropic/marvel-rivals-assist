#pragma once

#include <Windows.h>
#include <cstddef>

namespace TriggerControlShared {
    // Versioned name prevents an old loaded DLL from being mistaken for this producer.
    inline constexpr wchar_t MappingName[] = L"Local\\MarvelTriggerControlStateV4";
    inline constexpr LONG Magic = 0x4D544132;

    struct State {
        volatile LONG magic;
        volatile LONG triggerEnabled; // master enable; AHK starts and controls it OFF
        volatile LONG status;
        volatile LONG clickRequest; // independent firing request counter
        volatile LONG clickAcknowledged;
        volatile LONG aimX;
        volatile LONG aimY;
        volatile LONG aimActive;
        volatile LONG aimFovPx;
        volatile LONG aimUpdate;     // monotonic selected-head refresh counter
        volatile LONG aimHeightPct;  // AHK target height, 75..100 percent
        volatile LONG aimSmoothing;  // AHK smoothing, 0..100 (0 is strongest lock)
    };

    static_assert(sizeof(State) == 48);
    static_assert(offsetof(State, aimFovPx) == 32);
    static_assert(offsetof(State, aimUpdate) == 36);
    static_assert(offsetof(State, aimHeightPct) == 40);
    static_assert(offsetof(State, aimSmoothing) == 44);

    inline bool RequestLeftClick(State& state) {
        const LONG request = InterlockedIncrement(&state.clickRequest);
        return request != 0;
    }
}
