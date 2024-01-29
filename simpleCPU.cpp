#pragma warning(disable:4996)
#include "simpleCPU.h"
#include "sound.h"
#include "ppu.h"
#include "Rom.h"
//using namespace std;
/*
	_ : indicates a 6502 operation
	XX : opCode
	xxx : operation name
	M / C / _ / I / Z / A / N / R : iMplied, aCcumulator, Single-byte, Immediate, Zeropage, Absolute, iNdirect, Relative
*/
// 6502 registers
u8 ac, xr, yr, sr = SR_RST, sp = 0xFD;
u16 pc;

u8 ram[0x6000] = { 0 };
u32 cycles = 0;

u8 latchCommandCtrl1 = 0;
u8 latchCommandCtrl2 = 0;
u8 keys1 = 0;
u8 keys2 = 0;
u8 keyLatchCtrl1 = 0; // controller 1 buttons
u8 keyLatchCtrl2 = 0; // controller 2 buttons

Mapper* mapper;
PPU* ppu;

u32 comCount[256] = { 0 }; // count how many times each command is executed

u32 getCycles() {
	return cycles;
}
u64 nbcyc = 0;
u64 getTotalCycles() {
	return nbcyc;
}

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
	printf("The I Flag is %s\n", (sr & 0b00000100)  ? "SET" : "CLEAR");
	printf("Remaining audio samples: %d\n", (SDL_GetQueuedAudioSize(dev) / sizeof(float)));
	
}

void pressKey(u16 pressed) {
	keys1 = (u8)pressed;
	keys2 = (u8)(pressed >> 8);
}

//mainly for OAMDMA alignment.
void addCycle() {
	cycles++;
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
	if (at >= 0x6000) {
		return mapper->rdCPU(at);
	}
	else if (at <= 0x1FFF) {
		return ram[at & 0x7FF];
	}
	else if (at >= 0x2000 && at <= 0x3FFF) {
		return rdRegisters(at & 0x2007);
	}
	else if (at == 0x4016) {
		if (latchCommandCtrl1) {
			ram[0x4016] = ram[0x4016] & 0xFE | (keyLatchCtrl1) & 1;
			keyLatchCtrl1 >>= 1;
			keyLatchCtrl1 |= 0x80;
			return 0x40 | ram[0x4016];
		}
		else {
			return keys1 & 1;
		}
	}
	else if (at == 0x4017) {
		return 0;
	}
	else {
		return ram[at];
	}
}

void writeRegisters(u16 where, u8 what) {
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

void wr(u16 where, u8 what) {
	if (where <= 0x1FFF) {
		ram[where & 0x07FF] = what;
	}
	else if (where >= 0x2000 && where <= 0x3FFF) {
		writeRegisters(where & 0x2007, what);
	}
	else if ((where >= 0x6000) && (where <= 0xFFFF)) {
		mapper->wrCPU(where, what);
	}
	else {
		switch (where) {
		case 0x4014:
			ram[0x4014] = what;
			ppu->updateOam();
			cycles += 513 + (cycles & 1);
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
void _push(u8 what) {
	ram[STACK_END | sp--] = what;
}

u8 _pop() {
	return rd(STACK_END | ++sp);
}

u16 absAddr() {
	return (rd(pc + 1) << 8) | rd(pc);
}

//absolute memory access with arg (xr or yr) offset
u16 absArg(u8 arg, u8 cyc = 1) {
	u16 where = (rd(pc + 1) << 8) | rd(pc);
	u16 w2 = where + arg;
	if ((where & 0xFF00) != (w2 & 0xFF00)) {
		cycles += cyc;
	}
	return w2;
}

u8 zpgX() {
	return rd(pc) + xr;
}

u16 indX() {
	return ram[zpgX()] | (ram[(u8)(zpgX() + 1)] << 8);
}

u16 indY(u8 cyc = 1) {
	u16 where = ram[rd(pc)] | (ram[(u8)(rd(pc) + 1)] << 8);
	u16 w2 = where + yr;
	if ((where & 0xFF00) != (w2 & 0xFF00)) {
		cycles += cyc;
	}
	return w2;
}

void check_NZ(u8 obj) {
	sr &= NEG_NZ_FLAGS;
	sr |= (obj & N_FLAG) | ((obj == 0) << 1);
}

void check_CV(u8 what, u16 aluTmp) {
	sr &= NEG_CV_FLAGS;
	sr |= aluTmp & 0xFF00 ? C_FLAG : 0; // C applied if aluTmp > 0xff00
	sr |= V_FLAG & (((ac ^ aluTmp) & (what ^ aluTmp)) >> 1);
}

void _nmi() {
	_push(pc >> 8);
	_push((u8)pc);
	_push(sr);
	sr |= I_FLAG;
	pc = ((rd(0xFFFB) << 8) | rd(0xFFFA));
	cycles += 7;
}

void _00brk_() {
	_push((u8)((pc + 1) >> 8));
	_push((u8)(pc + 1));
	_push(sr | B_FLAG);
	sr |= I_FLAG;
	pc = rd(0xFFFE) | (rd(0xFFFF) << 8);
	cycles += 7;
}

void irq() {
	_push((u8)(pc >> 8));
	_push((u8)pc);
	_push(sr | B_FLAG);
	sr |= I_FLAG;
	pc = rd(0xFFFE) | (rd(0xFFFF) << 8);
	cycles += 7;
}

void manualIRQ() {
	if (!(sr & I_FLAG)) irq();
}

void _rst() {
	sp -= 3;
	sr |= I_FLAG;
	pc = rd(0xFFFD) << 8 | rd(0xFFFC);
	ram[0x4015] = 0;
	//updateAPU(0x4015);
}

void _adc(u8 what) {
	u16 aluTmp = ac + what + (sr & C_FLAG);
	check_CV(what, aluTmp);
	ac = (u8)aluTmp;
	check_NZ(ac);
}
void _69adcI() { _adc(rd(pc)); pc++; }
void _65adcZ() { _adc(ram[rd(pc)]); pc++; }
void _75adcZ() { _adc(rd(zpgX())); pc++; }
void _6DadcA() { _adc(rd(absAddr())); pc += 2; }
void _7DadcA() { _adc(rd(absArg(xr))); pc += 2; }
void _79adcA() { _adc(rd(absArg(yr))); pc += 2; }
void _61adcN() { _adc(rd(indX())); pc++; }
void _71adcN() { _adc(rd(indY())); pc++; }

void _and(u8 what) {
	ac &= what;
	check_NZ(ac);
}
void _29andI() { _and(rd(pc)); pc++; }
void _25andZ() { _and(ram[rd(pc)]); pc++; }
void _35andZ() { _and(rd(zpgX())); pc++; }
void _2DandA() { _and(rd(absAddr())); pc += 2; }
void _3DandA() { _and(rd(absArg(xr))); pc += 2; }
void _39andA() { _and(rd(absArg(yr))); pc += 2; }
void _21andN() { _and(rd(indX())); pc++; }
void _31andN() { _and(rd(indY())); pc++; }

void _24bitZ() {
	u8 termB = ram[rd(pc)];
	sr = sr & ~(N_FLAG | V_FLAG) | termB & (N_FLAG | V_FLAG);
	sr = sr & ~Z_FLAG | (((ac & termB) == 0) ? Z_FLAG : 0);
	pc++;
}

void _2CbitA() {
	u8 termB = rd(absAddr());
	sr = sr & ~(N_FLAG | V_FLAG) | termB & (N_FLAG | V_FLAG);
	sr = sr & ~Z_FLAG | (((ac & termB) == 0) ? Z_FLAG : 0);
	pc += 2;
}

void _branch(bool cond) {
	if (cond) {
		int8_t relTmp = rd(pc);
		if (((pc + 1) & 0xFF00) != ((pc + relTmp + 1) & 0xFF00)) {
			cycles += 1;
		}
		cycles += 1;
		pc += relTmp;
	}
	pc++;
}

void _90bccR() { _branch((sr & C_FLAG) == 0); }
void _B0bcsR() { _branch(sr & C_FLAG); }
void _F0beqR() { _branch(sr & Z_FLAG); }
void _30bmiR() { _branch(sr & N_FLAG); }
void _D0bneR() { _branch((sr & Z_FLAG) == 0); }
void _10bplR() { _branch((sr & N_FLAG) == 0); }
void _50bvcR() { _branch((sr & V_FLAG) == 0); }
void _70bvsR() { _branch(sr & V_FLAG); }

void _18clcM() { sr &= ~C_FLAG; }
void _D8cldM() { sr &= ~D_FLAG; }
void _58cliM() { sr &= ~I_FLAG; }
void _B8clvM() { sr &= ~V_FLAG; }

void _cmp(u8 reg, u8 what) {
	u8 tmpN = reg - what;
	sr = sr & ~N_FLAG & ~Z_FLAG & ~C_FLAG |
		((reg == what) ? Z_FLAG : 0) |
		((reg >= what) ? C_FLAG : 0) |
		tmpN & N_FLAG;
}
void _C9cmpI() { _cmp(ac, rd(pc)); pc++; }
void _C5cmpZ() { _cmp(ac, ram[rd(pc)]); pc++; }
void _D5cmpZ() { _cmp(ac, ram[zpgX()]); pc++; }
void _CDcmpA() { _cmp(ac, rd(absAddr())); pc += 2; }
void _DDcmpA() { _cmp(ac, rd(absArg(xr))); pc += 2; }
void _D9cmpA() { _cmp(ac, rd(absArg(yr))); pc += 2; }
void _C1cmpN() { _cmp(ac, rd(indX())); pc++; }
void _D1cmpN() { _cmp(ac, rd(indY())); pc++; }

void _E0cpxI() { _cmp(xr, rd(pc)); pc++; }
void _E4cpxZ() { _cmp(xr, ram[rd(pc)]); pc++; }
void _ECcpxA() { _cmp(xr, rd(absAddr())); pc += 2; }

void _C0cpyI() { _cmp(yr, rd(pc)); pc++; }
void _C4cpyZ() { _cmp(yr, ram[rd(pc)]); pc++; }
void _CCcpyA() { _cmp(yr, rd(absAddr())); pc += 2; }

void _C6decZ() {
	check_NZ(--ram[rd(pc)]);
	pc++;
}

void _D6decZ() {
	check_NZ(--ram[zpgX()]);
	pc++;
}

void _CEdecA() {
	u16 tmp = (rd(pc + 1) << 8) | rd(pc);
	u8 wrTmp = rd(tmp) - 1;
	wr(tmp, wrTmp);
	check_NZ(wrTmp);
	pc += 2;
}

void _DEdecA() {
	u16 tmp = absArg(xr, 0);
	u8 wrTmp = rd(tmp) - 1;
	wr(tmp, wrTmp);
	check_NZ(wrTmp);
	pc += 2;
}

void _CAdexM() { check_NZ(--xr); }
void _88deyM() { check_NZ(--yr); }

void _eor(u8 what) {
	ac ^= what;
	check_NZ(ac);
}
void _49eorI() { _eor(rd(pc)); pc++; }
void _45eorZ() { _eor(ram[rd(pc)]); pc++; }
void _55eorZ() { _eor(rd(zpgX())); pc++; }
void _4DeorA() { _eor(rd(absAddr())); pc += 2; }
void _5DeorA() { _eor(rd(absArg(xr))); pc += 2; }
void _59eorA() { _eor(rd(absArg(yr))); pc += 2; }
void _41eorN() { _eor(rd(indX())); pc++; }
void _51eorN() { _eor(rd(indY())); pc++; }

void _E6incZ() {
	check_NZ(++ram[rd(pc)]);
	pc++;
}

void _F6incZ() {
	check_NZ(++ram[zpgX()]);
	pc++;
}

void _EEincA() {
	//printf("EE\n");
	u16 tmp = (rd(pc + 1) << 8) | rd(pc);
	u8 writtenTmp = rd(tmp);
	wr(tmp, writtenTmp);
	wr(tmp, writtenTmp+1);
	check_NZ(writtenTmp+1);
	pc += 2;
}

void _FEincA() {
	//printf("FE\n");
	u16 tmp = absArg(xr, 0);
	u8 writtenTmp = rd(tmp);
	wr(tmp, writtenTmp);
	wr(tmp, writtenTmp+1);
	check_NZ(writtenTmp+1);
	pc += 2;
}

void _E8inxM() { check_NZ(++xr); }
void _C8inyM() { check_NZ(++yr); }
void _4CjmpA() {
	pc = rd(pc) | (rd(pc + 1) << 8); 
}

void _6CjmpN() {
	u16 pcTmp = rd(pc) | (rd(pc + 1) << 8);
	pc = rd(pcTmp) | (rd((pcTmp & 0xFF00) | (pcTmp + 1) & 0x00FF) << 8); // Here comes the (in)famous Indirect Jump bug implementation...
}

void _20jsrA() {
	u16 pcTmp = pc + 1;
	_push((u8)(pcTmp >> 8));
	_push((u8)pcTmp);
	pc = rd(pc) | (rd(pc + 1) << 8);
}

void _lda(u8 what) {
	ac = what;
	check_NZ(ac);
}
void _A9ldaI() { _lda(rd(pc)); pc++; }
void _A5ldaZ() { _lda(ram[rd(pc)]); pc++; }
void _B5ldaZ() { _lda(ram[zpgX()]); pc++; }
void _ADldaA() { _lda(rd(absAddr())); pc += 2; }
void _BDldaA() { _lda(rd(absArg(xr))); pc += 2; }
void _B9ldaA() { _lda(rd(absArg(yr))); pc += 2; }
void _A1ldaN() { _lda(rd(indX())); pc++; }
void _B1ldaN() { _lda(rd(indY())); pc++; }

void _ldx(u8 what) {
	xr = what;
	check_NZ(xr);
}
void _A2ldxI() { _ldx(rd(pc)); pc++; }
void _A6ldxZ() { _ldx(ram[rd(pc)]); pc++; }
void _B6ldxZ() { _ldx(ram[(u8)(rd(pc) + yr)]); pc++; }
void _AEldxA() { _ldx(rd(absAddr())); pc += 2; }
void _BEldxA() { _ldx(rd(absArg(yr))); pc += 2; }

void _ldy(u8 what) {
	yr = what;
	check_NZ(yr);
}
void _A0ldyI() { _ldy(rd(pc)); pc++; }
void _A4ldyZ() { _ldy(ram[rd(pc)]); pc++; }
void _B4ldyZ() { _ldy(ram[zpgX()]); pc++; }
void _ACldyA() { _ldy(rd(absAddr())); pc += 2; }
void _BCldyA() { _ldy(rd(absArg(xr))); pc += 2; }

void _lsr(u16 where) {
	u8 tmp = rd(where);
	sr = sr & ~N_FLAG & ~C_FLAG | tmp & C_FLAG;
	wr(where, tmp >> 1);
	sr &= NEG_NZ_FLAGS;
	sr |= (((tmp>>1) == 0) << 1);
}

void _4AlsrC() {
	sr = sr & ~N_FLAG & ~C_FLAG | ac & C_FLAG;
	ac >>= 1;
	sr &= NEG_NZ_FLAGS;
	sr |= ((ac == 0) << 1);
}
void _46lsrZ() { _lsr(rd(pc)); pc++; }
void _56lsrZ() { _lsr(zpgX()); pc++; }
void _4ElsrA() { _lsr(absAddr()); pc += 2; }
void _5ElsrA() { _lsr(absArg(xr, 0)); pc += 2; }

void _ora(u8 what) {
	ac |= what;
	check_NZ(ac);
}
void _01oraN() { _ora(rd(indX())); pc++; }
void _05oraZ() { _ora(ram[rd(pc)]); pc++; }
void _09oraI() { _ora(rd(pc)); pc++; }
void _0DoraA() { _ora(rd(absAddr())); pc += 2; }
void _11oraN() { _ora(rd(indY())); pc++; }
void _15oraZ() { _ora(ram[zpgX()]); pc++; }
void _19oraA() { _ora(rd(absArg(yr))); pc += 2; }
void _1DoraA() { _ora(rd(absArg(xr))); pc += 2; }

void _48phaM() {
	_push(ac);
}

void _08phpM() {
	_push(sr | B_FLAG);
}

void _68plaM() {
	ac = ram[STACK_END | ++sp];
	check_NZ(ac);
}

void _28plpM() { //ignores flags (B and _)
	sr = sr & (B_FLAG | __FLAG) | ram[STACK_END | ++sp] & ~(B_FLAG | __FLAG);
}

void _asl(u16 where) {// one bit left shift, shifted out bit is preserved in carry
	u8 data = rd(where);
	sr = sr & ~C_FLAG | ((data & N_FLAG) ? C_FLAG : 0);
	wr(where, data << 1);
	check_NZ(data << 1);
}
void _0Aasl_() {
	sr = sr & ~C_FLAG | ((ac & N_FLAG) ? C_FLAG : 0);
	ac <<= 1;
	check_NZ(ac);
}
void _06aslZ() { _asl(rd(pc)); pc++; }
void _16aslZ() { _asl(zpgX()); pc++; }
void _0EaslA() { _asl(absAddr()); pc += 2; }
void _1EaslA() { _asl((absAddr()) + xr); pc += 2; }

void _rol(u16 where) {
	u8 data = rd(where);
	bool futureC = data & N_FLAG;
	u8 what = (data << 1) | sr & C_FLAG;
	wr(where, what);
	sr = sr & ~C_FLAG | (u8)futureC;
	check_NZ(what);
}

void _2ArolC() {
	bool futureC = (ac & N_FLAG) > 0;
	ac = (ac << 1) | sr & C_FLAG;
	sr = sr & ~C_FLAG | (u8)futureC; // N_FLAG is bit 7, carry will be set to it
	check_NZ(ac);
}

void _26rolZ() { _rol(rd(pc)); pc++; }
void _36rolZ() { _rol(zpgX()); pc++; }
void _2ErolA() { _rol(absAddr()); pc += 2; }
void _3ErolA() { _rol((absAddr()) + xr); pc += 2; }

void _ror(u16 where) {
	u8 data = rd(where);
	u8 futureC = data & 1;
	u8 tmp = (data >> 1) | ((sr & C_FLAG) ? N_FLAG : 0);
	wr(where, tmp);
	sr = sr & ~C_FLAG | futureC;
	check_NZ(tmp);
}
void _6ArorC() {
	bool futureC = ac & 1;
	ac = (ac >> 1) | (sr & C_FLAG) << 7;
	sr = sr & ~C_FLAG | (u8)futureC;
	check_NZ(ac);
}
void _66rorZ() { _ror(rd(pc)); pc++; }
void _76rorZ() { _ror(zpgX()); pc++; }
void _6ErorA() { _ror(absAddr()); pc += 2; }
void _7ErorA() { _ror((absAddr()) + xr); pc += 2; }

void _40rtiM() {
	sr = sr & (B_FLAG | __FLAG) | ram[STACK_END | ++sp] & ~(B_FLAG | __FLAG);
	pc = ram[STACK_END | (u8)(sp + 1)] | (ram[STACK_END | (u8)(sp + 2)] << 8);
	sp += 2;
}

void _60rtsM() {
	pc = (ram[STACK_END | (u8)(sp + 1)] | (ram[STACK_END | (u8)(sp + 2)] << 8)) + 1;
	sp += 2;
}

void _E9sbcI() { _adc(~rd(pc)); pc++; }
void _E5sbcZ() { _adc(~ram[rd(pc)]); pc++; }
void _F5sbcZ() { _adc(~ram[zpgX()]); pc++; }
void _EDsbcA() { _adc(~rd(absAddr())); pc += 2; }
void _FDsbcA() { _adc(~rd(absArg(xr))); pc += 2; }
void _F9sbcA() { _adc(~rd(absArg(yr))); pc += 2; }
void _E1sbcN() { _adc(~rd(indX())); pc++; }
void _F1sbcN() { _adc(~rd(indY())); pc++; }

void _38secM() { sr |= C_FLAG; }
void _F8sedM() { sr |= D_FLAG; }
void _78seiM() { sr |= I_FLAG; }

void _85staZ() {
	ram[rd(pc)] = ac;
	pc++;
}

void _95staZ() {
	ram[zpgX()] = ac;
	pc++;
}

void _8DstaA() {
	u16 tmpPC = (rd(pc + 1) << 8) | rd(pc);
	pc += 2;
	wr(tmpPC, ac);
}

void _9DstaA() {
	u16 tmpPC = ((absAddr()) + xr);
	pc += 2;
	wr(tmpPC, ac);
}

void _99staA() {
	u16 tmpPC = ((absAddr()) + yr);
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
	ram[zpgX()] = yr;
	pc++;
}

void _8CstyA() {
	u16 tmpPC = (rd(pc + 1) << 8) | rd(pc);
	pc += 2;
	wr(tmpPC, yr);
}

void _AAtaxM() { xr = ac; check_NZ(xr); }
void _A8tayM() { yr = ac; check_NZ(yr); }
void _BAtsxM() { xr = sp; check_NZ(xr); }
void _8AtxaM() { ac = xr; check_NZ(ac); }
void _9AtxsM() { sp = xr; }
void _98tyaM() { ac = yr; check_NZ(ac); }

void _02jamM() { return; }

/**** ILLEGAL / UNOFFICIAL OPCODES ****/

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

void _BFlaxA() { _B9ldaA(); _AAtaxM(); }
void _A3laxN() { _A1ldaN(); _AAtaxM(); }
void _B3laxN() { _B1ldaN(); _AAtaxM(); }
void _6BarrM() { pc++; }

void _dcp(u16 where) {
	u8 m = rd(where) - 1;
	wr(where, m);
	_cmp(ac, m);
}
void _C7dcpZ() { _dcp(rd(pc)); pc++; }
void _D7dcpZ() { _dcp(zpgX()); pc++; }
void _CFdcpA() { _dcp(absAddr()); pc += 2; }
void _DFdcpA() { _dcp(absArg(xr)); pc += 2; }
void _DBdcpA() { _dcp(absArg(yr)); pc += 2; }
void _C3dcpN() { _dcp(indX()); pc++; }
void _D3dcpN() { _dcp(indY()); pc++; }

void _isc(u16 where) {
	u8 termB = rd(where) + 1;
	wr(where, termB);
	_adc(~termB);
}
void _E7iscZ() { _isc(rd(pc)); pc++; }
void _F7iscZ() { _isc(zpgX()); pc++; }
void _EFiscA() { _isc(absAddr()); pc += 2; }
void _FFiscA() { _isc(absArg(xr)); pc += 2; }
void _FBiscA() { _isc(absArg(yr)); pc += 2; }
void _E3iscN() { _isc(indX()); pc++; }
void _F3iscN() { _isc(indY()); pc++; }

void _ABlxaI() {
	ac = xr = ac & rd(pc);
	check_NZ(ac);
	pc++;
}

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
	pc -= 2;
	_2DandA();
}

void _3FrlaA() {
	_3ErolA();
	pc -= 2;
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

void _rra(u16 where) {
	_ror(where);
	_adc(rd(where));
}
void _67rraZ() { _rra(rd(pc)); pc++; }
void _77rraZ() { _rra(zpgX()); pc++; }
void _6FrraA() { _rra(absAddr()); pc += 2; }
void _7FrraA() { _rra(absArg(xr)); pc += 2; }
void _7BrraA() { _rra(absArg(yr)); pc += 2; }
void _63rraN() { _rra(indX()); pc += 1; }
void _73rraN() { _rra(indY()); pc += 1; }

void _sax(u16 where) { wr(where, ac & xr); }
void _87saxZ() { _sax(rd(pc)); pc++; }
void _97saxZ() { _sax((u8)(rd(pc) + yr)); pc++; }
void _8FsaxA() { _sax((absAddr())); pc += 2; }
void _83saxN() { _sax(indX()); pc++; }

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

void _slo(u16 where) {
	_asl(where);
	ac |= rd(where);
	check_NZ(ac);
}
void _07sloZ() { _slo(rd(pc)); pc++; }
void _17sloZ() { _slo(zpgX()); pc++; }
void _0FsloA() { _slo(absAddr()); pc += 2; }
void _1FsloA() { _slo(absArg(xr)); pc += 2; }
void _1BsloA() { _slo(absArg(yr)); pc += 2; }
void _03sloN() { _slo(indX()); pc += 1; }
void _13sloN() { _slo(indY()); pc += 1; }

void _sre(u16 where) {
	_lsr(where);
	ac ^= rd(where);
	check_NZ(ac);
}
void _47sre() { _sre(rd(pc)); pc++; }
void _57sre() { _sre(zpgX()); pc++; }
void _4Fsre() { _sre(absAddr()); pc += 2; }
void _5Fsre() { _sre(absArg(xr)); pc += 2; }
void _5Bsre() { _sre(absArg(yr)); pc += 2; }
void _43sre() { _sre(indX()); pc += 1; }
void _53sre() { _sre(indY()); pc += 1; }

void _EAnopM() {}
void _04nopZ() { pc++; }
void _0CnopA() { pc += 2; }
void _1CnopA() { absArg(xr); pc += 2; }

void _8BaneM() {
	pc++;
}

void _93shaN() {
	pc++;
}

void _9BtasA() {
	pc += 2;
}

void _BBlasA() {
	pc += 2;
}

void (*opCodePtr[])() = { _00brk_, _01oraN, _02jamM, _03sloN, _04nopZ, _05oraZ, _06aslZ, _07sloZ,
						_08phpM, _09oraI, _0Aasl_, _0BancI, _0CnopA, _0DoraA, _0EaslA, _0FsloA,
						_10bplR, _11oraN, _02jamM, _13sloN, _04nopZ, _15oraZ, _16aslZ, _17sloZ,
						_18clcM, _19oraA, _EAnopM, _1BsloA, _1CnopA, _1DoraA, _1EaslA, _1FsloA,
						_20jsrA, _21andN, _02jamM, _23rlaN, _24bitZ, _25andZ, _26rolZ, _27rlaZ,
						_28plpM, _29andI, _2ArolC, _0BancI, _2CbitA, _2DandA, _2ErolA, _2FrlaA,
						_30bmiR, _31andN, _02jamM, _33rlaN, _04nopZ, _35andZ, _36rolZ, _37rlaZ,
						_38secM, _39andA, _EAnopM, _3BrlaA, _1CnopA, _3DandA, _3ErolA, _3FrlaA,
						_40rtiM, _41eorN, _02jamM, _43sre, _04nopZ, _45eorZ, _46lsrZ,
						_47sre, _48phaM, _49eorI, _4AlsrC, _4BalrI, _4CjmpA, _4DeorA, _4ElsrA,
						_4Fsre, _50bvcR, _51eorN, _02jamM, _53sre, _04nopZ, _55eorZ, _56lsrZ,
						_57sre, _58cliM, _59eorA, _EAnopM, _5Bsre, _1CnopA, _5DeorA, _5ElsrA,
						_5Fsre, _60rtsM, _61adcN, _02jamM, _63rraN, _04nopZ, _65adcZ, _66rorZ,
						_67rraZ, _68plaM, _69adcI, _6ArorC, _6BarrM, _6CjmpN, _6DadcA, _6ErorA,
						_6FrraA, _70bvsR, _71adcN, _02jamM, _73rraN, _04nopZ, _75adcZ, _76rorZ,
						_77rraZ, _78seiM, _79adcA, _EAnopM, _7BrraA, _1CnopA, _7DadcA, _7ErorA,
						_7FrraA, _04nopZ, _81staZ, _04nopZ, _83saxN, _84styZ, _85staZ, _86stxZ,
						_87saxZ, _88deyM, _04nopZ, _8AtxaM, _8BaneM, _8CstyA, _8DstaA, _8EstxA,
						_8FsaxA, _90bccR, _91staZ, _02jamM, _93shaN, _94styZ, _95staZ, _96stxZ,
						_97saxZ, _98tyaM, _99staA, _9AtxsM, _9BtasA, _9CshyA, _9DstaA, _9EshxA,
						_9FshaA, _A0ldyI, _A1ldaN, _A2ldxI, _A3laxN, _A4ldyZ, _A5ldaZ, _A6ldxZ,
						_A7laxZ, _A8tayM, _A9ldaI, _AAtaxM, _ABlxaI, _ACldyA, _ADldaA, _AEldxA,
						_AFlaxA, _B0bcsR, _B1ldaN, _02jamM, _B3laxN, _B4ldyZ, _B5ldaZ, _B6ldxZ,
						_B7laxZ, _B8clvM, _B9ldaA, _BAtsxM, _BBlasA, _BCldyA, _BDldaA, _BEldxA,
						_BFlaxA, _C0cpyI, _C1cmpN, _04nopZ, _C3dcpN, _C4cpyZ, _C5cmpZ, _C6decZ,
						_C7dcpZ, _C8inyM, _C9cmpI, _CAdexM, _CBsbxI, _CCcpyA, _CDcmpA, _CEdecA,
						_CFdcpA, _D0bneR, _D1cmpN, _02jamM, _D3dcpN, _04nopZ, _D5cmpZ, _D6decZ,
						_D7dcpZ, _D8cldM, _D9cmpA, _EAnopM, _DBdcpA, _1CnopA, _DDcmpA, _DEdecA,
						_DFdcpA, _E0cpxI, _E1sbcN, _04nopZ, _E3iscN, _E4cpxZ, _E5sbcZ, _E6incZ,
						_E7iscZ, _E8inxM, _E9sbcI, _EAnopM, _E9sbcI, _ECcpxA, _EDsbcA, _EEincA,
						_EFiscA, _F0beqR, _F1sbcN, _02jamM, _F3iscN, _04nopZ, _F5sbcZ, _F6incZ,
						_F7iscZ, _F8sedM, _F9sbcA, _EAnopM, _FBiscA, _1CnopA, _FDsbcA, _FEincA, _FFiscA
};

Cpu::Cpu() {
	pc = (rd(0xFFFD) << 8) | rd(0xFFFC);
}

void Cpu::afficher() {
	printf("pc: %04X ac: %02x xr: %02x yr: %02x sp: %02x sr: %02x PPU: %3d SL: %3d \n",
		pc, ac, xr, yr, sp, sr, (3 * cycles) % 341, -1 + (((((3 * cycles)) / 341)) % 262));
	for (int i = 0; i < 8; i++) {
		printf("%d", (sr >> (7 - i)) & 1);
	}
}

Rom* rom;
float silence[5000];
int mainSYS(Screen* scr, const char* filePath) {
	rom = new Rom(filePath);
	ppu = new PPU(scr->getPixels(), scr, rom);
	rom->printInfo();
	mapper = rom->getMapper();
	scr->setRom(rom);
	soundmain();
	Cpu* f = new Cpu();
	auto t0 = Time::now();
	
	switch (VIDEO_MODE) {
	case NTSC:
		printf("\nVideo: NTSC");
		while (1) {
			while (1||(SDL_GetQueuedAudioSize(dev) / sizeof(float)) < 1 * 4470) {
				if ((SDL_GetQueuedAudioSize(dev) / sizeof(float)) < 1200) {
					printf("\nAUDIO DEVICE IS STARVING!\n");
					
					SDL_QueueAudio(dev, silence, 1000 * sizeof(float));
				}
				for (int i = 0; i < 59561 * 1; i++) {
					
					nbcyc++;

					ppu->tick(); ppu->tick(); ppu->tick();
					if (cycles == 0) {
						u8 op = rd(pc++);
						cycles += _6502InstBaseCycles[op];
						comCount[op]++;
						opCodePtr[op]();
					}
					cycles--;
					apuTick();
				}

				if (nbcyc == 1786830) {
					nbcyc = 0;
					auto t1 = Time::now();
					fsec fs = t1 - t0;
					long long millis = std::chrono::duration_cast<std::chrono::milliseconds>(fs).count();
					printf("\n1s elapsed in: %.3fs -- %.2f fps", millis / 1000.0, 60 * 1000.0 / millis);
					t0 = Time::now();
				}
			}
		}
		break;

	case PAL:
		printf("\nVideo: PAL");
		while (1) {
			
		}
		break;
	default:
		printf("rom ERROR!");
		exit(1);
	}
}