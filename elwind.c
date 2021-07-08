#include "elwind.h"

#include <signal.h>
#include <string.h>
#include <time.h>

ElwindMachine Machine;

void InitializeHardware(){
    Machine.memory.IOMemory[LCDC_Y] = 0x91;
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

void Process(){
    ElwindInstruction next_instruction;
    next_instruction = FetchInstruction();
    printf("0x%x: ", Machine.registers.pc);
    printf("%s", next_instruction.info);
    broken = ExecuteInstruction(&Machine, next_instruction);
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
    uint16_t breakpoints[10];

    ElwindInstruction next_instruction;
    while (!broken){
        for (uint8_t i = 0; i < 9; i++){
            if (breakpoints[i] == Machine.registers.pc){
                stopped++;
                breakpoints[i] = 0;
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
    FillTileCache(&Machine);

    //DrawBackground(&Machine);

    FILE* vram = fopen("vram.bin", "wb+");
    for (int c = 0; c < 0x2000; c++){
        fputc(Machine.memory.VRAM[c], vram);
    }
    printf("Crashdump done!\n");
}

int main(int argc, char** argv){
    srand(time(NULL));
    Machine.registers.pc = PC_START;
    FILE* ROMHandle;
    ROMHandle = fopen("tetris.gb", "r");
    fread(Machine.memory.Memory, sizeof(char), ROM_SIZE, ROMHandle);
    printf("Loaded first 32KiB of ROM into machine's memory\n");
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
