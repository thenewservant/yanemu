#include "ppu.h"
#include "simpleCPU.hpp"
#include "ScreenTools.h"

u8 WindowScaleFactor = 3;

int main(int argc, char* argv[]) {
	
	//Sleep(500);
	Screen scr(3);
	std::thread tsys(mainSYS, scr);

	while (1) {
		//scr.updateScreen();
		//printf("\n\nECRAN:%2X\n\n", scr.getPixelsPointer()[700]);
		scr.listener();
	}
	tsys.join();
	
	//do stuff
	
	//Sleep(3000);

	//
	return 0;
}

