#pragma once
#include "Mapper.h"

class M_000_NROM : public Mapper{
protected:
	u8* chrBank;
	u8 ramBank[0x2000];//almost never used, just for very specific non-standard NROM games.
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