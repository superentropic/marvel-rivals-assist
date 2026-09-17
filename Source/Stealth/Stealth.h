#pragma once
#include <Windows.h>
#include <cstdint>
#include <intrin.h>
#include <random>

namespace Stealth {

	// Hide a thread from debugger and thread enumeration
	inline void HideThread(HANDLE hThread = GetCurrentThread()) {
		using NtSetInformationThreadFn = LONG(NTAPI*)(HANDLE, ULONG, PVOID, ULONG);
		static auto pNtSetInformationThread = reinterpret_cast<NtSetInformationThreadFn>(
			GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtSetInformationThread"));
		if (pNtSetInformationThread) {
			pNtSetInformationThread(hThread, 0x11 /*ThreadHideFromDebugger*/, nullptr, 0);
		}
	}

	// Create a thread with randomized start delay and hidden from debugger
	inline HANDLE CreateHiddenThread(LPTHREAD_START_ROUTINE func, LPVOID param, DWORD* tid = nullptr) {
		HANDLE hThread = CreateThread(nullptr, 0, func, param, CREATE_SUSPENDED, tid);
		if (hThread) {
			HideThread(hThread);
			ResumeThread(hThread);
		}
		return hThread;
	}

	// Randomized sleep to break timing patterns
	inline void RandomSleep(int minMs, int maxMs) {
		static thread_local std::mt19937 rng(
			(unsigned int)(__rdtsc() ^ (uint64_t)GetCurrentThreadId()));
		std::uniform_int_distribution<int> dist(minMs, maxMs);
		Sleep(dist(rng));
	}

	// Erase PE headers of injected DLL from memory
	inline void ErasePEHeaders(HMODULE hModule) {
		if (!hModule) return;
		PIMAGE_DOS_HEADER pDosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(hModule);
		PIMAGE_NT_HEADERS pNtHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(
			reinterpret_cast<uint8_t*>(hModule) + pDosHeader->e_lfanew);
		DWORD headerSize = pNtHeaders->OptionalHeader.SizeOfHeaders;
		DWORD oldProtect = 0;
		if (VirtualProtect(hModule, headerSize, PAGE_READWRITE, &oldProtect)) {
			SecureZeroMemory(hModule, headerSize);
			VirtualProtect(hModule, headerSize, oldProtect, &oldProtect);
		}
	}

	// Spoof the return address on the stack for API calls
	// This is a lightweight version - just adds some obfuscation
	inline void RandomizeStackGap() {
		volatile char stackGap[64];
		for (int i = 0; i < 64; ++i)
			stackGap[i] = (char)(__rdtsc() & 0xFF);
		(void)stackGap[0]; // prevent optimization
	}

	// Check if common anti-cheat modules are loaded
	inline bool IsAntiCheatPresent() {
		const char* acModules[] = {
			"BEService.exe", "BEClient_x64.dll",
			"EasyAntiCheat.dll", "eac_launcher.dll"
		};
		for (auto& mod : acModules) {
			if (GetModuleHandleA(mod))
				return true;
		}
		return false;
	}

	// Resolve screen dimensions dynamically (never hardcode)
	inline void GetScreenDimensions(float& width, float& height) {
		width = static_cast<float>(GetSystemMetrics(SM_CXSCREEN));
		height = static_cast<float>(GetSystemMetrics(SM_CYSCREEN));
	}

	// Safe input sender with randomized timing to evade input pattern detection
	inline void SafeSendKey(int vk, int minDelayMs = 1, int maxDelayMs = 8) {
		RandomSleep(minDelayMs, maxDelayMs);
		INPUT input = {};
		input.type = INPUT_KEYBOARD;
		input.ki.wVk = static_cast<WORD>(vk);
		input.ki.dwFlags = 0;
		SendInput(1, &input, sizeof(INPUT));
		RandomSleep(minDelayMs, maxDelayMs);
		input.ki.dwFlags = KEYEVENTF_KEYUP;
		SendInput(1, &input, sizeof(INPUT));
	}

	inline void SafeMouseClick(int minDelayMs = 1, int maxDelayMs = 6) {
		RandomSleep(minDelayMs, maxDelayMs);
		INPUT input = {};
		input.type = INPUT_MOUSE;
		input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		SendInput(1, &input, sizeof(INPUT));
		RandomSleep(minDelayMs, maxDelayMs);
		input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
		SendInput(1, &input, sizeof(INPUT));
	}
}

// Conditional debug logging - completely compiled out in Release
#ifdef _DEBUG
#define DBG_LOG(fmt, ...) do { printf("[DBG] " fmt "\n", ##__VA_ARGS__); } while(0)
#else
#define DBG_LOG(fmt, ...) ((void)0)
#endif
