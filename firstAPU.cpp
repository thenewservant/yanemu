#include "firstAPU.h"
#include <corecrt_math.h>
#include <stdio.h>

// minimal APU excluding additional / game-specific sound channels.

static const uint8_t lengthCounterLUT[] = { 10, 254, 20, 2, 40, 4, 80, 6, 160, 8,
											60, 10, 14, 12, 26, 14, 12, 16, 24,
											18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30 };

uint8_t* mem = (uint8_t*) malloc(0x4020 * sizeof(int));

uint8_t dutyCycle1, dutyCycle2;
uint16_t timer[3]; // timers for pulse 1 & 2, and triangle
uint8_t volume[4];
uint8_t lengthCounter[4]; // length counters for pulse 1 & 2, triangle, and noise
uint8_t envelope[4]; //envelope of pulse1, pulse2 and noise


void workSound(char * buffer, uint64_t bufferSize) {
	//char *buffer = new char[bufferSize];
	const uint8_t dutyLut[][8] = {{0, 1, 0, 0, 0, 0, 0, 0},
								{0, 1, 1, 0, 0, 0, 0, 0},
								{0, 1, 1, 1, 1, 0, 0, 0},
								{1, 0, 0, 1, 1, 1, 1, 1} };

	mem[0x4000] = 0b10011111; //pulse1 
	mem[0x4004] = 0b10011000; //pulse2
	//pulse1 timings
	mem[0x4002] = 0b11100110; //pulse1 low timer bits 
	mem[0x4003] = 0b11111000; // pulse1 shall be of lower frequency (approx 235 hz)

	//pulse2 timings
	mem[0x4006] = 0b00000110; //pulse2 low timer bits
	mem[0x4007] = 0b11111000;//approx 560 hz
	readChannels();

	const double frequency = 440.0;
	const double amplitude = 1;
	const uint64_t sampleRate = 44100;

	uint16_t pulse1Counter = timer[0], pulse2Counter = timer[1];

	bool p1IsUp = true, p2IsUp = true;

	for (int i = 0; i < bufferSize; i += 1) {
		if (!pulse1Counter) {
			p1IsUp ^= 1;
			pulse1Counter = timer[0];
		}
		if (!pulse2Counter) {
			p2IsUp ^= 1;
			pulse2Counter = timer[1];
		}

		
		//printf("%d, %d\n", dutyCycle1, dutyCycle2);
		
		char sample=0;
		sample = envelope[0]*((p1IsUp * pulse1Counter--) > 0);//p1
		sample += envelope[1] * ((p2IsUp * pulse2Counter--) > 0);//p2

		//printf("\n%d", sample);
		//char sample = (i % (int)(sampleRate / (frequency))) < (sampleRate / (2*frequency));
		 //sample = (amplitude * 127 * (2*sample - 1));

		buffer[i] = sample;
		
	}

	
	constexpr double apuCycleDelay = 1.117460147;



}

void readChannels(){
	dutyCycle1 = (mem[0x4000] & DUTY_CYCLE) >> 6;
	dutyCycle2 = (mem[0x4004] & DUTY_CYCLE) >> 6;
	timer[0] = mem[0x4002] | ((mem[0x4003] & TIMER_HIGH) << 8); //p1
	timer[1] = mem[0x4006] | ((mem[0x4007] & TIMER_HIGH) << 8);	//p2
	timer[2] = mem[0x400A] | ((mem[0x400B] & TIMER_HIGH) << 8); //triangle

	envelope[0] = mem[0x4000] & ENVELOPE; //p1
	envelope[1] = mem[0x4004] & ENVELOPE; //p2
	envelope[2] = mem[0x400C] & ENVELOPE; //triangle

	lengthCounter[0] = (mem[0x4003] >> 3) & LEN_COUNT;
	lengthCounter[1] = (mem[0x4007] >> 3) & LEN_COUNT;
	lengthCounter[2] = (mem[0x400B] >> 3) & LEN_COUNT;
	lengthCounter[3] = (mem[0x400F] >> 3) & LEN_COUNT;

}

