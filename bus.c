//bus.c

#include "bus.h"
#include <stdio.h>

uint8_t cpu_read(uint16_t abs_address)
{
    DATABUS = ADDRESS[abs_address];
    return DATABUS;
}

void cpu_write(uint16_t abs_address, uint8_t data)
{
    DATABUS = data;
    ADDRESS[abs_address] = data;
}

