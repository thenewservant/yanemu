#pragma once
#include "Mapper.h"

class M_000_NROM : public Mapper{
private:
	u8* prg;
	u8** prgBanks;
	u8* chrBank;
	u8 prgRomSize; // in 16kb units
	u8 chrRomSize; // in 8kb units
public:
	void setPrgRom(u8* prgRom, u8 PRsize);
	void setChrRom(u8* chrRom, u8 CHRsize);
	u8 rdCPU(u16 where);
	void wrCPU(u16 where, u8 what);
	u8 rdPPU(u16 where);
	void wrPPU(u16 where, u8 what);
	M_000_NROM() : Mapper() {
		prgBanks = new u8 * [2];
	};
};