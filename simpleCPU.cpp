#include "simpleCPU.hpp"

uint16_t pc;
uint8_t ac = 0, xr, yr, sr = SR_RST, sp = 0xFD;

bool jammed = false; // is the cpu jammed because of certain instructions?

uint8_t* ram = (uint8_t*)calloc((1 << 16), sizeof(uint8_t));
uint8_t* chrrom = (uint8_t*)malloc(0x2000 * sizeof(uint8_t));
uint8_t* prog = ram;

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

void _nmi() {
	ram[STACK_END | sp--] = pc >> 8;
	ram[STACK_END | sp--] = pc;
	ram[STACK_END | sp--] = sr;
	pc = (ram[0xFFFB] << 8) | ram[0xFFFA];
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
	termB = ram[(uint8_t)(prog[pc] + xr)];
	_adc();
	pc++;
}

void _6DadcA() {
	termB = ram[((prog[pc + 1] << 8) | prog[pc])];
	_adc();
	pc += 2;
}
void _7DadcA() {
	termB = ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + xr)];
	_adc();
	pc += 2;
}

void _79adcA() {
	termB = ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + yr)];
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
	sr |= I_FLAG;
	ram[STACK_END | sp--] = (pc + 2) >> 8;
	ram[STACK_END | sp--] = (pc + 2);
	ram[STACK_END | sp--] = sr;
	pc = ram[0xFFFE] | (ram[0xFFFD] << 8);

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
	ac &= ram[(uint8_t)(prog[pc] + xr)];
	check_NZ(ac);
	pc++;
}

void _2DandA() {
	ac &= ram[(prog[pc + 1] << 8) | prog[pc]];
	check_NZ(ac);
	pc += 2;
}

void _3DandA() {
	ac &= ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + xr)];
	check_NZ(ac);
	pc += 2;
}

void _39andA() {
	ac &= ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + yr)];
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
	m = ram[(uint8_t)(prog[pc] + xr)];
	_cmp();
	pc++;
}

void _CDcmpA() {
	m = ram[(prog[pc + 1] << 8) | prog[pc]];
	_cmp();
	pc += 2;
}

void _DDcmpA() {
	m = ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + xr)];
	_cmp();
	pc += 2;
}

void _D9cmpA() {
	m = ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + yr)];
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
	check_NZ(--ram[(uint8_t)(prog[pc] + xr)]);
	pc++;
}

void _CEdecA() {
	check_NZ(--ram[(prog[pc + 1] << 8) | prog[pc]]);
	pc += 2;
}

void _DEdexA() {
	check_NZ(--ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + xr)]);
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
	ac ^= ram[(uint8_t)(prog[pc] + xr)];
	check_NZ(ac);
	pc++;
}

void _4DeorA() {
	ac ^= ram[(prog[pc + 1] << 8) | prog[pc]];
	check_NZ(ac);
	pc += 2;
}

void _5DeorA() {
	ac ^= ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + xr)];
	check_NZ(ac);
	pc += 2;
}

void _59eorA() {
	ac ^= ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + yr)];
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
	check_NZ(++ram[(uint8_t)(prog[pc] + xr)]);
	pc++;
}

void _EEincA() {
	check_NZ(++ram[(prog[pc + 1] << 8) | prog[pc]]);
	pc += 2;
}

void _FEincA() {
	check_NZ(++ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + xr)]);
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
	ac = ram[(uint8_t)(prog[pc] + xr)];
	pc++;
	check_NZ(ac);
}

void _ADldaA() {
	ac = ram[(prog[pc + 1] << 8) | prog[pc]];
	pc += 2;
	check_NZ(ac);
}

void _BDldaA() {
	ac = ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + xr)];
	pc += 2;
	check_NZ(ac);
}

void _B9ldaA() {
	ac = ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + yr)];
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
	xr = ram[(uint8_t)(prog[pc] + yr)];
	pc++;
	check_NZ(xr);
}

void _AEldxA() {
	xr = ram[(prog[pc + 1] << 8) | prog[pc]];
	pc += 2;
	check_NZ(xr);
}

void _BEldxA() {
	xr = ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + yr)];
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
	yr = ram[(uint8_t)(prog[pc] + xr)];
	pc++;
	check_NZ(yr);
}

void _ACldyA() {
	yr = ram[(prog[pc + 1] << 8) | prog[pc]];
	pc += 2;
	check_NZ(yr);
}

void _BCldyA() {
	yr = ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + xr)];
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
	ram[(uint8_t)(prog[pc] + xr)] >>= 1;
	pc++;
	check_NZ(ram[(uint8_t)(prog[pc] + xr)]);
}

void _4ElsrA() {
	sr = sr & ~N_FLAG & ~C_FLAG | ac & C_FLAG;
	ram[(prog[pc + 1] << 8) | prog[pc]] >>= 1;
	check_NZ(ram[(prog[pc + 1] << 8) | prog[pc]]);
	pc += 2;
}

void _5ElsrA() {
	sr = sr & ~N_FLAG & ~C_FLAG | ac & C_FLAG;
	ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + xr)] >>= 1;
	check_NZ(ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + xr)]);
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
	ac |= ram[(uint8_t)(prog[pc] + xr)];
	pc++;
	check_NZ(ac);
}

void _19oraA() {
	ac |= ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + yr)];
	pc += 2;
	check_NZ(ac);
}

void _1DoraA() {
	ac |= ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + xr)];
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
	ram[(uint8_t)(prog[pc] + xr)] <<= 1;
	check_NZ(ram[(uint8_t)(prog[pc] + xr)]);
	pc++;
}


void _0EaslA() {
	sr = sr & ~C_FLAG | ((ram[(prog[pc + 1] << 8) | prog[pc]] & N_FLAG) ? C_FLAG : 0);
	ram[(prog[pc + 1] << 8) | prog[pc]] <<= 1;
	check_NZ(ram[(prog[pc + 1] << 8) | prog[pc]]);
	pc += 2;
}

void _1EaslA() {
	sr = sr & ~C_FLAG | ((ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + xr)] & N_FLAG) ? C_FLAG : 0);
	ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + xr)] <<= 1;
	check_NZ(ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + xr)]);
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
	futureC = ram[(uint8_t)(prog[pc] + xr)] & N_FLAG;
	ram[(uint8_t)(prog[pc] + xr)] = (ram[(uint8_t)(prog[pc] + xr)] << 1) | sr & C_FLAG;
	sr = sr & ~C_FLAG | (futureC > 0);
	check_NZ(ram[(uint8_t)(prog[pc] + xr)]);
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
	futureC = ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + xr)] & N_FLAG;
	ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + xr)] = (ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + xr)] << 1) | sr & C_FLAG;
	sr = sr & ~C_FLAG | (futureC > 0);
	check_NZ(ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + xr)]);
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
	futureC = ram[(uint8_t)(prog[pc] + xr)] & 1;
	ram[(uint8_t)(prog[pc] + xr)] = (ram[(uint8_t)(prog[pc] + xr)] >> 1) | (sr & C_FLAG) << 7;
	sr = sr & ~C_FLAG | futureC;
	check_NZ(ram[(uint8_t)(prog[pc] + xr)]);
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
	futureC = ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + xr)] & 1;
	ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + xr)] = (ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + xr)] >> 1) | (sr & C_FLAG) << 7;
	sr = sr & ~C_FLAG | futureC;
	check_NZ(ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + xr)]);
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

	termB = ~ram[(uint8_t)(prog[pc] + xr)];
	_adc();
	pc++;
}

void _EDsbcA() {

	termB = ~ram[(prog[pc + 1] << 8) | prog[pc]];
	_adc();
	pc += 2;
}

void _FDsbcA() {

	termB = ~ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + xr)];
	_adc();
	pc += 2;
}

void _F9sbcA() {

	termB = ~ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + yr)];
	_adc();
	pc += 2;
}

void _E1sbcN() {

	termB = ~ram[ram[(uint8_t)(prog[pc] + xr)] | (ram[(uint8_t)(prog[pc] + xr + 1)] << 8)];
	_adc();
	pc++;
}

void _F1sbcN() {

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
	ram[(uint8_t)(prog[pc] + xr)] = ac;
	pc++;
}

void _8DstaA() {
	ram[(prog[pc + 1] << 8) | prog[pc]] = ac;
	pc += 2;
}

void _9DstaA() {
	ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + xr)] = ac;
	pc += 2;
}

void _99staA() {
	ram[(uint16_t)(((prog[pc + 1] << 8) | prog[pc]) + yr)] = ac;
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
	ram[(uint8_t)(prog[pc] + yr)] = xr;
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
	ram[(uint8_t)(prog[pc] + xr)] = yr;
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
	ram[(uint8_t)(prog[pc] + xr)] -= 1;
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



void (*opCodePtr[])() = { _00brk_, _01oraN, _02jamM, E, _04nopZ, _05oraZ, _06aslZ, _07sloZ,
						_08phpM, _09oraI, _0Aasl_, _0BancI, _0CnopA, _0DoraA, _0EaslA, _0FsloA,
						_10bplR, _11oraN, _02jamM, E, _04nopZ, _15oraZ, _16aslZ, E,
						_18clcM, _19oraA, _EAnopM, E, _0CnopA, _1DoraA, _1EaslA, E,
						_20jsrA, _21andN, _02jamM, E, _24bitZ, _25andZ,
						_26rolZ, _27rlaZ, _28plpM, _29andI, _2ArolC, E, _2CbitA, _2DandA,
						_2ErolA, _2FrlaA, _30bmiR, _31andN, _02jamM, E, _04nopZ, _35andZ,
						_36rolZ, _37rlaZ, _38secM, _39andA, _EAnopM, _3BrlaA, _0CnopA, _3DandA,
						_3ErolA, _3FrlaA, _40rtiM, _41eorN, _02jamM, E, _04nopZ, _45eorZ, _46lsrZ,
						E, _48phaM, _49eorI, _4AlsrC, E, _4CjmpA, _4DeorA, _4ElsrA,
						E, _50bvcR, _51eorN, _02jamM, E, _04nopZ, _55eorZ, _56lsrZ,
						E, _58cliM, _59eorA, _EAnopM, E, _0CnopA, _5DeorA, _5ElsrA,
						E, _60rtsM, _61adcN, _02jamM, E, _04nopZ, _65adcZ, _66rorZ,
						E, _68plaM, _69adcI, _6ArorC, E, _6CjmpN, _6DadcA, _6ErorA,
						E, _70bvsR, _71adcN, _02jamM, E, _04nopZ, _75adcZ, _76rorZ,
						E, _78seiM, _79adcA, _EAnopM, E, _0CnopA, _7DadcA, _7ErorA,
						E, _80nopI, _81staZ, _80nopI, E, _84styZ, _85staZ, _86stxZ,
						E, _88deyM, _80nopI, _8AtxaM, E, _8CstyA, _8DstaA, _8EstxA,
						E, _90bccR, _91staZ, _02jamM, E, _94styZ, _95staZ, _96stxZ,
						E, _98tyaM, _99staA, _9AtxsM, E, E, _9DstaA, E,
						E, _A0ldyI, _A1ldaN, _A2ldxI, E, _A4ldyZ, _A5ldaZ, _A6ldxZ,
						E, _A8tayM, _A9ldaI, _AAtaxM, E, _ACldyA, _ADldaA, _AEldxA,
						E, _B0bcsR, _B1ldaN, _02jamM, E, _B4ldyZ, _B5ldaZ, _B6ldxZ,
						E, _B8clvM, _B9ldaA, _BAtsxM, E, _BCldyA, _BDldaA, _BEldxA,
						E, _C0cpyI, _C1cmpN, _80nopI, E, _C4cpyZ, _C5cmpZ, _C6decZ,
						_C7dcpZ, _C8inyM, _C9cmpI, _CAdexM, E, _CCcpyA, _CDcmpA, _CEdecA,
						E, _D0bneR, _D1cmpN, _02jamM, E, _04nopZ, _D5cmpZ, _D6dexZ,
						_D7dcpZ, _D8cldM, _D9cmpA, _EAnopM, E, _0CnopA, _DDcmpA, _DEdexA,
						E, _E0cpxI, _E1sbcN, _80nopI, E, _E4cpxZ, _E5sbcZ, _E6incZ,
						E, _E8inxM, _E9sbcI, _EAnopM, _E9sbcI, _ECcpxA, _EDsbcA, _EEincA,
						E, _F0beqR, _F1sbcN, _02jamM, E, _04nopZ, _F5sbcZ, _F6incZ,
						E, _F8sedM, _F9sbcA, _EAnopM, E, _0CnopA, _FDsbcA, _FEincA, E
};

uint8_t nums = 0;
uint64_t loops = 0;

uint8_t* prgromm = (uint8_t*)malloc(0xFFFF * sizeof(uint8_t));


class Rom {// basic for just NROM support
private:
	uint8_t mapperType;
	uint8_t prgRomSize; // in 16kB units
	uint8_t chrRomSize; // in 8kB units
	uint8_t* prgRom;
	uint8_t* chrRom;
	uint8_t resetPosition;
public:
	//constructor is 
	Rom(FILE* romFile) {
		if (romFile == NULL) {
			printf("Error: ROM file not found");
			exit(1);
		}
		uint8_t header[16];

		fread(header, 1, 16, romFile);
		if (header[0] != 'N' || header[1] != 'E' || header[2] != 'S' || header[3] != 0x1A) {
			printf("Error: ROM file is not a valid NES ROM");
			exit(1);
		}

		mapperType = ((header[6] & 0xF0)) | ((header[7] & 0xF0) << 4);
		prgRomSize = header[4];
		chrRomSize = header[5];
		prgRom = (uint8_t*)malloc(prgRomSize * 0x4000);
		chrRom = (uint8_t*)malloc(chrRomSize * 0x2000);
		fread(prgRom, 1, prgRomSize * 0x4000, romFile);
		fread(chrRom, 1, chrRomSize * 0x2000, romFile);

		if (mapperType == 0) {
			prgromm = prgRom;
			mapNROM();
		}
		else {
			printf("Error: Only NROM (mapper 000) supported yet!");
		}
	}

	void mapNROM() {

		printf("%X", prgRomSize);
		uint16_t i = 0;
		if (prgRomSize == 1) {
			do {
				ram[i + 0x8000] = prgRom[i];
				i++;
			} while (((i + 0x8000 <= 0xBFFF)));
			i = 0;
			do {
				ram[i + 0xC000] = prgRom[i];
				i++;
			} while (((i + 0xC000 <= 0xFFFF)));
		}
		else {
			do {
				ram[i + 0x8000] = prgRom[i];
				i++;
			} while (((i + 0x8000 <= 0xFFFF)));
		}
	}

	void printInfo() {
		printf("Mapper type: %d \n", mapperType);
		printf("PRG ROM size: %d \n", prgRomSize);
	}
	uint8_t* getChrRom() {
		return chrRom;
	}
	uint8_t readPrgRom(uint16_t addr) {
		return prgRom[addr];
	}

	uint8_t readChrRom(uint16_t addr) {
		return chrRom[addr];
	}
	uint8_t getChrRomSize() {
		return chrRomSize;
	}
	~Rom() {
		free(prgRom);
		free(chrRom);
	}
};

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
		//if (ram[0x2002] & N_FLAG & ram[0x2000]) _nmi();
		opCodePtr[prgm[(pc)++]]();
	}

	void run(uint8_t sleepTime = 0) {

		while (!jammed) {

			if (sleepTime)Sleep(sleepTime);
		
			//printf("\n%5d:%2X:%2X %2X:", counter, prog[pc], prog[pc+1], prog[pc+2]); afficher(); 
			
			auto t0 = Time::now();
			auto t1 = Time::now();
			fsec fs = t1 - t0;

			long long microseconds = std::chrono::duration_cast<std::chrono::microseconds>(fs).count();

			while (microseconds < 5) {
				auto t1 = Time::now();
				fsec fs = t1 - t0;
				microseconds = std::chrono::duration_cast<std::chrono::microseconds>(fs).count();
			}

			exec(prog);
		}
		printf("\nCPU jammed at pc = $%X", pc - 1);
		//nums += 1;
	}

	void afficher() {
		printf("pc: %04X ac: %02x xr: %02x yr: %02x sp: %02x sr: %02x ", pc, ac, xr, yr, sp, sr);
		for (int i = 0; i < 8; i++) {
			printf("%d", (sr >> 7 - i) & 1);
		}
	}
};

//#define ORG 0xC000 // start position of PRG-ROM in map

void cpuf() {
	//measure the time accurately (milliseconds) it takes to run the program


	//insertAt(ram, ORG, prog2);

	Cpu f(ram, prog);

	pc = (ram[0xFFFD] << 8) | ram[0xFFFC];
	f.run(0); // SLEEP TIME

}

FILE* testFile;

int mainCPU() {
	uint32_t i = 0;
	uint8_t tmp;

	fopen_s(&testFile, "C:\\Users\\ppd49\\3D Objects\\C++\\yanemu\\tests\\snow.nes", "rb");

	Rom rom(testFile);

	chrrom = rom.getChrRom();

	if (!fopen_s) {
		printf("\nError: can't open file\n");
		exit(1);
	}

	std::thread tcpu(cpuf);

	int son = 0;
	if (son) {
		std::thread tsound(soundmain);
		tsound.join();
	}

	tcpu.join();

	return 0;
}