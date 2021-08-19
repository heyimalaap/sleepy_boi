#include "interrupt_controller.h"
#include <stdexcept>

uint8_t& InterruptController::operator[](const uint16_t addr) {
    switch (addr) {
    case 0xFF0F: return m_request_register;
    case 0xFFFF: return m_enable_register;
    }
    throw std::invalid_argument("invalid argument. out-of-bounds of interrupt controller's memory map.");
}

void InterruptController::request_service(InterruptType type) {
    m_request_register |= (1 << static_cast<int>(type));
}

bool InterruptController::check_enabled(InterruptType type) {
    return (m_enable_register & (1 << static_cast<int>(type))) != 0 ? true : false;
}

bool InterruptController::check_requested(InterruptType type) {
    return (m_request_register & (1 << static_cast<int>(type))) != 0 ? true : false;
}

void InterruptController::finished_service(InterruptType type) {
    m_request_register &= ~(1 << static_cast<int>(type));
}
