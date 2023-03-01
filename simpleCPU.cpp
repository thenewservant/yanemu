#include "simpleCPU.hpp"

uint16_t pc;
uint8_t ac = 0, xr, yr, sr = SR_RST, sp = 0xFF;

bool jammed = false; // is the cpu jammed because of certain instructions?

uint8_t* ram = (uint8_t*)calloc((1 << 16), sizeof(uint8_t));

uint8_t* prog = ram;
uint8_t caca = 0xDD;

//uint16_t prog2[] = {
///   0x78, 0xd8, 0xa9, 0x10, 0x8d, 0x00, 0x20, 0xa2, 0xff, 0x9a, 0xad, 0x02, 0x20, 0x10, 0xfb, 0xad, 0x02, 0x20, 0x10, 0xfb, 0xa0, 0xfe, 0xa2, 0x05, 0xbd, 0xd7, 0x07, 0xc9, 0x0a, 0xb0, 0x0c, 0xca, 0x10, 0xf6, 0xad, 0xff, 0x07, 0xc9, 0xa5, 0xd0, 0x02, 0xa0, 0xd6, 0x20, 0xcc, 0x90, 0x8d, 0x11, 0x40, 0x8d, 0x70, 0x07, 0xa9, 0xa5, 0x8d, 0xff, 0x07, 0x8d, 0xa7, 0x07, 0xa9, 0x0f, 0x8d, 0x15, 0x40, 0xa9, 0x06, 0x8d, 0x01, 0x20, 0x20, 0x20, 0x82, 0x20, 0x19, 0x8e, 0xee, 0x74, 0x07, 0xad, 0x78, 0x07, 0x09, 0x80, 0x20, 0xed, 0x8e, 0x4c, 0x57, 0x80, 0x01, 0xa4, 0xc8, 0xec, 0x10, 0x00, 0x41, 0x41, 0x4c, 0x34, 0x3c, 0x44, 0x54, 0x68, 0x7c, 0xa8, 0xbf, 0xde, 0xef, 0x03, 0x8c, 0x8c, 0x8c, 0x8d, 0x03, 0x03, 0x03, 0x8d, 0x8d, 0x8d, 0x8d, 0x8d, 0x8d, 0x8d, 0x8d, 0x8d, 0x8d, 0x8d, 0x00, 0x40, 0xad, 0x78, 0x07, 0x29, 0x7f, 0x8d, 0x78, 0x07, 0x29, 0x7e, 0x8d, 0x00, 0x20, 0xad, 0x79, 0x07, 0x29, 0xe6, 0xac, 0x74, 0x07, 0xd0, 0x05, 0xad, 0x79, 0x07, 0x09, 0x1e, 0x8d, 0x79, 0x07, 0x29, 0xe7, 0x8d, 0x01, 0x20, 0xae, 0x02, 0x20, 0xa9, 0x00, 0x20, 0xe6, 0x8e, 0x8d, 0x03, 0x20, 0xa9, 0x02, 0x8d, 0x14, 0x40, 0xae, 0x73, 0x07, 0xbd, 0x5a, 0x80, 0x85, 0x00, 0xbd, 0x6d, 0x80, 0x85, 0x01, 0x20, 0xdd, 0x8e, 0xa0, 0x00, 0xae, 0x73, 0x07, 0xe0, 0x06, 0xd0, 0x01, 0xc8
   // , 0xDEAD};

//registers_t reg = (registers_t) calloc(1,sizeof(registers_));

uint16_t bigEndianTmp; // uint16_t used to process absolute adresses

using namespace std;

/*
	_ : indicates a 6502 operation
	XX : opCode
	xxx : operation name
	M / C / _ / I / Z / A / N / R : iMplied, aCcumulator, Single-byte, Immediate, Zeropage, Absolute, iNdirect, Relative

*/
uint16_t aluTmp = 0; // used to store the result of an addition
uint8_t  termB; //used for signed alu operations with ACCUMULATOR

// multi-purpose core procedures
uint8_t lastReg;

void check_NZ() {
	/*
	sr = (sr & ~N_FLAG) | (ac | xr | yr) & N_FLAG;
	sr = (sr & ~Z_FLAG) | (((ac == 0) & (xr == 0) & (yr == 0)) << 1);*/
	switch (lastReg) {
	case AC:
		sr = (sr & ~N_FLAG) | ac & N_FLAG;
		sr = (sr & ~Z_FLAG) | ((ac == 0) << 1);
		break;
	case XR:
		sr = (sr & ~N_FLAG) | xr & N_FLAG;
		sr = (sr & ~Z_FLAG) | ((xr == 0) << 1);
		break;
	case YR:
		sr = (sr & ~N_FLAG) | yr & N_FLAG;
		sr = (sr & ~Z_FLAG) | ((yr == 0) << 1);
		break;
	default:
		sr = (sr & ~N_FLAG) | lastReg & N_FLAG;
		sr = (sr & ~Z_FLAG) | ((lastReg == 0) << 1);
		break;
	}

}

inline void check_CV() {
	sr = (aluTmp) > 0xff ? (sr | C_FLAG) : sr & ~C_FLAG;
	sr = (sr & ~V_FLAG) | V_FLAG & (((ac ^ aluTmp) & (termB ^ aluTmp)) >> 1);
}

//adc core procedure -- termB is the operand as termA is always ACCUMULATOR
inline void _adc() {
	aluTmp = ac + termB + (sr & C_FLAG);
	check_CV();
	ac = aluTmp; // relies on auto uint8_t conversion.
	check_NZ();
}



void _69adcI() { //seemS OK
	lastReg = AC;
	termB = prog[pc];
	_adc();
	pc++;
}

void _65adcZ() { //seems OK
	lastReg = AC;
	termB = ram[prog[pc]];
	_adc();
	pc++;
}

void _75adcZ() {
	lastReg = AC;
	termB = ram[prog[pc] + xr];
	_adc();
	pc++;
}

void _6DadcA() {
	lastReg = AC;
	termB = ram[((prog[pc + 1] << 8) | prog[pc])];
	_adc();
	pc += 2;
}
void _7DadcA() {
	lastReg = AC;
	termB = ram[((prog[pc + 1] << 8) | prog[pc]) + xr];
	_adc();
	pc += 2;
}

void _79adcA() {
	lastReg = AC;
	termB = ram[ram[((prog[pc + 1] << 8) | prog[pc])]];
	_adc();
	pc += 2;
}

void _61adcN() {
	lastReg = AC;
	termB = ram[ram[prog[pc] + xr]];
	_adc();
	pc += 2;
}

void _71adcN() {
	lastReg = AC;
	termB = ram[ram[prog[pc]] + yr];
	_adc();
	pc += 2;
}

void _00brk_() {
	jammed = true;
	NOP;
	printf("BRK");
}

void _29andI() {
	ac &= prog[pc];
	check_NZ();
	pc++;
}

void _25andZ() {
	ac &= ram[prog[pc]];
	check_NZ();
	pc++;
}

void _35andZ() {
	ac &= ram[prog[pc] + xr];
	check_NZ();
	pc++;
}

void _2DandA() {
	ac &= ram[(prog[pc + 1] << 8) | prog[pc]];
	check_NZ();
	pc += 2;
}

void _3DandA() {
	ac &= ram[((prog[pc + 1] << 8) | prog[pc]) + xr];
	check_NZ();
	pc += 2;
}

void _39andA() {
	ac &= ram[((prog[pc + 1] << 8) | prog[pc]) + yr];
	check_NZ();
	pc += 2;
}

void _21andN() {
	ac &= ram[ram[prog[pc] + xr]];
	check_NZ();
	pc += 2;
}

void _31andN() {
	ac &= ram[ram[prog[pc]] + yr];
	check_NZ();
	pc += 2;
}
#define RELATIVE_BRANCH_CORE ((prog[pc] & N_FLAG) ? -1 * ((uint8_t)(~prog[pc]) + 1) : prog[pc])

void _90bccR() {
	pc += ((sr & C_FLAG) == 0) ? RELATIVE_BRANCH_CORE : 0;
	pc++;
}

void _B0bcsR() {
	pc += ((sr & C_FLAG)) ? RELATIVE_BRANCH_CORE : 0;
	pc++;
}

void _F0beqR() {
	pc += ((sr & Z_FLAG)) ? RELATIVE_BRANCH_CORE : 0;
	pc++;
}

void _24bitZ() {
	termB = ram[prog[pc]];
	sr = sr & ~N_FLAG & ~V_FLAG & ~Z_FLAG | termB & (N_FLAG | V_FLAG) | (((ac & termB) == 0) ? Z_FLAG : 0);
	pc++;
}

void _2CbitA() {
	ac &= ram[((prog[pc + 1] << 8) | prog[pc])];
	sr = sr & ~N_FLAG & ~V_FLAG & ~Z_FLAG | termB & (N_FLAG | V_FLAG) | (((ac & termB) == 0) ? Z_FLAG : 0);
	pc++;
}

void _30bmiR() {
	pc += (sr & N_FLAG) ? RELATIVE_BRANCH_CORE : 0;
	pc++;
}
void _D0bneR() {
	pc += ((sr & Z_FLAG) == 0) ? RELATIVE_BRANCH_CORE : 0;
	pc++;
}

void _10bplR() {
	pc += ((sr & N_FLAG) == 0) ? RELATIVE_BRANCH_CORE : 0;
	pc++;
}


void _50bvcR() {
	pc += ((sr & V_FLAG) == 0) ? RELATIVE_BRANCH_CORE : 0;
	pc++;
}

void _70bvsR() {
	pc += (sr & V_FLAG) ? RELATIVE_BRANCH_CORE : 0;          //prog[pc]
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


uint8_t tmpN = 0; // used to store the result of a subtraction on 8 bits (to further deduce the N flag)
uint8_t m = 0; // temp memory for cmp computing

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
	m = ram[prog[pc]];
	_cmp();
	pc++;
}

void _D5cmpZ() {
	m = ram[prog[pc] + xr];
	_cmp();
	pc++;
}

void _CDcmpA() {
	m = ram[(prog[pc + 1] << 8) | prog[pc]];
	_cmp();
	pc += 2;
}

void _DDcmpA() {
	m = ram[((prog[pc + 1] << 8) | prog[pc]) + xr];
	_cmp();
	pc += 2;
}

void _D9cmpA() {
	m = ram[((prog[pc + 1] << 8) | prog[pc]) + yr];
	_cmp();
	pc += 2;
}

void _C1cmpN() {
	m = ram[ram[prog[pc] + xr]];
	_cmp();
	pc += 2;
}

void _D1cmpN() {
	m = ram[ram[prog[pc]] + yr];
	_cmp();
	pc += 2;
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
	m = ram[prog[pc]];
	_cpx();
	pc++;
}

void _ECcpxA() {
	m = ram[(prog[pc + 1] << 8) | prog[pc]];
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
	m = ram[prog[pc]];
	_cpy();
	pc++;
}

void _CCcpyA() {
	m = ram[(prog[pc + 1] << 8) | prog[pc]];
	_cpy();
	pc += 2;
}


void _C6decZ() {
	ram[prog[pc]]--;
	check_NZ();
	pc++;
}

void _D6dexZ() {
	ram[prog[pc] + xr]--;
	check_NZ();
	pc++;
}

void _CEdecA() {
	ram[(prog[pc + 1] << 8) | prog[pc]]--;
	check_NZ();
	pc += 2;
}

void _DEdexA() {
	ram[((prog[pc + 1] << 8) | prog[pc]) + xr]--;
	check_NZ();
	pc += 2;
}


void _CAdexM() {
	lastReg = XR;
	xr--;
	check_NZ();
}

void _88deyM() {
	lastReg = YR;
	yr--;
	check_NZ();
}

void _49eorI() {
    lastReg = AC;
	ac ^= prog[pc];
	check_NZ();
	pc++;
}

void _45eorZ() {
    lastReg = AC;
	ac ^= ram[prog[pc]];
	check_NZ();
	pc++;
}

void _55eorZ() {
    lastReg = AC;
	ac ^= ram[prog[pc] + xr];
	check_NZ();
	pc++;
}

void _4DeorA() {
    lastReg = AC;
	ac ^= ram[(prog[pc + 1] << 8) | prog[pc]];
	check_NZ();
	pc += 2;
}

void _5DeorA() {
    lastReg = AC;
	ac ^= ram[((prog[pc + 1] << 8) | prog[pc]) + xr];
	check_NZ();
	pc += 2;
}

void _59eorA() {
    lastReg = AC;
	ac ^= ram[((prog[pc + 1] << 8) | prog[pc]) + yr];
	check_NZ();
	pc += 2;
}

void _41eorN() {
    lastReg = AC;
	ac ^= ram[ram[prog[pc] + xr]];
	check_NZ();
	pc++;
}

void _51eorN() {
    lastReg = AC;
	ac ^= ram[ram[prog[pc]] + yr];
	check_NZ();
	pc++;
}

void _E6incZ() {
    lastReg = 	ram[prog[pc]]++;
	check_NZ();
	pc++;
}

void _F6incZ() {
    lastReg =     ram[prog[pc] + xr]++;
	check_NZ();
	pc++;
}

void _EEincA() {
    lastReg = 	ram[(prog[pc + 1] << 8) | prog[pc]]++;
	check_NZ();
	pc += 2;
}

void _FEincA() {
    lastReg = 	ram[((prog[pc + 1] << 8) | prog[pc]) + xr]++;
	pc += 2;
	check_NZ();
}

void _E8inxM() {
    lastReg = XR;
	xr++;
	check_NZ();
}

void _C8inyM() {
    lastReg = YR;
	yr++;
	check_NZ();
}


void _4CjmpA() {
	pc = prog[pc] | (prog[pc + 1] << 8);
}

void _6CjmpN() {
	bigEndianTmp = prog[pc] | prog[pc + 1] << 8;
	pc = ram[bigEndianTmp] | ram[bigEndianTmp + 1] << 8;
}

void _20jsrA() {
	bigEndianTmp = pc + 2;
	ram[STACK_END | sp] = bigEndianTmp >> 8;
	ram[STACK_END | sp - 1] = bigEndianTmp;
	sp -= 2;
	pc = prog[pc] | (prog[pc + 1] << 8);
}

void _A9ldaI() {
    lastReg = AC;
	ac = prog[pc];
	pc++;
	check_NZ();
}

void _A5ldaZ() {
    lastReg = AC;
	ac = ram[prog[pc]];
	pc++;
	check_NZ();
}

void _B5ldaZ() {
    lastReg = AC;
	ac = ram[prog[pc] + xr];
	pc++;
	check_NZ();
}

void _ADldaA() {
    lastReg = AC;
	ac = ram[(prog[pc + 1] << 8) | prog[pc]];
	pc += 2;
	check_NZ();
}

void _BDldaA() {
    lastReg = AC;
	ac = ram[((prog[pc + 1] << 8) | prog[pc]) + xr];
	pc += 2;
	check_NZ();
}

void _B9ldaA() {
    lastReg = AC;
	ac = ram[((prog[pc + 1] << 8) | prog[pc]) + yr];
	pc += 2;
	check_NZ();
}

void _A1ldaN() {
    lastReg = AC;
	ac |= ram[ram[prog[pc] + xr]];
	pc++;
	check_NZ();
}

void _B1ldaN() {
    lastReg = AC;
	ac |= ram[ram[prog[pc]] + yr];
	pc++;
	check_NZ();
}


void _A2ldxI() {
    lastReg = XR;
	xr = prog[pc];
	pc++;
	check_NZ();
}

void _A6ldxZ() {
    lastReg = XR;
	xr = ram[prog[pc]];
	pc++;
	check_NZ();
}

void _B6ldxZ() {
    lastReg = XR;
	xr = ram[prog[pc] + yr];
	pc++;
	check_NZ();
}

void _AEldxA() {
    lastReg = XR;
	xr = ram[(prog[pc + 1] << 8) | prog[pc]];
	pc += 2;
	check_NZ();
}

void _BEldxA() {
    lastReg = XR;
	xr = ram[((prog[pc + 1] << 8) | prog[pc]) + yr];
	pc += 2;
	check_NZ();
}


void _A0ldyI() {
    lastReg = YR;
	yr = prog[pc];
	pc++;
	check_NZ();
}

void _A4ldyZ() {
    lastReg = YR;
	yr = ram[prog[pc]];
	pc++;
	check_NZ();
}

void _B4ldyZ() {
    lastReg = YR;
	yr = ram[prog[pc] + xr];
	pc++;
	check_NZ();
}

void _ACldyA() {
    lastReg = YR;
	yr = ram[(prog[pc + 1] << 8) | prog[pc]];
	pc += 2;
	check_NZ();
}

void _BCldyA() {
    lastReg = YR;
	yr = ram[((prog[pc + 1] << 8) | prog[pc]) + xr];
	pc += 2;
	check_NZ();
}


void _4AlsrC() {
	sr = sr & ~N_FLAG | ac & C_FLAG;
	ac >>= 1;
	check_NZ();
}

void _46lsrZ() {
	sr = sr & ~N_FLAG | ac & C_FLAG;
	ram[prog[pc]] >>= 1;
	pc++;
	check_NZ();
}

void _56lsrZ() {
	sr = sr & ~N_FLAG | ac & C_FLAG;
	ram[prog[pc] + xr] >>= 1;
	pc++;
	check_NZ();
}

void _4ElsrA() {
	sr = sr & ~N_FLAG | ac & C_FLAG;
	ram[(prog[pc + 1] << 8) | prog[pc]] >>= 1;
	pc += 2;
	check_NZ();
}

void _5ElsrA() {
	sr = sr & ~N_FLAG | ac & C_FLAG;
	ram[((prog[pc + 1] << 8) | prog[pc]) + xr] >>= 1;
	pc += 2;
	check_NZ();
}


void _01oraN() {
    lastReg = AC;
	ac |= ram[ram[prog[pc] + xr]];
	pc++;
	check_NZ();
}

void _05oraZ() {
    lastReg = AC;
	ac |= ram[prog[pc]];
	pc++;
	check_NZ();
}

void _09oraI() {
    lastReg = AC;
	ac |= prog[pc];
	pc++;
	check_NZ();
}

void _0DoraA() {
    lastReg = AC;
	ac |= ram[(prog[pc + 1] << 8) | prog[pc]];
	pc += 2;
	check_NZ();
}

void _11oraN() {
    lastReg = AC;
	ac |= ram[ram[prog[pc]] + yr];
	pc++;
	check_NZ();
}

void _15oraZ() {
    lastReg = AC;
	ac |= ram[prog[pc] + xr];
	pc++;
	check_NZ();
}

void _19oraA() {
    lastReg = AC;
	ac |= ram[((prog[pc + 1] << 8) | prog[pc]) + yr];
	pc += 2;
	check_NZ();
}

void _1DoraA() {
    lastReg = AC;
	ac |= ram[((prog[pc + 1] << 8) | prog[pc]) + xr];
	pc += 2;
	check_NZ();
}

//ASL

void _0Aasl_() {
	sr = sr & ~C_FLAG | ac & C_FLAG;
	ac = ac << 1;
	check_NZ();
}

void _06aslZ() { // one bit left shift, shifted out bit is preserved in carry
	sr = sr & ~C_FLAG | ram[(uint8_t)prog[pc]] & C_FLAG;
	ram[(uint8_t)prog[pc]] <<= 1;
	pc++;
	check_NZ();
}

void _16aslZ() {
	sr = sr & ~C_FLAG | ram[(uint8_t)prog[pc] + xr] & C_FLAG;
	ram[(uint8_t)prog[pc] + xr] <<= 1;
	pc++;
	check_NZ();
}


void _0EaslA() {
	sr = sr & ~C_FLAG | ram[(prog[pc + 1] << 8) | prog[pc]] & C_FLAG;
	ram[(prog[pc + 1] << 8) | prog[pc]] <<= 1;
	pc += 2;
	check_NZ();
}

void _1EaslA() {
	sr = sr & ~C_FLAG | ram[((prog[pc + 1] << 8) | prog[pc]) + xr] & C_FLAG;
	ram[((prog[pc + 1] << 8) | prog[pc]) + xr] <<= 1;
	pc += 2;
	check_NZ();
}


void _48phaM() {
	ram[STACK_END | sp--] = ac;
}

void _08phpM() {
	ram[STACK_END | sp--] = sr;
}

void _68plaM() {
	ac = ram[STACK_END | ++sp];
	check_NZ();
}

void _28plpM() { //ignores flags (B and _)
	sr = sr & (B_FLAG | __FLAG) | ram[STACK_END | ++sp] & ~(B_FLAG | __FLAG);
}


uint8_t roTmp; // tmp used for storing the MSB or LSB before cycle-shifting
bool futureC; // carry holder

void _2ArolC() {
	futureC = ac & N_FLAG;
	ac = (ac << 1) | sr & C_FLAG;
	sr = sr & ~C_FLAG | futureC >> 7; // N_FLAG is bit 7, carry will be set to it
	check_NZ();
}

void _26rolZ() {
	futureC = ram[prog[pc]] & N_FLAG;
	ram[prog[pc]] = (ram[prog[pc]] << 1) | sr & C_FLAG;
	sr = sr & ~C_FLAG | futureC >> 7;
	pc++;
	check_NZ();
}

void _36rolZ() {
	futureC = ram[prog[pc] + xr] & N_FLAG;
	ram[prog[pc] + xr] = (ram[prog[pc] + xr] << 1) | sr & C_FLAG;
	pc++;
	check_NZ();
}

void _2ErolA() {
	futureC = ram[(prog[pc + 1] << 8) | prog[pc]] & N_FLAG;
	ram[(prog[pc + 1] << 8) | prog[pc]] = (ram[(prog[pc + 1] << 8) | prog[pc]] << 1) | sr & C_FLAG;
	pc += 2;
	check_NZ();
}

void _3ErolA() {
	futureC = ram[((prog[pc + 1] << 8) | prog[pc]) + xr] & N_FLAG;
	ram[((prog[pc + 1] << 8) | prog[pc]) + xr] = (ram[((prog[pc + 1] << 8) | prog[pc]) + xr] << 1) | sr & C_FLAG;
	pc += 2;
	check_NZ();
}

void _6ArorC() {
	futureC = ac & 1;
	ac = (ac >> 1) | (sr & C_FLAG) << 7;
	sr = sr & ~C_FLAG | futureC;
	check_NZ();
}

void _66rorZ() {
	futureC = ram[prog[pc]] & 1;
	ram[prog[pc]] = (ram[prog[pc]] >> 1) | (sr & C_FLAG) << 7;
	sr = sr & ~C_FLAG | futureC;
	pc++;
	check_NZ();
}

void _76rorZ() {
	futureC = ram[prog[pc] + xr] & 1;
	ram[prog[pc] + xr] = (ram[prog[pc] + xr] >> 1) | (sr & C_FLAG) << 7;
	sr = sr & ~C_FLAG | futureC;
	pc++;
	check_NZ();
}

void _6ErorA() {
	futureC = ram[(prog[pc + 1] << 8) | prog[pc]] & 1;
	ram[(prog[pc + 1] << 8) | prog[pc]] = (ram[(prog[pc + 1] << 8) | prog[pc]] >> 1) | (sr & C_FLAG) << 7;
	sr = sr & ~C_FLAG | futureC;
	pc += 2;
	check_NZ();
}

void _7ErorA() {
	futureC = ram[((prog[pc + 1] << 8) | prog[pc]) + xr] & 1;
	ram[((prog[pc + 1] << 8) | prog[pc]) + xr] = (ram[((prog[pc + 1] << 8) | prog[pc]) + xr] >> 1) | (sr & C_FLAG) << 7;
	sr = sr & ~C_FLAG | futureC;
	pc += 2;
	check_NZ();
}


void _40rtiM() {
	sr = ram[STACK_END | ++sp];
	pc = ram[STACK_END | ++sp];
}

void _60rtsM() {
	pc = ram[STACK_END | (uint8_t)(sp + 1)] | (ram[STACK_END | (uint8_t)(sp + 2)] << 8);
	sp += 2;
}


//SBCs
/* Archaic
uint8_t tmpCarry; // used for overflow
inline void _sbc() {
	aluTmp = ac + termB - ((sr & C_FLAG) ^ C_FLAG);
	tmpCarry = sr & C_FLAG;
	sr = (ac >= (uint8_t)(~termB + 1)) ? (sr | C_FLAG) : (sr & ~C_FLAG);
	//sr = (sr & ~V_FLAG) | V_FLAG & (((ac ^ aluTmp) & ((termB ) ^ aluTmp)) >> 1);
	sr = (sr & ~V_FLAG) | (V_FLAG & ((aluTmp <= 0xFF) >> 1));
	ac = aluTmp;
	check_NZ();
}
*/

void _E9sbcI() { //seems OK!
	lastReg = AC;
	termB = ~prog[pc];// (N_FLAG & prog[pc]) ? (~prog[pc] + 1) : prog[pc];
	//_sbc();
	_adc();
	pc++;
}

void _E5sbcZ() {
    lastReg = AC;
	termB = ~ram[prog[pc]];
	_adc();
	pc++;

}

void _F5sbcZ() {
    lastReg = AC;
	termB = ~ram[prog[pc] + xr];
	_adc();
	pc++;
}

void _EDsbcA() {
    lastReg = AC;
	termB = ~ram[(prog[pc + 1] << 8) | prog[pc]];
	_adc();
	pc += 2;
}

void _FDsbcA() {
    lastReg = AC;
	termB = ~ram[((prog[pc + 1] << 8) | prog[pc]) + xr] ;
	_adc();
	pc += 2;
}

void _F9sbcA() {
    lastReg = AC;
	termB = ~ram[((prog[pc + 1] << 8) | prog[pc]) + yr];
	_adc();
	pc += 2;
}

void _E1sbcN() {
    lastReg = AC;
	termB = ~ram[ram[prog[pc] + xr]];
	_adc();
	pc++;
}

void _F1sbcN() {
    lastReg = AC;
	termB = ~ram[ram[prog[pc]] + yr];
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
	ram[prog[pc]] = ac;
	pc++;
}

void _95staZ() {
	ram[prog[pc] + xr] = ac;
	pc++;
}

void _8DstaA() {
	ram[(prog[pc + 1] << 8) | prog[pc]] = ac;
	pc += 2;
}

void _9DstaA() {
	ram[((prog[pc + 1] << 8) | prog[pc]) + xr] = ac;
	pc += 2;
}

void _99staA() {
	ram[((prog[pc + 1] << 8) | prog[pc]) + yr] = ac;
	pc += 2;
}

void _81staZ() {
	ram[ram[prog[pc] + xr]] = ac;
	pc++;
}

void _91staZ() {
	ram[ram[prog[pc]] + yr] = ac;
	pc++;
}


void _86stxZ() {
	ram[prog[pc]] = xr;
	pc++;
}

void _96stxZ() {
	ram[prog[pc] + yr] = xr;
	pc++;
}

void _8EstxA() {
	ram[(prog[pc + 1] << 8) | prog[pc]] = xr;
	pc += 2;
}

void _84styZ() {
	ram[prog[pc]] = yr;
	pc++;
}

void _94styZ() {
	ram[prog[pc] + xr] = yr;
	pc++;
}

void _8CstyA() {
	ram[(prog[pc + 1] << 8) | prog[pc]] = yr;
	pc += 2;
}

void _AAtaxM() {
	xr = ac;
	check_NZ();
}

void _A8tayM() {
	yr = ac;
	check_NZ();
}

void _BAtsxM() {
	sp = xr;
	check_NZ();
}

void _8AtxaM() {
	ac = xr;
	check_NZ();
}

void _9AtxsM() {
	sp = xr;
	check_NZ();
}

void _98tyaM() {
	ac = yr;
	check_NZ();
}

/*ILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETC

ILLEGAL ETC



ILLEGAL ETC

ILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETC

*/
void _02jam() { //freezes the CPU
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

/*
DCP (DCM)

	DEC oper + CMP oper

	M - 1 -> M, A - M
	N	Z	C	I	D	V
	+	+	+	-	-	-
	addressing	assembler	opc	bytes	cycles
	zeropage	DCP oper	C7	2	5
	zeropage,X	DCP oper,X	D7	2	6
	absolute	DCP oper	CF	3	6
	absolut,X	DCP oper,X	DF	3	7
	absolut,Y	DCP oper,Y	DB	3	7
	(indirect,X)	DCP (oper,X)	C3	2	8
	(indirect),Y	DCP (oper),Y	D3	2	8
*/

void _C7dcpZ() {
	ram[prog[pc]] -= 1;
	_C5cmpZ();
	pc++;
}

void _D7dcpZ() {
	ram[prog[pc] + xr] -= 1;
	_D5cmpZ();
	pc++;
}

/*
RLA

	ROL oper + AND oper

	M = C <- [76543210] <- C, A AND M -> A
	N	Z	C	I	D	V
	+	+	+	-	-	-
	addressing	assembler	opc	bytes	cycles
	zeropage	RLA oper	27	2	5
	zeropage,X	RLA oper,X	37	2	6
	absolute	RLA oper	2F	3	6
	absolut,X	RLA oper,X	3F	3	7
	absolut,Y	RLA oper,Y	3B	3	7
	(indirect,X)	RLA (oper,X)	23	2	8
	(indirect),Y	RLA (oper),Y	33	2	8  */


	// NOPS

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

void _1AnopM() { NOP; }

void _3AnopM() { NOP; }

void _5AnopM() { NOP; }

void _7AnopM() { NOP; }

void _DAnopM() { NOP; }

void _FAnopM() { NOP; }

void _80nopI() { NOP; pc++; }

void _82nopI() { NOP; pc++; }

void _89nopI() { NOP; pc++; }

void _C2nopI() { NOP; pc++; }

void _E2nopI() { NOP; pc++; }


void _04nopZ() { NOP; pc++; }

void _44nopZ() { NOP; pc++; }

void _64nopZ() { NOP; pc++; }

void _14nopZ() { NOP; pc++; }

void _34nopZ() { NOP; pc++; }

void _54nopZ() { NOP; pc++; }

void _74nopZ() { NOP; pc++; }

void _D4nopZ() { NOP; pc++; }

void _F4nopZ() { NOP; pc++; }


void _0CnopA() { NOP; pc += 2; }

void _1CnopA() { NOP; pc += 2; }

void _3CnopA() { NOP; pc += 2; }

void _5CnopA() { NOP; pc += 2; }

void _7CnopA() { NOP; pc += 2; }

void _DCnopA() { NOP; pc += 2; }

void _FCnopA() { NOP; pc += 2; }


void _EAnopM() { NOP; }

/*
void _90bccR() {
	pc += ((sr & C_FLAG) == 0) ? RELATIVE_BRANCH_CORE : 0;
	pc++;
}

void _B0bcsR() {
	pc += ((sr & C_FLAG)) ? RELATIVE_BRANCH_CORE : 0;
	pc++;
}
*/

void (*opCodePtr[])() = { _00brk_, _01oraN, _02jam, E, _04nopZ, _05oraZ, _06aslZ, _07sloZ,
						_08phpM, _09oraI, _0Aasl_, _0BancI, _0CnopA, _0DoraA, _0EaslA, _0FsloA,
						_10bplR, _11oraN, E, E, _14nopZ, _15oraZ, _16aslZ, E,
						_18clcM, _19oraA, _1AnopM, E, _1CnopA, _1DoraA, _1EaslA, E,
						_20jsrA, _21andN, E, E, _24bitZ, _25andZ,
						_26rolZ, _27rlaZ, _28plpM, _29andI, _2ArolC, E, _2CbitA, _2DandA,
						_2ErolA, _2FrlaA, _30bmiR, _31andN, E, E, _34nopZ, _35andZ,
						_36rolZ, _37rlaZ, _38secM, _39andA, _3AnopM, _3BrlaA, _3CnopA, _3DandA,
						_3ErolA, _3FrlaA, _40rtiM, _41eorN, E, E, _44nopZ, _45eorZ, _46lsrZ,
						E, _48phaM, _49eorI, _4AlsrC, E, _4CjmpA, _4DeorA, _4ElsrA,
						E, _50bvcR, _51eorN, E, E, _54nopZ, _55eorZ, _56lsrZ,
						E, _58cliM, _59eorA, _5AnopM, E, _5CnopA, _5DeorA, _5ElsrA,
						E, _60rtsM, _61adcN, E, E, _64nopZ, _65adcZ, _66rorZ,
						E, _68plaM, _69adcI, _6ArorC, E, _6CjmpN, _6DadcA, _6ErorA,
						E, _70bvsR, _71adcN, E, E, _74nopZ, _75adcZ, _76rorZ,
						E, _78seiM, _79adcA, _7AnopM, E, _7CnopA, _7DadcA, _7ErorA,
						E, _80nopI, E, _82nopI, E, _84styZ, _85staZ, _86stxZ,
						E, _88deyM, _89nopI, _8AtxaM, E, _8CstyA, _8DstaA, _8EstxA,
						E, _90bccR, _91staZ, E, E, _94styZ, _95staZ, _96stxZ,
						E, _98tyaM, _99staA, _9AtxsM, E, E, _9DstaA, E,
						E, _A0ldyI, _A1ldaN, _A2ldxI, E, _A4ldyZ, _A5ldaZ, _A6ldxZ,
						E, _A8tayM, _A9ldaI, _AAtaxM, E, E, _ADldaA, _AEldxA,
						E, _B0bcsR, _B1ldaN, E, E, _B4ldyZ, _B5ldaZ, _B6ldxZ,
						E, _B8clvM, _B9ldaA, _BAtsxM, E, E, _BDldaA, _BEldxA,
						E, _C0cpyI, _C1cmpN, _C2nopI, E, _C4cpyZ, _C5cmpZ, _C6decZ,
						_C7dcpZ, _C8inyM, _C9cmpI, _CAdexM, E, _CCcpyA, _CDcmpA, _CEdecA,
						E, _D0bneR, _D1cmpN, E, E, _D4nopZ, _D5cmpZ, _D6dexZ,
						_D7dcpZ, _D8cldM, _D9cmpA, _DAnopM, E, _DCnopA, _DDcmpA, E,
						E, _E0cpxI, _E1sbcN, _E2nopI, E, _E4cpxZ, _E5sbcZ, _E6incZ,
						E, _E8inxM, _E9sbcI, _EAnopM, _E9sbcI, _ECcpxA, _EDsbcA, _EEincA,
						E, _F0beqR, _F1sbcN, E, E, _F4nopZ, _F5sbcZ, E,
						E, _F8sedM, _F9sbcA, _FAnopM, E, _FCnopA, _FDsbcA, E, E
};


uint8_t nums = 0;
uint64_t loops = 0;
tstime tt = { 10,10 };

BOOLEAN nanosleep(LONGLONG ns) {
	/* Declarations */
	HANDLE timer;	/* Timer handle */
	LARGE_INTEGER li;	/* Time defintion */
	/* Create timer */
	if (!(timer = CreateWaitableTimer(NULL, TRUE, NULL)))
		return FALSE;
	/* Set timer properties */
	li.QuadPart = -ns;
	if (!SetWaitableTimer(timer, &li, 0, NULL, NULL, FALSE)) {
		CloseHandle(timer);
		return FALSE;
	}
	/* Start & wait for timer */
	WaitForSingleObject(timer, INFINITE);
	/* Clean resources */
	CloseHandle(timer);
	/* Slept without problems */
	return TRUE;
}

class Cpu {
private:
	uint8_t* mem;
	uint8_t* prog;

public:
	Cpu(uint8_t* ram, uint8_t* pr) {

		mem = ram;
		prog = pr;
	}

	void exec(uint8_t* prgm) {

		opCodePtr[prgm[(pc)++]]();
	}

	void run(uint8_t sleepTime = 0) {
		if (sleepTime) {
			while (!jammed) {
				Sleep(sleepTime); printf("\n%X", prog[pc]); exec(prog); afficher();
			}
		}

		else {
			while (!jammed) {
				printf("\n%X", prog[pc]); exec(prog); afficher();
			}
		}


		printf("\nCPU jammed at pc = $%X", pc - 1);
		//nums += 1;
	}

	void afficher() {
		printf("pc: %04X ac: %02x xr: %02x yr: %02x sp: %02x sr: %02x ", pc, ac, xr, yr, sp, sr);
		for (int i = 0; i < 8; i++) {
			//if ((i!=2)&(i!=3))
			printf("%d", (sr >> 7 - i) & 1);
		}

	}

};

#define ORG 0xC000 // start position of PRG-ROM in map

void cpuf() {
	//measure the time accurately (milliseconds) it takes to run the program

	pc = ORG;
	//insertAt(ram, ORG, prog2);
	Cpu f(ram, prog);
	/*
	auto start = std::chrono::high_resolution_clock::now();
	for (uint64_t i = 0; i <1000000000; i++) {
		pc = 0;
		f.run();
		nums += 1;
	}

	auto finish = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = finish - start;
	std::cout << " elapsed time: " << elapsed.count() << " s\n";
	*/

	f.run(0); // SLEEP TIME
}

uint8_t* testMem = (uint8_t*)calloc((1 << 16), sizeof(uint8_t));
FILE* testFile;



int main() {

	fopen_s(&testFile, "C:\\Users\\ppd49\\3D Objects\\C++\\yanemu\\tests\\nestest.nes", "rb");
	if (!fopen_s) {
		printf("\nI CANt HAS VLID TEST FILE ?!!!1!11 >>:(\n");
		exit(1);
	}
	uint32_t i = 0;
	uint8_t tmp;
	uint32_t orgInRam = 0xC000;
	do {
		//printf("curr: %d\n", i);
		//tmp = getc(testFile);
		fread(&tmp, 1, 1, testFile);
		ram[i + orgInRam] = tmp;
		i++;
	} while (!feof(testFile));

	//fread(ram, 1, 1<<16, testFile);
	//testMem[6001] = 0xDEAD;

	//insertAt(ram, 0x400, testMem);
	printf("%d\n", ram[0x400]);

	//cpuf();
	//ram[0x4001] = 29;
	std::thread tcpu(cpuf);
	int son = 0;
	if (son) {
		std::thread tsound(soundmain);
		tsound.join();
	}

	tcpu.join();
	printf("\n VAL @ 02h:%X", ram[0xFF02]);
	printf("\n VAL @ 03h:%X", ram[0xFF03]);

	return 0;
}