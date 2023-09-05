#pragma once
#ifndef ROM_H
#define ROM_H
#include "simpleCPU.h"
#include "Mappers/Mapper.h"
#include "Mappers/000_NROM.h"
#include "Mappers/001_SxROM.h"
#include "Mappers/002_UxROM.h"
#include "Mappers/003_CNROM.h"

class Rom {// basic for just NROM support
private:
	u8 mapperType;
	u8 mirroringType;
	u8 prgRomSize; // in 16kB units
	u8 chrRomSize; // in 8kB units
	u8 resetPosition;
	u8* ram;
	u8** PRGBanks;
	u8** CHRBanks;
	u8* chrRom;
	Mapper* mapper;
	void callSxROM(u16 where, u8 what);
public:
	Rom(FILE* romFile, u8* ram);
	void mapNROM();
	void printInfo();
	u8 getChrRomSize();
	Mapper* getMapper();
	Rom();
};

#endif