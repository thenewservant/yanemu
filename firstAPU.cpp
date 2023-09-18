#include "firstAPU.h"
#include "sound.h"

// minimal APU excluding additional / game-specific sound channels.


u8 dutyCycle1, dutyCycle2;
u16 timer[4]; // timers for pulse 1 & 2, and triangle
u16 p1timer = timer[0];
u16 p2timer = timer[1];
u16 tritimer = timer[2];
u16 noisetimer = timer[3];

u8 lengthCounter[4] // length counters for pulse 1 & 2, triangle, and noise
, envelope[4] //envelope of pulse1, pulse2 and noise
, p1Halt, p2Halt, triHalt, noiseHalt;

u8 triCountReload;
u8 triLinCurrent;

u16 noiseBarrel =0x7FFF; //15 bits, init with 1's
u8 noiseMode; // >0: bit 6 xor'ed with bit 0; 0: bit 1 xor'ed with bit 0

u8 status = 0; // status register at 0x4015

u8 p1Stat = 0; //position in the duty cycle
u8 p2Stat = 0;

bool sweepNegate[2];

bool sweepReload[2];

bool sweepEnabled[2];
	
u8 sweepShift[2];
u8 sweepPeriod[2];
u8 dividerCounter[2];
float pulse1() {
	if (timer[0] < 8) {
		return 0;
	}
	else if (lengthCounter[0] && (status & 0b00000001)) {
		if (p1timer) {
			p1timer -= 1;
		}
		else {
			p1timer = timer[0];
			p1Stat += 1;
			p1Stat %= 8;
		}
	}
	return dutyLut[dutyCycle1][p1Stat] * envelope[0];
}

u8 pulse2() {
	if (timer[1] < 8) {
		return 0;
	}
	else if (lengthCounter[1] && (status & 0b00000010)) {
		if (p2timer) {
			p2timer -= 1;
		}
		else {
			p2timer = timer[1];
			p2Stat += 1;
			p2Stat %= 8;
		}
	}
	return dutyLut[dutyCycle2][p2Stat] * envelope[1];
}

u8 triStat = 0;
bool linCntReloadFlag = false;
float lastVal = 0;


float triangle() {
	static u16 lastVal = 0;
	if (lengthCounter[2] && (status & 0b00000100) && triLinCurrent && (timer[2] > 1)) {
		if (tritimer) {
			tritimer -= 1;
		}
		else {
			tritimer = timer[2];
			triStat += 1;
			triStat %= 32;	
		}
		lastVal =  triangleLUT[triStat];
		return triangleLUT[triStat];
	}
	return lastVal;
}

float noise() {
	static bool feedback;
	if (lengthCounter[3] && (status & 0b00001000)) {
		if (noisetimer) {
			noisetimer--;
		}
		else {
			noisetimer = timer[3];
			feedback = 0x1 & (noiseBarrel ^ (noiseBarrel >> (noiseMode ? 6 : 1)));

			noiseBarrel |= (feedback << 15);
			noiseBarrel >>= 1;
		}
	}
	return 1.0 * feedback * envelope[3];
}


constexpr u16 dmcBaseAdress = 0xC000;

bool directDMCOverride = false;
u8 dmcOutputLevel = 0;
u8 dmcRateIndex = 0;
u16 dmcSampleAdress = 0;
u16 dmcSampleLength = 0;
bool dmcLoop = false;
bool dmcIrqFlag = false;
u16 remainingBytes = 1;

void incDMCSampleAdress() {
	if (dmcSampleAdress == 0xFFFF) {
		dmcSampleAdress = 0x8000;
	}
	else {
		dmcSampleAdress++;
	}
}

u8 dmc() {
	static u16 dmcTimer = 0;
	static u8 oldLevel = 0;
	static u8 sampleBuffer = 0;
	static u8 bitsRemaining = 0;
	static bool silenceFlag = false;
	if (status & 0b00010000) {

		if (!bitsRemaining && (remainingBytes != 0)) {
			sampleBuffer = rd(dmcSampleAdress);
			incDMCSampleAdress();
			remainingBytes--;
		}
		else if (remainingBytes == 0) {
			sampleBuffer = 0;
		}
		if (bitsRemaining == 0) {
			bitsRemaining = 8;
			silenceFlag = false;
		}

		if (remainingBytes == 0) {
			if (dmcLoop) {
				remainingBytes = dmcSampleLength | (ram[0x4012] << 6);
				dmcSampleAdress = dmcBaseAdress;
			}
			else if (dmcIrqFlag) {
				//setDMCInterruptFlag();
			}
		}

		// OUTPUT unit

		//new cycle:

		if (!remainingBytes) {
			silenceFlag = true;
		}
		else {
			silenceFlag = false;
		}

		if (dmcTimer) {
			dmcTimer--;
		}
		else {
			dmcTimer = dmcTimerLUT[VIDEO_MODE][dmcRateIndex];
			if (!silenceFlag) {
				if (sampleBuffer & 1) {
					if (dmcOutputLevel <= 125) {
						dmcOutputLevel += 2;
					}
				}
				else {
					if (dmcOutputLevel >= 2) {
						dmcOutputLevel -= 2;
					}
				}
				sampleBuffer >>= 1;
				bitsRemaining--;
			}
		}
	}
	return  dmcOutputLevel ;
}

void tickLengthCounters() {
	if (lengthCounter[0] && !p1Halt) {
		lengthCounter[0] -= 1;
	}
	else if (!lengthCounter[0]) {
		ram[0x4015] &= ~0b00000001;
	}

	if (lengthCounter[1] && !triHalt) {
		lengthCounter[1] -= 1;

	}
	else if (!lengthCounter[1]) {
		ram[0x4015] &= ~0b00000010;
	}

	if (lengthCounter[2] && !triHalt) {
		lengthCounter[2] -= 1;
	}
	else if (!lengthCounter[2]) {
		ram[0x4015] &= ~0b00000100;
	}

	if (lengthCounter[3] && !noiseHalt) {
		lengthCounter[3] -= 1;
	}
	else if (!lengthCounter[3]) {
		ram[0x4015] &= ~0b00001000;
	}
}

void ticktriangleLinCount() {
	if (linCntReloadFlag) {
		triLinCurrent = triCountReload;
	}
	else if (triLinCurrent) {
		triLinCurrent--;
	}
	if (!triHalt) {
		linCntReloadFlag = false;
	}
}

void targetPeriod(u8 pulseIdx) {
	int16_t ca = (timer[pulseIdx] >> sweepShift[pulseIdx]);
	switch (sweepNegate[pulseIdx]) {
	case 0: //addition
		timer[pulseIdx] =(u16)( timer[pulseIdx] + ca);
		break;
	case 1:
		int16_t tmp = (timer[pulseIdx] - ca - (pulseIdx ? 0 : 1));//pulse1 uses one's complement
		timer[pulseIdx] = (u16)(tmp>=0?tmp:0) ; 
		break;
	}
}

void tickSweeps() {
	if (!dividerCounter[0] && sweepEnabled[0] && timer[0] >8) {
		targetPeriod(0);
	}
	if (!dividerCounter[0] || sweepReload[0]) {
		dividerCounter[0] = sweepPeriod[0];
		sweepReload[0] = false;
	}
	else {
		dividerCounter[0]--;
	}

	if (!dividerCounter[1] && sweepEnabled[1] && timer[1] > 8) {
		targetPeriod(1);
	}
	if (!dividerCounter[1] || sweepReload[1]) {
		dividerCounter[1] = sweepPeriod[1];
		sweepReload[1] = false;
	}
	else {
		dividerCounter[1]--;
	}
}

float ticks = 0;
bool firstAPUCycleHalf() {
	return (ticks == (int(ticks))); // TODO: check if this is correct
}


void apuTick() {
	
	ticks += 0.5;
	float tri = triangle();
	float delta = dmc();
	if ((ticks - (int)ticks) < 0.01) {
		float p1 = pulse1();
		float p2 = pulse2();
		
		float ns = noise();

		if (((int)(2 * ticks) % 40) == 0) {
			float pOut = 95.88 / ((8128.0 / (p1 + p2)) + 100);
			float tnd = 159.79 / ((1 / ((tri / 8227) + (ns /12241) + (delta/22638))) + 100);
			float sample = (pOut + tnd);
			SDL_QueueAudio(dev, &sample, sizeof(float));
		}
	}

	if (ticks == 3728.5) {
		//tickEnvelopes();
		ticktriangleLinCount();
	}
	else if (ticks == 7456.5) {
		//tickEnvelopes();
		ticktriangleLinCount();
		tickLengthCounters();
		tickSweeps();
	}
	else if (ticks == 11185.5) {
		//tickEnvelopes();
		ticktriangleLinCount();
	}

	if (APU_MODE_4_STEPS) {

		 if (ticks == 14914) {
			//setFrameIntFlagIfIntInhibitIsClear();
		}
		else if (ticks == 14914.5) {
			//tickEnvelopes();
			ticktriangleLinCount();
			tickLengthCounters();
			tickSweeps();
		}
		else if (ticks == 14915) {
			ticks = 0;
			//setFrameIntFlagIfIntInhibitIsClear();
		}
		
	}else if (APU_MODE_5_STEPS) {
		
		 if (ticks == 14914.5) {
			//setFrameIntFlagIfIntInhibitIsClear();
		}
		else if (ticks == 18640.5) {
			//tickEnvelopes();
			ticktriangleLinCount();
			tickLengthCounters();
			tickSweeps();
		}
		else if (ticks == 18641) {
			ticks = 0;
			//setFrameIntFlagIfIntInhibitIsClear();
		}
	}
}

void tickEnvelopes() {

}

void updateAPU(u16 where) {

	switch (where) {
	//P1
	case 0x4000:
		p1Halt = ram[0x4000] & 0x20;
		dutyCycle1 = (ram[0x4000] & DUTY_CYCLE) >> 6;
		envelope[0] = ram[0x4000] & 0x0F;
		break;
	case 0x4001:
		sweepEnabled[0] = ram[0x4001] & 0x80;
		sweepPeriod[0] = (ram[0x4001] & 0x70) >> 4;
		sweepNegate[0] = ram[0x4001] & 0x08;
		sweepShift[0] = ram[0x4001] & 0x07;
		break;
	case 0x4002:
		timer[0] = ram[0x4002] | ((ram[0x4003] & TIMER_HIGH) << 8); //p1
		break;
	case 0x4003:
		lengthCounter[0] = lengthCounterLUT[((ram[0x4007] >> 3) & 0b00011111)];
		timer[0] = ram[0x4002] | ((ram[0x4003] & TIMER_HIGH) << 8); //p1
		p1Stat = 0;
		break;

	//P2
	case 0x4004:
		p2Halt = ram[0x4004] & 0x20;
		dutyCycle2 = (ram[0x4004] & DUTY_CYCLE) >> 6;
		envelope[1] = ram[0x4004] & 0x0F;
		break;
	case 0x4005:
		sweepEnabled[1] = ram[0x4005] & 0x80;
		sweepPeriod[1] = (ram[0x4005] & 0x70) >> 4;
		sweepNegate[1] = ram[0x4005] & 0x08;
		sweepShift[1] = ram[0x4005] & 0x07;
		break;
	case 0x4006:
		timer[1] = ram[0x4006] | ((ram[0x4007] & TIMER_HIGH) << 8);	//p2
		break;
	case 0x4007:
		lengthCounter[1] = lengthCounterLUT[((ram[0x4007] >> 3) & 0b00011111)];
		timer[1] = ram[0x4006] | ((ram[0x4007] & TIMER_HIGH) << 8); //p1
		p2Stat = 0;
		break;

	//TRIANGLE
	case 0x4008:
		triHalt = ram[0x4008] & 0x80;
		triCountReload = ram[0x4008] & 0x7F;
		break;
	case 0x400A:
		timer[2] = (ram[0x400A] | ((ram[0x400B] & TIMER_HIGH) << 8)) ; //triangle
		tritimer = timer[2];
		break;
	case 0x400B:
		lengthCounter[2] = lengthCounterLUT[(ram[0x400B] >> 3) & 0b00011111];
		timer[2] = (ram[0x400A] | ((ram[0x400B] & TIMER_HIGH) << 8)); //triangle
		tritimer = timer[2];
		linCntReloadFlag = true;
		break;

	//NOISE
	case 0x400C:
		noiseHalt = ram[0x400C] & 0x20;
		envelope[3] = ram[0x400C] & 0x0F;
	case 0x400E:
		noiseMode = ram[0x400E] & 0x80;
		timer[3] = noiseTimerLUT[VIDEO_MODE][(ram[0x400E] & 0x0F)]; //noise
		noisetimer = timer[3];
		break;
	case 0x400F:
		lengthCounter[3] = lengthCounterLUT[(ram[0x400F] >> 3) & 0b00011111];
		break;

	//DMC
	case 0x4010:
		dmcLoop = ram[0x4010] & 0x40;
		dmcIrqFlag = ram[0x4010] & 0x80;
		dmcRateIndex = ram[0x4010] & 0x0F;
		break; 
	//DMC DIRECT OUTPUT LEVEL
	case 0x4011:
		dmcOutputLevel = ram[0x4011] & 0x7F;
		break;
	case 0x4012:
		dmcSampleAdress = dmcBaseAdress | (ram[0x4012] << 6);

		break;
	case 0x4013:
		dmcSampleLength = 0xFFF & ((ram[0x4013] << 4) + 1);
		remainingBytes = dmcSampleLength;
		break;
	//GENERAL
	case 0x4015: 

		status = ram[0x4015];

		if (!(status & 0b00000001)) {
			p1Halt = true;
		}
		if (!(status & 0b00000010)) {
			p2Halt = true;
		}
		if (!(status & 0b00000100)) {
			triHalt = true;
		}
		if (!(status & 0b00001000)) {
			noiseHalt = true;
		}
		break;
	default: break;
	}

}