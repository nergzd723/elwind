#include "elwind.h"

#include <signal.h>
#include <string.h>
#include <time.h>

long long ms_now;
long long ms_old;

ElwindMachine Machine;

void InitializeHardware(){
    Machine.memory.IOMemory[LCDC_Y] = 0x91;
    Machine.memory.Memory[0xffa0] = 0x1;
}

ElwindInstruction FetchInstruction(){
    uint8_t instructionByte = Machine.memory.Memory[Machine.registers.pc];
    ElwindInstruction Instruction = Instructions[instructionByte];
    return Instruction;
}

int ExecuteInstruction(ElwindMachine* machine, ElwindInstruction instruction){
    if (instruction.Execute == NULL){
        printf("\nError: instruction not implemented\n");
        printf("Instruction 0x%x, at 0x%x\n", (uint8_t)(Machine.memory.Memory[Machine.registers.pc]), Machine.registers.pc);
        printf("Fatal error!\n");
        return 1;
    }
    else{
        instruction.Execute(machine);
        machine->registers.pc += instruction.length;
        return 0;
    }
}

void ProcessInterrupt() {
    
    if (Machine.memory.Memory[0xFFFE] == 1){
#ifdef PROFILING
        clock_t begin = clock();
#endif
        FillTileCache(&Machine);
        DrawBackground(&Machine);
#ifdef PROFILING
        clock_t end = clock();
        double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
        printf("Total time spent drawing: %.6fs\n", time_spent);
#endif
        struct timespec time_now;
        clock_gettime(1, &time_now);
        ms_now = time_now.tv_sec*1000 + time_now.tv_nsec / 1e6;
        if ((ms_now - ms_old) > 100){

            if (Machine.memory.Memory[Machine.registers.pc] == 0x76) Machine.registers.pc++;
            Machine.registers.sp -= 2;
            Machine.memory.Memory[Machine.registers.sp] = Machine.registers.pc & 0xFF;
            Machine.memory.Memory[Machine.registers.sp+1] = Machine.registers.pc >> 8;
            Machine.registers.pc = 0x40;
            ms_old = ms_now;
            Machine.memory.Memory[0xFFFE] = 0;
        }
    }
}

void Process(){
    ElwindInstruction next_instruction;
    next_instruction = FetchInstruction();

    // HACK!

    if (strcmp("HALT\n", next_instruction.info)){
        printf("0x%x: ", Machine.registers.pc);
        printf("%s", next_instruction.info);
    }
    broken = ExecuteInstruction(&Machine, next_instruction);
    ProcessInterrupt();
}

void break_ctrl_c(int signal){
    stopped = 1;
}

void die(int signal){
    printf("Exiting\n");
    if (Machine.renderer.window){
        SDL_DestroyWindow(Machine.renderer.window);
        SDL_Quit();
    }
    exit(0);
}

void mainloop(){
    char buffer[512];
    char last_buffer[512];
    short breakpoints[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

    ElwindInstruction next_instruction;
    while (!broken){
        for (uint8_t i = 0; i < 9; i++){
            if (breakpoints[i] == Machine.registers.pc){
                stopped++;
            }
        }
        if (stopped){
            fputs(">>> ", stdout);
            fgets(buffer, 512, stdin);
            if (!strcmp(buffer, "\n")){
                memcpy(buffer, last_buffer, 512);
            }
            else {
                memcpy(last_buffer, buffer, 512);
            }
            if (!strcmp(buffer, "s\n")){
                Process();
            }
            if (!strcmp(buffer, "d\n")){
                FillTileCache(&Machine);
                DrawBackground(&Machine);
            }
            if (!strcmp(buffer, "c\n")){
                stopped = 0;
            }
            if (!strcmp(buffer, "m\n")){
                uint8_t value = Machine.memory.Memory[LCDC_CTRL];
                if (value & BIT(LCDC_CTRL_EN)) printf("Display enable\n");
                else printf("Display disable\n");
                if (value & BIT(LCDC_CTRL_TILEMAP_MAP)) printf("Tilemap at 9C00\n");
                else printf("Tilemap at 9800\n");
                if (value & BIT(LCDC_CTRL_WINDOW_EN)) printf("Window layer enable\n");
                else printf("Window layer disable\n");
                if (value & BIT(LCDC_CTRL_BG_SELECT)) printf("Window tiles at 8000\n");
                else printf("Window tiles at 8800\n");
                if (value & BIT(LCDC_CTRL_BG_TILEMAP)) printf("BG tiles at 9C00\n");
                else printf("BG tiles at 9800\n");
                if (value & BIT(LCDC_CTRL_SPRITE_SIZE)) printf("8x16 sprites\n");
                else printf("8x8 sprites\n");
                if (value & BIT(LCDC_CTRL_SPRITE_EN)) printf("Sprite enable\n");
                else printf("Sprites off\n");
                if (value & BIT(LCDC_CTRL_BG_WINDOW_PRIORITY)) printf("Window over BG\n");
                else printf("BG over window\n");
                printf("\n");
            }
            if (!strcmp(buffer, "r\n")){
                printf("a: 0x%x\n", Machine.registers.a);
                printf("b: 0x%x\n", Machine.registers.b);
                printf("c: 0x%x\n", Machine.registers.c);
                printf("d: 0x%x\n", Machine.registers.d);
                printf("e: 0x%x\n", Machine.registers.e);
                printf("f: 0x%x\n", Machine.registers.f);
                printf("h: 0x%x\n", Machine.registers.h);
                printf("l: 0x%x\n", Machine.registers.l);
                printf("sp: 0x%x\n", Machine.registers.sp);
                printf("pc: 0x%x\n", Machine.registers.pc);
            }
            if (!strcmp(buffer, "dvram\n")) {
                FILE* vram = fopen("vram.bin", "wb+");
                for (int c = 0; c < 0x2000; c++){
                    fputc(Machine.memory.VRAM[c], vram);
                }
                printf("Crashdump done!\n");
            }
            if (buffer[0] == 'b') {
                uint16_t integer = (uint16_t) strtol(buffer+2, NULL, 0);
                uint8_t breakpoint_index = strlen((char*)breakpoints) / 2;
                breakpoints[breakpoint_index] = integer;
            }
        }
        else Process();
    }
    //FillTileCache(&Machine);

    //DrawBackground(&Machine);

    FILE* vram = fopen("vram.bin", "wb+");
    fwrite(Machine.memory.VRAM, sizeof(uint8_t), sizeof(Machine.memory.VRAM), vram);
    printf("Crashdump done!\n");
}

int main(int argc, char** argv){
    srand(time(NULL));
    Machine.registers.pc = PC_START;
    FILE* ROMHandle;
    struct timespec old_time;
    clock_gettime(1, &old_time);
    ms_old = old_time.tv_sec*1000 + old_time.tv_nsec / 1e6;
    ROMHandle = fopen("smland.gb", "r");
    Machine.ElwindROM = ROMHandle;
    fread(Machine.memory.Memory, sizeof(char), ROM_BASE_SIZE, ROMHandle);
    printf("Loaded first 16KiB of ROM into machine's memory\n");
    char title[16];
    memcpy(title, Machine.memory.Memory+0x134, sizeof(title));
    printf("Game title: %s\n", title);
    uint8_t cartridge_type = Machine.memory.Memory[0x147];
    if (cartridge_type == 0x0) {
        printf("Cartridge type: plain ROM, loading rest 16K of memory\n");
        fread(Machine.memory.Memory+ROM_BASE_SIZE, sizeof(char), ROM_BASE_SIZE, ROMHandle);
        Machine.memory.banking_mode = MBC_MODE_NONE;
    }
    else if (cartridge_type == 0x1) {
        printf("Cartridge type: MBC1\n");
        Machine.memory.banking_mode = MBC_MODE_MBC1;
    }
    else printf("Unknown cartridge type!!! (0x%x)\n", cartridge_type);
    signal(SIGINT, break_ctrl_c);
    signal(SIGTSTP, die);
    InitializeHardware();
    if (InitSDL2(&Machine)){
        printf("Broken!\n");
        return 0;
    }
    mainloop();
    printf("Broken!\n");
    return 0;
}
