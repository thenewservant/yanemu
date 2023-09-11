#include "Rom.h"
#include "ppu.h"

Rom::Rom(FILE* romFile, u8* ram) {
	u8* prgRom;
	u8* chrRom;

	this->ram = ram;
	if (romFile == NULL) {
		printf("Error: ROM file not found");
		exit(1);
	}
	u8 header[16];

	fread(header, 1, 16, romFile);
	if (header[0] != 'N' || header[1] != 'E' || header[2] != 'S' || header[3] != 0x1A) {
		printf("Error: ROM file is not a valid NES ROM");
		exit(1);
	}
	
	// if bytes 12 to 15 are not zero, and NES 2.0 is not set, then this is an iNES 1.0 ROM, and we discard the upper nibble of bytes 6 and 7

	if (((header[7] & 0x0C) != 8) && ((header[12] != 0) || (header[13] != 0) || (header[14] != 0) || (header[15] != 0))) {
		mapperType = (header[6] & 0xF0)>>4 ;
	}
	else {
		mapperType = ((header[6] & 0xF0) >> 4) | ((header[7] & 0xF0));
	}
	PRGBanks = (u8**)calloc(prgRomSize, sizeof(u8*));
	CHRBanks = (u8**)calloc(chrRomSize, sizeof(u8*));
	mirroringType = header[6] & 1;
	mirror = mirroringType;
	prgRomSize = header[4];
	chrRomSize = header[5];
	
	printf("Mapper type: %d\n", mapperType);
	printf("CHR ROM size: %d (%d KB)\n", chrRomSize, chrRomSize * 8);
	printf("PRG ROM size: %d (%d KB)\n", prgRomSize, prgRomSize * 16);
	printf("Cartdrige %s battery-backed PRG RAM\n", header[6] & 2 ? "has" : "does not have");
	printf("PRG RAM size: %d\n", header[8]);
	printf("Nes 2.0 Format? %s\n", header[7] & 0x08 ? "Yes" : "No");
	if (header[6] & 4)printf("Trainer present !\n");


	prgRom = (u8*)malloc(prgRomSize * 0x4000 * sizeof(u8));
	chrRom = (u8*)malloc(chrRomSize * 0x2000 * sizeof(u8));
	fread(prgRom, 0x4000 * sizeof(u8), prgRomSize, romFile);
	fread(chrRom, 0x2000 * sizeof(u8), chrRomSize, romFile);
	

	if (ppuMemSpace) {
		if (chrRomSize) {
			for (int i = 0; i < chrRomSize; i++) {
				ppuMemSpace[i] = (u8*)calloc(0x2000, sizeof(u8)); // 1 slot for PT for now
			}
		}
		else {
			ppuMemSpace[0] = (u8*)calloc(0x2000, sizeof(u8)); // uxrom -- chr ram only
		}

		for (int i = 0; i < 4; i++) {
			ppuMemSpace[i + 1] = (u8*)calloc(0x400, sizeof(u8)); // 4 slots for NT for now
		}
	}

	switch (mapperType) {
	case 0:
		mapper = new M_000_NROM();
		break;
	case 1:
		mapper = new M_001_SxROM();
		break;
	case 2:
		mapper = new M_002_UxROM();
		break;
	case 3:
		mapper = new M_003_CNROM();
		break;
	default:
		printf("Error: mapper type %d not supported yet", mapperType);
		exit(1);
	}
	fclose(romFile);
	
	mapper->setPrgRom(prgRom, prgRomSize);
	mapper->setChrRom(chrRom, chrRomSize);
}

void Rom::printInfo() {
	printf("Mapper type: %d \n", mapperType);
	printf("PRG ROM size: %d \n", prgRomSize);
}


u8 Rom::getChrRomSize() {
	return chrRomSize;
}


Mapper* Rom::getMapper() {
	return mapper;
}

Rom::Rom(){}