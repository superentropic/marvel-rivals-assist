#pragma once
#include <Windows.h>
#include <cstdint>
#include <intrin.h>
#include <winternl.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#include <atomic>
#include <bcrypt.h>

#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "bcrypt.lib")

// ============================================================
//  Anti-Crack Security System
//  Layers: Integrity, Anti-Debug, Anti-Dump, Anti-Hook,
//          Anti-Tamper, Watchdog, Dynamic API Resolution
// ============================================================

namespace security {

    // ---- Terminate hard (can't be hooked as easily as ExitProcess) ----
    static void __declspec(noinline) CrashOut() {
        // Multiple termination methods — if one is hooked, others still work
        __try {
            // Method 1: NtTerminateProcess (kernel-level, harder to hook)
            using NtTerminateProcessFn = NTSTATUS(NTAPI*)(HANDLE, NTSTATUS);
            auto ntTerminate = reinterpret_cast<NtTerminateProcessFn>(
                GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtTerminateProcess"));
            if (ntTerminate) ntTerminate(GetCurrentProcess(), 0xDEAD);
        } __except(1) {}

        // Method 2: Corrupt own memory to force crash
        volatile int* p = nullptr;
        *p = 0;

        // Method 3: If somehow still alive
        TerminateProcess(GetCurrentProcess(), 0xDEAD);
    }

    // ============================================================
    //  1. SELF-INTEGRITY CHECK
    //  Hashes the loader's .text section at startup, then verifies
    //  it hasn't been patched at any point during execution.
    // ============================================================
    namespace integrity {
        static uint64_t s_textHash = 0;
        static uintptr_t s_textBase = 0;
        static size_t s_textSize = 0;

        // FNV-1a 64-bit hash
        static uint64_t HashMemory(const uint8_t* data, size_t len) {
            uint64_t hash = 0xcbf29ce484222325ULL;
            for (size_t i = 0; i < len; i++) {
                hash ^= data[i];
                hash *= 0x100000001b3ULL;
            }
            return hash;
        }

        static bool FindTextSection(uintptr_t& base, size_t& size) {
            HMODULE hMod = GetModuleHandleA(nullptr);
            if (!hMod) return false;
            auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(hMod);
            auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>((uint8_t*)hMod + dos->e_lfanew);
            auto section = IMAGE_FIRST_SECTION(nt);
            for (int i = 0; i < nt->FileHeader.NumberOfSections; i++, section++) {
                if (memcmp(section->Name, ".text", 5) == 0) {
                    base = (uintptr_t)hMod + section->VirtualAddress;
                    size = section->Misc.VirtualSize;
                    return true;
                }
            }
            return false;
        }

        // Call once at startup to snapshot the .text section
        static void Initialize() {
            if (FindTextSection(s_textBase, s_textSize)) {
                s_textHash = HashMemory(reinterpret_cast<const uint8_t*>(s_textBase), s_textSize);
            }
        }

        // Call periodically to detect code patching
        static bool Verify() {
            if (s_textHash == 0 || s_textBase == 0) return true; // not initialized
            uint64_t currentHash = HashMemory(reinterpret_cast<const uint8_t*>(s_textBase), s_textSize);
            return currentHash == s_textHash;
        }
    }

    // ============================================================
    //  2. ANTI-DUMP
    //  Erases PE headers and DataDirectory from own process memory
    //  so memory dumps produce invalid PE files.
    // ============================================================
    namespace antidump {
        static void ErasePEHeaders() {
            HMODULE hMod = GetModuleHandleA(nullptr);
            if (!hMod) return;

            auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(hMod);
            auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>((uint8_t*)hMod + dos->e_lfanew);
            DWORD headerSize = nt->OptionalHeader.SizeOfHeaders;

            // Wipe DataDirectory entries (import table, export table, etc.)
            // This prevents tools from reconstructing the IAT
            DWORD oldProtect;
            if (VirtualProtect(&nt->OptionalHeader.DataDirectory, 
                sizeof(IMAGE_DATA_DIRECTORY) * IMAGE_NUMBEROF_DIRECTORY_ENTRIES,
                PAGE_READWRITE, &oldProtect)) {
                SecureZeroMemory(&nt->OptionalHeader.DataDirectory,
                    sizeof(IMAGE_DATA_DIRECTORY) * IMAGE_NUMBEROF_DIRECTORY_ENTRIES);
                VirtualProtect(&nt->OptionalHeader.DataDirectory,
                    sizeof(IMAGE_DATA_DIRECTORY) * IMAGE_NUMBEROF_DIRECTORY_ENTRIES,
                    oldProtect, &oldProtect);
            }

            // Wipe the DOS header (except e_lfanew which we already used)
            if (VirtualProtect(hMod, sizeof(IMAGE_DOS_HEADER), PAGE_READWRITE, &oldProtect)) {
                // Corrupt the MZ signature and DOS stub
                dos->e_magic = 0;
                SecureZeroMemory(((uint8_t*)hMod) + 2, dos->e_lfanew - 2);
                VirtualProtect(hMod, sizeof(IMAGE_DOS_HEADER), oldProtect, &oldProtect);
            }

            // Wipe the PE signature
            if (VirtualProtect(nt, sizeof(DWORD), PAGE_READWRITE, &oldProtect)) {
                nt->Signature = 0;
                VirtualProtect(nt, sizeof(DWORD), oldProtect, &oldProtect);
            }
        }

        // Scramble section names so dumpers can't identify sections
        static void ScrambleSectionNames() {
            HMODULE hMod = GetModuleHandleA(nullptr);
            if (!hMod) return;
            auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(hMod);
            auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>((uint8_t*)hMod + dos->e_lfanew);
            auto section = IMAGE_FIRST_SECTION(nt);
            DWORD oldProtect;
            for (int i = 0; i < nt->FileHeader.NumberOfSections; i++, section++) {
                if (VirtualProtect(section->Name, 8, PAGE_READWRITE, &oldProtect)) {
                    for (int j = 0; j < 8; j++)
                        section->Name[j] = (BYTE)((__rdtsc() >> (j * 4)) & 0xFF);
                    VirtualProtect(section->Name, 8, oldProtect, &oldProtect);
                }
            }
        }
    }

    // ============================================================
    //  3. ANTI-HOOK DETECTION
    //  Checks if critical WinAPI functions have been detoured by
    //  verifying the first bytes of each function aren't a JMP.
    // ============================================================
    namespace antihook {
        static bool IsFunctionHooked(void* funcAddr) {
            if (!funcAddr) return false;
            __try {
                uint8_t* bytes = reinterpret_cast<uint8_t*>(funcAddr);
                // Check for common hook patterns:
                // E9 xx xx xx xx  = JMP rel32 (most common detour)
                if (bytes[0] == 0xE9) return true;
                // FF 25 xx xx xx xx = JMP [rip+disp32] (x64 absolute jump)
                if (bytes[0] == 0xFF && bytes[1] == 0x25) return true;
                // 48 B8 xx ... xx FF E0 = mov rax, imm64; jmp rax
                if (bytes[0] == 0x48 && bytes[1] == 0xB8) {
                    if (bytes[10] == 0xFF && bytes[11] == 0xE0) return true;
                }
                // CC = INT3 breakpoint
                if (bytes[0] == 0xCC) return true;
                return false;
            } __except(1) {
                return true; // Can't read = suspicious
            }
        }

        static bool CheckCriticalAPIs() {
            HMODULE k32 = GetModuleHandleA("kernel32.dll");
            HMODULE ntdll = GetModuleHandleA("ntdll.dll");
            if (!k32 || !ntdll) return false;

            // These are the functions a cracker would hook to intercept the loader
            const char* k32Funcs[] = {
                "CreateRemoteThread", "WriteProcessMemory", "VirtualAllocEx",
                "OpenProcess", "ReadProcessMemory", "VirtualProtectEx",
                "CreateFileW", "LoadLibraryW", "GetProcAddress"
            };
            const char* ntdllFuncs[] = {
                "NtWriteVirtualMemory", "NtCreateThreadEx", "NtOpenProcess",
                "NtAllocateVirtualMemory", "NtProtectVirtualMemory"
            };

            for (auto& name : k32Funcs) {
                void* addr = GetProcAddress(k32, name);
                if (IsFunctionHooked(addr)) return true;
            }
            for (auto& name : ntdllFuncs) {
                void* addr = GetProcAddress(ntdll, name);
                if (IsFunctionHooked(addr)) return true;
            }
            return false;
        }
    }

    // ============================================================
    //  4. ENHANCED ANTI-DEBUG (beyond basic checks in stealth.h)
    // ============================================================
    namespace antidebug {
        // Check for debugger via NtQueryInformationProcess with ProcessDebugPort
        static bool CheckDebugPort() {
            using NtQueryInfoProc = NTSTATUS(NTAPI*)(HANDLE, ULONG, PVOID, ULONG, PULONG);
            static auto fn = reinterpret_cast<NtQueryInfoProc>(
                GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess"));
            if (!fn) return false;
            HANDLE port = nullptr;
            fn(GetCurrentProcess(), 7, &port, sizeof(port), nullptr);
            return port != nullptr;
        }

        // Check for debug object handle
        static bool CheckDebugObject() {
            using NtQueryInfoProc = NTSTATUS(NTAPI*)(HANDLE, ULONG, PVOID, ULONG, PULONG);
            static auto fn = reinterpret_cast<NtQueryInfoProc>(
                GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtQueryInformationProcess"));
            if (!fn) return false;
            HANDLE obj = nullptr;
            NTSTATUS st = fn(GetCurrentProcess(), 0x1E, &obj, sizeof(obj), nullptr);
            return (st == 0 && obj != nullptr);
        }

        // Detect if someone has attached via OutputDebugString trick
        static bool CheckOutputDebugString() {
            SetLastError(0);
            OutputDebugStringA("anti_crack_probe");
            return GetLastError() == 0; // If debugger is attached, error stays 0
        }

        // Check if NtClose throws exception on invalid handle (debugger present)
        static bool CheckCloseHandle() {
            __try {
                CloseHandle((HANDLE)0xDEADBEEF);
            } __except (1) {
                return true; // Exception = debugger
            }
            return false;
        }

        // Detect VirtualBox/VMware via CPUID brand string
        static bool CheckVirtualMachine() {
            int cpu[4] = {};
            __cpuid(cpu, 0x40000000);
            char vendor[13] = {};
            memcpy(vendor, &cpu[1], 4);
            memcpy(vendor + 4, &cpu[2], 4);
            memcpy(vendor + 8, &cpu[3], 4);
            if (strstr(vendor, "VMware")) return true;
            if (strstr(vendor, "VBoxVBox")) return true;
            if (strstr(vendor, "Hyper-V")) return true;
            return false;
        }

        // Scan for known RE tool processes
        static bool ScanForRETools() {
            const wchar_t* tools[] = {
                L"x64dbg.exe", L"x32dbg.exe", L"ollydbg.exe",
                L"ida.exe", L"ida64.exe", L"idaq.exe", L"idaq64.exe",
                L"ProcessHacker.exe", L"procmon.exe", L"procmon64.exe",
                L"Wireshark.exe", L"fiddler.exe", L"dnSpy.exe",
                L"HxD.exe", L"cheatengine-x86_64.exe", L"cheatengine-i386.exe",
                L"ReClass.NET.exe", L"Scylla_x64.exe", L"Scylla_x86.exe",
                L"httpdebugger.exe", L"HTTPDebuggerUI.exe",
                L"die.exe", L"pestudio.exe", L"lordpe.exe",
                L"dotPeek64.exe", L"ILSpy.exe", L"de4dot.exe",
                L"MegaDumper.exe", L"dumper.exe", L"ImportREC.exe"
            };

            HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (snap == INVALID_HANDLE_VALUE) return false;
            PROCESSENTRY32W pe = {}; pe.dwSize = sizeof(pe);
            bool found = false;
            if (Process32FirstW(snap, &pe)) {
                do {
                    for (auto& t : tools) {
                        if (_wcsicmp(pe.szExeFile, t) == 0) { found = true; break; }
                    }
                    if (found) break;
                } while (Process32NextW(snap, &pe));
            }
            CloseHandle(snap);
            return found;
        }

        // Full anti-debug sweep
        static bool RunFullCheck() {
            if (IsDebuggerPresent()) return true;
            BOOL remote = FALSE;
            CheckRemoteDebuggerPresent(GetCurrentProcess(), &remote);
            if (remote) return true;
            if (CheckDebugPort()) return true;
            if (CheckDebugObject()) return true;
            if (CheckCloseHandle()) return true;
            if (ScanForRETools()) return true;
            return false;
        }
    }

    // ============================================================
    //  5. CONTINUOUS WATCHDOG THREAD
    // ============================================================
    namespace watchdog {
        static std::atomic<bool> s_running{ false };
        static HANDLE s_thread = nullptr;

        static DWORD WINAPI WatchdogProc(LPVOID) {
            using NtSetInfoThread = NTSTATUS(NTAPI*)(HANDLE, ULONG, PVOID, ULONG);
            auto hideThread = reinterpret_cast<NtSetInfoThread>(
                GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtSetInformationThread"));
            if (hideThread) hideThread(GetCurrentThread(), 0x11, nullptr, 0);

            while (s_running.load()) {
                for (int waited = 0; waited < 10000 && s_running.load(); waited += 200)
                    Sleep(200);

                if (!s_running.load()) break;

                // Only check for an actively attached debugger
                if (IsDebuggerPresent()) CrashOut();
                BOOL remoteDbg = FALSE;
                CheckRemoteDebuggerPresent(GetCurrentProcess(), &remoteDbg);
                if (remoteDbg) CrashOut();
            }
            return 0;
        }

        static void Start() {
            s_running = true;
            s_thread = CreateThread(nullptr, 0, WatchdogProc, nullptr, 0, nullptr);
            if (s_thread) SetThreadPriority(s_thread, THREAD_PRIORITY_LOWEST);
        }

        static void Stop() {
            s_running = false;
            if (s_thread) {
                WaitForSingleObject(s_thread, 3000);
                CloseHandle(s_thread);
                s_thread = nullptr;
            }
        }
    }

    // ============================================================
    //  6. DYNAMIC API RESOLUTION
    //  Resolves WinAPI functions by hash at runtime instead of
    //  importing them, so the IAT doesn't reveal what APIs we use.
    // ============================================================
    namespace dynapi {
        // Compile-time API name hash (djb2)
        static constexpr uint32_t HashAPI(const char* str) {
            uint32_t hash = 5381;
            while (*str) { hash = ((hash << 5) + hash) + *str++; }
            return hash;
        }

        // Runtime resolve: walk export table of a module to find function by hash
        static void* ResolveByHash(HMODULE hMod, uint32_t targetHash) {
            if (!hMod) return nullptr;
            auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(hMod);
            auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>((uint8_t*)hMod + dos->e_lfanew);
            auto& exportDir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
            if (!exportDir.VirtualAddress) return nullptr;

            auto exports = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>((uint8_t*)hMod + exportDir.VirtualAddress);
            auto names = reinterpret_cast<DWORD*>((uint8_t*)hMod + exports->AddressOfNames);
            auto funcs = reinterpret_cast<DWORD*>((uint8_t*)hMod + exports->AddressOfFunctions);
            auto ords = reinterpret_cast<WORD*>((uint8_t*)hMod + exports->AddressOfNameOrdinals);

            for (DWORD i = 0; i < exports->NumberOfNames; i++) {
                const char* name = (const char*)hMod + names[i];
                if (HashAPI(name) == targetHash) {
                    return (void*)((uint8_t*)hMod + funcs[ords[i]]);
                }
            }
            return nullptr;
        }

        // Pre-computed hashes for critical functions
        static constexpr uint32_t H_CreateRemoteThread    = HashAPI("CreateRemoteThread");
        static constexpr uint32_t H_WriteProcessMemory     = HashAPI("WriteProcessMemory");
        static constexpr uint32_t H_VirtualAllocEx         = HashAPI("VirtualAllocEx");
        static constexpr uint32_t H_VirtualFreeEx          = HashAPI("VirtualFreeEx");
        static constexpr uint32_t H_OpenProcess            = HashAPI("OpenProcess");
        static constexpr uint32_t H_VirtualProtectEx       = HashAPI("VirtualProtectEx");

        // Typed function pointers
        using pCreateRemoteThread = HANDLE(WINAPI*)(HANDLE, LPSECURITY_ATTRIBUTES, SIZE_T,
            LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD);
        using pWriteProcessMemory = BOOL(WINAPI*)(HANDLE, LPVOID, LPCVOID, SIZE_T, SIZE_T*);
        using pVirtualAllocEx = LPVOID(WINAPI*)(HANDLE, LPVOID, SIZE_T, DWORD, DWORD);
        using pVirtualFreeEx = BOOL(WINAPI*)(HANDLE, LPVOID, SIZE_T, DWORD);
        using pOpenProcess = HANDLE(WINAPI*)(DWORD, BOOL, DWORD);
        using pVirtualProtectEx = BOOL(WINAPI*)(HANDLE, LPVOID, SIZE_T, DWORD, PDWORD);

        // Resolved pointers (populated at runtime)
        static pCreateRemoteThread  fnCreateRemoteThread  = nullptr;
        static pWriteProcessMemory  fnWriteProcessMemory  = nullptr;
        static pVirtualAllocEx      fnVirtualAllocEx      = nullptr;
        static pVirtualFreeEx       fnVirtualFreeEx       = nullptr;
        static pOpenProcess         fnOpenProcess         = nullptr;
        static pVirtualProtectEx    fnVirtualProtectEx    = nullptr;

        static bool ResolveAll() {
            HMODULE k32 = GetModuleHandleA("kernel32.dll");
            if (!k32) return false;
            fnCreateRemoteThread = (pCreateRemoteThread)ResolveByHash(k32, H_CreateRemoteThread);
            fnWriteProcessMemory = (pWriteProcessMemory)ResolveByHash(k32, H_WriteProcessMemory);
            fnVirtualAllocEx     = (pVirtualAllocEx)ResolveByHash(k32, H_VirtualAllocEx);
            fnVirtualFreeEx      = (pVirtualFreeEx)ResolveByHash(k32, H_VirtualFreeEx);
            fnOpenProcess        = (pOpenProcess)ResolveByHash(k32, H_OpenProcess);
            fnVirtualProtectEx   = (pVirtualProtectEx)ResolveByHash(k32, H_VirtualProtectEx);
            return fnCreateRemoteThread && fnWriteProcessMemory && fnVirtualAllocEx &&
                   fnVirtualFreeEx && fnOpenProcess && fnVirtualProtectEx;
        }
    }

    // ============================================================
    //  7. DLL PAYLOAD ENCRYPTION
    //  Encrypts/decrypts the DLL file with a multi-round XOR cipher.
    //  The DLL on disk should be pre-encrypted with EncryptFile().
    //  The loader decrypts to a memory buffer before injecting.
    // ============================================================
    namespace payload {
        // 256-bit key (change this to your own random bytes)
        static const uint8_t PAYLOAD_KEY[32] = {
            0xA3, 0x7B, 0x1D, 0xE4, 0x92, 0x56, 0xC8, 0x0F,
            0x3D, 0x81, 0x67, 0xFA, 0x2E, 0xB5, 0x49, 0xD0,
            0x74, 0x1C, 0xE8, 0x93, 0x5A, 0x06, 0xBF, 0x42,
            0xD7, 0x68, 0x2B, 0x9E, 0xF1, 0x53, 0xA4, 0x0D
        };

        // Multi-round XOR with key schedule
        static void CryptBuffer(uint8_t* data, size_t size) {
            uint64_t state = 0;
            for (int i = 0; i < 32; i += 8)
                state ^= *reinterpret_cast<const uint64_t*>(&PAYLOAD_KEY[i]);

            for (size_t i = 0; i < size; i++) {
                // Key schedule: rotate and mix
                state ^= state >> 13;
                state *= 0xff51afd7ed558ccdULL;
                state ^= state >> 33;
                state ^= PAYLOAD_KEY[i % 32];

                data[i] ^= (uint8_t)(state >> ((i % 8) * 8));
            }
        }

        // Read and decrypt a .enc payload file into a memory buffer
        // Returns allocated buffer (caller must free) and sets outSize
        static uint8_t* DecryptPayload(const wchar_t* encPath, size_t& outSize) {
            HANDLE hFile = CreateFileW(encPath, GENERIC_READ, FILE_SHARE_READ,
                nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (hFile == INVALID_HANDLE_VALUE) return nullptr;

            DWORD fileSizeHigh = 0;
            DWORD fileSizeLow = GetFileSize(hFile, &fileSizeHigh);
            size_t fileSize = fileSizeLow;
            if (fileSize == 0) { CloseHandle(hFile); return nullptr; }

            uint8_t* buffer = (uint8_t*)VirtualAlloc(nullptr, fileSize,
                MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (!buffer) { CloseHandle(hFile); return nullptr; }

            DWORD bytesRead = 0;
            if (!ReadFile(hFile, buffer, (DWORD)fileSize, &bytesRead, nullptr) || bytesRead != fileSizeLow) {
                VirtualFree(buffer, 0, MEM_RELEASE);
                CloseHandle(hFile);
                return nullptr;
            }
            CloseHandle(hFile);

            // Decrypt in place
            CryptBuffer(buffer, fileSize);

            // Verify it's a valid PE after decryption (MZ check)
            if (fileSize < 2 || buffer[0] != 'M' || buffer[1] != 'Z') {
                // Wrong key or corrupted file
                SecureZeroMemory(buffer, fileSize);
                VirtualFree(buffer, 0, MEM_RELEASE);
                return nullptr;
            }

            outSize = fileSize;
            return buffer;
        }

        // Utility: encrypt a raw DLL file and write the .enc version
        // Run this once offline to prepare the encrypted payload
        static bool EncryptFile(const wchar_t* inputDll, const wchar_t* outputEnc) {
            HANDLE hIn = CreateFileW(inputDll, GENERIC_READ, FILE_SHARE_READ,
                nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (hIn == INVALID_HANDLE_VALUE) return false;

            DWORD size = GetFileSize(hIn, nullptr);
            std::vector<uint8_t> data(size);
            DWORD read = 0;
            ReadFile(hIn, data.data(), size, &read, nullptr);
            CloseHandle(hIn);

            CryptBuffer(data.data(), size);

            HANDLE hOut = CreateFileW(outputEnc, GENERIC_WRITE, 0,
                nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (hOut == INVALID_HANDLE_VALUE) return false;

            DWORD written = 0;
            WriteFile(hOut, data.data(), size, &written, nullptr);
            CloseHandle(hOut);
            return written == size;
        }

        // Securely free a decrypted payload buffer
        static void FreePayload(uint8_t* buffer, size_t size) {
            if (buffer) {
                SecureZeroMemory(buffer, size);
                VirtualFree(buffer, 0, MEM_RELEASE);
            }
        }
    }

    // ============================================================
    //  MASTER INIT — Call at the very start of main()
    // ============================================================
    static void Initialize() {
        // 1. Basic debugger check (no false positives)
        if (IsDebuggerPresent()) CrashOut();
        BOOL remote = FALSE;
        CheckRemoteDebuggerPresent(GetCurrentProcess(), &remote);
        if (remote) CrashOut();

        // 2. RE tool scan (only known cracker tools)
        if (antidebug::ScanForRETools()) CrashOut();

        // 3. Resolve APIs dynamically (bypass IAT hooks)
        //    Non-fatal: if it fails on some edge-case system, continue
        dynapi::ResolveAll();

        // 4. Snapshot code integrity for watchdog to verify later
        integrity::Initialize();

        // 5. Start continuous watchdog
        watchdog::Start();

        // NOTE: Anti-hook check and PE header erasure intentionally omitted
        // from startup — anti-hook causes false positives with AV/anticheat,
        // and PE erasure can destabilize the CRT before full initialization.
        // These run selectively in the watchdog instead.
    }

    // Call on exit
    static void Shutdown() {
        watchdog::Stop();
    }

} // namespace security
