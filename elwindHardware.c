#include "elwind.h"

void mbc1_handler(ElwindMachine* machine, uint16_t address, uint8_t val){
    printf("MBC: change mode\n");
    printf("address: 0x%x, val: 0x%x\n", address, val);
    if (address >= 0x2000 && address <= 0x3FFF) {
        printf("Change bank to bank %d\n", val);
        fseek(machine->ElwindROM, ROM_BASE_SIZE*val, SEEK_SET);
        fread(machine->memory.Memory+ROM_BASE_SIZE, sizeof(char), ROM_BASE_SIZE, machine->ElwindROM);
        printf("Loaded bank %d to 4000-7FFF\n", val);
    }
}