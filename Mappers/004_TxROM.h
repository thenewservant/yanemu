#ifndef _004_TXROM_H_
#define _004_TXROM_H_
#include "Mapper.h"

class M_004_TxROM : public Mapper {
protected:
	u8* chrBankRegisters[6];
	u8* prgBankRegisters[2];
	bool wRamEnable;
	bool prgBankSwappedMode;// bit 6 of the bank select register tells if R6 bank is swapped with the 2nd last bank.
	bool chrBankSwappedMode;
	u8* chr;
	u8* secondLastBank;
	u8* lastBank;
	u8 prgRam[0x2000];
	u8 chrRam[0x2000];

	//IRQ Specifics:

	u8 irqCounter;
	u8 irqReloadFlag;
	u8 irqLatch;
	bool irqEnabled;
	bool is8x16SpriteMode;
protected:
	u8 lowerPPURegs(u16 where);
	u8 higherPPURegs(u16 where);
	void wrBankRegisters(bool parity, u8 what);
	void wrMirrorPrgRamCtrl(bool oddWrite, u8 what);
	void wrIRQLatchReload(bool oddWrite, u8 what);
	//IRQ disable/enable
	void wrIRQActivate(bool oddWrite);
	void irqCounterTick();
	void acknowledgeNewScanline(bool risingEdge);
	void setMirroring(u8 mir);
public:
	//void wrNT(u16 where, u8 what);
	//u8 rdNT(u16 where); // 4-screen mirroring will be implemented later.
	void setPrgRom(u8* prgRom, u8 PRsize);
	void setChrRom(u8* chrRom, u8 CHRsize);
	u8 rdCPU(u16 where);
	void wrCPU(u16 where, u8 what);
	u8 rdPPU(u16 where);
	void wrPPU(u16 where, u8 what);
	u8* getPrgRam() { return prgRam; };
	void setPrgRam(u8* ramPtr) {
		memcpy(prgRam, ramPtr, 0x2000);
	};
	void setSpriteMode(u8 mode) { is8x16SpriteMode = (mode == 16); };
	M_004_TxROM() :Mapper(),
		wRamEnable(false), prgBankSwappedMode(false),
		chrBankSwappedMode(false), prgRam{ 0 }, chrBankRegisters{ 0 },
		prgBankRegisters{ 0 }, chr{ 0 }, secondLastBank{ 0 }, lastBank{ 0 }, is8x16SpriteMode{ false } {};
};
#endif