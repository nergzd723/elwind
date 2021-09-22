#define _GNU_SOURCE
#include <sys/mman.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "elwind.h"
#include "elwindJIT.h"

uint8_t* CompilerCache;
uint32_t codesize;
uint32_t prev_codesize;

void setupCodegen(){
    CompilerCache = mmap (NULL, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    printf("pointer size: 0x%x\n", sizeof(CompilerCache));
    printf("address: %llx\n", CompilerCache);
    uint8_t testCode[5] = {0x89, 0xc0, 0xc3};
    memcpy(CompilerCache, testCode, sizeof(testCode));

    void (*func_ptr)(void) = (void (*)(void))CompilerCache;
    func_ptr();
    printf("Test successful\n");
    memset(CompilerCache, 0, 4096);
}

int EmitInstruction(ElwindMachine* machine){
    switch (machine->memory.Memory[machine->registers.pc])
    {
    case 0x0:
        printf("(JIT) NOP\n");
        return emit_nop(machine);
    case 0xc3:
        printf("(JIT) JP nn\n");
        return emit_jp_nn(machine);
    case 0x3e:
        printf("(JIT) LD A, n\n");
        return emit_ld_a_n(machine);
    case 0xf3:
        printf("(JIT) DI\n");
        return emit_di(machine);
    case 0xe0:
        printf("(JIT) LDH (n), A\n");
        return emit_ldh_n_a(machine);
    default:
        printf("\nError: instruction not implemented\n");
        printf("Instruction 0x%x, at 0x%x\n", (uint8_t)(machine->memory.Memory[machine->registers.pc]), machine->registers.pc);
        printf("Fatal error!\n");
        return -1;
    }
    return -1;
}

void emit_putb(uint8_t byte){
    CompilerCache[codesize] = byte;
    codesize++;
}

void emit_puta(uint8_t* bytes, uint16_t size){
    for (int i = 0; i < size; i++)
    {
        CompilerCache[codesize] = bytes[i];
        codesize++;
    }
}

void runJIT(ElwindMachine* machine) {
    for (int i = 0; i < 3; i++){
        int size = EmitInstruction(machine);
        if (needExecuteImmediately) {
            needExecuteImmediately = 0;
            break;
        }
        if (!(size == -1)) machine->registers.pc += size;
        else exit(1);
    }
    CompilerCache[codesize] = RET;

    //for(int i = 0; i < 50; i++) printf("%x", CompilerCache[i+50]);
    //printf("\n");
    fflush(stdout);
    void (*func_ptr)(void) = (void (*)(void))CompilerCache + prev_codesize;
    func_ptr();
    prev_codesize = codesize;
    runJIT(machine);
}
