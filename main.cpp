
#include "ppu.h"
#include "simpleCPU.hpp"
#include "ScreenTools.h"
#include <Windows.h>
u8 WindowScaleFactor = 3;

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

	//Sleep(500);
	

#else
	FILE* testFile = fopen("C:\\Users\\ppd49\\3D Objects\\C++\\yanemu\\tests\\smb.nes", "rb");
#endif
	


	Screen scr(WindowScaleFactor);
	std::thread tsys(mainSYS, scr, testFile);
	while (1 && !scr.listener()) {
		//scr.updateScreen();
		//printf("\n\nECRAN:%2X\n\n", scr.getPixelsPointer()[700]);

	}

	scr.endSDLApplication();
	tsys.join();
	//do stuff

	//Sleep(3000);

	//
	return 0;
}

