#include "004_TxROM.h"
#include "../simpleCPU.h"
void M_004_TxROM::wrBankRegisters(bool oddWrite, u8 what) {
	static u8 nextBank = 0;
	if (!oddWrite) {//Even write --> bankSelect register
		prgBankSwappedMode = what & 0x40;
		chrBankSwappedMode = what & 0x80;
		nextBank = what & 0x7;
	}
	else {
		if ((nextBank & 0b110) == 0b110) { // PRG Banks
			prgBankRegisters[nextBank & 1] = prg + (what & (2 * prgRomSize - 1)) * 0x2000;
		}
		else { // CHR Banks
			if (nextBank < 2) { // 2KB CHR bank
				chrBankRegisters[nextBank] = chr + (what & (8 * chrRomSize - 1) & 0xFE) * 0x400;
			}
			else {
				chrBankRegisters[nextBank] = chr + (what & (8 * chrRomSize - 1)) * 0x400;
			}
		}
	}
}

void M_004_TxROM::wrMirrorPrgRamCtrl(bool oddWrite, u8 what) {
	if (!oddWrite) {
		mirror = (what & 1) ? HORIZONTAL : VERTICAL;
	}
}

void M_004_TxROM::wrIRQLatchReload(bool oddWrite, u8 what) {
	if (!oddWrite) {
		irqLatch = what;
	}
	else {
		irqReloadFlag = true;
	}
}


void M_004_TxROM::wrIRQActivate(bool oddWrite) {
	if (!oddWrite) {
		irqEnabled = false;
		//TODO: Acknowledge Pending IRQ
	}
	else {
		irqEnabled = true;
	}
}

void M_004_TxROM::irqCounterTick() {

	if (irqCounter == 0 || irqReloadFlag) {
		irqReloadFlag = false;
		if ((irqCounter == 0 )&& irqEnabled) {
			manualIRQ();
		}
		irqCounter = irqLatch;
	}
	else {
		irqCounter--;
	}
}

void M_004_TxROM::acknowledgeNewScanline(bool risingEdge) {
	if (risingEdge) {
		irqCounterTick();
	}
}

void M_004_TxROM::wrCPU(u16 where, u8 what) {
	bool addrParity = (bool)(where & 0x1);
	switch (where & 0xE000) {
	case 0x6000: prgRam[where - 0x6000] = what; break;
	case 0x8000:wrBankRegisters(addrParity, what); break;
	case 0xA000:wrMirrorPrgRamCtrl(addrParity, what); break;
	case 0xC000:wrIRQLatchReload(addrParity, what); break;
	case 0xE000:wrIRQActivate(addrParity); break;
	default:break;
	}
}

inline u8 M_004_TxROM::lowerPPURegs(u16 where)
{
	bool lowRegID = where & 0x800; // bit 11 tells which half of the quadrant we're in --> R1 else R0
	return chrBankRegisters[lowRegID][where & 0x07FF];
}

inline u8 M_004_TxROM::higherPPURegs(u16 where)
{
	u8 highRegID = (where >> 10) & 0x03;
	return chrBankRegisters[2 + highRegID][where & 0x03FF];
}

u8 M_004_TxROM::rdPPU(u16 where)
{
	if (!prgRomSize) {
		return chrRam[where];
	}
	if (where < 0x1000) {
		if (chrBankSwappedMode) {
			return higherPPURegs(where);
		}
		else {
			return lowerPPURegs(where);
		}
	}
	else {
		if (chrBankSwappedMode) {
			return lowerPPURegs(where);
		}
		else {
			return higherPPURegs(where);
		}
	}
}

void M_004_TxROM::wrPPU(u16 where, u8 what)
{
	if (!prgRomSize) {
		chrRam[where] = what;
	}
}

u8 M_004_TxROM::rdCPU(u16 where) {
	switch (where & 0xE000) {
	case 0x6000: return prgRam[where - 0x6000]; break;
	case 0x8000:
		return prgBankSwappedMode ?
			secondLastBank[where - 0x8000] :
			prgBankRegisters[0][where - 0x8000];
		break;
	case 0xA000: return prgBankRegisters[1][where - 0xA000]; break;
	case 0xC000:
		return prgBankSwappedMode ?
			prgBankRegisters[0][where - 0xC000] :
			secondLastBank[where - 0xC000];
		break;
	case 0xE000: return lastBank[where - 0xE000]; break;
	default:
		return 0;
	}
}

void M_004_TxROM::setPrgRom(u8* prgRom, u8 PRsize) {
	prg = prgRom;
	prgRomSize = PRsize;
	lastBank = (2 * prgRomSize - 1) * 0x2000 + prg;
	secondLastBank = lastBank - 0x2000;
	prgBankRegisters[0] = prg;
	prgBankRegisters[1] = prg;
}

void M_004_TxROM::setChrRom(u8* chrRom, u8 CHRsize) {
	chr = chrRom;
	chrRomSize = CHRsize;
	for (int i = 0; i < 6; i++) {
		chrBankRegisters[i] = chr;
	}
}

void M_004_TxROM::setMirroring(u8 mir) {
	mirror = mir;
}