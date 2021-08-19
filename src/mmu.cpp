#include "mmu.h"
#include <stdexcept>
#include <cassert>
#include <iostream>

MMU::MMU()
{
    m_memory.fill(0); // clear memory just in case
}

void MMU::oam_dma_transfer(uint16_t start_addr) {
    start_addr = start_addr << 8;
    for (int i = 0; i < 0xA0; i++)
        write_byte(0xFE00 + i, read_byte(start_addr + i));
}

uint8_t MMU::read_byte(const uint16_t address) const {
    if (address <= 0x7FFF) {
        // 0000 - 3FFF : 16 KiB ROM bank 00
        // 4000 - 7FFF : 16 KiB ROM bank 01~NN depending on mapper
        if (address < 0x100 && m_bootrom_mapped == true)
            return m_bootrom[address];

        if (m_cartridge) return m_cartridge->read(address);
        return 0xFF;
    } else if (address <= 0x9FFF) {
        // 8000 - 9FFF : 8 KiB of Video RAM
        // TODO: Replace this with video subsystem
        return m_memory[address];
    } else if (address <= 0xBFFF) {
        // A000 - BFFF : 8 KiB of External RAM (Cartridge RAM)
        // TODO: Replace this with cartridge subsystem
        if (m_cartridge) return m_cartridge->read(address);
        return 0x00;
    } else if (address <= 0xDFFF) {
        // C000 - DFFF : 8 KiB of Work RAM
        return m_memory[address];
    } else if (address <= 0xFDFF) {
        // E000 - FDFF : Echo RAM
        return m_memory[address - 0x2000];
    } else if (address <= 0xFE9F) {
        // FE00 - FE9F : OAM (Sprite attribute table)
        // TODO: Donno what to do with this? Probably with video subsystem. also DMA?
        return m_memory[address];
    } else if (address <= 0xFEFF) {
        // FEA0 - FEFF : Not usable
        // TODO: Log an error/warning, not usable range
        return 0x00;
    } else if (address <= 0xFF7F) {
        // FF00 - FF7F : I/O registers
        // TODO: Everything ! ! !

        // Timer I/O Register Writes
        if (address >= 0xFF04 && address <= 0xFF07)
            return (*m_timer)[address];

        // Interrupt request register
        if (address == 0xFF0F)
            return m_cpu->interrupt_controller()[address];

        // Video subsystem
        if (address >= 0xFF40 && address <= 0xFF4B) {
            if (address == 0xFF46) return m_memory[address];
            return (*m_video)[address];
        }

        return m_memory[address];
    } else if (address <= 0xfffe) {
        // FF80 - FFFE : High RAM
        return m_memory[address];
    } else {
        // FFFF : Interrupt Enable register
        return m_cpu->interrupt_controller()[address];
    }
    assert(!"unreachable code : read_byte(uint16_t)");
    return 0xFF;
}

void MMU::write_byte(uint16_t address, uint8_t value) {
    if (address <= 0x7FFF) {
        // 0000 - 3FFF : 16 KiB ROM bank 00
        // 4000 - 7FFF : 16 KiB ROM bank 01~NN depending on mapper
        if (m_cartridge) m_cartridge->write(address, value);
        return;
    } else if (address <= 0x9FFF) {
        // 8000 - 9FFF : 8 KiB of Video RAM
        // TODO: Replace this with video subsystem
        m_memory[address] = value;
        return;
    } else if (address <= 0xBFFF) {
        // A000 - BFFF : 8 KiB of External RAM (Cartridge RAM)
        // TODO: Replace this with cartridge subsystem
        if (m_cartridge) m_cartridge->write(address, value);
        return;
    } else if (address <= 0xDFFF) {
        // C000 - DFFF : 8 KiB of Work RAM
        m_memory[address] = value;
        return;
    } else if (address <= 0xFDFF) {
        // E000 - FDFF : Echo RAM
        m_memory[address - 0x2000] = value;
        return;
    } else if (address <= 0xFE9F) {
        // FE00 - FE9F : OAM (Sprite attribute table)
        // TODO: Donno what to do with this? Probably with video subsystem. also DMA?
        return;
    } else if (address <= 0xFEFF) {
        // FEA0 - FEFF : Not usable
        // TODO: Log an error/warning, not usable range
        return;
    } else if (address <= 0xFF7F) {
        // FF00 - FF7F : I/O registers
        // TODO: Everything ! ! !

        // Serial out
        if (address == 0xFF01) {
            std::cout << value;
            return;
        }

        // Timer I/O Register Writes
        if (address == 0xFF04) {
            m_timer->reset_divider_register();
            return;
        }

        if (address >= 0xFF05 && address <= 0xFF07) {
            (*m_timer)[address] = value;
            return;
        }

        // Interrupt request register
        if (address == 0xFF0F) {
            m_cpu->interrupt_controller()[address] = value;
            return;
        }

        // Video subsystem
        if (address >= 0xFF40 && address <= 0xFF4B) {
            if (address == 0xFF44) return;

            if (address == 0xFF46) {
                oam_dma_transfer(value);
                m_memory[address] = value;
                return;
            }

            (*m_video)[address] = value;
            return;
        }

        // DMA Transfer register
        if (address == 0xFF46)
            oam_dma_transfer(value);

        // Bootrom mapping register
        if (address == 0xFF50)
            m_bootrom_mapped = (value == 0);

        m_memory[address] = value;
        return;
    } else if (address <= 0xfffe) {
        // FF80 - FFFE : High RAM
        m_memory[address] = value;
        return;
    } else {
        // FFFF : Interrupt Enable register
        m_cpu->interrupt_controller()[address] = value;
        return;
    }
    assert(!"unreachable code : write_byte(uint16_t, uint8_t)");
}

void MMU::connect_cpu(CPU *cpu) {
    m_cpu = cpu;
}

void MMU::connect_timer(Timer* timer) {
    m_timer = timer;
}

void MMU::request_interrupt(InterruptController::InterruptType type) {
    m_cpu->request_interrupt(type);
}

void MMU::connect_video(Video* video) {
    m_video = video;
}

void MMU::connect_cartridge(Cartridge* cartridge) {
    m_cartridge = cartridge;
}
