#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <vector>
#include <Psapi.h>
#include <shellapi.h>
#include <conio.h>
#include <dwmapi.h>
#include <thread>
#include "stealth.h"
#include "security.h"
#include "keyauth.h"
#include <shlobj.h>

#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "dwmapi.lib")

std::string ka_name    = "";
std::string ka_ownerid = "";
std::string ka_version = "1.1";
std::string ka_url     = "https://keyauth.win/api/1.3/";
std::string ka_path    = "";

KeyAuth::api KeyAuthApp(ka_name, ka_ownerid, ka_version, ka_url, ka_path);
static bool g_authenticated = false;

namespace config {
    inline std::wstring PROCESS_NAME()   { return XSW(L"Marvel-Win64-Shipping.exe").str(); }
    inline std::wstring DLL_NAME()       { return XSW(L"msaud_drv.dll").str(); }
    inline std::wstring LOG_FILE()       { return XSW(L"svcdiag.log").str(); }
    inline std::wstring WINDOW_TITLE()   { return XSW(L"Marvel Abyss").str(); }
    constexpr int TIMEOUT_SECONDS        = 120;

    inline std::wstring AC_THREAD_NAME(int i) {
        switch (i) {
        case 0: return XSW(L"AcSDKThread").str();
        case 1: return XSW(L"RTHeartBeat").str();
        default: return L"";
        }
    }
    constexpr int AC_THREAD_COUNT = 2;
}

namespace debug {
    void init()  {}
    void close() {}
    void info(const std::wstring&)  {}
    void ok(const std::wstring&)    {}
    void warn(const std::wstring&)  {}
    void error(const std::wstring&) {}
}

namespace con {
    static HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    enum Color : WORD { DARK=0x08, WHITE=0x0F, GREEN=0x0A, RED=0x0C, YELLOW=0x0E, GRAY=0x07 };
    inline void set(WORD c) { SetConsoleTextAttribute(hOut, c); }
    inline void reset() { set(GRAY); }
    inline void print_center(WORD c, const char* text, int width = 64) {
        int pad = (width - (int)strlen(text)) / 2;
        if (pad < 0) pad = 0;
        set(c);
        for (int i = 0; i < pad; i++) putchar(' ');
        printf("%s\n", text);
        reset();
    }
    inline void line(WORD c, int width = 60) {
        set(c); printf("  ");
        for (int i = 0; i < width; i++) putchar('-');
        printf("\n"); reset();
    }
    inline void clear() {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hOut, &csbi);
        DWORD cells = csbi.dwSize.X * csbi.dwSize.Y, written;
        COORD home = {0,0};
        FillConsoleOutputCharacterA(hOut, ' ', cells, home, &written);
        FillConsoleOutputAttribute(hOut, csbi.wAttributes, cells, home, &written);
        SetConsoleCursorPosition(hOut, home);
    }
}

namespace color {
    void red()    { con::set(con::RED); }
    void green()  { con::set(con::GREEN); }
    void yellow() { con::set(con::YELLOW); }
    void white()  { con::set(con::WHITE); }
    void gray()   { con::set(con::DARK); }
    void cyan()   { con::set(con::WHITE); }
}

void configure_terminal() {
    HWND hConsole = GetConsoleWindow();
    if (!hConsole) return;
    SetConsoleOutputCP(CP_UTF8);
    RECT r; GetWindowRect(hConsole, &r);
    MoveWindow(hConsole, r.left, r.top, 740, 550, TRUE);
    CONSOLE_FONT_INFOEX cfi = {};
    cfi.cbSize = sizeof(cfi); cfi.dwFontSize.Y = 16;
    cfi.FontFamily = FF_DONTCARE; cfi.FontWeight = FW_NORMAL;
    wcscpy_s(cfi.FaceName, L"Cascadia Code");
    SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
    SetWindowPos(hConsole, HWND_TOPMOST, 0, 0, 0, 0, SWP_DRAWFRAME | SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    BOOL darkMode = TRUE;
    DwmSetWindowAttribute(hConsole, 20, &darkMode, sizeof(darkMode));
    SetWindowLongA(hConsole, GWL_EXSTYLE, GetWindowLongA(hConsole, GWL_EXSTYLE) | WS_EX_LAYERED);
    SetLayeredWindowAttributes(hConsole, 0, 230, LWA_ALPHA);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hOut, &csbi);
    COORD bufSize; bufSize.X = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    bufSize.Y = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    SetConsoleScreenBufferSize(hOut, bufSize);
}

void print_banner() {
    con::clear();
    printf("\n\n");
    con::print_center(con::WHITE, "Marvel Abyss");
    con::print_center(con::DARK, "~ Loader ~");
    printf("\n");
    con::line(con::DARK);
    printf("\n");
}

void log_step(const char* msg) {
    con::set(con::WHITE); printf("    %s", msg); con::reset();
}
void log_done() {
    con::set(con::DARK); printf("  Done\n"); con::reset();
}
void log_fail(const char* msg) {
    con::set(con::RED); printf("  Failed");
    if (msg && msg[0]) printf(" (%s)", msg);
    printf("\n"); con::reset();
}

void log_info(const std::wstring& msg) {
    con::set(con::DARK); wprintf(L"    %s\n", msg.c_str()); con::reset();
    debug::info(msg);
}

void log_ok(const std::wstring& msg) {
    con::set(con::WHITE); wprintf(L"    %s\n", msg.c_str()); con::reset();
    debug::ok(msg);
}

void log_err(const std::wstring& msg) {
    con::set(con::RED); wprintf(L"    %s\n", msg.c_str()); con::reset();
    debug::error(msg);
}

void log_warn(const std::wstring& msg) {
    con::set(con::DARK); wprintf(L"    %s\n", msg.c_str()); con::reset();
    debug::warn(msg);
}

void log_debug(const std::wstring& msg) {
    con::set(con::DARK); wprintf(L"    %s\n", msg.c_str()); con::reset();
    debug::info(L"[DEBUG] " + msg);
}

DWORD get_process_id(const wchar_t* process_name) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        debug::error(L"CreateToolhelp32Snapshot failed. Error: " + std::to_wstring(GetLastError()));
        return 0;
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, process_name) == 0) {
                CloseHandle(snapshot);
                return entry.th32ProcessID;
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return 0;
}

bool is_process_running(const wchar_t* process_name) {
    return get_process_id(process_name) != 0;
}

bool is_elevated() {
    BOOL elevated = FALSE;
    HANDLE token = nullptr;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        TOKEN_ELEVATION elev{};
        DWORD size = sizeof(elev);
        if (GetTokenInformation(token, TokenElevation, &elev, sizeof(elev), &size)) {
            elevated = elev.TokenIsElevated;
        }
        CloseHandle(token);
    }
    return elevated != FALSE;
}

struct ACKillResult {
    int threads_found = 0;
    int threads_killed = 0;
    int threads_failed = 0;
    std::vector<std::wstring> killed_names;
    std::vector<std::wstring> failed_names;
};

bool is_ac_thread_name(const std::wstring& name) {
    for (int i = 0; i < config::AC_THREAD_COUNT; i++) {
        if (name.find(config::AC_THREAD_NAME(i)) != std::wstring::npos) {
            return true;
        }
    }
    return false;
}

ACKillResult kill_ac_threads(DWORD target_pid) {
    ACKillResult result;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        debug::error(L"CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD) failed. Error: " + std::to_wstring(GetLastError()));
        return result;
    }

    THREADENTRY32 te{};
    te.dwSize = sizeof(te);

    if (Thread32First(snapshot, &te)) {
        do {
            if (te.th32OwnerProcessID != target_pid) continue;

            HANDLE thread = OpenThread(THREAD_QUERY_LIMITED_INFORMATION | THREAD_TERMINATE, FALSE, te.th32ThreadID);
            if (!thread) continue;

            PWSTR desc = nullptr;
            HRESULT hr = GetThreadDescription(thread, &desc);
            if (SUCCEEDED(hr) && desc && wcslen(desc) > 0) {
                std::wstring thread_name(desc);
                LocalFree(desc);

                if (is_ac_thread_name(thread_name)) {
                    result.threads_found++;
                    debug::info(L"Found AC thread: \"" + thread_name + L"\" (TID: " + std::to_wstring(te.th32ThreadID) + L")");

                    if (TerminateThread(thread, 0)) {
                        result.threads_killed++;
                        result.killed_names.push_back(thread_name + L" (TID: " + std::to_wstring(te.th32ThreadID) + L")");
                        debug::ok(L"Terminated: \"" + thread_name + L"\"");
                    }
                    else {
                        result.threads_failed++;
                        result.failed_names.push_back(thread_name + L" (TID: " + std::to_wstring(te.th32ThreadID) + L")");
                        debug::error(L"Failed to terminate: \"" + thread_name + L"\" Error: " + std::to_wstring(GetLastError()));
                    }
                }
            }
            else if (desc) {
                LocalFree(desc);
            }

            CloseHandle(thread);
        } while (Thread32Next(snapshot, &te));
    }

    CloseHandle(snapshot);
    return result;
}

std::wstring get_dll_path() {
    wchar_t exe_path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exe_path, MAX_PATH);

    std::filesystem::path p(exe_path);
    p = p.parent_path() / config::DLL_NAME();
    return p.wstring();
}

bool validate_dll(const std::wstring& dll_path) {
    if (!std::filesystem::exists(dll_path)) {
        log_err(L"DLL not found: " + dll_path);
        return false;
    }

    auto file_size = std::filesystem::file_size(dll_path);
    log_debug(L"DLL file size: " + std::to_wstring(file_size) + L" bytes");
    if (file_size < 1024) {
        log_err(L"DLL file is suspiciously small (" + std::to_wstring(file_size) + L" bytes). Possibly corrupt.");
        return false;
    }

    std::ifstream f(dll_path, std::ios::binary);
    if (!f.is_open()) {
        log_err(L"Cannot open DLL file for reading.");
        return false;
    }
    char magic[2]{};
    f.read(magic, 2);
    f.close();
    if (magic[0] != 'M' || magic[1] != 'Z') {
        log_err(L"DLL file is not a valid PE executable (missing MZ header).");
        return false;
    }

    std::ifstream f2(dll_path, std::ios::binary);
    if (f2.is_open()) {
        IMAGE_DOS_HEADER dos{};
        f2.read(reinterpret_cast<char*>(&dos), sizeof(dos));
        if (dos.e_lfanew > 0 && dos.e_lfanew < 1024) {
            f2.seekg(dos.e_lfanew, std::ios::beg);
            DWORD pe_sig = 0;
            f2.read(reinterpret_cast<char*>(&pe_sig), sizeof(pe_sig));
            if (pe_sig == IMAGE_NT_SIGNATURE) {
                IMAGE_FILE_HEADER file_header{};
                f2.read(reinterpret_cast<char*>(&file_header), sizeof(file_header));
                if (file_header.Machine == IMAGE_FILE_MACHINE_AMD64) {
                    log_debug(L"DLL architecture: x64 (correct)");
                }
                else if (file_header.Machine == IMAGE_FILE_MACHINE_I386) {
                    log_err(L"DLL is 32-bit (x86). Marvel Rivals requires a 64-bit (x64) DLL.");
                    f2.close();
                    return false;
                }
                else {
                    log_warn(L"DLL architecture unknown (Machine: " + std::to_wstring(file_header.Machine) + L")");
                }
            }
        }
        f2.close();
    }

    log_ok(L"DLL validation passed.");
    return true;
}

void log_system_info() {
    OSVERSIONINFOEXW os{};
    os.dwOSVersionInfoSize = sizeof(os);

    using RtlGetVersionPtr = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);
    auto ntdll = GetModuleHandleW(L"ntdll.dll");
    if (ntdll) {
        auto rtl_get_ver = reinterpret_cast<RtlGetVersionPtr>(GetProcAddress(ntdll, "RtlGetVersion"));
        if (rtl_get_ver) {
            rtl_get_ver(reinterpret_cast<PRTL_OSVERSIONINFOW>(&os));
            debug::info(L"Windows version: " + std::to_wstring(os.dwMajorVersion) + L"."
                + std::to_wstring(os.dwMinorVersion) + L" (Build "
                + std::to_wstring(os.dwBuildNumber) + L")");
        }
    }

    SYSTEM_INFO si{};
    GetNativeSystemInfo(&si);
    debug::info(L"Processor count: " + std::to_wstring(si.dwNumberOfProcessors));
    debug::info(L"Processor arch: " + std::to_wstring(si.wProcessorArchitecture));

    MEMORYSTATUSEX mem{};
    mem.dwLength = sizeof(mem);
    GlobalMemoryStatusEx(&mem);
    debug::info(L"Total RAM: " + std::to_wstring(mem.ullTotalPhys / (1024 * 1024)) + L" MB");
}

enum class InjectResult {
    SUCCESS,
    OPEN_PROCESS_FAILED,
    ALLOC_FAILED,
    WRITE_FAILED,
    KERNEL32_NOT_FOUND,
    LOADLIBRARYW_NOT_FOUND,
    THREAD_FAILED,
    THREAD_TIMEOUT,
    LOADLIBRARY_FAILED,
    VERIFY_FAILED,
};

struct InjectContext {
    InjectResult result = InjectResult::SUCCESS;
    DWORD last_error = 0;
    std::wstring detail;
};

InjectContext inject_dll(DWORD pid, const std::wstring& dll_path) {
    InjectContext ctx;

    debug::info(L"--- Operation Start ---");
    debug::info(L"Target PID: " + std::to_wstring(pid));
    debug::info(L"DLL path: " + dll_path);
    DWORD access = PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE
                 | PROCESS_VM_READ | PROCESS_QUERY_INFORMATION;
    HANDLE process = OpenProcess(access, FALSE, pid);
    if (!process) {
        ctx.last_error = GetLastError();
        ctx.result = InjectResult::OPEN_PROCESS_FAILED;
        ctx.detail = L"OpenProcess failed. Error code: " + std::to_wstring(ctx.last_error);
        if (ctx.last_error == 5)
            ctx.detail += L" (Access Denied - not running as admin?)";
        else if (ctx.last_error == 87)
            ctx.detail += L" (Invalid parameter - process may have exited)";
        debug::error(ctx.detail);
        return ctx;
    }
    debug::ok(L"Process opened. Handle: 0x" + (std::wstringstream() << std::hex << (uintptr_t)process).str());

    debug::info(L"Step 2: Allocating remote memory...");
    size_t path_size = (dll_path.size() + 1) * sizeof(wchar_t);
    debug::info(L"Path buffer size: " + std::to_wstring(path_size) + L" bytes");

    void* remote_buf = VirtualAllocEx(process, nullptr, path_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote_buf) {
        ctx.last_error = GetLastError();
        ctx.result = InjectResult::ALLOC_FAILED;
        ctx.detail = L"VirtualAllocEx failed. Error code: " + std::to_wstring(ctx.last_error);
        debug::error(ctx.detail);
        CloseHandle(process);
        return ctx;
    }
    debug::ok(L"Memory allocated at remote address: 0x" + (std::wstringstream() << std::hex << (uintptr_t)remote_buf).str());

    debug::info(L"Step 3: Writing DLL path...");
    SIZE_T bytes_written = 0;
    if (!WriteProcessMemory(process, remote_buf, dll_path.c_str(), path_size, &bytes_written)) {
        ctx.last_error = GetLastError();
        ctx.result = InjectResult::WRITE_FAILED;
        ctx.detail = L"WriteProcessMemory failed. Error code: " + std::to_wstring(ctx.last_error)
                   + L" Bytes written: " + std::to_wstring(bytes_written);
        debug::error(ctx.detail);
        VirtualFreeEx(process, remote_buf, 0, MEM_RELEASE);
        CloseHandle(process);
        return ctx;
    }
    debug::ok(L"Wrote " + std::to_wstring(bytes_written) + L" bytes to remote process.");

    debug::info(L"Step 4: Resolving LoadLibraryW...");
    HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!kernel32) {
        ctx.last_error = GetLastError();
        ctx.result = InjectResult::KERNEL32_NOT_FOUND;
        ctx.detail = L"GetModuleHandle(kernel32.dll) failed. Error: " + std::to_wstring(ctx.last_error);
        debug::error(ctx.detail);
        VirtualFreeEx(process, remote_buf, 0, MEM_RELEASE);
        CloseHandle(process);
        return ctx;
    }

    auto load_library_addr = reinterpret_cast<LPTHREAD_START_ROUTINE>(
        GetProcAddress(kernel32, "LoadLibraryW"));
    if (!load_library_addr) {
        ctx.last_error = GetLastError();
        ctx.result = InjectResult::LOADLIBRARYW_NOT_FOUND;
        ctx.detail = L"GetProcAddress(LoadLibraryW) failed. Error: " + std::to_wstring(ctx.last_error);
        debug::error(ctx.detail);
        VirtualFreeEx(process, remote_buf, 0, MEM_RELEASE);
        CloseHandle(process);
        return ctx;
    }
    debug::ok(L"LoadLibraryW at: 0x" + (std::wstringstream() << std::hex << (uintptr_t)load_library_addr).str());

    debug::info(L"Step 5: Creating remote thread...");
    DWORD thread_id = 0;
    HANDLE thread = CreateRemoteThread(process, nullptr, 0, load_library_addr, remote_buf, 0, &thread_id);
    if (!thread) {
        ctx.last_error = GetLastError();
        ctx.result = InjectResult::THREAD_FAILED;
        ctx.detail = L"CreateRemoteThread failed. Error code: " + std::to_wstring(ctx.last_error);
        if (ctx.last_error == 5)
            ctx.detail += L" (Access Denied - protection may be blocking)";
        else if (ctx.last_error == 0x00000057)
            ctx.detail += L" (Invalid parameter)";
        debug::error(ctx.detail);
        VirtualFreeEx(process, remote_buf, 0, MEM_RELEASE);
        CloseHandle(process);
        return ctx;
    }
    debug::ok(L"Remote thread created. TID: " + std::to_wstring(thread_id));

    debug::info(L"Step 6: Waiting for the thread to finish (10s timeout)...");
    DWORD wait_result = WaitForSingleObject(thread, 10000);
    if (wait_result == WAIT_TIMEOUT) {
        ctx.result = InjectResult::THREAD_TIMEOUT;
        ctx.detail = L"Remote thread did not complete within 10 seconds. "
                     L"The target may have frozen or protection blocked execution.";
        debug::error(ctx.detail);
        TerminateThread(thread, 0);
        CloseHandle(thread);
        VirtualFreeEx(process, remote_buf, 0, MEM_RELEASE);
        CloseHandle(process);
        return ctx;
    }
    else if (wait_result == WAIT_FAILED) {
        ctx.last_error = GetLastError();
        ctx.detail = L"WaitForSingleObject failed. Error: " + std::to_wstring(ctx.last_error);
        debug::error(ctx.detail);
    }
    debug::ok(L"Remote thread completed.");

    debug::info(L"Step 7: Checking return value...");
    DWORD exit_code = 0;
    GetExitCodeThread(thread, &exit_code);
    debug::info(L"Thread exit code (HMODULE low 32): 0x" + (std::wstringstream() << std::hex << exit_code).str());

    CloseHandle(thread);
    VirtualFreeEx(process, remote_buf, 0, MEM_RELEASE);

    if (exit_code == 0) {
        ctx.result = InjectResult::LOADLIBRARY_FAILED;
        ctx.detail = L"LoadLibraryW returned NULL. The DLL failed to load inside the game process.\n"
                     L"Possible causes:\n"
                     L"  - DLL has missing dependencies (check with Dependency Walker)\n"
                     L"  - DLL path contains special characters\n"
                     L"  - DLL's DllMain returned FALSE\n"
                     L"  - Protection module intercepted the call";
        debug::error(ctx.detail);
        CloseHandle(process);
        return ctx;
    }

    debug::info(L"Step 8: Verifying DLL load...");
    Sleep(500);
    std::wstring dll_name_only = std::filesystem::path(dll_path).filename().wstring();
    HMODULE modules[1024]{};
    DWORD cb_needed = 0;
    bool verified = false;
    if (EnumProcessModules(process, modules, sizeof(modules), &cb_needed)) {
        int mod_count = cb_needed / sizeof(HMODULE);
        debug::info(L"Target has " + std::to_wstring(mod_count) + L" loaded modules.");
        for (int i = 0; i < mod_count; i++) {
            wchar_t mod_name[MAX_PATH]{};
            if (GetModuleFileNameExW(process, modules[i], mod_name, MAX_PATH)) {
                std::wstring name = std::filesystem::path(mod_name).filename().wstring();
                if (_wcsicmp(name.c_str(), dll_name_only.c_str()) == 0) {
                    verified = true;
                    debug::ok(L"DLL verified in target process module list: " + std::wstring(mod_name));
                    break;
                }
            }
        }
    }
    else {
        debug::warn(L"Could not enumerate process modules for verification. Error: " + std::to_wstring(GetLastError()));
    }

    CloseHandle(process);

    if (!verified) {
        debug::warn(L"Could not verify DLL in module list (may still be loaded - exit code was non-zero).");
    }

    ctx.result = InjectResult::SUCCESS;
    ctx.detail = L"Module loaded successfully.";
    debug::ok(ctx.detail);
    debug::info(L"--- Operation Complete ---");
    return ctx;
}

namespace ac_watchdog {
    static volatile bool g_running = false;
    static HANDLE        g_thread  = nullptr;
    static DWORD         g_target_pid = 0;
    constexpr int        POLL_INTERVAL_MS = 3000;

    DWORD WINAPI WatchdogProc(LPVOID) {
        debug::info(L"[Watchdog] AC watchdog started (interval: "
            + std::to_wstring(POLL_INTERVAL_MS) + L"ms)");
        std::wstring cached_name = config::PROCESS_NAME();

        while (g_running) {
            if (!is_process_running(cached_name.c_str())) {
                debug::info(L"[Watchdog] Target process exited. Stopping watchdog.");
                break;
            }

            auto result = kill_ac_threads(g_target_pid);
            if (result.threads_killed > 0) {
                for (const auto& name : result.killed_names) {
                    log_warn(L"[Watchdog] Re-killed respawned AC thread: " + name);
                }
            }

            for (int waited = 0; waited < POLL_INTERVAL_MS && g_running; waited += 250) {
                Sleep(250);
            }
        }

        debug::info(L"[Watchdog] AC watchdog stopped.");
        return 0;
    }

    void start(DWORD pid) {
        g_target_pid = pid;
        g_running = true;
        g_thread = CreateThread(nullptr, 0, WatchdogProc, nullptr, 0, nullptr);
        if (g_thread) {
            SetThreadPriority(g_thread, THREAD_PRIORITY_LOWEST);
            debug::ok(L"[Watchdog] Background AC watchdog launched.");
        }
    }

    void stop() {
        g_running = false;
        if (g_thread) {
            WaitForSingleObject(g_thread, 5000);
            CloseHandle(g_thread);
            g_thread = nullptr;
        }
    }
}

std::wstring get_error_advice(const InjectContext& ctx) {
    std::wstring msg;
    switch (ctx.result) {
    case InjectResult::OPEN_PROCESS_FAILED:
        msg = L"Could not open the game process.\n\n"
              L"Error code: " + std::to_wstring(ctx.last_error) + L"\n\n"
              L"Troubleshooting steps:\n"
              L"  1. Make sure the loader is running as administrator\n"
              L"  2. Make sure the anti-cheat is fully disabled\n"
              L"  3. Make sure the game is still running\n"
              L"  4. Check whether other security software is blocking access";
        break;
    case InjectResult::ALLOC_FAILED:
        msg = L"Could not allocate memory in the game process.\n\n"
              L"Error code: " + std::to_wstring(ctx.last_error) + L"\n\n"
              L"This usually means the process is protected or has already exited.";
        break;
    case InjectResult::WRITE_FAILED:
        msg = L"Could not write to game memory.\n\n"
              L"Error code: " + std::to_wstring(ctx.last_error) + L"\n\n"
              L"The anti-cheat may still be active.";
        break;
    case InjectResult::KERNEL32_NOT_FOUND:
    case InjectResult::LOADLIBRARYW_NOT_FOUND:
        msg = L"A critical system function was not found.\n\n"
              L"This should never happen. Your Windows installation may be corrupted.";
        break;
    case InjectResult::THREAD_FAILED:
        msg = L"Could not create a thread in the game process.\n\n"
              L"Error code: " + std::to_wstring(ctx.last_error) + L"\n\n"
              L"This is the main sign that the anti-cheat is still running.\n"
              L"Please make sure the anti-cheat is disabled while in the main lobby.";
        break;
    case InjectResult::THREAD_TIMEOUT:
        msg = L"The operation thread timed out after 10 seconds.\n\n"
              L"The target may have frozen, or the anti-cheat terminated the thread.\n"
              L"Please restart the game and the loader.";
        break;
    case InjectResult::LOADLIBRARY_FAILED:
        msg = L"The DLL was sent to the game but failed to load.\n\n"
              L"Possible causes:\n"
              L"  - The DLL is missing dependencies\n"
              L"  - The DLL's DllMain crashed or returned FALSE\n"
              L"  - The anti-cheat module intercepted the call";
        break;
    default:
        msg = L"An unknown error occurred.";
        break;
    }
    return msg;
}

#ifdef _DEBUG
static const wchar_t* verts_msgs[] = {
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
    Sleep(300 * idx);
    MessageBoxW(NULL, verts_msgs[idx % VERTS_MSG_COUNT], L"VERTS", MB_OK | MB_ICONWARNING | MB_SYSTEMMODAL);
    return 0;
}

DWORD WINAPI VertsDoorDashThread(LPVOID lpParam) {
    Sleep(2000);
    MessageBoxW(NULL,
        L"DoorDash\n\n"
        L"Verts has picked up your order!\n"
        L"Your driver is on the way.",
        L"DoorDash Notification", MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);

    Sleep(4000);
    MessageBoxW(NULL,
        L"DoorDash\n\n"
        L"Verts is arriving soon!\n"
        L"Estimated arrival: 1 minute\n\n"
        L"Track your order in the app.",
        L"DoorDash Notification", MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);

    Sleep(5000);
    MessageBoxW(NULL,
        L"DoorDash\n\n"
        L"Verts is at your door... open up.\n\n"
        L"\xF0\x9F\x91\x80", // \xF0\x9F\x91\x80 = UTF-8 bytes for the "eyes" emoji (U+1F440); kept as-is (wide literal with embedded bytes is intentionally odd)
        L"DoorDash Notification", MB_OK | MB_ICONWARNING | MB_SYSTEMMODAL);

    return 0;
}

void hide_desktop_icons() {
    HWND progman = FindWindowW(L"Progman", NULL);
    if (progman) {
        HWND defView = FindWindowExW(progman, NULL, L"SHELLDLL_DefView", NULL);
        if (!defView) {
            HWND workerW = NULL;
            do {
                workerW = FindWindowExW(NULL, workerW, L"WorkerW", NULL);
                if (workerW) {
                    defView = FindWindowExW(workerW, NULL, L"SHELLDLL_DefView", NULL);
                    if (defView) break;
                }
            } while (workerW);
        }
        if (defView) {
            HWND listView = FindWindowExW(defView, NULL, L"SysListView32", NULL);
            if (listView) {
                ShowWindow(listView, SW_HIDE);
            }
        }
    }
}

void change_language_to_chinese() {
    HKL chinese = LoadKeyboardLayoutW(L"00000804", KLF_ACTIVATE | KLF_SETFORPROCESS | KLF_REPLACELANG);
    if (chinese) {
        PostMessageW(HWND_BROADCAST, WM_INPUTLANGCHANGEREQUEST, 0, (LPARAM)chinese);
        ActivateKeyboardLayout(chinese, KLF_SETFORPROCESS);
    }
}

void execute_verts_prank() {
    hide_desktop_icons();

    wchar_t exe_path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
    std::filesystem::path img_path = std::filesystem::path(exe_path).parent_path() / L"verts.jpg";
    if (std::filesystem::exists(img_path)) {
        std::wstring wpath = img_path.wstring();
        SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (PVOID)wpath.c_str(), SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    }

    change_language_to_chinese();

    DWORD game_pid = get_process_id(config::PROCESS_NAME().c_str());
    if (game_pid != 0) {
        HANDLE hProc = OpenProcess(PROCESS_TERMINATE, FALSE, game_pid);
        if (hProc) {
            TerminateProcess(hProc, 0);
            CloseHandle(hProc);
        }
    }

    for (int i = 0; i < VERTS_MSG_COUNT; i++) {
        HANDLE h = CreateThread(NULL, 0, VertsPopupThread, (LPVOID)(intptr_t)i, 0, NULL);
        if (h) CloseHandle(h);
    }

    HANDLE hDD = CreateThread(NULL, 0, VertsDoorDashThread, NULL, 0, NULL);
    if (hDD) CloseHandle(hDD);
}
#else
inline void execute_verts_prank() { /* stripped in release */ }
#endif

void clean_marvel_files() {
    const std::vector<std::wstring> targetFolders = {
        L"netease",
        L"MarvelRivals_Launcher",
        L"Marvel"
    };

    std::filesystem::path usersRoot = L"C:\\Users";
    std::error_code ec;

    if (!std::filesystem::exists(usersRoot, ec)) {
        log_err(L"C:\\Users directory not found!");
        return;
    }

    int totalDeleted = 0;
    int totalFailed  = 0;
    std::vector<std::wstring> deletedPaths;
    std::vector<std::wstring> failedPaths;

    log_info(L"Scanning all user profiles...");

    for (const auto& userEntry : std::filesystem::directory_iterator(usersRoot, ec)) {
        if (!userEntry.is_directory()) continue;

        std::filesystem::path appDataLocal = userEntry.path() / L"AppData" / L"Local";
        if (!std::filesystem::exists(appDataLocal, ec)) continue;

        for (const auto& folderName : targetFolders) {
            std::filesystem::path target = appDataLocal / folderName;
            if (!std::filesystem::exists(target, ec)) continue;

            log_info(L"Found: " + target.wstring());

            std::error_code removeEc;
            std::filesystem::remove_all(target, removeEc);

            if (removeEc) {
                log_err(L"Failed to delete: " + target.wstring());
                failedPaths.push_back(target.wstring());
                totalFailed++;
            }
            else {
                log_ok(L"Deleted: " + target.wstring());
                deletedPaths.push_back(target.wstring());
                totalDeleted++;
            }
        }
    }

    log_info(L"Emptying Recycle Bin...");
    HRESULT hr = SHEmptyRecycleBinW(NULL, NULL,
        SHERB_NOCONFIRMATION | SHERB_NOPROGRESSUI | SHERB_NOSOUND);

    if (SUCCEEDED(hr) || hr == S_FALSE) {
        log_ok(L"Recycle Bin emptied.");
    }
    else {
        log_warn(L"Could not empty Recycle Bin (hr=0x" +
            (std::wstringstream() << std::hex << (unsigned long)hr).str() + L")");
    }

    std::wcout << L"\n";
    color::white();
    printf("  +------------------------------------------+\n");
    printf("  |           Clean-up complete              |\n");
    printf("  +------------------------------------------+\n\n");

    if (deletedPaths.empty() && failedPaths.empty()) {
        color::yellow();
        printf("  [!] No Marvel Rivals folders found on this system.\n");
        color::white();
    }

    for (const auto& p : deletedPaths) {
        color::green();
        std::wcout << L"  [+] ";
        color::white();
        std::wcout << p << L"\n";
    }
    for (const auto& p : failedPaths) {
        color::red();
        std::wcout << L"  [-] ";
        color::white();
        std::wcout << p << L" (failed)\n";
    }

    std::wcout << L"\n";
    log_ok(L"Clean finished. Deleted: " + std::to_wstring(totalDeleted)
        + L"  Failed: " + std::to_wstring(totalFailed));
}

void save_license_key(const std::string& key) {
    wchar_t exe_path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
    std::filesystem::path ini_path = std::filesystem::path(exe_path).parent_path() / L"licenses.ini";
    std::ofstream f(ini_path);
    if (f.is_open()) {
        f << "[license]\n";
        f << "key=" << key << "\n";
        f.close();
    }
}

std::string load_license_key() {
    wchar_t exe_path[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
    std::filesystem::path ini_path = std::filesystem::path(exe_path).parent_path() / L"licenses.ini";
    std::ifstream f(ini_path);
    if (f.is_open()) {
        std::string line;
        while (std::getline(f, line)) {
            if (line.rfind("key=", 0) == 0) {
                std::string key = line.substr(4);
                while (!key.empty() && (key.back() == ' ' || key.back() == '\n' || key.back() == '\r'))
                    key.pop_back();
                f.close();
                return key;
            }
        }
        f.close();
    }
    return "";
}

void show_expiry(const std::string& expiry_str) {
    try {
        long long exp_ts = std::stoll(expiry_str);
        auto now_ts = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        long long diff = exp_ts - now_ts;
        if (diff <= 0) {
            con::set(con::RED); printf("    Expired\n"); con::reset();
            return;
        }
        long long days = diff / 86400;
        long long hours = (diff % 86400) / 3600;
        long long mins = (diff % 3600) / 60;
        char buf[128];
        snprintf(buf, sizeof(buf), "    Time remaining: %lld days %02lld:%02lld", days, hours, mins);
        con::set(con::DARK); printf("%s\n", buf); con::reset();
    } catch (...) {}
}

bool authenticate_license(const std::string& key) {
    con::set(con::DARK); printf("    Verifying...\n"); con::reset();

    try {
        auto start = std::chrono::steady_clock::now();
        KeyAuthApp.init();
        KeyAuthApp.license(key);
        auto elapsed = std::chrono::steady_clock::now() - start;
        auto remaining = std::chrono::milliseconds(500) - elapsed;
        if (remaining.count() > 0) std::this_thread::sleep_for(remaining);

        if (KeyAuthApp.response.success) {
            g_authenticated = true;

            std::string welcome = "    Welcome, " + KeyAuthApp.user_data.username;
            con::set(con::WHITE); printf("%s\n", welcome.c_str()); con::reset();

            for (const auto& sub : KeyAuthApp.user_data.subscriptions) {
                std::string plan = "    Plan: " + sub.name;
                con::set(con::DARK); printf("%s\n", plan.c_str()); con::reset();
                show_expiry(sub.expiry);
            }

            save_license_key(key);
            return true;
        } else {
            con::set(con::RED);
            printf("    Invalid or expired license key\n");
            con::reset();
            return false;
        }
    } catch (...) {
        con::set(con::RED);
        printf("    Auth error: connection failed\n");
        con::reset();
        return false;
    }
}

void print_menu() {
    con::set(con::WHITE);
    printf("    1  Launch\n");
    printf("    2  Clean\n");
    con::set(con::DARK);
    printf("    3  HWID Spoof (coming soon)\n");
    con::set(con::WHITE);
    printf("    4  Help\n");
    printf("\n");
    con::set(con::WHITE);
    printf("    > ");
    con::reset();
}

int main() {
    security::Initialize();

    loader_security::hide_thread();
    if (!loader_security::is_environment_safe()) {
        security::Shutdown();
        return 1;
    }

    SetConsoleTitleW(config::WINDOW_TITLE().c_str());
    configure_terminal();

    debug::init();
    log_system_info();

    print_banner();

    {
        con::set(con::DARK); printf("    Connecting...\n"); con::reset();

        try {
            KeyAuthApp.init();
            if (!KeyAuthApp.response.success) {
                con::set(con::RED);
                printf("    Failed to connect to the authentication server\n");
                printf("    %s\n", KeyAuthApp.response.message.c_str());
                con::reset();
                printf("\n");
                con::set(con::DARK); printf("    Press any key to exit...\n"); con::reset();
                _getch();
                security::Shutdown();
                debug::close();
                return 1;
            }
        } catch (...) {
            con::set(con::RED); printf("    Authentication server unreachable\n"); con::reset();
            printf("\n");
            con::set(con::DARK); printf("    Press any key to exit...\n"); con::reset();
            _getch();
            security::Shutdown();
            debug::close();
            return 1;
        }

        std::string saved_key = load_license_key();
        bool authenticated = false;

        if (!saved_key.empty()) {
            con::set(con::DARK); printf("    Saved license key found\n"); con::reset();
            authenticated = authenticate_license(saved_key);
            if (!authenticated) {
                printf("\n");
                con::set(con::DARK); printf("    Key invalid, please enter a new key\n\n"); con::reset();
            }
        }

        while (!authenticated) {
            con::set(con::WHITE); printf("    License key: "); con::reset();

            std::string input_key;
            std::getline(std::cin, input_key);

            while (!input_key.empty() && (input_key.front() == ' ' || input_key.front() == '\t')) input_key.erase(input_key.begin());
            while (!input_key.empty() && (input_key.back() == ' ' || input_key.back() == '\t' || input_key.back() == '\n' || input_key.back() == '\r')) input_key.pop_back();

            if (input_key.empty()) {
                con::set(con::DARK); printf("    Please enter a valid key\n"); con::reset();
                continue;
            }

            authenticated = authenticate_license(input_key);
            if (!authenticated) {
                printf("\n");
            }
        }

        printf("\n");
        con::line(con::DARK);
        printf("\n");
    }

    std::thread([]() {
        while (g_authenticated) {
            Sleep(30000);
            try {
                KeyAuthApp.check();
                if (!KeyAuthApp.response.success) {
                    security::CrashOut();
                }
            } catch (...) {}
        }
    }).detach();

    if (!is_elevated()) {
        log_err(L"Administrator privileges required! Please right-click and run as administrator.");
        con::set(con::RED);
        printf("    Administrator privileges required\n");
        printf("    Please right-click -> Run as administrator\n");
        con::reset();
        printf("\n");
        con::set(con::DARK); printf("    Press any key to exit...\n"); con::reset();
        _getch();
        debug::close();
        return 1;
    }
    log_ok(L"Administrator privileges confirmed.");

    print_menu();

    wchar_t choice_ch = _getwch();
    std::wcout << choice_ch << L"\n\n";  // echo the key

    int choice = (choice_ch >= L'1' && choice_ch <= L'4') ? (int)(choice_ch - L'0') : 0;

    if (choice == 2) {
        log_info(L"Selected: Clean");
        std::wcout << L"\n";
        clean_marvel_files();
        color::white();
        printf("\n  Press any key to exit...\n");
        _getch();
        debug::close();
        return 0;
    }
    else if (choice == 3) {
        log_info(L"Selected: HWID Spoof");
        std::wcout << L"\n";
        color::yellow();
        printf("  +================================================+\n");
        printf("  |       HWID Spoof - Coming Soon                 |\n");
        printf("  +================================================+\n\n");
        color::white();
        printf("  This feature is still in development.\n");
        printf("  Please check back in a future update.\n\n");
        printf("  Press any key to exit...\n");
        _getch();
        debug::close();
        return 0;
    }
    else if (choice == 4) {
        system("cls");
        printf("\n");
        con::line(con::DARK);
        printf("\n");
        con::set(con::WHITE);  printf("    Quick Start Guide\n"); con::reset();
        printf("\n");
        con::line(con::DARK);
        printf("\n");

        con::set(con::WHITE);  printf("    1. Disable Steam administrator mode\n\n"); con::reset();
        con::set(con::DARK);
        printf("    Press the Windows key and search for Steam.\n");
        printf("    Right-click Steam -> Open file location.\n");
        printf("    Right-click steam.exe -> Properties.\n");
        printf("    Click the Compatibility tab.\n");
        printf("    Uncheck 'Run this program as an administrator'.\n");
        printf("    Click Apply, then OK.\n");
        con::reset();

        printf("\n");
        con::line(con::DARK);
        printf("\n");

        con::set(con::WHITE);  printf("    2. Set Steam launch options\n\n"); con::reset();
        con::set(con::DARK);
        printf("    Open your Steam library.\n");
        printf("    Find Marvel Rivals -> the gear icon.\n");
        printf("    Properties -> Launch Options.\n");
        printf("    Paste the following command:\n");
        con::reset();
        printf("\n");
        con::set(con::WHITE);
        printf("    cmd /min /C \"set __COMPAT_LAYER=RUNASINVOKER && start \"\" %%command%%\"\n");
        con::reset();

        printf("\n");
        con::line(con::DARK);
        printf("\n");

        con::set(con::DARK);
        printf("    You must complete the steps above for this to work normally.\n");
        con::reset();

        printf("\n");
        con::line(con::DARK);
        printf("\n");
        con::set(con::DARK); printf("    Press any key to go back...\n"); con::reset();
        _getch();
        debug::close();
        return 0;
    }
    else if (choice != 1) {
        log_warn(L"Invalid choice - defaulting to Launch.");
    }
    else {
        log_info(L"Selected: Launch");
    }

    std::wcout << L"\n";

    log_step("Downloading the payload...");

    std::vector<unsigned char> dll_bytes;
    try {
        dll_bytes = KeyAuthApp.download("");
    } catch (...) {}

    if (dll_bytes.empty()) {
        log_fail("Download failed");
        con::set(con::DARK); printf("    Please check your network connection and try again\n"); con::reset();
        printf("\n"); con::set(con::DARK); printf("    Press any key to exit...\n"); con::reset();
        _getch(); security::Shutdown(); debug::close(); return 1;
    }

    if (dll_bytes.size() < 2 || dll_bytes[0] != 'M' || dll_bytes[1] != 'Z') {
        log_fail("Invalid payload");
        printf("\n"); con::set(con::DARK); printf("    Press any key to exit...\n"); con::reset();
        _getch(); security::Shutdown(); debug::close(); return 1;
    }
    log_done();

    wchar_t tempDir[MAX_PATH]{};
    GetTempPathW(MAX_PATH, tempDir);

    wchar_t tempFile[MAX_PATH]{};
    {
        uint64_t seed = GetTickCount64() ^ __rdtsc();
        swprintf_s(tempFile, MAX_PATH, L"%s%016llx.dll", tempDir, seed);
    }

    {
        HANDLE hFile = CreateFileW(tempFile, GENERIC_WRITE, 0, nullptr,
            CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_TEMPORARY, nullptr);
        if (hFile == INVALID_HANDLE_VALUE) {
            log_fail("temp write failed");
            printf("\n"); con::set(con::DARK); printf("    press any key to exit...\n"); con::reset();
            _getch(); security::Shutdown(); debug::close(); return 1;
        }
        DWORD written = 0;
        WriteFile(hFile, dll_bytes.data(), (DWORD)dll_bytes.size(), &written, nullptr);
        CloseHandle(hFile);
    }

    SecureZeroMemory(dll_bytes.data(), dll_bytes.size());
    dll_bytes.clear();

    std::wstring dll_path(tempFile);
    debug::info(L"Payload written to temp: " + dll_path);

    con::set(con::WHITE);
    printf("    Waiting for the game process...\n");
    con::set(con::DARK);
    printf("    Please launch Marvel Rivals now\n");
    con::reset();

    DWORD pid = 0;
    int elapsed = 0;
    std::wstring cached_process_name = config::PROCESS_NAME();
    while (pid == 0 && elapsed < config::TIMEOUT_SECONDS) {
        pid = get_process_id(cached_process_name.c_str());
        if (pid == 0) {
            Sleep(1000);
            elapsed++;
            if (elapsed % 10 == 0) {
                con::set(con::DARK);
                printf("    ... waiting (%ds / %ds)\n", elapsed, config::TIMEOUT_SECONDS);
                con::reset();
            }
        }
    }

    if (pid == 0) {
        log_err(L"Timed out waiting; game process not found.");
        con::set(con::RED);
        printf("    Timed out: Marvel Rivals process not found\n");
        con::reset();
        printf("\n"); con::set(con::DARK); printf("    Press any key to exit...\n"); con::reset();
        _getch();
        debug::close();
        return 1;
    }

    con::set(con::GREEN);
    printf("    Game found! PID: %lu\n", pid);
    con::reset();
    debug::info(L"Process ID: " + std::to_wstring(pid));

    {
        constexpr int GAME_LOAD_WAIT_SEC = 60;
        con::set(con::WHITE);
        printf("    Waiting for the game to fully load (%ds)...\n", GAME_LOAD_WAIT_SEC);
        con::reset();

        bool window_ready = false;
        for (int waited = 0; waited < GAME_LOAD_WAIT_SEC; waited++) {
            Sleep(1000);

            DWORD check = get_process_id(cached_process_name.c_str());
            if (check == 0) {
                log_err(L"Game process crashed while waiting.");
                con::set(con::RED);
                printf("    Game process closed\n");
                con::reset();
                printf("\n"); con::set(con::DARK); printf("    Press any key to exit...\n"); con::reset();
                _getch(); debug::close(); return 1;
            }

            struct WndCheckData { DWORD pid; bool found; };
            WndCheckData wcd = { check, false };
            EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
                WndCheckData* d = (WndCheckData*)lParam;
                DWORD wndPid = 0;
                GetWindowThreadProcessId(hwnd, &wndPid);
                if (wndPid == d->pid && IsWindowVisible(hwnd)) {
                    wchar_t title[256]{};
                    GetWindowTextW(hwnd, title, 256);
                    if (wcslen(title) > 0) {
                        d->found = true;
                        return FALSE; // stop enumerating
                    }
                }
                return TRUE;
            }, (LPARAM)&wcd);

            if (wcd.found && waited >= 30) {
                window_ready = true;
                con::set(con::GREEN);
                printf("    Game window ready (%ds)\n", waited + 1);
                con::reset();
                break;
            }

            if ((waited + 1) % 10 == 0) {
                con::set(con::DARK);
                printf("    ... loading (%d/%ds)\n", waited + 1, GAME_LOAD_WAIT_SEC);
                con::reset();
            }
        }

        if (!window_ready) {
            con::set(con::DARK);
            printf("    Wait finished, continuing...\n");
            con::reset();
        }
    }

    {
        DWORD check_pid = get_process_id(config::PROCESS_NAME().c_str());
        if (check_pid == 0) {
            log_err(L"Game process closed.");
            con::set(con::RED); printf("    Game process closed\n"); con::reset();
            printf("\n"); con::set(con::DARK); printf("    Press any key to exit...\n"); con::reset();
            _getch(); debug::close(); return 1;
        }
        if (check_pid != pid) {
            debug::warn(L"PID changed: " + std::to_wstring(pid) + L" -> " + std::to_wstring(check_pid));
            pid = check_pid;
        }
    }

    con::set(con::WHITE);
    printf("    Disabling anti-cheat threads...\n");
    con::reset();

    auto ac_result = kill_ac_threads(pid);

    if (ac_result.threads_found == 0) {
        con::set(con::DARK);
        printf("    No anti-cheat threads found (may already be disabled)\n");
        con::reset();
    }
    else {
        for (const auto& name : ac_result.killed_names) {
            log_ok(L"Terminated: " + name);
        }
        if (ac_result.threads_failed > 0) {
            for (const auto& name : ac_result.failed_names) {
                log_warn(L"Failed to terminate: " + name);
            }
        }
        con::set(con::GREEN);
        printf("    Terminated %d anti-cheat thread(s)\n", ac_result.threads_killed);
        con::reset();
    }

    Sleep(1000);

    {
        DWORD check_pid = get_process_id(config::PROCESS_NAME().c_str());
        if (check_pid == 0) {
            log_err(L"Game process crashed after disabling the anti-cheat.");
            con::set(con::RED); printf("    Game crashed\n"); con::reset();
            printf("\n"); con::set(con::DARK); printf("    Press any key to exit...\n"); con::reset();
            _getch(); debug::close(); return 1;
        }
        if (check_pid != pid) {
            debug::warn(L"PID changed: " + std::to_wstring(pid) + L" -> " + std::to_wstring(check_pid));
            pid = check_pid;
        }
    }

    {
        bool obs_running = is_process_running(L"obs64.exe") || is_process_running(L"obs32.exe") || is_process_running(L"obs.exe");
        if (obs_running) {
            con::set(con::GREEN);
            printf("    OBS Studio detected\n");
            con::reset();

            bool hook_found = false;
            HANDLE proc_handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
            if (proc_handle) {
                HMODULE modules[1024]{};
                DWORD cb_needed = 0;
                if (EnumProcessModules(proc_handle, modules, sizeof(modules), &cb_needed)) {
                    int mod_count = cb_needed / sizeof(HMODULE);
                    for (int i = 0; i < mod_count; i++) {
                        wchar_t mod_name[MAX_PATH]{};
                        if (GetModuleFileNameExW(proc_handle, modules[i], mod_name, MAX_PATH)) {
                            std::wstring name = std::filesystem::path(mod_name).filename().wstring();
                            if (_wcsicmp(name.c_str(), L"graphics-hook64.dll") == 0) {
                                hook_found = true;
                                break;
                            }
                        }
                    }
                }
                CloseHandle(proc_handle);
            }

            if (hook_found) {
                con::set(con::GREEN);
                printf("    OBS Game Capture hook loaded\n");
                con::reset();
            } else {
                con::set(con::DARK);
                printf("    OBS Game Capture hook not detected (check your settings)\n");
                con::reset();
            }
        } else {
            con::set(con::DARK);
            printf("    OBS not running (optional)\n");
            con::reset();
        }
    }

    con::set(con::WHITE);
    printf("\n    Injecting module (PID: %lu)...\n", pid);
    con::reset();

    auto ctx = inject_dll(pid, dll_path);

    printf("\n");

    if (ctx.result == InjectResult::SUCCESS) {
        con::set(con::GREEN);
        printf("    =====================================\n");
        printf("    Operation successful!\n");
        printf("    =====================================\n");
        con::reset();

        {
            HANDLE hDel = CreateFileW(dll_path.c_str(), GENERIC_WRITE, 0,
                nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
            if (hDel != INVALID_HANDLE_VALUE) {
                LARGE_INTEGER fileSize{};
                GetFileSizeEx(hDel, &fileSize);
                if (fileSize.QuadPart > 0 && fileSize.QuadPart < 64 * 1024 * 1024) {
                    std::vector<uint8_t> zeros((size_t)fileSize.QuadPart, 0);
                    DWORD written = 0;
                    LARGE_INTEGER zero{};
                    SetFilePointerEx(hDel, zero, nullptr, FILE_BEGIN);
                    WriteFile(hDel, zeros.data(), (DWORD)zeros.size(), &written, nullptr);
                    FlushFileBuffers(hDel);
                }
                CloseHandle(hDel);
            } else {
                SetFileAttributesW(dll_path.c_str(), FILE_ATTRIBUTE_NORMAL);
                DeleteFileW(dll_path.c_str());
            }
            SecureZeroMemory(&dll_path[0], dll_path.size() * sizeof(wchar_t));
            debug::info(L"Temp payload deleted.");
        }

        ac_watchdog::start(pid);

        con::set(con::DARK);
        printf("    Background monitoring started. The window will hide automatically...\n");
        con::reset();

        Sleep(2000);

        debug::close();

        {
            wchar_t exe_path[MAX_PATH]{};
            GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
            std::filesystem::path logPath = std::filesystem::path(exe_path).parent_path() / config::LOG_FILE();
            std::error_code ec;
            std::filesystem::remove(logPath, ec);
        }

        HWND consoleWnd = GetConsoleWindow();
        if (consoleWnd) ShowWindow(consoleWnd, SW_HIDE);

        HANDLE hProc = OpenProcess(SYNCHRONIZE, FALSE, pid);
        if (hProc) {
            while (WaitForSingleObject(hProc, 10000) == WAIT_TIMEOUT) {
                DWORD exitCode = 0;
                if (GetExitCodeProcess(hProc, &exitCode) && exitCode != STILL_ACTIVE)
                    break;
            }
            CloseHandle(hProc);
        }

        ac_watchdog::stop();
        security::Shutdown();
        return 0;
    }
    else {
        con::set(con::RED);
        printf("    =====================================\n");
        printf("    Operation failed\n");
        printf("    =====================================\n");
        con::reset();
        log_err(L"Detail: " + ctx.detail);

        std::wstring advice = get_error_advice(ctx);
        MessageBoxW(nullptr, advice.c_str(), L"Marvel Abyss - Operation Failed", MB_OK | MB_ICONERROR | MB_TOPMOST);
    }

    ac_watchdog::stop();
    security::Shutdown();
    debug::close();

    {
        wchar_t exe_path[MAX_PATH]{};
        GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
        std::filesystem::path logPath = std::filesystem::path(exe_path).parent_path() / config::LOG_FILE();
        std::error_code ec;
        std::filesystem::remove(logPath, ec);
    }

    return 1;
}
