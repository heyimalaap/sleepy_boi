#ifndef INTERRUPTCONTROLLER_H
#define INTERRUPTCONTROLLER_H

#include <cstdint>

class InterruptController
{
public:
    enum InterruptType {
        VBLANK = 0,
        LCD    = 1,
        TIMER  = 2,
        JOYPAD = 4
    };

    uint8_t& operator[](const uint16_t addr);
    void request_service(InterruptType type);
    bool check_enabled(InterruptType type);
    bool check_requested(InterruptType type);
    void finished_service(InterruptType type);

private:
    uint8_t m_enable_register = 0;
    uint8_t m_request_register = 0;
};

#endif // INTERRUPTCONTROLLER_H
