#ifndef _001_SxROM_H_
#define _001_SxROM_H_
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

constexpr u8 mmc1MappingEqu[4] = { SINGLE_SCREEN_0, SINGLE_SCREEN_1, VERTICAL, HORIZONTAL };

class M_001_SxROM : public Mapper {
protected:
	u8 chr0, chr1;// MMC1 registers. chr and prg are 5-bit wide.
	u8 chrRamBanks[2][0x1000];
	u8 prgRamBanks[4][0x2000];
	u8 prgSizeMode;
	bool chrSizeMode;
	bool wRamEnable;
	u8* chr;
protected:
	void chrUpdate();
	void setMirroring(u8 mir);
public:
	void setPrgRom(u8* prgRom, u8 PRsize);
	void setChrRom(u8* chrRom, u8 CHRsize);
	u8 rdCPU(u16 where);
	void wrCPU(u16 where, u8 what);
	u8 rdPPU(u16 where);
	void wrPPU(u16 where, u8 what);
	void wrNT(u16 where, u8 what);
	u8 rdNT(u16 where);
	u8* getPrgRam() { return prgRamBanks[0]; };
	void setPrgRam(u8* ramPtr) {
		memcpy(prgRamBanks[0], ramPtr, 0x2000);
	};
	M_001_SxROM() :Mapper(), chr0{ 0 }, chr1{ 0 }, chr{ 0 }, chrSizeMode{ 0 },
		wRamEnable{ 0 }, prgSizeMode{ 0 }, prgRamBanks{ 0 }, chrRamBanks{ 0 }
	{
		prgBanks = new u8 * [2];
		chrBanks = new u8 * [2];
		mirror = SINGLE_SCREEN_0;
		wRamEnable = true;
	};
};
#endif