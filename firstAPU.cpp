#include "firstAPU.h"
// minimal APU excluding additional / game-specific sound channels.

static const uint8_t lengthCounterLUT[] = { 10, 254, 20, 2, 40, 4, 80, 6, 160, 8,
											60, 10, 14, 12, 26, 14, 12, 16, 24,
											18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30 };

uint8_t* mem = (uint8_t*) malloc(0x4020 * sizeof(int));

uint8_t dutyCycle1, dutyCycle2;
uint16_t timer[3]; // timers for pulse 1 & 2, and triangle
uint8_t volume[5]; // with volume[4] unused, caused by uncontrolled triangle volume

int main() {
	
}

void readChannels(){
	dutyCycle1 = (mem[4000] & DUTY_CYCLE) >> 6;
	dutyCycle2 = (mem[4004] & DUTY_CYCLE) >> 6;

	timer[0] = mem[0x4002] | ((mem[0x4003] & TIMER_HIGH) << 8);
	timer[1] = mem[0x4006] | ((mem[0x4006] & TIMER_HIGH) << 8);
	timer[2] = mem[0x400] | ((mem[0x4003] & TIMER_HIGH) << 8);


}