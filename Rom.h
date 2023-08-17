#pragma once
#ifndef ROM_H
#define ROM_H

#include <cstdio>
#include "simpleCPU.hpp"

class Rom {// basic for just NROM support
private:
	u8 mapperType;
	u8 mirroringType;
	u8 prgRomSize; // in 16kB units
	u8 chrRomSize; // in 8kB units
	u8* prgRom;
	u8* chrRom;
	u8 resetPosition;
	u8* ram;
	u8** PRGBanks;
	u8** CHRBanks;
public:
	//constructor is 
	Rom(FILE* romFile, u8* ram);
	Rom();
	void mapNROM();
	void mapUxROM();
	void printInfo();
	u8* getChrRom();
	u8 readPrgRom(u16 addr);
	void contactMappers(u16 where, u8 what); //for bank-changing etc.
	u8 readChrRom(u16 addr);
	u8 getChrRomSize();
	~Rom();
};



#endif