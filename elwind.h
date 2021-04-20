#ifndef ELWIND_H
#define ELWIND_H

#include <stdint.h>
#include <stdio.h>

#define PC_START 0x100
#define ROM_SIZE 0x8000
#define FLAG_ZERO 7
#define FLAG_NEGATIVE 6
#define FLAG_HC 5
#define FLAG_CARRY 4

#define BIT(n) (1 << n)


typedef struct {
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t h;
    uint8_t l;
    uint8_t f;
    uint16_t sp;
    uint16_t pc;
} ElwindRegisters;

typedef struct {
    char Memory[0xFFFF];
    char VRAM[0x2000];
    char OAM[0xFF];
    char IOMemory[0x7F];
} ElwindMemory;

typedef struct {
    ElwindRegisters registers;
    ElwindMemory memory;
} ElwindMachine;

typedef struct {
    char* info;
    uint8_t length;
    void (*Execute)(ElwindMachine*);
} ElwindInstruction;

extern const ElwindInstruction Instructions[256];

static char broken;
static char stopped;
#endif