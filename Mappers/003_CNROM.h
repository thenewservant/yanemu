#include "Mapper.h"
#pragma once

class M_003_CNROM : public Mapper {
private:
	u8* chrBank;
	u8* currentChrRom;
	u8** prgBanks;
	u8** chrBanks;
	u8 prgRomSize; // in 16kb units
	u8 chrRomSize; // in 8kb units
public:
	void setPrgRom(u8* prgRom, u8 PRsize);
	void setChrRom(u8* chrRom, u8 CHRsize);
	u8 rdCPU(u16 where);
	void wrCPU(u16 where, u8 what); 
	u8 rdPPU(u16 where);
	void wrPPU(u16 where, u8 what);
	M_003_CNROM(){  
		prgBanks = new u8 * [2];
		chrBanks = new u8 * [1];
	};
};