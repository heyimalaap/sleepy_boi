#ifndef TIMER_H
#define TIMER_H

#include "mmu.h"
#include <cassert>

class MMU;

class Timer
{
public:
    Timer(MMU& mmu);
    uint8_t& operator[](const uint16_t addr);

    void tick(int cycles);
    void reset_divider_register();

private:
    // Timer registers
    uint8_t m_div;		// Divider register
    uint8_t m_tima;		// Timer counter
    uint8_t m_tma;		// Timer Modulo
    uint8_t m_tac;		// Timer Controller
    int m_timer_counter;
    int m_divider_counter;

    MMU& m_mmu;

    inline bool is_timer_enabled() {
        return (m_tac & 0b100) != 0 ? true : false;
    }

    inline int cycles_per_tick() {
        constexpr int CPU_FREQUENCY = 4194304; // Hz
        switch(m_tac & 0b11) {
        case 0:
            return CPU_FREQUENCY / 4096;
        case 1:
            return CPU_FREQUENCY / 262144;
        case 2:
            return CPU_FREQUENCY / 65536;
        case 3:
            return CPU_FREQUENCY / 16384;
        }
        assert(!"unreachable : frequency_per_cycle()");
    }
};

#endif // TIMER_H
