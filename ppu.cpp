#include "ppu.h"
#include <vector>

const uint32_t colors[] = { 0x54545400, 0x001e7400, 0x08109000, 0x30008800, 0x44006400, 0x5c003000, 0x54040000, 0x3c180000, 0x202a0000, 0x083a0000, 0x00400000, 0x003c0000, 0x00323c00, 0x00000000, 0x00000000, 0x00000000,
							0x98969800, 0x084cc400, 0x3032ec00, 0x5c1ee400, 0x8814b000, 0xa0146400, 0x98222000, 0x783c0000, 0x545a0000, 0x28720000, 0x087c0000, 0x00762800, 0x00667800, 0x00000000, 0x00000000, 0x00000000,
							0xeceeec00, 0x4c9aec00, 0x787cec00, 0xb062ec00, 0xe454ec00, 0xec58b400, 0xec6a6400, 0xd4882000, 0xa0aa0000, 0x74c40000, 0x4cd02000, 0x38cc6c00, 0x38b4cc00, 0x3c3c3c00, 0x00000000, 0x00000000,
							0xeceeec00, 0xa8ccec00, 0xbcbcec00, 0xd4b2ec00, 0xecaeec00, 0xecaed400, 0xecb4b000, 0xe4c49000, 0xccd27800, 0xb4de7800, 0xa8e29000, 0x98e2b400, 0xa0d6e400, 0xa0a2a000, 0x00000000, 0x00000000 };

int32_t scanLine = 0, cycle = 0;
u8* ppuRam;// = (u8*)calloc(1 << 15, sizeof(u8));
u16 ppuADDR = 0;
u8 oamADDR = 0;
u8* OAM = (u8*)calloc(256, sizeof(u8));

using namespace std;

void copyChrRom(u8* chr) {
	for (u16 t = 0; t < (1 << 13); t++) {
		ppuRam[t] = chr[t];
	}
}


void PPU::showNameTable() {
}

void updateOam() { // OAMDMA (0x4014) processing routine
	for (u16 i = 0; i <= 0xFF; i++) {
		OAM[(u8)(oamADDR++)] = ram[(ram[0x4014] << 8) | i];
		//printf("\n%X", i);
	}
}

/*void PPU::drawSprite(uint8_t yy, uint8_t id, uint8_t xx, uint8_t byte2, uint8_t palette) {
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			uint16_t cur = (yy + j) * SCREEN_WIDTH + xx + i;
			pixels[cur] = 0xFF00FF00;

			int y = (yy + j);
			int x = xx + i;
			uint16_t baseAd = (id & 1) ? (0x1000 | (id >> 1)) : (id >> 1);
			pixels[cur] = LUT[byte2 & 3][((chrrom[(16 * baseAd) + i] >> ((7 - j))) & 1) + ((chrrom[(16 * baseAd) + i + 8] >> ((7 - j))) & 1)];
		}
	}
}*/

void PPU::finalRender() {
	if (0)updateOam();
	//printf("\n\n%X:%X:%X:%X:%X:%X:%X\n", ram[0x2000] , ram[0x2001], ram[0x2003], ram[0x2004], ram[0x2005], ram[0x2006], ram[0x2007], ram[0x4014]);
}

void TELLNAMETABLE() {
	printf("\n\n\nNAMETABLE\n\n\n");
	for (int i = 0; i < 32 * 30; i++) {
		printf("%X:%X\n", i, ppuRam[0x2000 + i]);
	}
}

//stands for NameTable Prefix -- add that to needed ppuRAM address suffix
#define NT_PREFIX (0x2000 + (ram[0x2000] & 3) * 0x400)
#define PAT_TB_PREFIX (((ram[0x2000] & 0b00010000) > 0) ? 0x1000 : 0)
#define EXAMPLE 0 // show how scanlines work


void drawExample() {

		if (cycle < 256) {
			if (scanLine - 1) {
				for (int k = 0; k < 8; k++)
					pixels[(scanLine - 1) * SCREEN_WIDTH + cycle + k] = 0x00111100;
			}
			for (int k = 0; k < 8; k++)
				pixels[scanLine * SCREEN_WIDTH + cycle + k] = 0xFF00FF00;
		}
	
}


void drawNameTable(u16 cycle, u8 scanl) {
	std::vector<int> back, front; // relative position to the background
	u8 nbSprites = 0;

	if ((cycle == 248) && (ram[0x2001] & 0b00010000)) {
		for (u16 oamIter = 0; oamIter < 64; oamIter++) {
			
			//back (low priority)
			if (OAM[4 * oamIter + 2] & 0b00100000) { // if good priority ...
				if (OAM[4 * oamIter ] == (scanLine-1)) {//...and approriate y coordinate ...
					if (nbSprites <= 8) {
						nbSprites++;
						for (int k = 0; k < 8; k++) {


							pixels[scanl * SCREEN_WIDTH + OAM[4 * oamIter + 3] + k] = 0xFF00FF00;
						}
					}
				}
			}

			else if ((OAM[4 * oamIter + 2] & 0b00100000) ^ 0b00100000) {
				if (OAM[4 * oamIter ] == (scanLine-1)) {
					if (nbSprites <= 8) {
						nbSprites++;
						front.push_back(oamIter);
					}
				}
			}
		}
	}

		u8 attribute = ppuRam[NT_PREFIX + 0x3C0 + (cycle / 32) + 8 * (scanl / 32)]; // raw 2x2 tile attribute fetch
		u8 value = ((attribute >> (4 * (((scanl) / 16) % 2))) >> (2 * (((cycle) / 16) % 2))) & 3;	  // Temporary solution. MUST be opitmized later

		for (int k = 0; k < 8; k++) {
			if (((scanl * SCREEN_WIDTH + cycle + k) < SCREEN_WIDTH * SCREEN_HEIGHT) && (ram[0x2001] & 0b00001000)) {

				//printf("%X\n", NT_PREFIX + 8 * (cycle / 8) + 32 * scanl);
				pixels[scanl * SCREEN_WIDTH + cycle + k] = colors[ppuRam[0x3F00 + 4 * value + ((ppuRam[(scanl % 8) + PAT_TB_PREFIX + 16 * ppuRam[NT_PREFIX + (cycle / 8) + 32 * (scanl / 8)]] >> (7 - k)) & 1) + 2 *




					((ppuRam[(scanl % 8) + 8 + PAT_TB_PREFIX + 16 * ppuRam[NT_PREFIX + (cycle / 8) + 32 * (scanl / 8)]] >> (7 - k)) & 1)]];

				//if (k == 7) pixels[scanl * SCREEN_WIDTH + cycle] = 0x83894000;
			}
		}

		if (cycle == 248) {
			for (int sprite : front) {
				for (int l = 0; l < 8; l++) {
					for (int k = 0; k < 8; k++) {
						pixels[(scanl+l) * SCREEN_WIDTH + OAM[4 * sprite + 3] + k] = ((ppuRam[l+ ((OAM[4*sprite + 1] & 1) ? 0x1000 : 0) + 8*2*(OAM[4*sprite + 1] >> 1)] >> (7 - k)) & 1) * 10000000;
					}
				}
				OAM[sprite + 2] = 0;
			}
		}

		//printf("\nX: %d, Y: %d", OAM[3], OAM[0] + 1);
		if ((cycle == 160) && (scanl == 3) ) {
			ram[0x2002] |= 0b01000000;
		}
		//ppuADDR++;
	
		if ((cycle == 8) && (scanl == 5)) {
			ram[0x2002] |= 0b01100000;
		}
		
}


boolean isVBlank = false;

void PPU::sequencer() {
	
	if (scanLine < 240) {
		if ((cycle >= 257) && (cycle <= 320)) {
			oamADDR = 0;
		}
		else if (((cycle % 8) == 0) && (cycle < 256)) {
			
			drawNameTable(cycle, scanLine);
			
			//drawSprite();
			//updateOam();
			//printf("here");
		}
	}

	if ((cycle >= 257) && (cycle <= 320) && (scanLine == 261)) {
		oamADDR = 0;
	}


	if (scanLine == 240) {
		isVBlank = true;
		
		
	}
	if ((cycle == 1) && (scanLine == 241)) {
		ram[0x2002] |= 0b10000000;
		if ((ram[0x2000] & 0x80) )_nmi();
	}

	if (scanLine == 261) {
		if (cycle == 1) {
			//printf("wut");
			ram[0x2002] &= ~0b11100000;
			isVBlank = false;
		}

	}

}

boolean isInVBlank() {
	//printf("mdr");
	return isVBlank;
}

void PPU::tick() {
	cycle++;
	if (cycle / 341) {

		//ppuADDR+=1;
		//printf("\n %X", ram[0x4016]);
		//printf("\n\ncycle %d SCANLINE %d ppuADDR %2X", cycle, scanLine, ppuADDR);
		scanLine++;
		cycle = 0;
	}
	scanLine %= 262;

	if (EXAMPLE) {
		drawExample();
	}
	else {
		sequencer();
	}
	
	//printf("\n%d, %d\n", cycle, scanLine);
	//finalRender();
}


PPU::PPU(u32* px, Screen sc, Rom rom) {
	ram[0x2002] = 0b10000000;
	scanLine = -1;
	cycle = 0;
	scr = sc;
	ppuADDR = 0;
	oamADDR = 0;
	ppuRam = (u8*)calloc(1 << 16, sizeof(u8));
	pixels = px;
	copyChrRom(rom.getChrRom());
	printf("PPU Startup: ok!\n\n");
	//Sleep(300);
}



void writePPU(u8 what) {
	ppuRam[ppuADDR] = what;
	//printf("what?: %X actually : %X\n", what, ppuRam[ppuADDR]);
	incPPUADDR();
	//printf("\nppuADDR:%X", ppuADDR);
}

void writeOAM(u8 what) {
	if (!isVBlank) {
		OAM[oamADDR] = what;
		oamADDR++;
	}

}
u8 internalPPUreg = 0;
u8 readPPU() {
	u8 tmp = internalPPUreg;

	internalPPUreg = ppuRam[ppuADDR];
	incPPUADDR();
	return tmp;
}

u8 readOAM() {
	u8 tmp = OAM[oamADDR];

	if (!isVBlank) {
		oamADDR++;
	}
	return tmp;
}
void incPPUADDR() {
	if (ram[0x2000] & 4) {
		ppuADDR += 32;
	}
	else {
		ppuADDR++;
	}
}