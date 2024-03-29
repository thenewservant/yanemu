#ifndef _002_UxROM_H_
#define _002_UxROM_H_
#include "Mapper.h"
class M_002_UxROM : public Mapper {
protected:
	u8 chrRom[0x2000];
	u8* switchablePrgBank;
	u8* lastPrgBank;
public:
	void setPrgRom(u8* prgRom, u8 PRsize);
	void setChrRom(u8* chrRom, u8 CHRsize);
	u8 rdCPU(u16 where);
	void wrCPU(u16 where, u8 what);
	u8 rdPPU(u16 where);
	void wrPPU(u16 where, u8 what);
	M_002_UxROM():Mapper(),
		chrRom{ 0 }, switchablePrgBank{ 0 }, lastPrgBank{ 0 }
	{};
};
#endif