
/*
#include "console.h"
#pragma warning(disable:4996)

int mainSYS(u32* px) {
	u8* ram = (u8*)calloc((1 << 16), sizeof(u8));

	FILE* testFile = fopen("C:\\Users\\ppd49\\3D Objects\\C++\\yanemu\\tests\\nestest.nes", "rb");

	if (!testFile) {
		printf("\nError: can't open file\n");
		exit(1);
	}
	Rom rom(testFile);

	chrrom = rom.getChrRom();

	Cpu f(ram, ram);
	PPU p(px);

	switch (0) {
	case NTSC:
		while (1) {
			Sleep(1);
			mainClockTickNTSC(f);
		}
		break;
	case PAL:
		while (1) {
			mainClockTickPAL(f);
		}
	default:
		printf("rom ERROR!");
		exit(1);
	}
}

*/