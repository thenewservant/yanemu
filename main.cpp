#include "types.h"
#include "ppu.h"
#include "simpleCPU.h"
#include "ScreenTools.h"
#include <thread>

u8 WindowScaleFactor = 3;

int main(int argc, char* argv[]) {
	#ifdef WIN32
		SetProcessDPIAware();
	#endif

	char* gamePath;
	if (argc > 1) {
		gamePath = argv[1];
	}
	else {
		printf("Usage: yanemu.exe <rom>\n");
		return 1;
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