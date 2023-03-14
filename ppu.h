#pragma once
#ifndef PPU_H
#define PPU_H
//#include "simpleCPU.hpp"
//#include "ScreenTools.h"
#include "simpleCPU.hpp"
#include "ScreenTools.h"
#define V_BLANK ram[0x2000] & N_FLAG

//void finalRender(uint32_t* pixels);
extern u16 ppuADDR;
class PPU {
private:
	u8* OAM;// = (uint8_t*)calloc(64 * 4, sizeof(uint8_t));
	u32* pixels;
	Screen scr;
	Rom rom;
	//u32 scanLine, cycle;
public:
	void resetPixels();
	void checkers();
	void showNameTable();
	void showOam();
	void updateOam();
	void drawSprite(uint8_t yy, uint8_t id, uint8_t xx, uint8_t byte2, uint8_t palette = 0);
	void finalRender();
	void tick();
	void cac();
	void sequencer();
	PPU(u32 *px, Screen sc, Rom rom);
};

void writePPU(u8 what);

int mainSYS(Screen scr);
void incPPUADDR();

#endif