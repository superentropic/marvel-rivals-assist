#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "../Source/TriggerControlShared.h"

namespace s10 {
constexpr std::uintptr_t kGWorld = 0x0FAA4A18;
constexpr std::uintptr_t kWorldPersistentLevel = 0x38;
constexpr std::uintptr_t kWorldGameInstance = 0x290;
constexpr std::uintptr_t kGameInstanceLocalPlayers = 0x40;
constexpr std::uintptr_t kUPlayerController = 0x38;
constexpr std::uintptr_t kControllerAcknowledgedPawn = 0x7C8;
constexpr std::uintptr_t kControllerCameraManager = 0x7E0;
constexpr std::uintptr_t kPawnPlayerState = 0x728;
constexpr std::uintptr_t kPlayerStateOriginalTeam = 0x838;
constexpr std::uintptr_t kLevelActors = 0xD8;
}

struct TArrayRemote {
    std::uintptr_t data;
    std::int32_t count;
    std::int32_t capacity;
};

static DWORD FindProcessId(const wchar_t* name) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W entry{ sizeof(entry) };
    DWORD result = 0;
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, name) == 0) {
                result = entry.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return result;
}

static std::uintptr_t FindMainModuleBase(HANDLE process, DWORD processId, DWORD& failureError) {
    failureError = ERROR_SUCCESS;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
    if (snapshot != INVALID_HANDLE_VALUE) {
        MODULEENTRY32W entry{ sizeof(entry) };
        if (Module32FirstW(snapshot, &entry)) {
            CloseHandle(snapshot);
            // Toolhelp returns the process main executable first. Its name can
            // differ from the process name between Steam builds.
            return reinterpret_cast<std::uintptr_t>(entry.modBaseAddr);
        }
        failureError = GetLastError();
        CloseHandle(snapshot);
    } else {
        failureError = GetLastError();
    }

    HMODULE module = nullptr;
    DWORD required = 0;
    if (EnumProcessModulesEx(process, &module, sizeof(module), &required, LIST_MODULES_ALL) && module) {
        return reinterpret_cast<std::uintptr_t>(module);
    }

    failureError = GetLastError();
    return 0;
}

template <typename T>
static bool Read(HANDLE process, std::uintptr_t address, T& value) {
    SIZE_T bytesRead = 0;
    return address != 0 && ReadProcessMemory(process, reinterpret_cast<LPCVOID>(address), &value, sizeof(value), &bytesRead) && bytesRead == sizeof(value);
}

static std::ofstream gLog("MarvelExternalTrigger.log", std::ios::trunc);

static void Print(const std::string& message) {
    std::cout << message << '\n';
    if (gLog) gLog << message << '\n';
}

static int Finish(int status) {
    Print("\nPress Enter to close this window.");
    std::cin.get();
    return status;
}

static void PrintAddress(const char* label, std::uintptr_t address) {
    std::ostringstream stream;
    stream << std::left << std::setw(22) << label << " 0x" << std::hex << address;
    Print(stream.str());
}

// Sends a real desktop mouse click from the external reader process.  Keep the
// input emission here, next to the reader's request path, so target detection
// can call the same function without going through the injected DLL.
static bool SendLeftClick() {
    INPUT inputs[2]{};
    inputs[0].type = INPUT_MOUSE;
    inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    inputs[1].type = INPUT_MOUSE;
    inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    return SendInput(2, inputs, sizeof(INPUT)) == 2;
}

static bool RequestLeftClick() {
    HANDLE mapping = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, TriggerControlShared::MappingName);
    if (!mapping) return false;

    auto* state = static_cast<TriggerControlShared::State*>(MapViewOfFile(
        mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(TriggerControlShared::State)));
    if (!state) {
        CloseHandle(mapping);
        return false;
    }

    const bool valid = InterlockedCompareExchange(&state->magic, 0, 0) == TriggerControlShared::Magic;
    const bool requested = valid && TriggerControlShared::RequestLeftClick(*state);
    UnmapViewOfFile(state);
    CloseHandle(mapping);
    return requested;
}

int wmain(int argc, wchar_t** argv) {
    constexpr const wchar_t* kProcessName = L"Marvel-Win64-Shipping.exe";
    Print("Marvel Rivals S10 external reader");
    Print("Start the game, enter Practice Range, then run this program.");
    Print("This reader validates memory and can request one in-process left click with --request-click.");

    DWORD processId = FindProcessId(kProcessName);
    if (!processId) {
        Print("\nFAILED: Marvel-Win64-Shipping.exe was not found.");
        Print("Open Marvel Rivals first, then run this program again.");
        return Finish(1);
    }

    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (!process) {
        Print("\nFAILED: OpenProcess failed. Windows error: " + std::to_string(GetLastError()));
        Print("Run this program at the same privilege level as the game.");
        return Finish(1);
    }

    DWORD moduleError = ERROR_SUCCESS;
    const std::uintptr_t imageBase = FindMainModuleBase(process, processId, moduleError);
    if (!imageBase) {
        Print("\nFAILED: Could not resolve the game module base. Windows error: " + std::to_string(moduleError));
        Print("Run this program as administrator only if Marvel Rivals is also running as administrator.");
        CloseHandle(process);
        return Finish(1);
    }

    std::uintptr_t world = 0;
    if (!Read(process, imageBase + s10::kGWorld, world) || !world) {
        Print("\nFAILED: GWorld read failed at the S10 offset 0xFAA4A18.");
        Print("Check that the game build and Dumper-7 output are both S10.");
        CloseHandle(process);
        return Finish(1);
    }

    std::uintptr_t gameInstance = 0;
    std::uintptr_t persistentLevel = 0;
    TArrayRemote localPlayers{};
    if (!Read(process, world + s10::kWorldGameInstance, gameInstance) ||
        !Read(process, world + s10::kWorldPersistentLevel, persistentLevel) ||
        !Read(process, gameInstance + s10::kGameInstanceLocalPlayers, localPlayers) ||
        !localPlayers.data || localPlayers.count < 1 || localPlayers.count > 4) {
        Print("\nFAILED: World to LocalPlayers chain read failed.");
        Print("Wait until you are fully in Practice Range, then run it again.");
        CloseHandle(process);
        return Finish(1);
    }

    std::uintptr_t localPlayer = 0;
    std::uintptr_t controller = 0;
    std::uintptr_t pawn = 0;
    std::uintptr_t cameraManager = 0;
    std::uintptr_t playerState = 0;
    std::int32_t team = -1;
    TArrayRemote actors{};

    const bool chainOk =
        Read(process, localPlayers.data, localPlayer) &&
        Read(process, localPlayer + s10::kUPlayerController, controller) &&
        Read(process, controller + s10::kControllerAcknowledgedPawn, pawn) &&
        Read(process, controller + s10::kControllerCameraManager, cameraManager) &&
        Read(process, pawn + s10::kPawnPlayerState, playerState) &&
        Read(process, playerState + s10::kPlayerStateOriginalTeam, team) &&
        Read(process, persistentLevel + s10::kLevelActors, actors);

    if (!chainOk || !controller || !pawn || !cameraManager || !playerState || !actors.data || actors.count < 0 || actors.count > 50000) {
        Print("\nFAILED: Local-player or actor-list read failed.");
        Print("The S10 reader will not continue to any trigger logic.");
        CloseHandle(process);
        return Finish(1);
    }

    Print("\nSUCCESS: S10 external reader attached to PID " + std::to_string(processId) + ".");
    PrintAddress("image base", imageBase);
    PrintAddress("GWorld", world);
    PrintAddress("controller", controller);
    PrintAddress("local pawn", pawn);
    PrintAddress("camera manager", cameraManager);
    Print("local team             " + std::to_string(team));
    Print("level actors           " + std::to_string(actors.count));
    if (argc > 1 && _wcsicmp(argv[1], L"--request-click") == 0) {
        if (SendLeftClick()) {
            Print("\nLeft-click sent by the external reader.");
        } else {
            Print("\nFAILED: external SendInput did not accept the left-click.");
            CloseHandle(process);
            return Finish(1);
        }
    } else {
        Print("\nRead-only validation completed. No game memory was written and no mouse input was sent.");
    }

    CloseHandle(process);
    return Finish(0);
}
