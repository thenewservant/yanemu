#pragma once
#include "Mapper.h"
#include "../Rom.h"
#define CHR_MODE_8KB 0
#define CHR_MODE_4KB 1

#define PRG_MODE_FIXED32_0 0
#define PRG_MODE_FIXED32_1 1 //effectively the same as PRG_ROM_MODE_FIXED32_0
#define PRG_MODE_FIXED16_SWITCH16 2
#define PRG_MODE_SWITCH16_FIXED16 3

#define CHR_MODE_4KB 1
#define CHR_MODE_8KB 0

class M_001_SxROM : public Mapper {
private:
	u8 chr0, chr1;// MMC1 registers. chr and prg are 5-bit wide.
	u8 prgRomSize;
	u8 chrRomSize;
	u8* prgBanks[2];
	u8* chrBanks[2];
	u8 chrRamBanks[2][0x1000];
	u8 prgRamBanks[4][0x2000];
	u8 mirroringMode;
	u8 prgReg;
	u8 prgSizeMode;
	bool chrSizeMode;
	u8* prg;
	u8* chr;
protected:
	void chrUpdate();
public:
	void setPrgRom(u8* prgRom, u8 PRsize);
	void setChrRom(u8* chrRom, u8 CHRsize);
	u8 rdCPU(u16 where);
	void wrCPU(u16 where, u8 what);
	u8 rdPPU(u16 where);
	void wrPPU(u16 where, u8 what);
	M_001_SxROM() : prgBanks{ 0 }, chrBanks{ 0 } {

	};
};