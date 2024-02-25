#include "007_AxROM.h"

void M_007_AxROM::setPrgRom(u8* prgRom, u8 PRsize) {
	prg = prgRom;
	prgRomSize = PRsize;
	prgBanks[0] = prgRom + (((prgRomSize / 2) - 1)) * 0x8000;
}

void M_007_AxROM::setChrRom(u8* chrRom, u8 CHRsize) {
	chrRomSize = CHRsize;
	chrBanks[0] = (u8*)calloc(0x2000, sizeof(u8));
}

void M_007_AxROM::wrCPU(u16 where, u8 what) {
	if (where & 0xC000) {
		prgBanks[0] = prg + (what & ((prgRomSize/2)-1)) * 0x8000;
		mirror = (what & 0x10) ? SINGLE_SCREEN_1 : SINGLE_SCREEN_0;
	}
}

u8 M_007_AxROM::rdCPU(u16 where) {
	return prgBanks[0][where - 0x8000];
}

u8 M_007_AxROM::rdPPU(u16 where) {
	return chrBanks[0][where];
}

void M_007_AxROM::wrPPU(u16 where, u8 what) {
	chrBanks[0][where] = what;
}

void M_007_AxROM::wrNT(u16 where, u8 what) {
	if (mirror == SINGLE_SCREEN_0) {
		nameTables[0][where & 0x3FF] = what;
	}
	else {
		nameTables[1][where & 0x3FF] = what;
	}
}

u8 M_007_AxROM::rdNT(u16 where) {
	if (mirror == SINGLE_SCREEN_0) {
		return nameTables[0][where & 0x3FF];
	}
	else {
		return nameTables[1][where & 0x3FF];
	}
}