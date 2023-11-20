#pragma warning(disable:4996)
#include "Rom.h"

Rom::Rom(const char* filePath) :romPath{ nullptr }, romName{ nullptr } {
	FILE* romFile = fopen(filePath, "rb");
	romPath = filePath;
	romName = getFileNameFromPath(filePath);
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

	if (!(prgRom && chrRom)) {
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
	case 11:mapper = new M_011_COLOR_DREAMS();break;
	default:
		printf("Error: mapper type %d not supported yet", mapperType);
		exit(1);
	}
	fclose(romFile);
	mapper->setPrgRom(prgRom, prgRomSize);
	mapper->setChrRom(chrRom, chrRomSize);
	mapper->setMirroring(mirror);

	checkForSaveFolder();
	loadWorkRam();
}

void Rom::checkForSaveFolder() { //check if the folder for battery saves exists, if not, create it
	char* buf = (char*)malloc(104 * sizeof(char));
	if (MKDIR(saveFolder) == 0) {
		printf("Info: Battery save folder successfully created\n");
	}
}

void Rom::printInfo() {
	printf("ROM Name: %s\n", romName);
	printf("Mapper type: %d \n", mapperType);
	printf("Mirroring type: %s\n", header[6]&1 ? "Vertical" : "Horizontal");
	printf("CHR ROM size: %d (%d KB)\n", chrRomSize, chrRomSize * 8);
	printf("PRG ROM size: %d (%d KB)\n", prgRomSize, prgRomSize * 16);
	printf("Cartdrige %s battery-backed PRG RAM\n", header[6] & 2 ? "has" : "does not have");
	printf("Nes 2.0 Format? %s\n", header[7] & 0x08 ? "Yes" : "No");
	printf("Trainer %s\n", header[6] & 4 ? "present" : "absent");
}

Mapper* Rom::getMapper() {
	return mapper;
}

bool Rom::hasBattery() {
	return header[6] & 2;
}

const char* Rom::getFileNameFromPath(const char* path) {
	const char* fileName = path;
	const char* slash = strrchr(path, '/');
	const char* backslash = strrchr(path, '\\');

	if (slash || backslash) {
		// Use the last slash or backslash found, whichever comes last.
		const char* separator = (slash > backslash) ? slash : backslash;
		fileName = separator + 1;
	}

	char* fn2 = (char*)malloc(100 * sizeof(char));
	fn2 = strcpy(fn2, fileName);
	char* dot = strrchr(fn2, '.');
	if (dot) {
		*dot = '\0';
	}

	return fn2;
}

const char* Rom::getfileName() {
		return romName;
}

void Rom::loadWorkRam() {
	if (header[6] & 2) {
		char* buf = (char*)malloc(104 * sizeof(char));
		if (buf) {
			sprintf(buf, "%s/%s.7bs", saveFolder, romName);
			FILE* batteryFile = fopen(buf, "rb");
			if (batteryFile != NULL) {
				printf("Found battery file, loading... \n");
				fread(mapper->getPrgRam(), 0x2000, 1, batteryFile);
				fclose(batteryFile);
			}
		}
	}
}

void Rom::saveWorkRam() {
	if (this->hasBattery() && mapper->getPrgRam()) {
		printf("\nSaving Work Ram...\n");
		char* buf = (char*)malloc(104 * sizeof(char));
		if (buf) {
			sprintf(buf, "%s/%s.7bs", saveFolder, romName);
			FILE* chrRamFile = fopen(buf, "wb");
			free(buf);
			if (!chrRamFile) {
				printf("Error: can't open battery file\n");
				exit(1);
			}
			fwrite(mapper->getPrgRam(), 0x2000, 1, chrRamFile);
			fclose(chrRamFile);
		}
		else {
			printf("Error: can't allocate memory for battery file name\n");
			exit(1);
		}
	}
}