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
#include <thread>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
//#include "sound.h"
#endif

#define BE_LE_U16(u16) (u16 & 0xff00) >> 8 | (u16 & 0x00ff) << 8
#define E _illegal
#define PAL_CPU_CLOCK_DIVIDER 16
#define NTSC_CPU_CLOCK_DIVIDER 12
#define NOP (void)0
#define S 0x00
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
//#define RELATIVE_BRANCH_CORE ((prog[pc] & N_FLAG) ? -1 * ((uint8_t)(~prog[pc]) + 1) : prog[pc])

#define VERTICAL 1
#define HORIZONTAL 0 // mirroring type

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

extern uint8_t* ram;
extern uint8_t* chrrom;
extern u8 mirror;

static const uint8_t Cycles[] = {
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
typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::microseconds ms;
typedef std::chrono::duration<float> fsec;

#define PAL 0
#define NTSC 1
#define VIDEO_MODE NTSC

u8 rd(u16 at);
u16 absArg(u8 arg, u8 cyc);
u16 indY(u8 cyc);
void wr(u16 where, u8 what);
void check_NZ(u16 obj);
void check_CV();
void _nmi();
void pressKey(u8 pressed, u8 released);
void specialCom();


class Cpu {
private:
	u8* mem;
	u8* prog;

public:
	Cpu(u8* ram, u8* pr);
	void afficher();
};

#endif