
#pragma once
#include "simpleCPU.h"
#include "ScreenTools.h"
#include <stdio.h>

#define APU_MODE_5_STEPS ((ram[0x4017] & 0x80) == 0x80)
#define APU_MODE_4_STEPS ((ram[0x4017] & 0x80) == 0x00)

#define DUTY_CYCLE 0b11000000
#define TIMER_HIGH 0b00000111
#define ENVELOPE 0b00001111
//$4015-- - D NT21 	Control : DMC enable, length counter enables : noise, triangle, pulse 2, pulse 1 (write)
#define DMC_ENABLE 0b00010000
#define LEN_COUNT 0b11111000
#define SWEEP_ENABLE_1 0b10000000
#define SWEEP_PERIOD 0b01110000
#define SHIFT_COUNT 0b00000111
#define P1_ENABLE_FLAG ram[0x4001] & 0x80
#define P2_ENABLE_FLAG ram[0x4005] & 0x80
#define P1_NEGATE_FLAG ram[0x4001] & 0x08
#define P2_NEGATE_FLAG ram[0x4005] & 0x08

static const u8 dutyLut[][8] = { {0, 1, 0, 0, 0, 0, 0, 0},
								{0, 1, 1, 0, 0, 0, 0, 0},
								{0, 1, 1, 1, 1, 0, 0, 0},
								{1, 0, 0, 1, 1, 1, 1, 1} };

static const u8 lengthCounterLUT[] = { 10, 254, 20, 2, 40, 4, 80, 6, 160, 8,
									  60, 10, 14, 12, 26, 14, 12, 16, 24,
									  18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30 };

static const u8 triangleLUT[] = { 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,
							0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };

static const u16 noiseTimerLUT[][16] = { { 4, 8, 14, 30, 60, 88, 118, 148, 188, 236, 354, 472, 708, 944, 1890, 3778 },
										 {4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068} }; // 0 = PAL, 1 = NTSC

static const u16 dmcTimerLUT[][16] = { {398, 354, 316, 298, 276, 236, 210, 198, 176, 148, 132, 118,  98,  78,  66,  50},
									   {428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106,  84,  72,  54} };//0 = PAL, 1 = NTSC

void apuTick();
void updateAPU(u16 where);
bool firstAPUCycleHalf();