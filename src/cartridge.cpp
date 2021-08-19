#include "cartridge.h"
#include <stdexcept>
#include <string>

#define ROM_SIZE_CASE(x, y) case x: \
    rom_size = y; \
    break; \

#define RAM_SIZE_CASE(x, y) case x: \
    ram_size = y; \
    break; \

CartridgeHeader get_header_from_romdata(std::vector<uint8_t>& rom_data) {
    std::string title(' ', 16);
    for (int i = 0; i <= 0xF; i++)
        if (rom_data[0x134 + i] != 0) title[i] = rom_data[0x134 + i];

    CartridgeType type;
    switch (rom_data[0x0147]) {
    case 0x00:
    case 0x08:
    case 0x09:
        type = CartridgeType::NoMBC;
        break;
    case 0x01:
    case 0x02:
    case 0x03:
        type = CartridgeType::MBC1;
        break;
    default:
        type = CartridgeType::INVALID;
    }

    ROMSize rom_size;
    switch (rom_data[0x0148]) {
    ROM_SIZE_CASE(0x00, ROMSize::K32);
    ROM_SIZE_CASE(0x01, ROMSize::K64);
    ROM_SIZE_CASE(0x02, ROMSize::K128);
    ROM_SIZE_CASE(0x03, ROMSize::K256);
    ROM_SIZE_CASE(0x04, ROMSize::K512);
    ROM_SIZE_CASE(0x05, ROMSize::M1);
    ROM_SIZE_CASE(0x06, ROMSize::M2);
    ROM_SIZE_CASE(0x07, ROMSize::M4);
    ROM_SIZE_CASE(0x08, ROMSize::M8);
    ROM_SIZE_CASE(0x52, ROMSize::M1_1);
    ROM_SIZE_CASE(0x53, ROMSize::M1_2);
    ROM_SIZE_CASE(0x54, ROMSize::M1_5);
    default: rom_size = ROMSize::INVALID;
    }

    RAMSize ram_size;
    switch (rom_data[0x0149]) {
    RAM_SIZE_CASE(0x00, RAMSize::NoRAM);
    RAM_SIZE_CASE(0x01, RAMSize::Unused);
    RAM_SIZE_CASE(0x02, RAMSize::K8);
    RAM_SIZE_CASE(0x03, RAMSize::K32);
    RAM_SIZE_CASE(0x04, RAMSize::K128);
    RAM_SIZE_CASE(0x05, RAMSize::K64);
    default: ram_size = RAMSize::INVALID;
    }

    CartridgeHeader header = {
        .title = title,
        .type  = type,
        .rom_size = rom_size,
        .ram_size = ram_size
    };

    return header;
}

Cartridge::Cartridge(std::vector<uint8_t> rom_data, std::vector<uint8_t> ram_data)
    :m_rom(rom_data), m_ram(ram_data) {
    m_rom.reserve(0x8000);
    m_ram.reserve(0x2000);
}

Cartridge::~Cartridge() {}

CartridgeNoMBC::CartridgeNoMBC(std::vector<uint8_t> rom_data, std::vector<uint8_t> ram_data)
    : Cartridge(rom_data, ram_data) {}

uint8_t CartridgeNoMBC::read(uint16_t address) {
    if (address >= 0 && address <= 0x7FFF)
        return m_rom[address];

    if (address >= 0xA000 && address <= 0xBFFF)
        return m_ram[address - 0xA000];

    throw std::invalid_argument("invalid argument. out-of-bounds of cartridge's address map");
}

void CartridgeNoMBC::write(uint16_t address, uint8_t value) {
    if (address >= 0 && address <= 0x7FFF)
        return;

    if (address >= 0xA000 && address <= 0xBFFF) {
        m_ram[address - 0xA000] = value;
        return;
    }

    throw std::invalid_argument("invalid argument. out-of-bounds of cartridge's address map");
}

CartridgeMBC1::CartridgeMBC1(std::vector<uint8_t> rom_data, std::vector<uint8_t> ram_data)
    : Cartridge(rom_data, ram_data) {
    CartridgeHeader header = get_header_from_romdata(rom_data);
    m_ram.reserve(0x8000);
}

uint8_t CartridgeMBC1::read(uint16_t address) {
    if (address >= 0 && address <= 0x3FFF)
        return m_rom[address];

    if (address >= 0x4000 && address <= 0x7FFF) {
        return m_rom[0x4000 * m_current_rom_bank + (address - 0x4000)];
    }

    if (m_ram_enabled) {
        if (address >= 0xA000 && address <= 0xBFFF)
            return m_ram[0x2000 * m_current_ram_bank + (address - 0xA000)];
    } else {
        return 0x00;
    }

    throw std::invalid_argument("invalid argument. out-of-bounds of cartridge's address map");
}

void CartridgeMBC1::write(uint16_t address, uint8_t value) {
    if (address >= 0 && address <= 0x1FFF) {
        if ((value & 0xF) == 0xA)
            m_ram_enabled = true;
        else
            m_ram_enabled = false;
        return;
    }

    if (address >= 0x2000 && address <= 0x3FFF) {
        m_current_rom_bank &= 0b11100000;
        m_current_rom_bank |= value & 0b11111;
        if (m_current_rom_bank == 0) m_current_rom_bank = 1;
        return;
    }

    if (address >= 0x4000 && address <= 0x5FFF) {
        if (m_ram_banking_mode) {
            m_current_ram_bank = value & 0b11;
        } else {
            m_current_rom_bank &= 0b00011111;
            m_current_rom_bank |= value & 0b11100000;
            if (m_current_rom_bank == 0) m_current_rom_bank = 1;
        }
        return;
    }

    if (address >= 0x6000 && address <= 0x7FFF) {
        m_ram_banking_mode = ((value & 1) == 1);
        if (!m_ram_banking_mode)
            m_current_ram_bank = 0;
        return;
    }

    if (m_ram_enabled && address >= 0xA000 && address <= 0xBFFF) {
        m_ram[0x2000 * m_current_ram_bank + (address - 0xA000)] = value;
        return;
    }

    throw std::invalid_argument("invalid argument. out-of-bounds of cartridge's address map");
}
