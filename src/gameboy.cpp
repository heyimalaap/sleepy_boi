#include "gameboy.h"
#include "cartridge.h"
#include <iterator>
#include <vector>
#include <fstream>

Gameboy::Gameboy()
    : m_cpu(m_mmu), m_timer(m_mmu), m_video(m_mmu) {
    m_mmu.connect_cpu(&m_cpu);
    m_mmu.connect_timer(&m_timer);
    m_mmu.connect_video(&m_video);
}

Gameboy::~Gameboy() {
    delete m_cartridge;
}

void Gameboy::Update() {
    if (!m_gb_running) return;
    constexpr int CPU_CLOCK_FREQUENCY = 4194304; // 4ish MHz (from gameboy's specs)
    constexpr int FRAMERATE = 60;
    constexpr int CPU_CLOCK_CYCLES_PER_FRAME = CPU_CLOCK_FREQUENCY / FRAMERATE;

    int cycles_so_far = 0;
    while (cycles_so_far < CPU_CLOCK_CYCLES_PER_FRAME){
        int cycles = m_cpu.execute_next_opcode();
        cycles_so_far += cycles;
        m_timer.tick(cycles);
        m_video.update_graphics(cycles);
        m_cpu.handle_interrupts();
    }
}

void Gameboy::Step() {
    int cycles = m_cpu.execute_next_opcode();
    m_timer.tick(cycles);
    m_video.update_graphics(cycles);
    m_cpu.handle_interrupts();
}

uint8_t* Gameboy::GetFramebuffer() {
    return m_video.get_framebuffer();
}

void Gameboy::LoadROM(std::string path_to_rom) {
    std::ifstream file(path_to_rom, std::ios::binary);
    file.unsetf(std::ios::skipws);
    std::streampos fileSize;
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> rom_data;
    rom_data.reserve(fileSize);
    rom_data.insert(rom_data.begin(),
                    std::istream_iterator<uint8_t>(file),
                    std::istream_iterator<uint8_t>());

    CartridgeHeader header = get_header_from_romdata(rom_data);

    switch (header.type) {
    case CartridgeType::NoMBC:
        m_cartridge = new CartridgeNoMBC(rom_data);
        break;
    case CartridgeType::MBC1:
        m_cartridge = new CartridgeMBC1(rom_data);
        break;
    }

    m_mmu.connect_cartridge(m_cartridge);
}
