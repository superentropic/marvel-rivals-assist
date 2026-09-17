#pragma once
#include <xlocbuf>
#include <codecvt>

namespace ScreenInfo {
	inline double Width() { return static_cast<double>(GetSystemMetrics(SM_CXSCREEN)); }
	inline double Height() { return static_cast<double>(GetSystemMetrics(SM_CYSCREEN)); }
	inline double CenterX() { return Width() / 2.0; }
	inline double CenterY() { return Height() / 2.0; }
}

bool InScreen(SDK::FVector2D ScreenLocation)
{
	if (ScreenLocation.X < 5.0 || ScreenLocation.X > ScreenInfo::Width() - (5.0 * 2) && ScreenLocation.Y < 5.0 || ScreenLocation.Y > ScreenInfo::Height() - (5.0 * 2))
		return false;
	return true;
}

bool InRect(double Radius, SDK::FVector2D ScreenLocation)
{
	double cx = ScreenInfo::CenterX();
	return cx >= (cx - Radius) && cx <= (cx + Radius) &&
		ScreenLocation.Y >= (ScreenLocation.Y - Radius) && ScreenLocation.Y <= (ScreenLocation.Y + Radius);
}

bool InCircle(double Radius, SDK::FVector2D ScreenLocation)
{
	if (InRect(Radius, ScreenLocation))
	{
		double dx = ScreenInfo::CenterX() - ScreenLocation.X; dx *= dx;
		double dy = ScreenInfo::CenterY() - ScreenLocation.Y; dy *= dy;
		return dx + dy <= Radius * Radius;
	} return false;
}


void* PatternScan(const char* szModule, const char* szSignature) {
    static auto PatternToByte = [](const char* pattern) {
        auto arrBytes = std::vector<int>{};
        auto pStart = const_cast<char*>(pattern);
        auto pEnd = const_cast<char*>(pattern) + strlen(pattern);

        for (char* pCurrent = pStart; pCurrent < pEnd; ++pCurrent) {
            if (*pCurrent == '?') {
                ++pCurrent;
                if (*pCurrent == '?')
                    ++pCurrent;
                arrBytes.push_back(-1);
            }
            else {
                arrBytes.push_back(strtoul(pCurrent, &pCurrent, 16));
            }
        }
        return arrBytes;
        };

    void* pModule = (void*)GetModuleHandleA(szModule);
    PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)pModule;
    PIMAGE_NT_HEADERS NTHeaders = (PIMAGE_NT_HEADERS)((std::uint8_t*)pModule + DosHeader->e_lfanew);

    DWORD uSizeOfImage = NTHeaders->OptionalHeader.SizeOfImage;
    std::vector<int> arrPatternBytes = PatternToByte(szSignature);
    std::uint8_t* pScanBytes = reinterpret_cast<std::uint8_t*>(pModule);

    size_t uPatternBytesSize = arrPatternBytes.size();
    int* pPatternBytesData = arrPatternBytes.data();

    for (auto i = 0ul; i < uSizeOfImage - uPatternBytesSize; ++i) {
        bool bFound = true;
        for (auto j = 0ul; j < uPatternBytesSize; ++j) {
            if (pScanBytes[i + j] != pPatternBytesData[j] && pPatternBytesData[j] != -1) {
                bFound = false;
                break;
            }
        }
        if (bFound) {
            return &pScanBytes[i];
        }
    }
    return nullptr;
}