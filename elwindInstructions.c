#include "elwind.h"

uint8_t memory_readb(ElwindMachine* machine, uint16_t address){
    if (address >= 0x8000 && address <= 0x9FFF){
        return machine->memory.VRAM[address - 0x8000];
    }
    if (address >= 0xFE00 && address <= 0xFE9F){
        return machine->memory.OAM[address - 0xFE00];
    }
    if (address >= 0xFF00 && address <= 0xFF7F){
        return machine->memory.IOMemory[address - 0xFF00];
    }
    return machine->memory.Memory[address];
}

void memory_writeb(ElwindMachine* machine, uint16_t address, uint8_t val){
    if (address >= 0x8000 && address <= 0x9FFF){
        machine->memory.VRAM[address - 0x8000] = val;
    }
    if (address >= 0xFE00 && address <= 0xFE9F){
        machine->memory.OAM[address - 0xFE00] = val;
    }
    if (address >= 0xFF00 && address <= 0xFF7F){
        machine->memory.IOMemory[address - 0xFF00] = val;
    }
    machine->memory.Memory[address] = val;
}

void zero_helper(ElwindMachine* machine, uint16_t a){
    if (a == 0){
        machine->registers.f |= BIT(FLAG_ZERO);
    }
    else{
        machine->registers.f &= ~(BIT(FLAG_ZERO));
    }
}

uint16_t nn_helper(ElwindMachine* machine){
    return (uint8_t)machine->memory.Memory[machine->registers.pc+2] << 8 | (uint8_t)machine->memory.Memory[machine->registers.pc+1];
}

uint8_t n_helper(ElwindMachine* machine){
    return (uint8_t)machine->memory.Memory[machine->registers.pc+1];
}

void set_hl_helper(ElwindMachine* machine, uint16_t value){
    machine->registers.h = (value & 0xFF00) >> 8;
    machine->registers.l = value & 0xFF;
}

uint16_t get_hl_helper(ElwindMachine* machine){
    return (machine->registers.h << 8) | machine->registers.l;
}

void set_de_helper(ElwindMachine* machine, uint16_t value){
    machine->registers.d = (value & 0xFF00) >> 8;
    machine->registers.e = value & 0xFF;
}

uint16_t get_de_helper(ElwindMachine* machine){
    return (machine->registers.d << 8) | machine->registers.e;
}

void set_bc_helper(ElwindMachine* machine, uint16_t value){
    machine->registers.b = (value & 0xFF00) >> 8;
    machine->registers.c = value & 0xFF;
}

uint16_t get_bc_helper(ElwindMachine* machine){
    return (machine->registers.b << 8) | machine->registers.c;
}

void dec_helper(ElwindMachine* machine, uint8_t* targetRegister){
    *targetRegister = *targetRegister - 1;
    if (*targetRegister == UINT8_MAX){
        machine->registers.f |= BIT(FLAG_CARRY);
    }
    else machine->registers.f &= ~(BIT(FLAG_CARRY));
    zero_helper(machine, *targetRegister);
}

void dec_helper_16(ElwindMachine* machine, uint16_t* targetRegister){
    *targetRegister = *targetRegister - 1;
    if (*targetRegister == UINT16_MAX){
        machine->registers.f |= BIT(FLAG_CARRY);
    }
    else machine->registers.f &= ~(BIT(FLAG_CARRY));
    zero_helper(machine, *targetRegister);
}

void inc_helper(ElwindMachine* machine, uint8_t* targetRegister){
    *targetRegister = *targetRegister + 1;
    if (*targetRegister == 0){
        machine->registers.f |= BIT(FLAG_CARRY);
    }
    else machine->registers.f &= ~(BIT(FLAG_CARRY));
    zero_helper(machine, *targetRegister);
}

void inc_helper_16(ElwindMachine* machine, uint16_t* targetRegister){
    *targetRegister = *targetRegister + 1;
    if (*targetRegister == 0){
        machine->registers.f |= BIT(FLAG_CARRY);
    }
    else machine->registers.f &= ~(BIT(FLAG_CARRY));
    zero_helper(machine, *targetRegister);
}

void nop(ElwindMachine* machine){
    return;
}

void di(ElwindMachine* machine){
    memory_writeb(machine, 0xFFFF, 0);
}

void jp_nn(ElwindMachine* machine){
    uint16_t nn = (uint8_t)machine->memory.Memory[machine->registers.pc+2] << 8 | (uint8_t)machine->memory.Memory[machine->registers.pc+1];
    machine->registers.pc = nn;
}

void jr_nz_n(ElwindMachine* machine){
    if (!(machine->registers.f & BIT(FLAG_ZERO))){
        machine->registers.pc += ((n_helper(machine) ^ 0x80) - 0x80);
    }
}

void jr_n(ElwindMachine* machine){
    if (n_helper(machine) == 0xFE){
        stopped = 1;
        printf("Infinite loop detected\n");
    }
    else machine->registers.pc += ((n_helper(machine) ^ 0x80) - 0x80);
}

void jr_c_n(ElwindMachine* machine){
    if (machine->registers.f & BIT(FLAG_CARRY)){
        machine->registers.pc += ((n_helper(machine) ^ 0x80) - 0x80);
    }
}

void ld_hl_nn(ElwindMachine* machine){
    set_hl_helper(machine, nn_helper(machine));
}

void ld_de_nn(ElwindMachine* machine){
    set_de_helper(machine, nn_helper(machine));
}

void ld_bc_nn(ElwindMachine* machine){
    set_bc_helper(machine, nn_helper(machine));
}

void ld_a_n(ElwindMachine* machine){
    machine->registers.a = n_helper(machine);
}

void ld_a_b(ElwindMachine* machine){
    machine->registers.a = machine->registers.b;
}

void ld_b_n(ElwindMachine* machine){
    machine->registers.b = n_helper(machine);
}

void ld_c_n(ElwindMachine* machine){
    machine->registers.c = n_helper(machine);
}

void ld_a_de(ElwindMachine* machine){
    machine->registers.a = memory_readb(machine, get_de_helper(machine));
}

void ld_nn_a(ElwindMachine* machine){
    memory_writeb(machine, nn_helper(machine), machine->registers.a);
}

void ldh_n_a(ElwindMachine* machine){
    memory_writeb(machine, 0xFF00 + n_helper(machine), machine->registers.a);
}

void ldh_a_n(ElwindMachine* machine){
    machine->registers.a = memory_readb(machine, 0xFF00 + n_helper(machine));
}

void ldd_hl_a(ElwindMachine* machine){
    memory_writeb(machine, get_hl_helper(machine), machine->registers.a);
    set_hl_helper(machine, get_hl_helper(machine) - 1);
    if (get_hl_helper(machine) == UINT16_MAX){
        machine->registers.f |= BIT(FLAG_CARRY);
    }
    else machine->registers.f &= ~(BIT(FLAG_CARRY));
    zero_helper(machine, get_hl_helper(machine));
}

void ldi_hl_a(ElwindMachine* machine){
    memory_writeb(machine, get_hl_helper(machine), machine->registers.a);
    set_hl_helper(machine, get_hl_helper(machine) + 1);
    if (get_hl_helper(machine) == 0){
        machine->registers.f |= BIT(FLAG_CARRY);
    }
    else machine->registers.f &= ~(BIT(FLAG_CARRY));
    zero_helper(machine, get_hl_helper(machine));
}

void and_a(ElwindMachine* machine){
    machine->registers.a = machine->registers.a & machine->registers.a;
    zero_helper(machine, machine->registers.a);
}

void xor_a(ElwindMachine* machine){
    machine->registers.a = machine->registers.a ^ machine->registers.a;
    zero_helper(machine, machine->registers.a);
}

void or_b(ElwindMachine* machine){
    machine->registers.a = machine->registers.a | machine->registers.b;
    zero_helper(machine, machine->registers.a);
}

void or_c(ElwindMachine* machine){
    machine->registers.a = machine->registers.a | machine->registers.c;
    zero_helper(machine, machine->registers.a);
}

void inc_de(ElwindMachine* machine){
    static uint16_t de;
    de = get_de_helper(machine);
    inc_helper_16(machine, &de);
    set_de_helper(machine, de);
}

void dec_bc(ElwindMachine* machine){
    static uint16_t bc;
    bc = get_bc_helper(machine);
    dec_helper_16(machine, &bc);
    set_bc_helper(machine, bc);
}

void dec_b(ElwindMachine* machine){
    dec_helper(machine, &machine->registers.b);
}

void dec_c(ElwindMachine* machine){
    dec_helper(machine, &machine->registers.c);
}

void cp_n(ElwindMachine* machine){
    uint8_t n = n_helper(machine);
    printf("n: 0x%x\n", n);
    if (machine->registers.a == n) machine->registers.f |= BIT(FLAG_ZERO);
    else machine->registers.f &= ~(BIT(FLAG_ZERO));
    if (machine->registers.a < n) machine->registers.f |= BIT(FLAG_CARRY);
    else machine->registers.f &= ~(BIT(FLAG_ZERO));
    if ((machine->registers.a & 0x0f) < (n & 0x0f)) machine->registers.f |= BIT(FLAG_HC);
    else machine->registers.f &= ~(BIT(FLAG_HC));
    machine->registers.f |= BIT(FLAG_NEGATIVE);    
}

const ElwindInstruction Instructions[256] = {
    { "NOP\n", 1, nop },
    { "LD BC, nn\n", 3, ld_bc_nn },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "DEC B\n", 1, dec_b },
    { "LD B, n\n", 2, ld_b_n },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "DEC BC\n", 1, dec_bc },
    { "NULL", 1, NULL },
    { "DEC C\n", 1, dec_c },
    { "LD C, n\n", 2, ld_c_n },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "LD DE, nn\n", 3, ld_de_nn },
    { "NULL", 1, NULL },
    { "INC DE\n", 1, inc_de },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "JR n\n", 2, jr_n },
    { "NULL", 1, NULL },
    { "LD A, (DE)\n", 1, ld_a_de },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "JR NZ, n\n", 2, jr_nz_n },
    { "LD HL, nn\n", 3, ld_hl_nn },
    { "LDI (HL), A\n", 1, ldi_hl_a },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "LDD (HL), A\n", 1, ldd_hl_a },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "JR C, n\n", 2, jr_c_n },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "LD A, n\n", 2, ld_a_n },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "LD A, B\n", 1, ld_a_b },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "AND A\n", 1, and_a },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "XOR A\n", 1, xor_a },
    { "OR B\n", 1, or_b },
    { "OR C\n", 1, or_c },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "JP nn\n", 0, jp_nn },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "LDH (n), A\n", 2, ldh_n_a },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "LD (nn), A\n", 3, ld_nn_a },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "LDH A, (n)\n", 2, ldh_a_n },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "DI\n", 1, di },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "CP n\n", 2, cp_n },
    { "NULL", 1, NULL },
};
