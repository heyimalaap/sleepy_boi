#include "raylib.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>

#include "gameboy.h"
#include "debugger.h"
#include "utility.h"

#define RAYGUI_IMPLEMENTATION
#define RAYGUI_SUPPORT_ICONS
#include "raygui.h"

#undef RAYGUI_IMPLEMENTATION            // Avoid including raygui implementation again


class GUI {
public:
    GUI(Gameboy& gb, Debugger& debugger) : m_gb(gb), m_debugger(debugger) {}

    void Paint() {
        // debugger side panel
        GuiPanel(Rectangle {0, 0, sidepanel_width, screen_height});

        reset_step_button_group(0, 0);
        cpu_state_panel(0, 60);
        disassembly_panel(0, 500);

        // main gui
        GuiPanel(Rectangle {sidepanel_width, 0, main_gui_width, screen_height});
        GuiGrid(Rectangle {sidepanel_width, 0, main_gui_width, screen_height}, 10, 2);
    }

private:
    Rectangle paddedRectangle(float x, float y, float w, float h) {
        return Rectangle {x + padding, y + padding, w - 2*padding, h - 2*padding};
    }

    void reset_step_button_group(float x, float y) {
        if (GuiButton(paddedRectangle(x + 0, y + 0, sidepanel_width/2 - 30, 60), "Reset")) {
            m_debugger.reset();
        }

        m_gb.SetRunning(GuiToggle(paddedRectangle(x + sidepanel_width/2 - 40, y + 0, 80, 60), m_debugger.gb_is_running() ? "#132#" : "#131#", m_debugger.gb_is_running()));

        if (GuiButton(paddedRectangle(x + sidepanel_width/2 + 30, y + 0, sidepanel_width/2 - 30, 60), "Step")) {
            m_debugger.step();
        }
    }

    void cpu_state_panel(float x, float y) {
        GuiPanel(paddedRectangle(x, y, sidepanel_width, 440));
        DrawRectangle(x + padding + 10, y + 0, 100, 20, RAYWHITE);
        GuiLabel(Rectangle{x + padding + 10, y + 0, sidepanel_width, 20}, "CPU State");

        // 8-bit registers
        GuiPanel(paddedRectangle(x + padding, y + 25, sidepanel_width - 2*padding, 180 - 2*padding));
        DrawRectangle(x + padding + 25, y + 25, 160, 20, GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
        GuiLabel(Rectangle{x + padding + 35, y + 25, sidepanel_width, 20}, "8-bit registers");

        GuiLabel(Rectangle {x + padding + 40     , y + 70}, "A : ");
        GuiLabel(Rectangle {x + padding + 40 + 50, y + 70}, to_hex(m_debugger.cpu_r8_a()).c_str());

        GuiLabel(Rectangle {x + padding + 220     , y + 70}, "F : ");
        GuiLabel(Rectangle {x + padding + 220 + 50, y + 70}, to_hex(m_debugger.cpu_r8_f()).c_str());

        GuiLabel(Rectangle {x + padding + 40     , y + 95}, "B : ");
        GuiLabel(Rectangle {x + padding + 40 + 50, y + 95}, to_hex(m_debugger.cpu_r8_b()).c_str());

        GuiLabel(Rectangle {x + padding + 220     , y + 95}, "C : ");
        GuiLabel(Rectangle {x + padding + 220 + 50, y + 95}, to_hex(m_debugger.cpu_r8_c()).c_str());

        GuiLabel(Rectangle {x + padding + 40     , y + 120}, "D : ");
        GuiLabel(Rectangle {x + padding + 40 + 50, y + 120}, to_hex(m_debugger.cpu_r8_d()).c_str());

        GuiLabel(Rectangle {x + padding + 220     , y + 120}, "E : ");
        GuiLabel(Rectangle {x + padding + 220 + 50, y + 120}, to_hex(m_debugger.cpu_r8_e()).c_str());

        GuiLabel(Rectangle {x + padding + 40     , y + 145}, "H : ");
        GuiLabel(Rectangle {x + padding + 40 + 50, y + 145}, to_hex(m_debugger.cpu_r8_h()).c_str());

        GuiLabel(Rectangle {x + padding + 220     , y + 145}, "L : ");
        GuiLabel(Rectangle {x + padding + 220 + 50, y + 145}, to_hex(m_debugger.cpu_r8_l()).c_str());

        // 16-bit registers
        GuiPanel(paddedRectangle(x + padding, y + 185, sidepanel_width - 2*padding, 140 - 2*padding));
        DrawRectangle(x + padding + 25, y + 185, 165, 20, GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
        GuiLabel(Rectangle{x + padding + 35, y + 185, sidepanel_width, 20}, "16-bit registers");

        GuiLabel(Rectangle {x + padding + 40     , y + 225}, "AF : ");
        GuiLabel(Rectangle {x + padding + 40 + 50, y + 225}, to_hex(m_debugger.cpu_r16_af(), 4).c_str());

        GuiLabel(Rectangle {x + padding + 220     , y + 225}, "BC : ");
        GuiLabel(Rectangle {x + padding + 220 + 50, y + 225}, to_hex(m_debugger.cpu_r16_bc(), 4).c_str());

        GuiLabel(Rectangle {x + padding + 40     , y + 250}, "DE : ");
        GuiLabel(Rectangle {x + padding + 40 + 50, y + 250}, to_hex(m_debugger.cpu_r16_de(), 4).c_str());

        GuiLabel(Rectangle {x + padding + 220     , y + 250}, "HL : ");
        GuiLabel(Rectangle {x + padding + 220 + 50, y + 250}, to_hex(m_debugger.cpu_r16_hl(), 4).c_str());

        GuiLabel(Rectangle {x + padding + 40     , y + 275}, "SP : ");
        GuiLabel(Rectangle {x + padding + 40 + 50, y + 275}, to_hex(m_debugger.cpu_sp(), 4).c_str());

        GuiLabel(Rectangle {x + padding + 220     , y + 275}, "PC : ");
        GuiLabel(Rectangle {x + padding + 220 + 50, y + 275}, to_hex(m_debugger.cpu_pc(), 4).c_str());

        // Flag register
        GuiPanel(paddedRectangle(x + padding, y + 300, sidepanel_width - 2*padding, 140 - 2*padding));
        DrawRectangle(x + padding + 25, y + 300, 120, 20, GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
        GuiLabel(Rectangle{x + padding + 35, y + 300, sidepanel_width, 20}, "CPU Flags");

        GuiCheckBox(Rectangle{x + padding + 40, y + 330, 20, 20}, "Z", m_debugger.cpu_flag_z());
        GuiCheckBox(Rectangle{x + padding + 115, y + 330, 20, 20}, "N", m_debugger.cpu_flag_n());
        GuiCheckBox(Rectangle{x + padding + 190, y + 330, 20, 20}, "H", m_debugger.cpu_flag_h());
        GuiCheckBox(Rectangle{x + padding + 265, y + 330, 20, 20}, "C", m_debugger.cpu_flag_c());
        GuiCheckBox(Rectangle{x + padding + 40, y + 355, 20, 20}, "Interrupt Enable", m_debugger.cpu_flag_ime());
        GuiCheckBox(Rectangle{x + padding + 40, y + 380, 20, 20}, "Waiting for Interrupt", m_debugger.cpu_flag_iw());
    }

    void disassembly_panel(float x, float y) {
        GuiPanel(paddedRectangle(x, y, sidepanel_width, 500));
        DrawRectangle(x + padding + 10, y + 0, 140, 20, RAYWHITE);
        GuiLabel(Rectangle{x + padding + 10, y + 0, sidepanel_width, 20}, "DISASSEMBLY");
        uint16_t current_pc = m_debugger.cpu_pc();
        uint16_t offset = 0;
        for (int i = 0; i < 18; i++) {
            auto [instruction_str, instruction_size] = m_debugger.disassemble_instruction(current_pc + offset);
            if (i == 0) GuiLabel(Rectangle {x + padding + 20, y + 40 + i*25}, "PC->");
            GuiLabel(Rectangle {x + padding + 70, y + 40 + i*25}, to_hex(current_pc + offset, 4).c_str());
            GuiLabel(Rectangle {x + padding + 150, y + 40 + i*25}, instruction_str.c_str());
            offset += instruction_size;
        }
    }

    void gameboy_video_out(float x, float y) {
        //GuiPanel(Rectangle{x - 2, y - 2, 160 * 3 + 4, 144 * 3 + 4});
        uint8_t* gb_fb_ptr =  m_gb.GetFramebuffer();
        Image gb_fb_img = {
            .data = gb_fb_ptr,
            .width = 160,
            .height = 144,
            .mipmaps = 1,
            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8
        };
        Texture2D gb_fb_tx = LoadTextureFromImage(gb_fb_img);
        DrawTexture(gb_fb_tx, x, y, WHITE);
        //DrawTextureQuad(gb_fb_tx, Vector2 {1.0f, 1.0f}, Vector2 {0.0f, 0.0f}, Rectangle {x, y, 160 * 3, 144 * 3}, Color {255, 255, 255 ,255});
        UnloadTexture(gb_fb_tx);
    }


private:
    float screen_height = GetScreenHeight();
    float screen_width  = GetScreenWidth();

    constexpr static int padding = 10;
    constexpr static int sidepanel_width = 380;
    float main_gui_width = screen_width - sidepanel_width;

    Gameboy& m_gb;
    Debugger& m_debugger;
};

#include "timer.h"

int main() {
    constexpr int screenWidth = 1300;
    constexpr int screenHeight = 1000;
    SetTraceLogLevel(LOG_NONE);
    InitWindow(screenWidth, screenHeight, "sleepy-boi");
    SetExitKey(KEY_ESCAPE);
    SetTargetFPS(60);
    // DisableCursor();
    // ToggleFullscreen();

    Gameboy gb;
    gb.LoadROM("D:\\projects\\sleepy_boi\\res\\cpu_instrs.gb");
    Debugger debugger(gb);
    GUI gui(gb, debugger);

    while(!WindowShouldClose()) {
        gb.Update();
        Image gb_fb_img = {
            .data = gb.GetFramebuffer(),
            .width = 160,
            .height = 144,
            .mipmaps = 1,
            .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8
        };
        Texture2D gb_fb_tx = LoadTextureFromImage(gb_fb_img);


        BeginDrawing();

        // Gui Style settings
        GuiSetStyle(DEFAULT, TEXT_SIZE, 20);

        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
        gui.Paint();

        DrawTextureQuad(gb_fb_tx, Vector2 {1.0f, 1.0f}, Vector2 {0.0f, 0.0f}, Rectangle {(float)(190 + GetScreenWidth() / 2 - 80 * 3), (float)(GetScreenHeight() / 2 - 80*3), 160 * 3, 144 * 3}, WHITE);

        // Dumb custom cursor
        float mouseX = GetMouseX(), mouseY = GetMouseY();
        DrawLineEx(Vector2 {mouseX, mouseY}, Vector2 {mouseX + 10, mouseY}, 2, BLACK);
        DrawLineEx(Vector2 {mouseX, mouseY}, Vector2 {mouseX, mouseY + 10}, 2, BLACK);
        DrawLineEx(Vector2 {mouseX, mouseY}, Vector2 {mouseX + 15, mouseY + 15}, 2, BLACK);
        EndDrawing();

        UnloadTexture(gb_fb_tx);
    }

    CloseWindow();
}
