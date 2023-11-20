#pragma once

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

const u8 dutyLut[][8] = { {0, 1, 0, 0, 0, 0, 0, 0},
								{0, 1, 1, 0, 0, 0, 0, 0},
								{0, 1, 1, 1, 1, 0, 0, 0},
								{1, 0, 0, 1, 1, 1, 1, 1} };

const u8 lengthCounterLUT[] = { 10, 254, 20, 2, 40, 4, 80, 6, 160, 8,
									  60, 10, 14, 12, 26, 14, 12, 16, 24,
									  18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30 };

const float triangleLUT[] = {15.0f, 14.0f, 13.0f, 12.0f, 11.0f, 10.0f, 9.0f, 8.0f, 7.0f, 6.0f, 5.0f, 4.0f, 3.0f, 2.0f, 1.0f, 0.0f,
						   0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f};

const u16 noiseTimerLUT[][16] = { { 4, 8, 14, 30, 60, 88, 118, 148, 188, 236, 354, 472, 708, 944, 1890, 3778 },
										 {4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068} }; // 0 = PAL, 1 = NTSC

const u16 dmcTimerLUT[][16] = { {398, 354, 316, 298, 276, 236, 210, 198, 176, 148, 132, 118,  98,  78,  66,  50},
									   {428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106,  84,  72,  54} };//0 = PAL, 1 = NTSC

enum ApuFrameSteps {
	COMMON_3728_5 = 7457,
	COMMON_7456_5 = 14913,
	COMMON_11185_5 = 22371,
	MODE_4_STEPS_14914 = 29828,
	MODE_4_STEPS_14914_5 = 29829,
	MODE_4_STEPS_14915 = 29830,
	MODE_5_STEPS_14914_5 = 29829,
	MODE_5_STEPS_18640_5 = 37281,
	MODE_5_STEPS_18641 = 37282
};

void apuTick();
void updateAPU(u16 where);