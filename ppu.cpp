#include "ppu.h"
#define SPRITE_0_BIT 0b00000000000000000001000000000000


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
u16 t = 0, v = 0;
u8 x = 0;
boolean w = 0;

using namespace std;

void copyChrRom(u8* chr) {
	for (u16 t = 0; t < (1 << 13); t++) {
		ppuRam[t] = chr[t];
	}
}

void updateOam() { // OAMDMA (0x4014) processing routine
	for (u16 i = 0; i <= 0xFF; i++) {
		OAM[(u8)(oamADDR++)] = ram[(ram[0x4014] << 8) | i];
		//printf("\n%X", i);
	}
}

//stands for NameTable Prefix -- add that to required ppuRAM address suffix
#define NT_PREFIX (0x2000 | (ram[0x2000] & 3) * 0x400)
#define PAT_TB_PREFIX ((ram[0x2000] & 0b00010000) ? 0x1000 : 0)


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


void incCoarseX() {

	if ((v & 0x1F) == 31) {
		v ^= 0x0400;
		v &= ~0x1F; //reset coarse X
	}
	else {
		v++;
	}

}

void incFineY() {
	if ((v & 0x7000) != 0x7000) {
		v += 0x1000;                     // increment fine Y
	}
	else {
		v &= ~0x7000;                     // fine Y = 0
		u16 y = (v & 0x03E0) >> 5;       // let y = coarse Y
		if (y == 29) {
			y = 0;                    // coarse Y = 0
			v ^= 0x0800;                   // switch vertical nametable
		}
		else if (y == 31) {
			y = 0;                       // coarse Y = 0, nametable not switched
		}
		else {
			y += 1;                       // increment coarse Y
		}
		v = (v & ~0x03E0) | (y << 5);     // put coarse Y back into v
	}
}
u8 aC = 0;
void PPU::drawNameTable(u16 cycle, u8 scanl) {

	u8 nbSprites = 0;
	u8 nbSpritesCurSl = 0;
	u16 coarseV = v & 0x0FFF;
	u8 fineY = (v >> 12);

	u8 tile = ppuRam[0x2000 | coarseV]; // raw 2x2 tile ID fetch
	u16 attr = ppuRam[0x23C0 | (v & 0x0C00) | ((v >> 4) & 0x38) | ((v >> 2) & 0x07)]; // raw attribute fetch
	u8 paletteShiftings = 2 * (((v >> 6) & 1)!=0) + (((v >> 1) & 1) != 0);

	u8 palette = (attr >> (paletteShiftings*2)) & 3; // palette number

	//Background rendering
	if (BG_RENDERING) {
		for (u8 k = 0; k < 8; k++) {

			u16 destination =  0x3F00 + 4*palette + ((ppuRam[PAT_TB_PREFIX + 16 * tile + fineY    ] >> (7 - k)) & 1)
											+ 2 * ((ppuRam[PAT_TB_PREFIX + 16 * tile + fineY + 8] >> (7 - k)) & 1);

			u16 d = getColor(destination);

			u16 tmp = (d & 3) ? d : 0x3F00; //default background color 

			pixels[scanl * SCREEN_WIDTH + cycle + k] = colors[ppuRam[tmp]] + (!(tmp & 3) ? 1 : 0); // if the given color is identified as opaque, add 1, as it will be represented by SDL as slightly opaque, thus letting the ppu know this is a background color (will be used for sprite 0 evaluation)
		}
	}

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


	// Sprite rendering
	if (cycle == 248) {
		//if (xScroll)printf("\n%X", xScroll);
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

	if ((cycle == 248) && (scanl == 29)) {
		ram[0x2002] |= 0b01000000;
	}
	
	incCoarseX();
}

boolean renderable = false;
boolean isVBlank = false;

void PPU::sequencer() {

	if (scanLine < 240) {
		if ((cycle >= 257) && (cycle <= 320)) {
			oamADDR = 0;
		}
		else if ((((cycle % 8) == 0) && (cycle < 256)) && RENDERING) {
			drawNameTable(cycle, scanLine);
		}

		if (RENDERING) {
			if (cycle == 256) {
				incFineY();
				//incCoarseX();
			}
			if (cycle == 257) {
				v = (v & ~0xC1F) | (0xC1F & t); // hor (v) = hor(t)
			}
		}
	}

	else if ((cycle == 1) && (scanLine == 241)) {
		isVBlank = true;
		ram[0x2002] |= 0b10000000;
		if (ram[0x2000] & 0x80)_nmi();
	}

	else if (scanLine == 261) {
		if ((cycle >= 257) && (cycle <= 320)) {
			oamADDR = 0;
		}

		if (cycle == 1) {
			ram[0x2002] &= ~0b11100000;
			isVBlank = false;
		}

		if (RENDERING) {
			if ((cycle >= 280) && (cycle <= 304)) {
				v = (v & ~0x7BE0) | (t & 0x7BE0); // vert (v) = vert(t)
			}
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
	if ((scanLine == 261) && (cycle == 339)) {
		if (oddFrame) {
			oddFrame ^= 1;
			cycle = 0;
			scanLine = 0;
		}
	}
	scanLine %= 262;

	sequencer();
}

PPU::PPU(u32* px, Screen sc, Rom rom) {
	ram[0x2002] = 0b10000000;
	scanLine = 261;
	cycle = 0;
	scr = sc;
	oamADDR = 0;
	ppuRam = (u8*)calloc(1 << 16, sizeof(u8));
	pixels = px;
	copyChrRom(rom.getChrRom());
	printf("PPU Startup: ok!\n\n");
}

#define V (v&0x3FFF)

void writePPU(u8 what) {


	if ((V >= 0x3000) && (V <= 0x3EFF)) {
		ppuRam[V - 0x1000] = what;
	}
	else if (V > 0x3EFF) {
		ppuRam[getColor(V)] = what;
	}
	else {
		ppuRam[V] = what;

		if (V >= 0x2000) {
			if (mirror == VERTICAL) {
				if (V < 0x2800) {
					ppuRam[V + 0x800] = what;
				}
				else if (V < 0x3000) {
					ppuRam[V - 0x800] = what;
				}
			}
		}
	}
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


	if ((V >= 0x3000) && (V <= 0x3EFF)) {
		u8 tmp = internalPPUreg;

		internalPPUreg = ppuRam[(V)-0x1000];
		incPPUADDR();
		return tmp;

	}
	if (V > 0x3F1F) {
		return ppuRam[getColor(V)];
	}
	else if ((V >= 0x3EFF) && (V < 0x3F1F)) {


		internalPPUreg = ppuRam[getColor(V)];
		incPPUADDR();
		return internalPPUreg;
	}
	else {

		if (V >= 0x2000) {

			if (mirror == VERTICAL) {
				if (V <= 0x2800) {
					u8 tmp = internalPPUreg;

					internalPPUreg = ppuRam[getColor(V + 0x800)];
					incPPUADDR();

					return tmp;
				}
				else if (V < 0x3000) {

					u8 tmp = internalPPUreg;

					internalPPUreg = ppuRam[getColor(V - 0x800)];
					incPPUADDR();

					return tmp;
				}
			}
			else {
				u8 tmp = internalPPUreg;

				internalPPUreg = ppuRam[getColor(V)];
				incPPUADDR();
				return tmp;
			}
		}
		else {
			u8 tmp = internalPPUreg;

			internalPPUreg = ppuRam[getColor(V)];
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
		v += 32;
	}
	else {
		v++;
	}
}