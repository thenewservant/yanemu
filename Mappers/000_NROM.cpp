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
	switch (where & 0xC000) {
	case 0x8000:
		return prgBanks[0][where];
	case 0xC000:
		return prgBanks[1][where];
	default:
		return ramBank[where - 0x6000];
	}
}

void M_000_NROM::wrCPU(u16 where, u8 what) {
	if (where < 0x8000) {
		ramBank[where-0x6000] = what;
	}
}

u8 M_000_NROM::rdPPU(u16 where) {
	return chrBank[where];
}


void M_000_NROM::wrPPU(u16 where, u8 what) {
	if (!chrRomSize) {
		chrBank[where] = what;
	}
}
