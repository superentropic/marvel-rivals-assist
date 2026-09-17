#pragma once
#include <Windows.h>
#include <cstdint>
#include <intrin.h>
#include <string>

// ============================================================
//  Compile-time XOR string encryption for the loader
// ============================================================
namespace xor_detail {
    constexpr char xor_key(size_t i) {
        constexpr char keys[] = { 0x4B, 0x7A, 0x2D, 0x51, 0x63, 0x1F, 0x6E, 0x39 };
        return keys[i % 8];
    }

    template<size_t N>
    struct encrypted_string {
        char data[N];
        constexpr encrypted_string(const char(&str)[N]) : data{} {
            for (size_t i = 0; i < N; ++i)
                data[i] = str[i] ^ xor_key(i);
        }
    };

    template<size_t N>
    struct encrypted_wstring {
        wchar_t data[N];
        constexpr encrypted_wstring(const wchar_t(&str)[N]) : data{} {
            for (size_t i = 0; i < N; ++i)
                data[i] = str[i] ^ static_cast<wchar_t>(xor_key(i));
        }
    };
}

template<size_t N>
class xor_str {
    char buf[N];
public:
    xor_str(const xor_detail::encrypted_string<N>& enc) {
        for (size_t i = 0; i < N; ++i)
            buf[i] = enc.data[i] ^ xor_detail::xor_key(i);
    }
    ~xor_str() { SecureZeroMemory(buf, N); }
    const char* c_str() const { return buf; }
    operator const char*() const { return buf; }
};

template<size_t N>
class xor_wstr {
    wchar_t buf[N];
public:
    xor_wstr(const xor_detail::encrypted_wstring<N>& enc) {
        for (size_t i = 0; i < N; ++i)
            buf[i] = enc.data[i] ^ static_cast<wchar_t>(xor_detail::xor_key(i));
    }
    ~xor_wstr() { SecureZeroMemory(buf, N * sizeof(wchar_t)); }
    const wchar_t* c_str() const { return buf; }
    operator const wchar_t*() const { return buf; }
    std::wstring str() const { return std::wstring(buf); }
};

#define XS(s) (xor_str<sizeof(s)>(xor_detail::encrypted_string<sizeof(s)>(s)).c_str())
#define XSW(s) (xor_wstr<sizeof(s)/sizeof(wchar_t)>(xor_detail::encrypted_wstring<sizeof(s)/sizeof(wchar_t)>(s)))

// ============================================================
//  Anti-debug checks for the loader
// ============================================================
namespace loader_security {

    inline bool is_debugger_present() {
        return IsDebuggerPresent() != FALSE;
    }

    inline bool is_remote_debugger() {
        BOOL present = FALSE;
        CheckRemoteDebuggerPresent(GetCurrentProcess(), &present);
        return present != FALSE;
    }

    inline bool has_hardware_breakpoints() {
        CONTEXT ctx = {};
        ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        if (GetThreadContext(GetCurrentThread(), &ctx)) {
            return (ctx.Dr0 || ctx.Dr1 || ctx.Dr2 || ctx.Dr3);
        }
        return false;
    }

    inline bool check_timing_anomaly() {
        LARGE_INTEGER freq, start, end;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&start);
        // Do some trivial work
        volatile int x = 0;
        for (int i = 0; i < 100; ++i) x += i;
        QueryPerformanceCounter(&end);
        double elapsed_ms = (double)(end.QuadPart - start.QuadPart) / freq.QuadPart * 1000.0;
        return elapsed_ms > 50.0; // Should take < 1ms normally
    }

    inline bool is_environment_safe() {
        if (is_debugger_present()) return false;
        if (is_remote_debugger()) return false;
        if (has_hardware_breakpoints()) return false;
        if (check_timing_anomaly()) return false;
        return true;
    }

    // Hide the current thread from debugger
    inline void hide_thread(HANDLE hThread = GetCurrentThread()) {
        using NtSetInfoThread = LONG(NTAPI*)(HANDLE, ULONG, PVOID, ULONG);
        static auto fn = reinterpret_cast<NtSetInfoThread>(
            GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtSetInformationThread"));
        if (fn) fn(hThread, 0x11, nullptr, 0);
    }
}
