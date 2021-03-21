// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "PwnAdventure Hax.h"
#include "dllmain.h"

void createConsole()
{
	FILE* fDummy;

	if (!AllocConsole()) {
		// Add some error handling here.
		// You can call GetLastError() to get more info about the error.
		Beep(10000, 2000);
		return;
	}

	// std::cout, std::clog, std::cerr, std::cin

	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);
	freopen_s(&fDummy, "CONIN$", "r", stdin);
	std::cout.clear();
	std::clog.clear();
	std::cerr.clear();
	std::cin.clear();

	// std::wcout, std::wclog, std::wcerr, std::wcin
	HANDLE hConOut = CreateFile(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	HANDLE hConIn = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
	SetStdHandle(STD_ERROR_HANDLE, hConOut);
	SetStdHandle(STD_INPUT_HANDLE, hConIn);
	std::wcout.clear();
	std::wclog.clear();
	std::wcerr.clear();
	std::wcin.clear();;
}

DWORD WINAPI haxThread(HMODULE hmodule) {
	createConsole();
	std::cout << "Successfully injected DLL" << std::endl;

	CPwnAdventureHax hax;
	BOOL cleanInit = hax.initHooks();

	if (!cleanInit) {
		std::cout << "Failed to initialize hooks, bailing out ungracefully." << std::endl;
		return 0;
	}

	hax.startLoop(); // infinite loop awaiting commands from the user. RCONTROL to quit the loop
	BOOL cleanExit = hax.cleanupHooks();

	std::cout << "Detaching from process" << std::endl;


	// Don't attempt to free the library and exit the thread if we didn't get a clean detach from detours, will probably segfault the process or bluescreen
	if (cleanExit) {
		std::cout << "Got a clean DetourDetach, exiting gracefully." << std::endl;
		FreeLibraryAndExitThread(hmodule, 0); // IMPORTANT - kills the worker thread cleanly, so DLL can be reinjected
	}
	else {
		std::cout << "Ungraceful exit, don't attempt to re-inject the DLL into the process without restarting the process first." << std::endl;
	}

	std::cout << "(don't close this console window, process will crash)" << std::endl;
	FreeConsole();
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)haxThread, hModule, 0, nullptr));
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

