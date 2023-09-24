#pragma warning(disable:4996)
#include "simpleCPU.h"
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
static u8 ac, xr, yr, sr = SR_RST, sp = 0xFD;
static u16 pc;
bool jammed = false; // is the cpu jammed because of certain instructions?

u8 ram[0x10000] = { 0 };
u32 cycles = 0;

u8 latchCommandCtrl1 = 0;
u8 latchCommandCtrl2 = 0;
u8 keys1 = 0;
u8 keys2 = 0;
u8 keyLatchCtrl1 = 0; // controller 1 buttons
u8 keyLatchCtrl2 = 0; // controller 2 buttons

Rom MainRom;
Mapper* mapper;
PPU* ppu;

u32 comCount[256] = { 0 }; // count how many times each command is executed

u32 getCycles() {
	return cycles;
}

inline bool elemIn(u8 elem, u8* arr, u8 size) {
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
		u16 maxIndex = 0;
		u32 max = 0;
		for (u16 i = 0; i < 256; i++) {
			if ((comCount[i] > max) && !elemIn((u8)i, mostUsed, 10)) {
				max = comCount[i];
				maxIndex = i;

			}
		}mostUsed[top] = (u8)maxIndex;
	}
	printf("\nMost used commands :\n");
	for (u8 i = 0; i < 10; i++) {
		printf("%d: %2X - %d times\n", i + 1, mostUsed[i], comCount[mostUsed[i]]);
	}
	printf("PC: %4X\n", pc);
	printf("The I Flag is %s\n", (sr & 0b00000100) >> 2 ? "SET" : "CLEAR");
}

void pressKey(u16 pressed) {
	keys1 = (u8)pressed;
	keys2 = (u8)(pressed >> 8);
}

//mainly for OAMDMA alignment.
void addCycle() {
	cycles++;
}

void rstCtrl() {
	//reset controller latches
	latchCommandCtrl1 = 0;
	latchCommandCtrl2 = 0;
	keyLatchCtrl1 = 0;
	keyLatchCtrl2 = 0;
	keys1 = 0;
	keys2 = 0;
}

void changeMirror() {
	if (mirror == VERTICAL) {
		mirror = HORIZONTAL;
	}
	else {
		mirror = VERTICAL;
	}
}

u8 rdRegisters(u16 where) {
	switch (where) {
	case 0x2002:
		return ppu->readPPUSTATUS();
	case 0x2004:
		return ppu->readOAM();
	case 0x2007:
		return ppu->rdPPU();
	default:
		return ram[where];
	}
}

//rd is READ. Used to filter certain accesses (such as PPUADDR latch)
u8 rd(u16 at) {
	if (at <= 0x1FFF) {
		//printf("rd %4X\n", at);
		return ram[at & 0x7FF];
	}
	else if (at >= 0x2000 && at <= 0x3FFF) {
		return rdRegisters(at & 0x2007);
	}
	else if (at >= 0x6000) {
			return mapper->rdCPU(at);
	}
	switch (at) {
	case 0x4016:
		if (latchCommandCtrl1) {
			ram[0x4016] = ram[0x4016] & 0xFE | (keyLatchCtrl1) & 1;
			keyLatchCtrl1 >>= 1;
			keyLatchCtrl1 |= 0x80;
			return 0x40 | ram[0x4016];
		}
		else {
			return keys1 & 1;
		}
		
	case 0x4017: 
		if (latchCommandCtrl2) {
			ram[0x4017] = ram[0x4017] & 0xFE | (keyLatchCtrl2) & 1;
			keyLatchCtrl2 >>= 1;
			keyLatchCtrl2 |= 0x80;
			return 0x40 | ram[0x4017];
		}
		else {
			return keys2 & 1;
		}
		
	default:
		return ram[at];
	}	
}

inline void writeRegisters(u16 where, u8 what) {
	switch (where) {
	case 0x2000:
		ppu->writePPUCTRL(what);
		break;
	case 0x2001:
		ppu->writePPUMASK(what);
		ram[where] = what;
	case 0x2003:
		ppu->setOAMADDR(what);
		break;
	case 0x2004:
		ppu->writeOAM(what);
		break;
	case 0x2005:
		ppu->writePPUSCROLL(what);
		break;
	case 0x2006:
		ppu->writePPUADDR(what);
		break;
	case 0x2007:
		ppu->writePPU(what);
		break;
	default:
		break;
	}
}

inline void wr(u16 where, u8 what) {
	if (where <= 0x1FFF) {
		ram[where & 0x07FF] = what;
	}
	else if (where >= 0x2000 && where <= 0x3FFF) {
		writeRegisters(where&0x2007,what);
	}
	else if ((where >= 0x6000) && (where <= 0xFFFF)) {
		mapper->wrCPU(where, what);
	}
	else {
		switch (where) {
		case 0x4014:
			ram[0x4014] = what;
			ppu->updateOam();
			cycles += 513;
			return;
		case 0x4016:

			latchCommandCtrl1 = !(what & 1);
			latchCommandCtrl2 = !(what & 1);
			if (latchCommandCtrl1) {
				keyLatchCtrl1 = keys1;
			}
			if (latchCommandCtrl2) {
				keyLatchCtrl2 = keys2;
			}
			return;
		}

		if ((where >= 0x4000) && (where <= 0x4017)) {
			ram[where] = what;
			updateAPU(where);
		}
	}
}

//absolute memory access with arg (xr or yr) offset
inline u16 absArg(u8 arg, u8 cyc = 1) {
	u16 where = (rd(pc + 1) << 8) | rd(pc);
	u16 w2 = where + arg;
	if ((where & 0xFF00) != (w2 & 0xFF00)) {
		cycles += cyc;
	}
	return w2;
}

inline u16 indX() {
	return ram[(u8)(rd(pc) + xr)] | (ram[(u8)(rd(pc) + xr + 1)] << 8);
}

inline u16 indY(u8 cyc = 1) {
	u16 where = ram[rd(pc)] | (ram[(u8)(rd(pc) + 1)] << 8);
	u16 w2 = where + yr;
	if ((where & 0xFF00) != (w2 & 0xFF00)) {
		cycles += cyc;
	}
	return w2;
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

void _nmi() 
{
	wr(STACK_END | sp--, pc >> 8);
	wr(STACK_END | sp--, (u8)pc);
	wr(STACK_END | sp--, sr);
	sr |= I_FLAG;
	pc = ((rd(0xFFFB) << 8) | rd(0xFFFA)) ;
}

void _rst() {
	sp -= 3;
	sr |= I_FLAG;
	pc = (rd(0xFFFD) << 8) | rd(0xFFFC);
	ram[0x4015] = 0;
}

//adc core procedure -- termB is the operand as termA is always ACCUMULATOR
inline void _adc() {
	aluTmp = ac + termB + (sr & C_FLAG);
	check_CV();
	ac = (u8)aluTmp;
	check_NZ(ac);
}

void _69adcI() { //seemS OK
	termB = rd(pc);
	_adc();
	pc++;
}

void _65adcZ() { //seems OK
	termB = ram[rd(pc)];
	_adc();
	pc++;
}

void _75adcZ() {
	termB = rd((u8)(rd(pc) + xr));
	_adc();
	pc++;
}

void _6DadcA() {
	termB = rd(((rd(pc + 1) << 8) | rd(pc)));
	_adc();
	pc += 2;
}

void _7DadcA() {
	termB = rd(absArg(xr));
	_adc();
	pc += 2;
}

void _79adcA() {
	termB = rd(absArg(yr));
	_adc();
	pc += 2;
}

void _61adcN() {
	termB = rd(indX());
	_adc();
	pc++;
}

void _71adcN() {
	termB = rd(indY());
	_adc();
	pc++;
}

void _00brk_() {
	wr(STACK_END | sp--, (u8)((pc + 1) >> 8));
	wr(STACK_END | sp--, (u8)(pc + 1));
	wr(STACK_END | sp--, sr|B_FLAG);
	sr |= I_FLAG;
	pc = rd(0xFFFE) | (rd(0xFFFF) << 8);
}

void manualIRQ() {
	if((sr & I_FLAG)  == 0) { _00brk_(); }
}

void _29andI() {
	ac &= rd(pc);
	check_NZ(ac);
	pc++;
}

void _25andZ() {
	ac &= ram[rd(pc)];
	check_NZ(ac);
	pc++;
}

void _35andZ() {
	ac &= rd((u8)(rd(pc) + xr));
	check_NZ(ac);
	pc++;
}

void _2DandA() {
	ac &= rd((rd(pc + 1) << 8) | rd(pc));
	check_NZ(ac);
	pc += 2;
}

void _3DandA() {
	ac &= rd(absArg(xr));
	check_NZ(ac);
	pc += 2;
}

void _39andA() {
	ac &= rd(absArg(yr));
	check_NZ(ac);
	pc += 2;
}

void _21andN() {
	ac &= rd(indX());
	check_NZ(ac);
	pc++;
}

void _31andN() {
	ac &= rd(indY());
	check_NZ(ac);
	pc++;
}

int8_t  _rel() {
	u8 rdTmp = rd(pc);
	int8_t relTmp = (rdTmp & N_FLAG) ? - ((~rdTmp) + 1) : rdTmp;
	if ((rdTmp & 0xFF00) != ((rdTmp + relTmp) & 0xFF00)) {
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
	termB = ram[rd(pc)];
	sr = sr & ~(N_FLAG | V_FLAG) | termB & (N_FLAG | V_FLAG);
	sr = sr & ~Z_FLAG | (((ac & termB) == 0) ? Z_FLAG : 0);
	pc++;
}

void _2CbitA() {
	termB = rd((rd(pc + 1) << 8) | rd(pc));
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
	m = rd(pc);
	_cmp();
	pc++;
}

void _C5cmpZ() {
	m = ram[rd(pc)];
	_cmp();
	pc++;
}

void _D5cmpZ() {
	m = ram[(u8)(rd(pc) + xr)];
	_cmp();
	pc++;
}

void _CDcmpA() {
	m = rd((rd(pc + 1) << 8) | rd(pc));
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
	m = rd(indX());
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
	m = rd(pc);
	_cpx();
	pc++;
}

void _E4cpxZ() {
	m = ram[rd(pc)];
	_cpx();
	pc++;
}

void _ECcpxA() {
	m = rd((rd(pc + 1) << 8) | rd(pc));
	_cpx();
	pc += 2;
}

inline void _cpy() {
	u8 tmpN = yr - m;
	sr = sr & ~N_FLAG & ~Z_FLAG & ~C_FLAG | ((yr == m) ? Z_FLAG : 0) | ((yr >= m) ? C_FLAG : 0) | tmpN & N_FLAG;
}

void _C0cpyI() {
	m = rd(pc);
	_cpy();
	pc++;
}

void _C4cpyZ() {
	m = ram[rd(pc)];
	_cpy();
	pc++;
}

void _CCcpyA() {
	m = rd((rd(pc + 1) << 8) | rd(pc));
	_cpy();
	pc += 2;
}


void _C6decZ() {
	check_NZ(--ram[rd(pc)]);
	pc++;
}

void _D6decZ() {
	check_NZ(--ram[(u8)(rd(pc) + xr)]);
	pc++;
}

void _CEdecA() {
	u16 tmp = (rd(pc + 1) << 8) | rd(pc);
	wr(tmp, rd(tmp) - 1);
	check_NZ(rd(tmp));
	pc += 2;
}

void _DEdecA() {
	u16 tmp = absArg(xr, 0);
	wr(tmp, rd(tmp) - 1);
	check_NZ(rd(tmp));
	pc += 2;
}

void _CAdexM() {
	check_NZ(--xr);
}

void _88deyM() {
	check_NZ(--yr);
}

void _49eorI() {
	ac ^= rd(pc);
	check_NZ(ac);
	pc++;
}

void _45eorZ() {
	ac ^= ram[rd(pc)];
	check_NZ(ac);
	pc++;
}

void _55eorZ() {
	ac ^= rd((u8)(rd(pc) + xr));
	check_NZ(ac);
	pc++;
}

void _4DeorA() {
	ac ^= rd((rd(pc + 1) << 8) | rd(pc));
	check_NZ(ac);
	pc += 2;
}

void _5DeorA() {
	ac ^= rd(absArg(xr));
	check_NZ(ac);
	pc += 2;
}

void _59eorA() {
	ac ^= rd(absArg(yr));
	check_NZ(ac);
	pc += 2;
}

void _41eorN() {
	ac ^= rd(indX());
	check_NZ(ac);
	pc++;
}

void _51eorN() {
	ac ^= rd(indY());
	check_NZ(ac);
	pc++;
}

void _E6incZ() {
	check_NZ(++ram[rd(pc)]);
	pc++;
}

void _F6incZ() {
	check_NZ(++ram[(u8)(rd(pc) + xr)]);
	pc++;
}

void _EEincA() {
	u16 tmp = (rd(pc + 1) << 8) | rd(pc);
	wr(tmp, rd(tmp) + 1);
	check_NZ(rd(tmp));
	pc += 2;
}

void _FEincA() {
	u16 tmp = absArg(xr, 0);
	wr(tmp, rd(tmp) + 1);
	check_NZ(rd(tmp));
	pc += 2;
}

void _E8inxM() {
	check_NZ(++xr);
}

void _C8inyM() {
	check_NZ(++yr);
}


void _4CjmpA() {
	pc = rd(pc) | (rd(pc + 1) << 8);
}

void _6CjmpN() {
	u16 pcTmp = rd(pc) | (rd(pc + 1) << 8);
	pc = rd(pcTmp) | (rd((pcTmp & 0xFF00) | (pcTmp + 1) & 0x00FF) << 8); // Here comes the (in)famous Indirect Jump bug implementation...
}


void _20jsrA() {
	u16 pcTmp = pc + 1;
	wr(STACK_END | sp--, (u8)(pcTmp >> 8));
	wr(STACK_END | sp--, (u8)pcTmp);
	pc = rd(pc) | (rd(pc + 1) << 8);
}

void _A9ldaI() {
	ac = rd(pc);
	pc++;
	check_NZ(ac);
}

void _A5ldaZ() {
	ac = ram[rd(pc)];
	pc++;
	check_NZ(ac);
}

void _B5ldaZ() {
	ac = ram[(u8)(rd(pc) + xr)];
	pc++;
	check_NZ(ac);
}

void _ADldaA() {
	ac = rd((rd(pc + 1) << 8) | rd(pc));
	pc += 2;
	check_NZ(ac);
}

void _BDldaA() {
	ac = rd(absArg(xr));
	pc += 2;
	check_NZ(ac);
}

void _B9ldaA() {
	ac = rd(absArg(yr));
	pc += 2;
	check_NZ(ac);
}

void _A1ldaN() {
	ac = rd(indX());
	pc++;
	check_NZ(ac);
}

void _B1ldaN() {
	ac = rd(indY());
	pc++;
	check_NZ(ac);
}

void _A2ldxI() {
	xr = rd(pc);
	pc++;
	check_NZ(xr);
}

void _A6ldxZ() {
	xr = ram[rd(pc)];
	pc++;
	check_NZ(xr);
}

void _B6ldxZ() {
	xr = ram[(u8)(rd(pc) + yr)];
	pc++;
	check_NZ(xr);
}

void _AEldxA() {
	xr = rd((rd(pc + 1) << 8) | rd(pc));
	pc += 2;
	check_NZ(xr);
}

void _BEldxA() {
	xr = rd(absArg(yr));
	pc += 2;
	check_NZ(xr);
}


void _A0ldyI() {
	yr = rd(pc);
	pc++;
	check_NZ(yr);
}

void _A4ldyZ() {
	yr = ram[rd(pc)];
	pc++;
	check_NZ(yr);
}

void _B4ldyZ() {
	yr = rd((u8)(rd(pc) + xr));
	pc++;
	check_NZ(yr);
}

void _ACldyA() {
	yr = rd((rd(pc + 1) << 8) | rd(pc));
	pc += 2;
	check_NZ(yr);
}

void _BCldyA() {
	yr = rd(absArg(xr));
	pc += 2;
	check_NZ(yr);
}


void _lsr(u16 where) {
	u8 tmp = rd(where);
	sr = sr & ~N_FLAG & ~C_FLAG | tmp & C_FLAG;
	wr(where, tmp >> 1);
	check_NZ(rd(where));
}

void _4AlsrC() {
	sr = sr & ~N_FLAG & ~C_FLAG | ac & C_FLAG;
	ac >>= 1;
	check_NZ(ac);
}

void _46lsrZ() {
	_lsr(rd(pc));
	pc++;
}

void _56lsrZ() {
	_lsr((u8)(rd(pc) + xr));
	pc++;
}

void _4ElsrA() {
	_lsr((rd(pc + 1) << 8) | rd(pc));
	pc += 2;
}

void _5ElsrA() {
	_lsr(absArg(xr, 0));
	pc += 2;
}

void _01oraN() {
	ac |= rd(indX());
	check_NZ(ac);
	pc++;
}

void _05oraZ() {
	ac |= ram[rd(pc)];
	check_NZ(ac);
	pc++;
}

void _09oraI() {
	ac |= rd(pc);
	check_NZ(ac);
	pc++;
}

void _0DoraA() {
	ac |= rd((rd(pc + 1) << 8) | rd(pc));
	check_NZ(ac);
	pc += 2;
}

void _11oraN() {
	ac |= rd(indY());
	pc++;
	check_NZ(ac);
}

void _15oraZ() {
	ac |= ram[(u8)(rd(pc) + xr)];
	pc++;
	check_NZ(ac);
}

void _19oraA() {
	ac |= rd(absArg(yr));
	pc += 2;
	check_NZ(ac);
}

void _1DoraA() {
	ac |= rd(absArg(xr));
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

inline void _asl(u16 where) {
	sr = sr & ~C_FLAG | ((rd(where) & N_FLAG) ? C_FLAG : 0);
	wr(where, rd(where) << 1);
	check_NZ(rd(where));
}

void _0Aasl_() {
	sr = sr & ~C_FLAG | ((ac & N_FLAG) ? C_FLAG : 0);
	ac <<= 1;
	check_NZ(ac);
}

void _06aslZ() { // one bit left shift, shifted out bit is preserved in carry
	_asl(rd(pc));
	pc++;
}

void _16aslZ() {
	_asl((u8)(rd(pc) + xr));
	pc++;
}

void _0EaslA() {
	_asl((rd(pc + 1) << 8) | rd(pc));
	pc += 2;
}

void _1EaslA() {
	_asl(((rd(pc + 1) << 8) | rd(pc)) + xr);
	pc += 2;
}

u8 roTmp; // tmp used for storing the MSB or LSB before cycle-shifting
bool futureC; // carry holder

void _rol(u16 where) {
	futureC = rd(where) & N_FLAG;
	wr(where, (rd(where) << 1) | sr & C_FLAG);
	sr = sr & ~C_FLAG | futureC;
	check_NZ(rd(where));
}

void _2ArolC() {
	futureC = (ac & N_FLAG) > 0;
	ac = (ac << 1) | sr &C_FLAG;
	sr = sr & ~C_FLAG | futureC; // N_FLAG is bit 7, carry will be set to it
	check_NZ(ac);
}

void _26rolZ() {
	_rol(rd(pc));
	pc++;
}

void _36rolZ() {
	_rol((u8)(rd(pc) + xr));
	pc++;
}

void _2ErolA() {
	_rol((rd(pc + 1) << 8) | rd(pc));
	pc += 2;
}

void _3ErolA() {
	_rol(((rd(pc + 1) << 8) | rd(pc)) + xr);
	pc += 2;
}

void _ror(u16 where) {
	futureC = rd(where) & 1;
	wr(where, (rd(where) >> 1) | ((sr & C_FLAG) ? N_FLAG : 0));
	sr = sr & ~C_FLAG | futureC;
	check_NZ(rd(where));
}

void _6ArorC() {
	futureC = ac & 1;
	ac = (ac >> 1) | (sr & C_FLAG) << 7;
	sr = sr & ~C_FLAG | futureC;
	check_NZ(ac);
}

void _66rorZ() {
	_ror(rd(pc));
	pc++;
}

void _76rorZ() {
	_ror((u8)(rd(pc) + xr));
	pc++;
}

void _6ErorA() {
	_ror((rd(pc + 1) << 8) | rd(pc));
	pc += 2;
}

void _7ErorA() {
	_ror(((rd(pc + 1) << 8) | rd(pc)) + xr);
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

//SBC

void _E9sbcI() { //seems OK!
	termB = ~rd(pc);
	_adc();
	pc++;
}

void _E5sbcZ() {
	termB = ~ram[rd(pc)];
	_adc();
	pc++;
}

void _F5sbcZ() {
	termB = ~ram[(u8)(rd(pc) + xr)];
	_adc();
	pc++;
}

void _EDsbcA() {
	termB = ~rd((rd(pc + 1) << 8) | rd(pc));
	_adc();
	pc += 2;
}

void _FDsbcA() {
	termB = ~rd(absArg(xr));
	_adc();
	pc += 2;
}

void _F9sbcA() {
	termB = ~rd(absArg(yr));
	_adc();
	pc += 2;
}


void _E1sbcN() {
	termB = ~rd(indX());
	_adc();
	pc++;
}

void _F1sbcN() {
	termB = ~rd(indY());
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
	ram[rd(pc)] = ac;
	pc++;
}

void _95staZ() {
	ram[(u8)(rd(pc) + xr)] = ac;
	pc++;
}

void _8DstaA() {
	u16 tmpPC =  (rd(pc + 1) << 8) | rd(pc);
	pc += 2;
	wr(tmpPC, ac);
	
}

void _9DstaA() {
	u16 tmpPC = (((rd(pc + 1) << 8) | rd(pc)) + xr);
	pc += 2;
	wr(tmpPC, ac);
}

void _99staA() {
	u16 tmpPC = (((rd(pc + 1) << 8) | rd(pc)) + yr);
	pc += 2;
	wr(tmpPC, ac);
}

void _81staZ() {
	wr(indX(), ac);
	pc++;
}

void _91staZ() {
	wr(indY(0), ac);
	pc++;
}


void _86stxZ() {
	ram[rd(pc)] = xr;
	pc++;
}

void _96stxZ() {
	ram[(u8)(rd(pc) + yr)] = xr;
	pc++;
}

void _8EstxA() {
	u16 tmpPC = (rd(pc + 1) << 8) | rd(pc);
	pc += 2;
	wr(tmpPC, xr);
}

void _84styZ() {
	ram[rd(pc)] = yr;
	pc++;
}

void _94styZ() {
	ram[(u8)(rd(pc) + xr)] = yr;
	pc++;
}

void _8CstyA() {
	u16 tmpPC = (rd(pc + 1) << 8) | rd(pc);
	pc += 2;
	wr(tmpPC, yr);
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

void _4BalrI() {
	_29andI();
	pc--;
	_4AlsrC();
}

void _0BancI() {
	_29andI();
	pc--;
	sr = sr & ~C_FLAG | ((rd(pc) & N_FLAG) ? C_FLAG : 0);
	pc++;
}

//LAX

void _A7laxZ() {
	_A5ldaZ();
	_AAtaxM();
}

void _B7laxZ() {
	ac = ram[(u8)(rd(pc) + yr)];
	_AAtaxM();
	pc++;
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

void _6BarrM() {
	pc++;
}

//DCP
void _dcp(u16 where) {
	m = rd(where) - 1;
	wr(where, m);
	_cmp();
}

void _C7dcpZ() {
	_dcp(rd(pc));
	pc++;
}

void _D7dcpZ() {
	_dcp((u8)(rd(pc) + xr));
	pc++;
}

void _CFdcpA() {
	_dcp((rd(pc + 1) << 8) | rd(pc));
	pc += 2;
}

void _DFdcpA() {
	_dcp(absArg(xr));
	pc += 2;
}

void _DBdcpA() {
	_dcp(absArg(yr));
	pc += 2;
}

void _C3dcpN() {
	_dcp(indX());
	pc += 1;
}

void _D3dcpN() {
	_dcp(indY());
	pc += 1;
}

// ISC

void _isc(u16 where) {
	termB = rd(where)+1;
	wr(where, termB);
	termB = ~termB;
	_adc();
}

void _E7iscZ() {
	_isc(rd(pc));
	pc++;
}

void _F7iscZ() {
	_isc((u8)(rd(pc) + xr));
	pc++;
}

void _EFiscA() {
	_isc((rd(pc + 1) << 8) | rd(pc));
	pc += 2;
}

void _FFiscA() {
	_isc(absArg(xr));
	pc += 2;
}

void _FBiscA() {
	_isc(absArg(yr));
	pc += 2;
}

void _E3iscN() {
	_isc(indX());
	pc += 1;
}

void _F3iscN() {
	_isc(indY());
	pc += 1;
}

void _ABlxaI() {
	ac = xr = ac & rd(pc);
	check_NZ(ac);
	pc++;
}

//RLA

void _27rlaZ() {
	_26rolZ();
	pc--;
	_25andZ();
}

void _37rlaZ() {
	_36rolZ();
	pc--;
	_35andZ();
}

void _2FrlaA() {
	_2ErolA();
	pc-=2;
	_2DandA();
}

void _3FrlaA() {
	_3ErolA();
	pc-=2;
	_3DandA();
}

void _3BrlaA() {
	_rol(absArg(yr));
	_39andA();
}

void _23rlaN() {
	_rol(indX());
	_21andN();
}

void _33rlaN() {
	_rol(indY());
	_31andN();
}

//RRA


void _rra(u16 where) {
	_ror(where);
	termB = rd(where);
	_adc();
}

void _67rraZ() {
	_rra(rd(pc));
	pc++;
}

void _77rraZ() {
	_rra((u8)(rd(pc) + xr));
	pc++;
}

void _6FrraA(){ 
	_rra((rd(pc + 1) << 8) | rd(pc));
	pc += 2; 
}

void _7FrraA() {
	_rra(absArg(xr));
	pc += 2;
}

void _7BrraA() {
	_rra(absArg(yr));
	pc += 2;
}

void _63rraN() {
	_rra(indX());
	pc += 1;
}

void _73rraN() {
	_rra(indY());
	pc += 1;
}

//SAX

void _87saxZ() {
	wr((rd(pc)), ac & xr);
	pc++;
}

void _97saxZ() {
	wr((u8)(rd(pc) + yr), ac & xr);
	pc++;
}

void _8FsaxA() {
	wr(((rd(pc + 1) << 8) | rd(pc)), ac & xr);
	pc += 2;
}

void _83saxN() {
	wr(indX(), ac & xr);
	pc++;
}


void _CBsbxI() {
	pc++;
}

void _9FshaA() {
	pc += 2;
}

void _9EshxA() {
	pc += 2;
}

void _9CshyA() {
	pc += 2;
}

//SLO

void _slo(u16 where) {
	_asl(where);
	ac |= rd(where);
	check_NZ(ac);
}

void _07sloZ() {
	_slo(rd(pc));
	pc++;
}

void _17sloZ() {
	_slo((u8)(rd(pc) + xr));
	pc++;
}

void _0FsloA() {
	_slo((rd(pc + 1) << 8) | rd(pc));
	pc += 2;
}

void _1FsloA() {
	_slo(absArg(xr));
	pc += 2;
}

void _1BsloA() {
	_slo(absArg(yr));
	pc += 2;
}

void _03sloN() {
	_slo(indX());
	pc += 1;
}

void _13sloN() {
	_slo(indY());
	pc += 1;
}

//SRE

void _sre(u16 where) {
	_lsr(where);
	ac ^= rd(where);
	check_NZ(ac);
}

void _47sre() {
	_sre(rd(pc));
	pc++;
}

void _57sre() {
	_sre((u8)(rd(pc) + xr));
	pc++; 
}

void _4Fsre() {
	_sre((rd(pc + 1) << 8) | rd(pc));
	pc += 2;
}

void _5Fsre() {
	_sre(absArg(xr));
	pc += 2;
}

void _5Bsre() {
	_sre(absArg(yr));
	pc += 2;
}

void _43sre() {
	_sre(indX());
	pc += 1;
}

void _53sre() {
	_sre(indY());
	pc += 1;
}

//NOPS

void _EAnopM() { NOP; }

void _80nopI() { NOP; pc++; }

void _04nopZ() { NOP; pc++; }

void _0CnopA() { NOP; pc += 2; }

void _1CnopA() { NOP; absArg(xr); pc += 2; }


void _illegal() {
	printf("\nThis ILLEGAL OPCODE is not implemented YET! :%X, %X, %X - pc:%4X\n", rd(pc - 1), rd(pc), rd(pc + 1), pc-1);
	exit(0x1);
}


void (*opCodePtr[])() = { _00brk_, _01oraN, _02jamM, _03sloN, _04nopZ, _05oraZ, _06aslZ, _07sloZ,
						_08phpM, _09oraI, _0Aasl_, _0BancI, _0CnopA, _0DoraA, _0EaslA, _0FsloA,
						_10bplR, _11oraN, _02jamM, _13sloN, _04nopZ, _15oraZ, _16aslZ, _17sloZ,
						_18clcM, _19oraA, _EAnopM, _1BsloA, _1CnopA, _1DoraA, _1EaslA, _1FsloA,
						_20jsrA, _21andN, _02jamM, _23rlaN, _24bitZ, _25andZ,
						_26rolZ, _27rlaZ, _28plpM, _29andI, _2ArolC, _0BancI, _2CbitA, _2DandA,
						_2ErolA, _2FrlaA, _30bmiR, _31andN, _02jamM, _33rlaN, _04nopZ, _35andZ,
						_36rolZ, _37rlaZ, _38secM, _39andA, _EAnopM, _3BrlaA, _1CnopA, _3DandA,
						_3ErolA, _3FrlaA, _40rtiM, _41eorN, _02jamM, _43sre, _04nopZ, _45eorZ, _46lsrZ,
						_47sre, _48phaM, _49eorI, _4AlsrC, _4BalrI, _4CjmpA, _4DeorA, _4ElsrA,
						_4Fsre, _50bvcR, _51eorN, _02jamM, _53sre, _04nopZ, _55eorZ, _56lsrZ,
						_57sre, _58cliM, _59eorA, _EAnopM, _5Bsre, _1CnopA, _5DeorA, _5ElsrA,
						_5Fsre, _60rtsM, _61adcN, _02jamM, _63rraN, _04nopZ, _65adcZ, _66rorZ,
						_67rraZ, _68plaM, _69adcI, _6ArorC, _6BarrM, _6CjmpN, _6DadcA, _6ErorA,
						_6FrraA, _70bvsR, _71adcN, _02jamM, _73rraN, _04nopZ, _75adcZ, _76rorZ,
						_77rraZ, _78seiM, _79adcA, _EAnopM, _7BrraA, _1CnopA, _7DadcA, _7ErorA,
						_7FrraA, _80nopI, _81staZ, _80nopI, _83saxN, _84styZ, _85staZ, _86stxZ,
						_87saxZ, _88deyM, _80nopI, _8AtxaM, E, _8CstyA, _8DstaA, _8EstxA,
						_8FsaxA, _90bccR, _91staZ, _02jamM, E, _94styZ, _95staZ, _96stxZ,
						_97saxZ, _98tyaM, _99staA, _9AtxsM, E, _9CshyA, _9DstaA, _9EshxA,
						_9FshaA, _A0ldyI, _A1ldaN, _A2ldxI, _A3laxN, _A4ldyZ, _A5ldaZ, _A6ldxZ,
						_A7laxZ, _A8tayM, _A9ldaI, _AAtaxM, _ABlxaI, _ACldyA, _ADldaA, _AEldxA,
						_AFlaxA, _B0bcsR, _B1ldaN, _02jamM, _B3laxN, _B4ldyZ, _B5ldaZ, _B6ldxZ,
						_B7laxZ, _B8clvM, _B9ldaA, _BAtsxM, E, _BCldyA, _BDldaA, _BEldxA,
						_BFlaxA, _C0cpyI, _C1cmpN, _80nopI, _C3dcpN, _C4cpyZ, _C5cmpZ, _C6decZ,
						_C7dcpZ, _C8inyM, _C9cmpI, _CAdexM, _CBsbxI, _CCcpyA, _CDcmpA, _CEdecA,
						_CFdcpA, _D0bneR, _D1cmpN, _02jamM, _D3dcpN, _04nopZ, _D5cmpZ, _D6decZ,
						_D7dcpZ, _D8cldM, _D9cmpA, _EAnopM, _DBdcpA, _1CnopA, _DDcmpA, _DEdecA,
						_DFdcpA, _E0cpxI, _E1sbcN, _80nopI, _E3iscN, _E4cpxZ, _E5sbcZ, _E6incZ,
						_E7iscZ, _E8inxM, _E9sbcI, _EAnopM, _E9sbcI, _ECcpxA, _EDsbcA, _EEincA,
						_EFiscA, _F0beqR, _F1sbcN, _02jamM, _F3iscN, _04nopZ, _F5sbcZ, _F6incZ,
						_F7iscZ, _F8sedM, _F9sbcA, _EAnopM, _FBiscA, _1CnopA, _FDsbcA, _FEincA, _FFiscA
};


Cpu::Cpu(u8* ram, u8* pr) {
	mem = ram;
	prog = pr;
	pc = (rd(0xFFFD) << 8) | rd(0xFFFC);

}

void Cpu::afficher() {
	printf("pc: %04X ac: %02x xr: %02x yr: %02x sp: %02x sr: %02x PPU: %3d SL: %3d \n", pc, ac, xr, yr, sp, sr, (3 * cycles) % 341, -1 + (((((3 * cycles)) / 341)) % 262));
	for (int i = 0; i < 8; i++) {
		printf("%d", (sr >> (7 - i)) & 1);
	}
}

u8 masterClockCycles = 0;

int mainSYS(Screen scr, FILE* testFile) {


	Rom rom(testFile, ram);
	MainRom = rom;
	MainRom.printInfo();
	mapper = MainRom.getMapper();
	Cpu f(ram, ram);
	PPU p(scr.getPixels(), scr, MainRom);
	ppu = &p;
	
	soundmain();
	
	u64 nbcyc = 0;
	auto t0 = Time::now();

	switch (VIDEO_MODE) {
	case NTSC:
		printf("\nVideo: NTSC");
		while (1) {

			while ((SDL_GetQueuedAudioSize(dev) / sizeof(float)) < 1 * 4470) {
				for (int i = 0; i < 59561 * 1; i++) {
					nbcyc++;
					ppu->tick();

					if (masterClockCycles == 3) {

						if (cycles == 0) {

							u8 op = rd(pc++);
							cycles += _6502InstBaseCycles[op];
							comCount[op]++;
							opCodePtr[op]();
						}
						cycles--;

						apuTick();
						masterClockCycles = 0;
					}
					masterClockCycles++;
				}

				if (nbcyc == 5360490) {
					nbcyc = 0;
					auto t1 = Time::now();
					fsec fs = t1 - t0;
					long long millis = std::chrono::duration_cast<std::chrono::milliseconds>(fs).count();
					printf("\n1s elapsed in: %.3fs -- %.2f fps", millis / 1000.0, 60 * 1000.0 / millis);
					t0 = Time::now();
				}
			}
			//SDL_Delay(1); // ALWAYS way too much fps --> sleep instead of busy waiting
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