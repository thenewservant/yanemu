#pragma warning(disable:4996)
#include "simpleCPU.hpp"
#include "ppu.h"
#include "sound.h"
//using namespace std;
/*
	_ : indicates a 6502 operation
	XX : opCode
	xxx : operation name
	M / C / _ / I / Z / A / N / R : iMplied, aCcumulator, Single-byte, Immediate, Zeropage, Absolute, iNdirect, Relative
*/
u16 aluTmp = 0; // used to store the result of an addition
u8  termB; //used for signed alu operations with ACCUMULATOR
u8 mirror = 2;
u8 tmpN = 0; // used to store the result of a subtraction on 8 bits (to further deduce the N flag)
u8 m = 0; // temp memory for cmp computing

// multi-purpose core procedures
u8 lastReg;
u8 ac = 0, xr, yr, sr = SR_RST, sp = 0xFD;
u16 pc;
bool jammed = false; // is the cpu jammed because of certain instructions?

u8* ram;
u8* chrrom;
u8* prog;
u32 counter = 0;
u32 cycles = 0;
u16 bigEndianTmp; // u16 used to process absolute adresses

u8 wr2006Nb = 0;
u8 wr2006tmp = 0;

u8 latchCommandCtrl1 = 0;
u8 keys1 = 0;
u8 keyLatchCtrl1 = 0; // controller 1 buttons
u8 keyLatchStep = 0; // how many buttons did we show the CPU so far?

void pressKey(u8 pressed, u8 released) {
		keys1 = pressed;
}

//rd is READ. Used to filter certain accesses (such as PPUADDR latch)
inline u8 rd(u16 at) {
	u8 tmp;
	switch (at) {
	case 0x4016:
		if (latchCommandCtrl1) {
			ram[0x4016] = ram[0x4016] & 0xFE | (keyLatchCtrl1) & 1; // Yeah I love to obfuscate stuff out  
			keyLatchCtrl1 >>= 1;
			keyLatchCtrl1 |= 0x80;
			//if (keyLatchStep++ >= 8) { ram[0x4016] |= 1; }
		}
		return ram[0x4016];
	case 0x2002:
		ppuADDR = 0;
		tmp = ram[0x2002];
		ram[0x2002] &= ~0x80;
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
u16 absArg(u8 arg, u8 cyc = 1) {
	u16 where = ((prog[pc + 1] << 8) | prog[pc]);
	u16 w2 = where + arg;
	if ((where & 0xFF00) != (w2 & 0xFF00)) {
		cycles += cyc;
	}
	return w2;
}

u16 indY(u8 cyc = 1) {
	u16 where = ram[prog[pc]] | (ram[(u8)(prog[pc] + 1)] << 8);
	u16 w2 = where + yr;
	if ((where & 0xFF00) != (w2 & 0xFF00)) {
		cycles += cyc;
	}
	return w2;
}

u8 scrollLatchTurn = 0; //0 --> time to write X, 1 --> Y

inline void wr(u16 where, u8 what) {

	switch (where) {
	case 0x2005:
		if (!(scrollLatchTurn % 2)) {
			xScroll = what;
		}
		else {
			yScroll = 0;
		}
	case 0x4016:
		latchCommandCtrl1 = !(what & 1);
		if (latchCommandCtrl1) {
			keyLatchCtrl1 = keys1;
		}
		keyLatchStep = 0;
		break;
	case 0x2000:
		if (isInVBlank() && (ram[0x2002] & 0x80) && (!(ram[0x2000] & 0x80)) && (what & 0x80)) {
			ram[where] = what;
			_nmi();
			return;
			//printf("lol");
		}
		break;
	case 0x2003:
		oamADDR = what;
		break;
	case 0x2004:
		writeOAM(what);
		ram[0x2003] = oamADDR+1;
		break;
	case 0x2006:
		wr2006Nb += 1;
		if (!(wr2006Nb % 2)) {
			ppuADDR = (wr2006tmp << 8) | what;
			ram[0x2007] = ppuRam[ppuADDR];
			//printf("\nUPDATE");
		}
		else {
			wr2006tmp = what;
		}
		break;
	case 0x2007:
		writePPU(what);
		break;
	case 0x4014:
		ram[0x4014] = what;
		updateOam();
		break;
	//default:
	}
	ram[where] = what;
}

inline void check_NZ(u16 obj) {
	sr = (sr & ~N_FLAG) | (obj & N_FLAG);
	sr = (sr & ~Z_FLAG) | ((obj == 0) << 1);
}

inline void check_CV() {
	sr = (aluTmp) > 0xff ? (sr | C_FLAG) : sr & ~C_FLAG;
	sr = (sr & ~V_FLAG) | V_FLAG & (((ac ^ aluTmp) & (termB ^ aluTmp)) >> 1);
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
	termB = rd(prog[pc]);
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
	sr |= I_FLAG;
	wr(STACK_END | sp--, (pc + 2) >> 8);
	wr(STACK_END | sp--, (pc + 2));
	wr(STACK_END | sp--, sr);
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
	int8_t relTmp = ((prog[pc] & N_FLAG) ? -1 * ((uint8_t)(~prog[pc]) + 1) : prog[pc]);
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
	sr = sr & ~N_FLAG & ~V_FLAG & ~Z_FLAG | termB & (N_FLAG | V_FLAG) | (((ac & termB) == 0) ? Z_FLAG : 0);
	pc++;
}

void _2CbitA() {
	termB = rd(((prog[pc + 1] << 8) | prog[pc]));
	sr = sr & ~N_FLAG & ~V_FLAG & ~Z_FLAG | termB & (N_FLAG | V_FLAG) | (((ac & termB) == 0) ? Z_FLAG : 0);
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
	tmpN = ac - m;
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
	m = rd((u16)absArg(xr));
	_cmp();
	pc += 2;
}

void _D9cmpA() {
	m = rd((u16)absArg(yr));
	_cmp();
	pc += 2;
}

void _C1cmpN() {
	m = rd(ram[(u8)(prog[pc] + xr)] | (ram[(u8)(prog[pc] + xr + 1)] << 8));
	_cmp();
	pc++;
}

void _D1cmpN() {
	m = rd((u16)indY());
	_cmp();
	pc++;
}

inline void _cpx() {
	tmpN = xr - m;
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
	tmpN = yr - m;
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
	ac = rd(prog[pc]);
	pc++;
	check_NZ(ac);
}

void _B5ldaZ() {
	ac = rd((u8)(prog[pc] + xr));
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
	xr = rd(prog[pc]);
	pc++;
	check_NZ(xr);
}

void _B6ldxZ() {
	xr = rd((u8)(prog[pc] + yr));
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
	yr = rd(prog[pc]);
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
	sr = sr & ~N_FLAG & ~C_FLAG | ac & C_FLAG;
	ram[prog[pc]] >>= 1;
	check_NZ(ram[prog[pc]]);
	pc++;
}

void _56lsrZ() {
	sr = sr & ~N_FLAG & ~C_FLAG | ac & C_FLAG;
	ram[(u8)(prog[pc] + xr)] >>= 1;
	pc++;
	check_NZ(ram[(u8)(prog[pc] + xr)]);
}

void _4ElsrA() {
	sr = sr & ~N_FLAG & ~C_FLAG | ac & C_FLAG;
	ram[(prog[pc + 1] << 8) | prog[pc]] >>= 1;
	check_NZ(ram[(prog[pc + 1] << 8) | prog[pc]]);
	pc += 2;
}

void _5ElsrA() {
	sr = sr & ~N_FLAG & ~C_FLAG | ac & C_FLAG;
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
	wr(STACK_END | sp, ac);
	sp -= 1;
}

void _08phpM() {
	wr(STACK_END | sp--, sr);
}

void _68plaM() {
	ac = rd(STACK_END | ++sp);
	check_NZ(ac);
}

void _28plpM() { //ignores flags (B and _)
	sr = sr & (B_FLAG | __FLAG) | ram[STACK_END | ++sp] & ~(B_FLAG | __FLAG);
}

//ASL

void _0Aasl_() {
	sr = sr & ~C_FLAG | ((ac & N_FLAG) ? C_FLAG : 0);
	ac = ac << 1;
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
	sr = sr & ~C_FLAG | ((ram[(u16)(((prog[pc + 1] << 8) | prog[pc]) + xr)] & N_FLAG) ? C_FLAG : 0);
	ram[(u16)(((prog[pc + 1] << 8) | prog[pc]) + xr)] <<= 1;
	check_NZ(ram[(u16)(((prog[pc + 1] << 8) | prog[pc]) + xr)]);
	pc += 2;
}

u8 roTmp; // tmp used for storing the MSB or LSB before cycle-shifting
bool futureC; // carry holder

void _2ArolC() {
	futureC = (ac & N_FLAG) > 0;
	ac = (ac << 1) | sr & C_FLAG;
	sr = sr & ~C_FLAG | futureC; // N_FLAG is bit 7, carry will be set to it
	check_NZ(ac);
}

void _26rolZ() {
	futureC = ram[prog[pc]] & N_FLAG;
	wr(prog[pc], (ram[prog[pc]] << 1) | sr & C_FLAG);
	sr = sr & ~C_FLAG | (futureC > 0);
	check_NZ(ram[prog[pc]]);
	pc++;
}

void _36rolZ() {
	futureC = ram[(u8)(prog[pc] + xr)] & N_FLAG;
	wr((u8)(prog[pc] + xr), (ram[(u8)(prog[pc] + xr)] << 1) | sr & C_FLAG);
	sr = sr & ~C_FLAG | (futureC > 0);
	check_NZ(ram[(u8)(prog[pc] + xr)]);
	pc++;
}

void _2ErolA() {
	futureC = ram[(prog[pc + 1] << 8) | prog[pc]] & N_FLAG;
	wr((prog[pc + 1] << 8) | prog[pc], (ram[(prog[pc + 1] << 8) | prog[pc]] << 1) | sr & C_FLAG);
	sr = sr & ~C_FLAG | (futureC > 0);
	check_NZ(ram[(prog[pc + 1] << 8) | prog[pc]]);
	pc += 2;
}

void _3ErolA() {
	futureC = ram[(u16)(((prog[pc + 1] << 8) | prog[pc]) + xr)] & N_FLAG;
	wr((u16)(((prog[pc + 1] << 8) | prog[pc]) + xr), (ram[(u16)(((prog[pc + 1] << 8) | prog[pc]) + xr)] << 1) | sr & C_FLAG);
	sr = sr & ~C_FLAG | (futureC > 0);
	check_NZ(ram[(u16)(((prog[pc + 1] << 8) | prog[pc]) + xr)]);
	pc += 2;
}

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
	futureC = ram[(u8)(prog[pc] + xr)] & 1;
	wr((u8)(prog[pc] + xr), (ram[(u8)(prog[pc] + xr)] >> 1) | (sr & C_FLAG) << 7);
	sr = sr & ~C_FLAG | futureC;
	check_NZ(ram[(u8)(prog[pc] + xr)]);
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
/* Archaic
u8 tmpCarry; // used for overflow
inline void _sbc() {
	aluTmp = ac + termB - ((sr & C_FLAG) ^ C_FLAG);
	tmpCarry = sr & C_FLAG;
	sr = (ac >= (u8)(~termB + 1)) ? (sr | C_FLAG) : (sr & ~C_FLAG);
	//sr = (sr & ~V_FLAG) | V_FLAG & (((ac ^ aluTmp) & ((termB ) ^ aluTmp)) >> 1);
	sr = (sr & ~V_FLAG) | (V_FLAG & ((aluTmp <= 0xFF) >> 1));
	ac = aluTmp;
	check_NZ();
}
*/

void _E9sbcI() { //seems OK!

	termB = ~prog[pc];// (N_FLAG & prog[pc]) ? (~prog[pc] + 1) : prog[pc];
	//_sbc();
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

//NOPS

void _EAnopM() { NOP; }

void _80nopI() { NOP; pc++; }

void _04nopZ() { NOP; pc++; }

void _0CnopA() { NOP; pc += 2; }

void _1CnopA() { NOP; absArg(xr), pc += 2; }

u8 nums = 0;
u64 loops = 0;

u8* prgromm; //= (u8*)calloc(0xFFFF, sizeof(u8));

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
						_D7dcpZ, _D8cldM, _D9cmpA, _EAnopM, E, _1CnopA, _DDcmpA, _DEdecA,
						E, _E0cpxI, _E1sbcN, _80nopI, E, _E4cpxZ, _E5sbcZ, _E6incZ,
						E, _E8inxM, _E9sbcI, _EAnopM, _E9sbcI, _ECcpxA, _EDsbcA, _EEincA,
						E, _F0beqR, _F1sbcN, _02jamM, E, _04nopZ, _F5sbcZ, _F6incZ,
						E, _F8sedM, _F9sbcA, _EAnopM, E, _1CnopA, _FDsbcA, _FEincA, E
};

Cpu::Cpu(u8* ram, u8* pr) {
	mem = ram;
	prog = pr;

	pc = (ram[0xFFFD] << 8) | ram[0xFFFC];
	//pc = 0xC000;
}

u32 toAbsorb = 0;
u32 cyclesBefore = 0;

void Cpu::exec() {
	if (!(toAbsorb - cyclesBefore)) {
		cyclesBefore = cycles;

		cycles += Cycles[prog[pc]];
		opCodePtr[prog[pc++]]();

		toAbsorb = cycles - 1; // current tick is  taken into account, ofc
		//printf("\n%5d:%2X:%2X %2X:", ++counter, prog[pc], prog[pc + 1], prog[pc + 2]); afficher();
	}
	else {
		//cycles += 1;
		toAbsorb--;
	}

	//if (ram[0x2006])printf("\%X\n", ram[0x2006]);
}

void Cpu::run(u8 sleepTime = 0) {
	while (!jammed) {

		counter += 1;
		printf("\n%5d:%2X:%2X %2X:", counter, prog[pc], prog[pc + 1], prog[pc + 2]);// afficher();
		/*
		auto t0 = Time::now();
		auto t1 = Time::now();
		fsec fs = t1 - t0;

		long long microseconds = std::chrono::duration_cast<std::chrono::microseconds>(fs).count();

		while (microseconds < 2) {
			auto t1 = Time::now();
			fsec fs = t1 - t0;
			microseconds = std::chrono::duration_cast<std::chrono::microseconds>(fs).count();
		}
		*/
		exec();
	}
	printf("\nCPU jammed at pc = $%X", pc - 1);
	//nums += 1;
}

void Cpu::afficher() {
	printf("pc: %04X ac: %02x xr: %02x yr: %02x sp: %02x sr: %02x PPU: %3d SL: %3d ", pc, ac, xr, yr, sp, sr, (3 * cycles) % 341, -1 + (((((3 * cycles)) / 341)) % 262));
	for (int i = 0; i < 8; i++) {
		printf("%d", (sr >> (7 - i)) & 1);
	}
}

void cpuf() {

	if (!ram) {
		printf("\nallocation error");
		exit(1);
	}
	prog = ram;
	Cpu f(ram, prog);
	//measure the time accurately (milliseconds) it takes to run the program
	//insertAt(ram, ORG, prog2);
	pc = (ram[0xFFFD] << 8) | ram[0xFFFC];
	//pc = 0xC000;
	f.run(); // SLEEP TIME
}

int mainCPU() {
	ram = (u8*)calloc((1 << 16), sizeof(u8));

	FILE* testFile = fopen("C:\\Users\\ppd49\\3D Objects\\C++\\yanemu\\tests\\snow.nes", "rb");

	if (!testFile) {
		printf("\nError: can't open file\n");
		exit(1);
	}
	Rom rom(testFile, ram);

	chrrom = rom.getChrRom();

	printf("CPU");

	if (1) {
		cpuf();
		printf("hello");

		//Sleep(1000);
		if (0) {
			//std::thread tsound(soundmain);
			//sound.join();
		}

		//tcpu.join();
	}
	return 0;
}

u8 masterClockCycles = 0;

void mainClockTickNTSC(Cpu f, PPU p) {
	if (!(masterClockCycles % 3)) {
		f.exec();
	}
	if (!(masterClockCycles % 1)) {
		p.tick();
	}
	masterClockCycles++;
	masterClockCycles %= 12;
}

void mainClockTickPAL(Cpu f, PPU p) {
	if (!(masterClockCycles % 16)) {
		f.exec();
	}
	if (!(masterClockCycles % 5)) {
		p.tick();
	}
	masterClockCycles++;
	masterClockCycles %= 12;
}


inline Rom::Rom(FILE* romFile, u8* ram) {
	this->ram = ram;
	if (romFile == NULL) {
		printf("Error: ROM file not found");
		exit(1);
	}
	u8 header[16];

	fread(header, 1, 16, romFile);
	if (header[0] != 'N' || header[1] != 'E' || header[2] != 'S' || header[3] != 0x1A) {
		printf("Error: ROM file is not a valid NES ROM");
		exit(1);
	}

	mapperType = ((header[6] & 0xF0)) | ((header[7] & 0xF0) << 4);
	mirroringType = header[6] & 1;
	mirror = mirroringType;
	prgRomSize = header[4];
	chrRomSize = header[5];
	prgRom = (u8*)calloc(prgRomSize * 0x4000, sizeof(u8));
	chrRom = (u8*)calloc(chrRomSize * 0x2000, sizeof(u8));

	if (!prgRom || !chrRom) {
		printf("7FAULT!");
		exit(1);
	}
	fread(prgRom, 1, prgRomSize * 0x4000, romFile);
	fread(chrRom, 1, chrRomSize * 0x2000, romFile);

	if (mapperType == 0) {
		mapNROM();
	}
	else {
		printf("Only NROM(mapper 000) supported yet!");
		exit(0);
	}
}


void copyTo(u8* mem, u16 where, u8 what) {
	mem[where] = what;
}

inline void Rom::mapNROM() {
	u32 i = 0;
	printf("%d", prgRomSize);
	if (prgRomSize == 1) {
		do {
			copyTo(ram, i + 0x8000, prgRom[i]);
			i++;
		} while (((i + 0x8000 <= 0xBFFF)));
		i = 0;
		do {
			copyTo(ram, i + 0xC000, prgRom[i]);
			i++;
		} while ((i + 0xC000 <= 0xFFFF));
	}
	else {
		do {
			copyTo(ram, i + 0x8000, prgRom[i]);
			i++;
		} while ((i + 0x8000) <= 0xFFFF);
	}
}

inline void Rom::printInfo() {
	printf("Mapper type: %d \n", mapperType);
	printf("PRG ROM size: %d \n", prgRomSize);
}

inline u8* Rom::getChrRom() {
	return chrRom;
}

inline u8 Rom::readPrgRom(u16 addr) {
	return prgRom[addr];
}

inline u8 Rom::readChrRom(u16 addr) {
	return chrRom[addr];
}

inline u8 Rom::getChrRomSize() {
	return chrRomSize;
}

inline Rom::~Rom() {
	free(prgRom);
	free(chrRom);
}

Rom::Rom() {}

void sleepMicros(u8 micros) {
	auto t0 = Time::now();
	auto t1 = Time::now();
	fsec fs = t1 - t0;

	long long microseconds = std::chrono::duration_cast<std::chrono::microseconds>(fs).count();

	while (microseconds < micros) {
		auto t1 = Time::now();
		fsec fs = t1 - t0;
		microseconds = std::chrono::duration_cast<std::chrono::microseconds>(fs).count();
	}
}

int mainSYS(Screen scr, FILE *testFile) {
	
	ram = (u8*)calloc((1 << 16), sizeof(u8));
	// PPU POWER UP STATE NEEDS THIS!!
	
	prog = ram;

	Rom rom(testFile, ram);
	rom.printInfo();
	Cpu f(ram, ram);

	chrrom = rom.getChrRom();
	PPU p(scr.getPixelsPointer(), scr, rom);

	//#define SOUND
	#ifdef SOUND
	std::thread tsound(soundmain);
    #endif
	switch (NTSC) {
	case NTSC:
		printf("\ngoing NTSC mode!");
		while (1) {
			//sleepMicros(2);
			Sleep(2);
			for (int i = 0; i < 20000; i++) {
				mainClockTickNTSC(f, p);
			}

			//mainClockTickNTSC(f, p);
		}
		break;
	case PAL:
		while (1) {
			//sleepMicros(1);
			mainClockTickPAL(f, p);
		}
	default:
		printf("rom ERROR!");
		exit(1);
	}
#ifdef SOUND
	tsound.join();
#endif
}

