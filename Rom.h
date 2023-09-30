#pragma once
#ifndef ROM_H
#define ROM_H

#include "types.h"
#include "Mappers/Mapper.h"
#include "Mappers/000_NROM.h"
#include "Mappers/001_SxROM.h"
#include "Mappers/002_UxROM.h"
#include "Mappers/003_CNROM.h"
#include "Mappers/004_TxROM.h"
#include "Mappers/007_AxROM.h"

class Rom {// basic for just NROM support
private:
	u8 header[16];
	u8 mapperType;
	u8 prgRomSize; // in 16kB units
	u8 chrRomSize; // in 8kB units
	u8* prgRom;
	u8* chrRom;
	Mapper* mapper;
public:
	Rom(FILE* romFile);
	void printInfo();
	Mapper* getMapper();
	Rom();
};
#endif