#include "elwind.h"
#include <time.h>

long long ms_old, ms_now;

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
        printf("Upper number: %d\n", val);
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

void ProcessVsync(ElwindMachine* machine) {
#ifdef PROFILING
        clock_t begin = clock();
#endif
        FillTileCache(machine);
        DrawBackground(machine);
#ifdef PROFILING
        clock_t end = clock();
        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("Total time spent drawing: %.6fs\n", time_spent);
#endif
    struct timespec time_now;
    clock_gettime(1, &time_now);
    ms_now = time_now.tv_sec*1000 + time_now.tv_nsec / 1e6;
    if ((ms_now - ms_old) > 1000){

        if (machine->memory.Memory[machine->registers.pc] == 0x76) machine->registers.pc++; // if HALT
        machine->registers.sp -= 2;
        machine->memory.Memory[machine->registers.sp] = machine->registers.pc & 0xFF;
        machine->memory.Memory[machine->registers.sp+1] = machine->registers.pc >> 8;
        machine->registers.pc = 0x40;
        ms_old = ms_now;
        machine->memory.Memory[0xFFFE] = 0;
    }
}

void ProcessTimer(ElwindMachine* machine) {
    static uint8_t cpu_instructions;
    if (machine->memory.IOMemory[0x7] & BIT(2)) {
        return;
    }
    cpu_instructions++;
    machine->memory.IOMemory[0x5] += 32;
    if (cpu_instructions == 100) {
        machine->registers.sp -= 2;
        machine->memory.Memory[machine->registers.sp] = machine->registers.pc & 0xFF;
        machine->memory.Memory[machine->registers.sp+1] = machine->registers.pc >> 8;
        machine->registers.pc = 0x50;
        machine->memory.Memory[0xFFFE] = 0;
        machine->memory.IOMemory[0x5] = machine->memory.IOMemory[0x6]; // Reload with TMA
        machine->memory.IOMemory[0x7] &= ~(BIT(2)); // Clear timer?
        cpu_instructions = 0;
    }
}

void ProcessInterrupt(ElwindMachine* machine) {
    
    if (machine->memory.Memory[0xFFFE] == 1){
        //if (machine->memory.Memory[0xFF0F] & BIT(0)) { // VBLANK at position 0
            ProcessVsync(machine);
            //ProcessTimer(machine);
        //} // Hack for now, doesn't really work :/
    }
}

