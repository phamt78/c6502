/*
c6502.c
Legend to Flags:
+   modified
-   not modified
1   set
0   cleared
M6  memory bit 6
M7  memory bit 7
*/

#include "c6502.h"

// Global variable definition
c6502_cpu c6502;

/*
c6502_init() Initialize 6502 processor to boot up state.
On Power up the Interrupt disable flag is initialised to 1 by the CPU reset logic.
*/
void c6502_init(uint8_t PC_MSB, uint8_t PC_LSB)
{
    ADDRESS[0xFFFC] = PC_LSB;
    ADDRESS[0xFFFD] = PC_MSB;
    c6502.A = 0x00;
    c6502.SR = 0x00;
    c6502.SP = 0xFF; // Set Stack pointer to first address in the stack
    c6502.X = 0x00;
    c6502.Y = 0x00;
    c6502.abs_address = 0x0000;
    c6502.rel_address = 0x0000;
    c6502.opcode = 0x00;
    c6502.JAM = false;

    // Reset pin active low.
    c6502.reset_pin = 0;

    // Perform reset routine
    if (c6502.reset_pin == 0)
    {
        c6502_reset();
    }
}

// c6502_read_opcode() Fetch opcode
void c6502_read_opcode()
{
    c6502.opcode = cpu_read(c6502.PC);
}

// c6502_set_status_flag() Set CPU flag
void c6502_set_status_flag(c6502_status_flags flag, bool x)
{
    if (x)
    {
        // set flag bit
        c6502.SR |= flag;
    }
    else
    {
        // clear flag bit
        c6502.SR &= ~flag;
    }
}

// c6502_get_flag() Check if cpu flag is set.
uint8_t c6502_get_flag(c6502_status_flags flag)
{

    if ((c6502.SR & flag) > 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/*
c6502_sp_abs() Return stack pointer absolute address
6502 support for a stack implemented using 256byte whose location is hardcoded at page $01 (0x0100-0x01FF)
Note: Stack pointer starts at address 0x01FF and grows towards 0x0100
*/
uint16_t c6502_sp_abs(uint8_t SP)
{
    return SP + 0x0100;
}

/*
c6502_irq() Interrupt
Push PC MSB, PC LSB, and SR onto the stack. Set interrupt disable flag. Unset Break flag.
Load vector (0xFFFE/0xFFFF) to Program Counter.
Instruction take 7 clock cycles
*/
void c6502_irq()
{

    if (c6502_get_flag(I) == 0)
    {
        cpu_write(c6502_sp_abs(c6502.SP--), (c6502.PC >> 8) & 0x00FF);
        cpu_write(c6502_sp_abs(c6502.SP--), c6502.PC & 0x00FF);
        cpu_write(c6502_sp_abs(c6502.SP--), c6502.SR);
        c6502_set_status_flag(I, true);
        c6502_set_status_flag(B, false);
        uint16_t LSB = (uint16_t)cpu_read(0xFFFE);
        uint16_t MSB = (uint16_t)cpu_read(0xFFFF) << 8;
        c6502.PC = MSB | LSB;
    }
    c6502.cycles += 7;
}

/*
c6502_nmi() Non-maskable interrupt
Push PC MSB, PC LSB, and SR onto the stack. Set interrupt disable flag. Unset Break flag.
Load vector (0xFFFA/0xFFFB) to Program Counter.
Instruction take 7 clock cycles
*/
void c6502_nmi()
{
    cpu_write(c6502_sp_abs(c6502.SP--), (c6502.PC >> 8) & 0x00FF);
    cpu_write(c6502_sp_abs(c6502.SP--), c6502.PC & 0x00FF);
    cpu_write(c6502_sp_abs(c6502.SP--), c6502.SR);
    c6502_set_status_flag(I, true);
    c6502_set_status_flag(B, false);
    c6502.PC = (uint16_t)cpu_read(0xFFFA) | (uint16_t)cpu_read(0xFFFB) << 8;
    c6502.cycles += 7;
}

/*
c6502_reset() Reset 6502 cpu.
Load vector (0xFFFC/0xFFFD) to Program Counter.
Clear all register and status flag.
Set interrupt disable flag.
Instruction takes 7 clock cycles
*/
void c6502_reset()
{
    c6502.PC = (uint16_t)cpu_read(0xFFFC) | (uint16_t)cpu_read(0xFFFD) << 8;
    c6502.A = 0;
    /*
    To reuse common interrupt handling hardware the 6502 perform fake push operation. Normally pushed (PC and P) register
    isn't actually written to the stack memory. Despite these being fake pushes the stack pointer is decremented
    3 times as part of the process.
    */
    c6502.SP = c6502.SP - 2;
    c6502.SR = 0x00;
    c6502.X = 0x00;
    c6502.Y = 0x00;
    c6502.abs_address = 0x0000;
    c6502.rel_address = 0x0000;
    c6502.opcode = 0x00;
    c6502.JAM = false;
    // The unused bit is hardwired to logic 1 by the internal circuitry.
    c6502_set_status_flag(U, true);
    /*
    The interrupt disable flag in the status register is set to 1 default when the processor is reset.
    This prevents the processor from responding to IRQ signal until the flag is clear by software.
    */
    c6502_set_status_flag(I, true);

    // Pull the reset pin high to disable reset routine.
    c6502.reset_pin = 1;
    c6502.cycles += 7;
}

//----------------------
// 6502 Opcodes
//----------------------

// UNK() Unknown opcode. Use as place holder for illegal opcode not implemented yet.
void UNK() {}

/*
JAM() (KILL, HLT) *illegal opcode*

These instructions freeze the CPU. The processor will be trapped infinitely in T1 phase with
$FF on the data bus. - Reset required.

Instruction codes: 02,12,22,32,42,52,62,72,92,B2,D2,F2

The main processor loop should check for this flag and halt the cpu until the reset button is push.
*/
void JAM()
{
    c6502.JAM = true;
    DATABUS = 0xFF;
}

/*
ADC() Add Memory to Accumulator with Carry

A + M + C -> A, C
N	Z	C	I	D	V
+	+	+	-	-	+
addressing	    assembler	    opc bytes   cycles
immediate	    ADC #oper	    69	2       2
zeropage	    ADC oper	    65	2       3
zeropage,X	    ADC oper,X	    75	2	    4
absolute	    ADC oper	    6D	3	    4
absolute,X	    ADC oper,X	    7D	3	    4*
absolute,Y	    ADC oper,Y	    79	3	    4*
(indirect,X)	ADC (oper,X)	61	2	    6
(indirect),Y	ADC (oper),Y	71	2	    5*
*/
void ADC()
{
    lookup_table[c6502.opcode].address_mode();

    uint16_t augend = (uint16_t)c6502.A;
    uint16_t addend = (uint16_t)cpu_read(c6502.abs_address);
    uint16_t sum = augend + addend + c6502_get_flag(C);

    c6502.A = sum & 0x00FF;

    c6502_set_status_flag(N, c6502.A & 0x80);
    c6502_set_status_flag(Z, c6502.A == 0);
    c6502_set_status_flag(C, sum & 0xFF00);
    c6502_set_status_flag(V, (~(augend ^ addend)) & (augend ^ sum) & 0x80);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
AND() AND Memory with Accumulator

A AND M -> A
N	Z	C	I	D	V
+	+	-	-	-	-
addressing	    assembler	    opc	 bytes	cycles
immediate	    AND #oper	    29	 2	    2
zeropage	    AND oper	    25	 2	    3
zeropage,X	    AND oper,X	    35	 2	    4
absolute	    AND oper	    2D	 3	    4
absolute,X	    AND oper,X	    3D	 3	    4*
absolute,Y	    AND oper,Y	    39	 3	    4*
(indirect,X)	AND (oper,X)	21	 2	    6
(indirect),Y	AND (oper),Y	31	 2	    5*
*/
void AND()
{
    lookup_table[c6502.opcode].address_mode();
    uint8_t temp = cpu_read(c6502.abs_address);

    c6502.A = (c6502.A & temp);

    c6502_set_status_flag(N, c6502.A & 0x80);
    c6502_set_status_flag(Z, c6502.A == 0);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
ASL() Shift Left One Bit (Memory or Accumulator)

C <- [76543210] <- 0
N	Z	C	I	D	V
+	+	+	-	-	-

addressing	    assembler	opc	    bytes	cycles
accumulator	    ASL A	    0A	    1	    2
zeropage	    ASL oper	06	    2	    5
zeropage,X	    ASL oper,X	16	    2	    6
absolute	    ASL oper	0E	    3	    6
absolute,X	    ASL oper,X	1E	    3	    7
*/
void ASL()
{
    lookup_table[c6502.opcode].address_mode();
    uint16_t temp = 0;

    if (lookup_table[c6502.opcode].address_mode == A)
    {
        temp = (c6502.A << 1);
        c6502.A = temp & 0x00FF;
    }
    else
    {
        temp = cpu_read(c6502.abs_address);
        temp = temp << 1;
        cpu_write(c6502.abs_address, temp & 0x00FF);
    }

    c6502_set_status_flag(N, temp & 0x80);
    c6502_set_status_flag(Z, (temp & 0x00FF) == 0x00);
    c6502_set_status_flag(C, (temp & 0xFF00) > 0);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
BCC() Branch on Carry Clear

branch on C = 0
N	Z	C	I	D	V
-	-	-	-	-	-

addressing	assembler	opc	bytes	cycles
relative	BCC oper	90	2	    2**

The 6502 BCC (Branch on Carry Clear) instruction takes 2 cycles if the branch is not taken,
and 3 cycles if the branch is taken (and 4 cycles if the branch crosses a page boundary).
*/
void BCC()
{
    lookup_table[c6502.opcode].address_mode();

    if (c6502_get_flag(C) == 0)
    {
        // add a cycle if the branch is taken
        c6502.cycles++;

        c6502.abs_address = c6502.PC + c6502.rel_address;

        // add another cycle if the branch crosses a page boundary.
        if ((c6502.abs_address & 0xFF00) != (c6502.PC & 0xFF00))
        {
            c6502.cycles++;
        }
        c6502.PC = c6502.abs_address;
    }

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
BCS() Branch on Carry Set

branch on C = 1
N	Z	C	I	D	V
-	-	-	-	-	-
addressing	assembler	opc	bytes	cycles
relative	BCS oper	B0	2	    2**
*/
void BCS()
{
    lookup_table[c6502.opcode].address_mode();

    if (c6502_get_flag(C) == 1)
    {
        // add a cycle if the branch is taken
        c6502.cycles++;

        c6502.abs_address = c6502.PC + c6502.rel_address;

        // add another cycle if the branch crosses a page boundary.
        if ((c6502.abs_address & 0xFF00) != (c6502.PC & 0xFF00))
        {
            c6502.cycles++;
        }
        c6502.PC = c6502.abs_address;
    }

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
BEQ() Branch on Result Zero

branch on Z = 1
N	Z	C	I	D	V
-	-	-	-	-	-
addressing	assembler	opc	bytes	cycles
relative	BEQ oper	F0	2	    2**
*/
void BEQ()
{
    lookup_table[c6502.opcode].address_mode();

    if (c6502_get_flag(Z) == 1)
    {

        // add a cycle if the branch is taken
        c6502.cycles++;

        c6502.abs_address = c6502.PC + c6502.rel_address;

        // add another cycle if the branch crosses a page boundary.
        if ((c6502.abs_address & 0xFF00) != (c6502.PC & 0xFF00))
        {
            c6502.cycles++;
        }
        c6502.PC = c6502.abs_address;
    }

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
BIT() Test Bits in Memory with Accumulator

bits 7 and 6 of operand are transferred to bit 7 and 6 of SR (N,V);
the zero-flag is set according to the result of the operand AND
the accumulator (set, if the result is zero, unset otherwise).
This allows a quick check of a few bits at once without affecting
any of the registers, other than the status register (SR).

A AND M -> Z, M7 -> N, M6 -> V
N	Z	C	I	D	V
M7	+	-	-	-	M6
addressing	assembler	opc	bytes	cycles
zeropage	BIT oper	24	2	    3
absolute	BIT oper	2C	3	    4
*/
void BIT()
{
    lookup_table[c6502.opcode].address_mode();

    uint8_t operand;
    uint8_t results;
    operand = cpu_read(c6502.abs_address);
    results = c6502.A & operand;
    // bit  7
    c6502_set_status_flag(N, operand & N);
    // bit 6
    c6502_set_status_flag(V, operand & V);
    c6502_set_status_flag(Z, results == 0);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
BMI() Branch on Result Minus

branch on N = 1
N	Z	C	I	D	V
-	-	-	-	-	-
addressing	assembler	opc	bytes	cycles
relative	BMI oper	30	2	    2**
*/
void BMI()
{
    lookup_table[c6502.opcode].address_mode();

    if (c6502_get_flag(N) == 1)
    {
        // add a cycle if the branch is taken
        c6502.cycles++;

        c6502.abs_address = c6502.PC + c6502.rel_address;

        // add another cycle if the branch crosses a page boundary.
        if ((c6502.abs_address & 0xFF00) != (c6502.PC & 0xFF00))
        {
            c6502.cycles++;
        }
        c6502.PC = c6502.abs_address;
    }

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
BNE() Branch on Result not Zero

branch on Z = 0
N	Z	C	I	D	V
-	-	-	-	-	-
addressing	assembler	opc	bytes	cycles
relative	BNE oper	D0	2	    2**
*/
void BNE()
{
    lookup_table[c6502.opcode].address_mode();

    if (c6502_get_flag(Z) == 0)
    {

        // add a cycle if the branch is taken
        c6502.cycles++;

        c6502.abs_address = c6502.PC + c6502.rel_address;

        // add another cycle if the branch crosses a page boundary.
        if ((c6502.abs_address & 0xFF00) != (c6502.PC & 0xFF00))
        {
            c6502.cycles++;
        }
        c6502.PC = c6502.abs_address;
    }

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
BPL() Branch on Result Plus

branch on N = 0
N	Z	C	I	D	V
-	-	-	-	-	-
addressing	assembler	opc	bytes	cycles
relative	BPL oper	10	2	2**
*/
void BPL()
{
    lookup_table[c6502.opcode].address_mode();

    if (c6502_get_flag(N) == 0)
    {
        // add a cycle if the branch is taken
        c6502.cycles++;

        c6502.abs_address = c6502.PC + c6502.rel_address;

        // add another cycle if the branch crosses a page boundary.
        if ((c6502.abs_address & 0xFF00) != (c6502.PC & 0xFF00))
        {
            c6502.cycles++;
        }
        c6502.PC = c6502.abs_address;
    }

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
BRK() Force Break

N	Z	C	I	D	V
-	-	-	1	-	-
addressing	assembler	opc	bytes	cycles
implied	    BRK	        00	1	    7

Push PC MSB, PC LSB, and SR onto the stack. Set interrupt disable and break flag.

Load vector (0xFFFE/0xFFFF) to Program Counter.
*/
void BRK()
{
    uint16_t LSB = (uint16_t)cpu_read(c6502.PC++);
    uint16_t MSB = (uint16_t)cpu_read(c6502.PC++);
    cpu_write(c6502_sp_abs(c6502.SP--), (c6502.PC >> 8) & 0x00FF);
    cpu_write(c6502_sp_abs(c6502.SP--), c6502.PC & 0x00FF);
    cpu_write(c6502_sp_abs(c6502.SP--), c6502.SR);
    c6502_set_status_flag(I, true);
    c6502_set_status_flag(B, true);
    c6502.PC = (MSB << 8) | LSB;

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
BVC() Branch on Overflow Clear

branch on V = 0
N	Z	C	I	D	V
-	-	-	-	-	-
addressing	assembler	opc	bytes	cycles
relative	BVC oper	50	2	2**
*/
void BVC()
{
    lookup_table[c6502.opcode].address_mode();

    if (c6502_get_flag(V) == 0)
    {
        // add a cycle if the branch is taken
        c6502.cycles++;

        c6502.abs_address = c6502.PC + c6502.rel_address;

        // add another cycle if the branch crosses a page boundary.
        if ((c6502.abs_address & 0xFF00) != (c6502.PC & 0xFF00))
        {
            c6502.cycles++;
        }
        c6502.PC = c6502.abs_address;
    }

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
BVS() Branch on Overflow Set

branch on V = 1
N	Z	C	I	D	V
-	-	-	-	-	-
addressing	assembler	opc	bytes	cycles
relative	BVS oper	70	2	2**
*/
void BVS()
{
    lookup_table[c6502.opcode].address_mode();

    if (c6502_get_flag(V) == 1)
    {
        // add a cycle if the branch is taken
        c6502.cycles++;

        c6502.abs_address = c6502.PC + c6502.rel_address;

        // add another cycle if the branch crosses a page boundary.
        if ((c6502.abs_address & 0xFF00) != (c6502.PC & 0xFF00))
        {
            c6502.cycles++;
        }
        c6502.PC = c6502.abs_address;
    }

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
CLC() Clear Carry Flag

0 -> C
N	Z	C	I	D	V
-	-	0	-	-	-

addressing	assembler	opc	bytes	cycles
implied	    CLC	        18	1	    2
*/
void CLC()
{
    c6502_set_status_flag(C, false);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
CLD() Clear Decimal Mode
0 -> D
N	Z	C	I	D	V
-	-	-	-	0	-
addressing	assembler	opc	bytes	cycles
implied	    CLD	        D8	1	    2
CLI
*/
void CLD()
{
    c6502_set_status_flag(D, false);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
CLI Clear Interrupt Disable Bit

0 -> I
N	Z	C	I	D	V
-	-	-	0	-	-
addressing	assembler	opc	bytes	cycles
implied	    CLI	        58	1	    2
*/
void CLI()
{
    c6502_set_status_flag(I, false);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
CLV() Clear Overflow Flag

0 -> V
N	Z	C	I	D	V
-	-	-	-	-	0
addressing	assembler	opc	bytes	cycles
implied	    CLV	        B8	1	    2
*/
void CLV()
{
    c6502_set_status_flag(V, false);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
CMP() Compare Memory with Accumulator

A - M
N	Z	C	I	D	V
+	+	+	-	-	-
addressing	    assembler	    opc	bytes   cycles
immediate	    CMP #oper	    C9	2	    2
zeropage	    CMP oper	    C5	2	    3
zeropage,X	    CMP oper,X	    D5	2	    4
absolute	    CMP oper	    CD	3	    4
absolute,X	    CMP oper,X	    DD	3	    4*
absolute,Y	    CMP oper,Y	    D9	3	    4*
(indirect,X)	CMP (oper,X)	C1	2	    6
(indirect),Y	CMP (oper),Y	D1	2	    5*
*/
void CMP()
{
    lookup_table[c6502.opcode].address_mode();

    uint8_t temp = cpu_read(c6502.abs_address);
    uint16_t results = c6502.A - temp;

    c6502_set_status_flag(N, results & 0x80);
    c6502_set_status_flag(Z, results == 0x00);
    c6502_set_status_flag(C, c6502.A >= temp);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
CPX() Compare Memory and Index X

X - M
N	Z	C	I	D	V
+	+	+	-	-	-
addressing	assembler	opc	bytes	cycles
immediate	CPX #oper	E0	2	    2
zeropage	CPX oper	E4	2	    3
absolute	CPX oper	EC	3	    4
*/
void CPX()
{
    lookup_table[c6502.opcode].address_mode();

    uint8_t temp = cpu_read(c6502.abs_address);
    uint8_t results = c6502.X - temp;

    c6502_set_status_flag(N, results & 0x80);
    c6502_set_status_flag(Z, results == 0x00);
    c6502_set_status_flag(C, c6502.X >= temp);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
CPY() Compare Memory and Index Y

Y - M
N	Z	C	I	D	V
+	+	+	-	-	-
addressing	assembler	opc	bytes	cycles
immediate	CPY #oper	C0	2	    2
zeropage	CPY oper	C4	2	    3
absolute	CPY oper	CC	3	    4
*/
void CPY()
{
    lookup_table[c6502.opcode].address_mode();

    uint8_t temp = cpu_read(c6502.abs_address);
    uint8_t results = c6502.Y - temp;

    c6502_set_status_flag(N, results & 0x80);
    c6502_set_status_flag(Z, results == 0x00);
    c6502_set_status_flag(C, c6502.Y >= temp);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
DCP (DCM) *illegal opcode*
DEC oper + CMP oper

M - 1 -> M, A - M

Decrements the operand and then compares the result to the accumulator.

N	Z	C	I	D	V
+	+	+	-	-	-
addressing	    assembler	    opc	bytes	cycles
zeropage	    DCP oper	    C7	2	    5
zeropage,X	    DCP oper,X	    D7	2	    6
absolute	    DCP oper	    CF	3	    6
absolute,X	    DCP oper,X	    DF	3	    7
absolute,Y	    DCP oper,Y	    DB	3	    7
(indirect,X)	DCP (oper,X)	C3	2	    8
(indirect),Y	DCP (oper),Y	D3	2	    8
*/
void DCP()
{
    lookup_table[c6502.opcode].address_mode();

    // DEC()
    uint8_t temp = cpu_read(c6502.abs_address) - 1;
    c6502_set_status_flag(N, temp & 0x80);
    c6502_set_status_flag(Z, temp == 0x00);
    cpu_write(c6502.abs_address, temp);

    // CMP()
    uint16_t results = c6502.A - temp;
    c6502_set_status_flag(N, results & 0x80);
    c6502_set_status_flag(Z, results == 0x00);
    c6502_set_status_flag(C, c6502.A >= temp);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
DEC() Decrement Memory by One

M - 1 -> M
N	Z	C	I	D	V
+	+	-	-	-	-
addressing	assembler	opc	bytes	cycles
zeropage	DEC oper	C6	2	    5
zeropage,X	DEC oper,X	D6	2	    6
absolute	DEC oper	CE	3	    6
absolute,X	DEC oper,X	DE	3	    7
*/
void DEC()
{
    lookup_table[c6502.opcode].address_mode();

    uint8_t temp = cpu_read(c6502.abs_address) - 1;

    c6502_set_status_flag(N, temp & 0x80);
    c6502_set_status_flag(Z, temp == 0x00);

    cpu_write(c6502.abs_address, temp);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
DEX() Decrement Index X by One

X - 1 -> X
N	Z	C	I	D	V
+	+	-	-	-	-
addressing	assembler	opc	bytes	cycles
implied	    DEX	        CA	1	    2
*/
void DEX()
{
    c6502.X--;

    c6502_set_status_flag(N, c6502.X & 0x80);
    c6502_set_status_flag(Z, c6502.X == 0x00);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
DEY() Decrement Index Y by One

Y - 1 -> Y
N	Z	C	I	D	V
+	+	-	-	-	-
addressing	assembler	opc	bytes	cycles
implied	    DEY	        88	1	    2
 */
void DEY()
{
    c6502.Y--;

    c6502_set_status_flag(N, c6502.Y & 0x80);
    c6502_set_status_flag(Z, c6502.Y == 0x00);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
EOR() Exclusive-OR Memory with Accumulator

A EOR M -> A
N	Z	C	I	D	V
+	+	-	-	-	-
addressing	    assembler	    opc	bytes	cycles
immediate	    EOR #oper	    49	2	    2
zeropage	    EOR oper	    45	2	    3
zeropage,X	    EOR oper,X	    55	2	    4
absolute	    EOR oper	    4D	3	    4
absolute,X	    EOR oper,X	    5D	3	    4*
absolute,Y	    EOR oper,Y	    59	3	    4*
(indirect,X)	EOR (oper,X)	41	2	    6
(indirect),Y	EOR (oper),Y	51	2	    5*
*/
void EOR()
{
    lookup_table[c6502.opcode].address_mode();

    uint8_t temp = cpu_read(c6502.abs_address);

    c6502.A ^= temp;

    c6502_set_status_flag(N, c6502.A & 0x80);
    c6502_set_status_flag(Z, c6502.A == 0x00);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
INC() Increment Memory by One

M + 1 -> M
N	Z	C	I	D	V
+	+	-	-	-	-
addressing	assembler	opc	bytes	cycles
zeropage	INC oper	E6	2	    5
zeropage,X	INC oper,X	F6	2	    6
absolute	INC oper	EE	3	    6
absolute,X	INC oper,X	FE	3	    7
*/
void INC()
{
    lookup_table[c6502.opcode].address_mode();

    uint8_t results = cpu_read(c6502.abs_address) + 1;

    c6502_set_status_flag(N, results & 0x80);
    c6502_set_status_flag(Z, results == 0x00);

    cpu_write(c6502.abs_address, results);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
INX() Increment Index X by One

X + 1 -> X
N	Z	C	I	D	V
+	+	-	-	-	-
addressing	assembler	opc	bytes	cycles
implied	    INX	        E8	1	    2
*/
void INX()
{
    c6502.X++;

    c6502_set_status_flag(N, c6502.X & 0x80);
    c6502_set_status_flag(Z, c6502.X == 0x00);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
INY() Increment Index Y by One

Y + 1 -> Y
N	Z	C	I	D	V
+	+	-	-	-	-
addressing	assembler	opc	bytes	cycles
implied	    INY	        C8	1	    2
*/
void INY()
{
    c6502.Y++;

    c6502_set_status_flag(N, c6502.Y & 0x80);
    c6502_set_status_flag(Z, c6502.Y == 0x00);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
ISB() *illegal opcode*
ISC (ISB, INS)
INC oper + SBC oper

M + 1 -> M, A - M - C̅ -> A

N	Z	C	I	D	V
+	+	+	-	-	+
addressing	    assembler	    opc	bytes	cycles
zeropage	    ISB oper	    E7	2	    5
zeropage,X	    ISB oper,X	    F7	2	    6
absolute	    ISB oper	    EF	3	    6
absolute,X	    ISB oper,X	    FF	3	    7
absolute,Y	    ISB oper,Y	    FB	3	    7
(indirect,X)	ISB (oper,X)	E3	2	    8
(indirect),Y	ISB (oper),Y	F3	2	    8
*/
void ISB()
{
    lookup_table[c6502.opcode].address_mode();

    // INC()
    uint8_t results = cpu_read(c6502.abs_address) + 1;
    cpu_write(c6502.abs_address, results);

    c6502_set_status_flag(N, results & 0x80);
    c6502_set_status_flag(Z, results == 0x00);

    // SBC()
    uint16_t minuend = (uint16_t)c6502.A;
    uint16_t subtrahend = results;
    // Find the one's complement of the subtrahend by flipping the bits.
    uint16_t ones_complement = subtrahend ^ 0x00FF;
    // Find the two's complement of the subtrahend by adding the carry bit.
    uint16_t twos_complement = ones_complement + c6502_get_flag(C);
    // Add the two's complement to minuend
    uint16_t difference = minuend + twos_complement;

    c6502.A = difference & 0x00FF;

    c6502_set_status_flag(N, c6502.A & 0x80);
    c6502_set_status_flag(Z, c6502.A == 0);
    c6502_set_status_flag(C, difference & 0xFF00);

    c6502_set_status_flag(V, (~(minuend ^ twos_complement) & (minuend ^ difference) & 0x80));

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
JMP() Jump to New Location

operand 1st byte -> PCL
operand 2nd byte -> PCH
N	Z	C	I	D	V
-	-	-	-	-	-
addressing	assembler	opc	bytes	cycles
absolute	JMP oper	4C	3	    3
indirect	JMP (oper)	6C	3	    5
*/
void JMP()
{
    lookup_table[c6502.opcode].address_mode();

    c6502.PC = c6502.abs_address;

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
JSR() Jump to New Location Saving Return Address

push (PC+2),
operand 1st byte -> PCL
operand 2nd byte -> PCH
N	Z	C	I	D	V
-	-	-	-	-	-
addressing	assembler	opc	bytes	cycles
absolute	JSR oper	20	3	    6
*/
void JSR()
{
    lookup_table[c6502.opcode].address_mode();

    // Push The HIGH and LOW byte of the PC on to the stack.
    c6502.PC--; // Decrement Program Counter to start at return address OPCODE not the second byte.
    cpu_write(c6502_sp_abs(c6502.SP--), (c6502.PC >> 8) & 0x00FF);
    cpu_write(c6502_sp_abs(c6502.SP--), c6502.PC & 0x00FF);

    // Point the Program counter to the New Jump location
    c6502.PC = c6502.abs_address;

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
LAX()  LDA oper + LDX oper *illegal opcode*

M -> A -> X

N	Z	C	I	D	V
+	+	-	-	-	-
addressing	assembler	opc	bytes	cycles
zeropage	LAX oper	A7	2	3
zeropage,Y	LAX oper,Y	B7	2	4
absolute	LAX oper	AF	3	4
absolute,Y	LAX oper,Y	BF	3	4*
(indirect,X)	LAX (oper,X)	A3	2	6
(indirect),Y	LAX (oper),Y	B3	2	5*
*/
void LAX()
{
    lookup_table[c6502.opcode].address_mode();

    c6502.A = cpu_read(c6502.abs_address);

    c6502.X = c6502.A;

    c6502_set_status_flag(N, c6502.A & 0x80);
    c6502_set_status_flag(Z, c6502.A == 0x00);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
LDA() Load Accumulator with Memory

M -> A
N	Z	C	I	D	V
+	+	-	-	-	-
addressing	    assembler	        opc	    bytes	cycles
immediate	    LDA #oper	        A9	    2	    2
zeropage	    LDA oper	        A5	    2	    3
zeropage,X	    LDA oper,X	        B5	    2	    4
absolute	    LDA oper	        AD	    3	    4
absolute,X	    LDA oper,X	        BD	    3	    4*
absolute,Y	    LDA oper,Y	        B9	    3	    4*
(indirect,X)	LDA (oper,X)        A1	    2	    6
(indirect),Y	LDA (oper),Y	    B1	    2	    5*
*/
void LDA()
{
    lookup_table[c6502.opcode].address_mode();

    c6502.A = cpu_read(c6502.abs_address);

    c6502_set_status_flag(N, c6502.A & 0x80);
    c6502_set_status_flag(Z, c6502.A == 0x00);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
LDX() Load Index X with Memory

M -> X
N	Z	C	I	D	V
+	+	-	-	-	-
addressing	assembler	opc	bytes	cycles
immediate	LDX #oper	A2	2	    2
zeropage	LDX oper	A6	2	    3
zeropage,Y	LDX oper,Y	B6	2	    4
absolute	LDX oper	AE	3	    4
absolute,Y	LDX oper,Y	BE	3	    4*
*/
void LDX()
{
    lookup_table[c6502.opcode].address_mode();

    c6502.X = cpu_read(c6502.abs_address);

    c6502_set_status_flag(N, c6502.X & 0x80);
    c6502_set_status_flag(Z, c6502.X == 0x00);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
LDY() Load Index Y with Memory

M -> Y
N	Z	C	I	D	V
+	+	-	-	-	-
addressing	assembler	opc	bytes	cycles
immediate	LDY #oper	A0	2	    2
zeropage	LDY oper	A4	2	    3
zeropage,X	LDY oper,X	B4	2	    4
absolute	LDY oper	AC	3	    4
absolute,X	LDY oper,X	BC	3	    4*
*/
void LDY()
{
    lookup_table[c6502.opcode].address_mode();

    c6502.Y = cpu_read(c6502.abs_address);

    c6502_set_status_flag(N, c6502.Y & 0x80);
    c6502_set_status_flag(Z, c6502.Y == 0x00);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
LSR() Shift One Bit Right (Memory or Accumulator)

0 -> [76543210] -> C
N	Z	C	I	D	V
0	+	+	-	-	-

addressing	assembler	opc	bytes	cycles
accumulator	LSR A	    4A	1	    2
zeropage	LSR oper	46	2	    5
zeropage,X	LSR oper,X	56	2	    6
absolute	LSR oper	4E	3	    6
absolute,X	LSR oper,X	5E	3	    7
*/
void LSR()
{
    lookup_table[c6502.opcode].address_mode();

    uint8_t temp = 0;
    uint8_t data = 0;

    // Check if shifting the Accumulator and not Memory
    if (lookup_table[c6502.opcode].address_mode == A)
    {
        data = c6502.A;
        temp = data >> 1;
        c6502.A = temp;
    }
    else
    {
        data = cpu_read(c6502.abs_address);
        temp = data >> 1;
        cpu_write(c6502.abs_address, temp);
    }

    c6502_set_status_flag(N, temp & 0x80);
    c6502_set_status_flag(Z, temp == 0x00);
    c6502_set_status_flag(C, data & 0x01);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
NOP()
N	Z	C	I	D	V
-	-	-	-	-	-
addressing	assembler	opc	bytes	cycles
implied     NOP         EA  1       2

NOP() (including DOP, TOP) *illegal instruction*
Instructions effecting in 'no operations' in various address modes. Operands are ignored.

N	Z	C	I	D	V
-	-	-	-	-	-
opc	addressing	bytes	cycles
1A	implied	    1	    2
3A	implied	    1	    2
5A	implied	    1	    2
7A	implied	    1	    2
DA	implied	    1	    2
FA	implied	    1	    2
80	immediate	2	    2
82	immediate	2	    2
89	immediate	2	    2
C2	immediate	2	    2
E2	immediate	2	    2
04	zeropage	2	    3
44	zeropage	2	    3
64	zeropage	2	    3
14	zeropage,X	2	    4
34	zeropage,X	2	    4
54	zeropage,X	2	    4
74	zeropage,X	2	    4
D4	zeropage,X	2	    4
F4	zeropage,X	2	    4
0C	absolute	3	    4
1C	absolute,X	3	    4*
3C	absolute,X	3	    4*
5C	absolute,X	3	    4*
7C	absolute,X	3	    4*
DC	absolute,X	3	    4*
FC	absolute,X	3	    4*
*/
void NOP()
{
    lookup_table[c6502.opcode].address_mode();

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
ORA() OR Memory with Accumulator

A OR M -> A
N	Z	C	I	D	V
+	+	-	-	-	-
addressing	    assembler	    opc	bytes	cycles
immediate	    ORA #oper	    09	2	    2
zeropage	    ORA oper	    05	2	    3
zeropage,X	    ORA oper,X	    15	2	    4
absolute	    ORA oper	    0D	3	    4
absolute,X	    ORA oper,X	    1D	3	    4*
absolute,Y	    ORA oper,Y	    19	3	    4*
(indirect,X)	ORA (oper,X)	01	2	    6
(indirect),Y	ORA (oper),Y	11	2	    5*
*/
void ORA()
{

    lookup_table[c6502.opcode].address_mode();

    uint8_t temp = cpu_read(c6502.abs_address);

    c6502.A |= temp;

    c6502_set_status_flag(N, c6502.A & 0x80);
    c6502_set_status_flag(Z, c6502.A == 0x00);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
PHA() Push Accumulator on Stack

push A
N	Z	C	I	D	V
-	-	-	-	-	-
addressing	assembler	opc	bytes	cycles
implied	    PHA	        48	1	    3
*/
void PHA()
{
    cpu_write(c6502_sp_abs(c6502.SP--), c6502.A);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
PHP() Push Processor Status on Stack

The status register will be pushed with bit 4 (B)
and bit 5 (U) flag set to 1.

push SR
N	Z	C	I	D	V
-	-	-	-	-	-
addressing	assembler	opc	bytes	cycles
implied	    PHP	        08	1	    3
*/
void PHP()
{

    cpu_write(c6502_sp_abs(c6502.SP--), c6502.SR | B | U);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
PLA() Pull Accumulator from Stack

pull A
N	Z	C	I	D	V
+	+	-	-	-	-
addressing	assembler	opc	bytes	cycles
implied	    PLA	        68	1	    4
*/
void PLA()
{
    c6502.A = cpu_read(c6502_sp_abs(++c6502.SP));

    c6502_set_status_flag(N, c6502.A & 0x80);
    c6502_set_status_flag(Z, c6502.A == 0x00);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
PLP() Pull Processor Status from Stack

The status register will be pulled with the break
flag and bit 5 ignored.

pull SR from stack

N	Z	C	I	D	V

addressing	assembler	opc	bytes	cycles
implied	    PLP	        28	1	    4
*/
void PLP()
{

    // keep current SR bit 4 & 5
    uint8_t break_flag = c6502_get_flag(B);  // Bit 4
    uint8_t unused_flag = c6502_get_flag(U); // Bit 5

    // Pull SR from stack
    c6502.SR = cpu_read(c6502_sp_abs(++c6502.SP));

    // Restore the SR bit 4 & 5 state before pulling SR from the stack
    c6502_set_status_flag(B, break_flag);
    c6502_set_status_flag(U, unused_flag);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
RLA() *illegal opcode*
ROL oper + AND oper

M = C <- [76543210] <- C, A AND M -> A

N	Z	C	I	D	V
+	+	+	-	-	-
addressing	    assembler	    opc	bytes	cycles
zeropage	    RLA oper	    27	2	    5
zeropage,X	    RLA oper,X	    37	2	    6
absolute	    RLA oper	    2F	3	    6
absolute,X	    RLA oper,X	    3F	3	    7
absolute,Y	    RLA oper,Y	    3B	3	    7
(indirect,X)	RLA (oper,X)	23	2	    8
(indirect),Y	RLA (oper),Y	33	2	    8
*/
void RLA()
{
    lookup_table[c6502.opcode].address_mode();

    uint16_t temp = cpu_read(c6502.abs_address);

    // ROL()
    temp = (temp << 1) | c6502_get_flag(C);
    cpu_write(c6502.abs_address, temp & 0x00FF);
    c6502_set_status_flag(N, temp & 0x0080);
    c6502_set_status_flag(Z, (temp & 0x00FF) == 0x0000);
    c6502_set_status_flag(C, temp & 0xFF00);

    // AND()
    c6502.A = (c6502.A & (temp & 0x00FF));
    c6502_set_status_flag(N, c6502.A & 0x80);
    c6502_set_status_flag(Z, c6502.A == 0);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
ROL() Rotate One Bit Left (Memory or Accumulator)

C <- [76543210] <- C
N	Z	C	I	D	V
+	+	+	-	-	-
addressing	assembler	opc	bytes	cycles
accumulator	ROL A	    2A	1	    2
zeropage	ROL oper	26	2	    5
zeropage,X	ROL oper,X	36	2	    6
absolute	ROL oper	2E	3	    6
absolute,X	ROL oper,X	3E	3	    7
*/
void ROL()
{
    lookup_table[c6502.opcode].address_mode();

    uint16_t temp = 0;

    if (lookup_table[c6502.opcode].address_mode == A)
    {
        // Shift Accumulator value to the left and shift old carry flag value into LSB
        temp = (c6502.A << 1) | c6502_get_flag(C);

        c6502.A = temp & 0x00FF;
    }
    else
    {
        temp = cpu_read(c6502.abs_address);
        // Shift Memory value to the left and shift old carry flag value into LSB
        temp = (temp << 1) | c6502_get_flag(C);

        cpu_write(c6502.abs_address, temp & 0x00FF);
    }

    c6502_set_status_flag(N, temp & 0x0080);
    c6502_set_status_flag(Z, (temp & 0x00FF) == 0x0000);
    // Old MSB is shift into the carry flag.
    c6502_set_status_flag(C, temp & 0xFF00);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
ROR() Rotate One Bit Right (Memory or Accumulator)

C -> [76543210] -> C
N	Z	C	I	D	V
+	+	+	-	-	-
addressing	assembler	opc	bytes	cycles
accumulator	ROR A	    6A	1	    2
zeropage	ROR oper	66	2	    5
zeropage,X	ROR oper,X	76	2	    6
absolute	ROR oper	6E	3	    6
absolute,X	ROR oper,X	7E	3	    7
*/
void ROR()
{
    lookup_table[c6502.opcode].address_mode();

    uint16_t temp = 0;
    uint16_t result = 0;
    if (lookup_table[c6502.opcode].address_mode == A)
    {
        temp = c6502.A & 0x00FF;
        // Shift Accumulator value to the right and shift old carry flag value into MSB
        result = (temp >> 1) | c6502_get_flag(C) << 7;

        c6502.A = result & 0x00FF;
    }
    else
    {
        temp = cpu_read(c6502.abs_address);
        // Shift Memory value to the right and shift old carry flag value into MSB
        result = (temp >> 1) | c6502_get_flag(C) << 7;

        cpu_write(c6502.abs_address, result);
    }

    c6502_set_status_flag(N, result & 0x0080);
    c6502_set_status_flag(Z, result == 0x0000);
    // Old LSB is shift into the carry flag.
    c6502_set_status_flag(C, temp | 0x0001);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
RRA() *illegal opcode*
ROR oper + ADC oper

M = C -> [76543210] -> C, A + M + C -> A, C

N	Z	C	I	D	V
+	+	+	-	-	+
addressing	    assembler	    opc	bytes	cycles
zeropage	    RRA oper	    67	2	    5
zeropage,X	    RRA oper,X	    77	2	    6
absolute	    RRA oper	    6F	3	    6
absolute,X	    RRA oper,X	    7F	3	    7
absolute,Y	    RRA oper,Y	    7B	3	    7
(indirect,X)	RRA (oper,X)	63	2	    8
(indirect),Y	RRA (oper),Y	73	2	    8
*/
void RRA()
{
    lookup_table[c6502.opcode].address_mode();

    uint16_t temp = 0;
    uint16_t result = 0;
    temp = cpu_read(c6502.abs_address);

    // ROR()
    result = (temp >> 1) | c6502_get_flag(C) << 7;

    cpu_write(c6502.abs_address, result);

    c6502_set_status_flag(N, result & 0x0080);
    c6502_set_status_flag(Z, result == 0x0000);
    c6502_set_status_flag(C, temp | 0x0001);

    // ADC()
    uint16_t augend = (uint16_t)c6502.A;
    uint16_t addend = result;
    uint16_t sum = augend + addend + c6502_get_flag(C);

    c6502.A = sum & 0x00FF;

    c6502_set_status_flag(N, c6502.A & 0x80);
    c6502_set_status_flag(Z, c6502.A == 0);
    c6502_set_status_flag(C, sum & 0xFF00);
    c6502_set_status_flag(V, (~(augend ^ addend)) & (augend ^ sum) & 0x80);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
RTI() Return from Interrupt

The status register is pulled with the break flag
and bit 5 ignored. Then PC is pulled from the stack.

pull SR, pull PC
N	Z	C	I	D	V
from stack
addressing	assembler	opc	bytes	cycles
implied	    RTI	        40	1	    6
*/
void RTI()
{
    // Keep current SR bit 4 and 5
    uint8_t break_flag = c6502_get_flag(B);  // bit 4
    uint8_t unused_flag = c6502_get_flag(U); // bit 5

    // Pull SR from the stack
    c6502.SR = cpu_read(c6502_sp_abs(++c6502.SP));

    // Restore SR bit 4 and 5 prior to pulling SR from the stack
    c6502_set_status_flag(B, break_flag);
    c6502_set_status_flag(U, unused_flag);

    uint16_t LSB = (uint16_t)cpu_read(c6502_sp_abs(++c6502.SP));
    uint16_t MSB = (uint16_t)cpu_read(c6502_sp_abs(++c6502.SP));

    // Set program counter address to what pull from the stack
    c6502.PC = (MSB << 8) | LSB;

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
RTS() Return from Subroutine

pull PC, PC+1 -> PC
N	Z	C	I	D	V
-	-	-	-	-	-
addressing	assembler	opc	bytes	cycles
implied	    RTS	        60	1	    6
*/
void RTS()
{
    uint16_t LSB = (uint16_t)cpu_read(c6502_sp_abs(++c6502.SP));
    uint16_t MSB = (uint16_t)cpu_read(c6502_sp_abs(++c6502.SP));
    c6502.PC = (MSB << 8) | LSB;
    c6502.PC++;

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
SAX() (AXS, AAX) *illegal opcode*
A and X are put on the bus at the same time (resulting effectively in an AND operation) and stored in M

A AND X -> M

N	Z	C	I	D	V
-	-	-	-	-	-
addressing	    assembler	    opc	bytes	cycles
zeropage	    SAX oper	    87	2	    3
zeropage,Y	    SAX oper,Y	    97	2	    4
absolute	    SAX oper	    8F	3	    4
(indirect,X)	SAX (oper,X)	83	2	    6
*/
void SAX()
{
    lookup_table[c6502.opcode].address_mode();

    uint8_t results = c6502.A & c6502.X;

    cpu_write(c6502.abs_address, results);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
SBC() Subtract Memory from Accumulator with Borrow

A - M - C̅ -> A
N	Z	C	I	D	V
+	+	+	-	-	+
addressing	    assembler	    opc	bytes	cycles
immediate	    SBC #oper	    E9	2	    2
immediate	    SBC #oper	    EB	2	    2           *Illegal opcode EB, SBC oper + NOP*
zeropage	    SBC oper	    E5	2	    3
zeropage,X	    SBC oper,X	    F5	2	    4
absolute	    SBC oper	    ED	3	    4
absolute,X	    SBC oper,X	    FD	3	    4*
absolute,Y	    SBC oper,Y	    F9	3	    4*
(indirect,X)	SBC (oper,X)	E1	2	    6
(indirect),Y	SBC (oper),Y	F1	2	    5*
*/
void SBC()
{
    lookup_table[c6502.opcode].address_mode();

    uint16_t minuend = (uint16_t)c6502.A;
    uint16_t subtrahend = (uint16_t)cpu_read(c6502.abs_address);
    // Find the one's complement of the subtrahend by flipping the bits.
    uint16_t ones_complement = subtrahend ^ 0x00FF;
    // Find the two's complement of the subtrahend by adding the carry bit.
    uint16_t twos_complement = ones_complement + c6502_get_flag(C);
    // Add the two's complement to minuend
    uint16_t difference = minuend + twos_complement;

    c6502.A = difference & 0x00FF;

    c6502_set_status_flag(N, c6502.A & 0x80);
    c6502_set_status_flag(Z, c6502.A == 0);
    c6502_set_status_flag(C, difference & 0xFF00);

    c6502_set_status_flag(V, (~(minuend ^ twos_complement) & (minuend ^ difference) & 0x80));

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
SEC() Set Carry Flag

1 -> C
N	Z	C	I	D	V
-	-	1	-	-	-
addressing	assembler	opc	bytes	cycles
implied	    SEC	        38	1	    2
*/
void SEC()
{
    c6502_set_status_flag(C, true);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
SED() Set Decimal Flag

1 -> D
N	Z	C	I	D	V
-	-	-	-	1	-
addressing	assembler	opc	bytes	cycles
implied	    SED	        F8	1	    2
*/
void SED()
{
    c6502_set_status_flag(D, true);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
SEI() Set Interrupt Disable Status

1 -> I
N	Z	C	I	D	V
-	-	-	1	-	-
addressing	assembler	opc	bytes	cycles
implied	    SEI	        78	1	    2
*/
void SEI()
{
    c6502_set_status_flag(I, true);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
SLO (ASO) *illegal opcode*
ASL oper + ORA oper

M = C <- [76543210] <- 0, A OR M -> A

N	Z	C	I	D	V
+	+	+	-	-	-
addressing	    assembler	        opc	bytes	cycles
zeropage	    SLO oper	        07	2	    5
zeropage,X	    SLO oper,X	        17	2	    6
absolute	    SLO oper	        0F	3	    6
absolute,X	    SLO oper,X	        1F	3	    7
absolute,Y	    SLO oper,Y	        1B	3	    7
(indirect,X)	SLO (oper,X)	    03	2	    8
(indirect),Y	SLO (oper),Y	    13	2	    8
*/
void SLO()
{
    lookup_table[c6502.opcode].address_mode();

    // ASL()
    uint16_t temp = 0;
    temp = cpu_read(c6502.abs_address);
    temp = temp << 1;
    cpu_write(c6502.abs_address, temp & 0x00FF);
    c6502_set_status_flag(N, temp & 0x80);
    c6502_set_status_flag(Z, (temp & 0x00FF) == 0x00);
    c6502_set_status_flag(C, (temp & 0xFF00) > 0);

    // ORA()
    c6502.A |= temp;
    c6502_set_status_flag(N, c6502.A & 0x80);
    c6502_set_status_flag(Z, c6502.A == 0x00);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
SRE (LSE) *illegal opcode*
LSR oper + EOR oper

M = 0 -> [76543210] -> C, A EOR M -> A

N	Z	C	I	D	V
+	+	+	-	-	-
addressing	    assembler	    opc	bytes	cycles
zeropage	    SRE oper	    47	2	    5
zeropage,X	    SRE oper,X	    57	2	    6
absolute	    SRE oper	    4F	3	    6
absolute,X	    SRE oper,X	    5F	3	    7
absolute,Y	    SRE oper,Y	    5B	3	    7
(indirect,X)	SRE (oper,X)	43	2	    8
(indirect),Y	SRE (oper),Y	53	2	    8
*/
void SRE()
{
    lookup_table[c6502.opcode].address_mode();

    // LSR()
    uint8_t data = cpu_read(c6502.abs_address);
    uint8_t temp = data >> 1;
    cpu_write(c6502.abs_address, temp);
    c6502_set_status_flag(N, temp & 0x80);
    c6502_set_status_flag(Z, temp == 0x00);
    c6502_set_status_flag(C, data & 0x01);

    // EOR()
    c6502.A ^= temp;
    c6502_set_status_flag(N, c6502.A & 0x80);
    c6502_set_status_flag(Z, c6502.A == 0x00);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
STA() Store Accumulator in Memory

A -> M
N	Z	C	I	D	V
-	-	-	-	-	-
addressing	    assembler	    opc	bytes	cycles
zeropage	    STA oper	    85	2	    3
zeropage,X	    STA oper,X	    95	2	    4
absolute	    STA oper	    8D	3	    4
absolute,X	    STA oper,X	    9D	3	    5
absolute,Y	    STA oper,Y	    99	3	    5
(indirect,X)	STA (oper,X)	81	2	    6
(indirect),Y	STA (oper),Y	91	2	    6
*/
void STA()
{
    lookup_table[c6502.opcode].address_mode();

    cpu_write(c6502.abs_address, c6502.A);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
STX() Store Index X in Memory

X -> M
N	Z	C	I	D	V
-	-	-	-	-	-
addressing	assembler	opc	bytes	cycles
zeropage	STX oper	86	2	    3
zeropage,Y	STX oper,Y	96	2	    4
absolute	STX oper	8E	3	    4
*/
void STX()
{
    lookup_table[c6502.opcode].address_mode();

    cpu_write(c6502.abs_address, c6502.X);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
STY() Store Index Y in Memory

Y -> M
N	Z	C	I	D	V
-	-	-	-	-	-
addressing	assembler	opc	bytes	cycles
zeropage	STY oper	84	2	    3
zeropage,X	STY oper,X	94	2	    4
absolute	STY oper	8C	3	    4
*/
void STY()
{
    lookup_table[c6502.opcode].address_mode();

    cpu_write(c6502.abs_address, c6502.Y);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
TAX() Transfer Accumulator to Index X

A -> X
N	Z	C	I	D	V
+	+	-	-	-	-
addressing	assembler	opc	bytes	cycles
implied	    TAX	        AA	1	    2
*/
void TAX()
{
    c6502.X = c6502.A;

    c6502_set_status_flag(N, c6502.X & 0x80);
    c6502_set_status_flag(Z, c6502.X == 0);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
TAY() Transfer Accumulator to Index Y

A -> Y
N	Z	C	I	D	V
+	+	-	-	-	-
addressing	assembler	opc	bytes	cycles
implied	    TAY	        A8	1	    2
*/
void TAY()
{
    c6502.Y = c6502.A;

    c6502_set_status_flag(N, c6502.Y & 0x80);
    c6502_set_status_flag(Z, c6502.Y == 0);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
TSX() Transfer Stack Pointer to Index X

SP -> X
N	Z	C	I	D	V
+	+	-	-	-	-
addressing	assembler	opc	bytes	cycles
implied	    TSX	        BA	1	    2
*/
void TSX()
{
    c6502.X = c6502.SP;

    c6502_set_status_flag(N, c6502.X & 0x80);
    c6502_set_status_flag(Z, c6502.X == 0);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
TXA() Transfer Index X to Accumulator

X -> A
N	Z	C	I	D	V
+	+	-	-	-	-
addressing	assembler	opc	bytes	cycles
implied	    TXA	        8A	1	    2
*/
void TXA()
{
    c6502.A = c6502.X;

    c6502_set_status_flag(N, c6502.A & 0x80);
    c6502_set_status_flag(Z, c6502.A == 0);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
TXS() Transfer Index X to Stack Register

X -> SP
N	Z	C	I	D	V
-	-	-	-	-	-
addressing	assembler	opc	bytes	cycles
implied	    TXS	        9A	1	    2
*/
void TXS()
{
    c6502.SP = c6502.X;

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
TYA() Transfer Index Y to Accumulator

Y -> A
N	Z	C	I	D	V
+	+	-	-	-	-
addressing	assembler	opc	bytes	cycles
implied	    TYA	        98	1	    2
*/
void TYA()
{
    c6502.A = c6502.Y;

    c6502_set_status_flag(N, c6502.A & 0x80);
    c6502_set_status_flag(Z, c6502.A == 0);

    c6502.cycles += lookup_table[c6502.opcode].cycles;
}

/*
Address Mode:

Add an extra cycle if page boundary is cross for the following opcode.

addressing	    assembler	    opc bytes   cycles
absolute,X	    ADC oper,X	    7D	3	    4*
absolute,X	    AND oper,X	    3D	3	    4*
absolute,X	    EOR oper,X	    5D	3	    4*
absolute,X	    CMP oper,X	    DD	3	    4*
absolute,X	    LDA oper,X	    BD	3	    4*
absolute,X	    LDY oper,X	    BC	3	    4*
absolute,X	    ORA oper,X	    1D	3	    4*
absolute,X	    SBC oper,X	    FD	3	    4*
absolute,Y	    ADC oper,Y	    79	3	    4*
absolute,Y	    AND oper,Y	    39	3	    4*
absolute,Y	    CMP oper,Y	    D9	3	    4*
absolute,Y	    EOR oper,Y	    59	3	    4*
absolute,Y	    LDA oper,Y	    B9	3	    4*
absolute,Y	    LDX oper,Y	    BE	3	    4*
absolute,Y	    ORA oper,Y	    19	3	    4*
absolute,Y	    SBC oper,Y	    F9	3	    4*
(indirect),Y	ADC (oper),Y	71	2	    5*
(indirect),Y	AND (oper),Y	31	2	    5*
(indirect),Y	CMP (oper),Y	D1	2	    5*
(indirect),Y	EOR (oper),Y	51	2	    5*
(indirect),Y	LDA (oper),Y	B1	  	    5*
(indirect),Y	ORA (oper),Y	11	2	    5*
(indirect),Y	SBC (oper),Y	F1	2	    5*
*/

// Address Mode: Accumulator
void A()
{
}

// Address Mode: Absolute
void ABS()
{
    uint16_t LSB = (uint16_t)cpu_read(c6502.PC++);
    uint16_t MSB = (uint16_t)cpu_read(c6502.PC++);
    c6502.abs_address = (MSB << 8) | LSB;
}

/*
Address Mode: Absolute X-indexed

Add a cycle if page boundary is cross for the following opcodes:
addressing	    assembler	    opc bytes   cycles
absolute,X	    ADC oper,X	    7D	3	    4*
absolute,X	    AND oper,X	    3D	 3	    4*
absolute,X	    EOR oper,X	    5D	3	    4*
absolute,X	    CMP oper,X	    DD	3	    4*
absolute,X	    LDA oper,X	    BD	3	    4*
absolute,X	    LDY oper,X	    BC	3	    4*
absolute,X	    ORA oper,X	    1D	3	    4*
absolute,X	    SBC oper,X	    FD	3	    4*
absolute,X	    NOP     	    1C	3	    4*
absolute,X	    NOP     	    3C	3	    4*
absolute,X	    NOP     	    5C	3	    4*
absolute,X	    NOP     	    7C	3	    4*
absolute,X	    NOP     	    DC	3	    4*
absolute,X	    NOP     	    FC	3	    4*
*/
void ABS_X()
{
    uint16_t LSB = (uint16_t)cpu_read(c6502.PC++);
    uint16_t MSB = (uint16_t)cpu_read(c6502.PC++);
    c6502.abs_address = ((MSB << 8) | LSB) + c6502.X;
    if (((c6502.abs_address & 0xFF00) != (MSB << 8)))
    {
        // ADD a cycle if Address mode is one of the following:
        if ((lookup_table[c6502.opcode].run == ADC) |
            (lookup_table[c6502.opcode].run == AND) |
            (lookup_table[c6502.opcode].run == EOR) |
            (lookup_table[c6502.opcode].run == CMP) |
            (lookup_table[c6502.opcode].run == LDA) |
            (lookup_table[c6502.opcode].run == LDY) |
            (lookup_table[c6502.opcode].run == ORA) |
            (lookup_table[c6502.opcode].run == SBC) |
            (lookup_table[c6502.opcode].run == NOP))
        {
            c6502.cycles++;
        }
    }
}

/*
Address Mode: Absolute Y-Indexed

Add a cycle if page boundary is cross for the following opcodes:
addressing	    assembler	    opc bytes   cycles
absolute,Y	    ADC oper,Y	    79	3	    4*
absolute,Y	    AND oper,Y	    39	3	    4*
absolute,Y	    CMP oper,Y	    D9	3	    4*
absolute,Y	    EOR oper,Y	    59	3	    4*
absolute,Y	    LDA oper,Y	    B9	3	    4*
absolute,Y	    LDX oper,Y	    BE	3	    4*
absolute,Y	    ORA oper,Y	    19	3	    4*
absolute,Y	    SBC oper,Y	    F9	3	    4*
*/
void ABS_Y()
{
    uint16_t LSB = (uint16_t)cpu_read(c6502.PC++);
    uint16_t MSB = (uint16_t)cpu_read(c6502.PC++);
    c6502.abs_address = ((MSB << 8) | LSB) + c6502.Y;
    if ((c6502.abs_address & 0xFF00) != (MSB << 8))
    {
        // ADD a cycle if Address mode is one of the following:
        if ((lookup_table[c6502.opcode].run == ADC) |
            (lookup_table[c6502.opcode].run == AND) |
            (lookup_table[c6502.opcode].run == CMP) |
            (lookup_table[c6502.opcode].run == EOR) |
            (lookup_table[c6502.opcode].run == LAX) |
            (lookup_table[c6502.opcode].run == LDA) |
            (lookup_table[c6502.opcode].run == LDX) |
            (lookup_table[c6502.opcode].run == ORA) |
            (lookup_table[c6502.opcode].run == SBC))
        {
            c6502.cycles++;
        }
    }
}

// Address Mode: Immediate
void IMMED()
{
    c6502.abs_address = c6502.PC++;
}

// Address Mode: Implied
void IMPL() {}

// Address Mode: Indirect
void IND()
{
    uint16_t LSB = (uint16_t)cpu_read(c6502.PC++);
    uint16_t MSB = (uint16_t)cpu_read(c6502.PC++);

    /*
    The original 6502 does not fetch the target address correctly in an indirect JMP() if the target address falls on a page boundary ($xxFF).
    In this case the LSB address is fetched as expected but the MSB is taken from $xx00 instead of $xxFF+1.
    So we need to replicate this for our nes test rom to pass its indirect JMP() test.
     */
    uint16_t ptr_address = (MSB << 8) | LSB;
    if (LSB == 0x00FF)
    {
        c6502.abs_address = cpu_read(ptr_address & 0xFF00) << 8 | cpu_read(ptr_address);
    }
    else
    {
        c6502.abs_address = cpu_read(ptr_address + 1) << 8 | cpu_read(ptr_address);
    }
}

// Address Mode: Indirect X-indexed
void IND_X()
{
    uint16_t temp = (uint16_t)cpu_read(c6502.PC++);
    uint16_t LSB = (uint16_t)cpu_read((uint16_t)(temp + (uint16_t)c6502.X) & 0x00FF);
    uint16_t MSB = (uint16_t)cpu_read((uint16_t)(temp + 1 + (uint16_t)c6502.X) & 0x00FF);
    c6502.abs_address = (MSB << 8) | LSB;
}

/*
Address Mode: Indirect Y-indexed
(indirect),Y	ADC (oper),Y	71	2	    5*
(indirect),Y	AND (oper),Y	31	2	    5*
(indirect),Y	CMP (oper),Y	D1	2	    5*
(indirect),Y	EOR (oper),Y	51	2	    5*
(indirect),Y	LDA (oper),Y	B1	  	    5*
(indirect),Y	ORA (oper),Y	11	2	    5*
(indirect),Y	SBC (oper),Y	F1	2	    5*
*/
void IND_Y()
{
    uint16_t temp = (uint16_t)cpu_read(c6502.PC++);
    uint16_t LSB = (uint16_t)cpu_read((uint16_t)(temp) & 0x00FF);
    uint16_t MSB = (uint16_t)cpu_read((uint16_t)(temp + 1) & 0x00FF);
    c6502.abs_address = ((MSB << 8) | LSB) + c6502.Y;
    if ((c6502.abs_address & 0xFF00) != (MSB << 8))
    {
        // ADD a cycle if Address mode is one of the following:
        if ((lookup_table[c6502.opcode].run == ADC) |
            (lookup_table[c6502.opcode].run == AND) |
            (lookup_table[c6502.opcode].run == CMP) |
            (lookup_table[c6502.opcode].run == EOR) |
            (lookup_table[c6502.opcode].run == LAX) |
            (lookup_table[c6502.opcode].run == LDA) |
            (lookup_table[c6502.opcode].run == ORA) |
            (lookup_table[c6502.opcode].run == SBC))
        {
            c6502.cycles++;
        }
    }
}

// Address Mode: Relative
void REL()
{

    uint8_t temp = cpu_read(c6502.PC++);
    c6502.rel_address = temp & 0x00FF;
    if (temp & 0x80)
    {
        c6502.rel_address = temp | 0xFF00;
    }
}

// Address Mode: Zero Page
void ZPG()
{
    c6502.abs_address = cpu_read(c6502.PC++);
    c6502.abs_address &= 0x00FF;
}

// Address Mode: Zero Page X-indexed
void ZPG_X()
{
    c6502.abs_address = cpu_read(c6502.PC++) + c6502.X;

    c6502.abs_address &= 0x00FF;
}

// Address Mode: Zero Page Y-indexed
void ZPG_Y()
{
    c6502.abs_address = cpu_read(c6502.PC++) + c6502.Y;
    c6502.abs_address &= 0x00FF;
}

// Address Mode: None. Used as a placeholder for illegal opcode not implemented yet.
void NONE()
{
}
