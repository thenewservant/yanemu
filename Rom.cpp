#include "Rom.h"


Rom::Rom(FILE* romFile, u8* ram) {
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

	mapperType = ((header[6] & 0xF0)>>4) | ((header[7] & 0xF0));
	mirroringType = header[6] & 1;
	mirror = mirroringType;
	prgRomSize = header[4];
	chrRomSize = header[5];
	printf("Mapper type: %d\n", mapperType);
	printf("CHR ROM size: %d\n", chrRomSize);
	printf("Trainer present: %d\n", (header[6] & 0x4) >> 2);
	PRGBanks = (u8**)calloc(prgRomSize, sizeof(u8*));
	CHRBanks = (u8**)calloc(chrRomSize, sizeof(u8*));

	for (int PRGbank = 0; PRGbank < prgRomSize; PRGbank++) {
		PRGBanks[PRGbank] = (u8*)malloc(0x4000* sizeof(u8));
		if (!PRGBanks[PRGbank]) {
			printf("Error: can't allocate memory for PRG banks");
			exit(1);
		}
		else {
			fread(PRGBanks[PRGbank], 1, 0x4000, romFile);
		}
		
	}
	//now we need to copy data from file to CHR banks
	for (int CHRbank = 0; CHRbank < chrRomSize; CHRbank++) {
		CHRBanks[CHRbank] = (u8*)malloc(0x2000 * sizeof(u8));
		if (!CHRBanks[CHRbank]) {
			printf("Error: can't allocate memory for CHR banks");
			exit(1);
		}
		else {
			fread(CHRBanks[CHRbank], 1, 0x2000, romFile);
		}
	}

	switch (mapperType) {
	case 0:
	case 1:
		mapNROM();
		break;
	case 2:
		mapUxROM();
		break;
	default:
		printf("Error: mapper type %d not supported yet", mapperType);
		exit(1);
	}
}

void Rom::mapNROM() {
	memcpy(ram + 0x8000, PRGBanks[0], 0x4000);
	if (prgRomSize == 1) {
		memcpy(ram + 0xC000, PRGBanks[0], 0x4000);
	}
	else if (prgRomSize == 2) {
		memcpy(ram + 0xC000, PRGBanks[1], 0x4000);
	}
	chrRom = CHRBanks[0]? CHRBanks[0] : NULL;
}

void Rom::mapUxROM() {
	memcpy(ram + 0xC000, PRGBanks[prgRomSize - 1], 0x4000);
	//memcpy(ram + 0x8000, PRGBanks[0], 0x4000);
}

void Rom::printInfo() {
	printf("Mapper type: %d \n", mapperType);
	printf("PRG ROM size: %d \n", prgRomSize);
}

u8* Rom::getChrRom() {
	return chrRom;
}

u8 Rom::readPrgRom(u16 addr) {
	return prgRom[addr];
}

void Rom::contactMappers(u16 where, u8 what)
{
	switch (mapperType) {
	case 2:
	//case 66:
		memcpy(ram + 0x8000, PRGBanks[what], 0x4000);
		//ram[0x8000] = what;
		break;
	default:
		break;
	}
}

u8 Rom::readChrRom(u16 addr) {
	return chrRom[addr];
}

u8 Rom::getChrRomSize() {
	return chrRomSize;
}

Rom::~Rom() {
	free(prgRom);
	free(chrRom);
}

Rom::Rom() {}