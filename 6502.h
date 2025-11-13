// c6502.h

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "bus.h"

// 6502 status flags
typedef enum
{
    C = 0b00000001, // Carry
    Z = 0b00000010, // Zero
    I = 0b00000100, // Interrupt Disable
    D = 0b00001000, // Decimal
    B = 0b00010000, // Break
    U = 0b00100000, // Unused
    V = 0b01000000, // Overflow
    N = 0b10000000, // Negative
} c6502_status_flags;

// Struct definition for 6502 cpu state and register
typedef struct
{
    uint8_t A;            // Accumulator register
    uint16_t PC;          // Program counter
    uint8_t SR;           // Status Register
    uint8_t SP;           // Stack pointer 0x0100-0x01FF
    uint8_t X;            // X register
    uint8_t Y;            // Y register
    uint16_t cycles;      // Variable to keep track of cpu cycles
    uint16_t abs_address; // Variable to keep track of absolute address
    uint16_t rel_address; // Variable to keep track of relative address
    uint8_t opcode;       // Variable to keep track of cpu opcode fetched. Using the fetch_opcode() function
    uint8_t reset_pin;    // Reset pin active low
    uint8_t JAM;          // JAM cpu flag;
} c6502_cpu;

// Struct used to manipulate state of 6502 computer
c6502_cpu c6502;

// Struct used for lookup table
typedef struct
{
    char *name;                 // Name for OPCODE
    void (*run)(void);          // OPCODE function pointer
    void (*address_mode)(void); // Address mode function pointer
    uint8_t cycles;             // number of cycles for each instruction
} c6502_instruction;

/*
Initialize 6502 processor to boot up state.
Program argument: 6502 program start address MSB and LSB.
*/
void c6502_init(uint8_t PC_MSB, uint8_t PC_LSB);

// fetch opcode.
void c6502_read_opcode();

// set 6502 status flag
void c6502_set_status_flag(c6502_status_flags flag, bool x);

// return 1 if flag is set else return 0.
uint8_t c6502_get_flag(c6502_status_flags flag);

/*
Return stack pointer absolute address
6502 support for a stack implemented using 256byte whose location is hardcoded at page $01 (0x0100-0x01FF)
*/
uint16_t c6502_sp_abs(uint8_t SP);

/*
c6502_irq() Interrupt
Push PC MSB, PC LSB, and SR onto the stack. Set interrupt disable flag. Unset Break flag.
Load vector (0xFFFE/0xFFFF) to Program Counter.
Instruction take 7 clock cycles
*/
void c6502_irq();

/*
c6502_nmi() Non-maskable interrupt
Push PC MSB, PC LSB, and SR onto the stack. Set interrupt disable flag. Unset Break flag.
Load vector (0xFFFA/0xFFFB) to Program Counter.
Instruction take 7 clock cycles
*/
void c6502_nmi();

/*
c6502_reset() Reset 6502 cpu.
Load vector (0xFFFC/0xFFFD) to Program Counter.
Clear all register and status flag.
Set interrupt disable flag.
Instruction takes 7 clock cycles
*/
void c6502_reset();

/*-------
Opcode
--------*/

void UNK(); // Unknown opcode (Place holder for illegal opcode)
void JAM(); // JAM (KILL, HLT) *illegal opcode*
void ADC(); // Add with carry
void AND(); // And (with accumulator)
void ASL(); // Arithmetic shift left
void BCC(); // Branch on carry clear
void BCS(); // Branch on carry set
void BEQ(); // Branch on equal (zero set)
void BIT(); // Bit test
void BMI(); // Branch on minus (negative set)
void BNE(); // Branch on not equal (zero clear)
void BPL(); // Branch on plus (negative clear)
void BRK(); // Break / interrupt
void BVC(); // Branch on overflow clear
void BVS(); // Branch on overflow set
void CLC(); // Clear carry
void CLD(); // Clear decimal
void CLI(); // Clear interrupt disable
void CLV(); // Clear overflow
void CMP(); // Compare (with accumulator)
void CPX(); // Compare with X
void CPY(); // Compare with Y
void DCP(); // DEC oper + CMP oper *illegal opcode*
void DEC(); // Decrement
void DEX(); // Decrement X
void DEY(); // Decrement Y
void EOR(); // Exclusive or (with accumulator)
void INC(); // Increment
void INX(); // Increment X
void INY(); // Increment Y
void ISB(); // Increment oper + SBC oper *illegal opcode*
void JMP(); // Jump
void JSR(); // Jump subroutine
void LAX(); // Load A + Load X *illegal opcode*
void LDA(); // Load accumulator
void LDX(); // Load X
void LDY(); // Load Y
void LSR(); // Logical shift right
void NOP(); // No operation
void ORA(); // Or with accumulator
void PHA(); // Push accumulator
void PHP(); // Push processor status (SR)
void PLA(); // Pull accumulator
void PLP(); // Pull processor status (SR)
void RLA(); // ROL oper + AND oper *illegal opcode*
void ROL(); // Rotate left
void ROR(); // Rotate right
void RRA(); // ROR oper + ADC oper *illegal opcode*
void RTI(); // Return from interrupt
void RTS(); // Return from subroutine
void SAX(); // A & X, store in Memory *illegal opcode*
void SBC(); // Subtract with carry
void SEC(); // Set carry
void SED(); // Set decimal
void SEI(); // Set interrupt disable
void SLO(); // ASL oper + ORA oper *illegal opcode*
void SRE(); // LSR oper + EOR oper *illegal opcode*
void STA(); // Store accumulator
void STX(); // Store X
void STY(); // Store Y
void TAX(); // Transfer accumulator to X
void TAY(); // Transfer accumulator to Y
void TSX(); // Transfer stack pointer to X
void TXA(); // Transfer X to accumulator
void TXS(); // Transfer X to stack pointer
void TYA(); // Transfer Y to accumulator

/*-----------
Address Mode
------------*/

void A();     // Accumulator
void ABS();   // Absolute
void ABS_X(); // Absolute X-indexed
void ABS_Y(); // Absolute Y-Indexed
void IMMED(); // Immediate
void IMPL();  // Implied
void IND();   // Indirect
void IND_X(); // Indirect X-indexed
void IND_Y(); // Indirect Y-indexed
void REL();   // Relative
void ZPG();   // Zero Page
void ZPG_X(); // Zero Page X-indexed
void ZPG_Y(); // Zero Page Y-indexed
void NONE();  // None

/*------------------------------------------------------------------------------------
6502 instruction lookup table using opcode as the key:
Instuction name, Function Pointer, Address Mode, and Number of Cycles:

Note: Opcode names listed below with an * in front of them are illegal opcodes implemented.
      Opcode UNK is a placeholder for illegal opcodes yet to be implemented.
----------------------------------------------------------------------------------------*/
static c6502_instruction lookup_table[256] = {
    // 0x00
    {"BRK", &BRK, &IMPL, 7},
    // 0x01
    {"ORA", &ORA, &IND_X, 6},
    // 0x02
    {"*JAM", &JAM, &NONE, -1},
    // 0x03
    {"*SLO", &SLO, &IND_X, 8},
    // 0x04
    {"*NOP", &NOP, &ZPG, 3},
    // 0x05
    {"ORA", &ORA, &ZPG, 3},
    // 0x06
    {"ASL", &ASL, &ZPG, 5},
    // 0x07
    {"*SLO", &SLO, &ZPG, 5},
    // 0x08
    {"PHP", &PHP, &IMPL, 3},
    // 0x09
    {"ORA", &ORA, &IMMED, 2},
    // 0x0A
    {"ASL", &ASL, &A, 2},
    // 0x0B
    {"UNK", &UNK, &NONE, -1},
    // 0x0C
    {"*NOP", &NOP, &ABS, 4},
    // 0x0D
    {"ORA", &ORA, &ABS, 4},
    // 0x0E
    {"ASL", &ASL, &ABS, 6},
    // 0x0F
    {"*SLO", &SLO, &ABS, 6},
    // 0x10
    {"BPL", &BPL, &REL, 2},
    // 0x11
    {"ORA", &ORA, &IND_Y, 5},
    // 0x12
    {"*JAM", &JAM, &NONE, -1},
    // 0x13
    {"*SLO", &SLO, &IND_Y, 8},
    // 0x14
    {"*NOP", &NOP, &ZPG_X, 4},
    // 0x15
    {"ORA", &ORA, &ZPG_X, 4},
    // 0x16
    {"ASL", &ASL, &ZPG_X, 6},
    // 0x17
    {"*SLO", &SLO, &ZPG_X, 6},
    // 0x18
    {"CLC", &CLC, &IMPL, 2},
    // 0x19
    {"ORA", &ORA, &ABS_Y, 4},
    // 0x1A
    {"*NOP", &NOP, &IMPL, 2},
    // 0x1B
    {"*SLO", &SLO, &ABS_Y, 7},
    // 0x1C
    {"*NOP", &NOP, &ABS_X, 4},
    // 0x1D
    {"ORA", &ORA, &ABS_X, 4},
    // 0x1E
    {"ASL", &ASL, &ABS_X, 7},
    // 0x1F
    {"*SLO", &SLO, &ABS_X, 7},
    // 0x20
    {"JSR", &JSR, &ABS, 6},
    // 0x21
    {"AND", &AND, &IND_X, 6},
    // 0x22
    {"*JAM", &JAM, &NONE, -1},
    // 0x23
    {"*RLA", &RLA, &IND_X, 8},
    // 0x24
    {"BIT", &BIT, &ZPG, 3},
    // 0x25
    {"AND", &AND, &ZPG, 3},
    // 0x26
    {"ROL", &ROL, &ZPG, 5},
    // 0x27
    {"*RLA", &RLA, &ZPG, 5},
    // 0x28
    {"PLP", &PLP, &IMPL, 4},
    // 0x29
    {"AND", &AND, &IMMED, 2},
    // 0x2A
    {"ROL", &ROL, &A, 2},
    // 0x2B
    {"UNK", &UNK, &NONE, -1},
    // 0x2C
    {"BIT", &BIT, &ABS, 4},
    // 0x2D
    {"AND", &AND, &ABS, 4},
    // 0x2E
    {"ROL", &ROL, &ABS, 6},
    // 0x2F
    {"*RLA", &RLA, &ABS, 6},
    // 0x30
    {"BMI", &BMI, &REL, 2},
    // 0x31
    {"AND", &AND, &IND_Y, 5},
    // 0x32
    {"*JAM", &JAM, &NONE, -1},
    // 0X33
    {"*RLA", &RLA, &IND_Y, 8},
    // 0x34
    {"*NOP", &NOP, &ZPG_X, 4},
    // 0x35
    {"AND", &AND, &ZPG_X, 4},
    // 0x36
    {"ROL", &ROL, &ZPG_X, 6},
    // 0x37
    {"*RLA", &RLA, &ZPG_X, 6},
    // 0x38
    {"SEC", &SEC, &IMPL, 2},
    // 0x39
    {"AND", &AND, &ABS_Y, 4},
    // 0x3A
    {"*NOP", &NOP, &IMPL, 2},
    // 0x3B
    {"*RLA", &RLA, &ABS_Y, 7},
    // 0x3C
    {"*NOP", &NOP, &ABS_X, 4},
    // 0x3D
    {"AND", &AND, &ABS_X, 4},
    // 0x3E
    {"ROL", &ROL, &ABS_X, 7},
    // 0x3F
    {"*RLA", &RLA, &ABS_X, 7},
    // 0x40
    {"RTI", &RTI, &IMPL, 6},
    // 0x41
    {"EOR", &EOR, &IND_X, 6},
    // 0x42
    {"*JAM", &JAM, &NONE, -1},
    // 0x43
    {"*SRE", &SRE, &IND_X, 8},
    // 0x44
    {"*NOP", &NOP, &ZPG, 3},
    // 0x45
    {"EOR", &EOR, &ZPG, 3},
    // 0x46
    {"LSR", &LSR, &ZPG, 5},
    // 0x47
    {"*SRE", &SRE, &ZPG, 5},
    // 0x48
    {"PHA", &PHA, &IMPL, 3},
    // 0x49
    {"EOR", &EOR, &IMMED, 2},
    // 0x4A
    {"LSR", &LSR, &A, 2},
    // 0x4B
    {"UNK", &UNK, &NONE, -1},
    // 0x4C
    {"JMP", &JMP, &ABS, 3},
    // 0x4D
    {"EOR", &EOR, &ABS, 4},
    // 0x4E
    {"LSR", &LSR, &ABS, 6},
    // 0x4F
    {"*SRE", &SRE, &ABS, 6},
    // 0x50
    {"BVC", &BVC, &REL, 2},
    // 0x51
    {"EOR", &EOR, &IND_Y, 5},
    // 0x52
    {"*JAM", &JAM, &NONE, -1},
    // 0x53
    {"*SRE", &SRE, &IND_Y, 8},
    // 0x54
    {"*NOP", &NOP, &ZPG_X, 4},
    // 0x55
    {"EOR", &EOR, &ZPG_X, 4},
    // 0x56
    {"LSR", &LSR, &ZPG_X, 6},
    // 0x57
    {"*SRE", &SRE, &ZPG_X, 6},
    // 0x58
    {"CLI", &CLI, &IMPL, 2},
    // 0x59
    {"EOR", &EOR, &ABS_Y, 4},
    // 0x5A
    {"*NOP", &NOP, &IMPL, 2},
    // 0x5B
    {"*SRE", &SRE, &ABS_Y, 7},
    // 0x5C
    {"*NOP", &NOP, &ABS_X, 4},
    // 0x5D
    {"EOR", &EOR, &ABS_X, 4},
    // 0x5E
    {"LSR", &LSR, &ABS_X, 7},
    // 0x5F
    {"*SRE", &SRE, &ABS_X, 7},
    // 0x60
    {"RTS", &RTS, &IMPL, 6},
    // 0x61
    {"ADC", &ADC, &IND_X, 6},
    // 0x62
    {"*JAM", &JAM, &NONE, -1},
    // 0x63
    {"*RRA", &RRA, &IND_X, 8},
    // 0x64
    {"*NOP", &NOP, &ZPG, 3},
    // 0x65
    {"ADC", &ADC, &ZPG, 3},
    // 0x66
    {"ROR", &ROR, &ZPG, 5},
    // 0x67
    {"*RRA", &RRA, &ZPG, 5},
    // 0x68
    {"PLA", &PLA, &IMPL, 4},
    // 0x69
    {"ADC", &ADC, &IMMED, 2},
    // 0x6A
    {"ROR", &ROR, &A, 2},
    // 0x6B
    {"UNK", &UNK, &NONE, -1},
    // 0x6C
    {"JMP", &JMP, &IND, 5},
    // 0x6D
    {"ADC", &ADC, &ABS, 4},
    // 0x6E
    {"ROR", &ROR, &ABS, 6},
    // 0x6F
    {"*RRA", &RRA, &ABS, 6},
    // 0x70
    {"BVS", &BVS, &REL, 2},
    // 0x71
    {"ADC", &ADC, &IND_Y, 5},
    // 0x72
    {"*JAM", &JAM, &NONE, -1},
    // 0x73
    {"*RRA", &RRA, &IND_Y, 8},
    // 0x74
    {"*NOP", &NOP, &ZPG_X, 4},
    // 0x75
    {"ADC", &ADC, &ZPG_X, 4},
    // 0x76
    {"ROR", &ROR, &ZPG_X, 6},
    // 0x77
    {"*RRA", &RRA, &ZPG_X, 6},
    // 0x78
    {"SEI", &SEI, &IMPL, 2},
    // 0x79
    {"ADC", &ADC, &ABS_Y, 4},
    // 0x7A
    {"*NOP", &NOP, &IMPL, 2},
    // 0x7B
    {"*RRA", &RRA, &ABS_Y, 7},
    // 0x7C
    {"*NOP", &NOP, &ABS_X, 4},
    // 0x7D
    {"ADC", &ADC, &ABS_X, 4},
    // 0x7E
    {"ROR", &ROR, &ABS_X, 7},
    // 0x7F
    {"*RRA", &RRA, &ABS_X, 7},
    // 0x80
    {"*NOP", &NOP, &IMMED, 2},
    // 0x81
    {"STA", &STA, &IND_X, 6},
    // 0x82
    {"*NOP", &NOP, &IMMED, 2},
    // 0x83
    {"*SAX", &SAX, &IND_X, 6},
    // 0x84
    {"STY", &STY, &ZPG, 3},
    // 0x85
    {"STA", &STA, &ZPG, 3},
    // 0x86
    {"STX", &STX, &ZPG, 3},
    // 0x87
    {"*SAX", &SAX, &ZPG, 3},
    // 0x88
    {"DEY", &DEY, &IMPL, 2},
    // 0x89
    {"*NOP", &NOP, &IMMED, 2},
    // 0x8A
    {"TXA", &TXA, &IMPL, 2},
    // 0x8B
    {"UNK", &UNK, &NONE, -1},
    // 0x8C
    {"STY", &STY, &ABS, 4},
    // 0x8D
    {"STA", &STA, &ABS, 4},
    // 0x8E
    {"STX", &STX, &ABS, 4},
    // 0x8F
    {"*SAX", &SAX, &ABS, 4},
    // 0x90
    {"BCC", &BCC, &REL, 2},
    // 0x91
    {"STA", &STA, &IND_Y, 6},
    // 0x92
    {"*JAM", &JAM, &NONE, -1},
    // 0x93
    {"UNK", &UNK, &NONE, -1},
    // 0x94
    {"STY", &STY, &ZPG_X, 4},
    // 0x95
    {"STA", &STA, &ZPG_X, 4},
    // 0x96
    {"STX", &STX, &ZPG_Y, 4},
    // 0x97
    {"*SAX", &SAX, &ZPG_Y, 4},
    // 0x98
    {"TYA", &TYA, &IMPL, 2},
    // 0x99
    {"STA", &STA, &ABS_Y, 5},
    // 0x9A
    {"TXS", &TXS, &IMPL, 2},
    // 0x9B
    {"UNK", &UNK, &NONE, -1},
    // 0x9C
    {"UNK", &UNK, &NONE, -1},
    // 0x9D
    {"STA", &STA, &ABS_X, 5},
    // 0x9E
    {"UNK", &UNK, &NONE, -1},
    // 0x9F
    {"UNK", &UNK, &NONE, -1},
    // 0xA0
    {"LDY", &LDY, &IMMED, 2},
    // 0xA1
    {"LDA", &LDA, &IND_X, 6},
    // 0xA2
    {"LDX", &LDX, &IMMED, 2},
    // 0xA3
    {"*LAX", &LAX, &IND_X, 6},
    // 0xA4
    {"LDY", &LDY, &ZPG, 3},
    // 0xA5
    {"LDA", &LDA, &ZPG, 3},
    // 0xA6
    {"LDX", &LDX, &ZPG, 3},
    // 0xA7
    {"*LAX", &LAX, &ZPG, 3},
    // 0xA8
    {"TAY", &TAY, &IMPL, 2},
    // 0XA9
    {"LDA", &LDA, &IMMED, 2},
    // 0xAA
    {"TAX", &TAX, &IMPL, 2},
    // 0xAB
    {"UNK", &UNK, &NONE, -1},
    // 0xAC
    {"LDY", &LDY, &ABS, 4},
    // 0xAD
    {"LDA", &LDA, &ABS, 4},
    // 0xAE
    {"LDX", &LDX, &ABS, 4},
    // 0xAF
    {"*LAX", &LAX, &ABS, 4},
    // 0xB0
    {"BCS", &BCS, &REL, 2},
    // 0xB1
    {"LDA", &LDA, &IND_Y, 5},
    // 0xB2
    {"*JAM", &JAM, &NONE, -1},
    // 0xB3
    {"*LAX", &LAX, &IND_Y, 5},
    // 0xB4
    {"LDY", &LDY, &ZPG_X, 4},
    // 0xB5
    {"LDA", &LDA, &ZPG_X, 4},
    // 0xB6
    {"LDX", &LDX, &ZPG_Y, 4},
    // 0xB7
    {"*LAX", &LAX, &ZPG_Y, 4},
    // 0xB8
    {"CLV", &CLV, &IMPL, 2},
    // 0xB9
    {"LDA", &LDA, &ABS_Y, 4},
    // 0xBA
    {"TSX", &TSX, &IMPL, 2},
    // 0xBB
    {"UNK", &UNK, &NONE, -1},
    // 0xBC
    {"LDY", &LDY, &ABS_X, 4},
    // 0xBD
    {"LDA", &LDA, &ABS_X, 4},
    // 0xBE
    {"LDX", &LDX, &ABS_Y, 4},
    // 0XBF
    {"*LAX", &LAX, &ABS_Y, 4},
    // 0xC0
    {"CPY", &CPY, &IMMED, 2},
    // 0xC1
    {"CMP", &CMP, &IND_X, 6},
    // 0xC2
    {"*NOP", &NOP, &IMMED, 2},
    // 0xC3
    {"*DCP", &DCP, &IND_X, 8},
    // 0xC4
    {"CPY", &CPY, &ZPG, 3},
    // 0xC5
    {"CMP", &CMP, &ZPG, 3},
    // 0xC6
    {"DEC", &DEC, &ZPG, 5},
    // 0xC7
    {"*DCP", &DCP, &ZPG, 5},
    // 0xC8
    {"INY", &INY, &IMPL, 2},
    // 0xC9
    {"CMP", &CMP, &IMMED, 2},
    // 0xCA
    {"DEX", &DEX, &IMPL, 2},
    // 0xCB
    {"UNK", &UNK, &NONE, -1},
    // 0xCC
    {"CPY", &CPY, &ABS, 4},
    // 0XCD
    {"CMP", &CMP, &ABS, 4},
    // 0xCE
    {"DEC", &DEC, &ABS, 6},
    // 0xCF
    {"*DCP", &DCP, &ABS, 6},
    // 0xD0
    {"BNE", &BNE, &REL, 2},
    // 0xD1
    {"CMP", &CMP, &IND_Y, 5},
    // 0xD2
    {"*JAM", &JAM, &NONE, -1},
    // 0xD3
    {"*DCP", &DCP, &IND_Y, 8},
    // 0xD4
    {"*NOP", &NOP, &ZPG_X, 4},
    // 0xD5
    {"CMP", &CMP, &ZPG_X, 4},
    // 0xD6
    {"DEC", &DEC, &ZPG_X, 6},
    // 0xD7
    {"*DCP", &DCP, &ZPG_X, 6},
    // 0xD8
    {"CLD", &CLD, &IMPL, 2},
    // 0xD9
    {"CMP", &CMP, &ABS_Y, 4},
    // 0xDA
    {"*NOP", &NOP, &IMPL, 2},
    // 0xDB
    {"*DCP", &DCP, &ABS_Y, 7},
    // 0xDC
    {"*NOP", &NOP, &ABS_X, 4},
    // 0xDD
    {"CMP", &CMP, &ABS_X, 4},
    // 0xDE
    {"DEC", &DEC, &ABS_X, 7},
    // 0xDF
    {"*DCP", &DCP, &ABS_X, 7},
    // 0xE0
    {"CPX", &CPX, &IMMED, 2},
    // 0xE1
    {"SBC", &SBC, &IND_X, 6},
    // 0xE2
    {"*NOP", &NOP, &IMMED, 2},
    // 0xE3
    {"*ISB", &ISB, &IND_X, 8},
    // 0xE4
    {"CPX", &CPX, &ZPG, 3},
    // 0xE5
    {"SBC", &SBC, &ZPG, 3},
    // 0xE6
    {"INC", &INC, &ZPG, 5},
    // 0xE7
    {"*ISB", &ISB, &ZPG, 5},
    // 0xE8
    {"INX", &INX, &IMPL, 2},
    // 0xE9
    {"SBC", &SBC, &IMMED, 2},
    // 0xEA
    {"NOP", &NOP, &IMPL, 2},
    // 0xEB
    {"*SBC", &SBC, &IMMED, 2},
    // 0xEC
    {"CPX", &CPX, &ABS, 4},
    // 0XED
    {"SBC", &SBC, &ABS, 4},
    // 0xEE
    {"INC", &INC, &ABS, 6},
    // 0xEF
    {"*ISB", &ISB, &ABS, 6},
    // 0xF0
    {"BEQ", &BEQ, &REL, 2},
    // 0xF1
    {"SBC", &SBC, &IND_Y, 5},
    // 0xF2
    {"*JAM", &JAM, &NONE, -1},
    // 0xF3
    {"*ISB", &ISB, &IND_Y, 8},
    // 0xF4
    {"*NOP", &NOP, &ZPG_X, 4},
    // 0xF5
    {"SBC", &SBC, &ZPG_X, 4},
    // 0xF6
    {"INC", &INC, &ZPG_X, 6},
    // 0xF7
    {"*ISB", &ISB, &ZPG_X, 6},
    // 0xF8
    {"SED", &SED, &IMPL, 2},
    // 0xF9
    {"SBC", &SBC, &ABS_Y, 4},
    // 0xFA
    {"*NOP", &NOP, &IMPL, 2},
    // 0xFB
    {"*ISB", &ISB, &ABS_Y, 7},
    // 0xFC
    {"*NOP", &NOP, &ABS_X, 4},
    // 0xFD
    {"SBC", &SBC, &ABS_X, 4},
    // 0xFE
    {"INC", &INC, &ABS_X, 7},
    // 0xFF
    {"*ISB", &ISB, &ABS_X, 7},
};
