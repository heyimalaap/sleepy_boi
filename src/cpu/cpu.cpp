#include "cpu.h"

CPU::CPU(MMU& mmu)
    : m_mmu(mmu), m_af(m_a, m_f), m_bc(m_b, m_c), m_de(m_d, m_e),
      m_hl(m_h, m_l) {}

uint8_t CPU::get_uint8_pc() {
    uint16_t addr = m_pc;
    m_pc = m_pc + 1;
    return m_mmu.read_byte(addr);
}

int8_t CPU::get_int8_pc() {
    uint16_t addr = m_pc;
    m_pc = m_pc + 1;
    return (int8_t)m_mmu.read_byte(addr);
}

uint16_t CPU::get_uint16_pc() {
    uint16_t val = m_mmu.read_byte(m_pc);
    m_pc = m_pc + 1;
    val |= (m_mmu.read_byte(m_pc) << 8);
    m_pc = m_pc + 1;
    return val;
}

int CPU::execute_next_opcode() {
    if (m_interrupt_waiting)
        return 1;

    const CONDITION_FLAG cc_arr[] = {CONDITION_FLAG::NZ, CONDITION_FLAG::Z, CONDITION_FLAG::NC, CONDITION_FLAG::C};
    const R8 r8_arr[] = {R8::B, R8::C, R8::D, R8::E, R8::H, R8::L, R8::$HL, R8::A};
    const R16_GRP1 r16_grp1_arr[] = {R16_GRP1::BC, R16_GRP1::DE, R16_GRP1::HL, R16_GRP1::SP};
    const R16_GRP2 r16_grp2_arr[] = {R16_GRP2::BC, R16_GRP2::DE, R16_GRP2::HL_PLUS, R16_GRP2::HL_MINUS};
    const R16_GRP3 r16_grp3_arr[] = {R16_GRP3::BC, R16_GRP3::DE, R16_GRP3::HL, R16_GRP3::AF};
    void (CPU::*op_grp1_arr[])() = {&CPU::op_rlca, &CPU::op_rrca, &CPU::op_rla, &CPU::op_rra, &CPU::op_daa, &CPU::op_cpl, &CPU::op_scf, &CPU::op_ccf};
    void (CPU::*op_grp2r_arr[])(R8) = {&CPU::op_add_A_r, &CPU::op_adc_A_r, &CPU::op_sub_A_r, &CPU::op_sbc_A_r, &CPU::op_and_A_r, &CPU::op_xor_A_r, &CPU::op_or_A_r, &CPU::op_cp_A_r};
    void (CPU::*op_grp2n_arr[])(uint8_t) = {&CPU::op_add_A_n, &CPU::op_adc_A_n, &CPU::op_sub_A_n, &CPU::op_sbc_A_n, &CPU::op_and_A_n, &CPU::op_xor_A_n, &CPU::op_or_A_n, &CPU::op_cp_A_n};
    void (CPU::*op_grp3_arr[])(R8) = {&CPU::op_rlc_r, &CPU::op_rrc_r, &CPU::op_rl_r, &CPU::op_rr_r, &CPU::op_sla_r, &CPU::op_sra_r, &CPU::op_swap_r, &CPU::op_srl_r};

    int opcode = get_uint8_pc();
    if (opcode == 0x00) {
        // nop
        op_nop();
        return unprefixed_opcode_cycles_no_branch[opcode];
    } else if (opcode == 0x08) {
        // ld (u16), sp
        uint16_t nn = get_uint16_pc();
        op_ld_$nn_sp(nn);
        return unprefixed_opcode_cycles_no_branch[opcode];
    } else if (opcode == 0x10) {
        // stop
        op_stop();
        return unprefixed_opcode_cycles_no_branch[opcode];
    } else if (opcode == 0x18) {
        // jr (uncoditional)
        const int8_t n = get_int8_pc();
        op_jr_n(n);
        return unprefixed_opcode_cycles_branch[opcode];
    } else if ((opcode & 0b11100111) == 0x20) {
        // jr (conditional)
        const CONDITION_FLAG cc = cc_arr[(opcode >> 3) & 0b11];
        const int8_t n = get_int8_pc();
        bool branch = op_jr_cc_n(cc, n);
        if (branch)
            return unprefixed_opcode_cycles_branch[opcode];
        else
            return unprefixed_opcode_cycles_no_branch[opcode];
    } else if ((opcode & 0b11001111) == 0x01) {
        // ld r16, u16
        const uint16_t nn = get_uint16_pc();
        const R16_GRP1 rr = r16_grp1_arr[(opcode >> 4) & 0b11];
        op_ld_rr_nn(rr, nn);
        return unprefixed_opcode_cycles_no_branch[opcode];
    } else if ((opcode & 0b11001111) == 0x09) {
        // add HL, r16
        const R16_GRP1 rr = r16_grp1_arr[(opcode >> 4) & 0b11];
        op_add_hl_rr(rr);
        return unprefixed_opcode_cycles_no_branch[opcode];
    } else if ((opcode & 0b11001111) == 0x02) {
        // ld (r16), A
        const R16_GRP2 rr = r16_grp2_arr[(opcode >> 4) & 0b11];
        op_ld_$rr_A(rr);
        return unprefixed_opcode_cycles_no_branch[opcode];
    } else if ((opcode & 0b11001111) == 0x0A) {
        // ld A, (r16)
        const R16_GRP2 rr = r16_grp2_arr[(opcode >> 4) & 0b11];
        op_ld_A_$rr(rr);
        return unprefixed_opcode_cycles_no_branch[opcode];
    } else if ((opcode & 0b11001111) == 0x03) {
        // inc r16
        const R16_GRP1 rr = r16_grp1_arr[(opcode >> 4) & 0b11];
        op_inc_rr(rr);
        return unprefixed_opcode_cycles_no_branch[opcode];
    } else if ((opcode & 0b11001111) == 0x0B) {
        // dec r16
        const R16_GRP1 rr = r16_grp1_arr[(opcode >> 4) & 0b11];
        op_dec_rr(rr);
        return unprefixed_opcode_cycles_no_branch[opcode];
    } else if ((opcode & 0b11000111) == 0x04) {
        // inc r8
        const R8 r = r8_arr[(opcode >> 3) & 0b111];
        op_inc_r(r);
        return unprefixed_opcode_cycles_no_branch[opcode];
    } else if ((opcode & 0b11000111) == 0x05) {
        // dec r8
        const R8 r = r8_arr[(opcode >> 3) & 0b111];
        op_dec_r(r);
        return unprefixed_opcode_cycles_no_branch[opcode];
    } else if ((opcode & 0b11000111) == 0x06) {
        // ld r8, u8
        const R8 r = r8_arr[(opcode >> 3) & 0b111];
        uint8_t n = get_uint8_pc();
        op_ld_r_n(r, n);
        return unprefixed_opcode_cycles_no_branch[opcode];
    } else if ((opcode & 0b11000111) == 0x07) {
        // single byte opcode grp1
        (this->*op_grp1_arr[(opcode >> 3) & 0b111])();
        return unprefixed_opcode_cycles_no_branch[opcode];
    } else if (opcode == 0x76) {
        // halt
        op_halt();
        return unprefixed_opcode_cycles_branch[opcode];
    } else if ((opcode & 0b11000000) == 0x40) {
        // ld r8, r8
        const R8 r_src = r8_arr[opcode & 0b111];
        const R8 r_dst = r8_arr[(opcode >> 3) & 0b111];
        op_ld_r_r(r_dst, r_src);
        return unprefixed_opcode_cycles_no_branch[opcode];
    } else if ((opcode & 0b11000000) == 0x80) {
        // alu A, r8
        const R8 r = r8_arr[opcode & 0b111];
        (this->*op_grp2r_arr[(opcode >> 3) & 0b111])(r);
        return unprefixed_opcode_cycles_no_branch[opcode];
    } else if ((opcode & 0b11100111) == 0xC0) {
        // ret condition
        const CONDITION_FLAG cc = cc_arr[(opcode >> 3) & 0b11];
        bool branch = op_ret_cc(cc);
        if (branch)
            return unprefixed_opcode_cycles_branch[opcode];
        else
            return unprefixed_opcode_cycles_no_branch[opcode];
    } else if (opcode == 0xE0) {
        // ld ($ff00 + u8), A
        const uint8_t n = get_uint8_pc();
        op_ld_$n_A(n);
        return unprefixed_opcode_cycles_no_branch[opcode];
    } else if (opcode == 0xE8) {
        // add sp, i8
        const int8_t n = get_int8_pc();
        op_add_sp_n(n);
        return unprefixed_opcode_cycles_no_branch[opcode];
    } else if (opcode == 0xF0) {
        // ld A, ($ff00 + u8)
        const uint8_t n = get_uint8_pc();
        op_ld_A_$n(n);
        return unprefixed_opcode_cycles_no_branch[opcode];
    } else if (opcode == 0xF8) {
        // ld HL, sp + i8
        const int8_t n = get_int8_pc();
        op_ld_HL_SP_plus_n(n);
        return unprefixed_opcode_cycles_no_branch[opcode];
    } else if ((opcode & 0b11001111) == 0xC1) {
        // pop r16
        const R16_GRP3 rr = r16_grp3_arr[(opcode >> 4) & 0b11];
        op_pop_rr(rr);
        return unprefixed_opcode_cycles_no_branch[opcode];
    } else if (opcode == 0xC9) {
        // ret
        op_ret();
        return unprefixed_opcode_cycles_branch[opcode];
    } else if (opcode == 0xD9) {
        // reti
        op_reti();
        return unprefixed_opcode_cycles_branch[opcode];
    } else if (opcode == 0xE9) {
        // jp HL
        op_jp_hl();
        return unprefixed_opcode_cycles_branch[opcode];
    } else if (opcode == 0xF9) {
        // ld sp, HL
        op_ld_SP_HL();
        return unprefixed_opcode_cycles_no_branch[opcode];
    } else if ((opcode & 0b11100111) == 0xC2) {
        // jp condition
        const CONDITION_FLAG cc = cc_arr[(opcode >> 3) & 0b11];
        const uint16_t nn = get_uint16_pc();
        bool branch = op_jp_cc_nn(cc, nn);
        if (branch)
            return unprefixed_opcode_cycles_branch[opcode];
        else
            return unprefixed_opcode_cycles_no_branch[opcode];
    } else if (opcode == 0xE2) {
        // ld ($ff00 + C), A
        op_ld_$C_A();
        return unprefixed_opcode_cycles_no_branch[opcode];
    } else if (opcode == 0xEA) {
        // ld (u16), A
        const uint16_t nn = get_uint16_pc();
        op_ld_$nn_A(nn);
        return unprefixed_opcode_cycles_no_branch[opcode];
    } else if (opcode == 0xF2) {
        // ld A, ($ff00 + C)
        op_ld_A_$C();
        return unprefixed_opcode_cycles_no_branch[opcode];
    } else if (opcode == 0xFA) {
        // ld A, (u16)
        const uint16_t nn = get_uint16_pc();
        op_ld_A_$nn(nn);
        return unprefixed_opcode_cycles_no_branch[opcode];
    } else if (opcode == 0xC3) {
        // jp u16
        const uint16_t nn = get_uint16_pc();
        op_jp_nn(nn);
        return unprefixed_opcode_cycles_branch[opcode];
    } else if (opcode == 0xCB) {
        // cb prefix
        opcode = get_uint8_pc();
        const R8 r = r8_arr[opcode & 0b111];
        const int b = (opcode >> 3) & 0b111;
        if ((opcode >> 6) == 0) {
            (this->*op_grp3_arr[b])(r);
        } else if ((opcode >> 6) == 1) {
            op_bit_b_r(b, r);
        } else if ((opcode >> 6) == 2) {
            op_res_b_r(b, r);
        } else if ((opcode >> 6) == 3) {
            op_set_b_r(b, r);
        }
        return cbprefix_opcode_cycles[opcode];
    } else if (opcode == 0xF3) {
        // di
        op_di();
        return unprefixed_opcode_cycles_no_branch[opcode];
    } else if (opcode == 0xFB) {
        // ei
        op_ei();
        return unprefixed_opcode_cycles_no_branch[opcode];
    } else if ((opcode & 0b11100111) == 0xC4) {
        // call condition
        const CONDITION_FLAG cc = cc_arr[(opcode >> 3) & 0b11];
        const uint16_t nn = get_uint16_pc();
        bool branch = op_call_cc_nn(cc, nn);
        if (branch)
            return unprefixed_opcode_cycles_branch[opcode];
        else
            return unprefixed_opcode_cycles_no_branch[opcode];
    } else if ((opcode & 0b11001111) == 0xC5) {
        // push r16
        const R16_GRP3 rr = r16_grp3_arr[(opcode >> 4) & 0b11];
        op_push_rr(rr);
        return unprefixed_opcode_cycles_no_branch[opcode];
    } else if (opcode == 0xCD) {
        // call u16
        const uint16_t nn = get_uint16_pc();
        op_call_nn(nn);
        return unprefixed_opcode_cycles_branch[opcode];
    } else if ((opcode & 0b11000111) == 0xC6) {
        // alu a, u8
        const uint8_t n = get_uint8_pc();
        (this->*op_grp2n_arr[(opcode >> 3) & 0b111])(n);
        return unprefixed_opcode_cycles_no_branch[opcode];
    } else if ((opcode & 0b11000111) == 0xC7) {
        // RST (00exp000)
        op_rst_n(opcode & 0b00111000);
        return unprefixed_opcode_cycles_branch[opcode];
    } else {
        // TODO: Trigger debugger trap or something?
        return -1;
    }
}

InterruptController& CPU::interrupt_controller() {
    return m_interrupt_controller;
}

void CPU::handle_interrupts() {
    if (m_interrupt_enable) {
        for (auto interrupt : {InterruptController::VBLANK, InterruptController::LCD, InterruptController::TIMER, InterruptController::JOYPAD}) {
            if (m_interrupt_controller.check_requested(interrupt) & m_interrupt_controller.check_requested(interrupt))
                service_interrupt(interrupt);
        }
    }
}

void CPU::service_interrupt(InterruptController::InterruptType type) {
    constexpr uint16_t VBLANK_INT_VECTOR = 0x0040;
    constexpr uint16_t LCD_INT_VECTOR = 0x0048;
    constexpr uint16_t TIMER_INT_VECTOR = 0x0050;
    constexpr uint16_t JOYPAD_INT_VECTOR = 0x0060;

    m_interrupt_enable = false;
    m_interrupt_controller.finished_service(type);

    uint16_t old_pc = m_pc;
    m_sp = m_sp - 1;
    m_mmu.write_byte(m_sp, (old_pc & 0xff00) >> 8);
    m_sp = m_sp - 1;
    m_mmu.write_byte(m_sp, (old_pc & 0xff));

    switch (type) {
    case InterruptController::VBLANK:
        m_pc = VBLANK_INT_VECTOR;
        break;
    case InterruptController::LCD:
        m_pc = LCD_INT_VECTOR;
        break;
    case InterruptController::TIMER:
        m_pc = TIMER_INT_VECTOR;
        break;
    case InterruptController::JOYPAD:
        m_pc = JOYPAD_INT_VECTOR;
        break;
    }
}

void CPU::set_register(const CPU::R8 reg, const uint8_t value) {
    switch (reg) {
    case CPU::R8::$HL:
        m_mmu.write_byte(m_hl, value);
        break;
    case CPU::R8::A:
        m_a = value;
        break;
    case CPU::R8::B:
        m_b = value;
        break;
    case CPU::R8::C:
        m_c = value;
        break;
    case CPU::R8::D:
        m_d = value;
        break;
    case CPU::R8::E:
        m_e = value;
        break;
    case CPU::R8::H:
        m_h = value;
        break;
    case CPU::R8::L:
        m_l = value;
        break;
    }
}

uint8_t CPU::get_register(const CPU::R8 reg) {
    switch (reg) {
    case CPU::R8::$HL:
        return m_mmu.read_byte(m_hl);
    case CPU::R8::A:
        return m_a;
    case CPU::R8::B:
        return m_b;
    case CPU::R8::C:
        return m_c;
    case CPU::R8::D:
        return m_d;
    case CPU::R8::E:
        return m_e;
    case CPU::R8::H:
        return m_h;
    case CPU::R8::L:
        return m_l;
    }

}

void CPU::set_register(const CPU::R16_GRP1 reg, const uint16_t value) {
    switch (reg) {
    case CPU::R16_GRP1::BC:
        m_bc = value;
        break;
    case CPU::R16_GRP1::DE:
        m_de = value;
        break;
    case CPU::R16_GRP1::HL:
        m_hl = value;
        break;
    case CPU::R16_GRP1::SP:
        m_sp = value;
        break;
    }

}

uint16_t CPU::get_register(const CPU::R16_GRP1 reg) {
    switch (reg) {
    case CPU::R16_GRP1::BC:
        return m_bc;
    case CPU::R16_GRP1::DE:
        return m_de;
    case CPU::R16_GRP1::HL:
        return m_hl;
    case CPU::R16_GRP1::SP:
        return m_sp;
    }
}

void CPU::set_register(const CPU::R16_GRP2 reg, const uint16_t value) {
    switch (reg) {
    case CPU::R16_GRP2::BC:
        m_bc = value;
        break;
    case CPU::R16_GRP2::DE:
        m_de = value;
        break;
    case CPU::R16_GRP2::HL_MINUS:
        m_hl = value;
        m_hl = m_hl - 1;
        break;
    case CPU::R16_GRP2::HL_PLUS:
        m_hl = value;
        m_hl = m_hl + 1;
        break;
    }
}

uint16_t CPU::get_register(const CPU::R16_GRP2 reg) {
    switch (reg) {
    case CPU::R16_GRP2::BC:
        return m_bc;
    case CPU::R16_GRP2::DE:
        return m_de;
    case CPU::R16_GRP2::HL_MINUS:
        m_hl = m_hl - 1;
        return (m_hl + 1);
    case CPU::R16_GRP2::HL_PLUS:
        m_hl = m_hl + 1;
        return (m_hl - 1);
    }
}

void CPU::set_register(const CPU::R16_GRP3 reg, const uint16_t value) {
    switch (reg) {
    case CPU::R16_GRP3::AF:
        m_af = value;
        break;
    case CPU::R16_GRP3::BC:
        m_bc = value;
        break;
    case CPU::R16_GRP3::DE:
        m_de = value;
        break;
    case CPU::R16_GRP3::HL:
        m_hl = value;
        break;
    }
}

uint16_t CPU::get_register(const CPU::R16_GRP3 reg) {
    switch (reg) {
    case CPU::R16_GRP3::AF:
        return m_af;
    case CPU::R16_GRP3::BC:
        return m_bc;
    case CPU::R16_GRP3::DE:
        return m_de;
    case CPU::R16_GRP3::HL:
        return m_hl;
    }
}

bool CPU::check_condition(const CONDITION_FLAG flag) {
    switch (flag) {
    case CONDITION_FLAG::C:
        return m_f.c() == true;
    case CONDITION_FLAG::NC:
        return m_f.c() == false;
    case CONDITION_FLAG::Z:
        return m_f.z() == true;
    case CONDITION_FLAG::NZ:
        return m_f.z() == false;
    }
}

//----------------------------------------
// Special instructions
//----------------------------------------

void CPU::op_nop() {
    // do nothing. this is intentional.
}

void CPU::op_stop() {
    // TODO: Implement this later
}

void CPU::op_halt() {
    m_interrupt_waiting = true;
}

void CPU::op_swap_r(const R8 r) {
    // swap r
    // flags: z - set if result if 0
    //        n - reset
    //        h - reset
    //        c - reset
    uint8_t reg = get_register(r);
    reg = ((reg & 0xf) << 4) | ((reg & 0xf0) >> 4);
    set_register(r, reg);
    m_f.setZ(reg == 0);
    m_f.setN(false);
    m_f.setH(false);
    m_f.setC(false);
}

void CPU::op_daa() {
    // daa
    // flags: z - set if A register is 0
    //        n - not affected
    //        h - reset
    //        c - set if the result was > 0x99
    int a = m_a;
    int b = 0;
    bool carry_overflow = false;
    if (m_f.h() || (!m_f.n() && (a & 0xF) > 0x9))
        b |= 0x06;
    if (m_f.c() || (!m_f.n() && (a & 0xFF) > 0x99)) {
        b |= 0x60;
        carry_overflow = true;
    }
    a += m_f.n()? -b : b;
    m_a = a & 0xFF;
    m_f.setZ((uint8_t)(m_a) == 0);
    m_f.setH(false);
    m_f.setC(carry_overflow);
}

void CPU::op_cpl() {
    // cpl
    // flags: z - not affected
    //        n - set
    //        h - set
    //        c - not affected
    uint8_t a = m_a;
    a = ~a;
    m_a = a;
    m_f.setN(true);
    m_f.setH(true);
}

void CPU::op_ccf() {
    // ccf
    // flags: z - not affected
    //        n - reset
    //        h - reset
    //        c - complemented
    m_f.setN(false);
    m_f.setH(false);
    m_f.setC(!m_f.c());
}

void CPU::op_scf() {
    // scf
    // flags: z - not affected
    //        n - reset
    //        h - reset
    //        c - set
    m_f.setN(false);
    m_f.setH(false);
    m_f.setC(true);
}

void CPU::op_di() {
    m_interrupt_enable = false;
}

void CPU::op_ei() {
    m_interrupt_enable = true;
}



//----------------------------------------
// 8-bit load instructions
//----------------------------------------

void CPU::op_ld_r_n(const R8 r, uint8_t n) {
    // ld r, n
    set_register(r, n);
}

void CPU::op_ld_r_r(const R8 r_dst, const R8 r_src) {
    // ld r, r
    uint8_t data = get_register(r_src);
    set_register(r_dst, data);
}

void CPU::op_ld_A_$rr(const R16_GRP2 rr) {
    // ld A, (rr)
    m_a = m_mmu.read_byte(get_register(rr));
}

void CPU::op_ld_A_$nn(const uint16_t nn) {
    // ld A, (nn)
    m_a = m_mmu.read_byte(nn);
}

void CPU::op_ld_$rr_A(const R16_GRP2 rr) {
    // ld (rr), A
    m_mmu.write_byte(get_register(rr), m_a);
}

void CPU::op_ld_$nn_A(const uint16_t nn) {
    // ld (nn), A
    m_mmu.write_byte(nn, m_a);
}

void CPU::op_ld_A_$C() {
    // ld A, (0xff00 + C)
    m_a = m_mmu.read_byte(0xFF00 + m_c);
}

void CPU::op_ld_$C_A() {
    // ld (0xff00 + C), A
    m_mmu.write_byte(0xFF00 + m_c, m_a);
}

void CPU::op_ld_$n_A(const uint8_t n) {
    // ld (0xff00 + n), A
    m_mmu.write_byte(0xFF00 + n, m_a);
}

void CPU::op_ld_A_$n(const uint8_t n) {
    // ld A, (0xff00 + n)
    m_a = m_mmu.read_byte(0xFF00 + n);
}



//----------------------------------------
// 16-bit load instructions
//----------------------------------------

void CPU::op_ld_rr_nn(const CPU::R16_GRP1 rr, const uint16_t nn) {
    // ld rr, nn
    set_register(rr, nn);
}

void CPU::op_ld_SP_HL() {
    // ld SP, HL
    m_sp = m_hl;
}

void CPU::op_ld_HL_SP_plus_n(const int8_t n) {
    // ld HL, SP+n
    // flags: z - reset
    //        n - reset
    //        h - according to operation
    //        c - according to operation
    m_hl = m_sp + n;
    int a = m_sp, b = n;
    m_f.setZ(false);
    m_f.setN(false);
    m_f.setH(((a & 0xF) + (b & 0xF)) > 0xF);
    m_f.setC((a + b) > 0xFF);
}

void CPU::op_ld_$nn_sp(uint16_t nn) {
    // ld (nn), sp
    m_mmu.write_byte(nn  , m_sp &  0xFF);
    m_mmu.write_byte(nn+1, m_sp >> 8   );
}

void CPU::op_push_rr(const R16_GRP3 rr) {
    // push rr
    uint16_t rr_value = get_register(rr);
    m_sp = m_sp - 1;
    m_mmu.write_byte(m_sp, (rr_value & 0xff00) >> 8);
    m_sp = m_sp - 1;
    m_mmu.write_byte(m_sp, (rr_value & 0xff));
}

void CPU::op_pop_rr(const R16_GRP3 rr) {
    // pop rr
    uint16_t value = m_mmu.read_byte(m_sp);
    m_sp = m_sp + 1;
    value |= ((uint16_t)m_mmu.read_byte(m_sp)) << 8;
    m_sp = m_sp + 1;
    set_register(rr, value);
}



//----------------------------------------
// 8-bit alu instruction
//----------------------------------------

void CPU::op_add_A_r(const R8 r) {
    // add A, r
    // flags: z - set if result = 0
    //        n - reset
    //        h - set if carry from bit 3
    //        c - set if carry from bit 7
    int a = m_a;
    int b = get_register(r);
    m_a = a + b;
    m_f.setZ((uint8_t)(m_a) == 0);
    m_f.setN(false);
    m_f.setH(((a & 0xF) + (b & 0xF)) > 0xF);
    m_f.setC(((a & 0xFF) + (b & 0xFF)) > 0xFF);
}

void CPU::op_add_A_n(const uint8_t n) {
    // add A, r
    // flags: z - set if result = 0
    //        n - reset
    //        h - set if carry from bit 3
    //        c - set if carry from bit 7
    int a = m_a;
    int b = n;
    m_a = a + b;
    m_f.setZ((uint8_t)(m_a) == 0);
    m_f.setN(false);
    m_f.setH(((a & 0xF) + (b & 0xF)) > 0xF);
    m_f.setC(((a & 0xFF) + (b & 0xFF)) > 0xFF);
}

void CPU::op_adc_A_r(const R8 r) {
    // adc A, r
    // flags: z - set if result = 0
    //        n - reset
    //        h - set if carry from bit 3
    //        c - set if carry from bit 7
    int a = m_a;
    int b = get_register(r) + (int)m_f.c();
    m_f.setZ((uint8_t)(m_a) == 0);
    m_f.setN(false);
    m_f.setH(((a & 0xF) + (b & 0xF)) > 0xF);
    m_f.setC(((a & 0xFF) + (b & 0xFF)) > 0xFF);
}

void CPU::op_adc_A_n(const uint8_t n) {
    // adc A, r
    // flags: z - set if result = 0
    //        n - reset
    //        h - set if carry from bit 3
    //        c - set if carry from bit 7
    int a = m_a;
    int b = n + (int)m_f.c();
    m_a = a + b;
    m_f.setZ((uint8_t)(m_a) == 0);
    m_f.setN(false);
    m_f.setH(((a & 0xF) + (b & 0xF)) > 0xF);
    m_f.setC(((a & 0xFF) + (b & 0xFF)) > 0xFF);
}

void CPU::op_sub_A_r(const R8 r) {
    // sub A, r
    // flags: z - set if result = 0
    //        n - set
    //        h - set if no borrow from bit 4
    //        c - set if no borrow
    int a = m_a;
    int b = get_register(r);
    m_a = a - b;
    m_f.setZ((uint8_t)(m_a) == 0);
    m_f.setN(true);
    m_f.setH(((a & 0xF) - (b & 0xF)) < 0);
    m_f.setC(((a & 0xFF) - (b & 0xFF)) < 0);
}

void CPU::op_sub_A_n(const uint8_t n) {
    // sub A, n
    // flags: z - set if result = 0
    //        n - set
    //        h - set if no borrow from bit 4
    //        c - set if no borrow
    int a = m_a;
    int b = n;
    m_a = a - b;
    m_f.setZ((uint8_t)(m_a) == 0);
    m_f.setN(true);
    m_f.setH(((a & 0xF) - (b & 0xF)) < 0);
    m_f.setC(((a & 0xFF) - (b & 0xFF)) < 0);
}

void CPU::op_sbc_A_r(const R8 r) {
    // sbc A, r
    // flags: z - set if result = 0
    //        n - set
    //        h - set if no borrow from bit 4
    //        c - set if no borrow
    int a = m_a;
    int b = get_register(r) + (int)m_f.c();
    m_a = a - b;
    m_f.setZ((uint8_t)(m_a) == 0);
    m_f.setN(true);
    m_f.setH(((a & 0xF) - (b & 0xF)) < 0);
    m_f.setC(((a & 0xFF) - (b & 0xFF)) < 0);
}

void CPU::op_sbc_A_n(const uint8_t n) {
    // sub A, n
    // flags: z - set if result = 0
    //        n - set
    //        h - set if no borrow from bit 4
    //        c - set if no borrow
    int a = m_a;
    int b = n + (int)m_f.c();
    m_a = a - b;
    m_f.setZ((uint8_t)(m_a) == 0);
    m_f.setN(true);
    m_f.setH(((a & 0xF) - (b & 0xF)) < 0);
    m_f.setC(((a & 0xFF) - (b & 0xFF)) < 0);
}

void CPU::op_and_A_r(const R8 r) {
    // and A, r
    // flags: z - set if result = 0
    //        n - reset
    //        h - set
    //        c - reset
    uint8_t a = m_a;
    uint8_t b = get_register(r);
    m_a = a & b;
    m_f.setZ((uint8_t)(m_a) == 0);
    m_f.setN(false);
    m_f.setH(true);
    m_f.setC(false);
}

void CPU::op_and_A_n(const uint8_t n) {
    // and A, n
    // flags: z - set if result = 0
    //        n - reset
    //        h - set
    //        c - reset
    uint8_t a = m_a;
    uint8_t b = n;
    m_a = a & b;
    m_f.setZ((uint8_t)(m_a) == 0);
    m_f.setN(false);
    m_f.setH(true);
    m_f.setC(false);
}

void CPU::op_or_A_r(const R8 r) {
    // or A, r
    // flags: z - set if result = 0
    //        n - reset
    //        h - reset
    //        c - reset
    uint8_t a = m_a;
    uint8_t b = get_register(r);
    m_a = a | b;
    m_f.setZ((uint8_t)(m_a) == 0);
    m_f.setN(false);
    m_f.setH(false);
    m_f.setC(false);
}

void CPU::op_or_A_n(const uint8_t n) {
    // or A, n
    // flags: z - set if result = 0
    //        n - reset
    //        h - reset
    //        c - reset
    uint8_t a = m_a;
    uint8_t b = n;
    m_a = a | b;
    m_f.setZ((uint8_t)(m_a) == 0);
    m_f.setN(false);
    m_f.setH(false);
    m_f.setC(false);
}

void CPU::op_xor_A_r(const R8 r) {
    // xor A, r
    // flags: z - set if result = 0
    //        n - reset
    //        h - reset
    //        c - reset
    uint8_t a = m_a;
    uint8_t b = get_register(r);
    m_a = a ^ b;
    m_f.setZ((uint8_t)(m_a) == 0);
    m_f.setN(false);
    m_f.setH(false);
    m_f.setC(false);
}

void CPU::op_xor_A_n(const uint8_t n) {
    // xor A, n
    // flags: z - set if result = 0
    //        n - reset
    //        h - reset
    //        c - reset
    uint8_t a = m_a;
    uint8_t b = n;
    m_a = a ^ b;
    m_f.setZ((uint8_t)(m_a) == 0);
    m_f.setN(false);
    m_f.setH(false);
    m_f.setC(false);
}

void CPU::op_cp_A_r(const R8 r) {
    // cp A, r
    // flags: z - set if result = 0
    //        n - set
    //        h - set if no borrow from bit 4
    //        c - set if no borrow
    int a = m_a;
    int b = get_register(r);
    m_f.setZ((uint8_t)(a - b) == 0);
    m_f.setN(true);
    m_f.setH(((a & 0xF) - (b & 0xF)) < 0);
    m_f.setC(((a & 0xFF) - (b & 0xFF)) < 0);
}

void CPU::op_cp_A_n(const uint8_t n) {
    // cp A, n
    // flags: z - set if result = 0
    //        n - set
    //        h - set if no borrow from bit 4
    //        c - set if no borrow
    int a = m_a;
    int b = n;
    m_f.setZ((uint8_t)(a - b) == 0);
    m_f.setN(true);
    m_f.setH(((a & 0xF) - (b & 0xF)) < 0);
    m_f.setC(((a & 0xFF) - (b & 0xFF)) < 0);
}

void CPU::op_inc_r(const R8 r) {
    // inc r
    // flags: z - set if result = 0
    //        n - reset
    //        h - set if carry from bit 3
    //        c - not affected
    uint8_t a = get_register(r);
    uint8_t b = 1;
    set_register(r, a + b);
    m_f.setZ((uint8_t)(get_register(r)) == 0);
    m_f.setN(false);
    m_f.setH(((a & 0xF) + (b & 0xF)) > 0xF);
}

void CPU::op_dec_r(const R8 r) {
    // dec r
    // flags: z - set if result = 0
    //        n - set
    //        h - set if no borrow from bit 4
    //        c - not affected
    uint8_t a = get_register(r);
    uint8_t b = 1;
    set_register(r, a - b);
    m_f.setZ((uint8_t)(get_register(r)) == 0);
    m_f.setN(true);
    m_f.setH(((a & 0xF) - (b & 0xF)) < 0);
}



//----------------------------------------
// 16-bit alu instructions
//----------------------------------------

void CPU::op_add_hl_rr(const R16_GRP1 rr) {
    // add HL, rr
    // flags: z - not affected
    //        n - reset
    //        h - set if carry from bit 11
    //        c - set if carry from bit 15
    int a = m_hl;
    int b = get_register(rr);
    m_hl = a + b;
    m_f.setN(false);
    m_f.setH(((a & 0xFFF) + (b & 0xFFF)) > 0xFFF);
    m_f.setC(((a & 0xFFFF) + (b & 0xFFFF)) > 0xFFFF);
}

void CPU::op_add_sp_n(const int8_t n) {
    // add sp, n
    // flags: z - reset
    //        n - reset
    //        h - set if carry from bit 11
    //        c - set if carry from bit 15
    int a = m_sp;
    int b = n;
    m_sp = a + n;
    m_f.setZ(false);
    m_f.setN(false);
    m_f.setZ(((a & 0xFFF) + (b & 0xFFF)) > 0xFFF);
    m_f.setC(((a & 0xFFFF) + (b & 0xFFFF)) > 0xFFFF);
}

void CPU::op_inc_rr(const R16_GRP1 rr) {
    // inc rr
    // flags: non-affected
    uint16_t a = get_register(rr) + 1;
    set_register(rr, a);
}

void CPU::op_dec_rr(const R16_GRP1 rr) {
    // dec rr
    // flags: non-affected
    uint16_t a = get_register(rr) - 1;
    set_register(rr, a);
}



//----------------------------------------
// Shift and Rotate instructions
//----------------------------------------

void CPU::op_rlca() {
    // rlca
    // flags: z - reset
    //        n - reset
    //        h - reset
    //        c - old A's 7th bit
    uint8_t a = m_a;
    bool bit7_set = (a & 0b10000000) != 0;
    m_a = (a << 1) | (bit7_set? 1 : 0);
    m_f.setZ(false);
    m_f.setN(false);
    m_f.setH(false);
    m_f.setC(bit7_set);
}

void CPU::op_rla() {
    // rla
    // flags: z - reset
    //        n - reset
    //        h - reset
    //        c - old A's 7th bit
    uint8_t a = m_a;
    bool bit7_set = (a & 0b10000000) != 0;
    m_a = (a << 1) | (m_f.c()? 1 : 0);
    m_f.setZ(false);
    m_f.setN(false);
    m_f.setH(false);
    m_f.setC(bit7_set);
}

void CPU::op_rrca() {
    // rrca
    // flags: z - reset
    //        n - reset
    //        h - reset
    //        c - old A's 0th bit
    uint8_t a = m_a;
    bool bit0_set = (a & 0b00000001) != 0;
    m_a = (a >> 1) | (bit0_set? (1 << 7) : 0);
    m_f.setZ(false);
    m_f.setN(false);
    m_f.setH(false);
    m_f.setC(bit0_set);
}

void CPU::op_rra() {
    // rra
    // flags: z - reset
    //        n - reset
    //        h - reset
    //        c - old A's 0th bit
    uint8_t a = m_a;
    bool bit0_set = (a & 0b00000001) != 0;
    m_a = (a >> 1) | (m_f.c()? (1 << 7) : 0);
    m_f.setZ(false);
    m_f.setN(false);
    m_f.setH(false);
    m_f.setC(bit0_set);
}

void CPU::op_rlc_r(const R8 r) {
    // rlc r
    // flags: z - set if result is 0
    //        n - reset
    //        h - reset
    //        c - old r's 7th bit
    uint8_t a = get_register(r);
    bool bit7_set = (a & 0b10000000) != 0;
    set_register(r, (a << 1) | (bit7_set? 1 : 0));
    m_f.setZ((uint8_t)(m_a) == 0);
    m_f.setN(false);
    m_f.setH(false);
    m_f.setC(bit7_set);
}

void CPU::op_rl_r(const R8 r) {
    // rl r
    // flags: z - set if result is 0
    //        n - reset
    //        h - reset
    //        c - old r's 7th bit
    uint8_t a = get_register(r);
    bool bit7_set = (a & 0b10000000) != 0;
    set_register(r, (a << 1) | (bit7_set? 1 : 0));
    m_f.setZ((uint8_t)(get_register(r)) == 0);
    m_f.setN(false);
    m_f.setH(false);
    m_f.setC(bit7_set);
}

void CPU::op_rrc_r(const R8 r) {
    // rrc r
    // flags: z - set if result is 0
    //        c - reset
    //        h - reset
    //        c - old r's 0th bit
    uint8_t a = get_register(r);
    bool bit0_set = (a & 0b00000001) != 0;
    set_register(r, (a >> 1) | (bit0_set? (1 << 7) : 0));
    m_f.setZ((uint8_t)(get_register(r)) == 0);
    m_f.setN(false);
    m_f.setH(false);
    m_f.setC(bit0_set);
}

void CPU::op_rr_r(const R8 r) {
    // rr r
    // flags: z - set if result is 0
    //        n - reset
    //        h - reset
    //        c - old A's 0th bit
    uint8_t a = get_register(r);
    bool bit0_set = (a & 0b00000001) != 0;
    set_register(r, (a >> 1) | (m_f.c()? (1 << 7) : 0));
    m_f.setZ((uint8_t)(get_register(r)) == 0);
    m_f.setN(false);
    m_f.setH(false);
    m_f.setC(bit0_set);
}

void CPU::op_sla_r(const R8 r) {
    // sla r
    // flags: z - set if result is 0
    //        n - reset
    //        h - reset
    //        c - old r's 7th bit
    uint8_t a = get_register(r);
    bool bit7_set = (a & 0b10000000) != 0;
    set_register(r, a << 1);
    m_f.setZ((uint8_t)(get_register(r)) == 0);
    m_f.setN(false);
    m_f.setH(false);
    m_f.setC(bit7_set);
}

void CPU::op_sra_r(const R8 r) {
    // sra r
    // flags: z - set if result is 0
    //        n - reset
    //        h - reset
    //        c - old r's 0th bit
    uint8_t a = get_register(r);
    bool bit0_set = (a & 0b00000001) != 0;
    bool bit7_set = (a & 0b10000000) != 0;
    set_register(r, (a >> 1) | (bit7_set ? (1 << 7) : 0));
    m_f.setZ((uint8_t)(get_register(r)) == 0);
    m_f.setN(false);
    m_f.setH(false);
    m_f.setC(bit0_set);
}

void CPU::op_srl_r(const R8 r) {
    // srl r
    // flags: z - set if result is 0
    //        n - reset
    //        h - reset
    //        c - old r's 0th bit
    uint8_t a = get_register(r);
    bool bit0_set = (a & 0b00000001) != 0;
    set_register(r, (a >> 1));
    m_f.setZ((uint8_t)(get_register(r)) == 0);
    m_f.setN(false);
    m_f.setH(false);
    m_f.setC(bit0_set);
}



//----------------------------------------
// Bit-set-reset instructions
//----------------------------------------

void CPU::op_bit_b_r(const uint8_t b, const R8 r) {
    // bit b, r
    // flags: z - set if the selected bit is 0
    //        n - reset
    //        h - set
    //        c - not affected
    m_f.setC((get_register(r) & (1 << b)) == 0);
    m_f.setN(false);
    m_f.setH(true);
}

void CPU::op_set_b_r(const uint8_t b, const R8 r) {
    // set b, r
    uint8_t a = get_register(r);
    set_register(r, a | (1 << b));
}

void CPU::op_res_b_r(const uint8_t b, const R8 r) {
    // res b, r
    uint8_t a = get_register(r);
    set_register(r, a & (~(1 << b)));
}



//----------------------------------------
// Control flow instructions
//----------------------------------------

void CPU::op_jp_nn(const uint16_t nn) {
    // jp nn
    m_pc = nn;
}

bool CPU::op_jp_cc_nn(const CONDITION_FLAG cc, const uint16_t nn) {
    // jp cc, nn
    if (check_condition(cc)) {
        m_pc = nn;
        return true;
    }
    return false;
}

void CPU::op_jp_hl() {
    // jp hl
    m_pc = m_hl;
}

void CPU::op_jr_n(const int8_t n) {
    // jr n
    m_pc = m_pc + n;
}

bool CPU::op_jr_cc_n(const CONDITION_FLAG cc, const int8_t n) {
    // jr cc, n
    if (check_condition(cc)) {
        m_pc = m_pc + n;
        return true;
    }
    return false;
}

void CPU::op_call_nn(const uint16_t nn) {
    // call nn
    uint16_t old_pc = m_pc;
    m_sp = m_sp - 1;
    m_mmu.write_byte(m_sp, (old_pc & 0xff00) >> 8);
    m_sp = m_sp - 1;
    m_mmu.write_byte(m_sp, (old_pc & 0xff));
    m_pc = nn;
}

bool CPU::op_call_cc_nn(const CONDITION_FLAG cc, const uint16_t nn) {
    // call cc, nn
    if (check_condition(cc)) {
        uint16_t old_pc = m_pc;
        m_sp = m_sp - 1;
        m_mmu.write_byte(m_sp, (old_pc & 0xff00) >> 8);
        m_sp = m_sp - 1;
        m_mmu.write_byte(m_sp, (old_pc & 0xff));
        m_pc = nn;
        return true;
    }
    return false;
}

void CPU::op_rst_n(const uint8_t n) {
    // rst $0000+n
    op_call_nn(n);
}

void CPU::op_ret() {
    // ret
    uint16_t pop_address = m_mmu.read_byte(m_sp);
    m_sp = m_sp + 1;
    pop_address |= ((uint16_t)m_mmu.read_byte(m_sp)) << 8;
    m_sp = m_sp + 1;
    m_pc = pop_address;
}

bool CPU::op_ret_cc(const CONDITION_FLAG cc) {
    // ret cc
    if (check_condition(cc)) {
        uint16_t pop_address = m_mmu.read_byte(m_sp);
        m_sp = m_sp + 1;
        pop_address |= ((uint16_t)m_mmu.read_byte(m_sp)) << 8;
        m_sp = m_sp + 1;
        m_pc = pop_address;
        return true;
    }
    return false;
}

void CPU::op_reti() {
    // reti
    uint16_t pop_address = m_mmu.read_byte(m_sp);
    m_sp = m_sp + 1;
    pop_address |= ((uint16_t)m_mmu.read_byte(m_sp)) << 8;
    m_sp = m_sp + 1;
    m_pc = pop_address;
    op_ei();
}
