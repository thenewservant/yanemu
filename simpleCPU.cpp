#include "simpleCPU.hpp"

uint16_t pc;
uint8_t ac = 0, xr, yr, sr = SR_RST, sp = 0xFF;

bool jammed = false; // is the cpu jammed because of certain instructions?

uint8_t* ram = (uint8_t*)calloc((1 << 16), sizeof(uint8_t));

uint8_t* prog = ram;
uint8_t caca = 0xDD;


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

void check_NZ(uint8_t obj) {
	sr = (sr & ~N_FLAG) | (obj & N_FLAG);
	sr = (sr & ~Z_FLAG) | ((obj == 0) << 1);
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
	termB = ram[prog[pc] + xr];
	_adc();
	pc++;
}

void _6DadcA() {
	termB = ram[((prog[pc + 1] << 8) | prog[pc])];
	_adc();
	pc += 2;
}
void _7DadcA() {
	termB = ram[((prog[pc + 1] << 8) | prog[pc]) + xr];
	_adc();
	pc += 2;
}

void _79adcA() {
	termB = ram[ram[((prog[pc + 1] << 8) | prog[pc])]];
	_adc();
	pc += 2;
}

void _61adcN() {
	termB = ram[ram[(uint8_t)(prog[pc] + xr)] | (ram[(uint8_t)(prog[pc] + xr + 1)] << 8)];
	_adc();
	pc++;
}

void _71adcN() {
	termB = ram[(uint16_t)((ram[prog[pc]] | (ram[(uint8_t)(prog[pc] + 1)] << 8)) + yr)];
	_adc();
	pc++;
}

void _00brk_() {
	jammed = true;
	NOP;
	printf("BRK");
}

void _29andI() {
	ac &= prog[pc];
	check_NZ(ac);
	pc++;
}

void _25andZ() {
	ac &= ram[prog[pc]];
	check_NZ(ac);
	pc++;
}

void _35andZ() {
	ac &= ram[prog[pc] + xr];
	check_NZ(ac);
	pc++;
}

void _2DandA() {
	ac &= ram[(prog[pc + 1] << 8) | prog[pc]];
	check_NZ(ac);
	pc += 2;
}

void _3DandA() {
	ac &= ram[((prog[pc + 1] << 8) | prog[pc]) + xr];
	check_NZ(ac);
	pc += 2;
}

void _39andA() {
	ac &= ram[((prog[pc + 1] << 8) | prog[pc]) + yr];
	check_NZ(ac);
	pc += 2;
}

void _21andN() {
	ac &= ram[ram[(uint8_t)(prog[pc] + xr)] | (ram[(uint8_t)(prog[pc] + xr + 1)] << 8)];
	check_NZ(ac);
	pc++;
}

void _31andN() {
	ac &= ram[(uint16_t)((ram[prog[pc]] | (ram[(uint8_t)(prog[pc] + 1)] << 8)) + yr)];
	check_NZ(ac);
	pc++;
}


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
	termB = ram[((prog[pc + 1] << 8) | prog[pc])];
	sr = sr & ~N_FLAG & ~V_FLAG & ~Z_FLAG | termB & (N_FLAG | V_FLAG) | (((ac & termB) == 0) ? Z_FLAG : 0);
	pc += 2;
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
	m = ram[ram[(uint8_t)(prog[pc] + xr)] | (ram[(uint8_t)(prog[pc] + xr + 1)] << 8)];
	_cmp();
	pc++;
}

void _D1cmpN() {
	m = ram[(uint16_t)((ram[prog[pc]] | (ram[(uint8_t)(prog[pc] + 1)] << 8)) + yr)];
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
	check_NZ(--ram[prog[pc]]);
	pc++;
}

void _D6dexZ() {
	check_NZ(--ram[prog[pc] + xr]);
	pc++;
}

void _CEdecA() {
	check_NZ(--ram[(prog[pc + 1] << 8) | prog[pc]]);
	pc += 2;
}

void _DEdexA() {
	check_NZ(--ram[((prog[pc + 1] << 8) | prog[pc]) + xr]);
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
	ac ^= ram[prog[pc]];
	check_NZ(ac);
	pc++;
}

void _55eorZ() {
	ac ^= ram[prog[pc] + xr];
	check_NZ(ac);
	pc++;
}

void _4DeorA() {
	ac ^= ram[(prog[pc + 1] << 8) | prog[pc]];
	check_NZ(ac);
	pc += 2;
}

void _5DeorA() {
	ac ^= ram[((prog[pc + 1] << 8) | prog[pc]) + xr];
	check_NZ(ac);
	pc += 2;
}

void _59eorA() {
	ac ^= ram[((prog[pc + 1] << 8) | prog[pc]) + yr];
	check_NZ(ac);
	pc += 2;
}

void _41eorN() {
	ac ^= ram[ram[(uint8_t)(prog[pc] + xr)] | (ram[(uint8_t)(prog[pc] + xr + 1)] << 8)];
	check_NZ(ac);
	pc++;
}

void _51eorN() {
	ac ^= ram[(uint16_t)((ram[prog[pc]] | (ram[(uint8_t)(prog[pc] + 1)] << 8)) + yr)];
	check_NZ(ac);
	pc++;
}

void _E6incZ() {
	check_NZ(++ram[prog[pc]]);
	pc++;
}

void _F6incZ() {
	check_NZ(++ram[prog[pc] + xr]);
	pc++;
}

void _EEincA() {
	check_NZ(++ram[(prog[pc + 1] << 8) | prog[pc]]);
	pc += 2;
}

void _FEincA() {
	pc += 2;
	check_NZ(++ram[((prog[pc + 1] << 8) | prog[pc]) + xr]);
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
	pc = ram[bigEndianTmp] | (ram[bigEndianTmp + 1] << 8);
}

void _20jsrA() {
	bigEndianTmp = pc + 1;
	ram[STACK_END | sp] = bigEndianTmp >> 8;
	ram[STACK_END | sp - 1] = bigEndianTmp;
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
	ac = ram[prog[pc] + xr];
	pc++;
	check_NZ(ac);
}

void _ADldaA() {
	ac = ram[(prog[pc + 1] << 8) | prog[pc]];
	pc += 2;
	check_NZ(ac);
}

void _BDldaA() {
	ac = ram[((prog[pc + 1] << 8) | prog[pc]) + xr];
	pc += 2;
	check_NZ(ac);
}

void _B9ldaA() {
	ac = ram[((prog[pc + 1] << 8) | prog[pc]) + yr];
	pc += 2;
	check_NZ(ac);
}

void _A1ldaN() {
	ac = ram[ram[(uint8_t)(prog[pc] + xr)] | (ram[(uint8_t)(prog[pc] + xr + 1)] << 8)];
	pc++;
	check_NZ(ac);
}
//
void _B1ldaN() {
	ac = ram[(uint16_t)((ram[prog[pc]] | (ram[(uint8_t)(prog[pc] + 1)] << 8)) + yr)];
	pc++;
	check_NZ(ac);
}
//ram[ ram[prog[pc]] + yr];
//turns to: ram[(uint16_t)((ram[prog[pc]] | (ram[(uint8_t)(prog[pc] + 1)] <<8)) + yr)]

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
	xr = ram[prog[pc] + yr];
	pc++;
	check_NZ(xr);
}

void _AEldxA() {
	xr = ram[(prog[pc + 1] << 8) | prog[pc]];
	pc += 2;
	check_NZ(xr);
}

void _BEldxA() {
	xr = ram[((prog[pc + 1] << 8) | prog[pc]) + yr];
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
	yr = ram[prog[pc] + xr];
	pc++;
	check_NZ(yr);
}

void _ACldyA() {
	yr = ram[(prog[pc + 1] << 8) | prog[pc]];
	pc += 2;
	check_NZ(yr);
}

void _BCldyA() {
	yr = ram[((prog[pc + 1] << 8) | prog[pc]) + xr];
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
	ram[prog[pc] + xr] >>= 1;
	pc++;
	check_NZ(ram[prog[pc] + xr]);
}

void _4ElsrA() {
	sr = sr & ~N_FLAG & ~C_FLAG | ac & C_FLAG;
	ram[(prog[pc + 1] << 8) | prog[pc]] >>= 1;
	check_NZ(ram[(prog[pc + 1] << 8) | prog[pc]]);
	pc += 2;
}

void _5ElsrA() {
	sr = sr & ~N_FLAG & ~C_FLAG | ac & C_FLAG;
	ram[((prog[pc + 1] << 8) | prog[pc]) + xr] >>= 1;
	check_NZ(ram[((prog[pc + 1] << 8) | prog[pc]) + xr]);
	pc += 2;
}


void _01oraN() {
	ac |= ram[ram[(uint8_t)(prog[pc] + xr)] | (ram[(uint8_t)(prog[pc] + xr + 1)] << 8)];
	check_NZ(ac);
	pc++;
}

void _05oraZ() {
	ac |= ram[prog[pc]];
	check_NZ(ac);
	pc++;
}

void _09oraI() {
	ac |= prog[pc];
	check_NZ(ac);
	pc++;
}

void _0DoraA() {
	ac |= ram[(prog[pc + 1] << 8) | prog[pc]];
	check_NZ(ac);
	pc += 2;
}

void _11oraN() {
	ac |= ram[(uint16_t)((ram[prog[pc]] | (ram[(uint8_t)(prog[pc] + 1)] << 8)) + yr)];
	pc++;
	check_NZ(ac);
}

void _15oraZ() {
	ac |= ram[prog[pc] + xr];
	pc++;
	check_NZ(ac);
}

void _19oraA() {
	ac |= ram[((prog[pc + 1] << 8) | prog[pc]) + yr];
	pc += 2;
	check_NZ(ac);
}

void _1DoraA() {
	ac |= ram[((prog[pc + 1] << 8) | prog[pc]) + xr];
	pc += 2;
	check_NZ(ac);
}

//ASL

void _0Aasl_() {
	sr = sr & ~C_FLAG | ((ac & N_FLAG) ? C_FLAG : 0);
	ac = ac << 1;
	check_NZ(ac);
}

void _06aslZ() { // one bit left shift, shifted out bit is preserved in carry
	sr = sr & ~C_FLAG | ((ram[(uint8_t)prog[pc]] & N_FLAG) ? C_FLAG : 0);
	ram[prog[pc]] <<= 1;
	check_NZ(ram[prog[pc]]);
	pc++;
}

void _16aslZ() {
	sr = sr & ~C_FLAG | ((ram[(uint8_t)prog[pc] + xr] & N_FLAG) ? C_FLAG : 0);
	ram[prog[pc] + xr] <<= 1;
	check_NZ(ram[prog[pc] + xr]);
	pc++;
}


void _0EaslA() {
	sr = sr & ~C_FLAG | ((ram[(prog[pc + 1] << 8) | prog[pc]] & N_FLAG) ? C_FLAG : 0);
	ram[(prog[pc + 1] << 8) | prog[pc]] <<= 1;
	check_NZ(ram[(prog[pc + 1] << 8) | prog[pc]]);
	pc += 2;
}

void _1EaslA() {
	sr = sr & ~C_FLAG | ((ram[((prog[pc + 1] << 8) | prog[pc]) + xr] & N_FLAG) ? C_FLAG : 0);
	ram[((prog[pc + 1] << 8) | prog[pc]) + xr] <<= 1;
	check_NZ(ram[((prog[pc + 1] << 8) | prog[pc]) + xr]);
	pc += 2;
}


void _48phaM() {
	ram[STACK_END | sp] = ac;
	sp -= 1;
}

void _08phpM() {
	ram[STACK_END | sp--] = sr;
}

void _68plaM() {
	ac = ram[STACK_END | ++sp];
	check_NZ(ac);
}

void _28plpM() { //ignores flags (B and _)
	sr = sr & (B_FLAG | __FLAG) | ram[STACK_END | ++sp] & ~(B_FLAG | __FLAG);
}


uint8_t roTmp; // tmp used for storing the MSB or LSB before cycle-shifting
bool futureC; // carry holder

void _2ArolC() {
	futureC = (ac & N_FLAG) > 0;
	ac = (ac << 1) | sr & C_FLAG;
	sr = sr & ~C_FLAG | futureC; // N_FLAG is bit 7, carry will be set to it
	check_NZ(ac);
}

void _26rolZ() {
	futureC = ram[prog[pc]] & N_FLAG;
	ram[prog[pc]] = (ram[prog[pc]] << 1) | sr & C_FLAG;
	sr = sr & ~C_FLAG | (futureC > 0);
	check_NZ(ram[prog[pc]]);
	pc++;
}

void _36rolZ() {
	futureC = ram[prog[pc] + xr] & N_FLAG;
	ram[prog[pc] + xr] = (ram[prog[pc] + xr] << 1) | sr & C_FLAG;
	sr = sr & ~C_FLAG | (futureC > 0);
	check_NZ(ram[prog[pc] + xr]);
	pc++;
}

void _2ErolA() {
	futureC = ram[(prog[pc + 1] << 8) | prog[pc]] & N_FLAG;
	ram[(prog[pc + 1] << 8) | prog[pc]] = (ram[(prog[pc + 1] << 8) | prog[pc]] << 1) | sr & C_FLAG;
	sr = sr & ~C_FLAG | (futureC > 0);
	check_NZ(ram[(prog[pc + 1] << 8) | prog[pc]]);
	pc += 2;
}

void _3ErolA() {
	futureC = ram[((prog[pc + 1] << 8) | prog[pc]) + xr] & N_FLAG;
	ram[((prog[pc + 1] << 8) | prog[pc]) + xr] = (ram[((prog[pc + 1] << 8) | prog[pc]) + xr] << 1) | sr & C_FLAG;
	sr = sr & ~C_FLAG | (futureC > 0);
	check_NZ(ram[((prog[pc + 1] << 8) | prog[pc]) + xr]);
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
	ram[prog[pc]] = (ram[prog[pc]] >> 1) | ((sr & C_FLAG) ? N_FLAG : 0);
	sr = sr & ~C_FLAG | futureC;
	check_NZ(ram[prog[pc]]);
	pc++;
}

void _76rorZ() {
	futureC = ram[prog[pc] + xr] & 1;
	ram[prog[pc] + xr] = (ram[prog[pc] + xr] >> 1) | (sr & C_FLAG) << 7;
	sr = sr & ~C_FLAG | futureC;
	check_NZ(ram[prog[pc] + xr]);
	pc++;
}

void _6ErorA() {
	futureC = ram[(prog[pc + 1] << 8) | prog[pc]] & 1;
	ram[(prog[pc + 1] << 8) | prog[pc]] = (ram[(prog[pc + 1] << 8) | prog[pc]] >> 1) | (sr & C_FLAG) << 7;
	sr = sr & ~C_FLAG | futureC;
	check_NZ(ram[(prog[pc + 1] << 8) | prog[pc]]);
	pc += 2;
}

void _7ErorA() {
	futureC = ram[((prog[pc + 1] << 8) | prog[pc]) + xr] & 1;
	ram[((prog[pc + 1] << 8) | prog[pc]) + xr] = (ram[((prog[pc + 1] << 8) | prog[pc]) + xr] >> 1) | (sr & C_FLAG) << 7;
	sr = sr & ~C_FLAG | futureC;
	check_NZ(ram[((prog[pc + 1] << 8) | prog[pc]) + xr]);
	pc += 2;
}


void _40rtiM() {
	sr = sr & (B_FLAG | __FLAG) | ram[STACK_END | ++sp] & ~(B_FLAG | __FLAG);
	pc = ram[STACK_END | (uint8_t)(sp + 1)] | (ram[STACK_END | (uint8_t)(sp + 2)] << 8);
	sp += 2;
}

void _60rtsM() {
	pc = (ram[STACK_END | (uint8_t)(sp + 1)] | (ram[STACK_END | (uint8_t)(sp + 2)] << 8)) + 1;
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
	termB = ~ram[((prog[pc + 1] << 8) | prog[pc]) + xr];
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
	termB = ~ram[ram[(uint8_t)(prog[pc] + xr)] | (ram[(uint8_t)(prog[pc] + xr + 1)] << 8)];
	_adc();
	pc++;
}

void _F1sbcN() {
	lastReg = AC;
	termB = ~ram[(uint16_t)((ram[prog[pc]] | (ram[(uint8_t)(prog[pc] + 1)] << 8)) + yr)];
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
	ram[ram[(uint8_t)(prog[pc] + xr)] | (ram[(uint8_t)(prog[pc] + xr + 1)] << 8)] = ac;
	pc++;
}

void _91staZ() {
	ram[(uint16_t)((ram[prog[pc]] | (ram[(uint8_t)(prog[pc] + 1)] << 8)) + yr)] = ac;
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

/*ILLEGAL ETCILLEGAL 


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
//NOPS

void _EAnopM() { NOP; }

void _80nopI() { NOP; pc++; }

void _04nopZ() { NOP; pc++; }

void _0CnopA() { NOP; pc += 2; }


void (*opCodePtr[])() = { _00brk_, _01oraN, _02jam, E, _04nopZ, _05oraZ, _06aslZ, _07sloZ,
						_08phpM, _09oraI, _0Aasl_, _0BancI, _0CnopA, _0DoraA, _0EaslA, _0FsloA,
						_10bplR, _11oraN, E, E, _04nopZ, _15oraZ, _16aslZ, E,
						_18clcM, _19oraA, _EAnopM, E, _0CnopA, _1DoraA, _1EaslA, E,
						_20jsrA, _21andN, E, E, _24bitZ, _25andZ,
						_26rolZ, _27rlaZ, _28plpM, _29andI, _2ArolC, E, _2CbitA, _2DandA,
						_2ErolA, _2FrlaA, _30bmiR, _31andN, E, E, _04nopZ, _35andZ,
						_36rolZ, _37rlaZ, _38secM, _39andA, _EAnopM, _3BrlaA, _0CnopA, _3DandA,
						_3ErolA, _3FrlaA, _40rtiM, _41eorN, E, E, _04nopZ, _45eorZ, _46lsrZ,
						E, _48phaM, _49eorI, _4AlsrC, E, _4CjmpA, _4DeorA, _4ElsrA,
						E, _50bvcR, _51eorN, E, E, _04nopZ, _55eorZ, _56lsrZ,
						E, _58cliM, _59eorA, _EAnopM, E, _0CnopA, _5DeorA, _5ElsrA,
						E, _60rtsM, _61adcN, E, E, _04nopZ, _65adcZ, _66rorZ,
						E, _68plaM, _69adcI, _6ArorC, E, _6CjmpN, _6DadcA, _6ErorA,
						E, _70bvsR, _71adcN, E, E, _04nopZ, _75adcZ, _76rorZ,
						E, _78seiM, _79adcA, _EAnopM, E, _0CnopA, _7DadcA, _7ErorA,
						E, _80nopI, _81staZ, _80nopI, E, _84styZ, _85staZ, _86stxZ,
						E, _88deyM, _80nopI, _8AtxaM, E, _8CstyA, _8DstaA, _8EstxA,
						E, _90bccR, _91staZ, E, E, _94styZ, _95staZ, _96stxZ,
						E, _98tyaM, _99staA, _9AtxsM, E, E, _9DstaA, E,
						E, _A0ldyI, _A1ldaN, _A2ldxI, E, _A4ldyZ, _A5ldaZ, _A6ldxZ,
						E, _A8tayM, _A9ldaI, _AAtaxM, E, _ACldyA, _ADldaA, _AEldxA,
						E, _B0bcsR, _B1ldaN, E, E, _B4ldyZ, _B5ldaZ, _B6ldxZ,
						E, _B8clvM, _B9ldaA, _BAtsxM, E, _BCldyA, _BDldaA, _BEldxA,
						E, _C0cpyI, _C1cmpN, _80nopI, E, _C4cpyZ, _C5cmpZ, _C6decZ,
						_C7dcpZ, _C8inyM, _C9cmpI, _CAdexM, E, _CCcpyA, _CDcmpA, _CEdecA,
						E, _D0bneR, _D1cmpN, E, E, _04nopZ, _D5cmpZ, _D6dexZ,
						_D7dcpZ, _D8cldM, _D9cmpA, _EAnopM, E, _0CnopA, _DDcmpA, E,
						E, _E0cpxI, _E1sbcN, _80nopI, E, _E4cpxZ, _E5sbcZ, _E6incZ,
						E, _E8inxM, _E9sbcI, _EAnopM, _E9sbcI, _ECcpxA, _EDsbcA, _EEincA,
						E, _F0beqR, _F1sbcN, E, E, _04nopZ, _F5sbcZ, _F6incZ,
						E, _F8sedM, _F9sbcA, _EAnopM, E, _0CnopA, _FDsbcA, E, E
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
	uint32_t counter; // how many instr. executed so far?

public:
	Cpu(uint8_t* ram, uint8_t* pr) {
		mem = ram;
		prog = pr;
	}

	void exec(uint8_t* prgm) {
		opCodePtr[prgm[(pc)++]]();
	}

	void run(uint8_t sleepTime = 0) {
		printf("count|op|pc count |   ac    xr    yr    sp      sr   NV-BDIZC");

		while (!jammed) {
			counter += 1;
			if (sleepTime)Sleep(sleepTime);

			printf("\n%5d:%X:", counter, prog[pc]); afficher(); exec(prog);
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

	return 0;
}