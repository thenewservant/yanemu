#ifndef _000_NROM_H_
#define _000_NROM_H_
#include "Mapper.h"

class M_000_NROM : public Mapper{
protected:
	u8 chrBank[0x2000];
	u8 ramBank[0x2000];//almost never used, just for very specific non-standard NROM games.
public:
	void setPrgRom(u8* prgRom, u8 PRsize) override {
		prgBanks[0] = prgRom - 0x8000;

		switch (PRsize) {
		case 1:
			prgBanks[1] = prgRom - 0xC000;
			break;
		case 2:
			prgBanks[1] = prgRom + 0x4000 - 0xC000;
			break;
		default:
			printf("\nError: PRG ROM size not supported\n");
			exit(1);
		}
	}
	
	void setChrRom(u8* chrRom, u8 CHRsize) override {
		chrRomSize = CHRsize;
		if (CHRsize) {
			memcpy(chrBank, chrRom, 0x2000);
		}
	}
	
	u8 rdCPU(u16 where) override {
		switch (where & 0xC000) {
		case 0x8000:
			return prgBanks[0][where];
		case 0xC000:
			return prgBanks[1][where];
		default:
			return ramBank[where - 0x6000];
		}
	}

	void wrCPU(u16 where, u8 what) override {
		if (where < 0x8000) {
			ramBank[where - 0x6000] = what;
		}
	}

	u8 rdPPU(u16 where) override {
		return chrBank[where];
	}

	void wrPPU(u16 where, u8 what) override {
		if (!chrRomSize) {
			chrBank[where] = what;
		}
	}

	u8* getPrgRam() override {
		return ramBank; 
	}

	M_000_NROM() : Mapper(), chrBank{ 0 }, ramBank{ 0 } {
		prgBanks = new u8 * [2];
	}
};
#endif