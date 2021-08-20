#include "elwind.h"

uint8_t io_read_handler(ElwindMachine* machine, uint8_t address){
    uint8_t vblank;
    printf("reading from 0xFF%x\n", address);
    switch(address){
        case LCDC_Y:
            vblank = rand() % 153;
            return vblank; // make vblank a bit random
        case LCDC_CTRL:
            return machine->memory.IOMemory[address - 0xFF00];
        default:
            printf("Warning: READING unimplemented I/O region!\n");
            printf("READING I/O register 0x%x is not implemented!\n", address);
            return machine->memory.IOMemory[address - 0xFF00];
    }
}

uint8_t io_write_handler(ElwindMachine* machine, uint8_t address, uint8_t value){
    uint8_t vblank;
    printf("writing to 0xFF%x\n", address);
    switch(address){
        case LCDC_CTRL:
            printf("\nLCD control write!!!\n");
            if (value & BIT(LCDC_CTRL_EN)) printf("Display enable\n");
            else printf("Display disable\n");
            if (value & BIT(LCDC_CTRL_TILEMAP_MAP)) printf("Window layer tilemap at 9C00\n");
            else printf("Window layer tilemap at 9800\n");
            if (value & BIT(LCDC_CTRL_WINDOW_EN)) printf("Window layer enable\n");
            else printf("Window layer disable\n");
            if (value & BIT(LCDC_CTRL_BG_SELECT)) printf("Tiles addressing starts at 8000\n");
            else printf("Tiles addressing starts at 8800\n");
            if (value & BIT(LCDC_CTRL_BG_TILEMAP)) printf("BG tilemap at 9C00\n");
            else printf("BG tilemap at 9800\n");
            if (value & BIT(LCDC_CTRL_SPRITE_SIZE)) printf("8x16 sprites\n");
            else printf("8x8 sprites\n");
            if (value & BIT(LCDC_CTRL_SPRITE_EN)) printf("Sprite enable\n");
            else printf("Sprites off\n");
            if (value & BIT(LCDC_CTRL_BG_WINDOW_PRIORITY)) printf("Window over BG\n");
            else printf("BG over window\n");
            printf("\n");
            machine->memory.IOMemory[address - 0xFF00] = value;
            break;
        case SERIAL_CTRL:
            printf("SERIAL control write!!!\n");
            if (value & BIT(SERIAL_TRANSFER_START)) printf("START transfer!!!\n");
            else printf("STOP transfer!!!\n");            
            if (value & BIT(SERIAL_CLOCK_SPEED)) printf("Fast clock speed\n");
            else printf("Normal clock speed\n");            
            if (value & BIT(SERIAL_CLOCKSOURCE)) printf("Internal clocksource\n");
            else printf("External clocksource\n");
            machine->memory.IOMemory[address - 0xFF00] = value;
            break;
        case SERIAL_WRITE:
            printf("SERIAL SEND BYTE\n");
            printf("byte 0x%x\n", value);
            break;
        default:
            printf("Warning: WRITING unimplemented I/O region!\n");
            printf("WRITING I/O register 0x%x is not implemented!\n", address);
            return 0;
    }
    return 0;
}

uint8_t memory_readb(ElwindMachine* machine, uint16_t address){
    if (address >= 0x8000 && address <= 0x9FFF){
        return machine->memory.VRAM[address - 0x8000];
    }
    if (address >= 0xFE00 && address <= 0xFE9F){
        return machine->memory.OAM[address - 0xFE00];
    }
    if (address >= 0xFF00 && address <= 0xFF7F){
        return io_read_handler(machine, address - 0xFF00);
    }
    // HACK !
    if (address == 0xFFA0) return 1;
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
        io_write_handler(machine, address - 0xFF00, val);
    }
    if (machine->memory.banking_mode == MBC_MODE_MBC1 && 
    (address >= 0x2000 && address <= 0x3FFF) || (address >= 0 && address <= 0x1FFF) ||
    (address >= 0x4000 && address <= 0x5FFF) || (address >= 0x6000 && address <= 0x7FFF)) {
        mbc1_handler(machine, address, val);
    }
    else machine->memory.Memory[address] = val;
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

void set_af_helper(ElwindMachine* machine, uint16_t value){
    machine->registers.a = (value & 0xFF00) >> 8;
    machine->registers.f = value & 0xFF;
}

uint16_t get_af_helper(ElwindMachine* machine){
    return (machine->registers.a << 8) | machine->registers.f;
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

void jp_z_nn(ElwindMachine* machine){
    if (machine->registers.f & BIT(FLAG_ZERO)){
        machine->registers.pc = nn_helper(machine) - 3;
    }
}

void jp_nz_nn(ElwindMachine* machine){
    if (!(machine->registers.f & BIT(FLAG_ZERO))){
        machine->registers.pc = nn_helper(machine) - 3;
    }
}

void jp_hl(ElwindMachine* machine){
    machine->registers.pc = get_hl_helper(machine);
}

void jr_nz_n(ElwindMachine* machine){
    if (!(machine->registers.f & BIT(FLAG_ZERO))){
        machine->registers.pc += ((n_helper(machine) ^ 0x80) - 0x80);
    }
}

void jr_z_n(ElwindMachine* machine){
    if (machine->registers.f & BIT(FLAG_ZERO)){
        machine->registers.pc += ((n_helper(machine) ^ 0x80) - 0x80);
    }
}

void jr_n(ElwindMachine* machine){
    if (n_helper(machine) == 0xFE){
        printf("Infinite loop detected\n");
    }
    machine->registers.pc += ((n_helper(machine) ^ 0x80) - 0x80);
}

void jr_c_n(ElwindMachine* machine){
    if (machine->registers.f & BIT(FLAG_CARRY)){
        machine->registers.pc += ((n_helper(machine) ^ 0x80) - 0x80);
    }
}

void jr_nc_n(ElwindMachine* machine){
    if (!(machine->registers.f & BIT(FLAG_CARRY))){
        machine->registers.pc += ((n_helper(machine) ^ 0x80) - 0x80);
    }
}

void ld_sp_nn(ElwindMachine* machine){
    machine->registers.sp = nn_helper(machine);
}

void ld_hl_nn(ElwindMachine* machine){
    set_hl_helper(machine, nn_helper(machine));
}

void ld_de_a(ElwindMachine* machine){
    memory_writeb(machine, get_de_helper(machine), machine->registers.a);
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

void ld_a_h(ElwindMachine* machine){
    machine->registers.a = machine->registers.h;
}

void ld_a_l(ElwindMachine* machine){
    machine->registers.a = machine->registers.l;
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

void ld_b_b(ElwindMachine* machine){
    machine->registers.b = machine->registers.b;
    zero_helper(machine, machine->registers.b);
}

void ld_c_n(ElwindMachine* machine){
    machine->registers.c = n_helper(machine);
}

void ld_c_a(ElwindMachine* machine){
    machine->registers.c = machine->registers.a;
}

void ld_c_b(ElwindMachine* machine){
    machine->registers.c = machine->registers.b;
}

void ld_e_a(ElwindMachine* machine){
    machine->registers.e = machine->registers.a;
}

void ld_a_de(ElwindMachine* machine){
    machine->registers.a = memory_readb(machine, get_de_helper(machine));
}

void ld_a_hl(ElwindMachine* machine){
    machine->registers.a = memory_readb(machine, get_hl_helper(machine));
}

void ld_a_nn(ElwindMachine* machine){
    machine->registers.a = memory_readb(machine, nn_helper(machine));
}

void ld_e_hl(ElwindMachine* machine){
    machine->registers.e = memory_readb(machine, get_hl_helper(machine));
}

void ld_d_hl(ElwindMachine* machine){
    machine->registers.d = memory_readb(machine, get_hl_helper(machine));
}

void ld_h_hl(ElwindMachine* machine){
    machine->registers.h = memory_readb(machine, get_hl_helper(machine));
}

void ld_h_a(ElwindMachine* machine){
    machine->registers.h = machine->registers.a;
}

void ld_h_l(ElwindMachine* machine){
    machine->registers.h = machine->registers.l;
}

void ld_h_n(ElwindMachine* machine){
    machine->registers.h = n_helper(machine);
}

void ld_l_a(ElwindMachine* machine){
    machine->registers.l = machine->registers.a;
}

void ld_l_b(ElwindMachine* machine){
    machine->registers.l = machine->registers.b;
}

void ld_l_h(ElwindMachine* machine){
    machine->registers.l = machine->registers.h;
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
    printf("reading from 0x%x\n", 0xFF00 + n_helper(machine));
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

void ld_hl_a(ElwindMachine* machine){
    memory_writeb(machine, get_hl_helper(machine), machine->registers.a);
}

void ldi_a_hl(ElwindMachine* machine){
    uint16_t hl = get_hl_helper(machine);
    machine->registers.a = memory_readb(machine, hl);
    hl++;
    set_hl_helper(machine, hl);
    if (hl == 0){
        machine->registers.f |= BIT(FLAG_CARRY);
    }
    else machine->registers.f &= ~(BIT(FLAG_CARRY));
    zero_helper(machine, hl);
}

void inc_a(ElwindMachine* machine){
    inc_helper(machine, &machine->registers.a);
}

void inc_c(ElwindMachine* machine){
    inc_helper(machine, &machine->registers.c);
}

void inc_e(ElwindMachine* machine){
    inc_helper(machine, &machine->registers.e);
}

void inc_l(ElwindMachine* machine){
    inc_helper(machine, &machine->registers.l);
}

void call_nn(ElwindMachine* machine){
    machine->registers.sp -= 2;
    memory_writeb(machine, machine->registers.sp, (machine->registers.pc + 3) & 0xFF);
    memory_writeb(machine, machine->registers.sp+1, (machine->registers.pc + 3) >> 8);
    machine->registers.pc = nn_helper(machine);
}

void call_nz_nn(ElwindMachine* machine){
    if (!(machine->registers.f & BIT(FLAG_ZERO))){
        machine->registers.sp -= 2;
        memory_writeb(machine, machine->registers.sp, (machine->registers.pc + 3) & 0xFF);
        memory_writeb(machine, machine->registers.sp+1, (machine->registers.pc + 3) >> 8);
        machine->registers.pc = nn_helper(machine);
    }
    else machine->registers.pc += 3;
}

void call_z_nn(ElwindMachine* machine){
    if (machine->registers.f & BIT(FLAG_ZERO)){
        machine->registers.sp -= 2;
        memory_writeb(machine, machine->registers.sp, (machine->registers.pc + 3) & 0xFF);
        memory_writeb(machine, machine->registers.sp+1, (machine->registers.pc + 3) >> 8);
        machine->registers.pc = nn_helper(machine);
    }
    else machine->registers.pc += 3;
}

void push_af(ElwindMachine* machine){
    machine->registers.sp -= 2;
    memory_writeb(machine, machine->registers.sp, (get_af_helper(machine)) & 0xFF);
    memory_writeb(machine, machine->registers.sp+1, (get_af_helper(machine)) >> 8);
}

void push_bc(ElwindMachine* machine){
    machine->registers.sp -= 2;
    memory_writeb(machine, machine->registers.sp, (get_bc_helper(machine)) & 0xFF);
    memory_writeb(machine, machine->registers.sp+1, (get_bc_helper(machine)) >> 8);
}

void push_de(ElwindMachine* machine){
    machine->registers.sp -= 2;
    memory_writeb(machine, machine->registers.sp, (get_de_helper(machine)) & 0xFF);
    memory_writeb(machine, machine->registers.sp+1, (get_de_helper(machine)) >> 8);
}

void push_hl(ElwindMachine* machine){
    machine->registers.sp -= 2;
    memory_writeb(machine, machine->registers.sp, (get_hl_helper(machine)) & 0xFF);
    memory_writeb(machine, machine->registers.sp+1, (get_hl_helper(machine)) >> 8);
}

void rst_8(ElwindMachine* machine){
    machine->registers.sp -= 2;
    memory_writeb(machine, machine->registers.sp, (machine->registers.pc + 1) & 0xFF);
    memory_writeb(machine, machine->registers.sp+1, (machine->registers.pc + 1) >> 8);
    machine->registers.pc = 0x8;
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

void bit_6_b(ElwindMachine* machine){
    bit_helper(machine, 6, machine->registers.b);
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

void inc_at_hl(ElwindMachine* machine){
    static uint8_t at_hl;
    at_hl = memory_readb(machine, get_hl_helper(machine));
    inc_helper(machine, &at_hl);
    memory_writeb(machine, get_hl_helper(machine), at_hl);
}

void dec_bc(ElwindMachine* machine){
    static uint16_t bc;
    bc = get_bc_helper(machine);
    dec_helper_16(machine, &bc);
    set_bc_helper(machine, bc);
}

void dec_at_hl(ElwindMachine* machine){
    static uint8_t at_hl;
    at_hl = memory_readb(machine, get_hl_helper(machine));
    dec_helper(machine, &at_hl);
    memory_writeb(machine, get_hl_helper(machine), at_hl);
}

void dec_a(ElwindMachine* machine){
    dec_helper(machine, &machine->registers.a);
}

void dec_b(ElwindMachine* machine){
    dec_helper(machine, &machine->registers.b);
}

void dec_c(ElwindMachine* machine){
    dec_helper(machine, &machine->registers.c);
}

void dec_e(ElwindMachine* machine){
    dec_helper(machine, &machine->registers.e);
}

void dec_l(ElwindMachine* machine){
    dec_helper(machine, &machine->registers.l);
}

void add_a_a(ElwindMachine* machine){
    add_helper(machine, &machine->registers.a, machine->registers.a);
}

void add_a_e(ElwindMachine* machine){
    add_helper(machine, &machine->registers.a, machine->registers.e);
}

void add_a_l(ElwindMachine* machine){
    add_helper(machine, &machine->registers.a, machine->registers.l);
}

void add_a_n(ElwindMachine* machine){
    add_helper(machine, &machine->registers.a, n_helper(machine));
}

void add_hl_de(ElwindMachine* machine){
    uint16_t old_value = get_hl_helper(machine);
    set_hl_helper(machine, get_hl_helper(machine) + get_de_helper(machine));
    if (get_hl_helper(machine) < old_value){
        machine->registers.f |= BIT(FLAG_CARRY);
    }
    else machine->registers.f &= ~(BIT(FLAG_CARRY));
}

void add_hl_bc(ElwindMachine* machine){
    uint16_t old_value = get_hl_helper(machine);
    set_hl_helper(machine, get_hl_helper(machine) + get_bc_helper(machine));
    if (get_hl_helper(machine) < old_value){
        machine->registers.f |= BIT(FLAG_CARRY);
    }
    else machine->registers.f &= ~(BIT(FLAG_CARRY));
}

void ldhl_sp_d(ElwindMachine* machine){
    uint16_t sp = machine->registers.sp;
    sp += n_helper(machine);
    if (machine->registers.sp < sp) machine->registers.f |= BIT(FLAG_CARRY);
    else machine->registers.f &= ~(BIT(FLAG_CARRY));
    set_hl_helper(machine, sp);
}

void cp_n(ElwindMachine* machine){
    uint8_t n = n_helper(machine);
    printf("n: 0x%x, val: 0x%x\n", n, machine->registers.a);
    if (machine->registers.a == n) machine->registers.f |= BIT(FLAG_ZERO);
    else machine->registers.f &= ~(BIT(FLAG_ZERO));
    if (machine->registers.a < n) machine->registers.f |= BIT(FLAG_CARRY);
    else machine->registers.f &= ~(BIT(FLAG_CARRY));
    if ((machine->registers.a & 0x0f) < (n & 0x0f)) machine->registers.f |= BIT(FLAG_HC);
    else machine->registers.f &= ~(BIT(FLAG_HC));
    machine->registers.f |= BIT(FLAG_NEGATIVE);
}

void cp_c(ElwindMachine* machine){
    uint8_t n = machine->registers.c;
    if (machine->registers.a == n) machine->registers.f |= BIT(FLAG_ZERO);
    else machine->registers.f &= ~(BIT(FLAG_ZERO));
    if (machine->registers.a < n) machine->registers.f |= BIT(FLAG_CARRY);
    else machine->registers.f &= ~(BIT(FLAG_CARRY));
    if ((machine->registers.a & 0x0f) < (n & 0x0f)) machine->registers.f |= BIT(FLAG_HC);
    else machine->registers.f &= ~(BIT(FLAG_HC));
    machine->registers.f |= BIT(FLAG_NEGATIVE);
}

void cp_hl(ElwindMachine* machine){
    uint8_t n = memory_readb(machine, get_hl_helper(machine));
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

void reti(ElwindMachine* machine){
    machine->registers.pc = (memory_readb(machine, machine->registers.sp + 1) << 8) | memory_readb(machine, machine->registers.sp);
    printf("returning to 0x%x\n", machine->registers.pc);
    machine->registers.sp += 2;
    ei(machine);
}

void ret_z(ElwindMachine* machine){
    if (machine->registers.f & BIT(FLAG_ZERO)){
        machine->registers.pc = (memory_readb(machine, machine->registers.sp + 1) << 8) | memory_readb(machine, machine->registers.sp);
        machine->registers.sp += 2;
    }
    else machine->registers.pc++;
}

void ret_nz(ElwindMachine* machine){
    if (!(machine->registers.f & BIT(FLAG_ZERO))){
        machine->registers.pc = (memory_readb(machine, machine->registers.sp + 1) << 8) | memory_readb(machine, machine->registers.sp);
        machine->registers.sp += 2;
    }
    else machine->registers.pc++;
}

void ret_nc(ElwindMachine* machine){
    if (!(machine->registers.f & BIT(FLAG_CARRY))){
        machine->registers.pc = (memory_readb(machine, machine->registers.sp + 1) << 8) | memory_readb(machine, machine->registers.sp);
        machine->registers.sp += 2;
    }
    else machine->registers.pc++;
}

void pop_af(ElwindMachine* machine){
    set_af_helper(machine, (memory_readb(machine, machine->registers.sp + 1) << 8) | memory_readb(machine, machine->registers.sp));
    machine->registers.sp += 2;
}

void pop_bc(ElwindMachine* machine){
    set_bc_helper(machine, (memory_readb(machine, machine->registers.sp + 1) << 8) | memory_readb(machine, machine->registers.sp));
    machine->registers.sp += 2;
}

void pop_de(ElwindMachine* machine){
    set_de_helper(machine, (memory_readb(machine, machine->registers.sp + 1) << 8) | memory_readb(machine, machine->registers.sp));
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

void set_3_hl(ElwindMachine* machine){
    uint8_t value = memory_readb(machine, get_hl_helper(machine));
    value = value | BIT(3);
    memory_writeb(machine, get_hl_helper(machine), value);
}

void set_7_a(ElwindMachine* machine){
    machine->registers.a |= BIT(7);
}

void set_7_c(ElwindMachine* machine){
    machine->registers.c |= BIT(7);
}

void sla_a(ElwindMachine* machine){
    machine->registers.a = machine->registers.a << 1;
    zero_helper(machine, machine->registers.a);
}

void halt(ElwindMachine* machine){
    ei(machine);
    return;
}

void rrc_a(ElwindMachine* machine){
    uint16_t a = (machine->registers.a >> 1) + ((machine->registers.a & 1) << 7) + ((machine->registers.a & 1) << 8);
    if (a > 0xFF) machine->registers.f |= BIT(FLAG_CARRY);
    else machine->registers.f &= ~(BIT(FLAG_CARRY));
    a &= 0xFF;
    machine->registers.a = a;
    zero_helper(machine, machine->registers.a);
}

void sub_a_hl(ElwindMachine* machine){
    uint8_t val = memory_readb(machine, get_hl_helper(machine));
    if (machine->registers.a < val) machine->registers.f |= BIT(FLAG_CARRY);
    else machine->registers.f &= ~(BIT(FLAG_CARRY));
    machine->registers.a = machine->registers.a - val;
    zero_helper(machine, machine->registers.a);
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
    { "ADD HL, BC\n", 1, add_hl_bc },
    { "NULL", 1, NULL },
    { "DEC BC\n", 1, dec_bc },
    { "INC C\n", 1, inc_c },
    { "DEC C\n", 1, dec_c },
    { "LD C, n\n", 2, ld_c_n },
    { "RRC A\n", 1, rrc_a },
    { "NULL", 1, NULL },
    { "LD DE, nn\n", 3, ld_de_nn },
    { "LD (DE), A\n", 1, ld_de_a },
    { "INC DE\n", 1, inc_de },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "LD D, n\n", 2, ld_d_n },
    { "NULL", 1, NULL },
    { "JR n\n", 2, jr_n },
    { "ADD HL, DE\n", 1, add_hl_de },
    { "LD A, (DE)\n", 1, ld_a_de },
    { "NULL", 1, NULL },
    { "INC E\n", 1, inc_e },
    { "DEC E\n", 1, dec_e },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "JR NZ, n\n", 2, jr_nz_n },
    { "LD HL, nn\n", 3, ld_hl_nn },
    { "LDI (HL), A\n", 1, ldi_hl_a },
    { "INC HL\n", 1, inc_hl },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "LD H, n\n", 2, ld_h_n },
    { "NULL", 1, NULL },
    { "JR Z, n\n", 2, jr_z_n },
    { "NULL", 1, NULL },
    { "LDI A, (HL)\n", 1, ldi_a_hl },
    { "NULL", 1, NULL },
    { "INC L\n", 1, inc_l },
    { "DEC L\n", 1, dec_l },
    { "NULL", 1, NULL },
    { "CPL\n", 1, cpl },
    { "JR NC, n\n", 2, jr_nc_n },
    { "LD SP, nn\n", 3, ld_sp_nn },
    { "LDD (HL), A\n", 1, ldd_hl_a },
    { "NULL", 1, NULL },
    { "INC (HL)\n", 1, inc_at_hl },
    { "DEC (HL)\n", 1, dec_at_hl },
    { "LD (HL), n\n", 2, ld_hl_n },
    { "NULL", 1, NULL },
    { "JR C, n\n", 2, jr_c_n },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "INC A\n", 1, inc_a },
    { "DEC A\n", 1, dec_a },
    { "LD A, n\n", 2, ld_a_n },
    { "NULL", 1, NULL },
    { "LD B, B\n", 1, ld_b_b },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "LD B, A\n", 1, ld_b_a },
    { "LD C, B\n", 1, ld_c_b },
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
    { "LD H, L\n", 1, ld_h_l },
    { "LD H, (HL)\n", 1, ld_h_hl },
    { "LD H, A\n", 1, ld_h_a },
    { "LD L, B\n", 1, ld_l_b },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "LD L, H\n", 1, ld_l_h },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "LD L, A\n", 1, ld_l_a },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "HALT\n", 0, halt },
    { "LD (HL), A\n", 1, ld_hl_a },
    { "LD A, B\n", 1, ld_a_b },
    { "LD A, C\n", 1, ld_a_c },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "LD A, H\n", 1, ld_a_h },
    { "LD A, L\n", 1, ld_a_l },
    { "LD A, (HL)\n", 1, ld_a_hl },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "ADD A, E\n", 1, add_a_e },
    { "NULL", 1, NULL },
    { "ADD A, L\n", 1, add_a_l },
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
    { "SUB A, (HL)\n", 1, sub_a_hl },
    { "NULL", 1, NULL },
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
    { "CP C\n", 1, cp_c },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "CP (HL)\n", 1, cp_hl },
    { "NULL", 1, NULL },
    { "RET NZ\n", 0, ret_nz },
    { "POP BC\n", 1, pop_bc },
    { "JP NZ, nn\n", 3, jp_nz_nn },
    { "JP nn\n", 0, jp_nn },
    { "CALL NZ, nn", 0, call_nz_nn },
    { "PUSH BC\n", 1, push_bc },
    { "ADD A, n\n", 2, add_a_n },
    { "NULL", 1, NULL },
    { "RET Z\n", 0, ret_z },
    { "RET\n", 0, ret },
    { "JP Z, nn\n", 3, jp_z_nn },
    { "PREFIX\n", 1, exec_prefixed },
    { "CALL Z, nn\n", 0, call_z_nn },
    { "CALL nn\n", 0, call_nn },
    { "NULL", 1, NULL },
    { "RST 8\n", 0, rst_8 },
    { "RET NC\n", 0, ret_nc },
    { "POP DE\n", 1, pop_de },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "PUSH DE\n", 1, push_de },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "RETI\n", 0, reti },
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
    { "PUSH HL\n", 1, push_hl },
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
    { "POP AF\n", 1, pop_af },
    { "NULL", 1, NULL },
    { "DI\n", 1, di },
    { "NULL", 1, NULL },
    { "PUSH AF\n", 1, push_af },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "LDHL SP, d\n", 2, ldhl_sp_d },
    { "NULL", 1, NULL },
    { "LD A, (nn)\n", 3, ld_a_nn },
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
    { "SLA A\n", 1, sla_a },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
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
    { "BIT 6, B\n", 1, bit_6_b },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "SET 3, (HL)\n", 1, set_3_hl },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "SET 7, C\n", 1, set_7_c },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "NULL", 1, NULL },
    { "SET 7, A\n", 1, set_7_a },
};
