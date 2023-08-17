#pragma once
#ifndef PPU_H
#define PPU_H
#include "simpleCPU.hpp"
#include "ScreenTools.h"
#include "Rom.h"

#define BG_RENDERING	 (ram[0x2001] & 0b00001000)
#define SPRITE_RENDERING (ram[0x2001] & 0b00010000)
#define RENDERING		 (ram[0x2001] & 0b00011000)
#define V (v&0x3FFF) //since PPU address is only 15 bits wide

//renderer constants
#define NT_CYCLE 1
#define AT_CYCLE 3
#define BG_LSB_CYCLE 5
#define BG_MSB_CYCLE 7
#define H_INC_CYCLE 0
#define SR_LSB 0
#define SR_MSB 1

#define SECONDARY_OAM_CAP 64

#define NT_PREFIX (0x2000 | (ram[0x2000] & 3) * 0x400) // NameTable prefix -- add that to required ppuRAM address suffix
#define PAT_TB_PREFIX ((ram[0x2000] & 0b00010000) ? 0x1000 : 0)

extern u8 oamADDR;
extern u8* ppuRam;
extern u8 OAM[256];
extern boolean renderable;
extern u16 t, v;
extern u8 x;
extern boolean w;

class PPU {
private:
	u32* pixels;
	Screen scr;
	Rom rom;

public:
	void showNameTable();
	void finalRender();
	void tick();
	void cac();
	void BGRenderer();
	void sequencer();
	PPU(u32* px, Screen sc, Rom rom);
};

void writePPU(u8 what);
void updateOam();
boolean isInVBlank();
int mainSYS(Screen scr, FILE* testFile);
void incPPUADDR();
void writeOAM(u8 what);
u8 readOAM();
u8 readPPU();

#endif