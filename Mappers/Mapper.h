#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include "../types.h"

enum MirroringModes {
	HORIZONTAL,
	VERTICAL,
	SINGLE_SCREEN_0,
	SINGLE_SCREEN_1,
	FOUR_SCREEN
};

class Mapper{
protected:
	u8 prgRomSize; // in 16kb units
	u8 chrRomSize; // in 8kb units
	u8** prgBanks;
	u8** chrBanks;
	u8* chr;
	u8* prg;
	u8 nameTables[4][0x400];
	u8 mirror;
public:
	virtual void setPrgRom(u8* prgRom, u8 PRsize)=0;// Sets the PRG ROM (one contiguous block)
	virtual void setChrRom(u8* chrRom, u8 CHRsize)=0;// Sets the CHR ROM (one contiguous block)
	virtual u8 rdCPU(u16 where)=0;
	virtual void wrCPU(u16 where, u8 what)=0;
	virtual u8 rdPPU(u16 where)=0;
	virtual void wrPPU(u16 where, u8 what)=0;
	virtual u8 rdNT(u16 where);
	virtual void wrNT(u16 where, u8 what);
	void setMirroring(u8 mir);
	virtual void acknowledgeNewScanline(bool risingEdge) { (void)risingEdge; };
	virtual u8* getPrgRam() { return (u8*)0; };
	virtual void setPrgRam(u8* ramPtr) { (void)ramPtr; };
	virtual void setSpriteMode(u8 mode) { (void)mode; };
	Mapper() :
		chrRomSize{ 0 }, prgRomSize{ 0 }, prg{ nullptr },
		prgBanks{ nullptr }, chrBanks{ nullptr }, mirror{ 0 }, nameTables{ 0 }, chr{ 0 } {};
};