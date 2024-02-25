#include "003_CNROM.h"
#include "000_NROM.h"

void M_003_CNROM::setPrgRom(u8* prgRom, u8 PRsize) {
	prgBanks[0] = prgRom;
	switch (PRsize) {
	case 1:
		prgBanks[1] = prgRom;
		break;
	case 2:
		prgBanks[1] = prgRom + 0x4000;
		break;
	default:
		printf("\nError: PRG ROM size not supported\n");
		exit(1);
	}
}

void M_003_CNROM::setChrRom(u8* chrRom, u8 CHRsize) {
	chrBank = chrRom;
	chrRomSize = CHRsize;
	if (!CHRsize) {
		chrBank = (u8*)calloc(0x2000, sizeof(u8)); //8KB CHR RAM
	}
	currentChrRom = chrBank;
}

u8 M_003_CNROM::rdCPU(u16 where) {
	if (where < 0xC000) {
		return prgBanks[0][where - 0x8000];
	}
	else {
		return prgBanks[1][where - 0xC000];
	}
}

void M_003_CNROM::wrCPU(u16 where, u8 what) {
	if (where & 0x8000) {
		currentChrRom = chrBank + (what & (chrRomSize - 1)) * 0x2000;
	}
}

u8 M_003_CNROM::rdPPU(u16 where) {
	return currentChrRom[where];
}

void M_003_CNROM::wrPPU(u16 where, u8 what) {
	currentChrRom[where] = what;
}