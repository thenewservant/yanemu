
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
	#include "sound.h"
#endif

#define BE_LE_U16(u16) (u16 & 0xff00) >> 8 | (u16 & 0x00ff) << 8
#define E NULL
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

#define SR_RST 0b00100100 // IGNORED(_), B and INTERRUPT are set to 1
#define RELATIVE_BRANCH_CORE ((prog[pc] & N_FLAG) ? -1 * ((uint8_t)(~prog[pc]) + 1) : prog[pc])

typedef struct timespec tstime;

enum LAST_REG_ASS { AC, XR, YR }; // ensure check_NZ() updates N and Z accordingly to the last registers assigned
extern uint8_t* ram;
extern uint8_t caca;
static const uint8_t Cycles[256] = {
  7,6,2,8,3,3,5,5,3,2,2,2,4,4,6,6,
  2,5,2,8,4,4,6,6,2,4,2,7,5,5,7,7,
  6,6,2,8,3,3,5,5,4,2,2,2,4,4,6,6,
  2,5,2,8,4,4,6,6,2,4,2,7,5,5,7,7,
  6,6,2,8,3,3,5,5,3,2,2,2,3,4,6,6,
  2,5,2,8,4,4,6,6,2,4,2,7,5,5,7,7,
  6,6,2,8,3,3,5,5,4,2,2,2,5,4,6,6,
  2,5,2,8,4,4,6,6,2,4,2,7,5,5,7,7,
  2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
  2,6,2,6,4,4,4,4,2,5,2,5,5,5,5,5,
  2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
  2,5,2,5,4,4,4,4,2,4,2,5,4,4,4,4,
  2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
  2,5,2,8,4,4,6,6,2,4,2,7,5,5,7,7,
  2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
  2,5,2,8,4,4,6,6,2,4,2,7,5,5,7,7
};

int mainCPU();