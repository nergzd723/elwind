#include "elwind.h"

#include <signal.h>
#include <string.h>

ElwindMachine Machine;

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

void mainloop(){
    char buffer[512];
    char last_buffer[512];

    ElwindInstruction next_instruction;
    while (!broken){
        if (stopped){
            fputs(">>> ", stdout);
            fgets(buffer, 512, stdin);
            if (!strcmp(buffer, "\n")){
                memcpy(buffer, last_buffer, 512);
            }
            else{
                memcpy(last_buffer, buffer, 512);
            }
            if (!strcmp(buffer, "s\n")){
                Process();
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
        }
        else Process();
    }
}

int main(int argc, char** argv){
    Machine.registers.pc = PC_START;
    FILE* ROMHandle;
    ROMHandle = fopen("drmario.gb", "r");
    fread(Machine.memory.Memory, sizeof(char), ROM_SIZE, ROMHandle);
    printf("Loaded first 32KiB of ROM into machine's memory\n");
    signal(SIGINT, break_ctrl_c);
    mainloop();
    printf("Broken!\n");
    return 0;
}
