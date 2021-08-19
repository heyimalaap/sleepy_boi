#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <cstdint>

enum class FB_COLOR {
    FB_OFF,
    FB_WHITE,
    FB_LIGHT_GRAY,
    FB_DARK_GRAY,
    FB_BLACK
};

class Framebuffer
{
public:
    Framebuffer();
    void set_pixel(int x, int y, FB_COLOR color);
    uint8_t* get_buffer_ptr();
    void reset();
private:
    uint8_t m_buffer[160*144*3];
};

#endif // FRAMEBUFFER_H
