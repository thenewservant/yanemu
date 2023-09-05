#include "000_NROM.h"


void M_000_NROM::setPrgRom(u8* prgRom, u8 PRsize) {
	prgBanks[0] = prgRom - 0x8000;

	switch (PRsize) {
	case 1:
		prgBanks[1] = prgRom - 0xC000;
		break;
	case 2:
		prgBanks[1] = prgRom + 0x4000 - 0xC000;
		break;
	default:
		printf("\nError: PRG ROM size not supported\n");
		exit(1);
	}
}

void M_000_NROM::setChrRom(u8* chrRom, u8 CHRsize) {
	chrRomSize = CHRsize;
	if (CHRsize) {
		chrBank = chrRom;
	}
	else {
		chrBank = (u8*)calloc(0x2000, sizeof(u8)); //8KB CHR RAM
	}
}

u8 M_000_NROM::rdCPU(u16 where) {
	if (where < 0xC000) {
		return prgBanks[0][where];
	}
	else {
		return prgBanks[1][where];
	}
}

void M_000_NROM::wrCPU(u16 where, u8 what) {
	return;
}

u8 M_000_NROM::rdPPU(u16 where) {
	return chrBank[where];
}


void M_000_NROM::wrPPU(u16 where, u8 what) {
	chrBank[where] = what;
}