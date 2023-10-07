#include "011_COLOR_DREAMS.h"

void M_011_COLOR_DREAMS::setPrgRom(u8* prgRom, u8 PRsize) {
	prg = prgRom;
	prgRomSize = PRsize;
	currentPrgRom = prgRom;
}

void M_011_COLOR_DREAMS::setChrRom(u8* chrRom, u8 CHRsize) {
	chr = chrRom;
	chrRomSize = CHRsize;
	currentChrRom = chrRom;
}

void M_011_COLOR_DREAMS::wrCPU(u16 where, u8 what) {
	if (where & 0xC000) {
		currentPrgRom = prg + 0x8000 * (what & ((prgRomSize / 2) - 1));
		currentChrRom = chr + 0x2000 * ((what >> 4) & (chrRomSize - 1));
	}
}

void M_011_COLOR_DREAMS::wrPPU(u16 where, u8 what) {}

u8 M_011_COLOR_DREAMS::rdCPU(u16 where) {
	if (where & 0xC000) {
		return currentPrgRom[where - 0x8000];
	}
	else {
		return 0;
	}
}

u8 M_011_COLOR_DREAMS::rdPPU(u16 where) {
	return currentChrRom[where];
}