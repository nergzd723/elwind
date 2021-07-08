#include "elwind.h"

uint8_t io_read_handler(ElwindMachine* machine, uint8_t address){
    uint8_t vblank;
    switch(address){
        case LCDC_Y:
            vblank = rand() % 144;
            printf("random value: %x\n", vblank);
            return vblank; // make vblank a bit random
        default:
            printf("Warning: reading unimplemented I/O region!\n");
            return 0;
    }
}

uint8_t memory_readb(ElwindMachine* machine, uint16_t address){
    if (address >= 0x8000 && address <= 0x9FFF){
        return machine->memory.VRAM[address - 0x8000];
    }
    if (address >= 0xFE00 && address <= 0xFE9F){
        return machine->memory.OAM[address - 0xFE00];
    }
    if (address >= 0xFF00 && address <= 0xFF7F){
        if (io_read_handler(machine, address - 0xFF00)) return io_read_handler(machine, address - 0xFF00);
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

void bit_helper(ElwindMachine* machine, uint8_t bit, uint16_t number){
    if (number & BIT(bit)) machine->registers.f |= BIT(FLAG_ZERO);
    else machine->registers.f &= ~(BIT(FLAG_ZERO));
}

void res_helper(ElwindMachine* machine, uint8_t bit, uint8_t* number){
    *number &= ~(BIT(bit));
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

void add_helper(ElwindMachine* machine, uint8_t* targetRegister, uint8_t number){
    uint8_t old_value = *targetRegister;
    *targetRegister += number;
    if (*targetRegister < old_value){
        machine->registers.f |= BIT(FLAG_CARRY);
    }
    else machine->registers.f &= ~(BIT(FLAG_CARRY));
}

void nop(ElwindMachine* machine){
    return;
}

void di(ElwindMachine* machine){
    memory_writeb(machine, 0xFFFE, 0);
}

void ei(ElwindMachine* machine){
    memory_writeb(machine, 0xFFFE, 1);
}

void cpl(ElwindMachine* machine){
    machine->registers.a = ~(machine->registers.a);
    zero_helper(machine, machine->registers.a);
}

void jp_nn(ElwindMachine* machine){
    uint16_t nn = (uint8_t)machine->memory.Memory[machine->registers.pc+2] << 8 | (uint8_t)machine->memory.Memory[machine->registers.pc+1];
    machine->registers.pc = nn;
}

void jp_hl(ElwindMachine* machine){
    machine->registers.pc = get_hl_helper(machine);
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

void ld_sp_nn(ElwindMachine* machine){
    machine->registers.sp = nn_helper(machine);
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

void ld_a_c(ElwindMachine* machine){
    machine->registers.a = machine->registers.c;
}

void ld_b_n(ElwindMachine* machine){
    machine->registers.b = n_helper(machine);
}

void ld_b_a(ElwindMachine* machine){
    machine->registers.b = machine->registers.a;
}

void ld_c_n(ElwindMachine* machine){
    machine->registers.c = n_helper(machine);
}

void ld_c_a(ElwindMachine* machine){
    machine->registers.c = machine->registers.a;
}

void ld_e_a(ElwindMachine* machine){
    machine->registers.e = machine->registers.a;
}

void ld_a_de(ElwindMachine* machine){
    machine->registers.a = memory_readb(machine, get_de_helper(machine));
}

void ld_e_hl(ElwindMachine* machine){
    machine->registers.e = memory_readb(machine, get_hl_helper(machine));
}

void ld_d_hl(ElwindMachine* machine){
    machine->registers.d = memory_readb(machine, get_hl_helper(machine));
}

void ld_nn_a(ElwindMachine* machine){
    memory_writeb(machine, nn_helper(machine), machine->registers.a);
}

void ld_hl_n(ElwindMachine* machine){
    memory_writeb(machine, get_hl_helper(machine), n_helper(machine));
}

void ldh_n_a(ElwindMachine* machine){
    memory_writeb(machine, 0xFF00 + n_helper(machine), machine->registers.a);
}

void ldh_c_a(ElwindMachine* machine){
    memory_writeb(machine, 0xFF00 + machine->registers.c, machine->registers.a);
}

void ldh_a_n(ElwindMachine* machine){
    machine->registers.a = memory_readb(machine, 0xFF00 + n_helper(machine));
}

void ld_d_n(ElwindMachine* machine){
    machine->registers.d = n_helper(machine);
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

void ldi_a_hl(ElwindMachine* machine){
    machine->registers.a = memory_readb(machine, get_hl_helper(machine));
    set_hl_helper(machine, get_hl_helper(machine) + 1);
    if (get_hl_helper(machine) == 0){
        machine->registers.f |= BIT(FLAG_CARRY);
    }
    else machine->registers.f &= ~(BIT(FLAG_CARRY));
    zero_helper(machine, get_hl_helper(machine));
}

void inc_c(ElwindMachine* machine){
    inc_helper(machine, &machine->registers.c);
}

void call_nn(ElwindMachine* machine){
    machine->registers.sp -= 2;
    memory_writeb(machine, machine->registers.sp, (machine->registers.pc + 3) & 0xFF);
    memory_writeb(machine, machine->registers.sp+1, (machine->registers.pc + 3) >> 8);
    machine->registers.pc = nn_helper(machine);
}

void push_de(ElwindMachine* machine){
    machine->registers.sp -= 2;
    memory_writeb(machine, machine->registers.sp, (get_de_helper(machine)) & 0xFF);
    memory_writeb(machine, machine->registers.sp+1, (get_de_helper(machine)) >> 8);
}

void rst_28(ElwindMachine* machine){
    machine->registers.sp -= 2;
    memory_writeb(machine, machine->registers.sp, (machine->registers.pc + 1) & 0xFF);
    memory_writeb(machine, machine->registers.sp+1, (machine->registers.pc + 1) >> 8);
    machine->registers.pc = 0x28;
}

void and_a(ElwindMachine* machine){
    machine->registers.a = machine->registers.a & machine->registers.a;
    zero_helper(machine, machine->registers.a);
}

void and_c(ElwindMachine* machine){
    machine->registers.a = machine->registers.a & machine->registers.c;
    zero_helper(machine, machine->registers.a);
}

void and_n(ElwindMachine* machine){
    machine->registers.a = machine->registers.a & n_helper(machine);
    zero_helper(machine, machine->registers.a);
}

void xor_a(ElwindMachine* machine){
    machine->registers.a = machine->registers.a ^ machine->registers.a;
    zero_helper(machine, machine->registers.a);
}

void xor_c(ElwindMachine* machine){
    machine->registers.a = machine->registers.a ^ machine->registers.c;
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

void swap_a(ElwindMachine* machine){
    machine->registers.a = ((machine->registers.a & 0xF0) >> 4) | ((machine->registers.a & 0x0F) << 4);
    zero_helper(machine, machine->registers.a);
}

void bit_0_a(ElwindMachine* machine){
    bit_helper(machine, 0, machine->registers.a);
}

void bit_1_a(ElwindMachine* machine){
    bit_helper(machine, 1, machine->registers.a);
}

void res_4_c(ElwindMachine* machine){
    res_helper(machine, 4, &machine->registers.c);
}

void res_5_c(ElwindMachine* machine){
    res_helper(machine, 5, &machine->registers.c);
}

void inc_de(ElwindMachine* machine){
    static uint16_t de;
    de = get_de_helper(machine);
    inc_helper_16(machine, &de);
    set_de_helper(machine, de);
}

void inc_hl(ElwindMachine* machine){
    static uint16_t hl;
    hl = get_hl_helper(machine);
    inc_helper_16(machine, &hl);
    set_hl_helper(machine, hl);
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

void add_a_a(ElwindMachine* machine){
    add_helper(machine, &machine->registers.a, machine->registers.a);
}

void add_hl_de(ElwindMachine* machine){
    uint16_t old_value = get_hl_helper(machine);
    set_hl_helper(machine, get_hl_helper(machine) + get_de_helper(machine));
    if (get_hl_helper(machine) < old_value){
        machine->registers.f |= BIT(FLAG_CARRY);
    }
    else machine->registers.f &= ~(BIT(FLAG_CARRY));
}

void cp_n(ElwindMachine* machine){
    uint8_t n = n_helper(machine);
    printf("n: 0x%x\n", n);
    if (machine->registers.a == n) machine->registers.f |= BIT(FLAG_ZERO);
    else machine->registers.f &= ~(BIT(FLAG_ZERO));
    if (machine->registers.a < n) machine->registers.f |= BIT(FLAG_CARRY);
    else machine->registers.f &= ~(BIT(FLAG_CARRY));
    if ((machine->registers.a & 0x0f) < (n & 0x0f)) machine->registers.f |= BIT(FLAG_HC);
    else machine->registers.f &= ~(BIT(FLAG_HC));
    machine->registers.f |= BIT(FLAG_NEGATIVE);
}

void ret(ElwindMachine* machine){
    machine->registers.pc = (memory_readb(machine, machine->registers.sp + 1) << 8) | memory_readb(machine, machine->registers.sp);
    machine->registers.sp += 2;
}

void pop_hl(ElwindMachine* machine){
    set_hl_helper(machine, (memory_readb(machine, machine->registers.sp + 1) << 8) | memory_readb(machine, machine->registers.sp));
    machine->registers.sp += 2;
}

void exec_prefixed(ElwindMachine* machine){
    ElwindInstruction Instruction = PrefixedInstructions[machine->memory.Memory[machine->registers.pc+1]];
    if (!(Instruction.Execute == NULL)){
        printf("0x%x: ", machine->registers.pc+1);
        printf("%s", Instruction.info);
        Instruction.Execute(machine);
        machine->registers.pc += Instruction.length;
    }
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
    { "INC C\n", 1, inc_c },
    { "DEC C\n", 1, dec_c },
    { "LD C, n\n", 2, ld_c_n },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "LD DE, nn\n", 3, ld_de_nn },
    { "NULL", 1, NULL },
    { "INC DE\n", 1, inc_de },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "LD D, n\n", 2, ld_d_n },
    { "NULL", 1, NULL },
    { "JR n\n", 2, jr_n },
    { "ADD HL, DE\n", 1, add_hl_de },
    { "LD A, (DE)\n", 1, ld_a_de },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "JR NZ, n\n", 2, jr_nz_n },
    { "LD HL, nn\n", 3, ld_hl_nn },
    { "LDI (HL), A\n", 1, ldi_hl_a },
    { "INC HL\n", 1, inc_hl },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "LDI (HL), A\n", 1, ldi_hl_a },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "CPL\n", 1, cpl },
    { "NULL", 1, NULL },
    { "LD SP, nn\n", 3, ld_sp_nn },
    { "LDD (HL), A\n", 1, ldd_hl_a },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "LD (HL), n\n", 2, ld_hl_n },
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
    { "LD B, A\n", 1, ld_b_a },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "LD C, A\n", 1, ld_c_a },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "LD D, (HL)\n", 1, ld_d_hl },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "LD E, (HL)\n", 1, ld_e_hl },
    { "LD E, A\n", 1, ld_e_a },
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
    { "LD A, C\n", 1, ld_a_c },
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
    { "ADD A, A\n", 1, add_a_a },
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
    { "AND C\n", 1, and_c },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "AND A\n", 1, and_a },
    { "NULL", 1, NULL },
    { "XOR C\n", 1, xor_c },
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
    { "RET\n", 0, ret },
    { "NULL", 1, NULL },
    { "PREFIX\n", 1, exec_prefixed },
    { "NULL", 1, NULL },
    { "CALL nn\n", 0, call_nn },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "PUSH DE\n", 1, push_de },
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
    { "POP HL\n", 1, pop_hl },
    { "LDH (C), A\n", 1, ldh_c_a },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "AND n\n", 2, and_n },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "JP (HL)\n", 0, jp_hl },
    { "LD (nn), A\n", 3, ld_nn_a },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "RST 28\n", 0, rst_28 },
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
    { "EI\n", 1, ei },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "CP n\n", 2, cp_n },
    { "NULL", 1, NULL },
};

const ElwindInstruction PrefixedInstructions[256] = {
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
    { "SWAP A\n", 1, swap_a },
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
};
