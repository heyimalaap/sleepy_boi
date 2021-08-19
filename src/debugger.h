#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <cstdint>
#include <utility>
#include "gameboy.h"

class Gameboy;

class Debugger
{
public:
    Debugger(Gameboy& gb)
        : m_gb(gb) {}

    inline bool gb_is_running() const { return m_gb.m_gb_running; }

    inline uint8_t cpu_r8_a() const { return m_gb.m_cpu.m_a; }
    inline uint8_t cpu_r8_f() const { return m_gb.m_cpu.m_f; }
    inline uint8_t cpu_r8_b() const { return m_gb.m_cpu.m_b; }
    inline uint8_t cpu_r8_c() const { return m_gb.m_cpu.m_c; }
    inline uint8_t cpu_r8_d() const { return m_gb.m_cpu.m_d; }
    inline uint8_t cpu_r8_e() const { return m_gb.m_cpu.m_e; }
    inline uint8_t cpu_r8_h() const { return m_gb.m_cpu.m_h; }
    inline uint8_t cpu_r8_l() const { return m_gb.m_cpu.m_l; }
    inline uint16_t cpu_r16_af() const { return m_gb.m_cpu.m_af; }
    inline uint16_t cpu_r16_bc() const { return m_gb.m_cpu.m_bc; }
    inline uint16_t cpu_r16_de() const { return m_gb.m_cpu.m_de; }
    inline uint16_t cpu_r16_hl() const { return m_gb.m_cpu.m_hl; }
    inline uint16_t cpu_pc() const { return m_gb.m_cpu.m_pc; }
    inline uint16_t cpu_sp() const { return m_gb.m_cpu.m_sp; }
    inline bool cpu_flag_z() const { return m_gb.m_cpu.m_f.z(); }
    inline bool cpu_flag_n() const { return m_gb.m_cpu.m_f.n(); }
    inline bool cpu_flag_h() const { return m_gb.m_cpu.m_f.h(); }
    inline bool cpu_flag_c() const { return m_gb.m_cpu.m_f.c(); }
    inline bool cpu_flag_ime() const { return m_gb.m_cpu.m_interrupt_enable; }
    inline bool cpu_flag_iw() const { return m_gb.m_cpu.m_interrupt_waiting; }

    inline void step() {
        m_gb.SetRunning(false);
        m_gb.Step();
    }

    inline void reset() {
        m_gb.SetRunning(false);
        m_gb.m_cpu.reset();
        m_gb.m_video.reset();
        m_gb.m_mmu.m_bootrom_mapped = true;
    }

    std::pair<std::string, int> disassemble_instruction(uint16_t address) const;

private:
    Gameboy& m_gb;
};

#endif // DEBUGGER_H
