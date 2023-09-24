#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>

typedef uint8_t u8;
typedef uint16_t u16;

#define HORIZONTAL 0
#define VERTICAL 1
#define SINGLE_SCREEN_0 2
#define SINGLE_SCREEN_1 3

class Mapper
{
protected:
	u8 prgRomSize; // in 16kb units
	u8 chrRomSize; // in 8kb units
	u8** prgBanks;
	u8** chrBanks;
	u8* prg;
	u8** nameTables;
	u8 mirror;
public:
	virtual void setPrgRom(u8* prgRom, u8 PRsize)=0;// Sets the PRG ROM (one contiguous block)
	virtual void setChrRom(u8* chrRom, u8 CHRsize)=0;// Sets the CHR ROM (one contiguous block)
	virtual u8 rdCPU(u16 where)=0;
	virtual void wrCPU(u16 where, u8 what)=0;
	virtual u8 rdPPU(u16 where)=0;
	virtual void wrPPU(u16 where, u8 what)=0;
	
	virtual u8 rdNT(u16 where) {
		u8 ntID = (where >> 10) & 3;
		switch (mirror) {
		case HORIZONTAL:
			return nameTables[(ntID >> 1)][where & 0x03FF];
			break;
		case VERTICAL:
			return nameTables[(ntID & 1)][where & 0x03FF];
			break;
		default:
			break;
		}
		return 0;
	}

	virtual void wrNT(u16 where, u8 what) {
		u8 ntID = (where >> 10) & 3;
		switch (mirror) {
		case HORIZONTAL:
			nameTables[(ntID >> 1)][where & 0x03FF] = what;
			break;
		case VERTICAL:
			nameTables[(ntID & 1)][where & 0x03FF] = what;
			break;
		}
	}

	void setMirroring(u8 mir);

	Mapper() :
		chrRomSize{ 0 }, prgRomSize{ 0 }, prg{ nullptr }, prgBanks{ nullptr }, chrBanks{ nullptr }, mirror{ 0 }
	{
		nameTables = new u8 * [4];
		for (int i = 0; i < 4; i++) {
			nameTables[i] = new u8[0x400];
		}
	}
};