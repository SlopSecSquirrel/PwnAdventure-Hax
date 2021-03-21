// PwnAdventure Hax.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"
#include "framework.h"
#include "PwnAdventure Hax.h"

// Generic static helper vars 
static std::string command; // The most recent command that the user has typed into the chat box.
static std::set<std::string> validCommands = { "quit" }; // A list of commands that are acceptable.

// Hook defs. It's GROSS that these variables need to be declared outside of the class definition, but the function pointers all break if the hooked* functions are class members.

// GetTotalPlayerCount hook, hooked this because it was useful for debugging (doesn't take any arguments, returns an integer)
const char* GetTotalPlayerCountExportName = "?GetTotalPlayerCount@GameAPI@@QAEHXZ";
typedef int(__thiscall* RealGetTotalPlayerCount)(void*);		// MUST BE __THISCALL!!! matches the original prototype in the DLL, void* arg is 'this' keyword (because __thiscall)
RealGetTotalPlayerCount realGetTotalPlayerCount;				// typedef'd function pointer to the actual GetTotalPlayerCount func.
LPVOID realGetTotalPlayerCountAddr;								// The address returned by calling GetProcAddress on the GameLogic DLL.
int  __fastcall HookedGetTotalPlayerCount(void* This, void* _);	// Prototype for our detoured method, don't ask why it needs two void pointers. __thiscall fluff

// Chat hook, hooked this because we can type into the in-game chat box and give commands for the DLL to process like 'teleport x,y,z'
const char* ChatExportName = "?Chat@Player@@UAEXPBD@Z"; 
typedef void (__thiscall* RealChat)(void*, char const*);		// Again, first void* is 'this' variable in __thiscall calling convention
RealChat realChat;
LPVOID realChatAddr;
void __fastcall HookedChat(void*, void* _, char const*);		// yes I hate the void*, void*, char const* too, __thiscall fluff again.


// This is the constructor of a class that has been exported.
CPwnAdventureHax::CPwnAdventureHax()
{
    return;
}


BOOL CPwnAdventureHax::initHooks()
{
	HMODULE gamelogicModule = GetModuleHandleA("GameLogic.dll");
	DWORD getLastErr = GetLastError();
	std::cout << "gamelogicModule addr = " << std::hex << gamelogicModule << std::dec << std::endl;
	std::cout << "GetLastError = " << getLastErr << std::dec << std::endl;

	if (gamelogicModule == 0 || getLastErr != 0) {
		std::cout << "Bailing out." << std::endl;
		FreeConsole();
		return false;
	}
	//debugHmodule(gamelogicModule);

	// GetTotalPlayerCount hook setup
	auto addrGetTotalPlayerCount = &HookedGetTotalPlayerCount;
	auto ret = hooker(gamelogicModule, GetTotalPlayerCountExportName, (LPVOID&)addrGetTotalPlayerCount);
	if (ret == nullptr) {
		return false;
	}
	realGetTotalPlayerCount = (RealGetTotalPlayerCount)ret;
	
	// Player::Chat hook setup
	auto addrChat = &HookedChat;
	ret = hooker(gamelogicModule, ChatExportName, (LPVOID&)addrChat);
	if (ret == nullptr) {
		return false;
	}
	realChat = (RealChat)ret;

	return true;
}

LPVOID CPwnAdventureHax::hooker(HMODULE gamelogicModule, const char* exportedFunctionName, LPVOID& hookedAddress) {
	LPVOID realFunctionAddress = DetourFindFunction("GameLogic.dll", exportedFunctionName);
	std::cout << exportedFunctionName << "addr before hooks " << std::hex << realFunctionAddress << std::dec << std::endl;
	DetourUpdateThread(GetCurrentThread());
	DetourTransactionBegin();
	 
	DetourAttach(&realFunctionAddress, hookedAddress);

	// Commit transaction and Check for error
	int error = DetourTransactionCommit();
	std::cout << exportedFunctionName << "addr after hooks " << std::hex << realFunctionAddress << std::dec << std::endl;

	if (error != NO_ERROR)
	{
		std::cout << exportedFunctionName << " detoured unsuccessfully. Error is - " << error << std::endl;
		return nullptr;
	}
	else {
		std::cout << exportedFunctionName << "detoured successfully." << std::endl;;
		return realFunctionAddress;
	}

}
 
// infinite loop awaiting commands from the user. RCONTROL to quit the loop
void CPwnAdventureHax::startLoop()
{
	while (1) {
		if ((GetAsyncKeyState(VK_RCONTROL) & 0x01) || command == "quit")
		{
			std::wcout << L"RCONTROL pressed, attempting to exit gracefully (plz no bluescreen)" << std::endl;;
			break;
		}

		// Clear the command var after every loop iteration.
		command = "";
	}
}

// Need to detach our hooks after we detach our DLL or things will break
BOOL CPwnAdventureHax::cleanupHooks() {
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	auto addr = &HookedGetTotalPlayerCount;
	DetourDetach(&(LPVOID&)realGetTotalPlayerCount, (LPVOID&) addr);

	auto addr2 = &HookedChat;
	DetourDetach(&(LPVOID&)realChat, (LPVOID&)addr2);
	LONG error = DetourTransactionCommit();
	std::cout << "DetachDetour error code was " << error << std::endl;

	return error == NO_ERROR;
}


// HOOK CODE HOOK CODE HOOK CODE HOOK CODE \\
// Ignore the second param, some kind of __thiscall weirdness
int  __fastcall HookedGetTotalPlayerCount(void* This, void* _)
{
	// Game will have a VERY SAD TIME if the This pointer is null. :D 
	if (This == 0) {
		return 1;
	}

	// Get the actual result by calling the original function
	int count = realGetTotalPlayerCount(This); 

	std::cout << "Inside of HookedGetTotalPlayerCount, count is " << count << std::endl;
	std::cout.flush();
	return count;
} 

void __fastcall HookedChat(void* This, void* _, char const* message) {
	
	command = std::string(message);
	std::cout << "Inside of HookedChat, message is " << message << std::endl;
	// Don't actually broadcast the chat message if it's a valid command. stealthy, innit
	if (validCommands.find(command) != validCommands.end()) {
		return;
	}

	realChat(This, message);
}


// DEBUG CODE DEBUG CODE DEBUG CODE DEBUG CODE \\
// weird hooking debug code, gratuitously robbed from https://stackoverflow.com/questions/1128150/win32-api-to-enumerate-dll-export-functions
void CPwnAdventureHax::debugHmodule(HMODULE lib) {
	assert(((PIMAGE_DOS_HEADER)lib)->e_magic == IMAGE_DOS_SIGNATURE);
	PIMAGE_NT_HEADERS header = (PIMAGE_NT_HEADERS)((BYTE*)lib + ((PIMAGE_DOS_HEADER)lib)->e_lfanew);
	assert(header->Signature == IMAGE_NT_SIGNATURE);
	assert(header->OptionalHeader.NumberOfRvaAndSizes > 0);
	PIMAGE_EXPORT_DIRECTORY exports = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)lib + header->
		OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
	assert(exports->AddressOfNames != 0);
	BYTE** names = (BYTE**)((int)lib + exports->AddressOfNames);
	for (unsigned int i = 0; i < exports->NumberOfNames; i++) {
		char* temp = (char*)lib + (int)names[i];

		printf("Export: \"%s\"\n", temp);
		LPVOID procAddress = (LPVOID)GetProcAddress(lib, temp);
		std::cout << "Name: \"" << temp << "\" and proc address is " << std::hex << procAddress << std::dec << std::endl;
	}
}

