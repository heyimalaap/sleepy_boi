#include "framebuffer.h"

Framebuffer::Framebuffer() {
    reset();
}

void Framebuffer::reset() {
    for (int i = 0; i < 160; i++)
        for (int j = 0; j < 144; j++)
            set_pixel(i, j, FB_COLOR::FB_OFF);
}

void Framebuffer::set_pixel(int x, int y, FB_COLOR color) {
    if (x < 0 || x >= 160 || y < 0 || y >= 144)
        return;

    int r, g, b;
    switch (color) {
    case FB_COLOR::FB_BLACK:
        r = 0, g = 0, b = 0;
        break;
    case FB_COLOR::FB_DARK_GRAY:
        r = 102, g = 75, b = 0;
        break;
    case FB_COLOR::FB_LIGHT_GRAY:
        r = 168, g = 123, b = 0;
        break;
    case FB_COLOR::FB_WHITE:
        r = 255, g = 187, b = 0;
        break;
    case FB_COLOR::FB_OFF:
        r = 77, g = 77, b = 77;
    }

    m_buffer[(x + 160*y)*3 + 0] = r;
    m_buffer[(x + 160*y)*3 + 1] = g;
    m_buffer[(x + 160*y)*3 + 2] = b;
}

uint8_t* Framebuffer::get_buffer_ptr() {
    return m_buffer;
}
