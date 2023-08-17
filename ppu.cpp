#include "ppu.h"

const uint32_t colors[][64] = {{ 0x54545400, 0x001e7400, 0x08109000, 0x30008800, 0x44006400, 0x5c003000, 0x54040000, 0x3c180000, 0x202a0000, 0x083a0000, 0x00400000, 0x003c0000, 0x00323c00, 0x00000000, 0x00000000, 0x00000000,
								 0x98969800, 0x084cc400, 0x3032ec00, 0x5c1ee400, 0x8814b000, 0xa0146400, 0x98222000, 0x783c0000, 0x545a0000, 0x28720000, 0x087c0000, 0x00762800, 0x00667800, 0x00000000, 0x00000000, 0x00000000,
								 0xeceeec00, 0x4c9aec00, 0x787cec00, 0xb062ec00, 0xe454ec00, 0xec58b400, 0xec6a6400, 0xd4882000, 0xa0aa0000, 0x74c40000, 0x4cd02000, 0x38cc6c00, 0x38b4cc00, 0x3c3c3c00, 0x00000000, 0x00000000,
								 0xeceeec00, 0xa8ccec00, 0xbcbcec00, 0xd4b2ec00, 0xecaeec00, 0xecaed400, 0xecb4b000, 0xe4c49000, 0xccd27800, 0xb4de7800, 0xa8e29000, 0x98e2b400, 0xa0d6e400, 0xa0a2a000, 0x00000000, 0x00000000 },

								{0x51515100, 0x00127a00, 0x05009500, 0x2c008b00, 0x4b006100, 0x5a002100, 0x55000000, 0x3d0c0000, 0x19240000, 0x00360000, 0x003d0000, 0x00390000, 0x00294300, 0x00000000, 0x00000000, 0x00000000,
								 0x99999900, 0x0646ce00, 0x3527f100, 0x680fe500, 0x9005ad00, 0xa40c5900, 0x9d210000, 0x7e3f0000, 0x4f5d0000, 0x1c750000, 0x007f0000, 0x00782b00, 0x00638500, 0x00000000, 0x00000000, 0x00000000,
								 0xeaeaea00, 0x5797ff00, 0x8778ff00, 0xb961ff00, 0xe257ff00, 0xf65dab00, 0xef725000, 0xd0900800, 0xa1af0000, 0x6ec60000, 0x45d02800, 0x32ca7d00, 0x38b5d700, 0x3d3d3d00, 0x00000000, 0x00000000,
								 0xeaeaea00, 0xaec8ff00, 0xc1bbff00, 0xd6b2ff00, 0xe7aef300, 0xefb0d000, 0xecb9ab00, 0xdfc58d00, 0xccd27f00, 0xb7dc8400, 0xa6e09a00, 0x9eddbd00, 0xa1d4e200, 0xa3a3a300, 0x00000000, 0x00000000}};

const boolean palette = 1;

int16_t scanLine = 0;
u16 cycle = 0;

u8* ppuRam;// = (u8*)calloc(1 << 15, sizeof(u8));
u8 oamADDR = 0;
u8 OAM[256];
u8 secondaryOAM[SECONDARY_OAM_CAP];
u16 t = 0, v = 0;
u8 x = 0;
boolean w = 0;
boolean isVBlank = false;

u8 spriteContainer[4][4*SECONDARY_OAM_CAP + 8] = { 0 }; /* contains sprite pattern data for a single scanline.
											1st row - raw sprite priority (for this pixel),
											2nd row - calculated pixel output according to secondary OAM index,
											3rd row - palette index for the current pixel.
											4th row - 1 if pixel belongs to sprite 0, 0 otherwise.
											+8 for eventual overflows*/

using namespace std;

boolean isInVBlank() {
	return isVBlank;
}

void copyChrRom(u8* chr) {
	if (chr == NULL) {
		for (u16 t = 0; t < (1 << 13); t++) {
			ppuRam[t] = 0;
		}
	}
	else {
		for (u16 t = 0; t < (1 << 13); t++) {
			ppuRam[t] = chr[t];
		}
	}
}

void updateOam() { // OAMDMA (0x4014) processing routine
	for (u16 i = 0; i <= 0xFF; i++) {
		OAM[oamADDR] = ram[(ram[0x4014] << 8) | i];
		oamADDR++;
	}
}

void writeOAM(u8 what) {
	if (!isVBlank) {
		OAM[oamADDR] = what;
		oamADDR++;
	}
}

u8 readOAM() {
	return OAM[oamADDR];
}

u16 getColor(u16 where) { //compensates specific mirroring for palettes (0x3F10/0x3F14/0x3F18/0x3F1C are mirrors of 0x3F00/0x3F04/0x3F08/0x3F0C)
	if ((where == 0x3F10) || (where == 0x3F14) || (where == 0x3F18) || (where == 0x3F1C)) {
		return where & ~0x10;
	}
	else {
		return where;
	}
}

void incCoarseX() {
	if ((v & 0x1F) == 31) {
		v ^= 0x041F; //reset coarse X and switch horizontal nametable
	}
	else {
		v++;
	}
}

void incFineY() {
	v += 0x1000;
	if (v & 0x8000) {
		switch (v & 0x03E0) {
		case 928:		// switch vertical nametable
			v ^= 0x0800;                   
		case 992:		// coarse Y gets 0, nametable not switched
			v &= ~0x83E0;					
			break;
		default:		//increment coarse Y
			v += 0x8020; 
			break;
		}
	}
}

#define SPRITE_PT ((ram[0x2000] & 0x08) ? 0x1000 : 0x0000) //sprite pattern table address

//renders the next scanline of sprites, in the REVERSE order of how they are stored in OAM2.
// @param spritesQty - number of sprites in OAM2
// @param sp0present - is sprite 0 present in OAM2
void spriteProcessor(u8 spritesQty, bool sp0present) {
	//printf("%d\n", spritesQty);
	memset(spriteContainer[0], 0, SECONDARY_OAM_CAP*4);
	memset(spriteContainer[1], 0, SECONDARY_OAM_CAP*4);
	memset(spriteContainer[2], 0, SECONDARY_OAM_CAP*4);
	memset(spriteContainer[3], 0, SECONDARY_OAM_CAP*4);

	for (int spId = spritesQty-1; spId >= 0; spId--) {
		u8 spY = secondaryOAM[spId * 4 + 0];
		u16 spX = secondaryOAM[spId * 4 + 3];
		u8 spTile = secondaryOAM[spId * 4 + 1];//8x8 only for now
		u8 spAttr = secondaryOAM[spId * 4 + 2];
		bool vFlip = spAttr & 0x80;
		bool hFlip = spAttr & 0x40;
		u8 spPal = spAttr & 0x03;
		u8 spPriority = spAttr & 0x20;

		for (int iX = spX; iX < (spX + 8); iX++) { // each pixel of the sprite

			//computing pixel amplitude
			u8 yIdx = scanLine - spY;
			u8 hShift = iX - spX;

			u8 L = (ppuRam[SPRITE_PT+spTile * 16 + (vFlip ? (7 - yIdx) : yIdx)]     >> (hFlip ? hShift : (7 - hShift))) & 1; // this computes v and h flipping.
			u8 M = (ppuRam[SPRITE_PT+spTile * 16 + (vFlip ? (7 - yIdx) : yIdx) + 8] >> (hFlip ? hShift : (7 - hShift))) & 1; // 8x8 only for now. 
			u8 px = 2 * M + L;

			if (px != 0) {
				spriteContainer[0][iX] = spPriority;
				spriteContainer[1][iX] = px;
				spriteContainer[2][iX] = spPal;
				if ((spId == 0) && (sp0present) && (iX >7) && (iX!=255)) {
					spriteContainer[3][iX] = 1;
				}
			}
		}
	}
}

void spriteEvaluator() {
	static u8 seekN;//position in OAM to look for renderable sprites ( on the next scanline)
	static u8 seekM = 0;//optional positional argument (in case 8 sprites are found, in order to replicate the overflow bug)
	static u8 secOAMCursor;
	static u8 foundSprites;
	static bool sprite0present = false;
	static bool spritesOverflow = false,
				secOAMallow = true;

	if (cycle == 64) { //secondaryOAM initialization
		memset(secondaryOAM, 0xFF, SECONDARY_OAM_CAP);
		seekN = 0;
		sprite0present = false;
		spritesOverflow = false;
		secOAMallow = true;
		secOAMCursor = 0;
		foundSprites = 0;
	}

	else if ((cycle >= 65) && (cycle <= 256)) {//2-Sprite evaluation
		if (!(cycle % 2)) {
			if (seekN < 64) {//if there are still sprites to evaluate (8 sprites per scanline)
				if (foundSprites < 8) {
					u8 yValue = OAM[4 * seekN];
					u8 yDiff = (scanLine - yValue);
					if (yDiff < 8) { // if sprite is in range
						if ((secOAMCursor < (SECONDARY_OAM_CAP-3)) && secOAMallow) { 
							if (!seekN) { // if seekN is 0 and we're here, sprite 0 will be rendered.
								sprite0present = true;
							}
							secondaryOAM[secOAMCursor] = yValue;
							secondaryOAM[secOAMCursor + 1] = OAM[4 * seekN + 1];
							secondaryOAM[secOAMCursor + 2] = OAM[4 * seekN + 2];
							secondaryOAM[secOAMCursor + 3] = OAM[4 * seekN + 3];
							secOAMCursor += 4;
							if (++foundSprites==8) {
								secOAMallow = false;
							}
						}
					}
					seekN++;
				}
				else { // 8 sprites found
					u8 yValue = OAM[4 * seekN + seekM];
					if ((scanLine - yValue) < 8) { // another one found
						spritesOverflow = true; // i need to set the flag @ ram[0x2000]!
						ram[0x2002] |= 0b00100000;
					}
					else {
						seekM++;
						seekN++;
					}
				}
			}
		}
	}
	if (cycle == 321) { // preparing the next scanline here. Computing the result of overlapping sprites.
		spriteProcessor(foundSprites, sprite0present);
	}
}

void PPU::BGRenderer() {
	u16 coarseV = v & 0x0FFF;
	u8 fineY = (v >> 12);
	static u16 attr; // attribute table byte for current element
	static u8 tile; // Tile ID according to the correct nametable, to be used later

	static u8 patternSR1[2], //directly connected to MUX (pattern for current tile)
		patternSR2[2], //pattern for next tile
		preFetchSR[2], //this ones are used to store pattern values for the next tile
		attributeSR[2];//attribute for current tile
	static bool nextTilePaletteLatch[2]; //palette for next tile
	
	static u8 requiredShifting=0; //used to shift attribute byte to the right position (according to v)
	//if ((scanLine == 30)) {//29 for super mario bros
	//	ram[0x2002] |= 0b01000000;
	//}
	
	if (cycle < 257) {
		u8 shift = 1 << (7 - x);
		u8 bgOpaque = ((patternSR1[SR_LSB] & shift) > 0) + 2 * ((patternSR1[SR_MSB] & shift) > 0);
		u16 pixel = 0x3F00 + 4 * (((attributeSR[SR_LSB] & shift) > 0) + 2 * ((attributeSR[SR_MSB] & shift) > 0)) //offset to right palette
								 + bgOpaque; // actual pattern data

		u16 d = getColor(pixel);
		u16 tmp = BG_RENDERING && (d & 3) ? d : 0x3F00; //default background color 
		u32 bgPIX = colors[palette][ppuRam[tmp]];

		u16 spriteOpaque = spriteContainer[1][cycle-1];
		bool priority = spriteContainer[0][cycle - 1];

		u16 tmppx= 0x3f10 + 4 * (spriteContainer[2][cycle-1]) + spriteOpaque;
		u32 spritePIX = colors[palette][ppuRam[getColor(tmppx)]];

		if (spriteContainer[3][cycle - 1] && ((spriteOpaque > 0) && (bgOpaque > 0))) {
			ram[0x2002] |= 0b01000000;
		}

		pixels[scanLine * SCREEN_WIDTH + cycle - 1] =  (bgOpaque && spriteOpaque) ? (priority ? bgPIX : spritePIX) : (spriteOpaque ? spritePIX : bgPIX);

		// can't decide between two? check priority
	}
	if (cycle < 337) {
		patternSR1[SR_LSB] <<= 1;
		patternSR1[SR_MSB] <<= 1;

		patternSR1[SR_LSB] |= (patternSR2[SR_LSB] & 0x80) > 0;
		patternSR1[SR_MSB] |= (patternSR2[SR_MSB] & 0x80) > 0; //get the MSB and shift it back into pSR1

		patternSR2[SR_LSB] <<= 1;
		patternSR2[SR_MSB] <<= 1;

		attributeSR[SR_LSB] <<= 1;
		attributeSR[SR_MSB] <<= 1;

		attributeSR[SR_LSB] |= (nextTilePaletteLatch[SR_LSB]);
		attributeSR[SR_MSB] |= (nextTilePaletteLatch[SR_MSB]);
	}
	
	switch (cycle % 8) { // fetchings
		case NT_CYCLE:
			tile = ppuRam[0x2000 | coarseV]; // raw 2x2 tile ID fetch
			break;
		case AT_CYCLE:
			attr = ppuRam[0x23C0 | (v & 0x0C00) | ((v >> 4) & 0x38) | ((v >> 2) & 0x07)];
			requiredShifting = 2 * (2 * ((coarseV  & 64) != 0) + ((coarseV & 2) != 0));// 4 quadrants, 2 bits each. oddness of y is the most significant bit.
			break;
		case BG_LSB_CYCLE:
			preFetchSR[SR_LSB] = ppuRam[PAT_TB_PREFIX + 16 * tile + fineY];
			break;
		case BG_MSB_CYCLE:
			preFetchSR[SR_MSB] = ppuRam[PAT_TB_PREFIX + 16 * tile + fineY + 8];
			break;
		case H_INC_CYCLE:
			nextTilePaletteLatch[SR_LSB] =(attr >> requiredShifting) & 1;
			nextTilePaletteLatch[SR_MSB] = (attr >> requiredShifting) & 2;
			patternSR2[SR_LSB] = preFetchSR[SR_LSB];
			patternSR2[SR_MSB] = preFetchSR[SR_MSB];
			incCoarseX();
			break;
		default:
			break;
	}
	//drawSprites();
}

u8 frame = 0;
inline void PPU::sequencer() {
	 
	if (scanLine < 240) {
		if (RENDERING) {
			if (SPRITE_RENDERING) {
				spriteEvaluator();
			}

			if (((cycle > 0) && (cycle < 257)) || ((cycle > 320))) {
				BGRenderer();
				if (cycle == 256) {
					incFineY();
				}
			}
			
			else if (cycle == 257) {
				v = (v & ~0x41F) | (0x41F & t); // hor (v) = hor(t)
			}
		}

		if ((cycle >= 257) && (cycle <= 320)) {
			oamADDR = 0;
		}
	}

	else if (scanLine == 241) {
		if (cycle == 241) {
			isVBlank = true;
			scr.updateScreen();
			ram[0x2002] |= 0b10000000;
			if (ram[0x2000] & 0x80)_nmi();
		}
	}

	else if (scanLine == 261) {
		if (RENDERING) {
			if ((cycle < 256) && ((cycle % 8) == 0)) {
				incCoarseX();
			}
			else if (cycle == 257) {
				v = (v & ~0x41F) | (0x41F & t); // hor (v) = hor(t)
			}
			else if ((cycle >= 280) && (cycle <= 304)) {
				v = (v & ~0x7BE0) | (t & 0x7BE0); // vert (v) = vert(t)
			}
			else if (cycle > 320) {
				BGRenderer();
			}
		}

		if (cycle == 1) {
			ram[0x2002] &= ~0b11100000;
			isVBlank = false;
			frame++;
		}

		else if ((cycle >= 257) && (cycle <= 320)) {
			oamADDR = 0;
		}
	}
}

void PPU::tick() {
	static bool oddFrame = false;
	if (cycle / 341) {
		scanLine++;
		cycle = 0;
	}
	if ((scanLine == 261) && (cycle == 339)) {
		if (oddFrame) {
			cycle = 0;
			scanLine = 0;
		}
		oddFrame ^= 1;
	}
	scanLine %= 262;
	sequencer();
	cycle++;
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
	printf("PPU Startup: ");
	if (ppuRam) {
		printf(" OK!\n");
	}
	else {
		printf(" FAIL! could not allow video memory.\n");
		exit(1);
	}
	printf("Mirroring type: %s\n", (mirror == HORIZONTAL) ? "Horizontal" : "Vertical");
}

void writePPU(u8 what) {

	if ((V >= 0x3000) && (V <= 0x3EFF)) {
		ppuRam[V - 0x1000] = what;
	}
	else if (V > 0x3EFF) {
		ppuRam[getColor(V)] = what;
	}
	else {
		ppuRam[V] = what;

		if ((V >= 0x2000) && (V<=0x3000)) {
			if (mirror == VERTICAL) {
				if (V < 0x2800) {
					ppuRam[V + 0x800] = what;
				}
				else if (V < 0x3000) {
					ppuRam[V - 0x800] = what;
				}
			}
			else if (mirror == HORIZONTAL) {
				if (V < 0x2400) {
					ppuRam[V + 0x400] = what;
				}
				else if (V < 0x2800) {
					ppuRam[V - 0x400] = what;
				}
				else if (V < 0x2C00) {
					ppuRam[V + 0x400] = what;
				}
				else if (V < 0x3000) {
					ppuRam[V - 0x400] = what;
				}
			}
		}
		
	}
	incPPUADDR();
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
	else if ((V >= 0x3EFF) && (V <= 0x3F1F)) {

		internalPPUreg = ppuRam[getColor(V)];
		incPPUADDR();
		return internalPPUreg;
	}
	else {

		if (V >= 0x2000) {

				u8 tmp = internalPPUreg;

				internalPPUreg = ppuRam[getColor(V)];
				incPPUADDR();
				return tmp;
		}
		else {
			u8 tmp = internalPPUreg;

			internalPPUreg = ppuRam[V];
			incPPUADDR();
			return tmp;
		}

	}

}

inline void incPPUADDR() {
	v += (ram[0x2000] & 4) ? 32 : 1;
}