#ifndef VIDEO_H
#define VIDEO_H

#include <cstdint>
#include "../mmu.h"
#include "framebuffer.h"

class MMU;

class Video
{
public:
    Video(MMU& mmu);
    uint8_t& operator[](const int addr);
    void update_graphics(int cycles);
    uint8_t* get_framebuffer();
    void reset();
private:
    uint8_t m_lcd_control = 0x91;
    uint8_t m_lcd_status;
    uint8_t m_scroll_y = 0;
    uint8_t m_scroll_x = 0;
    uint8_t m_ly = 0;
    uint8_t m_ly_compare = 0;
    uint8_t m_bg_pallet = 0xFC;
    uint8_t m_obj_pallet0 = 0xFF;
    uint8_t m_obj_pallet1 = 0xFF;
    uint8_t m_winy = 0;
    uint8_t m_winx = 0;

    int m_scanline_counter;
    Framebuffer m_framebuffer;

    MMU& m_mmu;

    enum class PPUState {
        HBLANK = 0b00,
        VBLANK = 0b01,
        SEARCH_SPRITE_ATTRB = 0b10,
        TRANSFER_TO_LCD_DRIVER = 0b11
    };

    static constexpr int CYCLES_PER_SCANLINE = 456;
    static constexpr int CYCLES_FOR_OBJ_ATTRB_SEARCH = 80;
    static constexpr int CYCLES_FOR_LCD_TRANSFER = 172;
    static constexpr int CYCLES_FOR_HBLANK = CYCLES_PER_SCANLINE - (CYCLES_FOR_LCD_TRANSFER + CYCLES_FOR_OBJ_ATTRB_SEARCH);

    static constexpr uint8_t LCD_CTRL_ENABLE = (1 << 7);
    static constexpr uint8_t LCD_CTRL_WIN_TILEMAP_DISP_SELECT = (1 << 6);
    static constexpr uint8_t LCD_CTRL_WIN_EN = (1 << 5);
    static constexpr uint8_t LCD_CTRL_BG_WIN_TILE_DATA_SELECT = (1 << 4);
    static constexpr uint8_t LCD_CTRL_BG_TILEMAP_DISP_SELECT = (1 << 3);
    static constexpr uint8_t LCD_CTRL_OBJ_SIZE = (1 << 2);
    static constexpr uint8_t LCD_CTRL_OBJ_EN = (1 << 1);
    static constexpr uint8_t LCD_CTRL_BG_EN = 1;

    bool is_lcd_enabled();
    void ppu_set_state(PPUState state);
    PPUState ppu_get_state();
    bool is_interrupt_enabled(PPUState state);
    void set_coincidence_bit(bool coincidence);
    void draw_scanline();
    void render_tiles();
    void render_sprites();
    FB_COLOR get_color_from_pallet(uint8_t color, uint8_t pallet);
};

#endif // VIDEO_H
