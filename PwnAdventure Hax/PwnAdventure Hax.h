// This class is exported from the dll
class CPwnAdventureHax { 
public:
	CPwnAdventureHax(void);
	BOOL initHooks();
	void startLoop();
	BOOL cleanupHooks();  
	LPVOID hooker(HMODULE gamelogicModule, const char* exportedFunctionName, LPVOID& hookedAddress); 

	void debugHmodule(HMODULE lib);
	// TODO: add your methods here.
}; 