#include "video.h"
#include "../cpu/interrupt_controller.h"
#include <stdexcept>

Video::Video(MMU& mmu)
    :m_scanline_counter(0), m_mmu(mmu) { }

uint8_t& Video::operator[](const int addr) {
    switch (addr) {
    case 0xFF40: return m_lcd_control;
    case 0xFF41: return m_lcd_status;
    case 0xFF42: return m_scroll_y;
    case 0xFF43: return m_scroll_x;
    case 0xFF44: return m_ly;
    case 0xFF45: return m_ly_compare;
    case 0xFF47: return m_bg_pallet;
    case 0xFF48: return m_obj_pallet0;
    case 0xFF49: return m_obj_pallet1;
    case 0xFF4A: return m_winy;
    case 0xFF4B: return m_winx;
    }

    throw std::invalid_argument("invalid argument. out-of-bounds of memory map of video subsystem");
}

bool Video::is_lcd_enabled() {
    return (m_lcd_control >> 7) == 1;
}

void Video::ppu_set_state(PPUState state) {
    m_lcd_status &= 0b11111100;
    m_lcd_status |= static_cast<uint8_t>(state);
}

Video::PPUState Video::ppu_get_state() {
    return static_cast<Video::PPUState>(m_lcd_status & 0b11);
}

bool Video::is_interrupt_enabled(PPUState state) {
    switch (state) {
    case PPUState::HBLANK: return (m_lcd_status & 0b00001000) != 0;
    case PPUState::VBLANK: return (m_lcd_status & 0b00010000) != 0;
    case PPUState::SEARCH_SPRITE_ATTRB: return (m_lcd_status & 0b00100000) != 0;
    case PPUState::TRANSFER_TO_LCD_DRIVER: return false;
    }
    throw std::invalid_argument("invalid argument. there is no interrupt enabled status for that state.");
}

void Video::set_coincidence_bit(bool coincidence) {
    m_lcd_status &= 0b11111011;
    if (coincidence) m_lcd_status |= 0b100;
}

void Video::update_graphics(int cycles) {
    if (is_lcd_enabled()) {
        m_scanline_counter += cycles;
    } else {
        m_scanline_counter = 0;
        m_ly = 0;
        ppu_set_state(PPUState::VBLANK);
        return;
    }

    PPUState old_state = ppu_get_state();

    if (m_ly >= 144) {
        // Status: VBLANK
        ppu_set_state(PPUState::VBLANK);
    } else if (m_scanline_counter < CYCLES_FOR_OBJ_ATTRB_SEARCH) {
        // Status: SEARCH_SPRITE_ATTRB
        ppu_set_state(PPUState::SEARCH_SPRITE_ATTRB);
    } else if (m_scanline_counter < CYCLES_FOR_OBJ_ATTRB_SEARCH + CYCLES_FOR_LCD_TRANSFER) {
        // Status: TRANSFER_TO_LCD_DRIVER
        ppu_set_state(PPUState::TRANSFER_TO_LCD_DRIVER);
    } else if (m_scanline_counter < CYCLES_FOR_OBJ_ATTRB_SEARCH + CYCLES_FOR_LCD_TRANSFER + CYCLES_FOR_HBLANK) {
        // Status: HBLANK
        ppu_set_state(PPUState::HBLANK);
    }

    if (old_state != ppu_get_state() && is_interrupt_enabled(ppu_get_state()))
        m_mmu.request_interrupt(InterruptController::LCD);

    if (m_ly == m_ly_compare) {
        set_coincidence_bit(true);
        if ((m_lcd_status & 0b01000000) != 0)
            m_mmu.request_interrupt(InterruptController::LCD);
    } else {
        set_coincidence_bit(false);
    }

    if (m_scanline_counter >= CYCLES_PER_SCANLINE) {
        m_ly++;
        m_scanline_counter = 0;
        if (m_ly == 144)
            m_mmu.request_interrupt(InterruptController::VBLANK);
        if (m_ly > 153)
            m_ly = 0;
        draw_scanline();
    }
}

void Video::draw_scanline() {
    if ((m_lcd_control & LCD_CTRL_BG_EN) != 0)
        render_tiles();
    if ((m_lcd_control & LCD_CTRL_OBJ_EN) != 0)
        render_sprites();
}

void Video::render_tiles() {
    uint16_t tiledata_address = 0;
    uint16_t tilemap_address = 0;
    uint8_t corrected_winx = m_winx - 7;
    bool unsigned_index = true;
    bool drawing_window = false;

    if ((m_lcd_control & LCD_CTRL_WIN_EN) != 0 && m_winy <= m_ly)
        drawing_window = true;

    if ((m_lcd_control & LCD_CTRL_BG_WIN_TILE_DATA_SELECT) != 0) {
        unsigned_index = true;
        tiledata_address = 0x8000;
    } else {
        unsigned_index = false;
        tiledata_address = 0x8800;
    }

    uint16_t x_pos, y_pos; // where in the 256x256 space are we drawing?
    uint16_t tile_row, tile_col; // which of the 32x32 tile are we drawing?
    if (drawing_window) {
        // drawing window
        if ((m_lcd_control & LCD_CTRL_WIN_TILEMAP_DISP_SELECT) != 0) {
            tilemap_address = 0x9C00;
        } else {
            tilemap_address = 0x9800;
        }
        y_pos = m_ly - m_winy;
    } else {
        // drawing background
        if ((m_lcd_control & LCD_CTRL_BG_TILEMAP_DISP_SELECT) != 0) {
            tilemap_address = 0x9C00;
        } else {
            tilemap_address = 0x9800;
        }
        y_pos = m_ly + m_scroll_y;
    }
    tile_row = y_pos / 8;

    for (int x = 0; x < 160; x++) {
        x_pos = x + m_scroll_x;
        if (drawing_window && (x_pos >= corrected_winx))
            x_pos = x - corrected_winx;
        tile_col = x_pos / 8;
        int16_t tile_idx;
        if (unsigned_index)
            tile_idx = (uint8_t)m_mmu.read_byte(tilemap_address + tile_row * 32 + tile_col);
        else
            tile_idx = (int8_t)m_mmu.read_byte(tilemap_address + tile_row * 32 + tile_col);
        uint16_t tile_address = tiledata_address + (unsigned_index? tile_idx * 16 : (tile_idx + 128)*16);
        uint8_t tile_line = y_pos % 8;

        uint8_t tile_data1 = m_mmu.read_byte(tile_address + 2*tile_line);
        uint8_t tile_data2 = m_mmu.read_byte(tile_address + 2*tile_line + 1);

        uint8_t color_bit = 7 - (x_pos % 8);
        uint8_t color_number = ((tile_data2 >> (color_bit - 1)) & 0b10) | ((tile_data1 >> color_bit) & 0b1);
        FB_COLOR color = get_color_from_pallet(color_number, m_bg_pallet);
        int y = m_ly;
        m_framebuffer.set_pixel(x, y, color);
    }
}

void Video::render_sprites() {

}

FB_COLOR Video::get_color_from_pallet(uint8_t color, uint8_t pallet) {
    FB_COLOR color_arr[] = {FB_COLOR::FB_WHITE, FB_COLOR::FB_LIGHT_GRAY, FB_COLOR::FB_DARK_GRAY, FB_COLOR::FB_BLACK};
    return color_arr[(pallet >> (color * 2)) & 0b11];
}

uint8_t* Video::get_framebuffer() {
    return m_framebuffer.get_buffer_ptr();
}

void Video::reset() {
    m_framebuffer.reset();
}
