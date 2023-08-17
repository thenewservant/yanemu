#pragma warning(disable:4996)
#include "simpleCPU.hpp"
#include "ppu.h"
#include "sound.h"
#include "Rom.h"

//using namespace std;
/*
	_ : indicates a 6502 operation
	XX : opCode
	xxx : operation name
	M / C / _ / I / Z / A / N / R : iMplied, aCcumulator, Single-byte, Immediate, Zeropage, Absolute, iNdirect, Relative
*/
u16 aluTmp = 0; // used to store the result of an addition
u8 termB; //used for signed alu operations with ACCUMULATOR
u8 mirror;
u8* u8tmp; //multi-purpose temp u8 pointer;
u8 m = 0; // temp memory for cmp computing

// 6502 registers
u8 ac, xr, yr, sr = SR_RST, sp = 0xFD;
u16 pc;
bool jammed = false; // is the cpu jammed because of certain instructions?


u8* ram;
u8* chrrom;
u8* prog;
u32 counter = 0;
u32 cycles = 0;
u16 bigEndianTmp; // u16 used to process absolute adresses

u8 latchCommandCtrl1 = 0;
u8 latchCommandCtrl2 = 0;

u8 keys1 = 0;
u8 keys2 = 0;

u8 keyLatchCtrl1 = 0; // controller 1 buttons
u8 keyLatchCtrl2 = 0; // controller 2 buttons
u8 keyLatchStep = 0; // how many buttons did we show the CPU so far?


Rom MainRom;

u32 comCount[256] = { 0 }; // count how many times each command is executed

bool elemIn(u8 elem, u8* arr, u8 size) {
	for (u8 i = 0; i < size; i++) {
		if (arr[i] == elem) {
			return true;
		}
	}
	return false;
}

void specialCom() {
	//go through comCount to find the 10 most used commands
	u8 mostUsed[10] = { 0 };
	for (u8 top = 0; top < 10; top++) {
		u32 maxIndex = 0;
		u32 max = 0;
		for (u32 i = 0; i < 256; i++) {
			if ((comCount[i] > max) && !elemIn(i, mostUsed, 10)) {
				max = comCount[i];
				maxIndex = i;
				
			}
		}mostUsed[top] = maxIndex;
	}
    printf("\nMost used commands :\n");
	for (u8 i = 0; i < 10; i++) {
		printf("%d: %2X - %d times\n", i+1, mostUsed[i], comCount[mostUsed[i]]);
	}
}

void pressKey(u8 pressed, u8 released) {
	keys1 = pressed;

}

//rd is READ. Used to filter certain accesses (such as PPUADDR latch)
inline u8 rd(u16 at) {
	u8 tmp;
	switch (at) {
	case 0x4016:
		if (latchCommandCtrl1) {
			ram[0x4016] = ram[0x4016] & 0xFE | (keyLatchCtrl1) & 1;
			keyLatchCtrl1 >>= 1;
			keyLatchCtrl1 |= 0x80;
		}
		return ram[0x4016];
	case 0x4017:
		return ram[0x4017] & 0xFE;
	case 0x2002:
		w = 0;
		tmp = ram[0x2002];
		ram[0x2002] &= ~0b10000000;
		return tmp;
	case 0x2004:
		return readOAM();
	case 0x2007:
		return readPPU();
	default:
		return ram[at];
	}
}

//absolute memory access with arg (xr or yr) offset
inline u16 absArg(u8 arg, u8 cyc = 1) {
	u16 where = ((prog[pc + 1] << 8) | prog[pc]);
	u16 w2 = where + arg;
	if ((where & 0xFF00) != (w2 & 0xFF00)) {
		cycles += cyc;
	}
	return w2;
}

inline u16 indY(u8 cyc = 1) {
	u16 where = ram[prog[pc]] | (ram[(u8)(prog[pc] + 1)] << 8);
	u16 w2 = where + yr;
	if ((where & 0xFF00) != (w2 & 0xFF00)) {
		cycles += cyc;
	}
	return w2;
}

inline void wr(u16 where, u8 what) {
	if ((where >= 0x8000) && (where <= 0xFFFF)) {
		//ram[where] = what;
		MainRom.contactMappers(where, what);

		//printf("%X, %X\n", where, what);
		return;
	}
	else {
		switch (where) {

		case 0x4016:
			latchCommandCtrl1 = !(what & 1);
			if (latchCommandCtrl1) {
				keyLatchCtrl1 = keys1;
			}
			keyLatchStep = 0;
			break;
		case 0x2000:

			t = ((what & 0x3) << 10) | (t & 0b111001111111111);
			if (isInVBlank() && (ram[0x2002] & 0x80) && (!(ram[0x2000] & 0x80)) && (what & 0x80)) {
				ram[where] = what;
				_nmi();
				return;
			}
			break;

		case 0x2003:
			oamADDR = what;
			break;
		case 0x2004:
			writeOAM(what);
			break;

		case 0x2005:
			if (w == 0) {
				t = ((what & ~0b111) >> 3) | (t & 0b111111111100000);
				x = what & 0b111;
			}
			else {
				t = ((what & ~0b111) << 2) | (t & 0b111110000011111);
				t = ((what & 0b111) << 12) | (t & 0b000111111111111);
			}

			w ^= 1;
			break;
		case 0x2006:
			if (w == 0) {
				t = ((what & 0b111111) << 8) | (t & 0b00000011111111);
			}
			else {
				t = (t & 0xFF00) | what;
				v = t;
			}
			w ^= 1;
			break;

		case 0x2007:
			writePPU(what);
			break;
		case 0x4014:
			ram[0x4014] = what;
			updateOam();
			cycles += 513;
			break;
		}

		ram[where] = what;
		if ((where >= 0x4000) && (where <= 0x4017)) {
			updateAPU(where);
		
		}
		
	}
}

inline void check_NZ(u16 obj) {
	sr &= NEG_NZ_FLAGS;
	sr |= (obj & N_FLAG) | ((obj == 0) << 1);
}

inline void check_CV() {
	sr &= NEG_CV_FLAGS;
	sr |= aluTmp & 0xFF00 ? C_FLAG : 0; // C applied if aluTmp > 0xff
	sr |= V_FLAG & (((ac ^ aluTmp) & (termB ^ aluTmp)) >> 1);
}

void _nmi() {
	wr(STACK_END | sp--, pc >> 8);
	wr(STACK_END | sp--, pc);
	wr(STACK_END | sp--, sr);
	pc = (ram[0xFFFB] << 8) | ram[0xFFFA];
}

//adc core procedure -- termB is the operand as termA is always ACCUMULATOR
inline void _adc() {
	aluTmp = ac + termB + (sr & C_FLAG);
	check_CV();
	ac = aluTmp; // relies on auto u8 conversion.
	check_NZ(ac);
}

void _69adcI() { //seemS OK
	termB = prog[pc];
	_adc();
	pc++;
}

void _65adcZ() { //seems OK
	termB = ram[prog[pc]];
	_adc();
	pc++;
}

void _75adcZ() {
	termB = rd((u8)(prog[pc] + xr));
	_adc();
	pc++;
}

void _6DadcA() {
	termB = rd(((prog[pc + 1] << 8) | prog[pc]));
	_adc();
	pc += 2;
}
void _7DadcA() {
	termB = rd((u16)absArg(xr));
	_adc();
	pc += 2;
}

void _79adcA() {
	termB = rd((u16)absArg(yr));
	_adc();
	pc += 2;
}

void _61adcN() {
	termB = rd(ram[(u8)(prog[pc] + xr)] | (ram[(u8)(prog[pc] + xr + 1)] << 8));
	_adc();
	pc++;
}

void _71adcN() {
	termB = rd((u16)indY());
	_adc();
	pc++;
}

void _00brk_() {
	//printf("brk!\n");
	wr(STACK_END | sp--, (pc + 2) >> 8);
	wr(STACK_END | sp--, (pc + 2));
	wr(STACK_END | sp--, sr|B_FLAG);
	sr |= I_FLAG;
	pc = ram[0xFFFE] | (ram[0xFFFD] << 8);

}

void _29andI() {
	ac &= prog[pc];
	check_NZ(ac);
	pc++;
}

void _25andZ() {
	ac &= rd(prog[pc]);
	check_NZ(ac);
	pc++;
}

void _35andZ() {
	ac &= rd((u8)(prog[pc] + xr));
	check_NZ(ac);
	pc++;
}

void _2DandA() {
	ac &= rd((prog[pc + 1] << 8) | prog[pc]);
	check_NZ(ac);
	pc += 2;
}

void _3DandA() {
	ac &= rd((u16)absArg(xr));
	check_NZ(ac);
	pc += 2;
}

void _39andA() {
	ac &= rd((u16)absArg(yr));
	check_NZ(ac);
	pc += 2;
}

void _21andN() {
	ac &= rd(ram[(u8)(prog[pc] + xr)] | (ram[(u8)(prog[pc] + xr + 1)] << 8));
	check_NZ(ac);
	pc++;
}

void _31andN() {
	ac &= rd((u16)indY());
	check_NZ(ac);
	pc++;
}

int8_t  _rel() {
	int8_t relTmp = (prog[pc] & N_FLAG) ? - ((~prog[pc]) + 1) : prog[pc];
	if ((prog[pc] & 0xFF00) != ((prog[pc] + relTmp) & 0xFF00)) {
		cycles += 1;
	}
	cycles += 1;
	return relTmp;
}

void _90bccR() {
	pc += ((sr & C_FLAG) == 0) ? _rel() : 0;
	pc++;
}

void _B0bcsR() {
	pc += ((sr & C_FLAG)) ? _rel() : 0;
	pc++;
}

void _F0beqR() {
	pc += ((sr & Z_FLAG)) ? _rel() : 0;
	pc++;
}

void _24bitZ() {
	termB = rd(prog[pc]);
	sr = sr & ~(N_FLAG | V_FLAG) | termB & (N_FLAG | V_FLAG);
	sr = sr & ~Z_FLAG | (((ac & termB) == 0) ? Z_FLAG : 0);
	pc++;
}

void _2CbitA() {
	termB = rd((prog[pc + 1] << 8) | prog[pc]);
	sr = sr & ~(N_FLAG | V_FLAG) | termB & (N_FLAG | V_FLAG);
	sr = sr & ~Z_FLAG | (((ac & termB) == 0) ? Z_FLAG : 0);
	pc += 2;
}

void _30bmiR() {
	pc += (sr & N_FLAG) ? _rel() : 0;
	pc++;
}
void _D0bneR() {
	pc += ((sr & Z_FLAG) == 0) ? _rel() : 0;
	pc++;
}

void _10bplR() {
	pc += ((sr & N_FLAG) == 0) ? _rel() : 0;
	pc++;
}


void _50bvcR() {
	pc += ((sr & V_FLAG) == 0) ? _rel() : 0;
	pc++;
}

void _70bvsR() {
	pc += (sr & V_FLAG) ? _rel() : 0;
	pc++;
}

void _18clcM() {
	sr &= ~C_FLAG;
}

void _D8cldM() {
	sr &= ~D_FLAG;
}

void _58cliM() {
	sr &= ~I_FLAG;
}

void _B8clvM() {
	sr &= ~V_FLAG;
}


inline void _cmp() {
	u8 tmpN = ac - m;
	sr = sr & ~N_FLAG & ~Z_FLAG & ~C_FLAG | ((ac == m) ? Z_FLAG : 0) | ((ac >= m) ? C_FLAG : 0) | tmpN & N_FLAG;
}

void _C9cmpI() {
	m = prog[pc];
	_cmp();
	pc++;
}

void _C5cmpZ() {
	m = rd(prog[pc]);
	_cmp();
	pc++;
}

void _D5cmpZ() {
	m = rd((u8)(prog[pc] + xr));
	_cmp();
	pc++;
}

void _CDcmpA() {
	m = rd((prog[pc + 1] << 8) | prog[pc]);
	_cmp();
	pc += 2;
}

void _DDcmpA() {
	m = rd(absArg(xr));
	_cmp();
	pc += 2;
}

void _D9cmpA() {
	m = rd(absArg(yr));
	_cmp();
	pc += 2;
}

void _C1cmpN() {
	m = rd(ram[(u8)(prog[pc] + xr)] | (ram[(u8)(prog[pc] + xr + 1)] << 8));
	_cmp();
	pc++;
}

void _D1cmpN() {
	m = rd(indY());
	_cmp();
	pc++;
}

inline void _cpx() {
	u8 tmpN = xr - m;
	sr = sr & ~N_FLAG & ~Z_FLAG & ~C_FLAG | ((xr == m) ? Z_FLAG : 0) | ((xr >= m) ? C_FLAG : 0) | tmpN & N_FLAG;
}

void _E0cpxI() {
	m = prog[pc];
	_cpx();
	pc++;
}

void _E4cpxZ() {
	m = rd(prog[pc]);
	_cpx();
	pc++;
}

void _ECcpxA() {
	m = rd((prog[pc + 1] << 8) | prog[pc]);
	_cpx();
	pc += 2;
}

inline void _cpy() {
	u8 tmpN = yr - m;
	sr = sr & ~N_FLAG & ~Z_FLAG & ~C_FLAG | ((yr == m) ? Z_FLAG : 0) | ((yr >= m) ? C_FLAG : 0) | tmpN & N_FLAG;
}

void _C0cpyI() {
	m = prog[pc];
	_cpy();
	pc++;
}

void _C4cpyZ() {
	m = rd(prog[pc]);
	_cpy();
	pc++;
}

void _CCcpyA() {
	m = rd((prog[pc + 1] << 8) | prog[pc]);
	_cpy();
	pc += 2;
}


void _C6decZ() {
	check_NZ(--ram[prog[pc]]);
	pc++;
}

void _D6decZ() {
	check_NZ(--ram[(u8)(prog[pc] + xr)]);
	pc++;
}

void _CEdecA() {
	check_NZ(--ram[(prog[pc + 1] << 8) | prog[pc]]);
	pc += 2;
}

void _DEdecA() {
	check_NZ(--ram[(u16)absArg(xr, 0)]);
	pc += 2;
}


void _CAdexM() {
	check_NZ(--xr);
}

void _88deyM() {
	check_NZ(--yr);
}

void _49eorI() {
	ac ^= prog[pc];
	check_NZ(ac);
	pc++;
}

void _45eorZ() {
	ac ^= rd(prog[pc]);
	check_NZ(ac);
	pc++;
}

void _55eorZ() {
	ac ^= rd((u8)(prog[pc] + xr));
	check_NZ(ac);
	pc++;
}

void _4DeorA() {
	ac ^= rd((prog[pc + 1] << 8) | prog[pc]);
	check_NZ(ac);
	pc += 2;
}

void _5DeorA() {
	ac ^= rd((u16)absArg(xr));
	check_NZ(ac);
	pc += 2;
}

void _59eorA() {
	ac ^= rd((u16)absArg(yr));
	check_NZ(ac);
	pc += 2;
}

void _41eorN() {
	ac ^= rd(ram[(u8)(prog[pc] + xr)] | (ram[(u8)(prog[pc] + xr + 1)] << 8));
	check_NZ(ac);
	pc++;
}

void _51eorN() {
	ac ^= rd((u16)indY());
	check_NZ(ac);
	pc++;
}

void _E6incZ() {
	check_NZ(++ram[prog[pc]]);
	pc++;
}

void _F6incZ() {
	check_NZ(++ram[(u8)(prog[pc] + xr)]);
	pc++;
}

void _EEincA() {
	check_NZ(++ram[(prog[pc + 1] << 8) | prog[pc]]);
	pc += 2;
}

void _FEincA() {
	check_NZ(++ram[(u16)absArg(xr, 0)]);
	pc += 2;
}

void _E8inxM() {
	check_NZ(++xr);
}

void _C8inyM() {
	check_NZ(++yr);
}


void _4CjmpA() {
	pc = prog[pc] | (prog[pc + 1] << 8);
}

void _6CjmpN() {
	bigEndianTmp = prog[pc] | (prog[pc + 1] << 8);
	// Here comes the famous Indirect Jump bug implementation...
	pc = ram[bigEndianTmp] | (ram[(bigEndianTmp & 0xFF00) | (bigEndianTmp + 1) & 0x00FF] << 8);
}

void _20jsrA() {
	bigEndianTmp = pc + 1;
	wr(STACK_END | sp, bigEndianTmp >> 8);
	wr(STACK_END | sp - 1, bigEndianTmp);
	sp -= 2;
	pc = prog[pc] | (prog[pc + 1] << 8);
}

void _A9ldaI() {
	ac = prog[pc];
	pc++;
	check_NZ(ac);
}

void _A5ldaZ() {
	ac = ram[prog[pc]];
	pc++;
	check_NZ(ac);
}

void _B5ldaZ() {
	ac = ram[(u8)(prog[pc] + xr)];
	pc++;
	check_NZ(ac);
}

void _ADldaA() {
	ac = rd((prog[pc + 1] << 8) | prog[pc]);
	pc += 2;
	check_NZ(ac);
}

void _BDldaA() {
	ac = rd((u16)absArg(xr));
	pc += 2;
	check_NZ(ac);
}

void _B9ldaA() {
	ac = rd((u16)absArg(yr));
	pc += 2;
	check_NZ(ac);
}

void _A1ldaN() {
	ac = rd(ram[(u8)(prog[pc] + xr)] | (ram[(u8)(prog[pc] + xr + 1)] << 8));
	pc++;
	check_NZ(ac);
}
//
void _B1ldaN() {
	ac = rd((u16)indY());
	pc++;
	check_NZ(ac);
}
//ram[ ram[prog[pc]] + yr];
//turns to: ram[(u16)((ram[prog[pc]] | (ram[(u8)(prog[pc] + 1)] <<8)) + yr)]

void _A2ldxI() {
	xr = prog[pc];
	pc++;
	check_NZ(xr);
}

void _A6ldxZ() {
	xr = ram[prog[pc]];
	pc++;
	check_NZ(xr);
}

void _B6ldxZ() {
	xr = ram[(u8)(prog[pc] + yr)];
	pc++;
	check_NZ(xr);
}

void _AEldxA() {
	xr = rd((prog[pc + 1] << 8) | prog[pc]);
	pc += 2;
	check_NZ(xr);
}

void _BEldxA() {
	xr = rd((u16)absArg(yr));
	pc += 2;
	check_NZ(xr);
}


void _A0ldyI() {
	yr = prog[pc];
	pc++;
	check_NZ(yr);
}

void _A4ldyZ() {
	yr = ram[prog[pc]];
	pc++;
	check_NZ(yr);
}

void _B4ldyZ() {
	yr = rd((u8)(prog[pc] + xr));
	pc++;
	check_NZ(yr);
}

void _ACldyA() {
	yr = rd((prog[pc + 1] << 8) | prog[pc]);
	pc += 2;
	check_NZ(yr);
}

void _BCldyA() {
	yr = rd((u16)absArg(xr));
	pc += 2;
	check_NZ(yr);
}


void _4AlsrC() {
	sr = sr & ~N_FLAG & ~C_FLAG | ac & C_FLAG;
	ac >>= 1;
	check_NZ(ac);
}

void _46lsrZ() {
	termB = ram[prog[pc]];
	sr = sr & ~N_FLAG & ~C_FLAG | termB & C_FLAG;
	ram[prog[pc]] >>= 1;
	check_NZ(ram[prog[pc]]);
	pc++;
}

void _56lsrZ() {
	termB = ram[(u8)(prog[pc] + xr)];
	sr = sr & ~N_FLAG & ~C_FLAG | termB & C_FLAG;
	ram[(u8)(prog[pc] + xr)] >>= 1;
	pc++;
	check_NZ(ram[(u8)(prog[pc] + xr)]);
}

void _4ElsrA() {
	termB = ram[(prog[pc + 1] << 8) | prog[pc]];
	sr = sr & ~N_FLAG & ~C_FLAG | termB & C_FLAG;
	ram[(prog[pc + 1] << 8) | prog[pc]] >>= 1;
	check_NZ(ram[(prog[pc + 1] << 8) | prog[pc]]);
	pc += 2;
}

void _5ElsrA() {
	termB = ram[(u16)absArg(xr, 0)];
	sr = sr & ~N_FLAG & ~C_FLAG | termB & C_FLAG;
	ram[(u16)absArg(xr, 0)] >>= 1;
	check_NZ(ram[(u16)absArg(xr, 0)]);
	pc += 2;
}

void _01oraN() {
	ac |= rd(ram[(u8)(prog[pc] + xr)] | (ram[(u8)(prog[pc] + xr + 1)] << 8));
	check_NZ(ac);
	pc++;
}

void _05oraZ() {
	ac |= rd(prog[pc]);
	check_NZ(ac);
	pc++;
}

void _09oraI() {
	ac |= prog[pc];
	check_NZ(ac);
	pc++;
}

void _0DoraA() {
	ac |= rd((prog[pc + 1] << 8) | prog[pc]);
	check_NZ(ac);
	pc += 2;
}

void _11oraN() {
	ac |= rd((u16)indY());
	pc++;
	check_NZ(ac);
}

void _15oraZ() {
	ac |= rd((u8)(prog[pc] + xr));
	pc++;
	check_NZ(ac);
}

void _19oraA() {
	ac |= rd((u16)absArg(yr));
	pc += 2;
	check_NZ(ac);
}

void _1DoraA() {
	ac |= rd((u16)absArg(xr));
	pc += 2;
	check_NZ(ac);
}


void _48phaM() {
	wr(STACK_END | sp--, ac);
}

void _08phpM() {
	wr(STACK_END | sp--, sr | B_FLAG);
}

void _68plaM() {
	ac = rd(STACK_END | ++sp);
	check_NZ(ac);
}

void _28plpM() { //ignores flags (B and _)
	sr = sr & (B_FLAG | __FLAG) | rd(STACK_END | ++sp) & ~(B_FLAG | __FLAG);
}

//ASL

void _0Aasl_() {
	sr = sr & ~C_FLAG | ((ac & N_FLAG) ? C_FLAG : 0);
	ac <<= 1;
	check_NZ(ac);
}

void _06aslZ() { // one bit left shift, shifted out bit is preserved in carry
	sr = sr & ~C_FLAG | ((ram[(u8)prog[pc]] & N_FLAG) ? C_FLAG : 0);
	ram[prog[pc]] <<= 1;
	check_NZ(ram[prog[pc]]);
	pc++;
}

void _16aslZ() {
	sr = sr & ~C_FLAG | ((ram[(u8)prog[pc] + xr] & N_FLAG) ? C_FLAG : 0);
	ram[(u8)(prog[pc] + xr)] <<= 1;
	check_NZ(ram[(u8)(prog[pc] + xr)]);
	pc++;
}

void _0EaslA() {
	sr = sr & ~C_FLAG | ((ram[(prog[pc + 1] << 8) | prog[pc]] & N_FLAG) ? C_FLAG : 0);
	ram[(prog[pc + 1] << 8) | prog[pc]] <<= 1;
	check_NZ(ram[(prog[pc + 1] << 8) | prog[pc]]);
	pc += 2;
}

void _1EaslA() {
	u8* tmp = &ram[(u16)(((prog[pc + 1] << 8) | prog[pc]) + xr)];
	sr = sr & ~C_FLAG | ((*tmp & N_FLAG) ? C_FLAG : 0);
	*tmp <<= 1;
	check_NZ(*tmp);
	pc += 2;
}

u8 roTmp; // tmp used for storing the MSB or LSB before cycle-shifting
bool futureC; // carry holder

void _2ArolC() {
	futureC = (ac & N_FLAG) > 0;
	ac = (ac << 1) | sr &C_FLAG;
	sr = sr & ~C_FLAG | futureC; // N_FLAG is bit 7, carry will be set to it
	check_NZ(ac);
}

void _26rolZ() {
	futureC = ram[prog[pc]] & N_FLAG;
	wr(prog[pc], (ram[prog[pc]] << 1) | sr & C_FLAG);
	sr = sr & ~C_FLAG | futureC;
	check_NZ(ram[prog[pc]]);
	pc++;
}

void _36rolZ() {
	futureC = ram[(u8)(prog[pc] + xr)] & N_FLAG;
	wr((u8)(prog[pc] + xr), (ram[(u8)(prog[pc] + xr)] << 1) | sr & C_FLAG);
	sr = sr & ~C_FLAG | futureC;
	check_NZ(ram[(u8)(prog[pc] + xr)]);
	pc++;
}

void _2ErolA() {
	futureC = ram[(prog[pc + 1] << 8) | prog[pc]] & N_FLAG;
	wr((prog[pc + 1] << 8) | prog[pc], (ram[(prog[pc + 1] << 8) | prog[pc]] << 1) | sr & C_FLAG);
	sr = sr & ~C_FLAG | futureC;
	check_NZ(ram[(prog[pc + 1] << 8) | prog[pc]]);
	pc += 2;
}

void _3ErolA() {
	u16 indX = (((prog[pc + 1] << 8) | prog[pc]) + xr);
	futureC = ram[indX] & N_FLAG;
	wr(indX, (ram[indX] << 1) | sr & C_FLAG);
	sr = sr & ~C_FLAG & ~N_FLAG & ~Z_FLAG | futureC | (ram[indX] & N_FLAG) | ((ram[indX]>0)?Z_FLAG:0);
	pc += 2;
}

/* improved
void _3ErolA() {
	u16 indX = (((prog[pc + 1] << 8) | prog[pc]) + xr);
	futureC = ram[indX] & N_FLAG;
	wr(indX, (ram[indX] << 1) | sr & C_FLAG);
	sr = sr & ~C_FLAG | futureC;
	check_NZ(ram[indX]);
	pc += 2;
}

*/

/* working version in case optimization fails. 
void _3ErolA() {
	futureC = ram[(u16)(((prog[pc + 1] << 8) | prog[pc]) + xr)] & N_FLAG;
	wr((u16)(((prog[pc + 1] << 8) | prog[pc]) + xr), (ram[(u16)(((prog[pc + 1] << 8) | prog[pc]) + xr)] << 1) | sr & C_FLAG);
	sr = sr & ~C_FLAG | futureC;
	check_NZ(ram[(u16)(((prog[pc + 1] << 8) | prog[pc]) + xr)]);
	pc += 2;
}
*/

void _6ArorC() {
	futureC = ac & 1;
	ac = (ac >> 1) | (sr & C_FLAG) << 7;
	sr = sr & ~C_FLAG | futureC;
	check_NZ(ac);
}

void _66rorZ() {
	futureC = ram[prog[pc]] & 1;
	wr(prog[pc], (ram[prog[pc]] >> 1) | ((sr & C_FLAG) ? N_FLAG : 0));
	sr = sr & ~C_FLAG | futureC;
	check_NZ(ram[prog[pc]]);
	pc++;
}

void _76rorZ() {
	u8* tmp = &ram[(u8)(prog[pc] + xr)];
	futureC = *tmp & 1;
	wr((u8)(prog[pc] + xr), (*tmp >> 1) | (sr & C_FLAG) << 7);
	sr = sr & ~C_FLAG | futureC;
	check_NZ(*tmp);
	pc++;
}

void _6ErorA() {
	futureC = ram[(prog[pc + 1] << 8) | prog[pc]] & 1;
	wr((prog[pc + 1] << 8) | prog[pc], (ram[(prog[pc + 1] << 8) | prog[pc]] >> 1) | (sr & C_FLAG) << 7);
	sr = sr & ~C_FLAG | futureC;
	check_NZ(ram[(prog[pc + 1] << 8) | prog[pc]]);
	pc += 2;
}

void _7ErorA() {
	futureC = ram[(u16)(((prog[pc + 1] << 8) | prog[pc]) + xr)] & 1;
	wr((u16)(((prog[pc + 1] << 8) | prog[pc]) + xr), (ram[(u16)(((prog[pc + 1] << 8) | prog[pc]) + xr)] >> 1) | (sr & C_FLAG) << 7);
	sr = sr & ~C_FLAG | futureC;
	check_NZ(ram[(u16)(((prog[pc + 1] << 8) | prog[pc]) + xr)]);
	pc += 2;
}


void _40rtiM() {
	sr = sr & (B_FLAG | __FLAG) | ram[STACK_END | ++sp] & ~(B_FLAG | __FLAG);
	pc = ram[STACK_END | (u8)(sp + 1)] | (ram[STACK_END | (u8)(sp + 2)] << 8);
	sp += 2;
}

void _60rtsM() {
	pc = (ram[STACK_END | (u8)(sp + 1)] | (ram[STACK_END | (u8)(sp + 2)] << 8)) + 1;
	sp += 2;
}

//SBCs


void _E9sbcI() { //seems OK!

	termB = ~prog[pc];// (N_FLAG & prog[pc]) ? (~prog[pc] + 1) : prog[pc];
	_adc();
	pc++;
}

void _E5sbcZ() {
	termB = ~ram[prog[pc]];
	_adc();
	pc++;

}

void _F5sbcZ() {

	termB = ~ram[(u8)(prog[pc] + xr)];
	_adc();
	pc++;
}

void _EDsbcA() {

	termB = ~ram[(prog[pc + 1] << 8) | prog[pc]];
	_adc();
	pc += 2;
}

void _FDsbcA() {

	termB = ~ram[(u16)absArg(xr)];
	_adc();
	pc += 2;
}

void _F9sbcA() {
	termB = ~ram[(u16)absArg(yr)];
	_adc();
	pc += 2;
}


void _E1sbcN() {
	termB = ~ram[ram[(u8)(prog[pc] + xr)] | (ram[(u8)(prog[pc] + xr + 1)] << 8)];
	_adc();
	pc++;
}

void _F1sbcN() {
	termB = ~ram[(u16)indY()];
	_adc();
	pc++;
}

void _38secM() {
	sr |= C_FLAG;
}

void _F8sedM() {
	sr |= D_FLAG;
}


void _78seiM() {
	sr |= I_FLAG;
}


void _85staZ() {
	wr(prog[pc], ac);
	pc++;
}

void _95staZ() {
	wr((u8)(prog[pc] + xr), ac);
	pc++;
}

void _8DstaA() {
	wr((prog[pc + 1] << 8) | prog[pc], ac);
	pc += 2;
}

void _9DstaA() {
	wr((u16)(((prog[pc + 1] << 8) | prog[pc]) + xr), ac);
	pc += 2;
}

void _99staA() {
	wr((u16)(((prog[pc + 1] << 8) | prog[pc]) + yr), ac);
	pc += 2;
}

void _81staZ() {
	wr(ram[(u8)(prog[pc] + xr)] | (ram[(u8)(prog[pc] + xr + 1)] << 8), ac);
	pc++;
}

void _91staZ() {
	wr((u16)indY(0), ac);
	pc++;
}


void _86stxZ() {
	wr(prog[pc], xr);
	pc++;
}

void _96stxZ() {
	wr((u8)(prog[pc] + yr), xr);
	pc++;
}

void _8EstxA() {
	wr((prog[pc + 1] << 8) | prog[pc], xr);
	pc += 2;
}

void _84styZ() {
	wr(prog[pc], yr);
	pc++;
}

void _94styZ() {
	wr((u8)(prog[pc] + xr), yr);
	pc++;
}

void _8CstyA() {
	wr((prog[pc + 1] << 8) | prog[pc], yr);
	pc += 2;
}

void _AAtaxM() {
	xr = ac;
	check_NZ(xr);
}

void _A8tayM() {
	yr = ac;
	check_NZ(yr);
}

void _BAtsxM() {
	xr = sp;
	check_NZ(xr);
}

void _8AtxaM() {
	ac = xr;
	check_NZ(ac);
}

void _9AtxsM() {
	sp = xr;
}

void _98tyaM() {
	ac = yr;
	check_NZ(ac);
}

void _02jamM() { //freezes the CPU
	jammed = true;
}
// ILLEGAL

void _07sloZ() {
	_06aslZ();
	pc -= 1;
	_05oraZ();
}

void _0FsloA() {
	_07sloZ();
	pc -= 1;
	_0DoraA();
}

void _0BancI() {
	ac = (ac & prog[pc]) << C_FLAG;
	pc++;
}


void _C7dcpZ() {
	ram[prog[pc]] -= 1;
	_C5cmpZ();
	pc++;
}

void _D7dcpZ() {
	ram[(u8)(prog[pc] + xr)] -= 1;
	_D5cmpZ();
	pc++;
}

void _27rlaZ() {
	_26rolZ();
	pc -= 2;
	_25andZ();
}

void _37rlaZ() {
	_36rolZ();
	pc -= 2;
	_35andZ();
}

void _2FrlaA() {
	_2ErolA();
	pc -= 2;
	_2DandA();
}

void _3FrlaA() {
	_3ErolA();
	pc -= 2;
	_3DandA();
}

void _3BrlaA() {
	NOP;
}

//LAX

void _A7laxZ() {
	_A5ldaZ();
	_AAtaxM();
}

void _B7laxZ() {
	ac = rd((u8)(prog[pc] + yr));
	_AAtaxM();
	pc += 2;
}

void _AFlaxA() {
	_ADldaA();
	_AAtaxM();
}

void _BFlaxA() {
	_B9ldaA();
	_AAtaxM();
}

void _A3laxN() {
	_A1ldaN();
	_AAtaxM();
}

void _B3laxN() {
	_B1ldaN();
	_AAtaxM();
}

//SAX

//SBX
/*
void _CBsbxI() {
	xr = (ac & xr) - prog[pc];
	sr = sr & ~N_FLAG & ~Z_FLAG & ~C_FLAG | ((ac == m) ? Z_FLAG : 0) | ((ac >= m) ? C_FLAG : 0) | tmpN & N_FLAG;
	pc++;
}
*/

//DCP

void _DBdcpA() {
	u8 tmp = rd(absArg(yr));
	wr((u16)(((prog[pc + 1] << 8) | prog[pc]) + yr), tmp-1);
	m = tmp - 1;
	_cmp();
	pc += 2;
}

// ISC

void _FFiscA() {
	u8 tmp = rd(absArg(xr));
	wr((u16)(((prog[pc + 1] << 8) | prog[pc]) + xr), tmp+1);
	m = tmp  +1;
	termB = ~(tmp + 1);
	_adc();
	pc +=2;
}

//NOPS

void _EAnopM() { NOP; }

void _80nopI() { NOP; pc++; }

void _04nopZ() { NOP; pc++; }

void _0CnopA() { NOP; pc += 2; }

void _1CnopA() { NOP; absArg(xr), pc += 2; }


void _illegal() {
	printf("\nThis ILLEGAL OPCODE is not implemented YET! :%X, %X, %X\n", prog[pc - 1], prog[pc], prog[pc+1]);
	exit(0x1);
}

void (*opCodePtr[])() = { _00brk_, _01oraN, _02jamM, E, _04nopZ, _05oraZ, _06aslZ, _07sloZ,
						_08phpM, _09oraI, _0Aasl_, _0BancI, _0CnopA, _0DoraA, _0EaslA, _0FsloA,
						_10bplR, _11oraN, _02jamM, E, _04nopZ, _15oraZ, _16aslZ, E,
						_18clcM, _19oraA, _EAnopM, E, _1CnopA, _1DoraA, _1EaslA, E,
						_20jsrA, _21andN, _02jamM, E, _24bitZ, _25andZ,
						_26rolZ, _27rlaZ, _28plpM, _29andI, _2ArolC, E, _2CbitA, _2DandA,
						_2ErolA, _2FrlaA, _30bmiR, _31andN, _02jamM, E, _04nopZ, _35andZ,
						_36rolZ, _37rlaZ, _38secM, _39andA, _EAnopM, _3BrlaA, _1CnopA, _3DandA,
						_3ErolA, _3FrlaA, _40rtiM, _41eorN, _02jamM, E, _04nopZ, _45eorZ, _46lsrZ,
						E, _48phaM, _49eorI, _4AlsrC, E, _4CjmpA, _4DeorA, _4ElsrA,
						E, _50bvcR, _51eorN, _02jamM, E, _04nopZ, _55eorZ, _56lsrZ,
						E, _58cliM, _59eorA, _EAnopM, E, _1CnopA, _5DeorA, _5ElsrA,
						E, _60rtsM, _61adcN, _02jamM, E, _04nopZ, _65adcZ, _66rorZ,
						E, _68plaM, _69adcI, _6ArorC, E, _6CjmpN, _6DadcA, _6ErorA,
						E, _70bvsR, _71adcN, _02jamM, E, _04nopZ, _75adcZ, _76rorZ,
						E, _78seiM, _79adcA, _EAnopM, E, _1CnopA, _7DadcA, _7ErorA,
						E, _80nopI, _81staZ, _80nopI, E, _84styZ, _85staZ, _86stxZ,
						E, _88deyM, _80nopI, _8AtxaM, E, _8CstyA, _8DstaA, _8EstxA,
						E, _90bccR, _91staZ, _02jamM, E, _94styZ, _95staZ, _96stxZ,
						E, _98tyaM, _99staA, _9AtxsM, E, E, _9DstaA, E,
						E, _A0ldyI, _A1ldaN, _A2ldxI, _A3laxN, _A4ldyZ, _A5ldaZ, _A6ldxZ,
						_A7laxZ, _A8tayM, _A9ldaI, _AAtaxM, E, _ACldyA, _ADldaA, _AEldxA,
						_AFlaxA, _B0bcsR, _B1ldaN, _02jamM, _B3laxN, _B4ldyZ, _B5ldaZ, _B6ldxZ,
						_B7laxZ, _B8clvM, _B9ldaA, _BAtsxM, E, _BCldyA, _BDldaA, _BEldxA,
						_BFlaxA, _C0cpyI, _C1cmpN, _80nopI, E, _C4cpyZ, _C5cmpZ, _C6decZ,
						_C7dcpZ, _C8inyM, _C9cmpI, _CAdexM, E, _CCcpyA, _CDcmpA, _CEdecA,
						E, _D0bneR, _D1cmpN, _02jamM, E, _04nopZ, _D5cmpZ, _D6decZ,
						_D7dcpZ, _D8cldM, _D9cmpA, _EAnopM, _DBdcpA, _1CnopA, _DDcmpA, _DEdecA,
						E, _E0cpxI, _E1sbcN, _80nopI, E, _E4cpxZ, _E5sbcZ, _E6incZ,
						E, _E8inxM, _E9sbcI, _EAnopM, _E9sbcI, _ECcpxA, _EDsbcA, _EEincA,
						E, _F0beqR, _F1sbcN, _02jamM, E, _04nopZ, _F5sbcZ, _F6incZ,
						E, _F8sedM, _F9sbcA, _EAnopM, E, _1CnopA, _FDsbcA, _FEincA, _FFiscA
};



Cpu::Cpu(u8* ram, u8* pr) {
	mem = ram;
	prog = pr;
	pc = (ram[0xFFFD] << 8) | ram[0xFFFC];
}

void Cpu::afficher() {
	printf("pc: %04X ac: %02x xr: %02x yr: %02x sp: %02x sr: %02x PPU: %3d SL: %3d ", pc, ac, xr, yr, sp, sr, (3 * cycles) % 341, -1 + (((((3 * cycles)) / 341)) % 262));
	for (int i = 0; i < 8; i++) {
		printf("%d", (sr >> (7 - i)) & 1);
	}
}

u32 cyclesBefore = 0;
u32 toAbsorb = 0;
u8 masterClockCycles = 0;


int mainSYS(Screen scr, FILE* testFile) {

	ram = (u8*)calloc((1 << 16), sizeof(u8));
	
	prog = ram;

	Rom rom(testFile, ram);
	MainRom = rom;
	MainRom.printInfo();
	Cpu f(ram, ram);
	PPU p(scr.getPixelsPointer(), scr, MainRom);
	soundmain();
	
	u64 nbcyc = 0;
	
	auto t0 = Time::now();

	switch (VIDEO_MODE) {
	case NTSC:
		printf("\nVideo: NTSC");
		while (1) {
			while ((SDL_GetQueuedAudioSize(dev) / sizeof(float)) < 4000) {
				for (int i = 0; i < 59561 * 1; i++) {
					nbcyc++;
					if (!(masterClockCycles & 3)) { // !(x&3) <=> x%4==0
						p.tick();
					}
					if (masterClockCycles == 12) {

						if (!(toAbsorb - cyclesBefore)) {
							cyclesBefore = cycles;

							cycles += Cycles[ram[pc]];
							comCount[prog[pc]]++;
							
							opCodePtr[prog[pc++]]();

							toAbsorb = cycles - 1; // current tick is  taken into account, ofc
						}
						else {
							toAbsorb--;
						}
						apuTick(dev);
						masterClockCycles = 0;
					}
					masterClockCycles++;
				}

				if (nbcyc == 21441960) {
					nbcyc = 0;
					auto t1 = Time::now();
					fsec fs = t1 - t0;
					long long millis = std::chrono::duration_cast<std::chrono::milliseconds>(fs).count();
					printf("\n1s elapsed in: %.3fs -- %.2f fps", millis / 1000.0, 60*1000.0/millis );
					t0 = Time::now();
				}
			}
			SDL_Delay(1); // ALWAYS way too much fps --> sleep instead of busy waiting
		}
		break;

	case PAL:
		printf("\nVideo: PAL");
		while (1) {
			//mainClockTickPAL(f, p);
		}
		break;
	default:
		printf("rom ERROR!");
		exit(1);
		}
	}



/*************************************
switch (prog[pc++]) {

case 0x69:  //seemS OK
	termB = prog[pc];
	_adc();
	pc++;
	break;

case 0x65:  //seems OK
	termB = ram[prog[pc]];
	_adc();
	pc++;
	break;

case 0x75:
	termB = rd((u8)(prog[pc] + xr));
	_adc();
	pc++;
	break;

case 0x6D:
	termB = rd(((prog[pc + 1] << 8) | prog[pc]));
	_adc();
	pc += 2;
	break;
case 0x7D:
	termB = rd((u16)absArg(xr));
	_adc();
	pc += 2;
	break;

case 0x79:
	termB = rd((u16)absArg(yr));
	_adc();
	pc += 2;
	break;

case 0x61:
	termB = rd(ram[(u8)(prog[pc] + xr)] | (ram[(u8)(prog[pc] + xr + 1)] << 8));
	_adc();
	pc++;
	break;

case 0x71:
	termB = rd((u16)indY());
	_adc();
	pc++;
	break;

case 0x00:
	//printf("brk!\n");
	wr(STACK_END | sp--, (pc + 2) >> 8);
	wr(STACK_END | sp--, (pc + 2));
	wr(STACK_END | sp--, sr | B_FLAG);
	sr |= I_FLAG;
	pc = ram[0xFFFE] | (ram[0xFFFD] << 8);

	break;

case 0x29:
	ac &= prog[pc];
	check_NZ(ac);
	pc++;
	break;

case 0x25:
	ac &= rd(prog[pc]);
	check_NZ(ac);
	pc++;
	break;

case 0x35:
	ac &= rd((u8)(prog[pc] + xr));
	check_NZ(ac);
	pc++;
	break;

case 0x2D:
	ac &= rd((prog[pc + 1] << 8) | prog[pc]);
	check_NZ(ac);
	pc += 2;
	break;

case 0x3D:
	ac &= rd((u16)absArg(xr));
	check_NZ(ac);
	pc += 2;
	break;

case 0x39:
	ac &= rd((u16)absArg(yr));
	check_NZ(ac);
	pc += 2;
	break;

case 0x21:
	ac &= rd(ram[(u8)(prog[pc] + xr)] | (ram[(u8)(prog[pc] + xr + 1)] << 8));
	check_NZ(ac);
	pc++;
	break;

case 0x31:
	ac &= rd((u16)indY());
	check_NZ(ac);
	pc++;
	break;



case 0x90:
	pc += ((sr & C_FLAG) == 0) ? _rel() : 0;
	pc++;
	break;

case 0xB0:
	pc += ((sr & C_FLAG)) ? _rel() : 0;
	pc++;
	break;

case 0xF0:
	pc += ((sr & Z_FLAG)) ? _rel() : 0;
	pc++;
	break;

case 0x24:
	termB = rd(prog[pc]);
	sr = sr & ~(N_FLAG | V_FLAG) | termB & (N_FLAG | V_FLAG);
	sr = sr & ~Z_FLAG | (((ac & termB) == 0) ? Z_FLAG : 0);
	pc++;
	break;

case 0x2C:
	termB = rd((prog[pc + 1] << 8) | prog[pc]);
	sr = sr & ~(N_FLAG | V_FLAG) | termB & (N_FLAG | V_FLAG);
	sr = sr & ~Z_FLAG | (((ac & termB) == 0) ? Z_FLAG : 0);
	pc += 2;
	break;

case 0x30:
	pc += (sr & N_FLAG) ? _rel() : 0;
	pc++;
	break;
case 0xD0:
	pc += ((sr & Z_FLAG) == 0) ? _rel() : 0;
	pc++;
	break;

case 0x10:
	pc += ((sr & N_FLAG) == 0) ? _rel() : 0;
	pc++;
	break;


case 0x50:
	pc += ((sr & V_FLAG) == 0) ? _rel() : 0;
	pc++;
	break;

case 0x70:
	pc += (sr & V_FLAG) ? _rel() : 0;
	pc++;
	break;

case 0x18:
	sr &= ~C_FLAG;
	break;

case 0xD8:
	sr &= ~D_FLAG;
	break;

case 0x58:
	sr &= ~I_FLAG;
	break;

case 0xB8:
	sr &= ~V_FLAG;
	break;




case 0xC9:
	m = prog[pc];
	_cmp();
	pc++;
	break;

case 0xC5:
	m = rd(prog[pc]);
	_cmp();
	pc++;
	break;

case 0xD5:
	m = rd((u8)(prog[pc] + xr));
	_cmp();
	pc++;
	break;

case 0xCD:
	m = rd((prog[pc + 1] << 8) | prog[pc]);
	_cmp();
	pc += 2;
	break;

case 0xDD:
	m = rd(absArg(xr));
	_cmp();
	pc += 2;
	break;

case 0xD9:
	m = rd(absArg(yr));
	_cmp();
	pc += 2;
	break;

case 0xC1:
	m = rd(ram[(u8)(prog[pc] + xr)] | (ram[(u8)(prog[pc] + xr + 1)] << 8));
	_cmp();
	pc++;
	break;

case 0xD1:
	m = rd(indY());
	_cmp();
	pc++;
	break;



case 0xE0:
	m = prog[pc];
	_cpx();
	pc++;
	break;

case 0xE4:
	m = rd(prog[pc]);
	_cpx();
	pc++;
	break;

case 0xEC:
	m = rd((prog[pc + 1] << 8) | prog[pc]);
	_cpx();
	pc += 2;
	break;


case 0xC0:
	m = prog[pc];
	_cpy();
	pc++;
	break;

case 0xC4:
	m = rd(prog[pc]);
	_cpy();
	pc++;
	break;

case 0xCC:
	m = rd((prog[pc + 1] << 8) | prog[pc]);
	_cpy();
	pc += 2;
	break;


case 0xC6:
	check_NZ(--ram[prog[pc]]);
	pc++;
	break;

case 0xD6:
	check_NZ(--ram[(u8)(prog[pc] + xr)]);
	pc++;
	break;

case 0xCE:
	check_NZ(--ram[(prog[pc + 1] << 8) | prog[pc]]);
	pc += 2;
	break;

case 0xDE:
	check_NZ(--ram[(u16)absArg(xr, 0)]);
	pc += 2;
	break;


case 0xCA:
	check_NZ(--xr);
	break;

case 0x88:
	check_NZ(--yr);
	break;

case 0x49:
	ac ^= prog[pc];
	check_NZ(ac);
	pc++;
	break;

case 0x45:
	ac ^= rd(prog[pc]);
	check_NZ(ac);
	pc++;
	break;

case 0x55:
	ac ^= rd((u8)(prog[pc] + xr));
	check_NZ(ac);
	pc++;
	break;

case 0x4D:
	ac ^= rd((prog[pc + 1] << 8) | prog[pc]);
	check_NZ(ac);
	pc += 2;
	break;

case 0x5D:
	ac ^= rd((u16)absArg(xr));
	check_NZ(ac);
	pc += 2;
	break;

case 0x59:
	ac ^= rd((u16)absArg(yr));
	check_NZ(ac);
	pc += 2;
	break;

case 0x41:
	ac ^= rd(ram[(u8)(prog[pc] + xr)] | (ram[(u8)(prog[pc] + xr + 1)] << 8));
	check_NZ(ac);
	pc++;
	break;

case 0x51:
	ac ^= rd((u16)indY());
	check_NZ(ac);
	pc++;
	break;

case 0xE6:
	check_NZ(++ram[prog[pc]]);
	pc++;
	break;

case 0xF6:
	check_NZ(++ram[(u8)(prog[pc] + xr)]);
	pc++;
	break;

case 0xEE:
	check_NZ(++ram[(prog[pc + 1] << 8) | prog[pc]]);
	pc += 2;
	break;

case 0xFE:
	check_NZ(++ram[(u16)absArg(xr, 0)]);
	pc += 2;
	break;

case 0xE8:
	check_NZ(++xr);
	break;

case 0xC8:
	check_NZ(++yr);
	break;


case 0x4C:
	pc = prog[pc] | (prog[pc + 1] << 8);
	break;

case 0x6C:
	bigEndianTmp = prog[pc] | (prog[pc + 1] << 8);
	// Here comes the famous Indirect Jump bug implementation...
	pc = ram[bigEndianTmp] | (ram[(bigEndianTmp & 0xFF00) | (bigEndianTmp + 1) & 0x00FF] << 8);
	break;

case 0x20:
	bigEndianTmp = pc + 1;
	wr(STACK_END | sp, bigEndianTmp >> 8);
	wr(STACK_END | sp - 1, bigEndianTmp);
	sp -= 2;
	pc = prog[pc] | (prog[pc + 1] << 8);
	break;

case 0xA9:
	ac = prog[pc];
	pc++;
	check_NZ(ac);
	break;

case 0xA5:
	ac = ram[prog[pc]];
	pc++;
	check_NZ(ac);
	break;

case 0xB5:
	ac = ram[(u8)(prog[pc] + xr)];
	pc++;
	check_NZ(ac);
	break;

case 0xAD:
	ac = rd((prog[pc + 1] << 8) | prog[pc]);
	pc += 2;
	check_NZ(ac);
	break;

case 0xBD:
	ac = rd((u16)absArg(xr));
	pc += 2;
	check_NZ(ac);
	break;

case 0xB9:
	ac = rd((u16)absArg(yr));
	pc += 2;
	check_NZ(ac);
	break;

case 0xA1:
	ac = rd(ram[(u8)(prog[pc] + xr)] | (ram[(u8)(prog[pc] + xr + 1)] << 8));
	pc++;
	check_NZ(ac);
	break;
	//
case 0xB1:
	ac = rd((u16)indY());
	pc++;
	check_NZ(ac);
	break;
	//ram[ ram[prog[pc]] + yr];
	//turns to: ram[(u16)((ram[prog[pc]] | (ram[(u8)(prog[pc] + 1)] <<8)) + yr)]

case 0xA2:
	xr = prog[pc];
	pc++;
	check_NZ(xr);
	break;

case 0xA6:
	xr = ram[prog[pc]];
	pc++;
	check_NZ(xr);
	break;

case 0xB6:
	xr = ram[(u8)(prog[pc] + yr)];
	pc++;
	check_NZ(xr);
	break;

case 0xAE:
	xr = rd((prog[pc + 1] << 8) | prog[pc]);
	pc += 2;
	check_NZ(xr);
	break;

case 0xBE:
	xr = rd((u16)absArg(yr));
	pc += 2;
	check_NZ(xr);
	break;


case 0xA0:
	yr = prog[pc];
	pc++;
	check_NZ(yr);
	break;

case 0xA4:
	yr = ram[prog[pc]];
	pc++;
	check_NZ(yr);
	break;

case 0xB4:
	yr = rd((u8)(prog[pc] + xr));
	pc++;
	check_NZ(yr);
	break;

case 0xAC:
	yr = rd((prog[pc + 1] << 8) | prog[pc]);
	pc += 2;
	check_NZ(yr);
	break;

case 0xBC:
	yr = rd((u16)absArg(xr));
	pc += 2;
	check_NZ(yr);
	break;


case 0x4A:
	sr = sr & ~N_FLAG & ~C_FLAG | ac & C_FLAG;
	ac >>= 1;
	check_NZ(ac);
	break;

case 0x46:
	termB = ram[prog[pc]];
	sr = sr & ~N_FLAG & ~C_FLAG | termB & C_FLAG;
	ram[prog[pc]] >>= 1;
	check_NZ(ram[prog[pc]]);
	pc++;
	break;

case 0x56:
	termB = ram[(u8)(prog[pc] + xr)];
	sr = sr & ~N_FLAG & ~C_FLAG | termB & C_FLAG;
	ram[(u8)(prog[pc] + xr)] >>= 1;
	pc++;
	check_NZ(ram[(u8)(prog[pc] + xr)]);
	break;

case 0x4E:
	termB = ram[(prog[pc + 1] << 8) | prog[pc]];
	sr = sr & ~N_FLAG & ~C_FLAG | termB & C_FLAG;
	ram[(prog[pc + 1] << 8) | prog[pc]] >>= 1;
	check_NZ(ram[(prog[pc + 1] << 8) | prog[pc]]);
	pc += 2;
	break;

case 0x5E:
	termB = ram[(u16)absArg(xr, 0)];
	sr = sr & ~N_FLAG & ~C_FLAG | termB & C_FLAG;
	ram[(u16)absArg(xr, 0)] >>= 1;
	check_NZ(ram[(u16)absArg(xr, 0)]);
	pc += 2;
	break;

case 0x01:
	ac |= rd(ram[(u8)(prog[pc] + xr)] | (ram[(u8)(prog[pc] + xr + 1)] << 8));
	check_NZ(ac);
	pc++;
	break;

case 0x05:
	ac |= rd(prog[pc]);
	check_NZ(ac);
	pc++;
	break;

case 0x09:
	ac |= prog[pc];
	check_NZ(ac);
	pc++;
	break;

case 0x0D:
	ac |= rd((prog[pc + 1] << 8) | prog[pc]);
	check_NZ(ac);
	pc += 2;
	break;

case 0x11:
	ac |= rd((u16)indY());
	pc++;
	check_NZ(ac);
	break;

case 0x15:
	ac |= rd((u8)(prog[pc] + xr));
	pc++;
	check_NZ(ac);
	break;

case 0x19:
	ac |= rd((u16)absArg(yr));
	pc += 2;
	check_NZ(ac);
	break;

case 0x1D:
	ac |= rd((u16)absArg(xr));
	pc += 2;
	check_NZ(ac);
	break;


case 0x48:
	wr(STACK_END | sp--, ac);
	break;

case 0x08:
	wr(STACK_END | sp--, sr | B_FLAG);
	break;

case 0x68:
	ac = rd(STACK_END | ++sp);
	check_NZ(ac);
	break;

case 0x28:  //ignores flags (B and _)
	sr = sr & (B_FLAG | __FLAG) | rd(STACK_END | ++sp) & ~(B_FLAG | __FLAG);
	break;

	//ASL

case 0x0A:
	sr = sr & ~C_FLAG | ((ac & N_FLAG) ? C_FLAG : 0);
	ac <<= 1;
	check_NZ(ac);
	break;

case 0x06:  // one bit left shift, shifted out bit is preserved in carry
	sr = sr & ~C_FLAG | ((ram[(u8)prog[pc]] & N_FLAG) ? C_FLAG : 0);
	ram[prog[pc]] <<= 1;
	check_NZ(ram[prog[pc]]);
	pc++;
	break;

case 0x16:
	sr = sr & ~C_FLAG | ((ram[(u8)prog[pc] + xr] & N_FLAG) ? C_FLAG : 0);
	ram[(u8)(prog[pc] + xr)] <<= 1;
	check_NZ(ram[(u8)(prog[pc] + xr)]);
	pc++;
	break;

case 0x0E:
	sr = sr & ~C_FLAG | ((ram[(prog[pc + 1] << 8) | prog[pc]] & N_FLAG) ? C_FLAG : 0);
	ram[(prog[pc + 1] << 8) | prog[pc]] <<= 1;
	check_NZ(ram[(prog[pc + 1] << 8) | prog[pc]]);
	pc += 2;
	break;

case 0x1E:
	tmp = &ram[(u16)(((prog[pc + 1] << 8) | prog[pc]) + xr)];
	sr = sr & ~C_FLAG | ((*tmp & N_FLAG) ? C_FLAG : 0);
	*tmp <<= 1;
	check_NZ(*tmp);
	pc += 2;
	break;



case 0x2A:
	futureC = (ac & N_FLAG) > 0;
	ac = (ac << 1) | sr & C_FLAG;
	sr = sr & ~C_FLAG | futureC; // N_FLAG is bit 7, carry will be set to it
	check_NZ(ac);
	break;

case 0x26:
	futureC = ram[prog[pc]] & N_FLAG;
	wr(prog[pc], (ram[prog[pc]] << 1) | sr & C_FLAG);
	sr = sr & ~C_FLAG | futureC;
	check_NZ(ram[prog[pc]]);
	pc++;
	break;

case 0x36:
	futureC = ram[(u8)(prog[pc] + xr)] & N_FLAG;
	wr((u8)(prog[pc] + xr), (ram[(u8)(prog[pc] + xr)] << 1) | sr & C_FLAG);
	sr = sr & ~C_FLAG | futureC;
	check_NZ(ram[(u8)(prog[pc] + xr)]);
	pc++;
	break;

case 0x2E:
	futureC = ram[(prog[pc + 1] << 8) | prog[pc]] & N_FLAG;
	wr((prog[pc + 1] << 8) | prog[pc], (ram[(prog[pc + 1] << 8) | prog[pc]] << 1) | sr & C_FLAG);
	sr = sr & ~C_FLAG | futureC;
	check_NZ(ram[(prog[pc + 1] << 8) | prog[pc]]);
	pc += 2;
	break;

case 0x3E:
	indX = (((prog[pc + 1] << 8) | prog[pc]) + xr);
	futureC = ram[indX] & N_FLAG;
	wr(indX, (ram[indX] << 1) | sr & C_FLAG);
	sr = sr & ~C_FLAG & ~N_FLAG & ~Z_FLAG | futureC | (ram[indX] & N_FLAG) | ((ram[indX] > 0) ? Z_FLAG : 0);
	pc += 2;
	break;

	
case 0x6A:
	futureC = ac & 1;
	ac = (ac >> 1) | (sr & C_FLAG) << 7;
	sr = sr & ~C_FLAG | futureC;
	check_NZ(ac);
	break;

case 0x66:
	futureC = ram[prog[pc]] & 1;
	wr(prog[pc], (ram[prog[pc]] >> 1) | ((sr & C_FLAG) ? N_FLAG : 0));
	sr = sr & ~C_FLAG | futureC;
	check_NZ(ram[prog[pc]]);
	pc++;
	break;

case 0x76:
	tmp = &ram[(u8)(prog[pc] + xr)];
	futureC = *tmp & 1;
	wr((u8)(prog[pc] + xr), (*tmp >> 1) | (sr & C_FLAG) << 7);
	sr = sr & ~C_FLAG | futureC;
	check_NZ(*tmp);
	pc++;
	break;

case 0x6E:
	futureC = ram[(prog[pc + 1] << 8) | prog[pc]] & 1;
	wr((prog[pc + 1] << 8) | prog[pc], (ram[(prog[pc + 1] << 8) | prog[pc]] >> 1) | (sr & C_FLAG) << 7);
	sr = sr & ~C_FLAG | futureC;
	check_NZ(ram[(prog[pc + 1] << 8) | prog[pc]]);
	pc += 2;
	break;

case 0x7E:
	futureC = ram[(u16)(((prog[pc + 1] << 8) | prog[pc]) + xr)] & 1;
	wr((u16)(((prog[pc + 1] << 8) | prog[pc]) + xr), (ram[(u16)(((prog[pc + 1] << 8) | prog[pc]) + xr)] >> 1) | (sr & C_FLAG) << 7);
	sr = sr & ~C_FLAG | futureC;
	check_NZ(ram[(u16)(((prog[pc + 1] << 8) | prog[pc]) + xr)]);
	pc += 2;
	break;


case 0x40:
	sr = sr & (B_FLAG | __FLAG) | ram[STACK_END | ++sp] & ~(B_FLAG | __FLAG);
	pc = ram[STACK_END | (u8)(sp + 1)] | (ram[STACK_END | (u8)(sp + 2)] << 8);
	sp += 2;
	break;

case 0x60:
	pc = (ram[STACK_END | (u8)(sp + 1)] | (ram[STACK_END | (u8)(sp + 2)] << 8)) + 1;
	sp += 2;
	break;

	//SBCs


case 0xE9:  //seems OK!

	termB = ~prog[pc];// (N_FLAG & prog[pc]) ? (~prog[pc] + 1) : prog[pc];
	_adc();
	pc++;
	break;

case 0xE5:
	termB = ~ram[prog[pc]];
	_adc();
	pc++;

	break;

case 0xF5:

	termB = ~ram[(u8)(prog[pc] + xr)];
	_adc();
	pc++;
	break;

case 0xED:

	termB = ~ram[(prog[pc + 1] << 8) | prog[pc]];
	_adc();
	pc += 2;
	break;

case 0xFD:

	termB = ~ram[(u16)absArg(xr)];
	_adc();
	pc += 2;
	break;

case 0xF9:
	termB = ~ram[(u16)absArg(yr)];
	_adc();
	pc += 2;
	break;


case 0xE1:
	termB = ~ram[ram[(u8)(prog[pc] + xr)] | (ram[(u8)(prog[pc] + xr + 1)] << 8)];
	_adc();
	pc++;
	break;

case 0xF1:
	termB = ~ram[(u16)indY()];
	_adc();
	pc++;
	break;

case 0x38:
	sr |= C_FLAG;
	break;

case 0xF8:
	sr |= D_FLAG;
	break;


case 0x78:
	sr |= I_FLAG;
	break;


case 0x85:
	wr(prog[pc], ac);
	pc++;
	break;

case 0x95:
	wr((u8)(prog[pc] + xr), ac);
	pc++;
	break;

case 0x8D:
	wr((prog[pc + 1] << 8) | prog[pc], ac);
	pc += 2;
	break;

case 0x9D:
	wr((u16)(((prog[pc + 1] << 8) | prog[pc]) + xr), ac);
	pc += 2;
	break;

case 0x99:
	wr((u16)(((prog[pc + 1] << 8) | prog[pc]) + yr), ac);
	pc += 2;
	break;

case 0x81:
	wr(ram[(u8)(prog[pc] + xr)] | (ram[(u8)(prog[pc] + xr + 1)] << 8), ac);
	pc++;
	break;

case 0x91:
	wr((u16)indY(0), ac);
	pc++;
	break;


case 0x86:
	wr(prog[pc], xr);
	pc++;
	break;

case 0x96:
	wr((u8)(prog[pc] + yr), xr);
	pc++;
	break;

case 0x8E:
	wr((prog[pc + 1] << 8) | prog[pc], xr);
	pc += 2;
	break;

case 0x84:
	wr(prog[pc], yr);
	pc++;
	break;

case 0x94:
	wr((u8)(prog[pc] + xr), yr);
	pc++;
	break;

case 0x8C:
	wr((prog[pc + 1] << 8) | prog[pc], yr);
	pc += 2;
	break;

case 0xAA:
	xr = ac;
	check_NZ(xr);
	break;

case 0xA8:
	yr = ac;
	check_NZ(yr);
	break;

case 0xBA:
	xr = sp;
	check_NZ(xr);
	break;

case 0x8A:
	ac = xr;
	check_NZ(ac);
	break;

case 0x9A:
	sp = xr;
	break;

case 0x98:
	ac = yr;
	check_NZ(ac);
	break;

case 0x02:  //freezes the CPU
	jammed = true;
	break;
default:
	printf("FAIL - %2X", prog[pc + 1]);
	exit(1);
}

*************************************/