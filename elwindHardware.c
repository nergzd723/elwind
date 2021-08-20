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
    if (address >= 0x0 && address <= 0x1FFF) {
        printf("MBC: RAM enable\n");
        if (val == 0xA) printf("Enable RAM\n");
        // Do nothing for now, we have memory mapped onto background anyway
    }
    if (address >= 0x4000 && address <= 0x5FFF) {
        printf("MBC: Bank select\n");
        printf("Upper number: %d\n", val << 6);
        printf("WIP\n");
        getc(stdin);
    }
    if (address >= 0x6000 && address <= 0x7FFF) {
        printf("MBC: ROM/RAM mode select\n");
        if (val) printf("RAM mode\n");
        else printf("ROM mode\n");
        printf("WIP\n");
        getc(stdin);
    }
}