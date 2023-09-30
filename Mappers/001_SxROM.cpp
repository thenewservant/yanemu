#include "001_SxROM.h"

void M_001_SxROM::wrCPU(u16 where, u8 what) {
	static u8 shiftReg = 0;
	static u8 shiftedBits = 0; // how many times we have written to shift register
	static u8 mirrorMode;

	if ((where < 0x8000)) {
		prgRamBanks[0][where - 0x6000] = what;
	}
	else {
		if (what & 0x80) {
			//last bank is attached to CPU $C000-$FFFF
			shiftReg = 0;
			shiftedBits = 0;
			prgBanks[0] = prg + 0x4000 * ((prgRomSize - 1) & 0xE);
			prgBanks[1] = prgBanks[0] + 0x4000;
		}
		else {
			shiftReg = ((shiftReg >> 1) | ((what & 1) << 4)) & 0b11111;
			shiftedBits++;
			if (shiftedBits == 5) {
				shiftedBits = 0;

				switch (where & 0xE000) {
				case 0x8000: //ctrl
					mirror = mmc1MappingEqu[shiftReg & 3];
					chrSizeMode = (shiftReg & 0x10) > 0;
					prgSizeMode = (shiftReg >> 2) & 3;
					break;
				case 0xA000: //chr0
					chr0 = shiftReg & (2 * chrRomSize - 1);
					chrUpdate();
					break;
				case 0xC000: //chr1
					chr1 = shiftReg & (2 * chrRomSize - 1);
					chrUpdate();
					break;
				case 0xE000: //prg
					wRamEnable = !(shiftReg & 0x10);
					switch (prgSizeMode) {
					case PRG_MODE_FIXED32_0:
					case PRG_MODE_FIXED32_1:
						prgBanks[0] = prg + 0x4000 * (shiftReg & (prgRomSize - 1) & 0xE);
						prgBanks[1] = prgBanks[0] + 0x4000;
						break;
					case PRG_MODE_FIXED16_SWITCH16:
						prgBanks[0] = prg;
						prgBanks[1] = prg + 0x4000 * (shiftReg & (prgRomSize - 1));
						break;
					case PRG_MODE_SWITCH16_FIXED16:
						prgBanks[0] = prg + 0x4000 * (shiftReg & (prgRomSize - 1));
						prgBanks[1] = prg + 0x4000 * (prgRomSize - 1);
						break;
					}
					break;
				default:
					break;
				}
				shiftReg = 0;
			}
		}
	}
}

void M_001_SxROM::chrUpdate() {
	switch (chrSizeMode) {
	case CHR_MODE_4KB:
		if (chrRomSize) {
			chrBanks[0] = chr + 0x1000 * (chr0);
			chrBanks[1] = chr + 0x1000 * (chr1);
		}
		else {
			chrBanks[0] = chrRamBanks[chr0 & 1];
			chrBanks[1] = chrRamBanks[chr1 & 1];
		}
		break;
	case CHR_MODE_8KB:
		if (chrRomSize) {
			chrBanks[0] = chr + 0x1000 * (chr0);
			chrBanks[1] = chr + 0x1000 * (chr0 + 1);
		}
		else {
			chrBanks[0] = chrRamBanks[chr0 & 1];
			chrBanks[1] = chrRamBanks[1 + (chr0 & 1)];
		}
		break;
	}
}

u8 M_001_SxROM::rdPPU(u16 where) {
	if (where < 0x1000) {
		return chrBanks[0][where];
	}
	else {
		return chrBanks[1][where - 0x1000];
	}
}

void M_001_SxROM::wrPPU(u16 where, u8 what) {
	if (where < 0x1000) {
		chrBanks[0][where] = what;
	}
	else {
		chrBanks[1][where - 0x1000] = what;
	}
}

u8 M_001_SxROM::rdCPU(u16 where) {
	if ((where < 0x8000)) {
		if (wRamEnable) {
			return prgRamBanks[0][where - 0x6000];
		}
		else {
			return 0;
		}
	}
	else if (where < 0xC000) {

		return prgBanks[0][where - 0x8000];
	}
	else {
		return prgBanks[1][where - 0xC000];
	}
}

void M_001_SxROM::setPrgRom(u8* prgRom, u8 PRsize) {
	prgRomSize = PRsize;
	prg = prgRom;
	prgSizeMode = PRG_MODE_SWITCH16_FIXED16;

	prgBanks[0] = prgRom;
	prgBanks[1] = prgRom + 0x4000 * (prgRomSize - 1);
}

void M_001_SxROM::setChrRom(u8* chrRom, u8 CHRsize) {
	chrRomSize = CHRsize;
	chr = chrRom;
	if (CHRsize) {
		chr0 = 0;
		chrSizeMode = CHR_MODE_8KB;
		chrUpdate();
	}
	else {
		chrBanks[0] = chrRamBanks[0];
		chrBanks[1] = chrRamBanks[1];
	}
}

void M_001_SxROM::wrNT(u16 where, u8 what) {
	u8 ntID = (where >> 10) & 3;
	switch (mirror) {
	case HORIZONTAL:
		nameTables[(ntID >> 1)][where & 0x03FF] = what;
		break;
	case VERTICAL:
		nameTables[(ntID & 1)][where & 0x03FF] = what;
		break;
	case SINGLE_SCREEN_0:
		nameTables[0][where & 0x03FF] = what;
		break;
	case SINGLE_SCREEN_1:
		nameTables[1][where & 0x03FF] = what;
		break;
	default:
		break;
	}
}

u8 M_001_SxROM::rdNT(u16 where) {
	u8 ntID = (where >> 10) & 3;
	switch (mirror) {
	case HORIZONTAL:
		return nameTables[(ntID >> 1)][where & 0x03FF];
		break;
	case VERTICAL:
		return nameTables[(ntID & 1)][where & 0x03FF];
		break;
	case SINGLE_SCREEN_0:
		return nameTables[0][where & 0x03FF];
		break;
	case SINGLE_SCREEN_1:
		return nameTables[1][where & 0x03FF];
		break;
	default:
		break;
	}
	return 0;
}

void M_001_SxROM::setMirroring(u8 mir) {
	mirror = SINGLE_SCREEN_1;
}