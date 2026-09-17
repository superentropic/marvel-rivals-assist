#include "injector.hpp"
#include <dwmapi.h>
#include <conio.h>
#include <atomic>
#pragma comment(lib, "dwmapi.lib")

// ============================================================
//  Console color helpers (matches Fortnite loader style)
// ============================================================
namespace con {
	static HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

	enum Color : WORD {
		DARK    = 0x08,
		WHITE   = 0x0F,
		CYAN    = 0x0B,
		GREEN   = 0x0A,
		RED     = 0x0C,
		YELLOW  = 0x0E,
		MAGENTA = 0x0D,
		BLUE    = 0x09,
		GRAY    = 0x07,
	};

	inline void set(WORD c) { SetConsoleTextAttribute(hOut, c); }
	inline void reset()     { set(GRAY); }

	inline void print(WORD c, const char* msg) { set(c); printf("%s", msg); reset(); }
	inline void println(WORD c, const char* msg) { set(c); printf("%s\n", msg); reset(); }

	inline void print_center(WORD c, const char* text, int width = 64) {
		int len = (int)strlen(text);
		int pad = (width - len) / 2;
		if (pad < 0) pad = 0;
		set(c);
		for (int i = 0; i < pad; i++) putchar(' ');
		printf("%s\n", text);
		reset();
	}

	inline void line(WORD c, int width = 60) {
		set(c);
		printf("  ");
		for (int i = 0; i < width; i++) putchar('-');
		printf("\n");
		reset();
	}

	inline void clear() {
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(hOut, &csbi);
		DWORD cells = csbi.dwSize.X * csbi.dwSize.Y;
		COORD home = { 0, 0 };
		DWORD written;
		FillConsoleOutputCharacterA(hOut, ' ', cells, home, &written);
		FillConsoleOutputAttribute(hOut, csbi.wAttributes, cells, home, &written);
		SetConsoleCursorPosition(hOut, home);
	}
}

// ============================================================
//  Terminal configuration (dark mode, font, transparency, etc.)
// ============================================================
void configure_terminal() {
	HWND hConsole = GetConsoleWindow();
	if (!hConsole) return;

	SetConsoleOutputCP(CP_UTF8);

	RECT r;
	GetWindowRect(hConsole, &r);
	MoveWindow(hConsole, r.left, r.top, 740, 500, TRUE);

	CONSOLE_FONT_INFOEX cfi = {};
	cfi.cbSize = sizeof(cfi);
	cfi.nFont = 0;
	cfi.dwFontSize.X = 0;
	cfi.dwFontSize.Y = 16;
	cfi.FontFamily = FF_DONTCARE;
	cfi.FontWeight = FW_NORMAL;
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
	COORD bufSize;
	bufSize.X = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	bufSize.Y = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	SetConsoleScreenBufferSize(hOut, bufSize);
}

// ============================================================
//  Randomized window title (changes constantly in background)
// ============================================================
static std::atomic<bool> g_title_running{ true };

static std::string random_title(int len) {
	const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	std::string name(len, 0);
	ULONG seed = (ULONG)(__rdtsc() ^ GetTickCount64());
	for (int i = 0; i < len; i++) {
		seed = seed * 1103515245 + 12345;
		name[i] = charset[(seed >> 16) % (sizeof(charset) - 1)];
	}
	return name;
}

static DWORD WINAPI title_randomizer_thread(LPVOID) {
	while (g_title_running.load()) {
		std::string title = random_title(16);
		SetConsoleTitleA(title.c_str());
		Sleep(1500);
	}
	return 0;
}

// ============================================================
//  Styled output helpers
// ============================================================
void setup_header() {
	con::clear();
	printf("\n\n");
	con::print_center(con::WHITE, "Marvel Rivals - ABYSS");
	con::print_center(con::DARK, "~ Loading ~");
	printf("\n");
	con::line(con::DARK);
	printf("\n");
}

void setup_step(const char* msg) {
	con::set(con::WHITE);
	printf("    %s", msg);
	con::reset();
}

void setup_done() {
	con::set(con::DARK);
	printf("  done\n");
	con::reset();
}

void setup_fail(const char* msg) {
	con::set(con::RED);
	printf("  FAILED");
	if (msg && msg[0]) printf(" (%s)", msg);
	printf("\n");
	con::reset();
}

void setup_info(const char* msg) {
	con::set(con::DARK);
	printf("    %s\n", msg);
	con::reset();
}

void setup_line() {
	printf("\n");
	con::line(con::DARK);
	printf("\n");
}

// ============================================================
//  Main
// ============================================================
std::int32_t main() {

	// Allocate console for release builds
	if (!GetConsoleWindow()) {
		AllocConsole();
		FILE* f;
		freopen_s(&f, "CONOUT$", "w", stdout);
		freopen_s(&f, "CONOUT$", "w", stderr);
		freopen_s(&f, "CONIN$", "r", stdin);
	}

	// Start randomized title thread
	HANDLE hTitleThread = CreateThread(NULL, 0, title_randomizer_thread, NULL, 0, NULL);
	if (hTitleThread) CloseHandle(hTitleThread);

	// Configure terminal appearance
	configure_terminal();

	// Show branded header
	setup_header();

	// Phase 1: Anti-debug
	setup_step("Checking environment...");
	anti_debug::apply_protections();

	if (anti_debug::run_all_checks()) {
		setup_fail("hostile environment");
		setup_info("close debugging tools and retry");
		printf("\n");
		setup_info("press any key to exit...");
		_getch();
		g_title_running = false;
		return 1;
	}
	setup_done();

	// Phase 2: Resolve DLL
	setup_step("Resolving payload...");
	wchar_t exe_path[MAX_PATH] {};
	GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
	std::wstring dll_path(exe_path);
	auto last_slash = dll_path.find_last_of(L'\\');
	if (last_slash != std::wstring::npos)
		dll_path = dll_path.substr(0, last_slash + 1);
	dll_path += xsw(L"dll.dll");

	auto target_file = utils::read_file_by_name(dll_path.c_str());
	if (!target_file) {
		setup_fail("payload not found");
		setup_info("make sure dll.dll is in the same folder");
		printf("\n");
		setup_info("press any key to exit...");
		_getch();
		g_title_running = false;
		return 1;
	}
	setup_done();

	// Phase 3: Inject
	setup_step("Injecting...");
	injector::c_inject inj;
	if (!inj.inject_into_game(target_file)) {
		setup_fail("injection");
		printf("\n");
		setup_info("make sure the game is running");
		setup_info("press any key to exit...");
		_getch();
		g_title_running = false;
		return 1;
	}
	setup_done();

	// Phase 4: Cleanup
	setup_step("Cleaning up...");
	volatile wchar_t* p = &dll_path[0];
	for (size_t i = 0; i < dll_path.size(); ++i)
		p[i] = 0;
	setup_done();

	// Done
	setup_line();

	con::set(con::GREEN);
	printf("    Injection successful\n\n");
	con::reset();

	setup_info("you can close this window");
	setup_info("press any key to exit...");
	printf("\n");

	_getch();
	g_title_running = false;
	Sleep(100);

	return 0;
}