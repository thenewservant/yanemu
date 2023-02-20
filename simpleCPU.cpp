#include "simpleCPU.hpp"

 uint16_t pc;
 uint8_t ac=0, xr, yr, sr = SR_RST, sp = 0xFF;
#define S 0x00
#define STACK_END 0x100
bool jammed = false; // is the cpu jammed because of certain instructions?

//uint8_t prog[] = {0xA2, 0x05, 0xB4, 0x10, 0x00};
/*uint8_t prog[] = {0x69, 0x80, 0x69, 0x01,
                    0x2A,  0x2A, 
                    0x6A,  0x6A,
                    0xA2, 0xFF,
                    0x86, 0x58, 
                    0xA2, 0x00,
                    0xA4, 0x58,
                    0x02};*/

//

//test branch
//uint8_t prog[] = {0xA2, 0xFA, 0xCA, 0xC8, 0xE0, 0x41, 0xD0, 0xFA, 0x00};
//uint8_t prog[] = { 0x69, 0x10, 0x69, 0x10, 0x20, 0x07, 0x00, 0x69, 0x10, 0x60, 0x00 };
//uint8_t prog[] = {0x0C, 0xFF, 0xFF,0x0C, 0xFF, 0xFF,0x0C, 0xFF, 0xFF,0x00};
//uint8_t prog[] = { 0x6C, 0x08, 0x00, 0x00, 0x00, 0x69, 0x10, 0x00, 0x05, 0x00 };
//uint8_t prog[] = {  0xA2,0x10 , 0x8E ,0x00,0x02 };
extern uint8_t *ram = (uint8_t *) calloc((1 << 16) , sizeof(uint8_t));

uint8_t* prog = ram;

#define ORG 0x0000 // start position of PRG-ROM in map
uint16_t prog2[] = { 
    0x69, 0x87, 0x69, 0x48, 0x69, 0x00, 0x65, 0x03, 0x65, 0x02, 0x65, 0x00, 0x69, 0x29, 0x65, 0x27, 0x65, 0x28, 0x69, 0x01, 0x10, 0xFC
    , 0xDEAD};



//registers_t reg = (registers_t) calloc(1,sizeof(registers_));

uint16_t bigEndianTmp; // uint16_t used to process absolute adresses

using namespace std;

/*
    _ : indicates a 6502 operation
    XX : opCode
    xxx : operation name
    M / C / _ / I / Z / A / N / R : iMplied, aCcumulator, Single-byte, Immediate, Zeropage, Absolute, iNdirect, Relative
    
*/
uint16_t addTmp = 0; // used to store the result of an addition

inline void check_NZ() {
	sr = (sr & ~N_FLAG) | (ac | xr | yr) & N_FLAG;
	sr = (sr & ~Z_FLAG) | (ac & xr & yr & Z_FLAG);
}

inline void check_CV() {
    sr = (addTmp) > 0xff ? (sr | C_FLAG) : sr & ~C_FLAG;
    sr = (sr & ~V_FLAG) | V_FLAG&((N_FLAG & ~(ac ^ prog[pc]) & (ac ^ addTmp)) >> 1);
}

void _69adcI(){
    addTmp = ac + prog[pc] + (sr & C_FLAG);
    check_CV();
    ac = addTmp; // relies on auto uint8_t conversion.
    pc++;
    check_NZ();
}

void _65adcZ() {
	addTmp = ac + ram[prog[pc]] + (sr & C_FLAG);
	//sr = (ac + ram[prog[pc]] + (sr & C_FLAG)) > 0xff ? sr | C_FLAG : sr & ~C_FLAG;
    check_CV();
    ac = addTmp;
	pc++;
	check_NZ();
}

void _75adcZ(){
    ac += ram[prog[pc] + xr] + (sr & C_FLAG);
    sr = (ac + ram[prog[pc] + xr] + (sr & C_FLAG)) > 0xff ? sr | C_FLAG : sr & ~C_FLAG;
    pc++;
    check_NZ();
}

void _00brk_(){
    jammed = true;
    NOP;
    printf("BRK");
}

void _29andI(){
    check_NZ();
    ac &= prog[pc];
    pc++;
}

void _25andZ(){
    check_NZ();
    ac &= ram[prog[pc]];
    pc++;
}

void _35andZ(){
    check_NZ();
    ac &= ram[prog[pc] + xr];
    pc++;
}

void _2DandA(){
    check_NZ();
    ac &= ram[(prog[pc+1] << 8) | prog[pc]];
    pc+=2;
}

void _3DandA(){
    check_NZ();
    ac &= ram[((prog[pc+1] << 8) | prog[pc]) + xr ];
    pc+=2;
}

void _39andA(){
    check_NZ();
    ac &= ram[((prog[pc+1] << 8) | prog[pc]) + yr ];
    pc+=2;
}

void _21andN(){
    check_NZ();
    ac &= ram[ram[prog[pc]+xr]];
    pc+=2;
}

void _31andN(){
    check_NZ();
    ac &= ram[ram[prog[pc]] + yr];
    pc+=2;
}

void _30bmiR() {
    pc += (((sr & N_FLAG))>0) * (((prog[pc] >> 7) == 1) ? -1 * ((uint8_t)(~prog[pc]) + 1) : prog[pc]);          //prog[pc]
    pc++;
}
void _D0bneR(){
    pc += (!((sr & Z_FLAG) >> 1)) * (((prog[pc] >> 7) == 1) ? -1 * ((uint8_t)(~prog[pc]) + 1) : prog[pc]);          //prog[pc]
    pc++;
}

void _10bplR() {
    pc += (!((sr & N_FLAG) >> 1)) * (((prog[pc] >> 7) == 1) ? -1 * ((uint8_t)(~prog[pc]) + 1) : prog[pc]);          //prog[pc]
    pc++;
}


void _50bvcR(){

    pc += (!(sr & V_FLAG)) * ((prog[pc] >> 7 == 1) ? (~prog[pc]+1 ) :  prog[pc]);          //prog[pc]
    pc++;
}

void _70bvsR() {
    pc += (!(sr^V_FLAG&V_FLAG)) * ((prog[pc] >> 7 == 1) ? (~prog[pc]+1 ) :  prog[pc]);          //prog[pc]
    pc++;
}

void _18clcM(){
    sr &= ~C_FLAG;
}

void _D8cldM(){
    sr &= ~D_FLAG;
}

void _58cliM(){
    sr &= ~I_FLAG;
}

void _B8clvM(){
    sr &= ~V_FLAG;
}

/*


CMP

    Compare Memory with Accumulator

    A - M
    N	Z	C	I	D	V
    +	+	+	-	-	-
    addressing	assembler	opc	bytes	cycles
    immediate	CMP #oper	C9	2	2  
    zeropage	CMP oper	C5	2	3  
    zeropage,X	CMP oper,X	D5	2	4  
    absolute	CMP oper	CD	3	4  
    absolute,X	CMP oper,X	DD	3	4* 
    absolute,Y	CMP oper,Y	D9	3	4* 
    (indirect,X)	CMP (oper,X)	C1	2	6  
    (indirect),Y	CMP (oper),Y	D1	2	5* 
    */
uint8_t tmpN = 0; // used to store the result of a subtraction on 8 bits (to further deduce the N flag)

void _C9cmpI(){
    check_NZ();
    tmpN=ac-prog[pc];
    sr = SR_RST + Z_FLAG * (ac == prog[pc]) + C_FLAG * (ac >= prog[pc]) + N_FLAG * (tmpN >> 7 == 1);
    pc++;
}

void _C5cmpZ(){
    check_NZ();
    tmpN=ac-prog[pc];
    sr = SR_RST + Z_FLAG * (ac == ram[prog[pc]]) + C_FLAG * (ac >= ram[prog[pc]]) + N_FLAG * (tmpN >> 7 == 1);
    pc++;
}

void _D5cmpZ(){
    check_NZ();
    tmpN=ac-prog[pc];
    sr = SR_RST + Z_FLAG * (ac == ram[prog[pc] + xr]) + C_FLAG * (ac >= ram[prog[pc] + xr]) + N_FLAG * (tmpN >> 7 == 1);
    pc++;
}

void _CDcmpA(){
    check_NZ();
    tmpN=ac-prog[pc];
    bigEndianTmp = (prog[pc+1] << 8) | prog[pc];
    sr = SR_RST + Z_FLAG * (ac == ram[bigEndianTmp]) + C_FLAG * (ac >= ram[bigEndianTmp]) + N_FLAG * (tmpN >> 7 == 1);
    pc+=2;
}

void _DDcmpA(){
    check_NZ();
    tmpN=ac-prog[pc];
    bigEndianTmp = (prog[pc+1] << 8) | prog[pc];
    sr = SR_RST + Z_FLAG * (ac == ram[bigEndianTmp + xr]) + C_FLAG * (ac >= ram[bigEndianTmp + xr]) + N_FLAG * (tmpN >> 7 == 1);
    pc+=2;
}

void _D9cmpA(){
    check_NZ();
    tmpN=ac-prog[pc];
    bigEndianTmp = (prog[pc+1] << 8) | prog[pc];
    sr = SR_RST + Z_FLAG * (ac == ram[bigEndianTmp + yr]) + C_FLAG * (ac >= ram[bigEndianTmp + yr]) + N_FLAG * (tmpN >> 7 == 1);
    pc+=2;

}

void _C1cmpN(){
    check_NZ();
    tmpN=ac-prog[pc];
    bigEndianTmp = ram[prog[pc] + xr];
    sr = SR_RST + Z_FLAG * (ac == ram[bigEndianTmp]) + C_FLAG * (ac >= ram[bigEndianTmp]) + N_FLAG * (tmpN >> 7 == 1);
    pc+=2;
}

void _D1cmpN(){
    check_NZ();
    tmpN=ac-prog[pc];
    bigEndianTmp = ram[prog[pc]] + yr;
    sr = SR_RST + Z_FLAG * (ac == ram[bigEndianTmp]) + C_FLAG * (ac >= ram[bigEndianTmp]) + N_FLAG * (tmpN >> 7 == 1);
    pc+=2;
}


   /*
CPX

    Compare Memory and Index X

    X - M
    N	Z	C	I	D	V
    +	+	+	-	-	-
    addressing	assembler	opc	bytes	cycles
    immediate	CPX #oper	E0	2	2  
    zeropage	CPX oper	E4	2	3  
    absolute	CPX oper	EC	3	4  

*/
void _E0cpxI(){
    tmpN = xr - prog[pc];
    sr = sr&~Z_FLAG&~C_FLAG&~N_FLAG | Z_FLAG * (xr==prog[pc]) | C_FLAG * (xr >= prog[pc]) | N_FLAG * ((tmpN >> 7) == 1);
    pc++;
}
/*
CPY

    Compare Memory and Index Y

    Y - M
    N	Z	C	I	D	V
    +	+	+	-	-	-
    addressing	assembler	opc	bytes	cycles
    immediate	CPY #oper	C0	2	2  
    zeropage	CPY oper	C4	2	3  
    absolute	CPY oper	CC	3	4  

*/

void _C0cpyI(){
    check_NZ();
    tmpN=yr-prog[pc];
    sr = SR_RST + Z_FLAG * (yr == prog[pc]) + C_FLAG * (yr >= prog[pc]) + N_FLAG * (tmpN >> 7 == 1);
    pc++;
}

void _C4cpyZ(){
    check_NZ();
    tmpN=yr-prog[pc];
    sr = SR_RST + Z_FLAG * (yr == ram[prog[pc]]) + C_FLAG * (yr >= ram[prog[pc]]) + N_FLAG * (tmpN >> 7 == 1);
    pc++;
}

void _CCcpyA(){
    check_NZ();
    tmpN=yr-prog[pc];
    bigEndianTmp = (prog[pc+1] << 8) | prog[pc];
    sr = SR_RST + Z_FLAG * (yr == ram[bigEndianTmp]) + C_FLAG * (yr >= ram[bigEndianTmp]) + N_FLAG * (tmpN >> 7 == 1);
    pc+=2;
}


void _C6decZ(){
    check_NZ();
    ram[prog[pc]]--;
    pc++;
}

void _D6dexZ(){
    check_NZ();
    ram[prog[pc] + xr]--;
    pc++;
}

void _CEdecA(){
    check_NZ();
    ram[(prog[pc+1] << 8) | prog[pc]]--;
    pc+=2;
}

void _DEdexA(){
    check_NZ();
    ram[((prog[pc+1] << 8) | prog[pc]) + xr]--;
    pc+=2;
}


void _CAdexM(){
    check_NZ();
    xr--;
}

void _88deyM(){
    check_NZ();
    yr--;
}

void _49eorI(){
    check_NZ();
    ac ^= prog[pc];
    pc++;
}

void _45eorZ(){
    check_NZ();
    ac ^= ram[prog[pc]];
    pc++;
}

void _55eorZ(){
    check_NZ();
    ac ^= ram[prog[pc] + xr];
    pc++;
}

void _4DeorA(){
    check_NZ();
    ac ^= ram[(prog[pc+1] << 8) | prog[pc]];
    pc+=2;
}

void _5DeorA(){
    check_NZ();
    ac ^= ram[((prog[pc+1] << 8) | prog[pc]) + xr ];
    pc+=2;
}

void _59eorA(){
    check_NZ();
    ac ^= ram[((prog[pc+1] << 8) | prog[pc]) + yr ];
    pc+=2;
}

void _41eorN(){
    check_NZ();
    ac ^= ram[ram[prog[pc]+xr]];
    pc++;
}

void _51eorN(){
    check_NZ();
    ac ^= ram[ram[prog[pc]] + yr];
}

void _E6incZ(){
    check_NZ();
    ram[prog[pc]]++;
    pc++;
}

void _F6incZ(){
    check_NZ();
    ram[prog[pc] + xr]++;
    pc++;
}

void _EEincA(){
    check_NZ();
    ram[(prog[pc+1] << 8) | prog[pc]]++;
    pc+=2;
}

void _FEincA(){
    check_NZ();
    ram[((prog[pc+1] << 8) | prog[pc]) + xr]++;
    pc+=2;
}

void _E8inxM(){
    check_NZ();
    xr++;
}

void _C8inyM(){
    check_NZ();
    yr++;
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
    ram[STACK_END | sp-1] = bigEndianTmp;
    sp -= 2;
    pc = prog[pc] | (prog[pc+1] << 8);
}

void _A9ldaI(){
    check_NZ();
    ac = prog[pc];
    pc++;
}

void _A5ldaZ(){
    check_NZ();
    ac = ram[prog[pc]];
    pc++;
}

void _B5ldaZ(){
    check_NZ();
    ac = ram[prog[pc] + xr];
    pc++;
}

void _ADldaA(){
    check_NZ();
    ac = ram[(prog[pc+1] << 8) | prog[pc]];
    pc+=2;
}

void _BDldaA(){
    check_NZ();
    ac = ram[((prog[pc+1] << 8) | prog[pc]) + xr ];
    pc+=2;
}

void _B9ldaA(){
    check_NZ();
    ac = ram[((prog[pc+1] << 8) | prog[pc]) + yr ];
    pc+=2;
}

void _A1ldaN(){
    check_NZ();
    ac |= ram[ram[prog[pc]+xr]];
    pc++;
}

void _B1ldaN(){
    check_NZ();
    ac |= ram[ram[prog[pc]] + yr];
    pc++;
}


void _A2ldxI(){
    check_NZ();
    xr = prog[pc];
    pc++;
}

void _A6ldxZ(){
    check_NZ();
    xr = ram[prog[pc]];
    pc++;
}

void _B6ldxZ(){
    check_NZ();
    xr = ram[prog[pc] + yr];
    pc++;
}

void _AEldxA(){
    check_NZ();
    xr = ram[(prog[pc+1] << 8) | prog[pc]];
    pc+=2;
}

void _BEldxA(){
    check_NZ();
    xr = ram[((prog[pc+1] << 8) | prog[pc]) + yr];
    pc+=2;
}


void _A0ldyI(){
    check_NZ();
    yr = prog[pc];
    pc++;
}

void _A4ldyZ(){
    check_NZ();
    yr = ram[prog[pc]];
    pc++;
}

void _B4ldyZ(){
    check_NZ();
    yr = ram[prog[pc] + xr];
    pc++;
}

void _ACldyA(){
    check_NZ();
    yr = ram[(prog[pc+1] << 8) | prog[pc]];
    pc+=2;
}

void _BCldyA(){
    check_NZ();
    yr = ram[((prog[pc+1] << 8) | prog[pc]) + xr];
    pc+=2;
}


void _4AlsrC(){
    check_NZ();
    ac = ac >> 1 ;
}

void _46lsrZ(){
    check_NZ();
    ram[prog[pc]] = ram[prog[pc]] >> 1;
    pc++;
}

void _56lsrZ(){
    check_NZ();
    ram[prog[pc] + xr] = ram[prog[pc] + xr] >> 1;
    pc++;
}

void _4ElsrA(){
    check_NZ();
    ram[(prog[pc+1] << 8) | prog[pc]] = ram[(prog[pc+1] << 8) | prog[pc]] >> 1;
    pc+=2;
}

void _5ElsrA(){
    check_NZ();
    ram[((prog[pc+1] << 8) | prog[pc]) + xr] = ram[((prog[pc+1] << 8) | prog[pc]) + xr] >> 1;
    pc+=2;
}


void _01oraN(){
    check_NZ(); //indirect OR mem to AR
    ac |= ram[ram[prog[pc]+xr]];
    pc++;
}

void _05oraZ(){
    check_NZ();
    ac |= ram[prog[pc]];
    pc++;
}

void _09oraI(){
    check_NZ();
    ac |= prog[pc];
    pc++;
}

void _0DoraA(){
    check_NZ();
    ac |= ram[(prog[pc+1] << 8) | prog[pc]];
    pc+=2;
}

void _11oraN(){
    check_NZ(); //post-indexing with yr
    ac |= ram[ram[prog[pc]] + yr];
    pc++;
}

void _15oraZ(){
    check_NZ();
    ac |= ram[prog[pc] + xr];
    pc++;
}

void _19oraA(){
    check_NZ();
    ac |= ram[((prog[pc+1] << 8) | prog[pc]) + yr];
    pc+=2;
}

void _1DoraA(){
    check_NZ();
    ac |= ram[((prog[pc+1] << 8) | prog[pc]) + xr];
    pc+=2;
}

//ASL

void _06aslZ(){
    check_NZ(); // one bit left shift
    ram[prog[pc]] = ram[prog[pc]] << 1;
    pc++;
}

void _0Aasl_(){
    ac = ac << 1;
}

void _0EaslA(){
    check_NZ();
    ram[(prog[pc+1] << 8) | prog[pc]] = ram[(prog[pc+1] << 8) | prog[pc]] << 1;
    pc+=2;
}


void _48phaM() {
    ram[STACK_END | sp--] = ac;
}

void _08phpM() {
    ram[STACK_END | sp--] = sr;
}

void _68plaM() {
    ac = ram[STACK_END | ++sp];
}

void _28plpM() {
    sr = ram[STACK_END | ++sp];
}


uint8_t roTmp; // tmp used for storing the MSB or LSB before cycle-shifting

void _2ArolC(){
    check_NZ();
    roTmp = ac >> 7;
    ac = (ac<<1) | roTmp;
}

void _26rolZ(){
    check_NZ();
    roTmp = ram[prog[pc]] >> 7;
    ram[prog[pc]] = (ram[prog[pc]] << 1) | roTmp;
    pc++;
}

void _36rolZ(){
    check_NZ();
    roTmp = ram[prog[pc] + xr] >> 7;
    ram[prog[pc] + xr] = (ram[prog[pc] + xr] << 1) | roTmp;
    pc++;
}

void _2ErolA(){
    check_NZ();
    roTmp = ram[(prog[pc+1] << 8) | prog[pc]] >> 7;
    ram[(prog[pc+1] << 8) | prog[pc]] = (ram[(prog[pc+1] << 8) | prog[pc]] << 1) | roTmp;
    pc+=2;
}

void _3ErolA(){
    check_NZ();
    roTmp = ram[(prog[pc+1] << 8) | prog[pc]] >> 7;
    ram[((prog[pc+1] << 8) | prog[pc]) +  xr] = (ram[((prog[pc+1] << 8) | prog[pc])+  xr] << 1) | roTmp;
    pc+=2;
}

void _6ArorC(){
    check_NZ();
    roTmp = ac & 1;
    ac = (ac >> 1) | (roTmp<<7);
}

void _66rorZ(){
    check_NZ();
    roTmp = ram[prog[pc]] & 1;
    ram[prog[pc]] = (ram[prog[pc]] >> 1) | roTmp << 7;
    pc++;
}

void _76rorZ(){
    check_NZ();
    roTmp = ram[prog[pc] + xr] & 1;
    ram[prog[pc] + xr] = (ram[prog[pc] + xr] << 1) | roTmp;
    pc++;
}

void _6ErorA(){
    check_NZ();
    roTmp = ram[(prog[pc+1] << 8) | prog[pc]] & 1;
    ram[(prog[pc+1] << 8) | prog[pc]] = (ram[(prog[pc+1] << 8) | prog[pc]] >> 1) | (roTmp << 7);
    pc+=2;
}

void _7ErorA(){
    check_NZ();
    roTmp = ram[(prog[pc+1] << 8) | prog[pc]] & 1;
    ram[((prog[pc+1] << 8) | prog[pc]) + xr] = (ram[((prog[pc+1] << 8) | prog[pc])+ xr] >> 1) | (roTmp << 7);
    pc+=2;
}

/*

RTI

    Return from Interrupt

    The status register is pulled with the break flag
    and bit 5 ignored. Then PC is pulled from the stack.

    pull SR, pull PC
    N	Z	C	I	D	V
    from stack
    addressing	assembler	opc	bytes	cycles
    implied	RTI	40	1	6  
RTS

    Return from Subroutine

    pull PC, PC+1 -> PC
    N	Z	C	I	D	V
    -	-	-	-	-	-
    addressing	assembler	opc	bytes	cycles
    implied	RTS	60	1	6  

    */

void _60rtsM() {
    pc = ram[STACK_END|sp + 1] | (ram[STACK_END | sp + 2] << 8);
    sp += 2;
}

void _E9sbcI(){
    check_NZ();
    ac -= prog[pc] - ((sr & C_FLAG) ^ C_FLAG);
    pc++;
}

void _E5sbcZ(){
    check_NZ();
    ac -= ram[prog[pc]] - ((sr & C_FLAG) ^ C_FLAG);
    pc++;
}

void _F5sbcZ(){
    check_NZ();
    ac -= ram[prog[pc] + xr] - ((sr & C_FLAG) ^ C_FLAG);
    pc++;
}

void _EDsbcA(){
    check_NZ();
    ac -= ram[(prog[pc+1] << 8) | prog[pc]] - ((sr & C_FLAG) ^ C_FLAG);
    pc+=2;
}

void _FDsbcA(){
    check_NZ();
    ac -= ram[((prog[pc+1] << 8) | prog[pc]) + xr] - ((sr & C_FLAG) ^ C_FLAG);
    pc+=2;
}

void _F9sbcA(){
    check_NZ();
    ac -= ram[((prog[pc+1] << 8) | prog[pc]) + yr] - ((sr & C_FLAG) ^ C_FLAG);
    pc+=2;
}

void _E1sbcN(){
    check_NZ();
    ac -= ram[ram[prog[pc] + xr]] - ((sr & C_FLAG) ^ C_FLAG);
    pc++;
}

void _F1sbcN(){
    check_NZ();
    ac -= ram[ram[prog[pc]] + yr] - ((sr & C_FLAG) ^ C_FLAG);
    pc++;
}

void _38secM(){
    check_NZ();
    sr |= C_FLAG;
}

void _F8sedM(){
    check_NZ();
    sr |= D_FLAG;
}

void _78seiM(){
    check_NZ();
    sr |= I_FLAG;
}


void _85staZ(){
    check_NZ();
    ram[prog[pc]] = ac;
    pc++;
}

void _95staZ(){
    check_NZ();
    ram[prog[pc] + xr] = ac;
    pc++;
}

void _8DstaA(){
    check_NZ();
    ram[(prog[pc+1] << 8) | prog[pc]] = ac;
    pc+=2;
}

void _9DstaA(){
    check_NZ();
    ram[((prog[pc+1] << 8) | prog[pc]) + xr] = ac;
    pc+=2;
}

void _99staA(){
    check_NZ();
    ram[((prog[pc+1] << 8) | prog[pc]) + yr] = ac;
    pc+=2;
}

void _81staZ(){
    check_NZ();
    ram[ram[prog[pc] + xr]] = ac;
    pc++;
}

void _91staZ(){
    check_NZ();
    ram[ram[prog[pc]] + yr] = ac;
    pc++;
}


void _86stxZ(){
    check_NZ();
    ram[prog[pc]] = xr;
    pc++;
}

void _96stxZ(){
    check_NZ();
    ram[prog[pc] + yr] = xr;
    pc++;
}

void _8EstxA(){
    check_NZ();
    ram[(prog[pc+1] << 8) | prog[pc]] = xr;
    pc+=2;
}

void _84styZ(){
    check_NZ();
    ram[prog[pc]] = yr;
    pc++;
}

void _94styZ(){
    check_NZ();
    ram[prog[pc] + xr] = yr;
    pc++;
}

void _8CstyA(){
    check_NZ();
    ram[(prog[pc+1] << 8) | prog[pc]] = yr;
    pc+=2;
}

void _AAtaxM(){
    check_NZ();
    xr = ac;
}

void _A8tayM(){
    check_NZ();
    yr = ac;
}

void _BAtsxM(){
    check_NZ();
    sp = xr;
}

void _8AtxaM(){
    check_NZ();
    ac = xr;
}

void _9AtxsM(){
    check_NZ();
    sp = xr;
}

void _98tyaM(){
    check_NZ();
    ac = yr;
}

/*ILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETC

ILLEGAL ETC



ILLEGAL ETC

ILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETCILLEGAL ETC

*/
void _02jam(){ //freezes the CPU
    jammed = true;
}


// ILLEGAL

// SLO 

void _07sloZ(){
    check_NZ();
    _06aslZ();
    _05oraZ();
}

void _0FsloA(){
    check_NZ();
    _07sloZ();
    _0DoraA();
}

void _0BancI(){
    check_NZ();
    ac = (ac & prog[pc]) << C_FLAG;
    pc++;
}

// NOPS


void _1AnopM(){
    check_NZ();
    NOP;
}

void _3AnopM(){
    check_NZ();
    NOP;
}

void _5AnopM(){
    check_NZ();
    NOP;
}

void _7AnopM(){
    check_NZ();
    NOP;
}

void _DAnopM(){
    check_NZ();
    NOP;
}

void _FAnopM(){
    check_NZ();
    NOP;
}

void _80nopI(){
    check_NZ();
    NOP;
    pc++;
}

void _82nopI(){
    check_NZ();
    NOP;
    pc++;
}

void _89nopI(){
    check_NZ();
    NOP;
    pc++;
}

void _C2nopI(){
    check_NZ();
    NOP;
    pc++;
}

void _E2nopI(){
    check_NZ();
    NOP;
    pc++;
}



void _04nopZ(){
    check_NZ();
    NOP;
    pc++;
}

void _44nopZ(){
    check_NZ();
    NOP;
    pc++;
}

void _64nopZ(){
    check_NZ();
    NOP;
    pc++;
}

void _14nopZ(){
    check_NZ();
    NOP;
    pc++;
}

void _34nopZ(){
    check_NZ();
    NOP;
    pc++;
}

void _54nopZ(){
    check_NZ();
    NOP;
    pc++;
}

void _74nopZ(){
    check_NZ();
    NOP;
    pc++;
}

void _D4nopZ(){
    check_NZ();
    NOP;
    pc++;
}

void _F4nopZ(){
    check_NZ();
    NOP;
    pc++;
}

// NOPs

void _0CnopA(){
    check_NZ();
    NOP;
    pc += 2;
}

void _1CnopA(){
    check_NZ();
    NOP;
    pc += 2;
}

void _3CnopA(){
    check_NZ();
    NOP;
    pc += 2;
}

void _5CnopA(){
    check_NZ();
    NOP;
    pc += 2;
}

void _7CnopA(){
    check_NZ();
    NOP;
    pc += 2;
}

void _DCnopA(){
    check_NZ();
    NOP;
    pc += 2;
}

void _FCnopA(){
    check_NZ();
    NOP;
    pc += 2;
}


void _EAnopM(){
    check_NZ();
    NOP;
}

/*
_30bmiR() {
    pc += (((sr & N_FLAG) >> 1)) * (((prog[pc] >> 7) == 1) ? -1 * ((uint8_t)(~prog[pc]) + 1) : prog[pc]);          //prog[pc]
    pc++;
}
void _D0bneR(){
    pc += (!((sr & Z_FLAG) >> 1)) * (((prog[pc] >> 7) == 1) ? -1 * ((uint8_t)(~prog[pc]) + 1) : prog[pc]);          //prog[pc]
    pc++;
}

void _10bplR() {
*/
void (*opCodePtr[])() = {_00brk_, _01oraN, _02jam, E, _04nopZ, _05oraZ, _06aslZ, _07sloZ, 
                        _08phpM, _09oraI, _0Aasl_, _0BancI, _0CnopA, _0DoraA, _0EaslA, _0FsloA,
                        _10bplR, _11oraN, E, E, _14nopZ, _15oraZ, E, E,
                        _18clcM, _19oraA, _1AnopM, E, _1CnopA, _1DoraA, E, E, 
                        _20jsrA, _21andN, E, E, E, _25andZ,
                        _26rolZ, E, _28plpM, _29andI, _2ArolC, E, E, _2DandA, 
                        _2ErolA, E, _30bmiR, _31andN, E, E, _34nopZ, _35andZ,
                        _36rolZ, E, _38secM, _39andA, _3AnopM, E, _3CnopA, _3DandA,
                        _3ErolA, E, E, _41eorN, E, E, _44nopZ, _45eorZ, _46lsrZ,
                        E, _48phaM, _49eorI, _4AlsrC, E, _4CjmpA, _4DeorA, _4ElsrA,
                        E, _50bvcR, _51eorN, E, E, _54nopZ, _55eorZ, _56lsrZ,
                        E, _58cliM, _59eorA, _5AnopM, E, _5CnopA, _5DeorA, _5ElsrA,
                        E, _60rtsM, E, E, E, _64nopZ, _65adcZ, _66rorZ,
                        E, _68plaM, _69adcI, _6ArorC, E, _6CjmpN, E, _6ErorA,
                        E, _70bvsR, E, E, E, _74nopZ, _75adcZ, _76rorZ,
                        E, _78seiM, E, _7AnopM, E, _7CnopA, E, _7ErorA,
                        E, _80nopI, E, _82nopI, E, _84styZ, _85staZ, _86stxZ,
                        E, _88deyM, _89nopI, _8AtxaM, E, _8CstyA, _8DstaA, _8EstxA,
                        E, E, _91staZ, E, E, _94styZ, _95staZ, _96stxZ,
                        E, _98tyaM, _99staA, _9AtxsM, E, E, _9DstaA, E,
                        E, _A0ldyI, _A1ldaN, _A2ldxI, E, _A4ldyZ, _A5ldaZ, _A6ldxZ,
                        E, _A8tayM, _A9ldaI, _AAtaxM, E, E, _ADldaA, _AEldxA,
                        E, E, _B1ldaN, E, E, _B4ldyZ, _B5ldaZ, _B6ldxZ,
                        E, _B8clvM, _B9ldaA, _BAtsxM, E, E, _BDldaA, _BEldxA, 
                        E, _C0cpyI, _C1cmpN, _C2nopI, E, _C4cpyZ, _C5cmpZ, _C6decZ,
                        E, _C8inyM, _C9cmpI, _CAdexM, E, _CCcpyA, _CDcmpA, _CEdecA,
                        E, _D0bneR, _D1cmpN, E, E, _D4nopZ, _D5cmpZ, _D6dexZ,
                        E, _D8cldM, _D9cmpA, _DAnopM, E, _DCnopA, _DDcmpA, E,
                        E, _E0cpxI, _E1sbcN, _E2nopI, E, E, _E5sbcZ, _E6incZ,
                        E, _E8inxM, _E9sbcI, _EAnopM, _E9sbcI, E, _EDsbcA, _EEincA,
                        E, E, _F1sbcN, E, E, _F4nopZ, _F5sbcZ, E,
                        E, E, _F9sbcA, _FAnopM, E, _FCnopA, _FDsbcA, E, E
                        };

                        
uint8_t nums=0;
class Cpu{
    private:
        uint8_t *mem;
        uint8_t *prog;
        
    public:
        Cpu(uint8_t *ram, uint8_t *pr){

            mem = ram;
            prog = pr;
        }

        void exec(uint8_t *prgm){
            opCodePtr[prgm[(pc)++]](); 
            //++pc;
        }

        void run(){
            while(!jammed){
 
                exec(prog);
                afficher();
                
                
            }
            printf("\nCPU jammed at pc = $%X", pc-1);
            //nums += 1;
        }

        void afficher(){
            printf("\npc: %04X ac: %02x xr: %02x yr: %02x sp: %02x sr: %02x ",pc,ac,xr,yr,sp,sr);
            for (int i = 0; i < 8; i++) {
                //if ((i!=2)&(i!=3))
                printf("%d", (sr >> 7-i) & 1);
            }
           
        }
       
};

int main(){

    //measure the time accurately (milliseconds) it takes to run the program
    pc = ORG;
    insertAt(ram, ORG, prog2);
    Cpu f(ram,prog);
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
    
   f.run();
   //printf("%d", nums);
   

    return 0;
}

void insertAt(uint8_t* mem, uint64_t at, uint16_t* prog2){
    uint32_t i = 0;
    while (prog2[i] != 0xDEAD) {
        mem[at + i] = prog2[i];
        i++;
    }
}