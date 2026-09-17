#pragma once
#include "ThirdParty/ImGui/imgui.h"
#include "ThirdParty/ImGui/imgui_internal.h"
#include "ThirdParty/ImGui/imgui_settings.h"

// Font globals - defined in main.cpp, loaded during ImGui init
namespace font {
    extern ImFont* icomoon;
    extern ImFont* weapon_val;
    extern ImFont* calibri_bold;
    extern ImFont* calibri_regular;
    extern ImFont* icomoon_menu;
    extern ImFont* pixel_7_small;
    extern ImFont* calibri_bold_hint;
}
namespace font_inter {
    extern ImFont* inter_bold;
}

// ============================================================
// Thin compatibility wrappers: old PubgWidgets.h names -> PUBG ImGui calls
// ============================================================

inline void PubgToggle(const char* label, bool* v) {
    // Strip ##id suffix from hint text so it doesn't display in the UI
    static char hintBuf[256];
    const char* hashPos = strstr(label, "##");
    if (hashPos && hashPos == label) {
        // Label starts with ## — no visible text, pass empty hint
        ImGui::CheckboxWishTips(label, v, "");
    } else if (hashPos) {
        size_t len = (size_t)(hashPos - label);
        if (len >= sizeof(hintBuf)) len = sizeof(hintBuf) - 1;
        memcpy(hintBuf, label, len);
        hintBuf[len] = '\0';
        ImGui::CheckboxWishTips(label, v, hintBuf);
    } else {
        ImGui::CheckboxWishTips(label, v, label);
    }
}

inline bool PubgSliderInt(const char* label, int* v, int v_min, int v_max, const char* format = "%d") {
    return ImGui::SliderInt1(label, v, v_min, v_max, format);
}

inline bool PubgSliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.3f") {
    return ImGui::SliderFloat1(label, v, v_min, v_max, format);
}

inline bool PubgCombo(const char* label, int* v, const char* const items[], int count) {
    return ImGui::Combo_popup(label, v, items, count);
}

inline void PubgBeginChild(const char* title, ImVec2 size = ImVec2(0, 0)) {
    ImGui::BeginChild(true, title, "o", size);
}

inline void PubgEndChild() {
    ImGui::EndChild(true);
}

inline bool PubgTab(bool selected, const char* label) {
    return ImGui::Tabs(selected, "", label);
}

inline bool PubgTabs(bool selected, const char* label) {
    return ImGui::Tabs(selected, "", label);
}

inline bool PubgButton(const char* label) {
    return ImGui::Button(label);
}

inline void PubgSectionHeader(const char* /*text*/) {
    // No-op: PUBG menu doesn't use section headers
}

inline void PubgKeybind(const char* label, int* key) {
    ImGui::Keybind(label, key, true);
}

inline void PubgColorEdit4(const char* label, float col[4], ImGuiColorEditFlags flags = 0) {
    ImGui::ColorEdit5(label, col, flags);
}

// HelpMarker and CreateModernStyle are defined in main.cpp
