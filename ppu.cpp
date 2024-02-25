<<<<<<< HEAD
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
#define NT_PREFIX (0x2000 | (ram[0x2000] & 3) * 0x400)
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

#define SPRITE_0_BIT 0b00000000000000000001000000000000

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

	u16 offsetXScroll = ((xScroll / 8 + cycle / 8) / 32)* (0x3E0);

	u8 attribute = ppuRam[offsetXScroll+NT_PREFIX + 0x3C0 + ((cycle+xScroll) / 32) + 8 * (scanl / 32)]; // raw 2x2 tile attribute fetch
	u8 value = ((attribute >> (4 * ((scanl / 16) % 2))) >> (2 * (((cycle+xScroll) / 16) % 2))) & 3;	  // Temporary solution. MUST be opitmized later

	//Background rendering
	
	

	for (u8 k = 0; k < 8; k++){
		if ( (ram[0x2001] & 0b00001000)) {

			u16 destination = 0x3F00 + 4 * value + ((ppuRam[(scanl % 8)		+ PAT_TB_PREFIX + 16 * ppuRam[offsetXScroll + NT_PREFIX + xScroll/8+((cycle) / 8) + 32 * (scanl / 8)]] >> (7 - k)) & 1) + 2 *

												   ((ppuRam[(scanl % 8) + 8 + PAT_TB_PREFIX + 16 * ppuRam[offsetXScroll + NT_PREFIX + (xScroll/8)+((cycle) / 8) + 32 * (scanl / 8)]] >> (7 - k)) & 1);

			u16 t = getColor(destination);
			u16 tmp = (t &3)? t:0x3F00;

			pixels[scanl * SCREEN_WIDTH + cycle + k] = colors[ppuRam[tmp]] + (!(tmp&3)? 1:0); // if the given color is identified as opaque, add 1, as it will be represented by SDL as slightly opaque, thus letting the ppu know this is a background color (will be used for sprite 0 evaluation)
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
=======
#include "ppu.h"

u16 PPU::paletteMap(u16 where) { //compensates specific mirroring for palettes (0x3F10/0x3F14/0x3F18/0x3F1C are mirrors of 0x3F00/0x3F04/0x3F08/0x3F0C) till 0x3FFF
	u16 tmp = where & 0x001F;
	if (!(tmp & 0x0003) && (tmp & 0x0010)) {
		return tmp & ~0x0010;
	}
	else {
		return tmp;
	}
}

void PPU::wrVRAM(u8 what) {
	switch (V & 0x7000) {
	case 0x0000:
	case 0x1000:
		mapper->wrPPU(V, what);
		break;
	case 0x2000:
		mapper->wrNT(V, what);
		break;
	case 0x3000:
		if (V >= 0x3F00) {
			paletteMem[paletteMap(V)] = what;
		}
		else {
			mapper->wrNT(V, what);
		}
		break;
	default:
		break;
	}
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
		return mapper->rdNT(where);
	case 0x3000:
		if (where >= 0x3F00) {// 0x3000 - 0x3F00 are mirrors of 0x2000 - 0X2F00
			return paletteMem[paletteMap(where)];
		}
		else {
			return mapper->rdNT(where);
		}
	default:
		break;
	}
	return 0;
}

void PPU::incCoarseX() {
	if ((scrollRegs.v & 0x1F) == 31) {
		scrollRegs.v ^= 0x041F; //reset coarse X and switch horizontal nametable
	}
	else {
		scrollRegs.v++;
	}
}

void PPU::incFineY() {
	scrollRegs.v += 0x1000;
	if (scrollRegs.v & 0x8000) {
		switch (scrollRegs.v & 0x03E0) {
		case 928:		// switch vertical nametable
			scrollRegs.v ^= 0x0800;
		case 992:		// coarse Y gets 0, nametable not switched
			scrollRegs.v &= ~0x83E0;
			break;
		default:		//increment coarse Y
			scrollRegs.v += 0x8020;
			break;
		}
	}
}

//computes the adequate pixel, according to flips, for the current cycle of the NEXT line, and stores it in spriteContainer.
void PPU::spriteProcessor(u8 spritesQty, bool sp0present) {
	for (int spId = spritesQty - 1; spId >= 0; spId--) { // each sprite in secondary OAM (max 8 sprites per scanline)
		u8 spY = secondaryOAM[spId * 4 + 0];
		u8 spX = secondaryOAM[spId * 4 + 3];
		u8 spTile = secondaryOAM[spId * 4 + 1];
		u8 spAttr = secondaryOAM[spId * 4 + 2];
		bool vFlip = spAttr & 0x80; // 0 : scan sprite top to bottom, 1 : bottom to top (symetric vertical flipping)
		bool hFlip = spAttr & 0x40; // 0 : scan sprite left to right, 1 : right to left (symetric horizontal flipping)

		for (int iX = spX; iX < (spX + 8); iX++) { // each pixel of the sprite
			u8 yIdx = (u8)(scanLine - spY);
			u8 L = 0, M = 0; // L(SB) and M(SB) are the two Pattern bits of the pixel 
			u16 finalSpriteByte = 0; // the adress of the byte in CHR-ROM that contains the pixel data.

			// this switch computes the adress of the byte in CHR-ROM that contains the pixel data.
			if (SP_MODE == 8) { 
				u16 sprite8Adress = SPRITE_PT + spTile * 16; // adress of sprite tile in CHR-ROM
				u8 vFlipOffset = (vFlip ? (7 - yIdx) : yIdx); // if vFlip is needed.
				finalSpriteByte = sprite8Adress + vFlipOffset; // adress of the byte in CHR-ROM
			}
			else { // 8x16 sprites
				u16 pt = spTile & 1 ? 0x1000 : 0; // pattern table adress (0x2001 in RAM is ignored when in 8x16 mode)
				u16 sprite16Adress = pt + 16 * (spTile & 0xFE); // adress of sprite tile in CHR-ROM
				int8_t wrap = (yIdx > 7) ? (vFlip ? -8 : 8) : 0; // if the pixel is in the second tile, wrap is needed. -8 if vFlip is needed, 8 otherwise.
				u8 vFlipOffset = (vFlip ? (23 - yIdx) : yIdx); // if vFlip is needed. 23 is the max height (0-counting) of a 8x16 sprite. And an 8x16 vFlip means ALL sprite flipped.
				finalSpriteByte = sprite16Adress + vFlipOffset + wrap;
			}

			u8 hShift = (u8)(iX - spX);
			if (!hFlip) { // final pixel selection.
				hShift = 7 - hShift; // 7 - hShift means reading from left to right incrementally. (Normal scenario)
			}

			L = (mapper->rdPPU(finalSpriteByte) >> hShift) & 1;// One pixel from finalSpriteByte and finalSpriteByte + 8, at the same position in both, will be extracted
			M = (mapper->rdPPU(finalSpriteByte + 8) >> hShift) & 1;// MSB is exactly 8 bytes away from LSB
			u8 px = 2 * M + L; // 2*M + L is the color index of the pixel (4 values possible)

			if (px != 0) { // if the pixel is not transparent
				spriteContainer[iX] = spAttr & 0x20; //Sprite priority
				spriteContainer[iX] |= px;
				spriteContainer[iX] |= 4 * (spAttr & 0x03); //Palette ID
				if ((spId == 0) && (sp0present) && (iX != 255)) {
					spriteContainer[iX] |= 0x80; // Sprite 0 hit indicator. (1 if pixel belongs to sprite 0, 0 otherwise)
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

	else if ((cycle >= 65) && (cycle <= 256)) 
	{
		if ((cycle % 2) || (seekN >= 64)) 
		{
			return;
		}
		else if (foundSprites < 8) //if there are still sprites to evaluate (8 sprites per scanline)
		{ 
			u8 yValue = OAM[4 * seekN];

			if ((yValue <= 239) 
				&& ((u8)(scanLine - yValue) < SP_MODE)) // if sprite is in range
			{ 
				if ((secOAMCursor < (SECONDARY_OAM_CAP - 3)) && secOAMallow) {
					if (seekN == 0) { // if seekN is 0 and we're here, sprite 0 will be rendered.
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
	else if (cycle == 321) { // preparing the next scanline here. Computing the result of overlapping sprites.
		memset(spriteContainer, 0, SECONDARY_OAM_CAP * 4); // empty the next scanline's buffer
		spriteProcessor(foundSprites, sprite0present);
	}
}

void PPU::rendererComputeSpritePixel(const u8& spriteOpaque, u32& finalPIX)
{
	if (SPRITE_RENDERING && (LEFTMOST_SPRITES_SHOWN || (cycle > 8))) 
	{
		u8 spVramColor = (spriteContainer[cycle - 1] & 0xC) + spriteOpaque;
		u16 spTmp = (spVramColor & 3) ? (spVramColor|0x3F10) : 0x3F10;
		finalPIX = colors[paletteMem[paletteMap(spTmp)]];
	}
}

void PPU::rendererComputeBGPixel(u8 attributeSR[2], const u16& shift, const u8& bgOpaque, u32& finalPIX)
{
	if (BG_RENDERING && (LEFTMOST_BG_SHOWN || (cycle > 8))) 
	{
		u8 bgVramColor = 4 * (((attributeSR[SR_LSB] & shift) > 0) 
					   + 2 *  ((attributeSR[SR_MSB] & shift) > 0)) //offset to right palette
					   +        bgOpaque; // actual pattern data
		u16 bgTmp = (bgVramColor & 3) ? (bgVramColor | 0x3F00) : 0x3F00; //default background color
		finalPIX = colors[paletteMem[paletteMap(bgTmp)]];// even if LEFTMOST_BG_SHOWN or BG_RENDERING is false, bgPIX will be at least color at 0x3F00 (default background color)
	}
}

// Renders the background for the current cycle.
// Buffers, in parallel, the next tile's pattern and attribute bytes.
void PPU::pixelRenderer() {
	static u16 attr; // attribute table byte for current element
	static u16 tile; // Tile ID according to the correct nametable
	static u16 patternSR[2]; // this one does the job for initially two shift registers. fewer operations are needed to replicate the same circuitry.
	static u8 attributeSR[2],//attribute for current tile
			  preFetchSR[2]; //these ones are used to store pattern values for the next tile
	static bool nextTilePaletteLatch[2]; //palette for next tile
	static u8 requiredShifting = 0; //used to shift attribute byte to the right position (according to v)

	u16 shift = 0; // necessary amount to replicate both 2C02 shifting circuitry, as well as the fusion of the two shift registers into one.
	u8 bgOpaque = 0; // used to determine the color depth of the current pixel, thus its opacity. (0 is transparent)
	u8 spriteOpaque = 0; // same as above, but for sprites.
	u32 finalPIX = 0; // final pixel color, to be stored in the pixel buffer.

	if (cycle < 257) 
	{
		shift = 1 << (7 - scrollRegs.x);
		bgOpaque = ((patternSR[SR_LSB] & (shift << 8)) > 0) + 2 * ((patternSR[SR_MSB] & (shift << 8)) > 0);
		spriteOpaque = spriteContainer[cycle - 1] & 0x3;
		
		bool pixelConflict = (bgOpaque && spriteOpaque);

		bool priority = (spriteContainer[cycle - 1] & 0x20 ) > 0; // priority is true : sprite is BEHIND background, false : sprite is IN FRONT of background.
		if ((pixelConflict && priority) || !(spriteOpaque && SPRITE_RENDERING)){
			rendererComputeBGPixel(attributeSR, shift, bgOpaque, finalPIX);
		}
		else {
			rendererComputeSpritePixel(spriteOpaque, finalPIX);
		}

		bool sprite0Present = (spriteContainer[cycle - 1] & 0x80) != 0;
		if ( sprite0Present && pixelConflict && BOTH_RENDERING) { //sprite 0 hit detection
			if (((cycle - 1) >= 8) || LEFTMOST_BOTH_SHOWN) {
				ram[0x2002] |= 0b01000000;
			}
		}

		switch (VIDEO_MODE) {
		case NTSC:
			
			if ((scanLine > 7) && (scanLine < 232)) {
				pixels[(scanLine - 8) * SCREEN_WIDTH + cycle - 1] = (!(ram[0x2001] & 0b00000001)) ? finalPIX : 0xFFFFFF00; // greyscale characterisation (white here for mere testing)
			}
			break;
		case PAL:
			//not implemented yet. no 8-line-crop on top and bottom.
			break;
		default:
			break;
		}
	}
	if (cycle < 337) 
	{
		patternSR[SR_LSB] <<= 1;
		patternSR[SR_MSB] <<= 1;
		attributeSR[SR_LSB] <<= 1;
		attributeSR[SR_MSB] <<= 1;
		attributeSR[SR_LSB] |= (u8)(nextTilePaletteLatch[SR_LSB]);
		attributeSR[SR_MSB] |= (u8)(nextTilePaletteLatch[SR_MSB]);
	}

	switch (cycle % 8) 
	{ // fetchings
		case NT_CYCLE:
			tile = 16* mapper->rdNT(0x2000 | COARSE_V); // raw 2x2 tile ID fetch, multiplied by 16 (actual 2 bytes per tile)
			break;
		case AT_CYCLE:
			attr = mapper->rdNT(0x23C0 | (scrollRegs.v & 0x0C00) | ((scrollRegs.v >> 4) & 0x38) | ((scrollRegs.v >> 2) & 0x07));
			requiredShifting = 2 * (2 * ((COARSE_V & 64) != 0) + ((COARSE_V & 2) != 0));// 4 quadrants, 2 bits each. oddness of y>>1 is the most significant bit.
			break;
		case BG_LSB_CYCLE:
			preFetchSR[SR_LSB] = mapper->rdPPU(PAT_TB_PREFIX + tile + FINE_Y);
			break;
		case BG_MSB_CYCLE:
			preFetchSR[SR_MSB] = mapper->rdPPU(PAT_TB_PREFIX + tile + FINE_Y + 8);
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

void PPU::tick() {
	float average = 0;
	//static std::chrono::steady_clock::time_point t0 = Time::now();
	if (cycle / 341) {
		scanLine++;
		cycle = 0;
	}
	
	if ((scanLine == 261) && (cycle == 340) && EITHER_RENDERING) {
		if (oddFrame) {
			cycle = 0;
			scanLine = 0;
		}
		OverrideFrameOddEven = true;
		oddFrame ^= 1;
	}

	scanLine %= 262;
	if ((scanLine == 0) && (cycle == 0)) {
		//t0 = Time::now();
		maySetVBlankFlag = true;
		mayTriggerNMI = true;
	}

	static bool canFinallyNMI = false;
	static u8 nmiCyclesToWait = 0;
	if (scanLine < 240) {
		if (EITHER_RENDERING) {
			spriteEvaluator();

			if (((cycle > 0) && (cycle < 257)) || ((cycle > 320))) {
				pixelRenderer();

				if (cycle == 256) {
					incFineY();
				}
			}

			else if (cycle == 257) {
				scrollRegs.v = (scrollRegs.v & ~0x41F) | (0x41F & scrollRegs.t); // hor (v) = hor(t)
			}
		}
		else {
			if ((V >= 0x3F00) && (cycle > 0) && (cycle < 257) && (scanLine > 7) && (scanLine < 232)) {
				pixels[(scanLine - 8) * SCREEN_WIDTH + cycle - 1] = colors[rdVRAM(V)];
			}
		}

		if (cycle == 256) {
			if (!PAT_TB_PREFIX && SPRITE_PT /*&& (SP_MODE == 8)*/) {
				mapper->acknowledgeNewScanline(true);
			}
		}
		else if ((cycle >= 257) && (cycle <= 320)) {
			oamADDR = 0;
		}
		else if (cycle == 324) {
			if (PAT_TB_PREFIX && !SPRITE_PT /*&& (SP_MODE == 8)*/) {
				mapper->acknowledgeNewScanline(true);
			}
		}
	}

	else if (scanLine == 241) {
		if (cycle == 0) {
			//auto t1 = Time::now();
			//fsec fs = t1 - t0;
			//long long millis = std::chrono::duration_cast<std::chrono::milliseconds>(fs).count();
			//printf("\nframe elapsed in: %.3fs", millis / 1000.0);
		}
		else if (cycle == 1) {
			scr->updateScreen();
			if (maySetVBlankFlag) {
				ram[0x2002] |= 0b10000000;
			}
			if ((ram[0x2000] & 0x80) && mayTriggerNMI) {
				nmiPending = true;
			}
		}
	}
	if ((scanLine >= 241) && (scanLine <= 261)) {
		if (nmiPending && !getCycles()) {
			canFinallyNMI = true;
			nmiCyclesToWait = 2;
		}
		if (canFinallyNMI && getCycles()) {
			if (mayTriggerNMI) {
				if (!nmiCyclesToWait) {
					_nmi();
					canFinallyNMI = false;
					nmiPending = false;
				}
				else {
					nmiCyclesToWait--;
				}
			}
		}
	}

	if (scanLine == 261) {
		if (EITHER_RENDERING) {
			if ((cycle < 256) && ((cycle % 8) == 0)) {
				incCoarseX();
			}
			else if (cycle == 257) {
				scrollRegs.v = (scrollRegs.v & ~0x41F) | (0x41F & scrollRegs.t); // hor (v) = hor(t)
			}
			else if ((cycle >= 280) && (cycle <= 304)) {
				scrollRegs.v = (scrollRegs.v & ~0x7BE0) | (scrollRegs.t & 0x7BE0); // vert (v) = vert(t)
			}
			else if (cycle > 320) {
				pixelRenderer();
			}
		}

		if (cycle == 1) {
			maySetVBlankFlag = true;
			memset(spriteContainer, 0, (SECONDARY_OAM_CAP * 4 + 8)); // empty the next scanline's sprite buffer
			ram[0x2002] &= ~0b11100000; // clear sprite overflow, sprite 0 hit, and vblank flags all at once
		}

		else if ((cycle >= 257) && (cycle <= 320)) {
			oamADDR = 0;
		}
	}
	cycle++;
}

PPU::PPU(u32* px, Screen* sc, Rom* rom) :
	OAM{ 0 }, internalPPUreg(0), frame(0), cycle(0), scanLine(261),
	isVBlank(false), scr(sc), pixels(px), secondaryOAM{ 0 }, scrollRegs{ 0 },
	paletteMem{ 0 }, mayTriggerNMI{ true }, maySetVBlankFlag{ true }, oamADDR{ 0 }
{
	ram[0x2002] = 0b10000000;
	this->mapper = rom->getMapper();
	printf("PPU Started.\n");
}

u8 PPU::readPPUSTATUS() { 
	if ((scanLine == 241)) {
		if (2 <= cycle && cycle <= 3) {// TODO -- improve timing 
			maySetVBlankFlag = false;
			mayTriggerNMI = false;
		}
	}

	scrollRegs.w = 0;
	u8 tmp = ram[0x2002];
	ram[0x2002] &= ~0b10000000;
	return tmp;
}

void PPU::writePPUCTRL(u8 what) { // 0x2000
	//mapper -> setSpriteMode(SP_MODE);
	scrollRegs.t = ((what & 0x3) << 10) | (scrollRegs.t & 0x73FF);
	if ((ram[0x2002] & 0x80) && (!(ram[0x2000] & 0x80)) && (what & 0x80)) {
		if (!(((scanLine == 261) && (cycle == 0)) // TODO -- improve timing here as well
			)//|| ((scanLine == 241) && (cycle <= 0)))  
			) {
			nmiPending = true;
		}
	}
	ram[0x2000] = what;
}

void PPU::writePPUMASK(u8 what) {
	if (!(what & 0x18)) {
		OverrideFrameOddEven = true;
	}
}

void PPU::writePPUSCROLL(u8 what) {
	if (scrollRegs.w == 0) {
		scrollRegs.t = ((what & ~0b111) >> 3) | (scrollRegs.t & 0b111111111100000);
		scrollRegs.x = what & 0b111;
	}
	else {
		scrollRegs.t = ((what & ~0b111) << 2) | (scrollRegs.t & 0b111110000011111);
		scrollRegs.t = ((what & 0b111) << 12) | (scrollRegs.t & 0b000111111111111);
	}
	scrollRegs.w ^= 1;
}

void PPU::writePPUADDR(u8 what) {
	if (scrollRegs.w == 0) {
		scrollRegs.t = ((what & 0b111111) << 8) | (scrollRegs.t & 0b00000011111111);
	}
	else {
		scrollRegs.t = (scrollRegs.t & 0xFF00) | what;
		scrollRegs.v = scrollRegs.t;
	}
	scrollRegs.w ^= 1;
}

void PPU::writePPU(u8 what) {
	wrVRAM(what);
	incPPUADDR();
}

u8 PPU::rdPPU() {
	if ((V & 0xFF00) == 0x3F00) {
		internalPPUreg = rdVRAM(V - 0x1000);
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

void PPU::updateOam() { // OAMDMA (0x4014) processing routine
	for (u16 i = 0; i <= 0xFF; i++) {
		OAM[oamADDR] = rd((ram[0x4014] << 8) | i);
		oamADDR++;
	}
}

void PPU::writeOAM(u8 what) {
	if (!EITHER_RENDERING) {
		OAM[oamADDR] = what;
		oamADDR++;
	}
}

void PPU::setOAMADDR(u8 what) {
	oamADDR = what;
}

u8 PPU::readOAM() {
	return OAM[oamADDR];
}

void PPU::incPPUADDR() {
	scrollRegs.v += (ram[0x2000] & 4) ? 32 : 1;
>>>>>>> refs/remotes/origin/main
}