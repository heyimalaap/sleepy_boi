#include "timer.h"
#include "cpu/interrupt_controller.h"
#include <stdexcept>

Timer::Timer(MMU& mmu)
    : m_div(0), m_tima(0), m_tma(0), m_tac(0), m_mmu(mmu) {
    m_timer_counter = cycles_per_tick();
}

uint8_t& Timer::operator[](const uint16_t addr) {
    switch(addr) {
    case 0xFF04:
        return m_div;
    case 0xFF05:
        return m_tima;
    case 0xFF06:
        return m_tma;
    case 0xFF07:
        return m_tac;
    }
    throw std::invalid_argument("Invalid argument. Out-of-bounds of timer's memory map.");
}

void Timer::tick(int cycles) {
    m_divider_counter += cycles;
    if (m_divider_counter >= 0xFF) {
        m_divider_counter = 0;
        m_div = 0;
    }

    if (is_timer_enabled()) {
        m_timer_counter -= cycles;
        if (m_timer_counter <= 0) {
            m_timer_counter = cycles_per_tick();
            if (m_tima == 0xFF) {
                m_tima = m_tma;
                m_mmu.request_interrupt(InterruptController::TIMER);
            } else {
                m_tima++;
            }
        }
    }
}

void Timer::reset_divider_register() {
    m_div = 0;
}
