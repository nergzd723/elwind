#include "elwind.h"
#define NOP 0x90
#define RET 0xC3
#define ABS 0x48
#define AX 0xb8
#define CX 0xb9
#define DX 0xba

static int needExecuteImmediately = 0;

void emit_putb(uint8_t byte);
void emit_puta(uint8_t* bytes, uint16_t size);
uint8_t n_helper(ElwindMachine* machine);
uint16_t nn_helper(ElwindMachine* machine);

uint8_t emit_nop(ElwindMachine* machine){
    emit_putb(NOP);
    return 1;
}

void emit_ld_ax_64(void* address) { // Load 64bit value from memory into ax
    uint8_t arr[50] = {ABS, 0xa1, 0, 0, 0, 0, 0, 0, 0, 0}; // MOVABS RAX, ds:address
    memcpy(arr+2, &address, 8);
    emit_puta(arr, 10);
}

void emit_ld_ax_32(void* address) { // Load 32bit value from memory into ax
    uint8_t arr[50] = {0xa1, 0, 0, 0, 0, 0, 0, 0, 0}; // MOVABS EAX, ds:address
    memcpy(arr+1, &address, 8);
    emit_puta(arr, 9);
}

void emit_ld_ax_16(void* address) { // Load 16bit value from memory into ax
    uint8_t arr[50] = {0x66, 0xa1, 0, 0, 0, 0, 0, 0, 0, 0}; // MOVABS AX, ds:address
    memcpy(arr+2, &address, 8);
    emit_puta(arr, 10);
}

void emit_ld_ax_8(void* address) { // Load 8bit value from memory into ax
    uint8_t arr[50] = {0xa0, 0, 0, 0, 0, 0, 0, 0, 0}; // MOVABS AL, ds:address
    memcpy(arr+1, &address, 8);
    emit_puta(arr, 9);
}

void emit_stor_ax_64(void* address) { // Store RAX at given address
    uint8_t arr[50] = {ABS, 0xa3, 0, 0, 0, 0, 0, 0, 0, 0};
    memcpy(arr+2, &address, 8);
    emit_puta(arr, 10);
}

void emit_stor_ax_32(void* address) { // Store EAX at given address
    uint8_t arr[50] = {0xa3, 0, 0, 0, 0, 0, 0, 0, 0};
    memcpy(arr+1, &address, 8);
    emit_puta(arr, 9);
}

void emit_stor_ax_16(void* address) { // Store AX at given address
    uint8_t arr[50] = {0x66, 0xa3, 0, 0, 0, 0, 0, 0, 0, 0};
    memcpy(arr+2, &address, 8);
    emit_puta(arr, 10);
}

void emit_stor_ax_8(void* address) { // Store AL at given address
    uint8_t arr[50] = {0xa2, 0, 0, 0, 0, 0, 0, 0, 0};
    memcpy(arr+1, &address, 8);
    emit_puta(arr, 9);
}

void emit_ld_ax_64_val(uint64_t val) { // Load 64bit value into ax
    uint8_t arr[50] = {ABS, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0}; // MOV RAX, val
    memcpy(arr+2, &val, 8);
    emit_puta(arr, 10);
}

void emit_ld_ax_32_val(uint32_t val) { // Load 32bit value into ax
    uint8_t arr[50] = {0xb8, 0, 0, 0, 0}; // MOV EAX, val
    memcpy(arr+1, &val, 4);
    emit_puta(arr, 5);
}

void emit_ld_ax_16_val(uint16_t val) { // Load 16bit value into ax
    uint8_t arr[50] = {0x66, 0xb8, val & 0xFF, val >> 8}; // MOV AX, val
    emit_puta(arr, 4);
}

void emit_ld_ax_8_val(uint8_t val) { // Load 8bit value into ax
    uint8_t arr[50] = {0xb0, val}; // MOV AL, val
    emit_puta(arr, 2);
}

void emit_load_from_memory_and_store_at_8(void* src, void* dest){
    uint8_t arr[50] = {ABS, DX, 0, 0, 0, 0, 0, 0, 0, 0, // MOVABS RDX, addr
                    0x66, 0x8b, 0xa, // MOV CX, WORD PTR [rdx]
                    ABS, AX, 0, 0, 0, 0, 0, 0, 0, 0, // MOVABS RAX, addr
                    0x66, 0x89, 0x8}; // MOV WORD PTR [rax], cx
    memcpy(arr+2, &src, 8);
    memcpy(arr+2+8+2+2+1, &dest, 8);
    printf("src: %llx, dest: %llx\n", src, dest);
    for(int i = 0; i < 27; i++) printf("%x", arr[i]);
    printf("\n");
    emit_puta(arr, 26);
}

uint8_t emit_jp_nn(ElwindMachine* machine){
    uint16_t nn = nn_helper(machine);
    void* pc = &machine->registers.pc;
    emit_ld_ax_16_val(nn);
    emit_stor_ax_16(pc);
    needExecuteImmediately = 1;
    return 0;
}

uint8_t emit_ld_a_n(ElwindMachine* machine){
    uint8_t n = n_helper(machine);
    void* a_addr = &machine->registers.a;
    emit_ld_ax_8_val(n);
    emit_stor_ax_8(a_addr);
    return 2;
}

uint8_t emit_di(ElwindMachine* machine){
    void* reset_vector_addr = &machine->memory.Memory +0xFFFE;
    emit_ld_ax_8_val(0x0);
    emit_stor_ax_8(reset_vector_addr);
    return 1;
}

uint8_t emit_ldh_n_a(ElwindMachine* machine) {
    uint8_t n = n_helper(machine);
    emit_ld_ax_8(&machine->registers.a);
    emit_stor_ax_8(&machine->memory.Memory + 0xFF00 + n);
    return 2;
}