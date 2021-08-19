#ifndef REGISTER_H
#define REGISTER_H

#include <cstdint>

template<typename T>
class Register {
public:
    Register(T value = 0)
        : m_value(value) {}

    operator T() const { return m_value; }

    virtual void operator=(T value) {
        m_value = value;
    }
protected:
    T m_value;
};

class FlagRegister : public Register<uint8_t> {
public:
    FlagRegister(uint8_t value = 0)
        : Register<uint8_t>(value) {}

    void operator=(uint8_t value) override {
        m_value = value & 0xF0;
    }

    bool z() { return (m_value & 0b10000000) != 0; }
    bool n() { return (m_value & 0b01000000) != 0; }
    bool h() { return (m_value & 0b00100000) != 0; }
    bool c() { return (m_value & 0b00010000) != 0; }

    void setZ(bool z) {
        m_value = m_value & 0b01111111;
        if (z) m_value |= 0b10000000;
    }

    void setN(bool n) {
        m_value = m_value & 0b10111111;
        if (n) m_value |= 0b01000000;
    }

    void setH(bool h) {
        m_value = m_value & 0b11011111;
        if (h) m_value |= 0b00100000;
    }

    void setC(bool c) {
        m_value = m_value & 0b11101111;
        if (c) m_value |= 0b00010000;
    }
};

class RegisterPair {
public:
    RegisterPair(Register<uint8_t>& upper_byte, Register<uint8_t>& lower_byte)
        : m_upper_byte(upper_byte), m_lower_byte(lower_byte) {}

    operator uint16_t() const { return (static_cast<uint16_t>(m_upper_byte) << 8) | m_lower_byte; }

    void operator=(uint16_t value) {
        uint8_t lower_byte = value & 0xff;
        uint8_t upper_byte = (value >> 8);

        m_upper_byte = upper_byte;
        m_lower_byte = lower_byte;
    }
private:
    Register<uint8_t>& m_lower_byte;
    Register<uint8_t>& m_upper_byte;
};

#endif // REGISTER_H
