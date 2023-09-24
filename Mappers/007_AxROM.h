#pragma once
#include "Mapper.h"

class M_007_AxROM : public Mapper {
protected:
	u8* currentChrRom;
	u8* prgBank;
public:
	void setPrgRom(u8* prgRom, u8 PRsize);
	void setChrRom(u8* chrRom, u8 CHRsize);
	u8 rdCPU(u16 where);
	void wrCPU(u16 where, u8 what);
	u8 rdPPU(u16 where);
	void wrPPU(u16 where, u8 what);
	void wrNT(u16 where, u8 what);
	u8 rdNT(u16 where);
	M_007_AxROM() :Mapper() {
		prgBanks = new u8 * [1];
		chrBanks = new u8 * [1];
	};
};