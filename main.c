#include "c6502.h"

/*
The nestest.nes rom from Kevin Horton is used to test my 6502 emulator.

The log file nestest.log was used as reference to to validate the 6502 processor.

See the nestest.txt document for more information.
*/

/* Nes Header
Bytes
0-3     Constant 4E 45 53 1A
4       Size of PRG rom in 16kb chucks
5       Size of CHR rom in 8kb chucks
6       Flag 6 Mapper, mirroring, battery, trainer
7       Flag 7 Mapper, VS/Playchoice, NES 2.0
8       Flag 8 PGR Ram size
9       Flag 9 TV system
10      Flag 10 TV System, PGR-RAM presence
11-15   Unused padding
*/
typedef struct
{
    uint8_t nes[4];
    uint8_t prg_chunks;
    uint8_t chr_chunks;
    uint8_t mapper1;
    uint8_t mapper2;
    uint8_t prg_ram_size;
    uint8_t tv_system1;
    uint8_t tv_system2;
    uint8_t unused[5];
} iNesHeader;

int main()
{

    /*
    To run the nestest.rom on automation, set the program counter to 0c000h.
    */
    c6502_init(0xC0, 0x00);

    FILE *fp = fopen("nestest.nes", "rb");
    if (!fp)
    {
        printf("Rom file nestest.nes does not exist\n");
        return false;
    }

    iNesHeader header = {0};
    // read header from nestest.nes rom
    fread(&header, sizeof(iNesHeader), 1, fp);
    // If trainer is present skip it.
    if (header.mapper1 & 0x04)
    {
        fseek(fp, 512, SEEK_CUR);
    }
    // Write program chunks to ram address.
    fread(&ADDRESS[0xC000], header.prg_chunks * 16384, 1, fp);

    //  printf("%s\n", header.nes);

    fclose(fp);

    //variable used in print routine
    uint16_t temp = 0;
    uint16_t temp2 = 0;
    uint16_t temp3 = 0;
    uint16_t counter = 0;
    uint16_t LSB = 0;
    uint16_t MSB = 0;

    for (int i = 0; i < 8991; i++)
    {
        c6502_read_opcode();

        // Pointer to the next byte after opcode.
        counter = c6502.PC + 1;

        // Read ahead next two bytes for print routine below.
        LSB = cpu_read(counter) & 0x00FF;
        MSB = cpu_read(counter + 1) & 0x00FF;

        // Print routine to match nestest.log minus the PPU information.
        if (lookup_table[c6502.opcode].address_mode == IMPL)
        {
            printf("%-4X %-8X  %4s \t\t\t\tA:%02X X:%02X Y:%02X P:%-2X SP:%-2X CYC:%-6d\n", c6502.PC, c6502.opcode, lookup_table[c6502.opcode].name, c6502.A, c6502.X, c6502.Y, c6502.SR, c6502.SP, c6502.cycles);
        }
        else if (lookup_table[c6502.opcode].address_mode == A)
        {
            printf("%-4X %-8X  %4s A\t\t\t\tA:%02X X:%02X Y:%02X P:%-2X SP:%-2X CYC:%-6d\n", c6502.PC, c6502.opcode, lookup_table[c6502.opcode].name, c6502.A, c6502.X, c6502.Y, c6502.SR, c6502.SP, c6502.cycles);
        }
        else if (lookup_table[c6502.opcode].address_mode == IMMED)
        {
            printf("%-4X %02X %02X     %4s  #$%02X \t\t\tA:%02X X:%02X Y:%02X P:%-2X SP:%-2X CYC:%-6d\n", c6502.PC, c6502.opcode, LSB, lookup_table[c6502.opcode].name, LSB, c6502.A, c6502.X, c6502.Y, c6502.SR, c6502.SP, c6502.cycles);
        }
        else if (lookup_table[c6502.opcode].address_mode == ABS)
        {
            temp = (MSB << 8) | LSB;
            if (lookup_table[c6502.opcode].run == BCC ||
                lookup_table[c6502.opcode].run == BCS ||
                lookup_table[c6502.opcode].run == BEQ ||
                lookup_table[c6502.opcode].run == BMI ||
                lookup_table[c6502.opcode].run == BNE ||
                lookup_table[c6502.opcode].run == BPL ||
                lookup_table[c6502.opcode].run == BRK ||
                lookup_table[c6502.opcode].run == BVC ||
                lookup_table[c6502.opcode].run == BVS ||
                lookup_table[c6502.opcode].run == JMP ||
                lookup_table[c6502.opcode].run == JSR ||
                lookup_table[c6502.opcode].run == JAM)
            {
                printf("%-4X %02X %02X %02X  %4s  $%04X \t\t\tA:%02X X:%02X Y:%02X P:%-2X SP:%-2X CYC:%-6d\n", c6502.PC, c6502.opcode, LSB, MSB, lookup_table[c6502.opcode].name, temp, c6502.A, c6502.X, c6502.Y, c6502.SR, c6502.SP, c6502.cycles);
            }
            else
            {
                printf("%-4X %02X %02X %02X  %4s  $%04X = %02X \t\tA:%02X X:%02X Y:%02X P:%-2X SP:%-2X CYC:%-6d\n", c6502.PC, c6502.opcode, LSB, MSB, lookup_table[c6502.opcode].name, temp, cpu_read(temp), c6502.A, c6502.X, c6502.Y, c6502.SR, c6502.SP, c6502.cycles);
            }
        }
        else if (lookup_table[c6502.opcode].address_mode == ZPG)
        {
            printf("%-4X %02X %02X     %4s  $%02X = %02X \t\t\tA:%02X X:%02X Y:%02X P:%-2X SP:%-2X CYC:%-6d\n", c6502.PC, c6502.opcode, LSB, lookup_table[c6502.opcode].name, LSB, cpu_read(LSB), c6502.A, c6502.X, c6502.Y, c6502.SR, c6502.SP, c6502.cycles);
        }
        else if (lookup_table[c6502.opcode].address_mode == ABS_X)
        {
            temp = ((MSB << 8) | LSB) + c6502.X;
            printf("%-4X %02X %02X %02X  %4s  $%02X%02X,X @ %04X = %02X \tA:%02X X:%02X Y:%02X P:%-2X SP:%-2X CYC:%-6d\n", c6502.PC, c6502.opcode, LSB, MSB, lookup_table[c6502.opcode].name, MSB, LSB, temp, cpu_read(temp), c6502.A, c6502.X, c6502.Y, c6502.SR, c6502.SP, c6502.cycles);
        }
        else if (lookup_table[c6502.opcode].address_mode == ABS_Y)
        {
            temp = ((MSB << 8) | LSB) + c6502.Y;
            printf("%-4X %02X %02X %02X  %4s  $%02X%02X,Y @ %04X = %02X \tA:%02X X:%02X Y:%02X P:%-2X SP:%-2X CYC:%-6d\n", c6502.PC, c6502.opcode, LSB, MSB, lookup_table[c6502.opcode].name, MSB, LSB, temp, cpu_read(temp), c6502.A, c6502.X, c6502.Y, c6502.SR, c6502.SP, c6502.cycles);
        }
        else if (lookup_table[c6502.opcode].address_mode == ZPG_X)
        {
            temp = (LSB + c6502.X);
            temp &= 0x00FF;
            printf("%-4X %02X %02X     %4s  $%02X,X @ %02X = %02X \t\tA:%02X X:%02X Y:%02X P:%-2X SP:%-2X CYC:%-6d\n", c6502.PC, c6502.opcode, LSB, lookup_table[c6502.opcode].name, LSB, temp, cpu_read(temp), c6502.A, c6502.X, c6502.Y, c6502.SR, c6502.SP, c6502.cycles);
        }
        else if (lookup_table[c6502.opcode].address_mode == ZPG_Y)
        {
            temp = (LSB + c6502.Y);
            temp &= 0x00FF;
            printf("%-4X %02X %02X     %4s  $%02X,Y @ %02X = %02X \t\tA:%02X X:%02X Y:%02X P:%-2X SP:%-2X CYC:%-6d\n", c6502.PC, c6502.opcode, LSB, lookup_table[c6502.opcode].name, LSB, temp, cpu_read(temp), c6502.A, c6502.X, c6502.Y, c6502.SR, c6502.SP, c6502.cycles);
        }

        else if (lookup_table[c6502.opcode].address_mode == IND)
        {
            temp2 = (MSB << 8) | LSB;
            if (LSB == 0x00FF)
            {
                temp = cpu_read(temp2 & 0xFF00) << 8 | cpu_read(temp2);
            }
            else
            {
                temp = cpu_read(temp2 + 1) << 8 | cpu_read(temp2);
            }
            printf("%-4X %02X %02X %02X  %4s  ($%02X%02X) = %04X \t\tA:%02X X:%02X Y:%02X P:%-2X SP:%-2X CYC:%-6d\n", c6502.PC, c6502.opcode, LSB, MSB, lookup_table[c6502.opcode].name, MSB, LSB, temp, c6502.A, c6502.X, c6502.Y, c6502.SR, c6502.SP, c6502.cycles);
        }
        else if (lookup_table[c6502.opcode].address_mode == IND_X)
        {
            temp2 = (uint16_t)cpu_read((uint16_t)(LSB + (uint16_t)c6502.X) & 0x00FF);
            temp3 = (uint16_t)cpu_read((uint16_t)(LSB + 1 + (uint16_t)c6502.X) & 0x00FF);
            temp = (temp3 << 8) | temp2;
            printf("%-4X %02X %02X     %4s  ($%02X,X) @ %02X = %04X = %02X \tA:%02X X:%02X Y:%02X P:%-2X SP:%-2X CYC:%-6d\n", c6502.PC, c6502.opcode, LSB, lookup_table[c6502.opcode].name, LSB, LSB + c6502.X, temp, cpu_read(temp), c6502.A, c6502.X, c6502.Y, c6502.SR, c6502.SP, c6502.cycles);
        }
        else if (lookup_table[c6502.opcode].address_mode == IND_Y)
        {

            uint16_t temp2 = (uint16_t)cpu_read((uint16_t)(LSB) & 0x00FF);
            uint16_t temp3 = (uint16_t)cpu_read((uint16_t)(LSB + 1) & 0x00FF);
            temp = ((temp3 << 8) | temp2) + c6502.Y;

            printf("%-4X %02X %02X     %4s  ($%02X),Y = %02X%02X @ %04X = %02X A:%02X X:%02X Y:%02X P:%-2X SP:%-2X CYC:%-6d\n", c6502.PC, c6502.opcode, LSB, lookup_table[c6502.opcode].name, LSB, temp3, temp2, temp, cpu_read(temp), c6502.A, c6502.X, c6502.Y, c6502.SR, c6502.SP, c6502.cycles);
        }
        else if (lookup_table[c6502.opcode].address_mode == REL)
        {
            temp = LSB & 0x00FF;
            if (LSB & 0x80)
            {
                temp = LSB | 0xFF00;
            }
            printf("%-4X %02X %02X     %4s  $%02X \t\t\tA:%02X X:%02X Y:%02X P:%-2X SP:%-2X CYC:%-6d\n", c6502.PC, c6502.opcode, LSB, lookup_table[c6502.opcode].name, c6502.PC + 2 + temp, c6502.A, c6502.X, c6502.Y, c6502.SR, c6502.SP, c6502.cycles);
        }
        else
        {
            printf("%-4X %02X %02X %02X  %4s  $%02X%02X \t\t\tA:%02X X:%02X Y:%02X P:%-2X SP:%-2X CYC:%-6d\n", c6502.PC, c6502.opcode, LSB, MSB, lookup_table[c6502.opcode].name, MSB, LSB, c6502.A, c6502.X, c6502.Y, c6502.SR, c6502.SP, c6502.cycles);
        }
        // Advance Program Counter.
        c6502.PC++;
        // Run instruction
        lookup_table[c6502.opcode].run();
    }

    if (ADDRESS[0x02] == 0 && ADDRESS[0x03] == 0)
    {
        printf("\nC6502 cpu works!\n");
        // Print the result of nestest rom. If 02h and 03h result pass than the value should be 0.

        printf("\nNestest.nes rom result 02h:%X 03h:%X\n\n", ADDRESS[0x02], ADDRESS[0x03]);
    }
    else
    {
        // See nestest.txt for error in your 6502 emulation.
        printf("See nestest.txt for error code!\n");
        printf("\nNestest.nes rom result 02h:%X 03h:%X\n\n", ADDRESS[0x02], ADDRESS[0x03]);
    }

    return 0;
}
