#pragma once

// Community config sharing removed.
//
// This header previously implemented a "Community Configs" system that talked
// to an external file repository (downloading shared configs from, and
// uploading the user's configs to, a third-party host using an embedded
// access token). Sharing user configs with / pulling configs from an
// external service is not permitted, so the entire network layer has been
// removed.
//
// Local config save/load on the user's own disk is unaffected and remains
// available through ConfigSystem (configs\ directory next to the DLL).
//
// main.cpp calls CommunityConfigs::Tick() once per frame; it is kept as a
// no-op so the rest of the project still compiles and links unchanged.

namespace CommunityConfigs {

    static void Tick() {
        // Nothing to apply — no configs are fetched from anywhere.
    }
}
