#ifndef _011_COLOR_DREAMS_H_
#define _011_COLOR_DREAMS_H_
#include "Mapper.h"

class M_011_COLOR_DREAMS : public Mapper {
protected:
	u8* currentPrgRom;
	u8* currentChrRom;
public:
	void setPrgRom(u8* prgRom, u8 PRsize);
	void setChrRom(u8* chrRom, u8 CHRsize);
	u8 rdCPU(u16 where);
	void wrCPU(u16 where, u8 what);
	u8 rdPPU(u16 where);
	void wrPPU(u16 where, u8 what) { (void)where; (void)what; };
	M_011_COLOR_DREAMS() :Mapper(), currentPrgRom{ 0 }, currentChrRom{ 0 }
	{};
};
#endif