#pragma once
#include <d3d12.h>
#include <string>
#include <vector>
#include "../../ThirdParty/ImGui/imgui.h"
#include "../Icons/IconSystem.h"
#include "../../global.h"

namespace MatchStatus {

    // ── State ─────────────────────────────────────────────────────────
    enum class State { NONE, QUEUING, CONNECTING };

    static State        s_state       = State::NONE;
    static std::string  s_mapName;
    static std::string  s_modeName;
    static float        s_connectTime = 0.0f;
    static float        s_showDuration = 14.0f;

    static std::string  s_lastWorldName;

    // Dot animation for QUEUING state
    static float s_dotTimer = 0.0f;
    static int   s_dotCount = 1;

    // ── Map info table ────────────────────────────────────────────────
    struct MapInfo { const char* display; const char* bannerFile; };

    static const MapInfo* FindMap(const std::string& worldName) {
        static const struct { const char* key; MapInfo info; } table[] = {
            { "Midtown",    { "Midtown",                "Midtown.webp"                       } },
            { "Yggdrasil",  { "Yggdrasil",              "Yggdrasil.webp"                     } },
            { "Krakoa",     { "Krakoa",                 "Krakoa Carousel.webp"               } },
            { "Wakanda",    { "Wakanda",                "Wakanda.webp"                       } },
            { "Sanctum",    { "Sanctum Sanctorum",      "Sanctum Sanctorum.webp"             } },
            { "Tokyo",      { "Tokyo Web World",        "Tokyo Web World Metropolis.webp"    } },
            { "Hydra",      { "Hydra Base",             "Hydra Charteris Base.webp"          } },
            { "Archive",    { "The Archive",            "Archive.webp"                       } },
            { "Celestial",  { "Celestial",              "Celestial Hand.webp"                } },
            { "Central",    { "Central Park",           "Central Park.png"                   } },
            { "Golden",     { "Golden City",            "Golden City Warrior Falls.webp"     } },
            { "Dialia",     { "Hall of Dialia",         "Hall of Dialia.webp"                } },
            { "Hellfire",   { "Hellfire Gala",          "Hellfire Gala Arakko.webp"          } },
            { "Klyntar",    { "Klyntar",                "Klyntar Ruins.png"                  } },
            { "Thorny",     { "Thorny Jungle",          "Thorny Jungle.webp"                 } },
            { "Practice",   { "Practice Range",         "Practicerange.webp"                 } },
        };
        for (const auto& e : table)
            if (worldName.find(e.key) != std::string::npos)
                return &e.info;
        return nullptr;
    }

    static bool IsLobby(const std::string& w) {
        return w.find("Frontend") != std::string::npos
            || w.find("MainHub")  != std::string::npos
            || w.find("Lobby")    != std::string::npos
            || w.find("Menu")     != std::string::npos
            || w.find("Entry")    != std::string::npos
            || w.find("Hub")      != std::string::npos;
    }

    static const char* ModeStr(int id) {
        switch (id) {
        case 1:  return "Domination";
        case 2:  return "Convergence";
        case 3:  return "Flash Point";
        case 4:  return "Competitive";
        default: return "Standard";
        }
    }

    // ── Init (signature kept for compatibility; no GPU work is needed) ─
    static void Init(ID3D12Device*, ID3D12DescriptorHeap*, ID3D12CommandQueue*) {
    }

    // ── Called each frame from DrawAllWidgets ─────────────────────────
    static void UpdateFromWorld(SDK::UWorld* world, int modeId = 0) {
        if (!world) {
            if (s_state != State::QUEUING) {
                s_state = State::QUEUING;
                s_mapName.clear(); s_modeName.clear();
            }
            return;
        }

        // PERF: only call GetName() when the world pointer changes (avoids FString->std::string alloc per frame)
        static SDK::UWorld* s_lastWorldPtr = nullptr;
        if (world == s_lastWorldPtr) {
            if (s_state == State::CONNECTING) {
                float el = (float)ImGui::GetTime() - s_connectTime;
                if (el > s_showDuration) s_state = State::NONE;
            }
            return;
        }
        s_lastWorldPtr = world;
        std::string wn = world->GetName();
        s_lastWorldName = wn;

        if (IsLobby(wn)) {
            s_state = State::QUEUING;
            s_mapName.clear(); s_modeName.clear();
        } else {
            const MapInfo* info = FindMap(wn);
            s_mapName  = info ? info->display : "Loading Map...";
            s_modeName = ModeStr(modeId);
            s_state    = State::CONNECTING;
            s_connectTime = (float)ImGui::GetTime();
        }
    }

    // ── Draw (called from DrawAllWidgets) ─────────────────────────────
    static void Draw() {
        if (!mods::bMatchStatusWidget) return;
        if (s_state == State::NONE) return;

        ImDrawList* dl = ImGui::GetForegroundDrawList();
        ImVec2 disp = ImGui::GetIO().DisplaySize;

        constexpr float W = 360.0f;
        float H = (s_state == State::CONNECTING) ? 80.0f : 62.0f;
        ImVec2 pos(disp.x / 2.0f - W / 2.0f, 8.0f);

        // Body
        dl->AddRectFilled(pos, ImVec2(pos.x + W, pos.y + H), IM_COL32(8, 8, 12, 235));
        // Left accent strip
        dl->AddRectFilled(pos, ImVec2(pos.x + 3.0f, pos.y + H), IM_COL32(199, 43, 54, 255));
        // Outer border
        dl->AddRect(pos, ImVec2(pos.x + W, pos.y + H), IM_COL32(199, 43, 54, 90), 0.0f);
        // Subtle inner top highlight line
        dl->AddLine(ImVec2(pos.x + 3, pos.y + 1), ImVec2(pos.x + W - 1, pos.y + 1),
            IM_COL32(255, 255, 255, 12));

        if (s_state == State::QUEUING) {
            s_dotTimer += ImGui::GetIO().DeltaTime;
            if (s_dotTimer > 0.45f) { s_dotTimer = 0.0f; s_dotCount = (s_dotCount % 3) + 1; }
            char dots[5] = {};
            for (int i = 0; i < s_dotCount; i++) strcat_s(dots, ".");

            char title[48]; snprintf(title, sizeof(title), "MATCH QUEUE STARTED");
            char sub[48];   snprintf(sub,   sizeof(sub),   "Waiting for info%s", dots);

            ImVec2 ts = ImGui::CalcTextSize(title);
            ImVec2 ss = ImGui::CalcTextSize(sub);
            dl->AddText(ImVec2(pos.x + W * 0.5f - ts.x * 0.5f, pos.y + 9.0f),
                IM_COL32(199, 43, 54, 255), title);
            dl->AddText(ImVec2(pos.x + W * 0.5f - ss.x * 0.5f, pos.y + 30.0f),
                IM_COL32(155, 155, 162, 210), sub);

        } else if (s_state == State::CONNECTING) {
            char title[32]; snprintf(title, sizeof(title), "CONNECTING");
            char mapLn[128];
            if (!s_modeName.empty() && s_modeName != "Standard")
                snprintf(mapLn, sizeof(mapLn), "%s  /  %s", s_mapName.c_str(), s_modeName.c_str());
            else
                snprintf(mapLn, sizeof(mapLn), "%s", s_mapName.c_str());

            ImVec2 ts = ImGui::CalcTextSize(title);
            ImVec2 ms = ImGui::CalcTextSize(mapLn);
            dl->AddText(ImVec2(pos.x + W * 0.5f - ts.x * 0.5f, pos.y + 8.0f),
                IM_COL32(199, 43, 54, 255), title);
            dl->AddText(ImVec2(pos.x + W * 0.5f - ms.x * 0.5f, pos.y + 27.0f),
                IM_COL32(220, 220, 225, 255), mapLn);
        }
    }

    // ── Cleanup ───────────────────────────────────────────────────────
    static void Cleanup() {
        s_state = State::NONE;
    }
}
