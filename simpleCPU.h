#pragma warning(disable:4996)
#ifndef SIMPLE_CPU_H
#define SIMPLE_CPU_H
#include <stdint.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>
//import chrono time libs
#include <fstream>
#include <chrono>
#include <time.h>
#include "types.h"
#include "Mappers/Mapper.h"
#ifdef _WIN32
	#include <windows.h>
#endif

#define PAL_CPU_CLOCK_DIVIDER 16
#define NTSC_CPU_CLOCK_DIVIDER 12
#define STACK_END 0x100

#define N_FLAG 0b10000000
#define V_FLAG 0b01000000
#define __FLAG 0b00100000
#define B_FLAG 0b00010000
#define D_FLAG 0b00001000
#define I_FLAG 0b00000100
#define Z_FLAG 0b00000010
#define C_FLAG 0b00000001
#define NEG_NZ_FLAGS 0b01111101
#define NEG_CV_FLAGS 0b10111110

#define SR_RST 0b00100100 // IGNORED(_), B and INTERRUPT are set to 1

#define VERTICAL 1
#define HORIZONTAL 0 // mirroring type

extern u8 ram[];

enum VideoModes {
	PAL,
	NTSC
};

#define VIDEO_MODE NTSC

static const uint8_t _6502InstBaseCycles[256] = {
  7,6,2,8,3,3,5,5,3,2,2,2,4,4,6,6,
  2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
  6,6,2,8,3,3,5,5,4,2,2,2,4,4,6,6,
  2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
  6,6,2,8,3,3,5,5,3,2,2,2,3,4,6,6,
  2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
  6,6,2,8,3,3,5,5,4,2,2,2,5,4,6,6,
  2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
  2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
  2,6,2,6,4,4,4,4,2,5,2,5,5,5,5,5,
  2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
  2,5,2,5,4,4,4,4,2,4,2,5,4,4,4,4,
  2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
  2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
  2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
  2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7
};

class Cpu {
private:
	u8* mem;
	u8* prog;

public:
	Cpu(u8* ram, u8* pr);
	void afficher();
};

u8 rd(u16 at);
void _nmi();
void pressKey(u16 pressed);
void specialCom();
void addCycle();
void manualIRQ();
void _rst();
u64 getTotalCycles();
u32 getCycles();
#endif