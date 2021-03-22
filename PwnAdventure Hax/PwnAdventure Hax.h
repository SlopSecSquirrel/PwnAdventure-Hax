// This class is exported from the dll
class CPwnAdventureHax { 
public:
	BOOL fastModeOn = false;
	BOOL jumpModeOn = false;
	BOOL holdModeOn = false;
	std::string command = ""; // The most recent command that the user has typed into the chat box.
	CPwnAdventureHax(void);
	BOOL initHooks();
	void startLoop();
	BOOL cleanupHooks();  
	void processCommand(std::string);
	LPVOID hooker(HMODULE gamelogicModule, const char* exportedFunctionName, LPVOID& hookedAddress); 

	void debugHmodule(HMODULE lib);
	// TODO: add your methods here.
}; 