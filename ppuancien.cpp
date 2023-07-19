#include "ppu.h"


const uint32_t colors[] = { 0x54545400, 0x001e7400, 0x08109000, 0x30008800, 0x44006400, 0x5c003000, 0x54040000, 0x3c180000, 0x202a0000, 0x083a0000, 0x00400000, 0x003c0000, 0x00323c00, 0x00000000, 0x00000000, 0x00000000,
							0x98969800, 0x084cc400, 0x3032ec00, 0x5c1ee400, 0x8814b000, 0xa0146400, 0x98222000, 0x783c0000, 0x545a0000, 0x28720000, 0x087c0000, 0x00762800, 0x00667800, 0x00000000, 0x00000000, 0x00000000,
							0xeceeec00, 0x4c9aec00, 0x787cec00, 0xb062ec00, 0xe454ec00, 0xec58b400, 0xec6a6400, 0xd4882000, 0xa0aa0000, 0x74c40000, 0x4cd02000, 0x38cc6c00, 0x38b4cc00, 0x3c3c3c00, 0x00000000, 0x00000000,
							0xeceeec00, 0xa8ccec00, 0xbcbcec00, 0xd4b2ec00, 0xecaeec00, 0xecaed400, 0xecb4b000, 0xe4c49000, 0xccd27800, 0xb4de7800, 0xa8e29000, 0x98e2b400, 0xa0d6e400, 0xa0a2a000, 0x00000000, 0x00000000 };

int32_t scanLine = 0, cycle = 0;
u8* ppuRam;// = (u8*)calloc(1 << 15, sizeof(u8));
u16 ppuADDR = 0;
u8 oamADDR = 0;
u8* OAM = (u8*)calloc(256, sizeof(u8));
u8 xScroll = 0;
u8 yScroll = 0;
boolean oddFrame = false;

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

//stands for NameTable Prefix -- add that to needed ppuRAM address suffix
//dead
#define PAT_TB_PREFIX ((ram[0x2000] & 0b00010000) ? 0x1000 : 0)

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

std::set<u32> spritesSet; //sprites that need to be drawn

u16 getColor(u16 where) { //compensates specific mirroring for palettes (0x3F10/0x3F14/0x3F18/0x3F1C are mirrors of 0x3F00/0x3F04/0x3F08/0x3F0C)
	if ((where == 0x3F10) || (where == 0x3F14) || (where == 0x3F18) || (where == 0x3F1C)) {
		return where & ~0x10;
	}
	else {
		return where;
	}
}





u16 handleFlipping(u32 sprite, int8_t diff, boolean hor, boolean ver, u8 k) {
	
	u16 spritePatternTable = (ram[0x2000] & 8) ? 0x1000 : 0;
	u8 h = hor ? k : 7 - k;
	u8 v = ver ? 8 - diff : diff;
	return 0x3F10 + 4 * ((sprite >> 8) & 3) + ((ppuRam[spritePatternTable + v % 8 + 8 * 2 * ((sprite >> 16) & 0xFF) + 0] >> (h)) & 1) + 2 *
		((ppuRam[spritePatternTable + v % 8 + 8 * 2 * ((sprite >> 16) & 0xFF) + 8] >> (h)) & 1);
}

#define SPRITE_0_BIT 0b00000000000000000001000000000000 // an unused by default OAM byte 2's bit is purposedly used here as a sprite 0 indicator.

void PPU::drawNameTable(u16 cycle, u8 scanl) {
	u8 nbSprites = 0;
	u8 nbSpritesCurSl = 0;

	//Sprite retrieving
	if ((cycle == 248) && (ram[0x2001] & 0b00010000)) {
		if (OAM[0] == (scanLine - 1)) {
			spritesSet.insert(SPRITE_0_BIT | (OAM[0] << 24) | (OAM[1] << 16) | (OAM[2] << 8) | (OAM[3]));
		}
		for (u16 oamIter = 1; oamIter < 64; oamIter++) {
			if (OAM[4 * oamIter] == (scanLine - 1)) {
				spritesSet.insert((OAM[4 * oamIter] << 24) | (OAM[4 * oamIter + 1] << 16) | (OAM[4 * oamIter + 2] << 8) | (OAM[4 * oamIter + 3]));
			}
		}
	}

	u16 offsetXScroll =  ((xScroll / 8 + cycle / 8) / 32)* (0x3E0);
	u16 ntPrefix = (0x2000 | (ram[0x2000] & 3) * 0x400);
	u8 attribute = ppuRam[offsetXScroll+ntPrefix + 0x3C0 + ((cycle+xScroll) / 32) + 8 * (scanl / 32)]; // raw 2x2 tile attribute fetch
	u8 value = ((attribute >> (4 * ((scanl / 16) % 2))) >> (2 * (((cycle+xScroll) / 16) % 2))) & 3;	  // Temporary solution. MUST be opitmized later

	//Background rendering
	
	

	for (u8 k = 0; k < 8; k++){
		if ( (ram[0x2001] & 0b00001000)) {

			u16 destination = 0x3F00 + 4 * value + ((ppuRam[(scanl % 8) + PAT_TB_PREFIX + 16 * ppuRam[offsetXScroll + ntPrefix + xScroll / 8 + cycle / 8 + 32 * (scanl / 8)]] >> (7 - k)) & 1) + 2 *

												   ((ppuRam[(scanl % 8) + 8 + PAT_TB_PREFIX + 16 * ppuRam[offsetXScroll + ntPrefix + xScroll / 8 + cycle / 8 + 32 * (scanl / 8)]] >> (7 - k)) & 1);
			

			u16 t = getColor(destination);
			u16 tmp = (t &3)? t:0x3F00;

			pixels[scanl * SCREEN_WIDTH + cycle + k] = colors[ppuRam[tmp]] + (!(tmp&3)? 1:0); // if the given color is identified as opaque, add 1, as it will be represented by SDL as slightly opaque, thus letting the ppu know this is a background color (will be used for sprite 0 evaluation)
		}	
	}

	// Sprite rendering
	if (cycle == 248) {
		for (u32 sprite : spritesSet) {
			int8_t diff = (scanl - (sprite >> 24) - 1);
			if (diff < 8) {
				u16 destination;
				for (u8 k = 0; k < 8; k++) {
					destination = handleFlipping(sprite, diff, (sprite >> 8) & 0b01000000, (sprite >> 8) & 0b10000000, k);

					
					if (destination & 3) {
						if ((sprite & SPRITE_0_BIT) && (!(pixels[(scanl)*SCREEN_WIDTH + (sprite & 0xFF) + k]&1)) && (ram[0x2001] & 2) && (ram[0x2001] & 4) && (cycle>7) )  {
							ram[0x2002] |= 0b01000000;
							//printf("HEY");
						}
						pixels[(scanl)*SCREEN_WIDTH + (sprite & 0xFF) + k] = colors[ppuRam[(destination)]];
					}
				}
			}
			else {
				spritesSet.erase(sprite);
			}
		}
	}
	
	if ((cycle == 248) && (scanl == 30)) {
		ram[0x2002] |= 0b01000000;
	}
}

boolean renderable = false;
boolean isVBlank = false;

void PPU::sequencer() {

	if (((cycle > 248) && (cycle < 360)) || ((scanLine > 240) && scanLine <260)) {
		renderable = true;
	}
	else {
		renderable = false;
	}

	if (scanLine < 240) {
		if ((cycle >= 257) && (cycle <= 320)) {
			oamADDR = 0;
		}
		else if (((cycle % 8) == 0) && (cycle < 256)) {
			drawNameTable(cycle, scanLine);
		}
	}

	if ((cycle >= 257) && (cycle <= 320) && (scanLine == 261)) {
		oamADDR = 0;
	}


	if ((scanLine == 240) && (cycle==1)) {
		isVBlank = true;
	}

	if ((cycle == 1) && (scanLine == 241)) {
		ram[0x2002] |= 0b10000000;
		if (ram[0x2000] & 0x80)_nmi();
	}

	if (scanLine == 261) {
		if (cycle == 1) {
			ram[0x2002] &= ~0b11100000;
			isVBlank = false;
		}

	}

}

boolean isInVBlank() {
	return isVBlank;
}

void PPU::tick() {
	cycle++;
	if (cycle / 341) {
		scanLine++;
		cycle = 0;
	}
	if (scanLine == 261) {
		if (oddFrame) {
			oddFrame ^= 1;
			cycle++;
		}
	}
	scanLine %= 262;

	if (EXAMPLE) {
		drawExample();
	}
	else {
		sequencer();
	}
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
}

void writePPU(u8 what) {
	if ((ppuADDR >= 0x3000) && (ppuADDR <= 0x3EFF)) {
		ppuRam[ppuADDR - 0x1000] = what;
	}
	else if (ppuADDR > 0x3EFF) {
		ppuRam[getColor(ppuADDR)] = what;
	}
	else {
		ppuRam[ppuADDR] = what;
		if (ppuADDR >= 0x2000) {
			if (mirror == VERTICAL) {
				if (ppuADDR < 0x2800) {
					ppuRam[ppuADDR + 0x800] = what;
				}
				else if (ppuADDR < 0x3000){
					ppuRam[ppuADDR - 0x800] = what;
				}
			}
		}
	}
	if (ppuADDR == 0x2400)printf("\nrendering :%d, vblank: %d, cycle: %d, scanline: %d, 0x2400: %X, 0x2C00:%X", ram[0x2001] & 6, isVBlank, cycle, scanLine, ppuRam[0x2400], ppuRam[0x2C00]);
	incPPUADDR();
	
}

void writeOAM(u8 what) {
	if (!isVBlank) {
		OAM[oamADDR] = what;
		oamADDR++;
	}
}
u8 internalPPUreg = 0;

u8 readPPU() {
	if ((ppuADDR >= 0x3000) && (ppuADDR <= 0x3EFF)) {
		u8 tmp = internalPPUreg;

		internalPPUreg = ppuRam[(ppuADDR)-0x1000];
		incPPUADDR();
		return tmp;
		
	}
	if (ppuADDR > 0x3F1F){
		return ppuRam[getColor(ppuADDR)];
	}
	else if ((ppuADDR >= 0x3EFF) && (ppuADDR <  0x3F1F)){


		internalPPUreg = ppuRam[getColor(ppuADDR)];
		incPPUADDR();
		return internalPPUreg;
	}
	else {

		if (ppuADDR >= 0x2000) {


			if (mirror == VERTICAL) {
				if (ppuADDR <= 0x2800) {
					u8 tmp = internalPPUreg;

					internalPPUreg = ppuRam[getColor(ppuADDR + 0x800)];
					incPPUADDR();

					return tmp;
				}
				else if (ppuADDR < 0x3000) {

					u8 tmp = internalPPUreg;

					internalPPUreg = ppuRam[getColor(ppuADDR - 0x800)];
					incPPUADDR();

					return tmp;
				}
			}
			else {
				u8 tmp = internalPPUreg;

				internalPPUreg = ppuRam[getColor(ppuADDR)];
				incPPUADDR();
				return tmp;
			}
		}
		else {
			u8 tmp = internalPPUreg;

			internalPPUreg = ppuRam[getColor(ppuADDR)];
			incPPUADDR();
			return tmp;
		}

	}
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
