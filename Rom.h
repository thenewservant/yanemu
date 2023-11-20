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
#include "Mappers/011_COLOR_DREAMS.h"

class Rom {
private:
	const char* saveFolder = "BatterySaves";
	u8 header[16];
	u8 mapperType;
	u8 prgRomSize; // in 16kB units
	u8 chrRomSize; // in 8kB units
	u8* prgRom;
	u8* chrRom;
	Mapper* mapper;
	const char* romPath;
	const char* romName;
private:
	const char* getFileNameFromPath(const char* path);
	void checkForSaveFolder();
public:
	Rom(const char* filePath);
	void printInfo();
	Mapper* getMapper();
	bool hasBattery();
	const char* getfileName();
	void loadWorkRam();
	void saveWorkRam();
	Rom() : header{ 0 }, mapperType{ 0 }, prgRomSize{ 0 }, chrRomSize{ 0 },
		prgRom{ nullptr }, chrRom{ nullptr }, mapper{ nullptr },
		romPath{ nullptr }, romName{ nullptr }
	{};
};
#endif