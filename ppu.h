#pragma once
#ifndef PPU_H
#define PPU_H
//#include "simpleCPU.hpp"
//#include "ScreenTools.h"
#include "simpleCPU.hpp"
#include "ScreenTools.h"


//void finalRender(uint32_t* pixels);
extern u16 ppuADDR;
extern u8 oamADDR;
extern u8* ppuRam;
extern u8* OAM;
class PPU {
private:
	// = (uint8_t*)calloc(64 * 4, sizeof(uint8_t));
	u32* pixels;
	Screen scr;
	Rom rom;
	//u32 scanLine, cycle;
public:
	void showNameTable();
	//void drawSprite(uint8_t yy, uint8_t id, uint8_t xx, uint8_t byte2, uint8_t palette = 0);
	void finalRender();
	void tick();
	void cac();
	void sequencer();
	PPU(u32 *px, Screen sc, Rom rom);
};
void updateOam();
void writePPU(u8 what);
boolean isInVBlank();
int mainSYS(Screen scr);
void incPPUADDR();
void writeOAM(u8 what);
u8 readOAM();
u8 readPPU();
#endif