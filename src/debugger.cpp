#include "debugger.h"
#include "utility.h"
#include <sstream>

std::pair<std::string, int> Debugger::disassemble_instruction(uint16_t address) const {
    auto next_byte = [&]() -> uint8_t { return m_gb.m_mmu.read_byte(address++); };
    int opcode = next_byte();

    const std::string condition_flag[] = {"NZ", "Z", "NC", "C"};
    const std::string r16_grp1[] = {"BC", "DE", "HL", "SP"};
    const std::string r16_grp2[] = {"BC", "DE", "HL+", "HL-"};
    const std::string r16_grp3[] = {"BC", "DE", "HL", "AL"};
    const std::string r8_grp1[] = {"B", "C", "D", "E", "H", "L", "(HL)", "A"};
    const std::string opcode_grp1[] = {"RLCA", "RRCA", "RLA", "RRA", "DAA", "CPL", "SCF", "CCF"};
    const std::string opcode_grp2[] = {"ADD", "ADC", "SUB", "SBC", "AND", "XOR", "OR", "CP"};
    const std::string opcode_grp3[] = {"RLC", "RRC", "RL", "RR", "SLA", "SRA", "SWAP", "SRL"};
    std::stringstream ss;
    int size;
    if (opcode == 0x00) {
        // nop
        ss << "NOP";
        size = 1;
    } else if (opcode == 0x08) {
        // ld (u16), sp
        uint16_t val = next_byte();
        val = val | (next_byte() << 8);
        ss << "LD " << to_hex(val, 4) << ", SP";
        size = 3;
    } else if (opcode == 0x10) {
        // stop
        ss << "STOP";
        size = 1;
    } else if (opcode == 0x18) {
        // jr (uncoditional)
        const int address_offset = next_byte();
        ss << "JR PC+(" << address_offset << ")";
        size = 2;
    } else if ((opcode & 0b11100111) == 0x20) {
        // jr (conditional)
        const std::string condition = condition_flag[(opcode >> 3) & 0b11];
        const int address_offset = (int8_t)next_byte();
        ss << "JR " << condition << ", PC+(" << address_offset << ")";
        size = 2;
    } else if ((opcode & 0b11001111) == 0x01) {
        // ld r16, u16
        uint16_t val = next_byte();
        val = val | (next_byte() << 8);
        const std::string r16 = r16_grp1[(opcode >> 4) & 0b11];
        ss << "LD " << r16 << ", " << to_hex(val, 4);
        size = 3;
    } else if ((opcode & 0b11001111) == 0x09) {
        // add HL, r16
        const std::string r16 = r16_grp1[(opcode >> 4) & 0b11];
        ss << "ADD HL, " << r16;
        size = 1;
    } else if ((opcode & 0b11001111) == 0x02) {
        // ld (r16), A
        const std::string r16 = r16_grp2[(opcode >> 4) & 0b11];
        ss << "LD (" << r16 << "), A";
        size = 1;
    } else if ((opcode & 0b11001111) == 0x0A) {
        // ld A, (r16)
        const std::string r16 = r16_grp2[(opcode >> 4) & 0b11];
        ss << "LD A, (" << r16 << ")";
        size = 1;
    } else if ((opcode & 0b11001111) == 0x03) {
        // inc r16
        const std::string r16 = r16_grp1[(opcode >> 4) & 0b11];
        ss << "INC " << r16;
        size = 1;
    } else if ((opcode & 0b11001111) == 0x0B) {
        // dec r16
        const std::string r16 = r16_grp1[(opcode >> 4) & 0b11];
        ss << "DEC " << r16;
        size = 1;
    } else if ((opcode & 0b11000111) == 0x04) {
        // inc r8
        const std::string r8 = r8_grp1[(opcode >> 3) & 0b111];
        ss << "INC " << r8;
        size = 1;
    } else if ((opcode & 0b11000111) == 0x05) {
        // dec r8
        const std::string r8 = r8_grp1[(opcode >> 3) & 0b111];
        ss << "DEC " << r8;
        size = 1;
    } else if ((opcode & 0b11000111) == 0x06) {
        // ld r8, u8
        const std::string r8 = r8_grp1[(opcode >> 3) & 0b111];
        int val = next_byte();
        ss << "LD " << r8 << ", " << to_hex(val);
        size = 2;
    } else if ((opcode & 0b11000111) == 0x07) {
        // single byte opcode grp1
        ss << opcode_grp1[(opcode >> 3) & 0b111];
        size = 1;
    } else if (opcode == 0x76) {
        // halt
        ss << "HALT";
        size = 1;
    } else if ((opcode & 0b11000000) == 0x40) {
        // ld r8, r8
        const std::string src_r8 = r8_grp1[opcode & 0b111];
        const std::string dst_d8 = r8_grp1[(opcode >> 3) & 0b111];
        ss << "LD " << dst_d8 << ", " << src_r8;
        size = 1;
    } else if ((opcode & 0b11000000) == 0x80) {
        // alu A, r8
        const std::string r8 = r8_grp1[opcode & 0b111];
        const std::string operation = opcode_grp2[(opcode >> 3) & 0b111];
        ss << operation << " A, " << r8;
        size = 1;
    } else if ((opcode & 0b11100111) == 0xC0) {
        // ret condition
        const std::string condition = condition_flag[(opcode >> 3) & 0b11];
        ss << "RET " << condition;
        size = 1;
    } else if (opcode == 0xE0) {
        // ld ($ff00 + u8), A
        const int addr = 0xff00 + next_byte();
        ss << "LD (" << to_hex(addr) << "), A";
        size = 2;
    } else if (opcode == 0xE8) {
        // add sp, i8
        const int offset = (int8_t)next_byte();
        ss << "ADD SP, " << offset;
        size = 2;
    } else if (opcode == 0xF0) {
        // ld A, ($ff00 + u8)
        const int addr = 0xff00 + next_byte();
        ss << "LD A, (" << to_hex(addr) << ")";
        size = 2;
    } else if (opcode == 0xF8) {
        // ld HL, sp + i8
        const int offset = (int8_t)next_byte();
        ss << "LD HL, SP + (" << offset << ")";
        size = 2;
    } else if ((opcode & 0b11001111) == 0xC1) {
        // pop r16
        const std::string r16 = r16_grp3[(opcode >> 4) & 0b11];
        ss << "POP " << r16;
        size = 1;
    } else if (opcode == 0xC9) {
        // ret
        ss << "RET";
        size = 1;
    } else if (opcode == 0xD9) {
        // reti
        ss << "RETI";
        size = 1;
    } else if (opcode == 0xE9) {
        // jp HL
        ss << "JP HL";
        size = 1;
    } else if (opcode == 0xF9) {
        // ld sp, HL
        ss << "ld sp, HL";
        size = 1;
    } else if ((opcode & 0b11100111) == 0xC2) {
        // jp condition
        const std::string condition = condition_flag[(opcode >> 3) & 0b11];
        uint16_t val = next_byte();
        val = val | (next_byte() << 8);
        ss << "JP " << condition << ", " << to_hex(val, 4);
        size = 3;
    } else if (opcode == 0xE2) {
        // ld ($ff00 + C), A
        ss << "LD ($FF00 + C), A";
        size = 1;
    } else if (opcode == 0xEA) {
        // ld (u16), A
        uint16_t addr = next_byte();
        addr = addr | (next_byte() << 8);
        ss << "LD " << to_hex(addr, 4) << ", A";
        size = 3;
    } else if (opcode == 0xF2) {
        // ld A, ($ff00 + C)
        ss << "LD A, ($FF00 + C)";
        size = 1;
    } else if (opcode == 0xFA) {
        // ld A, (u16)
        uint16_t addr = next_byte();
        addr = addr | (next_byte() << 8);
        ss << "LD A, " << to_hex(addr, 4);
        size = 3;
    } else if (opcode == 0xC3) {
        // jp u16
        uint16_t addr = next_byte();
        addr = addr | (next_byte() << 8);
        ss << "JP " << to_hex(addr, 4);
        size = 3;
    } else if (opcode == 0xCB) {
        // cb prefix
        opcode = next_byte();
        const std::string r8 = r8_grp1[opcode & 0b111];
        const int bit = (opcode >> 3) & 0b111;
        if ((opcode >> 6) == 0) {
            const std::string operation = opcode_grp3[bit];
            ss << operation << " " << r8;
        } else if ((opcode >> 6) == 1) {
            ss << "BIT " << bit << ", " << r8;
        } else if ((opcode >> 6) == 2) {
            ss << "RES " << bit << ", " << r8;
        } else if ((opcode >> 6) == 3) {
            ss << "SET " << bit << ", " << r8;
        }
        size = 2;
    } else if (opcode == 0xF3) {
        // di
        ss << "DI";
        size = 1;
    } else if (opcode == 0xFB) {
        // ei
        ss << "EI";
        size = 1;
    } else if ((opcode & 0b11100111) == 0xC4) {
        // call condition
        const std::string condition = condition_flag[(opcode >> 3) & 0b11];
        uint16_t addr = next_byte();
        addr = addr | (next_byte() << 8);
        ss << "CALL " << condition << ", " << to_hex(addr, 4);
        size = 3;
    } else if ((opcode & 0b11001111) == 0xC5) {
        // push r16
        const std::string r16 = r16_grp3[(opcode >> 4) & 0b11];
        ss << "PUSH " << r16;
        size = 1;
    } else if (opcode == 0xCD) {
        // call u16
        uint16_t addr = next_byte();
        addr = addr | (next_byte() << 8);
        ss << "CALL " << to_hex(addr, 4);
        size = 3;
    } else if ((opcode & 0b11000111) == 0xC6) {
        // alu a, u8
        const std::string operation = opcode_grp2[(opcode >> 3) & 0b111];
        const int value = next_byte();
        ss << operation << " A, " << to_hex(value);
        size = 2;
    } else if ((opcode & 0b11000111) == 0xC7) {
        // RST (00exp000)
        const int addr = (opcode & 0b00111000);
        ss << "RST " << to_hex(addr, 4);
        size = 1;
    } else {
        ss << "INVALID";
        size = 1;
    }
    return {ss.str(), size};
}
