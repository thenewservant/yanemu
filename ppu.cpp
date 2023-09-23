#include "ppu.h"
#include "firstAPU.h"
#define HOW_MANY_PT_SLOTS 1

u8 oamADDR = 0;
u8** ppuMemSpace = (u8**)calloc(1 + 4, sizeof(u8*)); // for now, 1 slot for PT, 4 for NT.
u8 spriteContainer[4][4*SECONDARY_OAM_CAP + 8] = { 0 }; /* contains sprite pattern data for a single scanline.
											1st row - raw sprite priority (for this pixel),
											2nd row - calculated pixel output according to secondary OAM index,
											3rd row - palette index for the current pixel.
											4th row - 1 if pixel belongs to sprite 0, 0 otherwise.
											+8 for eventual overflows*/


u16 PPU::paletteMap(u16 where) { //compensates specific mirroring for palettes (0x3F10/0x3F14/0x3F18/0x3F1C are mirrors of 0x3F00/0x3F04/0x3F08/0x3F0C) till 0x3FFF
	u16 tmp = where & 0x3F1F;
	if (tmp & 0x0003) {
		return tmp;
	} else if (tmp & 0x0010) {
		return tmp & ~0x0010;
	}
	else {
		return tmp;
	}
}

void PPU::wrNT(u16 where, u8 what) {
	u8 ntID = (where >> 10) & 3;
	switch (mirror) {
	case HORIZONTAL:
		ppuMemSpace[HOW_MANY_PT_SLOTS +(ntID >> 1)][where & 0x03FF] = what;
		break;
	case VERTICAL:
		ppuMemSpace[HOW_MANY_PT_SLOTS +(ntID & 1)][where & 0x03FF] = what;
		break;
	}
}

void PPU::wrVRAM(u8 what) {
	switch (V & 0x7000) {
	case 0x0000:
	case 0x1000:
		 mapper->wrPPU(V, what);
		 break;
	case 0x2000:
		 wrNT(V, what);
		 break;
	case 0x3000:
		if (V >= 0x3F00) {
			ppuRam[paletteMap(V)] = what;
		} else {
			wrNT(V, what);
		}
		break;
	default:
		break;
	}
}

u8 PPU::rdNT(u16 where) {
	u8 ntID = (where >> 10)&3;
	switch (mirror) {
	case HORIZONTAL:
		return ppuMemSpace[HOW_MANY_PT_SLOTS +(ntID >> 1)][where & 0x03FF];
		break;
	case VERTICAL:
		return ppuMemSpace[HOW_MANY_PT_SLOTS +(ntID&1)][where & 0x03FF];
		break;
	default:
		break;
	}
	return 0;
}

// direct VRAM access, no increment of v
u8 PPU::rdVRAM(u16 where) {
	//Three main areas; pattern tables (0x0000-0x1FFF), nametables (0x2000-0x2FFF), palettes (0x3F00-0x3FFF).
	//The first four (actually two) bits of the address determine which area is being accessed
	switch (where & 0x3000) {
	case 0x0000:
	case 0x1000:
		return mapper->rdPPU(where);
	case 0x2000:
		return rdNT(where);
	case 0x3000:
		if (where >= 0x3F00) {// 0x3000 - 0x3FFF are mirrors of 0x2000 - 0X2FFF
			return ppuRam[paletteMap(where)];
		}
		else {
			return rdNT(where);
		}
	default:
		break;
	}
	return 0;
}

void PPU::updateOam() { // OAMDMA (0x4014) processing routine
	for (u16 i = 0; i <= 0xFF; i++) {
		OAM[oamADDR] = rd((ram[0x4014] << 8) | i);
		oamADDR++;
	}
	if (firstAPUCycleHalf()) {
		addCycle();
	}
}

void PPU::writeOAM(u8 what) {
	if (!EITHER_RENDERING) {
		OAM[oamADDR] = what;
		oamADDR++;
	}
}

u8 PPU::readOAM() {
	return OAM[oamADDR];
}

void PPU::incCoarseX() {
	if ((regs.v & 0x1F) == 31) {
		regs.v ^= 0x041F; //reset coarse X and switch horizontal nametable
	}
	else {
		regs.v++;
	}
}

void PPU::incFineY() {
	regs.v += 0x1000;
	if (regs.v & 0x8000) {
		switch (regs.v & 0x03E0) {
		case 928:		// switch vertical nametable
			regs.v ^= 0x0800;                   
		case 992:		// coarse Y gets 0, nametable not switched
			regs.v &= ~0x83E0;					
			break;
		default:		//increment coarse Y
			regs.v += 0x8020;
			break;
		}
	}
}

void PPU::spriteProcessor(u8 spritesQty, bool sp0present) {
	for (int spId = spritesQty - 1; spId >= 0; spId--) {
		u8 spY = secondaryOAM[spId * 4 + 0];
		u8 spX = secondaryOAM[spId * 4 + 3];
		u8 spTile = secondaryOAM[spId * 4 + 1];
		u8 spAttr = secondaryOAM[spId * 4 + 2];
		bool vFlip = spAttr & 0x80;
		bool hFlip = spAttr & 0x40;
		u8 spPal = spAttr & 0x03;
		bool spPriority = spAttr & 0x20;

		for (int iX = spX; iX < (spX + SP_MODE); iX++) { // each pixel of the sprite
			//computing pixel amplitude
			u8 yIdx = (u8)scanLine - spY;
			u8 hShift = (u8)iX - spX;
			u8 L = 0, M = 0; // L(SB) and M(SB) are the two Pattern bits of the pixel 

			switch (SP_MODE) { // this computes v and h flipping.
			case 8:
				L = (rdVRAM(SPRITE_PT + spTile * 16 + (vFlip ? (7 - yIdx) : yIdx)) >> (hFlip ? hShift : (7 - hShift))) & 1;
				M = (rdVRAM(SPRITE_PT + spTile * 16 + (vFlip ? (7 - yIdx) : yIdx) + 8) >> (hFlip ? hShift : (7 - hShift))) & 1;
				break;
			case 16:
				u16 pt = spTile & 1 ? 0x1000 : 0;
				u8 spTile16 = spTile & 0xFE;
				int8_t wrap = (yIdx > 7) ? (vFlip ? -8 : 8) : 0;
				L = (rdVRAM(pt + spTile16 * 16 + (vFlip ? (23 - yIdx) : yIdx) + wrap) >> (hFlip ? hShift : (7 - hShift))) & 1;
				M = (rdVRAM(pt + spTile16 * 16 + (vFlip ? (23 - yIdx) : yIdx) + 8 + wrap) >> (hFlip ? hShift : (7 - hShift))) & 1;
				break;
			}

			u8 px = 2 * M + L;

			if (px != 0) {
				spriteContainer[0][iX] = spPriority;
				spriteContainer[1][iX] = px;
				spriteContainer[2][iX] = spPal;
				if ((spId == 0) && (sp0present) && (iX != 255)) {
					spriteContainer[3][iX] = 1;
				}
			}
		}
	}
}

// Evaluates the next scanline of sprites, and stores them in OAM2.
void PPU::spriteEvaluator() {
	static u8 seekN = 0;//position in OAM to look for renderable sprites ( on the next scanline)
	static u8 seekM = 0;//optional positional argument (in case 8 sprites are found, in order to replicate the overflow bug)
	static u8 secOAMCursor = 0;
	static u8 foundSprites = 0;
	static bool sprite0present = false;
	static bool secOAMallow = true;

	if (cycle == 64) {// resets
		seekN = 0;
		sprite0present = false;
		secOAMallow = true;
		secOAMCursor = 0;
		foundSprites = 0;
		seekM = 0;
	}

	else if ((cycle >= 65) && (cycle <= 256)) {//2-Sprite evaluation
		if (!(cycle % 2)) {
			if (seekN < 64) {//if there are still sprites to evaluate (8 sprites per scanline)
				if (foundSprites < 8) {
					u8 yDiff;
					u8 yValue = OAM[4 * seekN];
					if (yValue <= 239) {
						yDiff = (u8)(scanLine - yValue);
					}
					else {
						yDiff = 17;// so that sprites with Y value > 239 are not rendered (17 is strictly greater than 8 and 16, the max height of a sprite (resp. double height sprite))
					}

					if (yDiff < SP_MODE) { // if sprite is in range
						if ((secOAMCursor < (SECONDARY_OAM_CAP - 3)) && secOAMallow) {
							if (!seekN) { // if seekN is 0 and we're here, sprite 0 will be rendered.
								sprite0present = true;
							}
							memcpy(&secondaryOAM[secOAMCursor], &OAM[4 * seekN], 4); // copy the sprite to one slot of secondary OAM
							secOAMCursor += 4;
							if (++foundSprites == 8) {
								secOAMallow = false;
							}
						}
					}
					seekN++;
				}
				else { // 8 sprites found
					u8 yValue = OAM[4 * seekN + seekM];
					if ((scanLine - yValue) < SP_MODE) { // another one found
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
	else if (cycle == 321) { // preparing the next scanline here. Computing the result of overlapping sprites.
		memset(spriteContainer, 0, 4 * (SECONDARY_OAM_CAP * 4 + 8)); // empty the next scanline's buffer
		spriteProcessor(foundSprites, sprite0present);
	}
}

// Renders the background for the current cycle.
// Buffers, in parallel, the next tile's pattern and attribute bytes.
void PPU::BGRenderer() {
	static u16 attr; // attribute table byte for current element
	static u8 tile; // Tile ID according to the correct nametable, to be used later

	static u16 patternSR[2]; // this one does the job for initially two shift registers. fewer operations are needed to replicate the same circuitry.
	static u8 attributeSR[2],//attribute for current tile
			  preFetchSR[2]; //this ones are used to store pattern values for the next tile
	static bool nextTilePaletteLatch[2]; //palette for next tile

	static u8 requiredShifting = 0; //used to shift attribute byte to the right position (according to v)

	u16 shift = 0; // necessary amount to replicate both 2C02 shifting circuitry, as well as the fusion of the two shift registers into one.
	u8 bgOpaque = 0; // used to determine the color depth of the current pixel, thus its opacity. (0 is transparent)
	u8 spriteOpaque = 0; // same as above, but for sprites.
	u16 bgVramColor = 0; // used to store what will be the color index (in the color table) of the current background pixel, according to the palette and the pattern data.
	u16 spVramColor = 0; // same as above, but for sprites.
	bool priority = false; // priority of 1 <=> true : sprite is BEHIND background, 0 <=> false : sprite is IN FRONT of background.
	u16 bgTmp = 0x3F00; // checks need to be made, involving PPUMASK bits, before getting the u32 color
	u16 spTmp = 0x3F00; // same as above, but for sprites.
	u32 bgPIX = 0; // the actual color of the background pixel
	u32 spritePIX = 0; // same as above, but for sprites.

	if (cycle < 257) {
		if (BG_RENDERING && (LEFTMOST_BG_SHOWN || ((cycle) > 8))) {
			shift = 1 << ((7 - regs.x));
			bgOpaque = ((patternSR[SR_LSB] & (shift << 8)) > 0) + 2 * ((patternSR[SR_MSB] & (shift << 8)) > 0);
			bgVramColor = 0x3F00 + 4 * (((attributeSR[SR_LSB] & shift) > 0) + 2 * ((attributeSR[SR_MSB] & shift) > 0)) //offset to right palette
				+ bgOpaque; // actual pattern data
			bgTmp = (bgVramColor & 3) ? bgVramColor : 0x3F00; //default background color
		}
		
		bgPIX = colors[palette][rdVRAM(bgTmp)];// even if LEFTMOST_BG_SHOWN or BG_RENDERING is false, bgPIX will be at least color at 0x3F00 (default background color)

		if (SPRITE_RENDERING && (LEFTMOST_SPRITES_SHOWN || ((cycle) > 8))) {
			spriteOpaque = spriteContainer[1][cycle - 1];
			priority = spriteContainer[0][cycle - 1];
			spVramColor = 0x3F10 + 4 * (spriteContainer[2][cycle - 1]) + spriteOpaque;
			spTmp = (spVramColor & 3) ? spVramColor : 0x3F10;
			spritePIX = colors[palette][rdVRAM(spTmp)];
		}

		if (spriteContainer[3][cycle - 1] && ((spriteOpaque > 0) && (bgOpaque > 0)) && BOTH_RENDERING) { //sprite 0 hit detection
			if ((((cycle - 1) >= 8) || LEFTMOST_BOTH_SHOWN) ){
				ram[0x2002] |= 0b01000000;
			}
		}

		switch (VIDEO_MODE) {
		case NTSC:
			if ((scanLine > 7) && (scanLine < 232)) {
				pixels[(scanLine - 8) * SCREEN_WIDTH + cycle - 1] = (bgOpaque && spriteOpaque) ? (priority ? bgPIX : spritePIX) : ((spriteOpaque&&SPRITE_RENDERING) ? spritePIX : bgPIX);
			}
			break;
		case PAL:
			//not implemented yet. no 8-line-crop on top and bottom.
			break;
		default:
			break;
		}
	}
	if (cycle < 337) {
		patternSR[SR_LSB] <<= 1;
		patternSR[SR_MSB] <<= 1;

		attributeSR[SR_LSB] <<= 1;
		attributeSR[SR_MSB] <<= 1;

		attributeSR[SR_LSB] |= (nextTilePaletteLatch[SR_LSB]);
		attributeSR[SR_MSB] |= (nextTilePaletteLatch[SR_MSB]);
	}

	switch (cycle % 8) { // fetchings
	case NT_CYCLE:
		tile = rdVRAM(0x2000 | COARSE_V); // raw 2x2 tile ID fetch
		break;
	case AT_CYCLE:
		attr = rdVRAM(0x23C0 | (regs.v & 0x0C00) | ((regs.v >> 4) & 0x38) | ((regs.v >> 2) & 0x07));
		requiredShifting = 2 * (2 * ((COARSE_V & 64) != 0) + ((COARSE_V & 2) != 0));// 4 quadrants, 2 bits each. oddness of y>>1 is the most significant bit.
		break;
	case BG_LSB_CYCLE:
		preFetchSR[SR_LSB] = rdVRAM(PAT_TB_PREFIX + 16 * tile + FINE_Y);
		break;
	case BG_MSB_CYCLE:
		preFetchSR[SR_MSB] = rdVRAM(PAT_TB_PREFIX + 16 * tile + FINE_Y + 8);
		break;
	case H_INC_CYCLE:
		nextTilePaletteLatch[SR_LSB] = (attr >> requiredShifting) & 1;
		nextTilePaletteLatch[SR_MSB] = (attr >> requiredShifting) & 2;
		patternSR[SR_LSB] |= preFetchSR[SR_LSB];
		patternSR[SR_MSB] |= preFetchSR[SR_MSB];
		incCoarseX();
		break;
	default:
		break;
	}
}

void PPU::sequencer() {
	static bool canFinallyNMI = false;
	if (scanLine < 240) {
		if (EITHER_RENDERING) {
				spriteEvaluator();

			if (((cycle > 0) && (cycle < 257)) || ((cycle > 320))) {
				BGRenderer();
				
				if (cycle == 256) {
					incFineY();
				}
			}
			
			else if (cycle == 257) {
				regs.v = (regs.v & ~0x41F) | (0x41F & regs.t); // hor (v) = hor(t)
			}
		}
		else {
			if ((V >= 0x3F00) && (cycle > 0) && (cycle < 257) && (scanLine>7) && (scanLine <232)) {
			pixels[(scanLine - 8) * SCREEN_WIDTH + cycle - 1] = colors[1][rdVRAM(V)];
			}
		}

		if ((cycle >= 257) && (cycle <= 320)) {
			oamADDR = 0;
		}
	}
	
	else if (scanLine == 241) {
		if (cycle == 1) {
			scr.updateScreen();
			if (maySetVBlankFlag) {
				ram[0x2002] |= 0b10000000;
			}
			if ((ram[0x2000] & 0x80)) {
				nmiPending = true;
			}
		}
	}
	if ((scanLine >= 241) && (scanLine <= 261)) {
		if (nmiPending && !getCycles()) {
			canFinallyNMI = true;
		}
		if (canFinallyNMI && getCycles()) {
			_nmi();
			canFinallyNMI = false;
			nmiPending = false;
		}
	}

	if (scanLine == 261) {
		if (EITHER_RENDERING) {
			if ((cycle < 256) && ((cycle % 8) == 0)) {
				incCoarseX();
			}
			else if (cycle == 257) {
				regs.v = (regs.v & ~0x41F) | (0x41F & regs.t); // hor (v) = hor(t)
			}
			else if ((cycle >= 280) && (cycle <= 304)) {
				regs.v = (regs.v & ~0x7BE0) | (regs.t & 0x7BE0); // vert (v) = vert(t)
			}
			else if (cycle > 320) {
				BGRenderer();
			}
		}

		if (cycle == 1) {
			maySetVBlankFlag = true;
			memset(spriteContainer, 0, 4 * (SECONDARY_OAM_CAP * 4 + 8)); // empty the next scanline's sprite buffer
			ram[0x2002] &= ~0b11100000; // clear sprite overflow, sprite 0 hit, and vblank flags all at once
			frame++;
		}

		else if ((cycle >= 257) && (cycle <= 320)) {
			oamADDR = 0;
		}
	}
}

void PPU::tick() {
	if (cycle / 341) {
		scanLine++;
		cycle = 0;
	}
	if ((scanLine == 261) && (cycle == 340) && EITHER_RENDERING) {
		if (oddFrame ) {
			cycle = 0;
			scanLine = 0;
			
		}
		OverrideFrameOddEven = true;
		oddFrame ^= 1;
	}
	if ((scanLine == 0) && (cycle == 0)) {
		maySetVBlankFlag = true;
		mayTriggerNMI = true;
	}

	scanLine %= 262;
	sequencer();
	cycle++;
}

PPU::PPU(u32* px, Screen sc, Rom rom) :
	OAM{0}, internalPPUreg(0), frame(0), cycle(0), scanLine(261),
	isVBlank(false), scr(sc), pixels(px), secondaryOAM{0}, regs{0},
	ppuRam{0}, mayTriggerNMI{ true }, maySetVBlankFlag{ true }
{
	ram[0x2002] = 0b10000000;
	this->mapper = rom.getMapper();
	printf("PPU Started.\n");
}

u8 PPU::readPPUSTATUS() {
	if ((scanLine == 241)) {
		if ((cycle == 1)) { // passes blargg's test, but cycle should be 0 (one ppu cycle before setting )
			maySetVBlankFlag = false;
			mayTriggerNMI = false;
		}
	}
	
	regs.w = 0;
	u8 tmp = ram[0x2002];
	ram[0x2002] &= ~0b10000000;
	return tmp;
}

void PPU::writePPUCTRL(u8 what) {
	regs.t = ((what & 0x3) << 10) | (regs.t & 0x73FF);
	if ((ram[0x2002] & 0x80) && (!(ram[0x2000] & 0x80)) && (what & 0x80)) {
		nmiPending=true;
	}
	ram[0x2000] = what;
}

void PPU::writePPUMASK(u8 what) {
	if (!(what & 0x18)) {
		OverrideFrameOddEven = true;
	}
}

void PPU::writePPUSCROLL(u8 what) {
	if (regs.w == 0) {
		regs.t = ((what & ~0b111) >> 3) | (regs.t & 0b111111111100000);
		regs.x = what & 0b111;
	}
	else {
		regs.t = ((what & ~0b111) << 2) | (regs.t & 0b111110000011111);
		regs.t = ((what & 0b111) << 12) | (regs.t & 0b000111111111111);
	}
	regs.w ^= 1;
}

void PPU::writePPUADDR(u8 what) {
	if (regs.w == 0) {
		regs.t = ((what & 0b111111) << 8) | (regs.t & 0b00000011111111);
	}
	else {
		regs.t = (regs.t & 0xFF00) | what;
		regs.v = regs.t;
	}
	regs.w ^= 1;
}

void PPU::writePPU(u8 what) {
	wrVRAM(what);
	incPPUADDR();
}

u8 PPU::rdPPU() {
	 if ((V & 0xFF00) == 0x3F00) {
		internalPPUreg = rdVRAM(V-0x1000);
		u8 tmp = rdVRAM(V);
		incPPUADDR();
		return tmp;
	 }
	 else {
		 u8 tmp = internalPPUreg;
		 internalPPUreg = rdVRAM(V);
		 incPPUADDR();
		 return tmp; 
	 }
}

inline void PPU::incPPUADDR() {
	regs.v += (ram[0x2000] & 4) ? 32 : 1;
}