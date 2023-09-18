#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>

typedef uint8_t u8;
typedef uint16_t u16;

class Mapper
{
protected:
	u8 prgRomSize; // in 16kb units
	u8 chrRomSize; // in 8kb units
	u8** prgBanks;
	u8** chrBanks;
	u8* prg;
public:
	virtual void setPrgRom(u8* prgRom, u8 PRsize)=0;// Sets the PRG ROM (one contiguous block)
	virtual void setChrRom(u8* chrRom, u8 CHRsize)=0;// Sets the CHR ROM (one contiguous block)
	virtual u8 rdCPU(u16 where)=0;
	virtual void wrCPU(u16 where, u8 what)=0;
	virtual u8 rdPPU(u16 where)=0;
	virtual void wrPPU(u16 where, u8 what)=0;
	Mapper() :
		chrRomSize{ 0 }, prgRomSize{ 0 }, prg{ nullptr }, prgBanks{ nullptr }, chrBanks{ nullptr } {}
};