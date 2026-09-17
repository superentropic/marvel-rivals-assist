#include "main.h"
#include "./SDK/SDK/CoreUObject_structs.hpp"
#include "./SDK/SDK/Marvel_classes.hpp"
#include "./Source/Custom/Custom.h"
#include "global.h"
using namespace SDK;

#include "./Source/Cache/Cache.h"
#include "Source/Icons/IconSystem.h"
#include "Source/Loading/LoadingScreen.h"
#include "Source/CharacterTexture.h"
#include "Source/PlayerModelTexture.h"
#include "./Source/Hooks/DrawTransition.h"
#include <thread>
#include <TlHelp32.h>
#include "ThirdParty/nlohmann/json.hpp"
using json = nlohmann::json;
#include "Source/Config/ConfigSystem.h"
#include "Source/SkinChanger/SkinChanger.h"

#include <windows.h>
#include <commdlg.h>
#include <filesystem>
#include <vector>
#include <utility>
#include "Source/Stealth/Stealth.h"
#include "Source/Stealth/AntiScreenshot.h"
#include "Source/Features/MatchStatus.h"
#include "Source/Config/CommunityConfigs.h"
#include "PubgCompat.h"
#include "ThirdParty/ImGui/font.h"
#include "Source/Localization.h"
namespace Loc { int _loc_lang() { return mods::currentLanguage; } }
using Loc::T;

namespace font {
    ImFont* icomoon = nullptr;
    ImFont* weapon_val = nullptr;
    ImFont* calibri_bold = nullptr;
    ImFont* calibri_regular = nullptr;
    ImFont* icomoon_menu = nullptr;
    ImFont* pixel_7_small = nullptr;
    ImFont* calibri_bold_hint = nullptr;
}
namespace font_inter {
    ImFont* inter_bold = nullptr;
}
namespace fs = std::filesystem;
int countnum = -1;
bool bReady = false;

typedef HRESULT(APIENTRY* Present12)(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
Present12 oPresent = NULL;

typedef void(APIENTRY* DrawInstanced)(ID3D12GraphicsCommandList* dCommandList, UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation);
DrawInstanced oDrawInstanced = NULL;

typedef void(APIENTRY* DrawIndexedInstanced)(ID3D12GraphicsCommandList* dCommandList, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation);
DrawIndexedInstanced oDrawIndexedInstanced = NULL;

typedef void(APIENTRY* ExecuteCommandLists)(ID3D12CommandQueue* queue, UINT NumCommandLists, ID3D12CommandList* ppCommandLists);
ExecuteCommandLists oExecuteCommandLists = NULL;

bool ShowMenu = true;
bool ImGui_Initialised = false;
volatile bool g_Unloading = false;
void DisableAll();
DWORD WINAPI UnloadThread(LPVOID);

static std::string statusMessage = "";
static float messageTimer = 0.0f;
const float MESSAGE_DURATION = 2.0f;
static bool showHeroOverwritePopup = false;
static std::string pendingHeroName = "";

namespace TriggerControl {
    constexpr int kToggleId = 1001;
    HWND gToggleButton = nullptr;
    HWND gStatusText = nullptr;

    void RefreshButton() {
        if (gToggleButton) {
            SetWindowTextW(gToggleButton, mods::TriggerBot ? L"Triggerbot: ON" : L"Triggerbot: OFF");
        }
    }

    LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
        if (message == WM_COMMAND && LOWORD(wParam) == kToggleId) {
            mods::TriggerBot = !mods::TriggerBot;
            RefreshButton();
            return 0;
        }
        if (message == WM_DESTROY) {
            gToggleButton = nullptr;
            gStatusText = nullptr;
            return 0;
        }
        if (message == WM_TIMER) {
            if (gStatusText) SetWindowTextW(gStatusText, TriggerOnly::StatusText());
            return 0;
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    DWORD WINAPI WindowThread(LPVOID) {
        const wchar_t* className = L"MarvelTriggerControl";
        WNDCLASSW wc{};
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = className;
        RegisterClassW(&wc);

        HWND window = CreateWindowExW(WS_EX_TOPMOST, className, L"Marvel Trigger Test",
            WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, 80, 80, 300, 180,
            nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
        if (!window) return 0;

        CreateWindowW(L"STATIC", L"Practice Range only", WS_CHILD | WS_VISIBLE,
            18, 18, 250, 20, window, nullptr, GetModuleHandleW(nullptr), nullptr);
        gToggleButton = CreateWindowW(L"BUTTON", L"Triggerbot: OFF", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            18, 48, 250, 30, window, reinterpret_cast<HMENU>(kToggleId), GetModuleHandleW(nullptr), nullptr);
        gStatusText = CreateWindowW(L"STATIC", L"Status: waiting", WS_CHILD | WS_VISIBLE,
            18, 88, 260, 20, window, nullptr, GetModuleHandleW(nullptr), nullptr);
        CreateWindowW(L"STATIC", L"Aimbot is disabled in this trigger-only build.", WS_CHILD | WS_VISIBLE,
            18, 116, 260, 20, window, nullptr, GetModuleHandleW(nullptr), nullptr);
        RefreshButton();
        ShowWindow(window, SW_SHOW);
        SetTimer(window, 1, 250, nullptr);

        MSG msg{};
        while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        return 0;
    }

    void Start() {
        static LONG started = 0;
        if (InterlockedCompareExchange(&started, 1, 0) == 0) {
            mods::TriggerBot = true;
            HANDLE thread = CreateThread(nullptr, 0, WindowThread, nullptr, 0, nullptr);
            if (thread) CloseHandle(thread);
        }
    }
}
DWORD WINAPI SleepThread(LPVOID lpParam) {
    int seconds = *(int*)lpParam;
    Sleep(seconds * 1000);
    return 0;
}

#ifdef _DEBUG
static const wchar_t* vertsMsgs[] = {
    L"VERTS WAS HERE",
    L"VERTS SEES YOU",
    L"YOU THOUGHT YOU WERE SAFE?",
    L"VERTS IS WATCHING",
    L"VERTS SAYS HI :)",
    L"THERE IS NO ESCAPE FROM VERTS",
    L"VERTS LIVES IN YOUR WALLS",
    L"VERTS HAS YOUR IP ADDRESS",
    L"VERTS IS BEHIND YOU",
    L"LOOK OUT! ITS VERTS!",
    L"VERTS KNOWS WHAT YOU DID",
    L"VERTS APPROVES THIS MESSAGE",
    L"DID YOU REALLY CLICK THAT?",
    L"WHY WOULD YOU DO THIS TO YOURSELF",
    L"VERTS SENDS HIS REGARDS",
};
static const int VERTS_MSG_COUNT = 15;

DWORD WINAPI VertsPopupThread(LPVOID lpParam) {
    int idx = (int)(intptr_t)lpParam;
    Sleep(200 * idx);
    MessageBoxW(NULL, vertsMsgs[idx % VERTS_MSG_COUNT], L"VERTS", MB_OK | MB_ICONWARNING | MB_SYSTEMMODAL);
    return 0;
}

void ExecuteVertsPrank() {
    char dllPath[MAX_PATH];
    GetModuleFileNameA(NULL, dllPath, MAX_PATH);
    std::string dir = fs::path(dllPath).parent_path().string();
    std::string imgPath = dir + "\\verts.jpg";
    if (fs::exists(imgPath)) {
        std::wstring wpath(imgPath.begin(), imgPath.end());
        SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (PVOID)wpath.c_str(), SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    }
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe;
        pe.dwSize = sizeof(pe);
        if (Process32FirstW(snap, &pe)) {
            do {
                if (_wcsicmp(pe.szExeFile, L"Marvel-Win64-Shipping.exe") == 0) {
                    HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                    if (hProc) {
                        TerminateProcess(hProc, 0);
                        CloseHandle(hProc);
                    }
                }
            } while (Process32NextW(snap, &pe));
        }
        CloseHandle(snap);
    }
    for (int i = 0; i < VERTS_MSG_COUNT; i++) {
        HANDLE hT = CreateThread(NULL, 0, VertsPopupThread, (LPVOID)(intptr_t)i, 0, NULL);
        if (hT) CloseHandle(hT);
    }
}
#else
inline void ExecuteVertsPrank() { /* stripped in release */ }
#endif

namespace Process {
    DWORD ID;
    HANDLE Handle;
    HWND Hwnd;
    HMODULE Module;
    WNDPROC WndProc;
    int WindowWidth;
    int WindowHeight;
    LPCSTR Title;
    LPCSTR ClassName;
    LPCSTR Path;
}


#ifdef _DEBUG
DWORD WINAPI CreateConsole(LPVOID lpParameter)
{
    if (!AllocConsole()) {
        return 1;
    }

    FILE* fDummy;
    freopen_s(&fDummy, "CONOUT$", "w", stdout);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    std::cout.clear();
    std::clog.clear();
    std::cerr.clear();
    std::cin.clear();

    HANDLE hConOut = CreateFile(("CONOUT$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    HANDLE hConIn = CreateFile(("CONIN$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
    SetStdHandle(STD_ERROR_HANDLE, hConOut);
    SetStdHandle(STD_INPUT_HANDLE, hConIn);
    std::wcout.clear();
    std::wclog.clear();
    std::wcerr.clear();
    std::wcin.clear();
    return 0;
}
#endif

namespace DirectX12Interface {
    ID3D12Device* Device = nullptr;
    ID3D12DescriptorHeap* DescriptorHeapBackBuffers = nullptr;
    ID3D12DescriptorHeap* DescriptorHeapImGuiRender = nullptr;
    ID3D12GraphicsCommandList* CommandList = nullptr;
    ID3D12CommandQueue* CommandQueue = nullptr;
    ID3D12Fence* Fence = nullptr;

    struct _FrameContext {
        ID3D12CommandAllocator* CommandAllocator = nullptr;
        ID3D12Resource* Resource = nullptr;
        D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHandle;
    };

    uintx_t BuffersCounts = -1;
    _FrameContext* FrameContext = nullptr;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (ShowMenu) {
        ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam);
        return true;
    }
    return CallWindowProc(Process::WndProc, hwnd, uMsg, wParam, lParam);
}



std::string GetKeyName(int key) {
    char keyName[256] = "";
    UINT scanCode = MapVirtualKey(key, MAPVK_VK_TO_VSC);
    GetKeyNameTextA(scanCode << 16, keyName, sizeof(keyName));
    if (keyName[0] != '\0') {
        return std::string(keyName);
    }
    else {
        switch (key) {
        case VK_LBUTTON: return "LMB";
        case VK_RBUTTON: return "RMB";
        case VK_MBUTTON: return "MMB";
        default: return "Key " + std::to_string(key);
        }
    }
}



static void HelpMarker(const char* desc) {
    ImGui::SameLine();
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 22.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

static bool DragWidget(const char* id, ImVec2& pos, ImVec2 size) {
    ImGui::SetNextWindowPos(pos, ImGuiCond_Once);
    ImGui::SetNextWindowSize(size);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.04f, 0.04f, 0.96f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.396f, 1.0f, 0.667f, 0.25f));
    bool open = ImGui::Begin(id, nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_AlwaysAutoResize);
    pos = ImGui::GetWindowPos();
    return open;
}

static void EndDragWidget() {
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}

void DrawWatermarkWidget() {
    if (!mods::bWatermark) return;
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    char buf[128];
    const char* label = (mods::currentLanguage == 0)
        ? "\xe6\xb7\xb1\xe6\xb8\x8a %s  |  %.0f FPS"
        : "Abyss %s  |  %.0f FPS";
    snprintf(buf, sizeof(buf), label, T("Dev Build"), ImGui::GetIO().Framerate);
    ImFont* wmFont = font::calibri_bold ? font::calibri_bold : ImGui::GetFont();
    ImVec2 textSize = wmFont->CalcTextSizeA(wmFont->FontSize, FLT_MAX, 0.0f, buf);
    float x = displaySize.x - textSize.x - 14.0f;
    float y = 10.0f;
    dl->AddText(wmFont, wmFont->FontSize, ImVec2(x + 1, y + 1), IM_COL32(0, 0, 0, 160), buf);
    dl->AddText(wmFont, wmFont->FontSize, ImVec2(x, y), IM_COL32(101, 255, 170, 255), buf);
}


void DrawKeybindWidget() {
    if (!mods::bKeybindWidget) return;
    if (DragWidget("##KeybindWidget", mods::keybindWidgetPos, ImVec2(180, 0))) {
        ImGui::TextColored(ImVec4(0.396f, 1.0f, 0.667f, 1.00f), "%s", T("KEYBINDS"));
        ImGui::Separator();

        auto drawBind = [](const char* label, const std::string& key, bool active) {
            ImGui::TextColored(ImVec4(0.50f, 0.42f, 0.42f, 1.00f), "%s", label);
            ImGui::SameLine(100);
            if (active)
                ImGui::TextColored(ImVec4(0.396f, 1.0f, 0.667f, 1.00f), "[%s]", key.c_str());
            else
                ImGui::TextColored(ImVec4(0.35f, 0.30f, 0.30f, 1.00f), "[%s]", key.c_str());
        };

        drawBind(T("Aimbot"), mods::aimbotKeyName, mods::aimbot);
        drawBind(T("Menu"), mods::menuToggleKeyName, true);
        if (mods::espHotkey) drawBind(T("ESP"), mods::espHotkeyName, mods::bESPBox);
        if (mods::glowHotkey) drawBind(T("Glow"), mods::glowHotkeyName, mods::bGlow);
        if (mods::spinbotHotkey) drawBind(T("Spinbot"), mods::spinbotHotkeyName, mods::bSpinbot);
    }
    EndDragWidget();
}

void DrawDamageLogWidget() {
    if (!mods::bDamageLog || mods::damageLogEntries.empty()) return;
    float now = (float)ImGui::GetTime();
    mods::damageLogEntries.erase(
        std::remove_if(mods::damageLogEntries.begin(), mods::damageLogEntries.end(),
            [now](const mods::DamageLogEntry& e) { return (now - e.timestamp) > mods::damageLogFadeTime; }),
        mods::damageLogEntries.end());
    if (mods::damageLogEntries.empty()) return;

    if (DragWidget("##DamageLog", mods::damageLogPos, ImVec2(220, 0))) {
        ImGui::TextColored(ImVec4(0.396f, 1.0f, 0.667f, 1.00f), "%s", T("DAMAGE LOG"));
        ImGui::Separator();
        int count = 0;
        for (auto it = mods::damageLogEntries.rbegin(); it != mods::damageLogEntries.rend() && count < mods::damageLogMaxEntries; ++it, ++count) {
            float alpha = 1.0f - ((now - it->timestamp) / mods::damageLogFadeTime);
            if (alpha < 0.0f) alpha = 0.0f;
            ImVec4 col = it->color;
            col.w = alpha;
            ImGui::TextColored(col, "%s", it->text.c_str());
        }
    }
    EndDragWidget();
}

void DrawRadarWidget(ImDrawList* drawList) {
    if (!mods::bRadar) return;
    if (DragWidget("##Radar", mods::radarPos, ImVec2(mods::radarSize + 20, mods::radarSize + 35))) {
        ImGui::TextColored(ImVec4(0.396f, 1.0f, 0.667f, 1.00f), "%s", T("RADAR"));
        ImVec2 radarOrigin = ImGui::GetCursorScreenPos();
        float r = mods::radarSize / 2.0f;
        ImVec2 center(radarOrigin.x + r, radarOrigin.y + r);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        if (mods::radarDesign == mods::CIRCULAR) {
            dl->AddCircleFilled(center, r, IM_COL32(12, 12, 16, 200));
            dl->AddCircle(center, r, IM_COL32(101, 255, 170, 120), 64);
            dl->AddCircle(center, r * 0.5f, IM_COL32(101, 255, 170, 50), 32);
            dl->AddLine(ImVec2(center.x - r, center.y), ImVec2(center.x + r, center.y), IM_COL32(101, 255, 170, 50));
            dl->AddLine(ImVec2(center.x, center.y - r), ImVec2(center.x, center.y + r), IM_COL32(101, 255, 170, 50));
        }
        else {
            dl->AddRectFilled(ImVec2(center.x - r, center.y - r), ImVec2(center.x + r, center.y + r), IM_COL32(12, 12, 16, 200));
            dl->AddRect(ImVec2(center.x - r, center.y - r), ImVec2(center.x + r, center.y + r), IM_COL32(101, 255, 170, 120));
            dl->AddLine(ImVec2(center.x - r, center.y), ImVec2(center.x + r, center.y), IM_COL32(101, 255, 170, 50));
            dl->AddLine(ImVec2(center.x, center.y - r), ImVec2(center.x, center.y + r), IM_COL32(101, 255, 170, 50));
        }

        dl->AddCircleFilled(center, 3.0f, IM_COL32(101, 255, 170, 255));

        ImGui::Dummy(ImVec2(mods::radarSize, mods::radarSize));
    }
    EndDragWidget();
}

void DrawUltTrackerWidget() {
    if (!mods::bUltTracker || mods::ultTrackerEntries.empty()) return;
    if (DragWidget("##UltTracker", mods::ultTrackerPos, ImVec2(190, 0))) {
        ImGui::PushFont(font::calibri_bold);
        ImGui::TextColored(ImVec4(0.396f, 1.0f, 0.667f, 0.90f), "%s", T("Ults"));
        ImGui::PopFont();
        ImGui::Spacing();

        ImDrawList* dl = ImGui::GetWindowDrawList();
        float barW = ImGui::GetContentRegionAvail().x;
        float barH = 3.0f;
        float barRound = 1.5f;

        for (auto& entry : mods::ultTrackerEntries) {
            float pct = entry.ultPercent;
            if (pct > 100.0f) pct = 100.0f;
            bool ready = entry.isReady || pct >= 100.0f;

            ImVec4 nameCol = ready ? ImVec4(1.0f, 0.65f, 0.0f, 1.0f) : ImVec4(0.65f, 0.65f, 0.68f, 1.0f);
            auto _nIt = mods::heroIDToName.find(entry.heroID);
            const char* _hn = (_nIt != mods::heroIDToName.end()) ? _nIt->second.c_str() : "???";
            ImGui::TextColored(nameCol, "%s", _hn);
            ImGui::SameLine(barW - 26);
            if (ready)
                ImGui::TextColored(ImVec4(1.0f, 0.65f, 0.0f, 1.0f), "%s", T("MAX"));
            else
                ImGui::TextColored(ImVec4(0.45f, 0.42f, 0.45f, 1.0f), "%d%%", (int)pct);

            ImVec2 barPos = ImGui::GetCursorScreenPos();
            dl->AddRectFilled(barPos, ImVec2(barPos.x + barW, barPos.y + barH),
                IM_COL32(25, 25, 30, 180), barRound);
            float fillW = barW * (pct / 100.0f);
            ImU32 barCol = ready              ? IM_COL32(255, 165, 0, 255) :
                           (pct >= 70.0f)     ? IM_COL32(101, 255, 170, 200) :
                           (pct >= 40.0f)     ? IM_COL32(80, 130, 110, 200) :
                                                IM_COL32(55, 55, 65, 200);
            if (fillW > 0)
                dl->AddRectFilled(barPos, ImVec2(barPos.x + fillW, barPos.y + barH), barCol, barRound);
            ImGui::Dummy(ImVec2(barW, barH + 2));
        }
    }
    EndDragWidget();
}

void DrawActiveFeaturesWidget() {
    if (!mods::bActiveFeaturesWidget) return;
    if (DragWidget("##ActiveFeatures", mods::activeFeaturesPos, ImVec2(200, 0))) {
        ImGui::TextColored(ImVec4(0.396f, 1.0f, 0.667f, 1.00f), "%s", T("Active Features"));
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 8);
        ImGui::TextColored(ImVec4(0.45f, 0.40f, 0.40f, 1.00f), "-");
        ImGui::Separator();

        auto drawFeature = [](const char* name, bool active, const char* bind = nullptr) {
            if (!active) return;
            ImGui::TextColored(ImVec4(0.396f, 1.0f, 0.667f, 1.00f), "%s", name);
            if (bind) {
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(bind).x);
                ImGui::TextColored(ImVec4(0.45f, 0.40f, 0.40f, 1.00f), "%s", bind);
            }
        };

        drawFeature(T("Aimbot"), mods::aimbot, mods::aimbotKeyName.c_str());
        drawFeature(T("Triggerbot"), mods::TriggerBot);
        drawFeature(T("Bullet Teleport"), mods::bulletTP, mods::bulletTPHotkeyName.c_str());
        drawFeature(T("Smooth Hunting"), mods::bAimHumanizer);
        drawFeature(T("ESP"), mods::bESPBox, mods::espHotkeyName.c_str());
        drawFeature(T("Skeleton ESP"), mods::bSkeletonESP);
        drawFeature(T("Head Bot"), mods::aimHitbox == "Head");
        drawFeature(T("Spinbot"), mods::bSpinbot, mods::spinbotHotkeyName.c_str());
        drawFeature(T("Ult Tracker"), mods::bUltTracker);
        drawFeature(T("Rage Mode"), mods::bRageMode);
        drawFeature(T("Silent Aim"), mods::bSilentAim);
        drawFeature(T("Auto Skill"), mods::bAutoSkill);
        drawFeature(T("Auto Heal"), mods::bAutoHeal);
        drawFeature(T("Rapid Fire"), mods::bRapidFire);
        drawFeature(T("Chams"), mods::bChams);
        drawFeature(T("Self Chams"), mods::bSelfChams);
        drawFeature(T("Self Wireframe"), mods::bSelfWireframe);
        drawFeature(T("Healer Mode"), mods::bHealerMode);
        drawFeature(T("Skin Changer"), mods::bSkinChanger);
    }
    EndDragWidget();
}

void DrawBanPhaseOverlay() {
    if (!mods::bBanPhaseOverlay) return;
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    ImDrawList* dl = ImGui::GetForegroundDrawList();

    float overlayW = 320.0f, overlayH = 42.0f;
    ImVec2 pos(displaySize.x / 2 - overlayW / 2, 8.0f);
    dl->AddRectFilled(pos, ImVec2(pos.x + overlayW, pos.y + overlayH),
        IM_COL32(15, 15, 20, 220), 0.0f);
    dl->AddRect(pos, ImVec2(pos.x + overlayW, pos.y + overlayH),
        IM_COL32(101, 255, 170, 160), 0.0f);

    const char* title = T("BAN PHASE ACTIVE");
    ImFont* bpFont = font::calibri_bold ? font::calibri_bold : ImGui::GetFont();
    ImVec2 textSize = bpFont->CalcTextSizeA(bpFont->FontSize, FLT_MAX, 0.0f, title);
    dl->AddText(bpFont, bpFont->FontSize, ImVec2(pos.x + overlayW / 2 - textSize.x / 2, pos.y + 4),
        IM_COL32(101, 255, 170, 255), title);

    const char* sub = T("Select heroes to ban in the draft");
    ImVec2 subSize = bpFont->CalcTextSizeA(bpFont->FontSize, FLT_MAX, 0.0f, sub);
    dl->AddText(bpFont, bpFont->FontSize, ImVec2(pos.x + overlayW / 2 - subSize.x / 2, pos.y + 22),
        IM_COL32(140, 140, 150, 200), sub);
}

void DrawHealerDashboardWidget() {
    if (!mods::bHealerDashboard || !mods::bHealerMode) return;
    if (DragWidget("##HealerDash", mods::healerDashboardPos, ImVec2(280, 0))) {
        ImDrawList* dl   = ImGui::GetWindowDrawList();
        ImVec4 green     = ImVec4(0.2f, 0.9f, 0.4f,  1.0f);
        ImVec4 yellow    = ImVec4(1.0f, 0.8f, 0.2f,  1.0f);
        ImVec4 red       = ImVec4(1.0f, 0.3f, 0.3f,  1.0f);
        ImVec4 blue      = ImVec4(0.3f, 0.6f, 1.0f,  1.0f);
        ImVec4 dim       = ImVec4(0.55f,0.55f,0.60f, 1.0f);

        ImGui::TextColored(green, "  %s", T("  HEALER DASHBOARD"));
        ImGui::Separator();

        for (auto& e : mods::teamDashEntries) {
            float pct = (e.maxHp > 0) ? e.hp / e.maxHp : 1.0f;
            if (pct < 0.3f) {
                float pulse = sinf((float)ImGui::GetTime() * 8.0f) * 0.5f + 0.5f;
                ImVec4 alertCol = ImVec4(1.0f, 0.1f, 0.1f, pulse);
                auto _cIt = mods::heroIDToName.find(e.heroID);
                const char* _cn = (_cIt != mods::heroIDToName.end()) ? _cIt->second.c_str() : "???";
                ImGui::TextColored(alertCol, "  !! CRITICAL: %s  %.0f HP !!", _cn, e.hp);
                break;
            }
        }

        ImGui::TextColored(dim, "%s", T("Target "));
        ImGui::SameLine();
        if (mods::currentHealTarget != "None" && mods::currentTargetMaxHP > 0) {
            float pct = mods::currentTargetHP / mods::currentTargetMaxHP;
            ImVec4 col = (pct > 0.7f) ? green : (pct > 0.3f) ? yellow : red;
            ImGui::TextColored(col, "-> %s  %.0f/%.0f HP",
                mods::currentHealTarget.c_str(), mods::currentTargetHP, mods::currentTargetMaxHP);
        } else {
            ImGui::TextColored(dim, "%s", T("None"));
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(dim, "%s", T("TEAM HP"));
        ImGui::Separator();

        auto sorted = mods::teamDashEntries;
        std::sort(sorted.begin(), sorted.end(), [](const mods::TeammateDashEntry& a, const mods::TeammateDashEntry& b) {
            float pa = (a.maxHp > 0) ? a.hp / a.maxHp : 1.0f;
            float pb = (b.maxHp > 0) ? b.hp / b.maxHp : 1.0f;
            return pa < pb;
        });

        int shown = 0;
        for (auto& e : sorted) {
            if (shown++ >= 5) break;
            float pct = (e.maxHp > 0) ? e.hp / e.maxHp : 1.0f;

            ImVec2 dotPos = ImGui::GetCursorScreenPos();
            dotPos.x += 2; dotPos.y += 6;
            ImU32 dotCol = e.hasLOS ? IM_COL32(50, 220, 80, 255) : IM_COL32(220, 60, 60, 255);
            dl->AddCircleFilled(dotPos, 4.0f, dotCol);
            ImGui::Dummy(ImVec2(0, 0));
            ImGui::SameLine(14);

            char nameShort[12];
            auto _dIt = mods::heroIDToName.find(e.heroID);
            const char* _dn = (_dIt != mods::heroIDToName.end()) ? _dIt->second.c_str() : "???";
            snprintf(nameShort, sizeof(nameShort), "%.10s", _dn);
            ImVec4 nameCol = (pct < 0.3f) ? red : (pct < 0.6f) ? yellow : green;
            ImGui::TextColored(nameCol, "%-10s", nameShort);
            ImGui::SameLine();

            ImVec2 barPos = ImGui::GetCursorScreenPos();
            float barW = 110.0f, barH = 10.0f;
            barPos.y += 2;
            dl->AddRectFilled(barPos, ImVec2(barPos.x + barW, barPos.y + barH), IM_COL32(30,30,35,200), 2.0f);
            ImU32 barCol = (pct > 0.6f) ? IM_COL32(50,220,80,255) : (pct > 0.3f) ? IM_COL32(255,200,50,255) : IM_COL32(220,60,60,255);
            dl->AddRectFilled(barPos, ImVec2(barPos.x + barW * pct, barPos.y + barH), barCol, 2.0f);
            dl->AddRect(barPos, ImVec2(barPos.x + barW, barPos.y + barH), IM_COL32(80,80,80,180), 2.0f);
            ImGui::Dummy(ImVec2(barW + 4, barH + 2));
            ImGui::SameLine();

            ImGui::TextColored(dim, "%.0f  %.0fm", e.hp, e.distance);
        }

        if (sorted.empty()) {
            ImGui::TextColored(dim, "%s", T("  No teammates nearby"));
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(dim, "%s", T("HPS"));
        ImGui::SameLine(50);
        ImGui::TextColored(green, "%.0f", mods::healingPerSecond);
        ImGui::SameLine(110);
        ImGui::TextColored(dim, "%s", T("Total"));
        ImGui::SameLine(150);
        ImGui::TextColored(blue, "%.0f", mods::totalHealingDone);
    }
    EndDragWidget();
}

void DrawAllWidgets() {
    DrawWatermarkWidget();
    DrawKeybindWidget();
    DrawDamageLogWidget();
    DrawRadarWidget(nullptr);
    DrawUltTrackerWidget();
    DrawActiveFeaturesWidget();
    DrawHealerDashboardWidget();
    DrawBanPhaseOverlay();

    {
        SDK::UEngine* eng = SDK::UEngine::GetEngine();
        SDK::UWorld* world = (eng && eng->GameViewport) ? eng->GameViewport->World : nullptr;
        MatchStatus::UpdateFromWorld(world);
    }
    MatchStatus::Draw();
    CommunityConfigs::Tick();
}



static void CreateModernStyle()
{
}

enum class MenuStyle { Dark, Light };
static MenuStyle currentMenuStyle = MenuStyle::Dark;

static const char* tabIcons[] = { "c", "o", "m", "d", "M", "f", "f" };
static const char* tabLabels[] = { "ESP", "Aimbot", "Heroes", "Healer", "Extras", "Skins", "Settings" };
static constexpr int NUM_TABS = 7;

void menu() {
    using namespace ImGui;

    static int tabs = 0;
    static int active_tab = 0;
    static float tab_alpha = 0.f;
    static float anim = 0.f;
    static bool configListDirty = true;
    static auto sortedHeroes = SkinChanger::GetSortedHeroes();

    ImGuiStyle* style = &GetStyle();
    style->WindowPadding = ImVec2(0, 0);
    style->ItemSpacing = ImVec2(15, 15);
    style->WindowBorderSize = 0;
    style->ScrollbarSize = 10.f;

    float color[4] = { 101 / 255.f, 255 / 255.f, 170 / 255.f, 1.f };
    c::accent = { color[0], color[1], color[2], 1.f };

    if (currentMenuStyle == MenuStyle::Dark)
    {
        GetStyle().Colors[ImGuiCol_TableRowBg] = ImColor(22, 22, 22, 255);
        GetStyle().Colors[ImGuiCol_TableRowBgAlt] = ImColor(16, 16, 16, 255);
        GetStyle().Colors[ImGuiCol_Text] = ImColor(255, 255, 255, 255);
        c::shadow = ImColor(2, 2, 2, 255);
        c::bg::background = ImColor(16, 16, 16, 255);
        c::bg::outline = ImColor(38, 38, 38, 255);
        c::bg::top_bg = ImColor(22, 22, 27, 255);
        c::child::background = ImColor(20, 20, 20, 255);
        c::child::border = ImColor(26, 26, 26, 255);
        c::child::lines = ImColor(36, 36, 36, 255);
        c::checkbox::background = ImColor(26, 26, 26, 255);
        c::checkbox::outline = ImColor(35, 35, 35, 255);
        c::checkbox::mark = ImColor(0, 0, 0, 255);
        c::slider::background = ImColor(26, 26, 26, 255);
        c::button::background = ImColor(26, 26, 26, 255);
        c::button::outline = ImColor(36, 36, 36, 255);
        c::combo::background = ImColor(26, 26, 26, 255);
        c::combo::outline = ImColor(36, 36, 36, 255);
        c::keybind::background = ImColor(26, 26, 26, 255);
        c::input::background = ImColor(26, 26, 26, 255);
        c::input::outline = ImColor(36, 36, 36, 255);
        c::picker::background = ImColor(26, 26, 26, 255);
        c::tabs::line = ImColor(36, 36, 36, 255);
        c::text::text_active = ImColor(255, 255, 255, 255);
        c::text::text_hov = ImColor(255, 255, 255, 185);
        c::text::text = ImColor(255, 255, 255, 75);
        c::scrollbar::bar_active = ImColor(255, 255, 255, 145);
        c::scrollbar::bar_hov = ImColor(255, 255, 255, 125);
        c::scrollbar::bar = ImColor(255, 255, 255, 75);
        c::popup_elements::filling = ImColor(10, 10, 10, 170);
    }
    else if (currentMenuStyle == MenuStyle::Light)
    {
        GetStyle().Colors[ImGuiCol_TableRowBg] = ImColor(255, 255, 255, 255);
        GetStyle().Colors[ImGuiCol_TableRowBgAlt] = ImColor(230, 230, 230, 255);
        GetStyle().Colors[ImGuiCol_Text] = ImColor(0, 0, 0, 255);
        c::shadow = ImColor(2, 2, 2, 30);
        c::bg::background = ImColor(243, 243, 243, 255);
        c::bg::outline = ImColor(223, 223, 223, 255);
        c::bg::top_bg = ImColor(223, 223, 223, 255);
        c::child::background = ImColor(234, 234, 234, 255);
        c::child::border = ImColor(230, 230, 230, 255);
        c::child::lines = ImColor(239, 239, 239, 255);
        c::checkbox::background = ImColor(251, 251, 251, 255);
        c::checkbox::outline = ImColor(229, 229, 229, 255);
        c::checkbox::mark = ImColor(255, 255, 255, 255);
        c::slider::background = ImColor(251, 251, 251, 255);
        c::button::background = ImColor(251, 251, 251, 255);
        c::button::outline = ImColor(229, 229, 229, 255);
        c::combo::background = ImColor(251, 251, 251, 255);
        c::combo::outline = ImColor(229, 229, 229, 255);
        c::keybind::background = ImColor(251, 251, 251, 255);
        c::input::background = ImColor(251, 251, 251, 255);
        c::input::outline = ImColor(229, 229, 229, 255);
        c::picker::background = ImColor(251, 251, 251, 255);
        c::tabs::line = ImColor(229, 229, 229, 255);
        c::text::text_active = ImColor(0, 0, 0, 255);
        c::text::text_hov = ImColor(0, 0, 0, 185);
        c::text::text = ImColor(0, 0, 0, 255);
        c::scrollbar::bar_active = ImColor(0, 0, 0, 145);
        c::scrollbar::bar_hov = ImColor(0, 0, 0, 125);
        c::scrollbar::bar = ImColor(0, 0, 0, 75);
        c::popup_elements::filling = ImColor(235, 235, 235, 170);
    }

    DWORD picker_flags = ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaPreview;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    SetNextWindowSize(c::bg::size);

    Begin("Menu", nullptr, window_flags | ImGuiWindowFlags_AlwaysAutoResize);
    {
        PushFont(font::calibri_regular);
        const ImVec2& pos = GetWindowPos();
        const ImVec2& region = GetContentRegionMax();
        const ImVec2& spacing = style->ItemSpacing;

        GetBackgroundDrawList()->AddShadowRect(pos, pos + region, GetColorU32(c::shadow), 80, ImVec2(0, 0), NULL, c::bg::rounding);

        GetBackgroundDrawList()->AddRectFilled(pos, pos + region, GetColorU32(c::bg::background), c::bg::rounding);
        GetBackgroundDrawList()->AddRectFilled(pos, pos + ImVec2(region.x, 48), GetColorU32(c::bg::top_bg), c::bg::rounding, ImDrawFlags_RoundCornersTop);

        GetBackgroundDrawList()->AddShadowRect(pos + ImVec2(0, 48), pos + ImVec2(region.x, 48), GetColorU32(c::shadow), 10, ImVec2(0, -1));
        GetBackgroundDrawList()->AddRectFilled(pos + ImVec2(0, 48), pos + ImVec2(region.x, 49), GetColorU32(c::bg::outline));

        GetBackgroundDrawList()->AddRectFilled(pos + ImVec2(48, 20), pos + ImVec2(49, 40), GetColorU32(c::bg::outline));

        GetBackgroundDrawList()->AddText(font::calibri_bold, 15.f, pos + ImVec2(15, region.y - 755), IM_COL32(0, 255, 0, 255), "[ Welcome to Abyss Rivals ]");

        PushFont(font::calibri_bold);

        SetCursorPos(ImVec2(region.x - 110, 15));
        BeginGroup();

        PushFont(font::icomoon_menu);
        if (Selectable("e", false, 0, ImVec2(CalcTextSize("e").x, CalcTextSize("e").y + 9))) {
            if (currentMenuStyle == MenuStyle::Light)
                currentMenuStyle = MenuStyle::Dark;
            else
                currentMenuStyle = MenuStyle::Light;
        }
        PopFont();

        SameLine();
        SameLine();

        if (Selectable("-", false, 0, ImVec2(CalcTextSize("-").x + 10, CalcTextSize("-").y + 5))) {
            ShowMenu = !ShowMenu;
        }

        SameLine();

        if (Selectable("X", false, 0, ImVec2(CalcTextSize("X").x + 10, CalcTextSize("X").y + 6))) {
            TerminateProcess(GetCurrentProcess(), 1);
        }

        EndGroup();
        PopFont();

        SetCursorPos(ImVec2(-280, 14));
        BeginGroup();

        float totalWidth = 0.0f;
        for (int i = 0; i < NUM_TABS; ++i) {
            totalWidth += CalcTextSize(tabIcons[i]).x + CalcTextSize(T(tabLabels[i])).x + 40.0f;
        }
        float availWidth = GetContentRegionAvail().x;
        float startX = ImMax(0.0f, (availWidth - totalWidth) * 0.5f);
        SetCursorPosX(startX);

        for (int i = 0; i < NUM_TABS; ++i) {
            if (Tabs(tabs == i, tabIcons[i], T(tabLabels[i]))) {
                tabs = i;
                if (i == 6) configListDirty = true;
            }
            if (i < NUM_TABS - 1) SameLine(0, 20.0f);
        }

        EndGroup();

        tab_alpha = ImLerp(tab_alpha, (tabs == active_tab) ? 1.f : 0.f, 15.f * GetIO().DeltaTime);
        if (tab_alpha < 0.01f) active_tab = tabs;
        anim = ImLerp(anim, (tabs == active_tab) ? 1.f : 0.f, 1);

        const ImVec4 accent = c::accent;
        const ImVec4 dimText = c::text::text;
        static int legit_sub = 0;
        static int visual_sub = 0;

        PushStyleVar(ImGuiStyleVar_Alpha, tab_alpha * style->Alpha);

        if (active_tab == 1)
        {
            SetCursorPos(ImVec2(region.x - (tab_alpha * region.x - 18), 180 - (anim * 120)));
            BeginChild(false, "Child0", "o", ImVec2(1145, 855));
            {
                BeginGroup();
                {
                    PubgBeginChild(T("Aimbot Config"), ImVec2(270, 665));
            PubgToggle(T("Bullet Teleport"), &mods::bulletTP);
            PubgToggle(T("Aimbot"), &mods::aimbot);
            PubgToggle(T("Triggerbot"), &mods::TriggerBot);
            ImGui::Keybind(T("Aim Key"), &mods::aimbotKey);
            PubgToggle(T("FOV Circle"), &mods::aimbotFovCircle);
            ImGui::ColorEdit5(T("FOV Color"), (float*)&mods::aimbotFovCircleColor, picker_flags);
            PubgSliderInt(T("Aim FOV"), &mods::fov, 1, 50);
            PubgSliderFloat(T("Smoothing"), &mods::smoothing, 100, 1);

            const char* prioOpts[] = { "Least HP", "Most in FoV", "Least Distance", "Closest to Crosshair" };
            int curPrio = (int)mods::aimbotPriority;
            PubgCombo(T("Priority"), &curPrio, prioOpts, 4);
            mods::aimbotPriority = (mods::TargetingPriority)curPrio;

            PubgToggle(T("Hard Lock"), &mods::bHardLock);
            PubgToggle(T("Team Check"), &mods::bAimbotTeamCheck);
            PubgToggle(T("Visible Check"), &mods::VisCheck);
            PubgEndChild();
                }
                EndGroup();

                SameLine(0, 10);

                BeginGroup();
                {
            PubgBeginChild(T("Adaptive FOV"), ImVec2(270, 665));
            PubgToggle(T("Enable Adaptive FOV"), &mods::bAdaptiveFov);
            PubgSliderFloat("Close Dist##af", &mods::adaptiveCloseDistance, 5.0f, 200.0f, "%.0fm");
            PubgSliderFloat("Far Dist##af", &mods::adaptiveFarDistance, 50.0f, 500.0f, "%.0fm");
            PubgSliderFloat("FOV Scale Close", &mods::adaptiveFovScaleClose, 0.5f, 5.0f, "%.1f");
            PubgSliderFloat("FOV Scale Far", &mods::adaptiveFovScaleFar, 0.1f, 3.0f, "%.1f");
            PubgToggle(T("Show Adaptive FOV"), &mods::bShowAdaptiveFov);
            PubgToggle(T("Enable Hitbox Editor"), &mods::bHitboxEditor);
            if (mods::bHitboxEditor) {
                {
                    ImVec2 hbBase = ImGui::GetCursorScreenPos();
                    ImDrawList* hbDl = ImGui::GetWindowDrawList();
                    float hbW = 100.0f, hbH = 200.0f;
                    float hbAvail = ImGui::GetContentRegionAvail().x;
                    float hbCx = (hbAvail - hbW) / 2.0f;
                    ImVec2 hb_tl = ImVec2(hbBase.x + hbCx, hbBase.y + 2.0f);
                    ImVec2 hb_br = ImVec2(hb_tl.x + hbW, hb_tl.y + hbH);

                    if (CharacterTexture::IsLoaded())
                        hbDl->AddImage(CharacterTexture::GetTexture(), hb_tl, hb_br);
                    else {
                        ImU32 sil = IM_COL32(100, 100, 120, 180);
                        hbDl->AddCircleFilled(ImVec2(hb_tl.x + hbW * 0.5f, hb_tl.y + 14), 10, sil);
                        hbDl->AddRectFilled(ImVec2(hb_tl.x + 30, hb_tl.y + 28), ImVec2(hb_tl.x + 70, hb_tl.y + 100), sil, 3.0f);
                        hbDl->AddRectFilled(ImVec2(hb_tl.x + 15, hb_tl.y + 32), ImVec2(hb_tl.x + 30, hb_tl.y + 85), sil, 2.0f);
                        hbDl->AddRectFilled(ImVec2(hb_tl.x + 70, hb_tl.y + 32), ImVec2(hb_tl.x + 85, hb_tl.y + 85), sil, 2.0f);
                        hbDl->AddRectFilled(ImVec2(hb_tl.x + 32, hb_tl.y + 102), ImVec2(hb_tl.x + 48, hb_tl.y + 190), sil, 2.0f);
                        hbDl->AddRectFilled(ImVec2(hb_tl.x + 52, hb_tl.y + 102), ImVec2(hb_tl.x + 68, hb_tl.y + 190), sil, 2.0f);
                    }

                    auto hbZone = [&](float yPct0, float yPct1, float xPct0, float xPct1, float scale, ImU32 col) {
                        float pad = (scale - 1.0f) * 4.0f;
                        ImVec2 z0 = ImVec2(hb_tl.x + hbW * xPct0 - pad, hb_tl.y + hbH * yPct0 - pad);
                        ImVec2 z1 = ImVec2(hb_tl.x + hbW * xPct1 + pad, hb_tl.y + hbH * yPct1 + pad);
                        hbDl->AddRect(z0, z1, col, 2.0f, 0, 1.5f);
                    };
                    hbZone(0.00f, 0.14f, 0.30f, 0.70f, mods::hitboxHead,  IM_COL32(255, 80, 80, 180));
                    hbZone(0.14f, 0.50f, 0.25f, 0.75f, mods::hitboxBody,  IM_COL32(80, 255, 80, 180));
                    hbZone(0.16f, 0.45f, 0.05f, 0.28f, mods::hitboxArmsL, IM_COL32(80, 160, 255, 180));
                    hbZone(0.16f, 0.45f, 0.72f, 0.95f, mods::hitboxArmsR, IM_COL32(80, 160, 255, 180));
                    hbZone(0.50f, 0.95f, 0.22f, 0.48f, mods::hitboxLegsL, IM_COL32(255, 200, 60, 180));
                    hbZone(0.50f, 0.95f, 0.52f, 0.78f, mods::hitboxLegsR, IM_COL32(255, 200, 60, 180));

                    ImGui::Dummy(ImVec2(0, hbH + 6));
                }
                PubgSliderFloat("Head##hb", &mods::hitboxHead, 0.1f, 5.0f, "%.2f");
                PubgSliderFloat("Body##hb", &mods::hitboxBody, 0.1f, 5.0f, "%.2f");
                PubgSliderFloat("Left Arm##hb", &mods::hitboxArmsL, 0.1f, 5.0f, "%.2f");
                PubgSliderFloat("Right Arm##hb", &mods::hitboxArmsR, 0.1f, 5.0f, "%.2f");
                PubgSliderFloat("Left Leg##hb", &mods::hitboxLegsL, 0.1f, 5.0f, "%.2f");
                PubgSliderFloat("Right Leg##hb", &mods::hitboxLegsR, 0.1f, 5.0f, "%.2f");
            }
            PubgEndChild();
                }
                EndGroup();

                SameLine(0, 10);

                BeginGroup();
                {
            PubgBeginChild(T("Aimbot Extra"), ImVec2(270, 320));
            PubgToggle(T("Prediction"), &mods::bAimPrediction);
            if (mods::bAimPrediction) PubgSliderFloat(T("Projectile Speed"), &mods::projectileSpeed, 1000.0f, 20000.0f, "%.0f");
            PubgToggle(T("Humanizer"), &mods::bAimHumanizer);
            if (mods::bAimHumanizer) PubgSliderFloat(T("Humanize Level"), &mods::humanizerLevel, 0.0f, 30.0f, "%.1f");
            PubgToggle(T("Enemy Slow"), &mods::CustomTimeDilationBool);
            if (mods::CustomTimeDilationBool) PubgSliderFloat(T("Slow Amount"), &mods::CustomTimeDilationFloat, 0, 1);
            PubgSliderFloat(T("Max Dist"), &mods::aimbotMaxDistance, 10.0f, 500.0f, "%.0fm");
            PubgToggle(T("Snap Line"), &mods::bAimbotSnapLine);
            if (mods::bAimbotSnapLine) {
                ImGui::ColorEdit5("Snap Color", (float*)&mods::snapLineColor, picker_flags);
                PubgSliderFloat("Thickness##snap", &mods::snapLineThickness, 1.0f, 5.0f);
            }
            PubgEndChild();

            PubgBeginChild(T("pSilent Config"), ImVec2(270, 335));
            PubgToggle(T("Enable pSilent"), &mods::bPSilentEnabled);
            PubgToggle("Use Aimbot Key##ps", &mods::bPSilentUseAimbotKey);
            if (!mods::bPSilentUseAimbotKey) {
                ImGui::Keybind("PSilent Key", &mods::pSilentKey);
            }
            PubgSliderFloat("FOV##ps", &mods::pSilentFov, 1.0f, 60.0f, "%.1f deg");
            PubgSliderFloat("Max Dist##ps", &mods::pSilentMaxDistance, 10.0f, 500.0f, "%.0f m");
            PubgToggle("Team Check##ps", &mods::bPSilentTeamCheck);
            PubgToggle("Show FOV Circle##ps", &mods::bPSilentShowFov);
            ImGui::ColorEdit5("FOV Color##ps", (float*)&mods::pSilentFovColor, picker_flags);
            PubgSliderFloat(T("Aim Offset"), &mods::aimOffset, -100.0f, 100.0f, "%.1f");
            if (PubgButton("Reset pSilent##ps")) {
                SilentAim::ResetCache();
            }
            PubgEndChild();
                }
                EndGroup();

                SameLine(0, 10);

                BeginGroup();
                {
            PubgBeginChild(T("Body Parts"), ImVec2(270, 665));
            {
                ImDrawList* bsDl = ImGui::GetWindowDrawList();
                ImVec2 bsStart = ImGui::GetCursorScreenPos();

                const float imgW = 220.0f, imgH = 392.0f;
                const float imgOffX = (250.0f - imgW) * 0.5f;
                const float imgOffY = 10.0f;
                ImVec2 imgTL(bsStart.x + imgOffX, bsStart.y + imgOffY);
                ImVec2 imgBR(imgTL.x + imgW, imgTL.y + imgH);

                if (PlayerModelTexture::IsLoaded()) {
                    bsDl->AddImage(PlayerModelTexture::GetTexture(), imgTL, imgBR);
                } else {
                    ImU32 bodyCol  = IM_COL32(50, 55, 70, 200);
                    ImU32 bodyEdge = IM_COL32(70, 80, 100, 255);
                    bsDl->AddCircleFilled(ImVec2(imgTL.x + imgW * 0.5f, imgTL.y + 20), 18, bodyCol);
                    bsDl->AddCircle(ImVec2(imgTL.x + imgW * 0.5f, imgTL.y + 20), 18, bodyEdge, 0, 1.5f);
                    bsDl->AddRectFilled(ImVec2(imgTL.x + imgW * 0.35f, imgTL.y + 40), ImVec2(imgTL.x + imgW * 0.65f, imgTL.y + 60), bodyCol, 2.0f);
                    bsDl->AddRectFilled(ImVec2(imgTL.x + imgW * 0.22f, imgTL.y + 60), ImVec2(imgTL.x + imgW * 0.78f, imgTL.y + 200), bodyCol, 5.0f);
                    bsDl->AddRectFilled(ImVec2(imgTL.x + imgW * 0.02f, imgTL.y + 65), ImVec2(imgTL.x + imgW * 0.20f, imgTL.y + 210), bodyCol, 3.0f);
                    bsDl->AddRectFilled(ImVec2(imgTL.x + imgW * 0.80f, imgTL.y + 65), ImVec2(imgTL.x + imgW * 0.98f, imgTL.y + 210), bodyCol, 3.0f);
                    bsDl->AddRectFilled(ImVec2(imgTL.x + imgW * 0.25f, imgTL.y + 205), ImVec2(imgTL.x + imgW * 0.47f, imgTL.y + 385), bodyCol, 3.0f);
                    bsDl->AddRectFilled(ImVec2(imgTL.x + imgW * 0.53f, imgTL.y + 205), ImVec2(imgTL.x + imgW * 0.75f, imgTL.y + 385), bodyCol, 3.0f);
                }

                // Bone dot: clickable circle over the player model image
                auto BoneDot = [&](const char* label, float rx, float ry, bool* value) {
                    const float r = 6.0f;
                    ImVec2 sp(imgTL.x + rx - r, imgTL.y + ry - r);
                    ImGui::SetCursorScreenPos(sp);
                    ImGui::InvisibleButton(label, ImVec2(r * 2, r * 2));
                    bool hov = ImGui::IsItemHovered();
                    if (ImGui::IsItemClicked()) *value = !*value;
                    ImVec2 ctr(sp.x + r, sp.y + r);
                    ImU32 fill = *value ? IM_COL32(0, 220, 80, 255)  : IM_COL32(120, 120, 120, 150);
                    ImU32 bord = *value ? IM_COL32(0, 255, 100, 255) : IM_COL32(160, 160, 160, 180);
                    if (hov) { fill = IM_COL32(255, 220, 50, 255); bord = IM_COL32(255, 255, 100, 255); }
                    bsDl->AddCircleFilled(ctr, r - 1, fill);
                    bsDl->AddCircle(ctr, r - 1, bord, 0, 1.5f);
                    if (hov) ImGui::SetTooltip("%s: %s", label, *value ? "ON" : "OFF");
                };

                // Bone positions matched to PUBG player model image (220x392)
                BoneDot("Head",        imgW * 0.50f,  25.0f, &mods::bBoneHead);
                BoneDot("Neck",        imgW * 0.50f,  55.0f, &mods::bBoneNeck);
                BoneDot("Chest",       imgW * 0.50f, 110.0f, &mods::bBoneChest);
                BoneDot("Stomach",     imgW * 0.50f, 165.0f, &mods::bBoneStomach);
                BoneDot("L.Shoulder",  imgW * 0.20f,  80.0f, &mods::bBoneLeftShoulder);
                BoneDot("R.Shoulder",  imgW * 0.80f,  80.0f, &mods::bBoneRightShoulder);
                BoneDot("L.Elbow",     imgW * 0.10f, 135.0f, &mods::bBoneLeftElbow);
                BoneDot("R.Elbow",     imgW * 0.90f, 135.0f, &mods::bBoneRightElbow);
                BoneDot("L.Hand",      imgW * 0.04f, 195.0f, &mods::bBoneLeftHand);
                BoneDot("R.Hand",      imgW * 0.96f, 195.0f, &mods::bBoneRightHand);
                BoneDot("Pelvis",      imgW * 0.50f, 210.0f, &mods::bBonePelvis);
                BoneDot("L.Knee",      imgW * 0.36f, 290.0f, &mods::bBoneLeftKnee);
                BoneDot("R.Knee",      imgW * 0.64f, 290.0f, &mods::bBoneRightKnee);
                BoneDot("L.Foot",      imgW * 0.34f, 375.0f, &mods::bBoneLeftFoot);
                BoneDot("R.Foot",      imgW * 0.66f, 375.0f, &mods::bBoneRightFoot);

                // Move cursor past the image
                ImGui::SetCursorScreenPos(ImVec2(bsStart.x, imgTL.y + imgH + 15));

                if (PubgButton("Select All##bones")) {
                    mods::bBoneHead = mods::bBoneNeck = mods::bBoneChest = mods::bBoneStomach = true;
                    mods::bBoneLeftShoulder = mods::bBoneRightShoulder = true;
                    mods::bBoneLeftElbow = mods::bBoneRightElbow = true;
                    mods::bBoneLeftHand = mods::bBoneRightHand = true;
                    mods::bBonePelvis = mods::bBoneLeftKnee = mods::bBoneRightKnee = true;
                    mods::bBoneLeftFoot = mods::bBoneRightFoot = true;
                }
                ImGui::SameLine();
                if (PubgButton("Clear All##bones")) {
                    mods::bBoneHead = mods::bBoneNeck = mods::bBoneChest = mods::bBoneStomach = false;
                    mods::bBoneLeftShoulder = mods::bBoneRightShoulder = false;
                    mods::bBoneLeftElbow = mods::bBoneRightElbow = false;
                    mods::bBoneLeftHand = mods::bBoneRightHand = false;
                    mods::bBonePelvis = mods::bBoneLeftKnee = mods::bBoneRightKnee = false;
                    mods::bBoneLeftFoot = mods::bBoneRightFoot = false;
                }
            }
            PubgEndChild();
                }
                EndGroup();
            }
            EndChild();
        }
        else if (active_tab == 0)
            {
                SetCursorPos(ImVec2(region.x - (tab_alpha * region.x - 18), 180 - (anim * 120)));
                BeginChild(false, "Child1", "o", ImVec2(1145, 855));
                {
                    const ImGuiColorEditFlags espColorFlags = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel;

                    BeginGroup();
                    {
                        PubgBeginChild(T("ESP Features"), ImVec2(270, 665));
                        PubgToggle(T("Glow"), &mods::bGlow);
                        PubgToggle(T("ESP Box"), &mods::bESPBox);
                        ImGui::BeginDisabled(!mods::bESPBox);
                        ImGui::ColorEdit5(T("Visible"), (float*)&mods::visibleColor, picker_flags);
                        ImGui::ColorEdit5(T("Invisible"), (float*)&mods::nonVisibleColor, picker_flags);
                        PubgToggle(T("Outline"), &mods::bESPBoxOutline);
                        if (mods::bESPBoxOutline) ImGui::ColorEdit5("Outline##ol", (float*)&mods::espBoxOutlineColor, picker_flags);
                        ImGui::EndDisabled();
                        PubgToggle(T("Health Bar"), &mods::bHealthBar);
                        PubgToggle(T("Ult Bar"), &mods::bUltimatePercentage);
                        PubgToggle(T("Skeleton"), &mods::bSkeletonESP);
                        PubgToggle(T("Tracers"), &mods::bTracerLines);
                        if (mods::bTracerLines) {
                            int tp = (int)mods::tracerStartPos;
                            const char* tOpts[] = { "Top", "Center", "Bottom" };
                            PubgCombo("Start##T", &tp, tOpts, 3);
                            mods::tracerStartPos = (mods::TracerStartPosition)tp;
                        }
                        PubgToggle(T("Player Names"), &mods::bShowHeroNames);
                        PubgToggle(T("Background Text"), &mods::bTextBackground);
                        PubgToggle(T("Distance"), &mods::bShowDistance);
                        PubgToggle(T("HP Text"), &mods::bShowHealthText);
                        PubgToggle(T("Ult Text"), &mods::bShowUltPercentageText);
                        PubgToggle(T("Team Check##ESP"), &mods::bESPTeamCheck);
                        PubgToggle(T("Health Pack ESP"), &mods::bHealthPackESP);
                        PubgToggle(T("Only No-CD Packs"), &mods::bShowOnlyNoCDHealthPack);
                        PubgToggle("Snaplines##hp", &mods::bHealthPackSnaplines);
                        PubgEndChild();
                    }
                    EndGroup();

                    SameLine(0, 10);

                    BeginGroup();
                    {
                        PubgBeginChild(T("ESP Settings"), ImVec2(270, 665));
                        PubgToggle(T("Hero Name##ext"), &mods::bShowHeroNameESP);
                        HelpMarker("Shows the hero the enemy is playing.");
                        PubgToggle(T("Hero Icons##ext"), &mods::bShowHeroIcons);
                        if (mods::bShowHeroIcons) PubgSliderFloat("##IconSize", &mods::heroIconSize, 16.0f, 48.0f, "%.0fpx");
                        PubgToggle(T("Kills (K/D/A)##ext"), &mods::bShowKillsESP);
                        PubgToggle("KDR##ext", &mods::bShowKDRESP);
                        PubgToggle("Healing##ext", &mods::bShowHealingESP);
                        PubgToggle("Kill Streak##ext", &mods::bShowKillStreakESP);
                        PubgToggle("Platform##ext", &mods::bShowPlatformESP);
                        ImGui::Spacing();
                        const char* posOpts[] = { "Left", "Right", "Top", "Bottom" };
                        int hbp = (int)mods::healthBarPosition;
                        PubgCombo("HP Bar Pos", &hbp, posOpts, 4); mods::healthBarPosition = (mods::ESPFeaturePosition)hbp;
                        int ubp = (int)mods::ultBarPosition;
                        PubgCombo("Ult Pos", &ubp, posOpts, 4); mods::ultBarPosition = (mods::ESPFeaturePosition)ubp;
                        int dpp = (int)mods::distancePosition;
                        PubgCombo("Dist Pos", &dpp, posOpts, 4); mods::distancePosition = (mods::ESPFeaturePosition)dpp;
                        int np = (int)mods::heroNamePosition;
                        PubgCombo("Name Pos", &np, posOpts, 4); mods::heroNamePosition = (mods::ESPFeaturePosition)np;
                        if (mods::bESPBox) {
                            const char* bOpts[] = { "2D", "3D", "Cornered" };
                            int bt = (int)mods::espBoxType;
                            PubgCombo("Box Type", &bt, bOpts, 3); mods::espBoxType = (mods::ESPBoxType)bt;
                            PubgSliderFloat("Thick##box", &mods::espBoxThickness, 1.0f, 5.0f);
                        }
                        PubgSliderFloat("Max Dist##esp", &mods::espMaxDistance, 10.0f, 500.0f, "%.0fm");
                        PubgSliderFloat("Distance Filter", &mods::distanceFilter, 0.0f, 1000.0f, "%.0fm");
                        PubgSliderFloat("Font Size##esp", &mods::espFontSize, 8.0f, 30.0f, "%.0f");
                        const char* styleOpts[] = { "Default", "Minimalist" };
                        int ds = (int)mods::drawStyle;
                        PubgCombo("Draw Style", &ds, styleOpts, 2); mods::drawStyle = (mods::DrawStyle)ds;
                        const char* tpOpts[] = { "Down", "Middle", "Up" };
                        int tp = (int)mods::textPosition;
                        PubgCombo("Text Position", &tp, tpOpts, 3); mods::textPosition = (mods::TextPosition)tp;
                        PubgEndChild();
                    }
                    EndGroup();

                    SameLine(0, 10);

                    BeginGroup();
                    {
                        PubgBeginChild(T("Chams & Crosshair"), ImVec2(270, 665));
                        PubgToggle(T("Enable Chams"), &mods::bChams);
                        ImGui::BeginDisabled(!mods::bChams);
                        PubgToggle("Through Walls##ch", &mods::bChamsThroughWalls);
                        PubgToggle("Health-Based Color##ch", &mods::bChamsHealthBased);
                        const char* chamsStyleOpts[] = { "Flat", "Wireframe", "Glow", "Pulse" };
                        int cs = (int)mods::chamsStyle;
                        PubgCombo("Style##chams", &cs, chamsStyleOpts, 4); mods::chamsStyle = (mods::ChamsStyle)cs;
                        PubgSliderFloat("Opacity##ch", &mods::chamsOpacity, 0.1f, 1.0f, "%.2f");
                        ImGui::EndDisabled();
                        PubgToggle(T("Enable Self Chams"), &mods::bSelfChams);
                        PubgToggle(T("Self Wireframe"), &mods::bSelfWireframe);
                        ImGui::BeginDisabled(!mods::bSelfChams);
                        const char* selfChamsOpts[] = { "Flat", "Wireframe", "Glow" };
                        int scs = (int)mods::selfChamsStyle;
                        PubgCombo("Style##selfchams", &scs, selfChamsOpts, 3); mods::selfChamsStyle = (mods::SelfChamsStyle)scs;
                        PubgSliderFloat("Opacity##sch", &mods::selfChamsOpacity, 0.1f, 1.0f, "%.2f");
                        ImGui::EndDisabled();
                        const char* xhTypes[] = { "None", "Dot", "Cross", "Circle" };
                        int xht = (int)mods::crosshairType;
                        PubgCombo("Crosshair##xh", &xht, xhTypes, 4); mods::crosshairType = (mods::CrosshairType)xht;
                        PubgSliderFloat("Size##xh", &mods::crosshairSize, 1.0f, 30.0f, "%.1f");
                        PubgSliderFloat("Thick##xh", &mods::crosshairThickness, 0.5f, 5.0f, "%.1f");
                        if (mods::crosshairType == mods::CROSS) PubgSliderFloat("Gap##xh", &mods::crosshairGap, 0.0f, 20.0f, "%.1f");
                        PubgToggle("Outline##xh", &mods::bCrosshairOutline);
                        PubgEndChild();
                    }
                    EndGroup();

                    SameLine(0, 10);

                    BeginGroup();
                    {
                        PubgBeginChild(T("Colors"), ImVec2(280, 665));
                        ImGui::PushFont(font::calibri_bold); ImGui::TextColored(dimText, "%s", T("ESP Colors")); ImGui::PopFont();
                        ImGui::ColorEdit5("Visible##col", (float*)&mods::visibleColor, picker_flags);
                        ImGui::ColorEdit5("Invisible##col", (float*)&mods::nonVisibleColor, picker_flags);
                        ImGui::ColorEdit5("Box Outline##col", (float*)&mods::espBoxOutlineColor, picker_flags);
                        ImGui::ColorEdit5("Skeleton##col", (float*)&mods::skeletonESPColor, picker_flags);
                        ImGui::ColorEdit5("Tracers##col", (float*)&mods::tracerColor, picker_flags);
                        ImGui::ColorEdit5("Crosshair##col", (float*)&mods::crosshairColor, picker_flags);
                        ImGui::Spacing();
                        ImGui::PushFont(font::calibri_bold); ImGui::TextColored(dimText, "%s", T("Text Colors")); ImGui::PopFont();
                        ImGui::ColorEdit5("Hero Name##col", (float*)&mods::heroNameTextColor, picker_flags);
                        ImGui::ColorEdit5("Distance##col", (float*)&mods::distanceTextColor, picker_flags);
                        ImGui::ColorEdit5("HP Text##col", (float*)&mods::healthTextColor, picker_flags);
                        ImGui::ColorEdit5("Ult Text##col", (float*)&mods::ultTextColor, picker_flags);
                        ImGui::Spacing();
                        ImGui::PushFont(font::calibri_bold); ImGui::TextColored(dimText, "%s", T("Health Bar Colors")); ImGui::PopFont();
                        ImGui::ColorEdit5("HP High##col", (float*)&mods::healthHighColor, picker_flags);
                        ImGui::ColorEdit5("HP Mid##col", (float*)&mods::healthMidColor, picker_flags);
                        ImGui::ColorEdit5("HP Low##col", (float*)&mods::healthLowColor, picker_flags);
                        ImGui::ColorEdit5("Ult Bar##col", (float*)&mods::ultBarColor, picker_flags);
                        ImGui::ColorEdit5("Bar BG##col", (float*)&mods::barBackgroundColor, picker_flags);
                        ImGui::Spacing();
                        ImGui::PushFont(font::calibri_bold); ImGui::TextColored(dimText, "%s", T("Chams Colors")); ImGui::PopFont();
                        ImGui::ColorEdit5("Chams Visible##col", (float*)&mods::chamsVisibleColor, picker_flags);
                        ImGui::ColorEdit5("Chams Hidden##col", (float*)&mods::chamsNotVisibleColor, picker_flags);
                        ImGui::ColorEdit5("Chams Outline##col", (float*)&mods::chamsOutlineColor, picker_flags);
                        ImGui::ColorEdit5("Self Chams##col", (float*)&mods::selfChamsColor, picker_flags);
                        ImGui::Spacing();
                        ImGui::PushFont(font::calibri_bold); ImGui::TextColored(dimText, "%s", T("Aim Colors")); ImGui::PopFont();
                        ImGui::ColorEdit5("FOV Circle##col", (float*)&mods::aimbotFovCircleColor, picker_flags);
                        ImGui::ColorEdit5("Snap Line##col", (float*)&mods::snapLineColor, picker_flags);
                        ImGui::ColorEdit5("pSilent FOV##col", (float*)&mods::pSilentFovColor, picker_flags);
                        ImGui::Spacing();
                        ImGui::PushFont(font::calibri_bold); ImGui::TextColored(dimText, "%s", T("Threat Colors")); ImGui::PopFont();
                        ImGui::ColorEdit5("Threat High##col", (float*)&mods::threatHighColor, picker_flags);
                        ImGui::ColorEdit5("Threat Med##col", (float*)&mods::threatMedColor, picker_flags);
                        ImGui::ColorEdit5("Threat Low##col", (float*)&mods::threatLowColor, picker_flags);
                        PubgEndChild();
                    }
                    EndGroup();
                }
                EndChild();
            }
            else if (active_tab == 2)
            {
                SetCursorPos(ImVec2(region.x - (tab_alpha * region.x - 18), 180 - (anim * 120)));
                BeginChild(false, "Child2", "o", ImVec2(1145, 855));
                {
                    BeginGroup();
                    {
                        PubgBeginChild(T("Combo Set 1"), ImVec2(270, 665));
                        PubgToggle(T("Enable Combos"), &mods::bCombosEnabled);
                        PubgToggle(T("Hold Key to Continue"), &mods::bComboHoldKey);
                        PubgToggle(T("Disable Triggerbot"), &mods::bComboDisableTriggerbot);
                        ImGui::Keybind("Combo Key 1", &mods::comboKey1);
                        ImGui::Spacing();
                        ImGui::BeginDisabled(!mods::bCombosEnabled);
                        PubgToggle("##cRogue", &mods::bComboRogue); ImGui::SameLine(); ImGui::Text("Rogue"); ImGui::SameLine(); ImGui::TextDisabled("- Shift+Shift+E+V+LMB+E+LMB");
                        PubgToggle("##cAngela", &mods::bComboAngela); ImGui::SameLine(); ImGui::Text("Angela"); ImGui::SameLine(); ImGui::TextDisabled("- V+LMB+E");
                        PubgToggle("##cGambit", &mods::bComboGambit); ImGui::SameLine(); ImGui::Text("Gambit"); ImGui::SameLine(); ImGui::TextDisabled("- E+LMB+LMB+Shift+LMB");
                        PubgToggle("##cDaredevil", &mods::bComboDaredevil); ImGui::SameLine(); ImGui::Text("Daredevil"); ImGui::SameLine(); ImGui::TextDisabled("- Shift+LMB+E+LMB+LMB");
                        PubgToggle("##cBP", &mods::bComboBlackPanther); ImGui::SameLine(); ImGui::Text("Black Panther"); ImGui::SameLine(); ImGui::TextDisabled("- Shift on marked");
                        if (mods::bComboBlackPanther) { ImGui::SameLine(); PubgToggle("Auto marker##bpmark", &mods::bComboBlackPantherAutoMark); }
                        PubgToggle("##cDS", &mods::bComboDoctorStrange); ImGui::SameLine(); ImGui::Text("Doctor Strange"); ImGui::SameLine(); ImGui::TextDisabled("- LMB+V+RMB");
                        if (mods::bComboDoctorStrange) { ImGui::SameLine(); PubgToggle("Hold shield##dsshield", &mods::bComboDoctorStrangeHoldShield); }
                        PubgToggle("##cM11", &mods::bComboMagic1_1); ImGui::SameLine(); ImGui::Text("Magik 1"); ImGui::SameLine(); ImGui::TextDisabled("- LMB+V+(RMB/E)+LMB+V");
                        if (mods::bComboMagic1_1) { ImGui::SameLine(); PubgToggle("Aimbot##m11aim", &mods::bComboMagic1_1Aimbot); }
                        PubgToggle("##cM12", &mods::bComboMagic1_2); ImGui::SameLine(); ImGui::Text("Magik 2"); ImGui::SameLine(); ImGui::TextDisabled("- Aim+E+LMB+V");
                        if (mods::bComboMagic1_2) { ImGui::Indent(20.0f); PubgSliderFloat("Hitbox Scale##m12", &mods::comboMagic1_2HitboxScale, 0.5f, 3.0f, "%.1f"); ImGui::Unindent(20.0f); }
                        PubgToggle("##cGroot", &mods::bComboGroot); ImGui::SameLine(); ImGui::Text("Groot"); ImGui::SameLine(); ImGui::TextDisabled("- E(hold)+Q+LMB+Shift");
                        PubgToggle("##cVenom", &mods::bComboVenom); ImGui::SameLine(); ImGui::Text("Venom"); ImGui::SameLine(); ImGui::TextDisabled("- LMB+V");
                        PubgToggle("##cJeff", &mods::bComboJeff); ImGui::SameLine(); ImGui::Text("Jeff"); ImGui::SameLine(); ImGui::TextDisabled("- E(hold)+LMB+Q+LMB");
                        PubgToggle("##cBW", &mods::bComboBlackWidow); ImGui::SameLine(); ImGui::Text("Black Widow"); ImGui::SameLine(); ImGui::TextDisabled("- LMB+V+E+E");
                        PubgToggle("##cCA", &mods::bComboCaptainAmerica); ImGui::SameLine(); ImGui::Text("Captain America"); ImGui::SameLine(); ImGui::TextDisabled("- LMB+E+RMB+LMB+F");
                        PubgToggle("##cPsy1", &mods::bComboPsylocke1); ImGui::SameLine(); ImGui::Text("Psylocke 1"); ImGui::SameLine(); ImGui::TextDisabled("- E+LMB+Shift+LMB+LMB");
                        PubgToggle("##cPsy12", &mods::bComboPsylocke1_2); ImGui::SameLine(); ImGui::Text("Psylocke 2"); ImGui::SameLine(); ImGui::TextDisabled("- Shift+E+LMB+LMB+Q");
                        PubgToggle("##cMag", &mods::bComboMagneto); ImGui::SameLine(); ImGui::Text("Magneto"); ImGui::SameLine(); ImGui::TextDisabled("- LMB+V+(RMB+LMB)");
                        ImGui::EndDisabled();
                        PubgEndChild();
                    }
                    EndGroup();

                    SameLine(0, 10);

                    BeginGroup();
                    {
                        PubgBeginChild(T("Combo Set 2"), ImVec2(270, 665));
                        ImGui::BeginDisabled(!mods::bCombosEnabled);
                        PubgToggle("##cSM11", &mods::bComboSpiderMan1_1); ImGui::SameLine(); ImGui::Text("Spider-Man 1"); ImGui::SameLine(); ImGui::TextDisabled("- E+LMB+Shift+LMB");
                        PubgToggle("##cSM12", &mods::bComboSpiderMan1_2); ImGui::SameLine(); ImGui::Text("Spider-Man 2"); ImGui::SameLine(); ImGui::TextDisabled("- Shift+E+LMB+LMB");
                        PubgToggle("##cSM13", &mods::bComboSpiderMan1_3); ImGui::SameLine(); ImGui::Text("Spider-Man 3"); ImGui::SameLine(); ImGui::TextDisabled("- E+LMB+Shift+LMB");
                        PubgToggle("##cMrF", &mods::bComboMisterFantastic); ImGui::SameLine(); ImGui::Text("Mister Fantastic"); ImGui::SameLine(); ImGui::TextDisabled("- E+LMB+Shift+Q+LMB");
                        PubgToggle("##cWS1", &mods::bComboWinterSoldier1_1); ImGui::SameLine(); ImGui::Text("Winter Soldier 1"); ImGui::SameLine(); ImGui::TextDisabled("- R+E+LMB+LMB");
                        PubgToggle("##cWS2", &mods::bComboWinterSoldier1_2); ImGui::SameLine(); ImGui::Text("Winter Soldier 2"); ImGui::SameLine(); ImGui::TextDisabled("- R+Shift+E+LMB");
                        PubgToggle("##cWolv", &mods::bComboWolverine); ImGui::SameLine(); ImGui::Text("Wolverine"); ImGui::SameLine(); ImGui::TextDisabled("- E+LMB+Shift+LMB+LMB");
                        PubgToggle("##cPhoe", &mods::bComboPhoenix); ImGui::SameLine(); ImGui::Text("Phoenix"); ImGui::SameLine(); ImGui::TextDisabled("- E+LMB+Q+Shift");
                        PubgToggle("##cThor", &mods::bComboThor); ImGui::SameLine(); ImGui::Text("Thor"); ImGui::SameLine(); ImGui::TextDisabled("- LMB+(RMB)+F+LMBx9");
                        PubgToggle("##cEmma", &mods::bComboEmmaFrost); ImGui::SameLine(); ImGui::Text("Emma Frost"); ImGui::SameLine(); ImGui::TextDisabled("- RMB+E+SHIFT+E+LMB+RMB");
                        PubgToggle("##cHawk", &mods::bComboHawkeye); ImGui::SameLine(); ImGui::Text("Hawkeye"); ImGui::SameLine(); ImGui::TextDisabled("- E+LMB+LMB+Shift");
                        ImGui::EndDisabled();
                        ImGui::Keybind("Combo Key 2", &mods::comboKey2);
                        ImGui::BeginDisabled(!mods::bCombosEnabled);
                        PubgToggle("##cSM21", &mods::bComboSpiderMan2_1); ImGui::SameLine(); ImGui::Text("Spider-Man (Non-Marked)"); ImGui::SameLine(); ImGui::TextDisabled("- LMB+E+LMB+Shift");
                        PubgToggle("##cNamor", &mods::bComboNamor); ImGui::SameLine(); ImGui::Text("Namor"); ImGui::SameLine(); ImGui::TextDisabled("- E(hold)+Shift+LMB+Q");
                        PubgToggle("##cMantis", &mods::bComboMantisAutoHeal); ImGui::SameLine(); ImGui::Text("Mantis"); ImGui::SameLine(); ImGui::TextDisabled("- Auto Heal: E+Q cycle");
                        PubgToggle("##cPhoeBey", &mods::bComboPhoenixBeyblade); ImGui::SameLine(); ImGui::Text("Phoenix Beyblade"); ImGui::SameLine(); ImGui::TextDisabled("- Shift+E+LMB+E+LMB+Shift");
                        PubgToggle("##cHT", &mods::bComboHumanTorch); ImGui::SameLine(); ImGui::Text("Human Torch"); ImGui::SameLine(); ImGui::TextDisabled("- E+LMB+Shift+LMB+Q");
                        ImGui::EndDisabled();
                        PubgEndChild();
                    }
                    EndGroup();

                    SameLine(0, 10);

                    BeginGroup();
                    {
                        PubgBeginChild(T("Auto Keys"), ImVec2(270, 665));
                        PubgToggle(T("Enable Auto Key"), &mods::bAutoKeyEnabled);
                        ImGui::BeginDisabled(!mods::bAutoKeyEnabled);
                        PubgToggle("Auto Melee##ak", &mods::bAutoMeleeKey);
                        if (mods::bAutoMeleeKey) {
                            PubgSliderFloat("Range (m)##mel", &mods::autoMeleeRange, 1.0f, 10.0f, "%.1f m");
                            ImGui::Keybind("Melee Key", &mods::autoMeleeVK);
                        }
                        PubgToggle("Auto Shield##ak", &mods::bAutoShieldAbility);
                        if (mods::bAutoShieldAbility) {
                            PubgSliderFloat("HP%%##shab", &mods::autoShieldAbilityHPPct, 5.0f, 80.0f, "%.0f%%");
                            ImGui::Keybind("Shield Key", &mods::autoShieldAbilityVK);
                        }
                        PubgToggle("Invisible Woman##ak", &mods::bAutoShieldIW);
                        if (mods::bAutoShieldIW) PubgSliderFloat("HP##iwshield", &mods::autoShieldIWHP, 1.0f, 276.0f, "%.0f HP");
                        PubgToggle("Cloak & Dagger##ak", &mods::bAutoShieldCD);
                        if (mods::bAutoShieldCD) PubgSliderFloat("HP##cdshield", &mods::autoShieldCDHP, 1.0f, 274.0f, "%.0f HP");
                        PubgToggle("Auto Immune##ak", &mods::bAutoImmune);
                        if (mods::bAutoImmune) {
                            PubgSliderFloat("HP%%##imm", &mods::autoImmuneHPPct, 1.0f, 50.0f, "%.0f%%");
                            ImGui::Keybind("Immune Key", &mods::autoImmuneVK);
                        }
                        ImGui::EndDisabled();
                        PubgEndChild();
                    }
                    EndGroup();

                    SameLine(0, 10);

                    BeginGroup();
                    {
                        PubgBeginChild(T("Auto Keys Extra & Dodge"), ImVec2(270, 665));
                        ImGui::BeginDisabled(!mods::bAutoKeyEnabled);
                        PubgToggle("Auto Kill##ak", &mods::bAutoKillAbility);
                        if (mods::bAutoKillAbility) {
                            PubgSliderFloat("Enemy HP##kill", &mods::autoKillAbilityEnemyHP, 1.0f, 500.0f, "%.0f HP");
                            ImGui::Keybind("Kill Key", &mods::autoKillAbilityVK);
                        }
                        PubgToggle("Anim Cancel##ak", &mods::bAnimCancel);
                        if (mods::bAnimCancel) {
                            PubgSliderInt("Delay (ms)##animc", &mods::animCancelDelayMs, 20, 300, "%d ms");
                            ImGui::Keybind("Anim Key", &mods::animCancelVK);
                        }
                        PubgToggle("Auto Buff##ak", &mods::bAutoBuff);
                        if (mods::bAutoBuff) { PubgSliderInt("Interval##buff", &mods::autoBuffIntervalMs, 500, 15000, "%d ms"); }
                        PubgToggle("Auto Heal##ak", &mods::bAutoKeyHeal);
                        if (mods::bAutoKeyHeal) { PubgSliderFloat("HP%%##akh", &mods::autoKeyHealHPPct, 5.0f, 90.0f, "%.0f%%"); }
                        PubgToggle("Auto Shift##ak", &mods::bAutoShiftAbility);
                        if (mods::bAutoShiftAbility) { PubgSliderInt("Interval##shft", &mods::autoShiftIntervalMs, 500, 10000, "%d ms"); }
                        ImGui::EndDisabled();
                        PubgToggle(T("Enable Dodge##dg"), &mods::bDodgeEnabled);
                        ImGui::BeginDisabled(!mods::bDodgeEnabled);
                        PubgSliderFloat("Max Range##dg", &mods::dodgeMaxRange, 5.0f, 200.0f, "%.0f m");
                        PubgToggle("Invisible Woman##dg", &mods::bDodgeInvisibleWoman);
                        PubgToggle("Rogue##dg", &mods::bDodgeRogue);
                        PubgToggle("Luna Snow##dg", &mods::bDodgeLuna);
                        PubgToggle("Hulk##dg", &mods::bDodgeHulk);
                        PubgToggle("Peni Parker##dg", &mods::bDodgePeniParker);
                        PubgToggle("Spider-Man##dg", &mods::bDodgeSpiderMan);
                        PubgToggle("Mantis##dg", &mods::bDodgeMantis);
                        ImGui::EndDisabled();
                        PubgEndChild();
                    }
                    EndGroup();

                }
                EndChild();
            }
            else if (active_tab == 3)
            {
                SetCursorPos(ImVec2(region.x - (tab_alpha * region.x - 18), 180 - (anim * 120)));
                BeginChild(false, "Child3", "o", ImVec2(1145, 855));
                {
                    BeginGroup();
                    {
                        PubgBeginChild(T("Healer Config"), ImVec2(270, 665));
                        PubgToggle(T("Healer Mode"), &mods::bHealerMode);
                        PubgToggle(T("Enable Auto Heal"), &mods::bAutoHeal);
                        if (mods::bAutoHeal) {
                            ImGui::BeginChild(false, "##AutoHealScroll", "", ImVec2(250, 120), true);
                            for (auto& [name, cfg] : mods::autoHealConfigs) {
                                ImGui::PushID(name.c_str());
                                PubgToggle(name.c_str(), &cfg.enabled);
                                if (cfg.enabled) {
                                    ImGui::SameLine(200);
                                    float rangeMax = 249.0f;
                                    if (name == "Loki" || name == "Cloak & Dagger") rangeMax = 274.0f;
                                    ImGui::PushItemWidth(120);
                                    PubgSliderFloat("##hp", &cfg.hpThreshold, 1.0f, rangeMax, "%.0f HP");
                                    ImGui::PopItemWidth();
                                }
                                ImGui::PopID();
                            }
                            ImGui::EndChild();
                        }
                        PubgToggle(T("Auto Heal Teammates"), &mods::bAutoHealTeammates);
                        PubgToggle(T("Priority: Lowest HP"), &mods::bHealPriorityLowest);
                        PubgSliderFloat(T("Heal Threshold"), &mods::healThresholdPercent, 10.0f, 100.0f, "%.0f%%");
                        PubgToggle(T("Smart Heal Target"), &mods::bSmartHealTarget);
                        PubgToggle(T("Line of Sight Check"), &mods::bHealerLOSCheck);
                        PubgEndChild();
                    }
                    EndGroup();

                    SameLine(0, 10);

                    BeginGroup();
                    {
                        PubgBeginChild(T("Shields & Abilities"), ImVec2(270, 665));
                        ImGui::BeginChild(false, "##AutoShieldScroll", "", ImVec2(250, 150), true);
                        for (auto& [name, cfg] : mods::autoShieldConfigs) {
                            ImGui::PushID(name.c_str());
                            PubgToggle(name.c_str(), &cfg.enabled);
                            if (cfg.enabled) {
                                ImGui::SameLine(200);
                                ImGui::PushItemWidth(120);
                                float shieldMax = 799.0f;
                                if (name == "Invisible Woman") shieldMax = 276.0f;
                                else if (name == "Namor") shieldMax = 274.0f;
                                else if (name == "Scarlet Witch") shieldMax = 249.0f;
                                else if (name == "Mister Fantastic") shieldMax = 374.0f;
                                else if (name == "Hulk") shieldMax = 749.0f;
                                else if (name == "Venom") shieldMax = 799.0f;
                                PubgSliderFloat("##shp", &cfg.hpThreshold, 1.0f, shieldMax, "%.0f HP");
                                ImGui::PopItemWidth();
                            }
                            ImGui::PopID();
                        }
                        ImGui::EndChild();
                        PubgToggle(T("Auto Ult Heal"), &mods::bAutoUltHeal);
                        if (mods::bAutoUltHeal) PubgSliderFloat("Ult HP Thresh", &mods::autoUltHealThreshold, 10.0f, 50.0f, "%.0f%%");
                        PubgToggle(T("Auto Shield Ability"), &mods::bHealerAutoShield);
                        if (mods::bHealerAutoShield) PubgSliderFloat("Shield Thresh", &mods::autoShieldThreshold, 10.0f, 80.0f, "%.0f%%");
                        PubgToggle(T("Low HP Alert"), &mods::bHealerNotifyLowHP);
                        if (mods::bHealerNotifyLowHP) PubgSliderFloat("Alert Thresh", &mods::healerNotifyThreshold, 5.0f, 50.0f, "%.0f%%");
                        PubgToggle(T("Teammate ESP"), &mods::bTeammateESP);
                        PubgToggle(T("Teammate HP Bars"), &mods::bTeammateHealthBars);
                        PubgToggle(T("Heal Range Ring"), &mods::bHealRangeIndicator);
                        if (mods::bHealRangeIndicator) PubgSliderFloat("Range##heal", &mods::healRange, 5.0f, 100.0f, "%.0fm");
                        PubgToggle(T("Healer Dashboard"), &mods::bHealerDashboard);
                        PubgEndChild();
                    }
                    EndGroup();

                    SameLine(0, 10);

                    BeginGroup();
                    {
                        PubgBeginChild(T("Heal Aim & Advanced"), ImVec2(270, 665));
                        PubgToggle(T("Enable Heal Aim"), &mods::bHealAim);
                        ImGui::Keybind(T("Heal Aim Key"), &mods::healAimKey);
                        ImGui::PushItemWidth(140);
                        PubgCombo("##healAimPri", &mods::healAimPriority, mods::healAimPriorityNames, 3);
                        ImGui::PopItemWidth();
                        PubgSliderFloat("FOV Radius##ha", &mods::healAimFov, 20.0f, 600.0f, "%.0f px");
                        PubgSliderFloat("Smoothing##ha", &mods::healAimSmoothing, 1.0f, 50.0f, "%.1f");
                        PubgToggle("Show FOV Circle##ha", &mods::bHealAimFovCircle);
                        PubgToggle(T("Smart Heal Queue"), &mods::bSmartHealQueue);
                        PubgToggle(T("Heal Prediction"), &mods::bHealPrediction);
                        PubgToggle(T("Auto Ability Rotation"), &mods::bAutoAbilityRotation);
                        if (mods::bAutoAbilityRotation) PubgSliderInt("Delay##aar", &mods::abilityRotationDelayMs, 50, 1000, "%d ms");
                        PubgToggle(T("Team HP Dashboard"), &mods::bTeamHPDashboard);
                        PubgToggle(T("Heal Snipe Alert"), &mods::bHealSnipeAlert);
                        if (mods::bHealSnipeAlert) PubgSliderFloat("Thresh##hsa", &mods::healSnipeAlertThreshold, 20.0f, 300.0f, "%.0f HP");
                        PubgEndChild();
                    }
                    EndGroup();

                    SameLine(0, 10);

                    BeginGroup();
                    {
                        PubgBeginChild(T("Teammate Display"), ImVec2(270, 665));
                        ImGui::PushFont(font::calibri_bold); ImGui::TextColored(dimText, "%s", T("Teammate Colors")); ImGui::PopFont();
                        ImGui::ColorEdit5("Teammate##tcol", (float*)&mods::teammateColor, picker_flags);
                        ImGui::ColorEdit5("Healable##tcol", (float*)&mods::teammateHealableColor, picker_flags);
                        ImGui::ColorEdit5("Critical##tcol", (float*)&mods::teammateCriticalColor, picker_flags);
                        ImGui::Spacing();
                        ImGui::PushFont(font::calibri_bold); ImGui::TextColored(dimText, "%s", T("Radar Colors")); ImGui::PopFont();
                        ImGui::ColorEdit5("Radar BG##rcol", (float*)&mods::radarBgColor, picker_flags);
                        ImGui::ColorEdit5("Radar Local##rcol", (float*)&mods::radarLocalColor, picker_flags);
                        ImGui::ColorEdit5("Radar Enemy##rcol", (float*)&mods::radarEnemyColor, picker_flags);
                        PubgEndChild();
                    }
                    EndGroup();
                }
                EndChild();
            }
            else if (active_tab == 4)
            {
                SetCursorPos(ImVec2(region.x - (tab_alpha * region.x - 18), 180 - (anim * 120)));
                BeginChild(false, "Child4", "o", ImVec2(1145, 855));
                {
                    BeginGroup();
                    {
                        PubgBeginChild(T("Movement Exploits"), ImVec2(270, 665));
                        PubgToggle(T("Speed Hack"), &mods::bSpeedHack);
                        if (mods::bSpeedHack) {
                            PubgSliderFloat("Speed##spd", &mods::speedHackMultiplier, 1.0f, 5.0f, "%.1fx");
                            PubgToggle("Pulse Mode##spd", &mods::bSpeedPulseMode);
                            if (mods::bSpeedPulseMode) {
                                PubgSliderInt("On (ms)##spd", &mods::speedPulseOnMs, 50, 500, "%d ms");
                                PubgSliderInt("Off (ms)##spd", &mods::speedPulseOffMs, 50, 500, "%d ms");
                            }
                            PubgToggle("Anti-Correction", &mods::bAntiCorrection);
                        }
                        PubgToggle(T("Super Jump"), &mods::bSuperJump);
                        if (mods::bSuperJump) PubgSliderFloat("Jump##sjmp", &mods::superJumpMultiplier, 1.0f, 10.0f, "%.1fx");
                        PubgToggle(T("Fly Hack"), &mods::bFlyHack);
                        if (mods::bFlyHack) {
                            PubgSliderFloat("Fly Speed", &mods::flySpeed, 100.0f, 2000.0f, "%.0f");
                            PubgToggle("Use Movement Mode##fly", &mods::bFlyUseMovementMode);
                            if (!mods::bFlyUseMovementMode)
                                PubgSliderFloat("Gravity", &mods::gravityScaleOverride, 0.0f, 1.0f, "%.2f");
                        }
                        PubgToggle(T("Infinite Dash"), &mods::bInfiniteDash);
                        PubgToggle(T("Wall Climb Anywhere"), &mods::bWallClimbAnywhere);
                        PubgToggle(T("No Fall Damage"), &mods::bNoFallDamage);
                        PubgEndChild();
                    }
                    EndGroup();

                    SameLine(0, 10);

                    BeginGroup();
                    {
                        PubgBeginChild(T("Player & Game"), ImVec2(270, 665));
                        PubgToggle(T("FOV Changer"), &mods::fov_changer);
                        if (mods::fov_changer) PubgSliderInt("FOV", &mods::fov_changer_amount, 1, 200);
                        PubgToggle(T("Spinbot"), &mods::bSpinbot);
                        if (mods::bSpinbot) {
                            PubgToggle("X##spin", &mods::bSpinbotX); ImGui::SameLine();
                            PubgToggle("Y##spin", &mods::bSpinbotY); ImGui::SameLine();
                            PubgToggle("Z##spin", &mods::bSpinbotZ);
                            PubgSliderFloat("Speed X", &mods::SpiningSpeedX, 1.0f, 50.f);
                            PubgSliderFloat("Speed Y", &mods::SpiningSpeedY, 1.0f, 50.f);
                            PubgSliderFloat("Speed Z", &mods::SpiningSpeedZ, 1.0f, 50.f);
                        }
                        PubgToggle(T("Small Player"), &mods::Experimental::SmallPerson);
                        if (mods::Experimental::SmallPerson) PubgSliderFloat("Scale", &mods::Experimental::SmallPersonScale, 0.1f, 5.0f, "%.1f");
                        PubgToggle(T("Hide Local"), &mods::Experimental::HideLocalPlayer);
                        PubgToggle(T("Self Speed Modifier"), &mods::SelfCustomTimeDilationBool);
                        if (mods::SelfCustomTimeDilationBool) PubgSliderFloat("Speed##self", &mods::SelfCustomTimeDilationFloat, 0.0f, 1.0f);
                        PubgToggle(T("Objective Timer"), &mods::bObjectiveTimer);
                        PubgToggle(T("Spawn Timer"), &mods::bSpawnTimer);
                        PubgToggle(T("Flank Alert"), &mods::bFlankAlert);
                        if (mods::bFlankAlert) {
                            PubgSliderFloat("Angle##fa", &mods::flankAlertAngle, 45.0f, 150.0f, "%.0f deg");
                            PubgSliderFloat("Range##fa", &mods::flankAlertRange, 500.0f, 10000.0f, "%.0f");
                            PubgToggle("Alert Sound##fa", &mods::bFlankAlertSound);
                        }
                        PubgEndChild();
                    }
                    EndGroup();

                    SameLine(0, 10);

                    BeginGroup();
                    {
                        PubgBeginChild(T("Overlay & Intel"), ImVec2(270, 665));
                        PubgToggle(T("Enemy Overlay"), &mods::bShowEnemyOverlay);
                        PubgToggle(T("Enemy Cooldown ESP"), &mods::bEnemyCooldownESP);
                        PubgToggle(T("Exact Ult Tracker"), &mods::bExactUltTracker);
                        PubgToggle(T("Health Pack Timer"), &mods::bHealthPackTimer);
                        PubgToggle(T("Enhanced Minimap"), &mods::bEnhancedMinimap);
                        if (mods::bEnhancedMinimap) {
                            PubgSliderFloat("Size##mm", &mods::minimapSize, 100.0f, 400.0f, "%.0f");
                            PubgSliderFloat("Zoom##mm", &mods::minimapZoom, 0.5f, 5.0f, "%.1f");
                        }
                        PubgEndChild();
                    }
                    EndGroup();

                    SameLine(0, 10);

                    BeginGroup();
                    {
                        PubgBeginChild(T("Misc & Utility"), ImVec2(270, 665));
                        PubgToggle(T("Threat Indicator"), &mods::bThreatIndicator);
                        if (mods::bThreatIndicator) {
                            PubgSliderFloat("High##threat", &mods::threatHighThreshold, 10.0f, 100.0f, "%.0f%%");
                            PubgSliderFloat("Med##threat", &mods::threatMedThreshold, 5.0f, 80.0f, "%.0f%%");
                        }
                        PubgToggle(T("Kill Prediction"), &mods::bKillPrediction);
                        PubgToggle(T("Damage Numbers"), &mods::bDamageNumbers);
                        PubgToggle(T("LOS Indicator"), &mods::bLOSIndicator);
                        PubgToggle(T("Sound ESP"), &mods::bSoundESP);
                        if (mods::bSoundESP) PubgSliderFloat("Range##sesp", &mods::soundESPRange, 500.0f, 10000.0f, "%.0f");
                        ImGui::Spacing();
                        PubgSliderFloat("Max ESP Dist", &mods::espMaxDistance, 10.0f, 500.0f, "%.0fm");
                        PubgSliderFloat("Dist Filter", &mods::distanceFilter, 0.0f, 1000.0f, "%.0fm");
                        PubgSliderFloat("Font Size", &mods::espFontSize, 8.0f, 30.0f, "%.0f");
                        PubgSliderFloat("HP Pack Dist", &mods::healthPackMaxDistance, 1.0f, 300.0f, "%.0fm");
                        PubgEndChild();
                    }
                    EndGroup();
                }
                EndChild();
            }
            else if (active_tab == 5)
            {
                SetCursorPos(ImVec2(region.x - (tab_alpha * region.x - 18), 180 - (anim * 120)));
                BeginChild(false, "Child5", "o", ImVec2(1145, 855));
                {
                    static int skinFilterIndex = 0;
                    static bool skinFilterCheck[64] = { true };

                    PubgToggle(T("Enable Skin Changer"), &mods::bSkinChanger);
                    ImGui::Spacing();

                    BeginGroup();
                    {
                        ImGui::BeginChild(true, "Skin Filter", "e", ImVec2(460, 665));
                        {
                            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, -10));

                            const int row = 2;
                            const ImVec2 desiredTextureSize(80.0f, 80.0f);

                            for (int i = 0; i < (int)sortedHeroes.size(); i++)
                            {
                                bool wasSelected = skinFilterCheck[i];

                                ImTextureID heroTex = HeroIcons::GetIcon(sortedHeroes[i].second);

                                bool heroEnabled = false;
                                auto eit = mods::skinEnabled.find(sortedHeroes[i].first);
                                if (eit != mods::skinEnabled.end()) heroEnabled = eit->second;
                                int lv = heroEnabled ? 1 : 0;

                                ImGui::Item_checkbox(
                                    desiredTextureSize,
                                    heroTex,
                                    sortedHeroes[i].second.c_str(),
                                    &skinFilterCheck[i],
                                    ImVec2(210, 90),
                                    lv
                                );

                                if (skinFilterCheck[i] && !wasSelected)
                                {
                                    for (int j = 0; j < (int)sortedHeroes.size(); j++)
                                    {
                                        if (j != i) skinFilterCheck[j] = false;
                                    }
                                    skinFilterIndex = i;
                                    mods::skinChangerSelectedHero = i;
                                }
                                else if (!skinFilterCheck[i] && wasSelected)
                                {
                                    skinFilterIndex = -1;
                                }

                                if ((i + 1) % row != 0 && i < (int)sortedHeroes.size() - 1)
                                {
                                    ImGui::SameLine();
                                }
                                else if (i < (int)sortedHeroes.size() - 1)
                                {
                                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 13);
                                }
                            }

                            ImGui::PopStyleVar();
                        }
                        ImGui::EndChild(true);
                    }
                    EndGroup();

                    SameLine();

                    BeginGroup();
                    {
                        ImGui::BeginChild(true, "Skin List", "f", ImVec2(660, 665));
                        {
                            if (ImGui::BeginTable("SkinTable", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_BordersH | ImGuiTableFlags_BordersOuter))
                            {
                                ImGui::TableSetupColumn("Image", ImGuiTableColumnFlags_WidthFixed, 55.0f);
                                ImGui::TableSetupColumn("Skin Name", ImGuiTableColumnFlags_WidthFixed, 180.0f);
                                ImGui::TableSetupColumn("Rarity", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                                ImGui::TableSetupColumn("Enable", ImGuiTableColumnFlags_WidthFixed, 80.0f);

                                ImGui::TableHeadersRow();

                                if (skinFilterIndex >= 0 && skinFilterIndex < (int)sortedHeroes.size())
                                {
                                    int32_t heroId = sortedHeroes[skinFilterIndex].first;

                                    static int32_t s_cachedSkinHeroId = -1;
                                    static std::vector<std::pair<int32_t, std::string>> s_cachedSkins;
                                    if (s_cachedSkinHeroId != heroId) {
                                        s_cachedSkinHeroId = heroId;
                                        s_cachedSkins = SkinChanger::GetSortedSkins(heroId);
                                    }

                                    int32_t curSkinId = 1;
                                    auto oit = mods::skinOverrides.find(heroId);
                                    if (oit != mods::skinOverrides.end()) curSkinId = oit->second;

                                    bool heroHasSkinEnabled = false;
                                    auto eit = mods::skinEnabled.find(heroId);
                                    if (eit != mods::skinEnabled.end()) heroHasSkinEnabled = eit->second;

                                    size_t index = 0;
                                    for (auto& sk : s_cachedSkins)
                                    {
                                        SkinChanger::SkinRarity r = SkinChanger::GetRarity(sk.second);
                                        ImVec4 rarCol = SkinChanger::GetRarityColor(r);
                                        const char* rarName = SkinChanger::GetRarityName(r);
                                        bool isSel = heroHasSkinEnabled && (curSkinId == sk.first);

                                        ImGui::PushID((int)index);
                                        ImGui::TableNextRow(ImGuiTableRowFlags_None, 48.0f);

                                        ImGui::TableNextColumn();
                                        if (ImGui::Selectable("##row", isSel, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap, ImVec2(0, 42))) {
                                            if (!isSel) {
                                                mods::skinOverrides[heroId] = sk.first;
                                                mods::skinEnabled[heroId] = true;
                                            } else {
                                                mods::skinOverrides[heroId] = 1;
                                                mods::skinEnabled[heroId] = false;
                                            }
                                        }
                                        ImGui::SameLine();
                                        ImTextureID heroTex = HeroIcons::GetIcon(sortedHeroes[skinFilterIndex].second);
                                        if (heroTex)
                                            ImGui::Image(heroTex, ImVec2(42, 42));
                                        else
                                            ImGui::Dummy(ImVec2(42, 42));

                                        ImGui::TableNextColumn();
                                        ImGui::Text("%s", sk.second.c_str());

                                        ImGui::TableNextColumn();
                                        ImGui::TextColored(rarCol, "%s", rarName);

                                        ImGui::TableNextColumn();
                                        if (isSel)
                                            ImGui::TextColored(ImVec4(0.396f, 1.0f, 0.667f, 1.0f), "ON");
                                        else
                                            ImGui::TextColored(ImVec4(0.35f, 0.35f, 0.38f, 1.0f), "---");

                                        ImGui::PopID();
                                        ++index;
                                    }
                                }

                                ImGui::EndTable();
                            }
                        }
                        ImGui::EndChild(true);
                    }
                    EndGroup();
                }
                EndChild();
            }
            else if (active_tab == 6)
            {
                SetCursorPos(ImVec2(region.x - (tab_alpha * region.x - 18), 180 - (anim * 120)));
                BeginChild(false, "Child6", "o", ImVec2(1145, 855));
                {
                    static char cfgNameBuf[64] = "my_config";
                    static int  localSel       = -1;
                    static bool localDirty     = true;

                    BeginGroup();
                    {
                        PubgBeginChild(T("Keybinds & General"), ImVec2(370, 665));
                        ImGui::PushFont(font::calibri_bold);
                        ImGui::TextColored(accent, "%s", T("Keybinds"));
                        ImGui::PopFont();
                        ImGui::Separator();
                        ImGui::Keybind(T("Menu Key"), &mods::menuToggleKey);
                        ImGui::Keybind(T("Safe Exit"), &mods::safeExitKey);
                        ImGui::Keybind(T("Aim Key"), &mods::aimbotKey);
                        ImGui::Keybind(T("ESP Toggle"), &mods::espHotkey);
                        ImGui::Keybind(T("Glow Toggle"), &mods::glowHotkey);
                        ImGui::Keybind(T("Spinbot Toggle"), &mods::spinbotHotkey);
                        ImGui::Keybind(T("Heal Aim Key"), &mods::healAimKey);
                        ImGui::Spacing();
                        ImGui::PushFont(font::calibri_bold);
                        ImGui::TextColored(accent, "%s", T("General"));
                        ImGui::PopFont();
                        ImGui::Separator();
                        PubgToggle(T("Watermark"), &mods::bWatermark);
                        PubgToggle(T("Keybind Widget"), &mods::bKeybindWidget);
                        PubgToggle(T("Active Features"), &mods::bActiveFeaturesWidget);
                        PubgToggle(T("Ban Phase Overlay"), &mods::bBanPhaseOverlay);
                        PubgToggle(T("Ult Tracker"), &mods::bUltTracker);
                        PubgToggle(T("Healer Dashboard"), &mods::bHealerDashboard);
                        { bool prev = mods::bAntiScreenshot;
                          PubgToggle(T("Anti-Screenshot"), &mods::bAntiScreenshot);
                          if (mods::bAntiScreenshot != prev) {
                              if (mods::bAntiScreenshot) AntiScreenshot::Enable();
                              else                       AntiScreenshot::Disable();
                          }
                        }
                        PubgToggle(T("Stream Safe Mode"), &mods::bStreamSafeMode);
                        ImGui::Spacing();
                        ImGui::PushFont(font::calibri_bold);
                        ImGui::TextColored(accent, "%s", T("Language"));
                        ImGui::PopFont();
                        ImGui::Separator();
                        {
                            const char* LanguageItems[] = { "[CN] \xe7\xae\x80\xe4\xbd\x93\xe4\xb8\xad\xe6\x96\x87", "[US] English" };
                            ImGui::SetNextItemWidth(230);
                            ImGui::Combo_popup(mods::currentLanguage == 0 ? "\xe8\xaf\xad\xe8\xa8\x80\xe5\x88\x87\xe6\x8d\xa2" : "Language Toggle", &mods::currentLanguage, LanguageItems, IM_ARRAYSIZE(LanguageItems));
                        }
                        PubgEndChild();
                    }
                    EndGroup();

                    SameLine(0, 10);

                    BeginGroup();
                    {
                        PubgBeginChild(T("Widgets & Anti-Detection"), ImVec2(370, 665));
                        ImGui::PushFont(font::calibri_bold);
                        ImGui::TextColored(accent, "%s", T("Widgets"));
                        ImGui::PopFont();
                        ImGui::Separator();
                        PubgToggle(T("Damage Log"), &mods::bDamageLog);
                        if (mods::bDamageLog) {
                            PubgSliderInt("Max Entries", &mods::damageLogMaxEntries, 3, 20);
                            PubgSliderFloat("Fade Time", &mods::damageLogFadeTime, 2.0f, 15.0f, "%.1fs");
                        }
                        PubgToggle(T("Radar"), &mods::bRadar);
                        if (mods::bRadar) {
                            const char* rOpts[] = { "Circular", "Square" };
                            int rd = (int)mods::radarDesign;
                            PubgCombo("Style##r", &rd, rOpts, 2); mods::radarDesign = (mods::RadarDesign)rd;
                            PubgSliderFloat("Size##r", &mods::radarSize, 80.0f, 300.0f, "%.0f");
                            PubgSliderFloat("Range##r", &mods::radarRange, 1000.0f, 20000.0f, "%.0f");
                            PubgSliderFloat("Zoom##r", &mods::radarZoom, 0.5f, 3.0f, "%.1fx");
                        }
                        PubgToggle(T("Session Stats"), &mods::bSessionStats);
                        PubgToggle(T("Match Timer"), &mods::bMatchTimer);
                        PubgToggle(T("Hotkey Cheat Sheet"), &mods::bHotkeyCheatSheet);
                        PubgToggle(T("Match Status Widget"), &mods::bMatchStatusWidget);
                        ImGui::Spacing();
                        ImGui::PushFont(font::calibri_bold);
                        ImGui::TextColored(accent, "%s", T("Anti-Detection"));
                        ImGui::PopFont();
                        ImGui::Separator();
                        PubgToggle(T("Input Jitter"), &mods::bInputJitter);
                        if (mods::bInputJitter) PubgSliderFloat("Jitter##ms", &mods::inputJitterMs, 5.0f, 50.0f, "%.0f ms");
                        PubgToggle(T("Timing Randomization"), &mods::bTimingRandomization);
                        if (mods::bTimingRandomization) PubgSliderFloat("Variance", &mods::timingVariance, 0.1f, 1.0f, "%.1f");
                        PubgToggle(T("Humanized Mouse"), &mods::bHumanizedMouse);
                        if (mods::bHumanizedMouse) PubgSliderFloat("Strength##hm", &mods::mouseHumanizeStrength, 0.1f, 1.0f, "%.1f");
                        PubgEndChild();
                    }
                    EndGroup();

                    SameLine(0, 10);

                    BeginGroup();
                    {
                        PubgBeginChild(T("Configs"), ImVec2(370, 665));

                        if (localDirty || configListDirty) {
                            try { mods::configFiles = ConfigSystem::GetConfigList(); } catch (...) {}
                            localDirty = false;
                            configListDirty = false;
                            if (localSel >= (int)mods::configFiles.size()) localSel = -1;
                        }

                        ImGui::TextColored(dimText, "%s", T("Active:"));
                        ImGui::SameLine();
                        ImGui::TextColored(accent, "%s", mods::currentConfigName.empty() ? T("none") : mods::currentConfigName.c_str());
                        ImGui::Spacing();

                        ImGui::PushItemWidth(-1);
                        ImGui::InputText("##cfgname", cfgNameBuf, sizeof(cfgNameBuf));
                        ImGui::PopItemWidth();

                        float fullW = ImGui::GetContentRegionAvail().x;
                        float halfW = (fullW - 4.0f) * 0.5f;
                        if (ImGui::Button("Save##lc", ImVec2(halfW, 26))) {
                            if (strlen(cfgNameBuf) > 0) {
                                ConfigSystem::SaveConfig(cfgNameBuf);
                                localDirty = true;
                            }
                        }
                        ImGui::SameLine(0, 4);
                        if (ImGui::Button("Save As##lc", ImVec2(halfW, 26)))
                            ImGui::OpenPopup("##SaveAsPopup");

                        static char saveAsBuf[64] = "";
                        if (ImGui::BeginPopup("##SaveAsPopup")) {
                            ImGui::TextColored(dimText, "New name:");
                            ImGui::SetNextItemWidth(160);
                            ImGui::InputText("##sanew", saveAsBuf, sizeof(saveAsBuf));
                            ImGui::SameLine();
                            if (ImGui::Button("OK", ImVec2(40, 26)) && strlen(saveAsBuf) > 0) {
                                ConfigSystem::SaveConfig(saveAsBuf);
                                strncpy_s(cfgNameBuf, saveAsBuf, sizeof(cfgNameBuf) - 1);
                                localDirty = true;
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::SameLine();
                            if (ImGui::Button("Cancel", ImVec2(60, 26))) ImGui::CloseCurrentPopup();
                            ImGui::EndPopup();
                        }

                        ImGui::Spacing();

                        float lpW = ImGui::GetContentRegionAvail().x;
                        float listH = mods::configFiles.empty() ? 38.0f
                            : (std::min)(380.0f, (float)mods::configFiles.size() * 22.0f + 6.0f);
                        ImGui::BeginChild(false, "##LocalList", "", ImVec2(lpW, listH), true);
                        if (mods::configFiles.empty()) {
                            ImGui::TextColored(dimText, "  No configs yet.");
                        }
                        for (int i = 0; i < (int)mods::configFiles.size(); i++) {
                            bool cfgActive = (mods::configFiles[i] == mods::currentConfigName);
                            bool sel = (localSel == i);

                            if (cfgActive) {
                                ImVec2 rp = ImGui::GetCursorScreenPos();
                                ImGui::GetWindowDrawList()->AddRectFilled(rp,
                                    ImVec2(rp.x + ImGui::GetContentRegionAvail().x, rp.y + 20),
                                    IM_COL32(101, 255, 170, 30));
                            }

                            ImGui::PushID(i);
                            if (ImGui::Selectable(("  " + mods::configFiles[i] + (cfgActive ? " *" : "")).c_str(),
                                    sel, ImGuiSelectableFlags_AllowDoubleClick)) {
                                localSel = i;
                                strncpy_s(cfgNameBuf, mods::configFiles[i].c_str(), sizeof(cfgNameBuf) - 1);
                                if (ImGui::IsMouseDoubleClicked(0))
                                    ConfigSystem::LoadConfig(mods::configFiles[i]);
                            }
                            ImGui::PopID();
                        }
                        ImGui::EndChild();

                        if (localSel >= 0 && localSel < (int)mods::configFiles.size()) {
                            float halfW2 = (ImGui::GetContentRegionAvail().x - 4.0f) * 0.5f;
                            if (ImGui::Button("Load##lc2", ImVec2(halfW2, 26))) {
                                ConfigSystem::LoadConfig(mods::configFiles[localSel]);
                                localDirty = true;
                            }
                            ImGui::SameLine(0, 4);
                            if (ImGui::Button("Delete##lc2", ImVec2(halfW2, 26))) {
                                ConfigSystem::DeleteConfig(mods::configFiles[localSel]);
                                localSel = -1;
                                localDirty = true;
                            }
                        }

                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();
                        float qW = ImGui::GetContentRegionAvail().x;
                        if (ImGui::Button("Load Default##qi", ImVec2(qW, 26))) {
                            ConfigSystem::LoadConfig("default");
                            localDirty = true;
                        }
                        if (ImGui::Button("Save Active##qi", ImVec2(qW, 26))) {
                            if (!mods::currentConfigName.empty()) {
                                ConfigSystem::SaveConfig(mods::currentConfigName);
                                localDirty = true;
                            }
                        }
                        ImGui::Spacing();
                        ImGui::TextColored(dimText, "Double-click to load.");

                        PubgEndChild();
                    }
                    EndGroup();

                }
                EndChild();
            }

        PopStyleVar(); // tab_alpha
        PopFont();
    }
    End();
}

HRESULT APIENTRY hkPresent(IDXGISwapChain3* pSwapChain, UINT SyncInterval, UINT Flags) {
#ifdef TRIGGER_ONLY_EXTERNAL
    if (!g_Unloading) TriggerOnly::Tick();
    return oPresent(pSwapChain, SyncInterval, Flags);
#endif

    if (g_Unloading) return oPresent(pSwapChain, SyncInterval, Flags);
    if (!pSwapChain) {
        DBG_LOG("swap null");
        return E_POINTER;
    }

    HRESULT hr = S_OK;

    if (!ImGui_Initialised) {
        if (FAILED(pSwapChain->GetDevice(__uuidof(ID3D12Device), (void**)&DirectX12Interface::Device))) {
            DBG_LOG("dev fail");
            return oPresent(pSwapChain, SyncInterval, Flags);
        }
        if (!DirectX12Interface::Device) {
            DBG_LOG("dev null");
            return E_FAIL;
        }

        hr = DirectX12Interface::Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&DirectX12Interface::Fence));
        if (FAILED(hr)) {
            DBG_LOG("fence fail 0x%X", hr);
            DirectX12Interface::Device->Release();
            DirectX12Interface::Device = nullptr;
            return oPresent(pSwapChain, SyncInterval, Flags);
        }

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        mods::fontNames.clear();
        mods::availableFonts.clear();

        ImFont* defaultFont = io.Fonts->AddFontDefault();
        if (defaultFont) {
            mods::availableFonts.push_back(defaultFont);
            mods::fontNames.push_back("Default");
        }

        ImFont* tahomaFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\tahoma.ttf", mods::baseFontSize);
        if (tahomaFont) {
            mods::availableFonts.push_back(tahomaFont);
            mods::fontNames.push_back("Tahoma");
        }

        ImFont* arialFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", mods::baseFontSize);
        if (arialFont) {
            mods::availableFonts.push_back(arialFont);
            mods::fontNames.push_back("Arial");
        }

        ImFont* courierFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\cour.ttf", mods::baseFontSize);
        if (courierFont) {
            mods::availableFonts.push_back(courierFont);
            mods::fontNames.push_back("Courier New");
        }

        ImFont* verdanaFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\verdana.ttf", mods::baseFontSize);
        if (verdanaFont) {
            mods::availableFonts.push_back(verdanaFont);
            mods::fontNames.push_back("Verdana");
        }

        ImFont* impactFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\impact.ttf", mods::baseFontSize);
        if (impactFont) {
            mods::availableFonts.push_back(impactFont);
            mods::fontNames.push_back("Impact");
        }

        ImFont* comicFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\comic.ttf", mods::baseFontSize);
        if (comicFont) {
            mods::availableFonts.push_back(comicFont);
            mods::fontNames.push_back("Comic Sans MS");
        }

        {
            ImFontConfig fc;
            fc.FontDataOwnedByAtlas = false;

            font::calibri_bold = io.Fonts->AddFontFromMemoryTTF((void*)calibri_bold, sizeof(calibri_bold), 14.0f, &fc);
            {
                ImFontConfig merge_cfg;
                merge_cfg.MergeMode = true;
                io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 14.0f, &merge_cfg, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
            }
            fc.FontDataOwnedByAtlas = false;
            font::calibri_regular = io.Fonts->AddFontFromMemoryTTF((void*)calibri_regular, sizeof(calibri_regular), 14.0f, &fc);
            {
                ImFontConfig merge_cfg;
                merge_cfg.MergeMode = true;
                io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 14.0f, &merge_cfg, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
            }
            fc.FontDataOwnedByAtlas = false;
            font::icomoon = io.Fonts->AddFontFromMemoryTTF((void*)icomoon, sizeof(icomoon), 18.0f, &fc);
            fc.FontDataOwnedByAtlas = false;
            font::icomoon_menu = io.Fonts->AddFontFromMemoryTTF((void*)icomoon_sizeof, sizeof(icomoon_sizeof), 15.0f, &fc);
            fc.FontDataOwnedByAtlas = false;
            font::weapon_val = io.Fonts->AddFontFromMemoryTTF((void*)weapon_icon, sizeof(weapon_icon), 16.0f, &fc);
            fc.FontDataOwnedByAtlas = false;
            font_inter::inter_bold = io.Fonts->AddFontFromMemoryTTF((void*)calibri_bold, sizeof(calibri_bold), 14.0f, &fc);
            fc.FontDataOwnedByAtlas = false;
            font::pixel_7_small = io.Fonts->AddFontFromMemoryTTF((void*)pixel_7_small, sizeof(pixel_7_small), 10.0f, &fc);
            fc.FontDataOwnedByAtlas = false;
            font::calibri_bold_hint = io.Fonts->AddFontFromMemoryTTF((void*)calibri_bold, sizeof(calibri_bold), 12.0f, &fc);

            ImFont* fallback = io.Fonts->Fonts.Size > 0 ? io.Fonts->Fonts[0] : nullptr;
            if (!font::calibri_bold) font::calibri_bold = fallback;
            if (!font::calibri_regular) font::calibri_regular = fallback;
            if (!font::icomoon) font::icomoon = fallback;
            if (!font::icomoon_menu) font::icomoon_menu = fallback;
            if (!font::weapon_val) font::weapon_val = fallback;
            if (!font_inter::inter_bold) font_inter::inter_bold = fallback;
            if (!font::pixel_7_small) font::pixel_7_small = fallback;
            if (!font::calibri_bold_hint) font::calibri_bold_hint = fallback;
        }

        io.Fonts->Build();

        DXGI_SWAP_CHAIN_DESC Desc;
        hr = pSwapChain->GetDesc(&Desc);
        if (FAILED(hr)) {
            DBG_LOG("sc desc fail 0x%X", hr);
            DirectX12Interface::Fence->Release();
            DirectX12Interface::Fence = nullptr;
            DirectX12Interface::Device->Release();
            DirectX12Interface::Device = nullptr;
            return oPresent(pSwapChain, SyncInterval, Flags);
        }

        Desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        Desc.OutputWindow = Process::Hwnd;
        Desc.Windowed = ((GetWindowLongPtr(Process::Hwnd, GWL_STYLE) & WS_POPUP) != 0) ? false : true;

        DirectX12Interface::BuffersCounts = Desc.BufferCount;
        DirectX12Interface::FrameContext = new(std::nothrow) DirectX12Interface::_FrameContext[DirectX12Interface::BuffersCounts];
        if (!DirectX12Interface::FrameContext) {
            DBG_LOG("fc alloc fail");
            DirectX12Interface::Fence->Release();
            DirectX12Interface::Fence = nullptr;
            DirectX12Interface::Device->Release();
            DirectX12Interface::Device = nullptr;
            return E_OUTOFMEMORY;
        }

        D3D12_DESCRIPTOR_HEAP_DESC DescriptorImGuiRender = {};
        DescriptorImGuiRender.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        DescriptorImGuiRender.NumDescriptors = DirectX12Interface::BuffersCounts + HeroIcons::MAX_ICON_SLOTS + 3;
        DescriptorImGuiRender.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        hr = DirectX12Interface::Device->CreateDescriptorHeap(&DescriptorImGuiRender, IID_PPV_ARGS(&DirectX12Interface::DescriptorHeapImGuiRender));
        if (FAILED(hr)) {
            DBG_LOG("dh fail 0x%X", hr);
            delete[] DirectX12Interface::FrameContext;
            DirectX12Interface::FrameContext = nullptr;
            DirectX12Interface::Fence->Release();
            DirectX12Interface::Fence = nullptr;
            DirectX12Interface::Device->Release();
            DirectX12Interface::Device = nullptr;
            return oPresent(pSwapChain, SyncInterval, Flags);
        }

        ID3D12CommandAllocator* Allocator = nullptr;
        hr = DirectX12Interface::Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&Allocator));
        if (FAILED(hr)) {
            DBG_LOG("ca fail 0x%X", hr);
            DirectX12Interface::DescriptorHeapImGuiRender->Release();
            delete[] DirectX12Interface::FrameContext;
            DirectX12Interface::FrameContext = nullptr;
            DirectX12Interface::Fence->Release();
            DirectX12Interface::Fence = nullptr;
            DirectX12Interface::Device->Release();
            DirectX12Interface::Device = nullptr;
            return oPresent(pSwapChain, SyncInterval, Flags);
        }

        for (size_t i = 0; i < DirectX12Interface::BuffersCounts; i++) {
            DirectX12Interface::FrameContext[i].CommandAllocator = Allocator;
        }

        hr = DirectX12Interface::Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, Allocator, NULL, IID_PPV_ARGS(&DirectX12Interface::CommandList));
        if (FAILED(hr) || FAILED(DirectX12Interface::CommandList->Close())) {
            DBG_LOG("cl fail 0x%X", hr);
            Allocator->Release();
            DirectX12Interface::DescriptorHeapImGuiRender->Release();
            delete[] DirectX12Interface::FrameContext;
            DirectX12Interface::FrameContext = nullptr;
            DirectX12Interface::Fence->Release();
            DirectX12Interface::Fence = nullptr;
            DirectX12Interface::Device->Release();
            DirectX12Interface::Device = nullptr;
            return oPresent(pSwapChain, SyncInterval, Flags);
        }

        D3D12_DESCRIPTOR_HEAP_DESC DescriptorBackBuffers = {};
        DescriptorBackBuffers.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        DescriptorBackBuffers.NumDescriptors = DirectX12Interface::BuffersCounts;
        DescriptorBackBuffers.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        DescriptorBackBuffers.NodeMask = 1;

        hr = DirectX12Interface::Device->CreateDescriptorHeap(&DescriptorBackBuffers, IID_PPV_ARGS(&DirectX12Interface::DescriptorHeapBackBuffers));
        if (FAILED(hr)) {
            DBG_LOG("bb dh fail 0x%X", hr);
            DirectX12Interface::CommandList->Release();
            Allocator->Release();
            DirectX12Interface::DescriptorHeapImGuiRender->Release();
            delete[] DirectX12Interface::FrameContext;
            DirectX12Interface::FrameContext = nullptr;
            DirectX12Interface::Fence->Release();
            DirectX12Interface::Fence = nullptr;
            DirectX12Interface::Device->Release();
            DirectX12Interface::Device = nullptr;
            return oPresent(pSwapChain, SyncInterval, Flags);
        }

        const auto RTVDescriptorSize = DirectX12Interface::Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE RTVHandle = DirectX12Interface::DescriptorHeapBackBuffers->GetCPUDescriptorHandleForHeapStart();

        for (size_t i = 0; i < DirectX12Interface::BuffersCounts; i++) {
            ID3D12Resource* pBackBuffer = nullptr;
            hr = pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
            if (FAILED(hr)) {
                DBG_LOG("bb %zu fail 0x%X", i, hr);
                DirectX12Interface::DescriptorHeapBackBuffers->Release();
                DirectX12Interface::CommandList->Release();
                Allocator->Release();
                DirectX12Interface::DescriptorHeapImGuiRender->Release();
                delete[] DirectX12Interface::FrameContext;
                DirectX12Interface::FrameContext = nullptr;
                DirectX12Interface::Fence->Release();
                DirectX12Interface::Fence = nullptr;
                DirectX12Interface::Device->Release();
                DirectX12Interface::Device = nullptr;
                return oPresent(pSwapChain, SyncInterval, Flags);
            }
            DirectX12Interface::FrameContext[i].DescriptorHandle = RTVHandle;
            DirectX12Interface::Device->CreateRenderTargetView(pBackBuffer, nullptr, RTVHandle);
            DirectX12Interface::FrameContext[i].Resource = pBackBuffer;
            RTVHandle.ptr += RTVDescriptorSize;
        }

        if (!ImGui_ImplWin32_Init(Process::Hwnd) || !ImGui_ImplDX12_Init(DirectX12Interface::Device, DirectX12Interface::BuffersCounts, DXGI_FORMAT_R8G8B8A8_UNORM, DirectX12Interface::DescriptorHeapImGuiRender, DirectX12Interface::DescriptorHeapImGuiRender->GetCPUDescriptorHandleForHeapStart(), DirectX12Interface::DescriptorHeapImGuiRender->GetGPUDescriptorHandleForHeapStart())) {
            DBG_LOG("imgui init fail");
            for (size_t i = 0; i < DirectX12Interface::BuffersCounts; i++) {
                if (DirectX12Interface::FrameContext[i].Resource) DirectX12Interface::FrameContext[i].Resource->Release();
            }
            DirectX12Interface::DescriptorHeapBackBuffers->Release();
            DirectX12Interface::CommandList->Release();
            Allocator->Release();
            DirectX12Interface::DescriptorHeapImGuiRender->Release();
            delete[] DirectX12Interface::FrameContext;
            DirectX12Interface::FrameContext = nullptr;
            DirectX12Interface::Fence->Release();
            DirectX12Interface::Fence = nullptr;
            DirectX12Interface::Device->Release();
            DirectX12Interface::Device = nullptr;
            return oPresent(pSwapChain, SyncInterval, Flags);
        }

        ImGui_ImplDX12_CreateDeviceObjects();
        ImGui::GetIO().ImeWindowHandle = Process::Hwnd;
        Process::WndProc = (WNDPROC)SetWindowLongPtr(Process::Hwnd, GWLP_WNDPROC, (__int3264)(LONG_PTR)WndProc);
        ImGui_Initialised = true;
        LoadingScreen::Init();
    }

    if (!DirectX12Interface::CommandQueue) {
        DBG_LOG("cq null");
        return oPresent(pSwapChain, SyncInterval, Flags);
    }

    if (ImGui_Initialised) {
        if (!HeroIcons::bInitialized) {
            HeroIcons::Init(DirectX12Interface::Device,
                            DirectX12Interface::DescriptorHeapImGuiRender,
                            DirectX12Interface::CommandQueue);
            MatchStatus::Init(DirectX12Interface::Device,
                              DirectX12Interface::DescriptorHeapImGuiRender,
                              DirectX12Interface::CommandQueue);
        }
        HeroIcons::Tick();
        LoadingScreen::Tick(DirectX12Interface::Device,
                           DirectX12Interface::DescriptorHeapImGuiRender,
                           DirectX12Interface::CommandQueue);
        CharacterTexture::Init(DirectX12Interface::Device,
                               DirectX12Interface::DescriptorHeapImGuiRender,
                               DirectX12Interface::CommandQueue);
        PlayerModelTexture::Init(DirectX12Interface::Device,
                                 DirectX12Interface::DescriptorHeapImGuiRender,
                                 DirectX12Interface::CommandQueue);
    }

    static UINT64 fenceValue = 0;
    static HANDLE fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
    hr = DirectX12Interface::CommandQueue->Signal(DirectX12Interface::Fence, ++fenceValue);
    if (FAILED(hr)) {
        DBG_LOG("fence sig fail 0x%X", hr);
        return oPresent(pSwapChain, SyncInterval, Flags);
    }

    if (DirectX12Interface::Fence->GetCompletedValue() < fenceValue) {
        if (fenceEvent == nullptr) {
            DBG_LOG("evt fail");
            return oPresent(pSwapChain, SyncInterval, Flags);
        }
        hr = DirectX12Interface::Fence->SetEventOnCompletion(fenceValue, fenceEvent);
        if (FAILED(hr)) {
            DBG_LOG("evt comp fail 0x%X", hr);
            return oPresent(pSwapChain, SyncInterval, Flags);
        }
        WaitForSingleObject(fenceEvent, INFINITE);
    }


    if (GetAsyncKeyState(mods::menuToggleKey) & 1) ShowMenu = !ShowMenu;

    if (GetAsyncKeyState(mods::safeExitKey) & 1) {
        g_Unloading = true;
        HANDLE hUnload = CreateThread(nullptr, 0, UnloadThread, nullptr, 0, nullptr);
        if (hUnload) CloseHandle(hUnload);
        return oPresent(pSwapChain, SyncInterval, Flags);
    }

    if (GetAsyncKeyState(mods::espHotkey) & 1) mods::esp = !mods::esp;
    if (GetAsyncKeyState(mods::glowHotkey) & 1) mods::bGlow = !mods::bGlow;
    if (GetAsyncKeyState(mods::bulletTPHotkey) & 1) mods::bulletTP = !mods::bulletTP;
    if (GetAsyncKeyState(mods::spinbotHotkey) & 1) mods::bSpinbot = !mods::bSpinbot;
    if (mods::rageKey && (GetAsyncKeyState(mods::rageKey) & 1)) mods::bRageMode = !mods::bRageMode;

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::GetIO().MouseDrawCursor = ShowMenu;

    if (messageTimer > 0.0f) {
        messageTimer -= ImGui::GetIO().DeltaTime;
        if (messageTimer <= 0.0f) statusMessage = "";
    }


    if (!mods::settingHotkeyFor.empty()) {
        for (int i = 1; i < 256; i++) {
            if (GetAsyncKeyState(i) & 0x8000) {
                if (mods::settingHotkeyFor == "ESP") {
                    mods::espHotkey = i;
                    mods::espHotkeyName = GetKeyName(i);
                }
                else if (mods::settingHotkeyFor == "Glow") {
                    mods::glowHotkey = i;
                    mods::glowHotkeyName = GetKeyName(i);
                }
                else if (mods::settingHotkeyFor == "BulletTP") {
                    mods::bulletTPHotkey = i;
                    mods::bulletTPHotkeyName = GetKeyName(i);
                }
                else if (mods::settingHotkeyFor == "Spinbot") {
                    mods::spinbotHotkey = i;
                    mods::spinbotHotkeyName = GetKeyName(i);
                }
                else if (mods::settingHotkeyFor == "Self-TimeDilation") {
                    mods::SelfTimeHotkey = i;
                    mods::SelfTimekeyName = GetKeyName(i);
                }
                mods::settingHotkeyFor = "";
                break;
            }
        }
    }

    if (!LoadingScreen::bShowing) {
        DrawTransition(ImGui::GetBackgroundDrawList(), ImGui::GetForegroundDrawList());

        DrawAllWidgets();

        if (ShowMenu) {
            menu();
        }
    }
    LoadingScreen::Draw();
    ImGui::EndFrame();

    DirectX12Interface::_FrameContext& CurrentFrameContext = DirectX12Interface::FrameContext[pSwapChain->GetCurrentBackBufferIndex()];
    if (CurrentFrameContext.CommandAllocator && CurrentFrameContext.Resource) {
        hr = CurrentFrameContext.CommandAllocator->Reset();
        if (FAILED(hr)) {
            DBG_LOG("ca reset fail 0x%X", hr);
            return oPresent(pSwapChain, SyncInterval, Flags);
        }

        D3D12_RESOURCE_BARRIER Barrier = {};
        Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        Barrier.Transition.pResource = CurrentFrameContext.Resource;
        Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

        hr = DirectX12Interface::CommandList->Reset(CurrentFrameContext.CommandAllocator, nullptr);
        if (FAILED(hr)) {
            DBG_LOG("cl reset fail 0x%X", hr);
            return oPresent(pSwapChain, SyncInterval, Flags);
        }

        DirectX12Interface::CommandList->ResourceBarrier(1, &Barrier);
        DirectX12Interface::CommandList->OMSetRenderTargets(1, &CurrentFrameContext.DescriptorHandle, FALSE, nullptr);
        DirectX12Interface::CommandList->SetDescriptorHeaps(1, &DirectX12Interface::DescriptorHeapImGuiRender);

        ImGui::Render();
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), DirectX12Interface::CommandList);
        Barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        Barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        DirectX12Interface::CommandList->ResourceBarrier(1, &Barrier);
        hr = DirectX12Interface::CommandList->Close();
        if (FAILED(hr)) {
            DBG_LOG("cl close fail 0x%X", hr);
            return oPresent(pSwapChain, SyncInterval, Flags);
        }

        DirectX12Interface::CommandQueue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(&DirectX12Interface::CommandList));
    }
    else {
        DBG_LOG("bad fc");
    }

    return oPresent(pSwapChain, SyncInterval, Flags);
}

void hkExecuteCommandLists(ID3D12CommandQueue* queue, UINT NumCommandLists, ID3D12CommandList* ppCommandLists) {
    if (g_Unloading || !queue) {
        if (queue) oExecuteCommandLists(queue, NumCommandLists, ppCommandLists);
        return;
    }
    if (!DirectX12Interface::CommandQueue) DirectX12Interface::CommandQueue = queue;
    oExecuteCommandLists(queue, NumCommandLists, ppCommandLists);
}

void APIENTRY hkDrawInstanced(ID3D12GraphicsCommandList* dCommandList, UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation) {
    oDrawInstanced(dCommandList, VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void APIENTRY hkDrawIndexedInstanced(ID3D12GraphicsCommandList* dCommandList, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation) {
    oDrawIndexedInstanced(dCommandList, IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

namespace ACWatchdog {
    static volatile bool g_running = false;
    static HANDLE        g_thread  = nullptr;

    static const wchar_t* AC_THREAD_NAMES[] = {
        L"AcSDKThread",
        L"RTHeartBeat",
    };
    static constexpr int AC_THREAD_NAME_COUNT = _countof(AC_THREAD_NAMES);

    static bool IsACThreadName(const std::wstring& name) {
        for (int i = 0; i < AC_THREAD_NAME_COUNT; i++) {
            if (name.find(AC_THREAD_NAMES[i]) != std::wstring::npos)
                return true;
        }
        return false;
    }

    DWORD WINAPI WatchdogProc(LPVOID) {
        Stealth::HideThread();
        DWORD myPid = GetCurrentProcessId();

        while (g_running) {
            if (!mods::bACWatchdog) {
                Sleep(500);
                continue;
            }

            HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
            if (snap != INVALID_HANDLE_VALUE) {
                THREADENTRY32 te{};
                te.dwSize = sizeof(te);

                if (Thread32First(snap, &te)) {
                    do {
                        if (te.th32OwnerProcessID != myPid) continue;

                        HANDLE hThread = OpenThread(
                            THREAD_QUERY_LIMITED_INFORMATION | THREAD_TERMINATE,
                            FALSE, te.th32ThreadID);
                        if (!hThread) continue;

                        PWSTR desc = nullptr;
                        HRESULT hr = GetThreadDescription(hThread, &desc);
                        if (SUCCEEDED(hr) && desc && wcslen(desc) > 0) {
                            std::wstring threadName(desc);
                            LocalFree(desc);

                            if (IsACThreadName(threadName)) {
                                TerminateThread(hThread, 0);
                                DBG_LOG("ACWatchdog: killed thread %ls (TID %u)",
                                    threadName.c_str(), te.th32ThreadID);
                            }
                        } else if (desc) {
                            LocalFree(desc);
                        }

                        CloseHandle(hThread);
                    } while (Thread32Next(snap, &te));
                }
                CloseHandle(snap);
            }

            int interval = mods::acWatchdogIntervalMs;
            for (int waited = 0; waited < interval && g_running; waited += 200) {
                Sleep(200);
            }
        }
        return 0;
    }

    void Start() {
        g_running = true;
        g_thread = Stealth::CreateHiddenThread(WatchdogProc, nullptr);
        if (g_thread) {
            SetThreadPriority(g_thread, THREAD_PRIORITY_LOWEST);
        }
    }

    void Stop() {
        g_running = false;
        if (g_thread) {
            WaitForSingleObject(g_thread, 3000);
            CloseHandle(g_thread);
            g_thread = nullptr;
        }
    }
}

DWORD WINAPI MainThread(LPVOID lpParameter) {
    bool WindowFocus = false;
    while (!WindowFocus) {
        DWORD ForegroundWindowProcessID;
        HWND fgWindow = GetForegroundWindow();
        if (!fgWindow) continue;
        GetWindowThreadProcessId(fgWindow, &ForegroundWindowProcessID);
        if (GetCurrentProcessId() == ForegroundWindowProcessID) {
            Process::ID = GetCurrentProcessId();
            Process::Handle = GetCurrentProcess();
            Process::Hwnd = fgWindow;

            RECT TempRect;
            if (GetWindowRect(Process::Hwnd, &TempRect)) {
                Process::WindowWidth = TempRect.right - TempRect.left;
                Process::WindowHeight = TempRect.bottom - TempRect.top;
            }

            char TempTitle[MAX_PATH];
            if (GetWindowText(Process::Hwnd, TempTitle, sizeof(TempTitle))) {
                Process::Title = TempTitle;
            }

            char TempClassName[MAX_PATH];
            if (GetClassName(Process::Hwnd, TempClassName, sizeof(TempClassName))) {
                Process::ClassName = TempClassName;
            }

            char TempPath[MAX_PATH];
            if (GetModuleFileNameEx(Process::Handle, NULL, TempPath, sizeof(TempPath))) {
                Process::Path = TempPath;
            }

            WindowFocus = true;
        }
        Sleep(100);
    }

    bool InitHook = false;
#ifdef TRIGGER_ONLY_EXTERNAL
    mods::TriggerBot = true;
#endif
    while (!InitHook) {
        if (DirectX12::Init()) {
            if (!CreateHook(54, (void**)&oExecuteCommandLists, hkExecuteCommandLists) ||
                !CreateHook(140, (void**)&oPresent, hkPresent)) {
                DBG_LOG("hook fail");
            }
            else {
                InitHook = true;
            }
        }
        Sleep(100);
    }
    return 0;
}

void DisableAll() {
    static volatile LONG s_disabledOnce = 0;
    if (InterlockedCompareExchange(&s_disabledOnce, 1, 0) != 0) return;
    ACWatchdog::Stop();
    AntiScreenshot::Shutdown();
    MH_DisableHook(MH_ALL_HOOKS);
    if (MethodsTable) {
        free(MethodsTable);
        MethodsTable = NULL;
    }
    if (DirectX12Interface::FrameContext) {
        ID3D12CommandAllocator* releasedAlloc = nullptr;
        for (size_t i = 0; i < DirectX12Interface::BuffersCounts; i++) {
            auto* alloc = DirectX12Interface::FrameContext[i].CommandAllocator;
            if (alloc && alloc != releasedAlloc) {
                alloc->Release();
                releasedAlloc = alloc;
            }
            if (DirectX12Interface::FrameContext[i].Resource) {
                DirectX12Interface::FrameContext[i].Resource->Release();
            }
        }
        delete[] DirectX12Interface::FrameContext;
        DirectX12Interface::FrameContext = nullptr;
    }
    if (DirectX12Interface::DescriptorHeapBackBuffers) {
        DirectX12Interface::DescriptorHeapBackBuffers->Release();
        DirectX12Interface::DescriptorHeapBackBuffers = nullptr;
    }
    HeroIcons::Cleanup();
    LoadingScreen::Cleanup();
    MatchStatus::Cleanup();
    if (DirectX12Interface::DescriptorHeapImGuiRender) {
        DirectX12Interface::DescriptorHeapImGuiRender->Release();
        DirectX12Interface::DescriptorHeapImGuiRender = nullptr;
    }
    if (DirectX12Interface::CommandList) {
        DirectX12Interface::CommandList->Release();
        DirectX12Interface::CommandList = nullptr;
    }
    if (DirectX12Interface::Fence) {
        DirectX12Interface::Fence->Release();
        DirectX12Interface::Fence = nullptr;
    }
    if (DirectX12Interface::Device) {
        DirectX12Interface::Device->Release();
        DirectX12Interface::Device = nullptr;
    }
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

DWORD WINAPI UnloadThread(LPVOID) {
    Sleep(200);
    if (Process::WndProc)
        SetWindowLongPtr(Process::Hwnd, GWLP_WNDPROC, (LONG_PTR)Process::WndProc);
    Sleep(100);
    DisableAll();
    FreeLibraryAndExitThread(Process::Module, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        Process::Module = hModule;

        {
            HANDLE hMain = CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
            if (hMain) CloseHandle(hMain);
        }
        break;
    case DLL_PROCESS_DETACH:
        DisableAll();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    return TRUE;
}
