#ifndef PPU_H
#define PPU_H
#include "ScreenTools.h"

#define BG_RENDERING	 (ram[0x2001] & 0b00001000)
#define SPRITE_RENDERING (ram[0x2001] & 0b00010000)
#define RENDERING		 (ram[0x2001] & 0b00011000)
#define EITHER_RENDERING RENDERING
#define BOTH_RENDERING   ((ram[0x2001] & 0b00011000) == 0b00011000)

#define LEFTMOST_BG_SHOWN (ram[0x2001] & 0b00000010)
#define LEFTMOST_SPRITES_SHOWN (ram[0x2001] & 0b00000100)
#define LEFTMOST_BOTH_SHOWN ((ram[0x2001] & 0b00000110) == 0b00000110)

#define V (scrollRegs.v&0x3FFF) //since PPU address is only 15 bits wide

#define SR_LSB 0
#define SR_MSB 1

#define SECONDARY_OAM_CAP 64

#define PAT_TB_PREFIX ((ram[0x2000] & 0b00010000) ? 0x1000 : 0)

#define SPRITE_PT ((ram[0x2000] & 0x08) ? 0x1000 : 0x0000) //sprite pattern table address
#define SP_MODE ((ram[0x2000] & 0x20) ? 16 : 8) //sprite size in pixels

#define COARSE_V (scrollRegs.v & 0x0FFF)
#define FINE_Y (scrollRegs.v >> 12)

enum PpuCycles {
	NT_CYCLE = 1,
	AT_CYCLE = 3,
	BG_LSB_CYCLE = 5,
	BG_MSB_CYCLE = 7,
	H_INC_CYCLE = 0
};

static const uint32_t colors[256] = {
								0x51515100, 0x00127a00, 0x05009500, 0x2c008b00, 0x4b006100, 0x5a002100, 0x55000000, 0x3d0c0000, 0x19240000, 0x00360000, 0x003d0000, 0x00390000, 0x00294300, 0x00000000, 0x00000000, 0x00000000,
								 0x99999900, 0x0646ce00, 0x3527f100, 0x680fe500, 0x9005ad00, 0xa40c5900, 0x9d210000, 0x7e3f0000, 0x4f5d0000, 0x1c750000, 0x007f0000, 0x00782b00, 0x00638500, 0x00000000, 0x00000000, 0x00000000,
								 0xeaeaea00, 0x5797ff00, 0x8778ff00, 0xb961ff00, 0xe257ff00, 0xf65dab00, 0xef725000, 0xd0900800, 0xa1af0000, 0x6ec60000, 0x45d02800, 0x32ca7d00, 0x38b5d700, 0x3d3d3d00, 0x00000000, 0x00000000,
								 0xeaeaea00, 0xaec8ff00, 0xc1bbff00, 0xd6b2ff00, 0xe7aef300, 0xefb0d000, 0xecb9ab00, 0xdfc58d00, 0xccd27f00, 0xb7dc8400, 0xa6e09a00, 0x9eddbd00, 0xa1d4e200, 0xa3a3a300, 0x00000000, 0x00000000,
								 0x51515100, 0x00127a00, 0x05009500, 0x2c008b00, 0x4b006100, 0x5a002100, 0x55000000, 0x3d0c0000, 0x19240000, 0x00360000, 0x003d0000, 0x00390000, 0x00294300, 0x00000000, 0x00000000, 0x00000000,
								 0x99999900, 0x0646ce00, 0x3527f100, 0x680fe500, 0x9005ad00, 0xa40c5900, 0x9d210000, 0x7e3f0000, 0x4f5d0000, 0x1c750000, 0x007f0000, 0x00782b00, 0x00638500, 0x00000000, 0x00000000, 0x00000000,
								 0xeaeaea00, 0x5797ff00, 0x8778ff00, 0xb961ff00, 0xe257ff00, 0xf65dab00, 0xef725000, 0xd0900800, 0xa1af0000, 0x6ec60000, 0x45d02800, 0x32ca7d00, 0x38b5d700, 0x3d3d3d00, 0x00000000, 0x00000000,
								 0xeaeaea00, 0xaec8ff00, 0xc1bbff00, 0xd6b2ff00, 0xe7aef300, 0xefb0d000, 0xecb9ab00, 0xdfc58d00, 0xccd27f00, 0xb7dc8400, 0xa6e09a00, 0x9eddbd00, 0xa1d4e200, 0xa3a3a300, 0x00000000, 0x00000000,
								 0x51515100, 0x00127a00, 0x05009500, 0x2c008b00, 0x4b006100, 0x5a002100, 0x55000000, 0x3d0c0000, 0x19240000, 0x00360000, 0x003d0000, 0x00390000, 0x00294300, 0x00000000, 0x00000000, 0x00000000,
								 0x99999900, 0x0646ce00, 0x3527f100, 0x680fe500, 0x9005ad00, 0xa40c5900, 0x9d210000, 0x7e3f0000, 0x4f5d0000, 0x1c750000, 0x007f0000, 0x00782b00, 0x00638500, 0x00000000, 0x00000000, 0x00000000,
								 0xeaeaea00, 0x5797ff00, 0x8778ff00, 0xb961ff00, 0xe257ff00, 0xf65dab00, 0xef725000, 0xd0900800, 0xa1af0000, 0x6ec60000, 0x45d02800, 0x32ca7d00, 0x38b5d700, 0x3d3d3d00, 0x00000000, 0x00000000,
								 0xeaeaea00, 0xaec8ff00, 0xc1bbff00, 0xd6b2ff00, 0xe7aef300, 0xefb0d000, 0xecb9ab00, 0xdfc58d00, 0xccd27f00, 0xb7dc8400, 0xa6e09a00, 0x9eddbd00, 0xa1d4e200, 0xa3a3a300, 0x00000000, 0x00000000,
								 0x51515100, 0x00127a00, 0x05009500, 0x2c008b00, 0x4b006100, 0x5a002100, 0x55000000, 0x3d0c0000, 0x19240000, 0x00360000, 0x003d0000, 0x00390000, 0x00294300, 0x00000000, 0x00000000, 0x00000000,
								 0x99999900, 0x0646ce00, 0x3527f100, 0x680fe500, 0x9005ad00, 0xa40c5900, 0x9d210000, 0x7e3f0000, 0x4f5d0000, 0x1c750000, 0x007f0000, 0x00782b00, 0x00638500, 0x00000000, 0x00000000, 0x00000000,
								 0xeaeaea00, 0x5797ff00, 0x8778ff00, 0xb961ff00, 0xe257ff00, 0xf65dab00, 0xef725000, 0xd0900800, 0xa1af0000, 0x6ec60000, 0x45d02800, 0x32ca7d00, 0x38b5d700, 0x3d3d3d00, 0x00000000, 0x00000000,
								 0xeaeaea00, 0xaec8ff00, 0xc1bbff00, 0xd6b2ff00, 0xe7aef300, 0xefb0d000, 0xecb9ab00, 0xdfc58d00, 0xccd27f00, 0xb7dc8400, 0xa6e09a00, 0x9eddbd00, 0xa1d4e200, 0xa3a3a300, 0x00000000, 0x00000000};


struct PpuScrollRegs_t {
	u16 v, t;
	u8 x; //fine X scroll
	bool w; //common latch for 0x2005 and 0x2006
};

struct PpuControlRegs_t {
	bool nmiEnable;
	bool masterSlave;
	u8 spHeight = 8;
	u16 bgPatternTable, spPatternTable;

	void setPPUCtrl(u8 what) {
		nmiEnable = what & 0b10000000;
		masterSlave = what & 0b01000000;
	}

	bool blueEmph, greenEmph, redEmph, greyScale;
	bool spEnable, bgEnable;
	bool spLeftShown, bgLeftShown;

	void setPPUMask(u8 what) {
		blueEmph = what & 0b10000000;
		greenEmph = what & 0b01000000;
		redEmph = what & 0b00100000;
		spEnable = what & 0b00010000;
		bgEnable = what & 0b00001000;
		spLeftShown = what & 0b00000100;
		bgLeftShown = what & 0b00000010;
		greyScale = what & 0b00000001;
	}
};

class PPU {
private:
	u8 spriteContainer[4 * SECONDARY_OAM_CAP + 8] = { 0 }; /* contains sprite pattern data for a single scanline.
											0bA0B0CCDD
											A - is it sprite 0?
											B - is it low-priority?
											C - palette number
											D -  pixel color value
											+8 for eventual overflows*/
	struct PpuScrollRegs_t scrollRegs;
	struct PpuControlRegs_t ctrlRegs;
	bool isVBlank;
	bool maySetVBlankFlag;
	bool mayTriggerNMI;
	u16 cycle, scanLine;
	u8 frame;
	u32* pixels;
	Screen* scr;
	u8 oamADDR;
	u8 OAM[256];
	u8 secondaryOAM[SECONDARY_OAM_CAP];
	u8 internalPPUreg;// internal 0x2007 read buffer
	u8 paletteMem[0x20];
	Mapper* mapper;
	bool oddFrame = false;
	bool OverrideFrameOddEven = false;
	bool nmiPending = false;
private:
	//renders the next scanline of sprites, in the REVERSE order of how they are stored in OAM2.
	// @param spritesQty - number of sprites in OAM2
	// @param sp0present - Is sprite 0 present in OAM2 ?
	void spriteProcessor(u8 spritesQty, bool sp0present);
	void spriteEvaluator();
	void rendererComputeSpritePixel(const u8& spriteOpaque, u32& finalPIX);
	void rendererComputeBGPixel(u8  attributeSR[2], const u16& shift, const u8& bgOpaque, u32& finalPIX);
	void pixelRenderer();
	void incPPUADDR();
	u16 paletteMap(u16 where);
	void wrVRAM(u8 what);
	u8 rdVRAM(u16 where);
	void incFineY();
	void incCoarseX();
public:
	PPU(u32* px, Screen* sc, Rom* rom);
	void tick();
	void updateOam();
	void writeOAM(u8 what);
	void setOAMADDR(u8 what);
	void writePPU(u8 what);
	//read 0x2002
	u8 readPPUSTATUS();
	//write to 0x2000
	void writePPUCTRL(u8 what);
	//write to 0x2001
	void writePPUMASK(u8 what);
	//write to 0x2005
	void writePPUSCROLL(u8 what);
	//write to 0x2006
	void writePPUADDR(u8 what);
	u8 readOAM();
	//read VRam WITH ppu address increment AND internal buffer fill.
	u8 rdPPU();
};

int mainSYS(Screen* scr, const char* filePath);

#endif