#include <iostream>
#include "cpu/register.h"
#include <cstdint>
#include <string>
#include <array>
#include <sstream>
#include <iomanip>
#include <fstream>
// Gameboy opcode disassembly test

#define FORMAT_HEX(d) "0x" << std::uppercase << std::setfill('0') << std::setw(d) << std::hex

uint16_t pc = 0;

std::array<uint8_t, 0x10000> memory;

std::string to_hex(int num, int d = 2) {
    std::stringstream ss;
    ss << FORMAT_HEX(d) << num << std::dec << std::nouppercase;
    return ss.str();
}

std::string p_hex_n(int pc, int n = 1) {
    std::stringstream ss;
    ss << "[" << to_hex(memory[pc]);
    for(int i = 1; i < n; i++) {
        ss << " " << to_hex(memory[pc + i]);
    }
    ss << "] ";
    return ss.str();
}

std::string decode_opcode(uint16_t _pc = pc) {
    int opcode = memory[_pc];
    const std::string condition_flag[] = {"NZ", "Z", "NC", "C"};
    const std::string r16_grp1[] = {"BC", "DE", "HL", "SP"};
    const std::string r16_grp2[] = {"BC", "DE", "HL+", "HL-"};
    const std::string r16_grp3[] = {"BC", "DE", "HL", "AL"};
    const std::string r8_grp1[] = {"B", "C", "D", "E", "H", "L", "(HL)", "A"};
    const std::string opcode_grp1[] = {"RLCA", "RRCA", "RLA", "RRA", "DAA", "CPL", "SCF", "CCF"};
    const std::string opcode_grp2[] = {"ADD", "ADC", "SUB", "SBC", "AND", "XOR", "OR", "CP"};
    const std::string opcode_grp3[] = {"RLC", "RRC", "RL", "RR", "SLA", "SRA", "SWAP", "SRL"};
    std::stringstream ss;
    if (opcode == 0x00) {
        // nop
        ss << p_hex_n(_pc) << "nop";
        pc += 1;
    } else if (opcode == 0x08) {
        // ld (u16), sp
        const uint16_t val = (memory[pc + 1]) | (memory[pc + 2] << 8);
        ss << p_hex_n(_pc, 3) << "ld " << to_hex(val, 4) << ", sp";
        pc += 3;
    } else if (opcode == 0x10) {
        // stop
        ss << p_hex_n(_pc) << "] stop";
        pc += 1;
    } else if (opcode == 0x18) {
        // jr (uncoditional)
        const int address_offset = memory[_pc + 1];
        ss << p_hex_n(_pc, 2)
                  << "jr pc+(" << address_offset << ")";
        pc += 2;
    } else if ((opcode & 0b11100111) == 0x20) {
        // jr (conditional)
        const std::string condition = condition_flag[(opcode >> 3) & 0b11];
        const int address_offset = (int8_t)memory[_pc + 1];
        ss << p_hex_n(_pc, 1)
                  << "jr " << condition << ", pc+(" << address_offset << ")";
        pc += 2;
    } else if ((opcode & 0b11001111) == 0x01) {
        // ld r16, u16
        const uint16_t val = (memory[pc + 1]) | (memory[pc + 2] << 8);
        const std::string r16 = r16_grp1[(opcode >> 4) & 0b11];
        ss << p_hex_n(_pc, 3)
                  << "ld " << r16 << ", " << val;
        pc += 3;
    } else if ((opcode & 0b11001111) == 0x09) {
        // add HL, r16
        const std::string r16 = r16_grp1[(opcode >> 4) & 0b11];
        ss << p_hex_n(_pc)
                  << "add HL, " << r16;
        pc += 1;
    } else if ((opcode & 0b11001111) == 0x02) {
        // ld (r16), A
        const std::string r16 = r16_grp2[(opcode >> 4) & 0b11];
        ss << p_hex_n(_pc)
                  << "ld (" << r16 << "), A";
        pc += 1;
    } else if ((opcode & 0b11001111) == 0x0A) {
        // ld A, (r16)
        const std::string r16 = r16_grp2[(opcode >> 4) & 0b11];
        ss << p_hex_n(_pc)
                  << "ld A, (" << r16 << ")";
        pc += 1;
    } else if ((opcode & 0b11001111) == 0x03) {
        // inc r16
        const std::string r16 = r16_grp1[(opcode >> 4) & 0b11];
        ss << p_hex_n(_pc)
                  << "inc " << r16;
        pc += 1;
    } else if ((opcode & 0b11001111) == 0x0B) {
        // dec r16
        const std::string r16 = r16_grp1[(opcode >> 4) & 0b11];
        ss << p_hex_n(_pc)
                  << "dec " << r16;
        pc += 1;
    } else if ((opcode & 0b11000111) == 0x04) {
        // inc r8
        const std::string r8 = r8_grp1[(opcode >> 3) & 0b111];
        ss << p_hex_n(_pc)
                  << "inc " << r8;
        pc += 1;
    } else if ((opcode & 0b11000111) == 0x05) {
        // dec r8
        const std::string r8 = r8_grp1[(opcode >> 3) & 0b111];
        ss << p_hex_n(_pc)
                  << "dec " << r8;
        pc += 1;
    } else if ((opcode & 0b11000111) == 0x06) {
        // ld r8, u8
        const std::string r8 = r8_grp1[(opcode >> 3) & 0b111];
        int val = memory[_pc + 1];
        ss << p_hex_n(_pc, 2)
                  << "ld " << r8 << ", " << val;
        pc += 2;
    } else if ((opcode & 0b11000111) == 0x07) {
        // single byte opcode grp1
        ss << p_hex_n(_pc)
           << opcode_grp1[(opcode >> 3) & 0b111];
        pc += 1;
    } else if (opcode == 0x76) {
        // halt
        ss << p_hex_n(_pc) << "halt";
        pc += 1;
    } else if ((opcode & 0b11000000) == 0x40) {
        // ld r8, r8
        const std::string src_r8 = r8_grp1[opcode & 0b111];
        const std::string dst_d8 = r8_grp1[(opcode >> 3) & 0b111];
        ss << p_hex_n(_pc)
           << "ld " << dst_d8 << ", " << src_r8;
        pc += 1;
    } else if ((opcode & 0b11000000) == 0x80) {
        // alu A, r8
        const std::string r8 = r8_grp1[opcode & 0b111];
        const std::string operation = opcode_grp2[(opcode >> 3) & 0b111];
        ss << p_hex_n(_pc)
           << operation << " A, " << r8;
        pc += 1;
    } else if ((opcode & 0b11100111) == 0xC0) {
        // ret condition
        const std::string condition = condition_flag[(opcode >> 3) & 0b11];
        ss << p_hex_n(_pc) << "ret " << condition;
        pc += 1;
    } else if (opcode == 0xE0) {
        // ld ($ff00 + u8), A
        const int addr = 0xff00 + memory[_pc + 1];
        ss << p_hex_n(_pc, 2)
                  << "ld (" << to_hex(addr) << "), A";
        pc += 2;
    } else if (opcode == 0xE8) {
        // add sp, i8
        const int offset = (int8_t)memory[_pc+1];
        ss << p_hex_n(_pc, 2) << "add sp, " << offset;
        pc += 2;
    } else if (opcode == 0xF0) {
        // ld A, ($ff00 + u8)
        const int addr = 0xff00 + memory[_pc + 1];
        ss << p_hex_n(_pc, 2)
                  << "ld A, (" << to_hex(addr) << ")";
        pc += 2;
    } else if (opcode == 0xF8) {
        // ld HL, sp + i8
        const int offset = (int8_t)memory[_pc+1];
        ss << p_hex_n(_pc, 2)
                  << "ld HL, sp + (" << offset << ")";
        pc += 2;
    } else if ((opcode & 0b11001111) == 0xC1) {
        // pop r16
        const std::string r16 = r16_grp3[(opcode >> 4) & 0b11];
        ss << p_hex_n(_pc)
                  << "pop " << r16;
        pc += 1;
    } else if (opcode == 0xC9) {
        // ret
        ss << p_hex_n(_pc)
                  << "ret";
        pc += 1;
    } else if (opcode == 0xD9) {
        // reti
        ss << p_hex_n(_pc)
                  << "reti";
        pc += 1;
    } else if (opcode == 0xE9) {
        // jp HL
        ss << p_hex_n(_pc)
                  << "jp HL";
        pc += 1;
    } else if (opcode == 0xF9) {
        // ld sp, HL
        ss << p_hex_n(_pc)
                  << "ld sp, HL";
        pc += 1;
    } else if ((opcode & 0b11100111) == 0xC2) {
        // jp condition
        const std::string condition = condition_flag[(opcode >> 3) & 0b11];
        ss << p_hex_n(_pc)
                  << "jp " << condition;
        pc += 1;
    } else if (opcode == 0xE2) {
        // ld ($ff00 + C), A
        ss << p_hex_n(_pc)
                  << "ld ($FF00 + C), A";
        pc += 1;
    } else if (opcode == 0xEA) {
        // ld (u16), A
        const uint16_t addr = (memory[pc + 1]) | (memory[pc + 2] << 8);
        ss << p_hex_n(_pc, 3)
                  << "ld " << to_hex(addr, 4) << ", A";
        pc += 3;
    } else if (opcode == 0xF2) {
        // ld A, ($ff00 + C)
        ss << p_hex_n(_pc)
                  << "ld A, ($FF00 + C)";
        pc += 1;
    } else if (opcode == 0xFA) {
        // ld A, (u16)
        const uint16_t addr = (memory[pc + 1]) | (memory[pc + 2] << 8);
        ss << p_hex_n(_pc, 3)
                  << "ld A, " << to_hex(addr, 4);
        pc += 3;
    } else if (opcode == 0xC3) {
        // jp u16
        const uint16_t addr = (memory[pc + 1]) | (memory[pc + 2] << 8);
        ss << p_hex_n(_pc, 3)
                  << "jp " << to_hex(addr, 4);
        pc += 3;
    } else if (opcode == 0xCB) {
        // cb prefix
        pc += 1;
        opcode = memory[pc];
        const std::string r8 = r8_grp1[opcode & 0b111];
        const int bit = (opcode >> 3) & 0b111;
        ss << p_hex_n(_pc, 2);
        if ((opcode >> 6) == 0) {
            const std::string operation = opcode_grp3[bit];
            ss << operation << " " << r8;
        } else if ((opcode >> 6) == 1) {
            ss << "bit " << bit << ", " << r8;
        } else if ((opcode >> 6) == 2) {
            ss << "res " << bit << ", " << r8;
        } else if ((opcode >> 6) == 3) {
            ss << "set " << bit << ", " << r8;
        }
        pc += 1;
    } else if (opcode == 0xF3) {
        // di
        ss << p_hex_n(_pc)
                  << "di";
        pc += 1;
    } else if (opcode == 0xFB) {
        // ei
        ss << p_hex_n(_pc)
                  << "ei";
        pc += 1;
    } else if ((opcode & 0b11100111) == 0xC4) {
        // call condition
        const std::string condition = condition_flag[(opcode >> 3) & 0b11];
        const uint16_t addr = (memory[pc + 1]) | (memory[pc + 2] << 8);
        ss << p_hex_n(_pc, 3)
                  << "call " << condition << ", " << to_hex(addr, 4);
        pc += 3;
    } else if ((opcode & 0b11001111) == 0xC5) {
        // push r16
        const std::string r16 = r16_grp3[(opcode >> 4) & 0b11];
        ss << p_hex_n(_pc)
                  << "push " << r16;
        pc += 1;
    } else if (opcode == 0xCD) {
        // call u16
        const uint16_t addr = (memory[pc + 1]) | (memory[pc + 2] << 8);
        ss << p_hex_n(_pc, 3)
                  << "call " << to_hex(addr, 4);
        pc += 3;
    } else if ((opcode & 0b11000111) == 0xC6) {
        // alu a, u8
        const std::string operation = opcode_grp2[(opcode >> 3) & 0b111];
        const int value = memory[_pc + 1];
        ss << p_hex_n(_pc, 2)
                  << operation << " a, " << to_hex(value);
        pc += 2;
    } else if ((opcode & 0b11000111) == 0xC7) {
        // RST (00exp000)
        const int addr = (opcode & 0b00111000);
        ss << p_hex_n(_pc)
                  << "rst " << to_hex(addr, 4);
        pc += 1;
    } else {
        ss << p_hex_n(_pc) << "Unknown instruction. Anything that follows may be incorrect due to misallignment";
        pc += 1;
    }
    return ss.str();
}

int read_from_bootrom() {
    std::ifstream bootrom("../sleepy_boi/res/dmg_boot.bin", std::ios_base::binary);
    int length = 0;
    if (bootrom) {
        bootrom.seekg(0, bootrom.end);
        length = bootrom.tellg();
        bootrom.seekg(0, bootrom.beg);
        char* buffer = new char[length];
        bootrom.read(buffer, length);
        for (int i = 0; i < length; i++)
            memory[i] = buffer[i];
        delete [] buffer;
    } else {
        std::cerr << "Failed to open bootrom!" << std::endl;
    }
    return length;
}

int main()
{
    const int length = read_from_bootrom();
    for (int i = 0; i < length; i++) {
        std::cout << decode_opcode();
        std::cout << "\n";
    }
    return 0;
}
