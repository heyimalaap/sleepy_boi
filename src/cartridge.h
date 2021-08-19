#ifndef CARTRIDGE_H
#define CARTRIDGE_H

#include <cstdint>
#include <vector>
#include <string>

enum class CartridgeType {
    NoMBC,
    MBC1,
    INVALID
};

enum class ROMSize {
    K32,
    K64,
    K128,
    K256,
    K512,
    M1,
    M2,
    M4,
    M8,
    M1_1,
    M1_2,
    M1_5,
    INVALID
};

enum class RAMSize {
    NoRAM = 0,
    Unused = 2,
    K8 = 8,
    K32 = 32,
    K128 = 128,
    K64 = 64,
    INVALID
};

struct CartridgeHeader {
    std::string title;
    CartridgeType type;
    ROMSize rom_size;
    RAMSize ram_size;
};

CartridgeHeader get_header_from_romdata(std::vector<uint8_t>& rom_data);

class Cartridge
{
public:
    Cartridge(std::vector<uint8_t> rom_data, std::vector<uint8_t> ram_data = {});
    virtual ~Cartridge();
    virtual uint8_t read(uint16_t address) = 0;
    virtual void write(uint16_t address, uint8_t value) = 0;
protected:
    std::vector<uint8_t> m_rom;
    std::vector<uint8_t> m_ram;
};

class CartridgeNoMBC : public Cartridge {
public:
    CartridgeNoMBC(std::vector<uint8_t> rom_data, std::vector<uint8_t> ram_data = {});
    uint8_t read(uint16_t address) override;
    void write(uint16_t address, uint8_t value) override;
};

class CartridgeMBC1 : public Cartridge {
public:
    CartridgeMBC1(std::vector<uint8_t> rom_data, std::vector<uint8_t> ram_data = {});
    uint8_t read(uint16_t address) override;
    void write(uint16_t address, uint8_t value) override;
private:
    uint8_t m_current_ram_bank = 0;
    uint8_t m_current_rom_bank = 1;
    bool m_ram_enabled = false;
    bool m_ram_banking_mode = false;
};

#endif // CARTRIDGE_H
