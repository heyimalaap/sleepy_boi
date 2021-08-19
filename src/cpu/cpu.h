#ifndef CPU_H
#define CPU_H

#include <stack>
#include "register.h"
#include "../mmu.h"
#include "interrupt_controller.h"

class MMU;
class InterruptController;

class CPU
{
public:
    CPU(MMU& mmu);
    int execute_next_opcode();
    void handle_interrupts();
    InterruptController& interrupt_controller();
    inline void request_interrupt(InterruptController::InterruptType type) {
        m_interrupt_waiting = false;
        m_interrupt_controller.request_service(type);
    }

private:
    Register<uint8_t> m_a;
    FlagRegister m_f;
    Register<uint8_t> m_b, m_c;
    Register<uint8_t> m_d, m_e;
    Register<uint8_t> m_h, m_l;
    Register<uint16_t> m_sp;
    Register<uint16_t> m_pc;

    RegisterPair m_af;
    RegisterPair m_bc;
    RegisterPair m_de;
    RegisterPair m_hl;

    bool m_interrupt_enable = false;
    bool m_interrupt_waiting = false;
    InterruptController m_interrupt_controller;

    MMU& m_mmu;

    friend class Debugger;

    inline void reset() {
        m_a = 0;
        m_b = 0;
        m_c = 0;
        m_d = 0;
        m_e = 0;
        m_h = 0;
        m_l = 0;
        m_sp = 0;
        m_pc = 0;
        m_f = 0;
        m_interrupt_enable = false;
    }

    enum class CONDITION_FLAG {
        NZ, Z, NC, C
    };

    enum class R8 {
        B, C, D, E, H, L, $HL, A
    };

    enum class R16_GRP1 {
        BC, DE, HL, SP
    };

    enum class R16_GRP2 {
        BC, DE, HL_PLUS, HL_MINUS
    };

    enum class R16_GRP3 {
        BC, DE, HL, AF
    };

    const int unprefixed_opcode_cycles_no_branch[256] = {
        4, 12,  8,  8,  4,  4,  8,  4, 20,  8,  8,  8,  4,  4,  8,  4,
        4, 12,  8,  8,  4,  4,  8,  4, 12,  8,  8,  8,  4,  4,  8,  4,
        8, 12,  8,  8,  4,  4,  8,  4,  8,  8,  8,  8,  4,  4,  8,  4,
        8, 12,  8,  8, 12, 12, 12,  4,  8,  8,  8,  8,  4,  4,  8,  4,
        4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
        4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
        4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
        8,  8,  8,  8,  8,  8,  4,  8,  4,  4,  4,  4,  4,  4,  8,  4,
        4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
        4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
        4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
        4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
        8, 12, 12, 16, 12, 16,  8, 16,  8, 16, 12,  4, 12, 24,  8, 16,
        8, 12, 12,  0, 12, 16,  8, 16,  8, 16, 12,  0, 12,  0,  8, 16,
       12, 12,  8,  0,  0, 16,  8, 16, 16,  4, 16,  0,  0,  0,  8, 16,
       12, 12,  8,  4,  0, 16,  8, 16, 12,  8, 16,  4,  0,  0,  8, 16
    };

    const int unprefixed_opcode_cycles_branch[256] = {
        4, 12,  8,  8,  4,  4,  8,  4, 20,  8,  8,  8,  4,  4,  8,  4,
        4, 12,  8,  8,  4,  4,  8,  4, 12,  8,  8,  8,  4,  4,  8,  4,
       12, 12,  8,  8,  4,  4,  8,  4, 12,  8,  8,  8,  4,  4,  8,  4,
       12, 12,  8,  8, 12, 12, 12,  4, 12,  8,  8,  8,  4,  4,  8,  4,
        4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
        4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
        4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
        8,  8,  8,  8,  8,  8,  4,  8,  4,  4,  4,  4,  4,  4,  8,  4,
        4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
        4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
        4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
        4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,
       20, 12, 16, 16, 24, 16,  8, 16, 20, 16, 16,  4, 24, 24,  8, 16,
       20, 12, 16,  0, 24, 16,  8, 16, 20, 16, 16,  0, 24,  0,  8, 16,
       12, 12,  8,  0,  0, 16,  8, 16, 16,  4, 16,  0,  0,  0,  8, 16,
       12, 12,  8,  4,  0, 16,  8, 16, 12,  8, 16,  4,  0,  0,  8, 16
    };

    const int cbprefix_opcode_cycles[256] = {
        8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
        8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
        8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
        8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
        8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, 12,  8,
        8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, 12,  8,
        8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, 12,  8,
        8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, 12,  8,
        8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
        8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
        8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
        8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
        8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
        8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
        8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
        8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8
    };

    void service_interrupt(InterruptController::InterruptType type);

    uint8_t get_uint8_pc();
    int8_t get_int8_pc();
    uint16_t get_uint16_pc();

    void set_register(const R8 reg, const uint8_t value);
    void set_register(const R16_GRP1 reg, const uint16_t value);
    void set_register(const R16_GRP2 reg, const uint16_t value);
    void set_register(const R16_GRP3 reg, const uint16_t value);
    uint8_t get_register(const R8 reg);
    uint16_t get_register(const R16_GRP1 reg);
    uint16_t get_register(const R16_GRP2 reg);
    uint16_t get_register(const R16_GRP3 reg);

    bool check_condition(const CONDITION_FLAG flag);

    //----------------------------------------
    // Special instructions
    //----------------------------------------
    void op_nop();
    void op_stop();
    void op_halt();
    void op_swap_r(const R8 r);
    void op_daa();
    void op_cpl();
    void op_ccf();
    void op_scf();
    void op_di();
    void op_ei();

    //----------------------------------------
    // 8-bit load instructions
    //----------------------------------------
    void op_ld_r_n(const R8 r, const uint8_t n);
    void op_ld_r_r(const R8 r_dst, const R8 r_src);
    void op_ld_A_$rr(const R16_GRP2 rr);
    void op_ld_A_$nn(const uint16_t nn);
    void op_ld_$rr_A(const R16_GRP2 rr);
    void op_ld_$nn_A(const uint16_t nn);
    void op_ld_A_$C();
    void op_ld_$C_A();
    void op_ld_$n_A(const uint8_t n);
    void op_ld_A_$n(const uint8_t n);

    //----------------------------------------
    // 16-bit load instructions
    //----------------------------------------
    void op_ld_rr_nn(const R16_GRP1 rr, const uint16_t nn);
    void op_ld_SP_HL();
    void op_ld_HL_SP_plus_n(const int8_t n);
    void op_ld_$nn_sp(const uint16_t nn);
    void op_push_rr(const R16_GRP3 rr);
    void op_pop_rr(const R16_GRP3 rr);

    //----------------------------------------
    // 8-bit alu instructions
    //----------------------------------------
    void op_add_A_r(const R8 r);
    void op_add_A_n(const uint8_t n);
    void op_adc_A_r(const R8 r);
    void op_adc_A_n(const uint8_t n);
    void op_sub_A_r(const R8 r);
    void op_sub_A_n(const uint8_t n);
    void op_sbc_A_r(const R8 r);
    void op_sbc_A_n(const uint8_t n);
    void op_and_A_r(const R8 r);
    void op_and_A_n(const uint8_t n);
    void op_or_A_r(const R8 r);
    void op_or_A_n(const uint8_t n);
    void op_xor_A_r(const R8 r);
    void op_xor_A_n(const uint8_t n);
    void op_cp_A_r(const R8 r);
    void op_cp_A_n(const uint8_t n);
    void op_inc_r(const R8 r);
    void op_dec_r(const R8 r);

    //----------------------------------------
    // 16-bit alu instructions
    //----------------------------------------
    void op_add_hl_rr(const R16_GRP1 rr);
    void op_add_sp_n(const int8_t nn);
    void op_inc_rr(const R16_GRP1 rr);
    void op_dec_rr(const R16_GRP1 rr);

    //----------------------------------------
    // Shift and rotate instructions
    //----------------------------------------
    void op_rlca();
    void op_rla();
    void op_rrca();
    void op_rra();
    void op_rlc_r(const R8 r);
    void op_rl_r(const R8 r);
    void op_rrc_r(const R8 r);
    void op_rr_r(const R8 r);
    void op_sla_r(const R8 r);
    void op_sra_r(const R8 r);
    void op_srl_r(const R8 r);

    //----------------------------------------
    // Bit-set-reset instructions
    //----------------------------------------
    void op_bit_b_r(const uint8_t b, const R8 r);
    void op_set_b_r(const uint8_t b, const R8 r);
    void op_res_b_r(const uint8_t b, const R8 r);

    //----------------------------------------
    // Control flow instructions
    //----------------------------------------
    void op_jp_nn(const uint16_t nn);
    bool op_jp_cc_nn(const CONDITION_FLAG cc, const uint16_t nn);
    void op_jp_hl();
    void op_jr_n(const int8_t n);
    bool op_jr_cc_n(const CONDITION_FLAG cc, const int8_t n);
    void op_call_nn(const uint16_t nn);
    bool op_call_cc_nn(const CONDITION_FLAG cc, const uint16_t nn);
    void op_rst_n(const uint8_t n);
    void op_ret();
    bool op_ret_cc(const CONDITION_FLAG cc);
    void op_reti();

};

#endif // CPU_H
