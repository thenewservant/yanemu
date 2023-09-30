#include "Rom.h"

Rom::Rom(FILE* romFile) {
	if (romFile == NULL) {
		printf("Error: ROM file not found");
		exit(1);
	}

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
	bool mirror = header[6] & 1;
	prgRomSize = header[4];
	chrRomSize = header[5];
	prgRom = (u8*)malloc(prgRomSize * 0x4000 * sizeof(u8));
	chrRom = (u8*)malloc(chrRomSize * 0x2000 * sizeof(u8));

	if (!(prgRom || chrRom)) {
		printf("Error: not enough memory to load ROM");
		exit(1);
	}
	else {
		fread(prgRom, 0x4000 * sizeof(u8), prgRomSize, romFile);
		fread(chrRom, 0x2000 * sizeof(u8), chrRomSize, romFile);
	}
	
	switch (mapperType) {
	case 0:mapper = new M_000_NROM();break;
	case 1:mapper = new M_001_SxROM();break;
	case 2:mapper = new M_002_UxROM();break;
	case 3:mapper = new M_003_CNROM();break;
	case 4:mapper = new M_004_TxROM();break;
	case 7:mapper = new M_007_AxROM();break;
	default:
		printf("Error: mapper type %d not supported yet", mapperType);
		exit(1);
	}
	fclose(romFile);
	mapper->setPrgRom(prgRom, prgRomSize);
	mapper->setChrRom(chrRom, chrRomSize);
	mapper->setMirroring(mirror);
}

void Rom::printInfo() {
	printf("Mapper type: %d \n", mapperType);
	printf("Mirroring type: %s\n", header[6]&1 ? "Vertical" : "Horizontal");
	printf("CHR ROM size: %d (%d KB)\n", chrRomSize, chrRomSize * 8);
	printf("PRG ROM size: %d (%d KB)\n", prgRomSize, prgRomSize * 16);
	printf("Cartdrige %s battery-backed PRG RAM\n", header[6] & 2 ? "has" : "does not have");
	printf("PRG RAM size: %d\n", header[8]);
	printf("Nes 2.0 Format? %s\n", header[7] & 0x08 ? "Yes" : "No");
	printf("Trainer %s\n", header[6] & 4 ? "present" : "absent");
}

Mapper* Rom::getMapper() {
	return mapper;
}

Rom::Rom(){}