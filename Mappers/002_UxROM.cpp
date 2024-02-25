#include "002_UxROM.h"

void M_002_UxROM::setPrgRom(u8* prgRom, u8 PRsize) {
	prg = prgRom;
	prgRomSize = PRsize;
	switchablePrgBank = prgRom;
	lastPrgBank = prgRom + 0x4000 * (PRsize - 1); // fixed to last bank
}

void M_002_UxROM::setChrRom(u8* chrRom, u8 CHRsize) {
	chrRomSize = CHRsize;
}

u8 M_002_UxROM::rdCPU(u16 where) {
	if (where < 0xC000) {
		return switchablePrgBank[where - 0x8000];
	}
	return lastPrgBank[where - 0xC000];
}

void M_002_UxROM::wrCPU(u16 where, u8 what) {
	if (where & 0x8000) {
		switchablePrgBank = prg + 0x4000 * (what & (prgRomSize -1));
	}
}

u8 M_002_UxROM::rdPPU(u16 where) {
	return chrRom[where];
}

void M_002_UxROM::wrPPU(u16 where, u8 what) {
	chrRom[where] = what;
}