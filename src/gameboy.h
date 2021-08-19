#ifndef GAMEBOY_H
#define GAMEBOY_H

#include "cpu/cpu.h"
#include "mmu.h"
#include "timer.h"
#include "video/video.h"
#include "cartridge.h"
#include <cstdint>
#include <string>

class Gameboy
{
public:
    Gameboy();
    ~Gameboy();
    void Update();
    void Step();
    inline void SetRunning(bool running) { m_gb_running = running; }
    uint8_t* GetFramebuffer();
    void LoadROM(std::string path_to_rom);

private:
    CPU m_cpu;
    MMU m_mmu;
    Timer m_timer;
    Video m_video;
    Cartridge* m_cartridge;

    bool m_gb_running = false;

    friend class Debugger;
};

#endif // GAMEBOY_H
