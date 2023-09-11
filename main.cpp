
#include "ppu.h"
#include "simpleCPU.h"
#include "ScreenTools.h"
#include <Windows.h>
u8 WindowScaleFactor = 4;

#define DEBUG

int main(int argc, char* argv[]) {

#ifndef DEBUG
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

#else

    //const char* gameFile = "blargg\\sprite\\01.basics.nes";
	const char* gameFile = "\\oc.nes";
	const char *gameDir = "C:\\Users\\ppd49\\3D Objects\\C++\\yanemu\\tests\\";
	char* gamePath=(char*)malloc(100 *sizeof(char));
	gamePath = strcpy(gamePath, gameDir);
	gamePath = strcat(gamePath, gameFile);
	FILE* testFile = fopen(gamePath, "rb");
#endif
	SetProcessDPIAware();
	Screen scr(WindowScaleFactor, "Game");
	std::thread tsys(mainSYS, scr, testFile);

	while (!scr.listener()) {

	}
	printf("\nExiting...\n");
	
	tsys.join();
	scr.endSDLApplication();
	return 0;
}
