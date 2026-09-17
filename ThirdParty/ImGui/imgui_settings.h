#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "imgui.h"
#include "string"
using namespace ImGui;

// ========== 全局变量和设置 ==========
inline int menu_state = 0;
inline char search_all_widgets[120] = { "" };
inline std::string search_all_widgets_;
inline float anim_speed;
inline int rotation_start_index;

// ========== 字体定义 ==========
inline ImFont* default_font;
inline ImFont* big_icon;
inline ImFont* big_font;
inline ImFont* icon_font;
inline ImFont* medium_icon_font;
inline ImFont* bold_font;
inline ImFont* bold_big_font;
inline ImFont* icon_small;
inline ImFont* dot_font;

// ========== 工具函数 ==========
inline float random_float(float min, float max)
{
    return min + float(rand() / float(RAND_MAX)) * (max - min);
}

inline void rect_glow(ImDrawList* draw, ImVec2 start, ImVec2 end, ImColor col, float rounding, float intensity) {
    while (true) {
        if (col.Value.w < 0.0019f)
            break;

        draw->AddRectFilled(start, end, col, rounding);

        col.Value.w -= col.Value.w / intensity;
        start = ImVec2(start.x - 1, start.y - 1);
        end = ImVec2(end.x + 1, end.y + 1);
    }
}

inline void ImRotateStart()
{
    rotation_start_index = ImGui::GetWindowDrawList()->VtxBuffer.Size;
}

inline ImVec2 center_text(ImVec2 min, ImVec2 max, const char* text)
{
    return min + (max - min) / 2 - ImGui::CalcTextSize(text) / 2;
}

inline ImVec2 ImRotationCenter()
{
    ImVec2 l(FLT_MAX, FLT_MAX), u(-FLT_MAX, -FLT_MAX);

    const auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
    for (int i = rotation_start_index; i < buf.Size; i++)
        l = ImMin(l, buf[i].pos), u = ImMax(u, buf[i].pos);

    return ImVec2((l.x + u.x) / 2, (l.y + u.y) / 2);
}

inline ImVec4 ImColorToImVec4(const ImColor& color)
{
    return ImVec4(color.Value.x, color.Value.y, color.Value.z, color.Value.w);
}

inline void ImRotateEnd(float rad, ImVec2 center = ImRotationCenter())
{
    float s = sin(rad), c = cos(rad);
    center = ImRotate(center, s, c) - center;

    auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
    for (int i = rotation_start_index; i < buf.Size; i++)
        buf[i].pos = ImRotate(buf[i].pos, s, c) - center;
}

// 确保包含必要的头文件
#include <map>

inline bool button_text(const char* first_text, const char* label)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;
    ImVec2 pos = window->DC.CursorPos;
    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    // 使用静态map存储每个按钮的状态
    static std::map<ImGuiID, std::pair<ImVec4, ImVec4>> button_colors;

    // 如果这个ID的按钮还没有状态，则初始化
    auto it = button_colors.find(id);
    if (it == button_colors.end()) {
        button_colors[id] = std::make_pair(ImVec4(0, 0, 0, 0), ImVec4(0, 0, 0, 0));
        it = button_colors.find(id);
    }

    // 获取当前按钮的颜色状态（使用引用）
    ImVec4& color_text = it->second.first;
    ImVec4& color_shadow = it->second.second;

    ImVec2 size = CalcTextSize(label);
    ImVec2 first_text_size = CalcTextSize(first_text);
    const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    const ImRect bb_text(ImVec2(pos.x + first_text_size.x, pos.y),
        ImVec2(pos.x + first_text_size.x + size.x, pos.y + size.y));
    ItemSize(size, style.FramePadding.y);
    ItemAdd(bb, id);
    bool hovered, held;
    bool pressed = ButtonBehavior(bb_text, id, &hovered, &held, 0);

    // 使用第一个主题的主色调
    ImColor main_color(80, 129, 201, 255);
    ImColor text_color[2]{ ImColor(214, 214, 214, 255), ImColor(214, 214, 214, 65) };

    // 更新当前按钮的颜色
    color_text = ImLerp(color_text, hovered ? main_color : text_color[0], anim_speed);
    color_shadow = ImLerp(color_shadow, hovered ? ImColor(main_color.Value.x, main_color.Value.y, main_color.Value.z, 0.1f) : ImColor(main_color.Value.x, main_color.Value.y, main_color.Value.z, 0.f), anim_speed);

    rect_glow(window->DrawList,
        ImVec2(pos.x + first_text_size.x, pos.y + 2),
        ImVec2(pos.x + first_text_size.x + size.x, pos.y + size.y - 2),
        color_shadow, 360.f, 5.f);
    window->DrawList->AddText(pos, text_color[1], first_text);
    window->DrawList->AddText(ImVec2(pos.x + first_text_size.x, pos.y), GetColorU32(color_text), label);

    return pressed;
}

// ========== 主题配色方案 ==========

// 主题 A - 蓝紫色调主题
namespace theme_a {
    inline ImColor main_color(80, 129, 201, 255);
    inline ImColor main_color_shadow(80, 129, 201, 170);
    inline ImColor main_color2(150, 125, 200, 120);
    inline ImColor main_color_outline(150, 125, 200, 255);
    inline ImColor accent = ImColor(112, 110, 215);

    inline ImColor background_color(22, 28, 41, 255);
    inline ImColor second_color(20, 20, 20, 255);
    inline ImColor winbg(16, 17, 23, 255);

    inline ImColor text_color[2]{ ImColor(214, 214, 214, 255), ImColor(214, 214, 214, 65) };
    inline ImVec4 inputtext_color[3] = { background_color, main_color, text_color[0] };

    namespace background {
        inline ImVec4 filling = ImColor(12, 12, 12);
        inline ImVec4 stroke = ImColor(24, 26, 36);
        inline ImVec4 left = ImColor(206, 206, 206);
        inline ImVec4 left_glow = ImColor(40, 40, 40);
        inline ImVec4 top = ImColor(42, 40, 42);
        inline ImVec4 bottom = ImColor(38, 36, 38);
        inline ImVec2 size = ImVec2(1050, 780);
        inline ImVec2 Loginsize = ImVec2(600, 400);
        inline float rounding = 12;
    }

    namespace checkbox {
        inline ImVec4 circle_inactive = ImColor(43, 48, 54, 255);
        inline ImVec4 background = ImColor(27, 29, 32, 255);
        inline ImVec4 outline_background = ImColor(30, 32, 36, 255);
        inline float rounding = 4;
    }

    namespace elements {
        inline ImVec4 mark = ImColor(255, 255, 255);
        inline ImVec4 child_bg = ImColor(30, 29, 31);
        inline ImVec4 child_top = ImColor(40, 39, 40);
        inline ImVec4 stroke = ImColor(28, 26, 37);
        inline ImVec4 background = ImColor(15, 15, 17);
        inline ImVec4 background_hov = ImColor(50, 50, 50);
        inline ImVec4 background_widget = ImColor(21, 23, 26);
        inline ImVec4 background_widget_stroke = ImColor(63, 63, 63);
        inline ImVec4 checkbox = ImColor(206, 206, 206);
        inline ImVec4 checkbox_active = ImColor(0, 0, 0);
        inline ImVec4 combo_stroke = ImColor(46, 46, 46);
        inline ImVec4 text_active = ImColor(255, 255, 255);
        inline ImVec4 text_hov = ImColor(81, 92, 109);
        inline ImVec4 text = ImColor(43, 51, 63);
        inline ImVec4 tab_active = ImColor(45, 43, 45, 200);
        inline ImVec4 slider_bg = ImColor(139, 138, 139);
        inline float rounding = 4;
    }

    namespace tab {
        inline ImVec4 tab_active_child = ImColor(45, 45, 45);
        inline ImVec4 tab_hov_child = ImColor(45, 45, 45);
        inline ImVec4 tab_child_active = ImColor(39, 39, 39);
        inline ImVec4 tab_active = ImColor(45, 45, 45);
        inline ImVec4 tab_hov = ImColor(45, 45, 45);
        inline ImVec4 tab = ImColor(45, 45, 45, 0);
        inline ImVec4 acc_active = ImColor(255, 255, 255, 60);
        inline ImVec4 acc_hov = ImColor(255, 255, 255, 50);
        inline ImVec4 border = ImColor(14, 14, 15);
    }
}

namespace c {

	inline ImVec4 accent = ImColor(130, 143, 234);
	inline ImVec4 accent1 = ImColor(64, 128, 255, 255);
	inline ImVec4 shadow = ImColor(2, 2, 2);
	inline ImVec4 accent_low = ImColor(64, 152, 254, 150);
	inline ImVec4 logocolor = ImColor(255, 64, 64, 255);

	inline ImVec4 white_light = ImColor(201, 201, 201);
	inline ImVec4 image = ImColor(255, 255, 255, 255);
	inline ImVec4 accent_transparent = ImColor(64, 152, 254, 0);

	namespace bg
	{
		inline ImVec4 background = ImColor(16, 16, 16, 255);
		inline ImVec4 roughness = ImColor(255, 255, 255, 15);
		inline ImVec4 outline = ImColor(38, 38, 38, 255);
		inline ImVec4 top_bg = ImColor(22, 22, 27, 255);

		inline ImVec4 background1 = ImColor(0, 0, 0, 230);

		inline ImVec4 gradient_line0 = ImColor(57, 58, 64, 255);
		inline ImVec4 gradient_line1 = ImColor(57, 58, 64, 0);

		inline ImVec2 size = ImVec2(1160, 770);

		inline ImVec2 size1 = ImVec2(500, 650);
		inline float rounding1 = 8.f;

		inline float rounding = 6;
	}

	namespace child
	{
		inline ImVec4 background = ImColor(20, 20, 20, 255);
		inline ImVec4 border = ImColor(26, 26, 26, 255);
		inline ImVec4 lines = ImColor(36, 36, 36, 255);
		inline float rounding = 4;
	}

	namespace popup_elements
	{
		inline ImVec4 filling = ImColor(10, 10, 10, 170);
		inline ImVec4 cog = ImColor(255, 255, 255, 255);
	}

	namespace checkbox
	{
		inline ImVec4 background = ImColor(26, 26, 26, 255);
		inline ImVec4 outline = ImColor(35, 35, 35, 255);
		inline ImVec4 mark = ImColor(0, 0, 0, 255);
		inline float rounding = 2;


		inline ImVec4 background2 = ImColor(26, 26, 26, 255);
		inline ImVec4 outline2 = ImColor(35, 35, 35, 255);
		inline ImVec4 mark2 = ImColor(0, 0, 0, 255);
		inline float rounding2 = 40;
		inline float rounding3 = 4;

	}



	namespace slider
	{
		inline ImVec4 background = ImColor(26, 26, 26, 255);
		inline float rounding = 4;
	}

	namespace button
	{
		inline ImVec4 background = ImColor(26, 26, 26, 255);
		inline ImVec4 outline = ImColor(36, 36, 36, 255);
		inline float rounding = 10.f;
		inline float rounding1 = 4.f;

	}

	namespace combo
	{
		inline ImVec4 background = ImColor(26, 26, 26, 255);
		inline ImVec4 outline = ImColor(36, 36, 36, 255);
		inline float rounding = 10.f;
	}

	namespace keybind
	{
		inline ImVec4 background = ImColor(26, 26, 26, 255);
		inline float rounding = 10.f;
	}

	namespace input
	{
		inline ImVec4 background = ImColor(26, 26, 26, 255);
		inline ImVec4 outline = ImColor(36, 36, 36, 255);
		inline float rounding = 0;
		inline ImVec4 background1 = ImColor(30, 31, 36, 245);
		inline ImVec4 outline1 = ImColor(48, 52, 65, 245);

		inline float rounding1 = 4.f;
	}

	namespace exit_panel
	{
		inline ImVec4 background1 = ImColor(30, 31, 36, 245);
		inline ImVec4 outline = ImColor(48, 52, 65, 245);
	}

	namespace picker
	{
		inline ImVec4 background = ImColor(26, 26, 26, 255);
		inline float rounding = 2;
	}

	namespace tabs
	{
		inline ImVec4 line = ImColor(36, 36, 36, 255);

	}

	namespace knobs
	{
		inline ImVec4 background = ImColor(32, 32, 34, 255);

	}

	namespace checkbox1
	{
		inline ImVec4 checkmark_active = ImColor(255, 255, 255, 245);
		inline ImVec4 checkmark_inactive = ImColor(255, 255, 255, 0);


		inline ImVec4 background = ImColor(30, 31, 36, 245);
		inline float rounding = 3.f;
	}

	namespace text
	{
		inline ImVec4 text_active = ImColor(255, 255, 255, 255);
		inline ImVec4 text_hov = ImColor(255, 255, 255, 185);
		inline ImVec4 text = ImColor(255, 255, 255, 75);
		inline ImVec4 text_active1 = ImColor(255, 255, 255, 255);
		inline ImVec4 text1 = ImColor(134, 137, 158, 255);

	}

	namespace scrollbar
	{
		inline ImVec4 bar_active = ImColor(255, 255, 255, 145);
		inline ImVec4 bar_hov = ImColor(255, 255, 255, 125);
		inline ImVec4 bar = ImColor(255, 255, 255, 75);
	}

}
class c_custom {

public:
	float col_buf[4] = { 1.f, 1.f, 1.f, 1.f };
};
inline c_custom custom2;