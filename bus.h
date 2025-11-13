#include <stdint.h>
#include <stdbool.h>

// Change cpu_read and cpu_write to access hardware your trying to emulate.

// The 6502 has a 16 bit address space alowing it directly access 2^16 = 64KB of memory.

uint8_t ADDRESS[65536];

uint8_t DATABUS; // Data from busline.

uint8_t cpu_read(uint16_t abs_address);

void cpu_write(uint16_t abs_address, uint8_t data);
