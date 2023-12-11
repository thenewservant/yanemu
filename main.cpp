#include "ppu.h"
#include "simpleCPU.h"
#include "ScreenTools.h"
#include <thread>

u8 WindowScaleFactor = 4;

int main(int argc, char* argv[]) {
	
#ifdef WIN32
	SetProcessDPIAware();
#endif
	char* gamePath;
	if (argc > 1) {
		gamePath = argv[1];
	}
	else {
		//const char* gameFile = "blargg/spritesOK/08.double_height.nes";
		const char* gameFile = "magicof.nes";
		const char* gameDir = "C:\\Users\\ppd49\\3D Objects\\C++\\yanemu\\tests\\";
		gamePath = (char*)malloc(100 * sizeof(char));
		if (gamePath) {
			gamePath = strcpy(gamePath, gameDir);
			gamePath = strcat(gamePath, gameFile);
		}
		else {
			printf("\nMemory allocation error!");
			exit(1);
		}
	}

	Screen scr(WindowScaleFactor);
	std::thread tsys(mainSYS, &scr, gamePath);

	u8 listn = scr.listener();
	while (!(listn & 1)) {
		SDL_Delay(2);
		listn = scr.listener();
	}
	printf("\nExiting...\n");

	tsys.join();
	scr.endSDLApplication();
	return 0;
}