
#include "ppu.h"
#include "simpleCPU.h"
#include "ScreenTools.h"
#include <thread>

u8 WindowScaleFactor = 4;

#define MANUAL
 

int main(int argc, char* argv[]) {
#ifdef WIN32
	SetProcessDPIAware();
#endif
#ifdef CLI
	FreeConsole();
	if (argc < 2) {
		printf("\nnes ROM expected!");
		exit(1);
	}
	FILE* testFile = fopen(argv[1], "rb");

	if (!testFile) {
		printf("\nError: can't open file\n");
		exit(1);
	}
#endif
#ifdef MANUAL
	//const char* gameFile = "blargg\\pputiming\\06-suppression.nes";
	const char* gameFile = "castlevania2.nes";
	const char* gameDir = "C:\\Users\\ppd49\\3D Objects\\C++\\yanemu\\tests\\";
	char* gamePath=(char*)malloc(100 *sizeof(char));
	gamePath = strcpy(gamePath, gameDir);
	gamePath = strcat(gamePath, gameFile);
	FILE* droppedFile = fopen(gamePath, "rb");
	Screen scr(WindowScaleFactor, "Game");
#endif:
#ifdef DROP

	SetProcessDPIAware();
	Screen scr(WindowScaleFactor, "Game");
	while (!scr.listener()) {
		SDL_Delay(1);
	}
	FILE* droppedFile = fopen(scr.getFilePath(), "rb");

#endif

	std::thread tsys(mainSYS, scr, droppedFile);

	u8 listn = scr.listener();
	while (!(listn & 1)) {
		SDL_Delay(1);
		listn = scr.listener();
	}
	printf("\nExiting...\n");
	
	tsys.join();
	scr.endSDLApplication();
	return 0;
}