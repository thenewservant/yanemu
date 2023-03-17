
#include "firstAPU.h"



// minimal APU excluding additional / game-specific sound channels.

static const uint8_t lengthCounterLUT[] = { 10, 254, 20, 2, 40, 4, 80, 6, 160, 8,
											60, 10, 14, 12, 26, 14, 12, 16, 24,
											18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30 };
static const uint8_t triangleLUT[] = { 15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0,
							0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 };


//uint8_t* ram = (uint8_t*) malloc(0x4020 * sizeof(int));

uint8_t dutyCycle1, dutyCycle2;
uint16_t timer[3]; // timers for pulse 1 & 2, and triangle
uint8_t volume[4];
uint8_t lengthCounter[4]; // length counters for pulse 1 & 2, triangle, and noise
uint8_t envelope[4]; //envelope of pulse1, pulse2 and noise


void workSound(char* buffer, uint64_t bufferSize) {
	//char *buffer = new char[bufferSize];
	const uint8_t dutyLut[][8] = { {0, 1, 0, 0, 0, 0, 0, 0},
								{0, 1, 1, 0, 0, 0, 0, 0},
								{0, 1, 1, 1, 1, 0, 0, 0},
								{1, 0, 0, 1, 1, 1, 1, 1} };
	/*
	ram[0x4000] = 0b10011000; //pulse1 settings
	ram[0x4004] = 0b10011000; //pulse2 settings
	//pulse1 timings
	ram[0x4002] = 0b11100110; //pulse1 low timer bits
	ram[0x4003] = 0b11111000; // pulse1 shall be of lower frequency (approx 235 hz)

	//pulse2 timings
	ram[0x4006] = 0b01000000; //pulse2 low timer bits
	ram[0x4007] = 0b11111000;//approx 560 hz

	ram[0x400A] = 0b01100110; //triangle
	ram[0x400B] = 0b00000000;
	*/
	//ram = ram;
	readChannels();

	const double frequency = 440.0;
	const double amplitude = 1;
	const uint64_t sampleRate = 22100;

	int16_t pulse1Counter = timer[0], pulse2Counter = timer[1], triangleCounter = timer[2];
	//tIsUp means Triangle is going down
	bool p1IsUp = true, p2IsUp = true, tIsUp = true;

	char sample = 0;
	char sampleTriangle = 0;

	for (int i = 0; i < bufferSize; i += 1) {

		readChannels();
		if (pulse1Counter<0) {
			p1IsUp ^= 1;
			pulse1Counter = timer[0];
		}
		if (pulse2Counter<0) {
			p2IsUp ^= 1;
			pulse2Counter = timer[1];
		}
		if (triangleCounter<0) {
			triangleCounter = timer[2];
		}

		//ram[0x4002] = 0;
		//ram[0x4006] -= ram[0x4006]; //pulse2 low timer bits

		//printf("\n%d", ram[0x4002]);
		//printf("%d, %d\n", dutyCycle1, dutyCycle2);


		sample = envelope[0] * ((p1IsUp * pulse1Counter) > 0);//p1
		sample += envelope[1] * ((p2IsUp * pulse2Counter) > 0);//p2

		pulse1Counter -= 5;

		pulse2Counter -= 5;


		sampleTriangle = 1 * triangleLUT[((triangleCounter) * 32 / (timer[2] | (timer[2] == 0))) % 32];
		triangleCounter -= 5;

		//printf("\n%d", sampleTriangle);
		//printf("\n%d", sample);
		//char sample = (i % (int)(sampleRate / (frequency))) < (sampleRate / (2*frequency));
		 //sample = (amplitude * 127 * (2*sample - 1));

		buffer[i] = 4 * sample / 2 + 3 * sampleTriangle;

	}


	constexpr double apuCycleDelay = 1.117460147;

}

void readChannels() {
	dutyCycle1 = (ram[0x4000] & DUTY_CYCLE) >> 6;
	dutyCycle2 = (ram[0x4004] & DUTY_CYCLE) >> 6;
	timer[0] = ram[0x4002] | ((ram[0x4003] & TIMER_HIGH) << 8); //p1
	timer[1] = ram[0x4006] | ((ram[0x4007] & TIMER_HIGH) << 8);	//p2
	timer[2] = ram[0x400A] | ((ram[0x400B] & TIMER_HIGH) << 8); //triangle

	envelope[0] = ram[0x4000] & ENVELOPE; //p1
	envelope[1] = ram[0x4004] & ENVELOPE; //p2
	envelope[2] = ram[0x400C] & ENVELOPE; //triangle

	lengthCounter[0] = (ram[0x4003] >> 3) & LEN_COUNT;
	lengthCounter[1] = (ram[0x4007] >> 3) & LEN_COUNT;
	lengthCounter[2] = (ram[0x400B] >> 3) & LEN_COUNT;
	lengthCounter[3] = (ram[0x400F] >> 3) & LEN_COUNT;

}

