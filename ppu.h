#pragma once
#ifndef PPU_H
#define PPU_H
#include "simpleCPU.h"
#include "ScreenTools.h"
#include "Rom.h"

#define BG_RENDERING	 (ram[0x2001] & 0b00001000)
#define SPRITE_RENDERING (ram[0x2001] & 0b00010000)
#define RENDERING		 (ram[0x2001] & 0b00011000)
#define V (regs.v&0x3FFF) //since PPU address is only 15 bits wide

//renderer constants
#define NT_CYCLE 1
#define AT_CYCLE 3
#define BG_LSB_CYCLE 5
#define BG_MSB_CYCLE 7
#define H_INC_CYCLE 0
#define SR_LSB 0
#define SR_MSB 1

#define SECONDARY_OAM_CAP 64

#define PAT_TB_PREFIX ((ram[0x2000] & 0b00010000) ? 0x1000 : 0)

#define SPRITE_PT ((ram[0x2000] & 0x08) ? 0x1000 : 0x0000) //sprite pattern table address
#define SP_MODE ((ram[0x2000] & 0x20) ? 16 : 8) //sprite size in pixels

#define COARSE_Y (regs.v & 0x0FFF)
#define FINE_Y (regs.v >> 12)

extern u8 oamADDR;
extern u8** ppuMemSpace;

static const uint32_t colors[][64] = { { 0x54545400, 0x001e7400, 0x08109000, 0x30008800, 0x44006400, 0x5c003000, 0x54040000, 0x3c180000, 0x202a0000, 0x083a0000, 0x00400000, 0x003c0000, 0x00323c00, 0x00000000, 0x00000000, 0x00000000,
								 0x98969800, 0x084cc400, 0x3032ec00, 0x5c1ee400, 0x8814b000, 0xa0146400, 0x98222000, 0x783c0000, 0x545a0000, 0x28720000, 0x087c0000, 0x00762800, 0x00667800, 0x00000000, 0x00000000, 0x00000000,
								 0xeceeec00, 0x4c9aec00, 0x787cec00, 0xb062ec00, 0xe454ec00, 0xec58b400, 0xec6a6400, 0xd4882000, 0xa0aa0000, 0x74c40000, 0x4cd02000, 0x38cc6c00, 0x38b4cc00, 0x3c3c3c00, 0x00000000, 0x00000000,
								 0xeceeec00, 0xa8ccec00, 0xbcbcec00, 0xd4b2ec00, 0xecaeec00, 0xecaed400, 0xecb4b000, 0xe4c49000, 0xccd27800, 0xb4de7800, 0xa8e29000, 0x98e2b400, 0xa0d6e400, 0xa0a2a000, 0x00000000, 0x00000000 },

								{0x51515100, 0x00127a00, 0x05009500, 0x2c008b00, 0x4b006100, 0x5a002100, 0x55000000, 0x3d0c0000, 0x19240000, 0x00360000, 0x003d0000, 0x00390000, 0x00294300, 0x00000000, 0x00000000, 0x00000000,
								 0x99999900, 0x0646ce00, 0x3527f100, 0x680fe500, 0x9005ad00, 0xa40c5900, 0x9d210000, 0x7e3f0000, 0x4f5d0000, 0x1c750000, 0x007f0000, 0x00782b00, 0x00638500, 0x00000000, 0x00000000, 0x00000000,
								 0xeaeaea00, 0x5797ff00, 0x8778ff00, 0xb961ff00, 0xe257ff00, 0xf65dab00, 0xef725000, 0xd0900800, 0xa1af0000, 0x6ec60000, 0x45d02800, 0x32ca7d00, 0x38b5d700, 0x3d3d3d00, 0x00000000, 0x00000000,
								 0xeaeaea00, 0xaec8ff00, 0xc1bbff00, 0xd6b2ff00, 0xe7aef300, 0xefb0d000, 0xecb9ab00, 0xdfc58d00, 0xccd27f00, 0xb7dc8400, 0xa6e09a00, 0x9eddbd00, 0xa1d4e200, 0xa3a3a300, 0x00000000, 0x00000000} };

static const bool palette = 1;

typedef struct ppu_regs_t {
	u16 v, t;
	u8 x; //fine X scroll
	bool w; //common latch for 0x2005 and 0x2006
} ppuRegs_t;

class PPU {
private:
	bool isVBlank;
	u16 cycle, scanLine;
	u8 frame;
	u32* pixels;
	Screen scr;
	Rom rom;
	u8 OAM[256];
	u8 secondaryOAM[SECONDARY_OAM_CAP];
	u8 internalPPUreg;// internal 0x2007 read buffer
	u8 ppuRam[0x4000];
	Mapper* mapper;

	//renders the next scanline of sprites, in the REVERSE order of how they are stored in OAM2.
	// @param spritesQty - number of sprites in OAM2
	// @param sp0present - Is sprite 0 present in OAM2 ?
	void spriteProcessor(u8 spritesQty, bool sp0present);
	void spriteEvaluator();
	void BGRenderer();
	void sequencer();
	void incPPUADDR();
	u16 paletteMap(u16 where);
	void wrNT(u16 where, u8 what);
	void wrVRAM(u8 what);
	u8 rdNT(u16 where);
	u8 rdVRAM(u16 where);
	void incFineY();
	void incCoarseX();
public:
	ppuRegs_t regs;
	PPU(u32* px, Screen sc, Rom rom);
	void tick();
	void updateOam();
	void writeOAM(u8 what);
	void writePPU(u8 what);
	void writePPUCTRL(u8 what);
	void writePPUSCROLL(u8 what);
	void writePPUADDR(u8 what);
	u8 readOAM();
	u8 rdPPU();
	bool isInVBlank();
};

int mainSYS(Screen scr, FILE* testFile);

#endif