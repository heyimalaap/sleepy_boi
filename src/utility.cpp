#include "utility.h"

std::string to_hex(int num, int d) {
    std::stringstream ss;
    ss << "0x" << std::uppercase << std::setfill('0') << std::setw(d) << std::hex << num << std::dec << std::nouppercase;
    return ss.str();
}

CartridgeType get_cartridge_type(std::vector<uint8_t> rom_data) {
    switch (rom_data[0x0147]) {
    case 0x00:
    case 0x08:
    case 0x09:
        return CartridgeType::NoMBC;
    case 0x01:
    case 0x02:
    case 0x03:
        return CartridgeType::MBC1;
    }
    return CartridgeType::INVALID;
}
