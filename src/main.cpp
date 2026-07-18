/*******************************************************************************************
*
*   raylib-extras [ImGui] example - Simple Integration
*
*    This is a simple ImGui Integration
*    It is done using C++ but with C style code
*    It can be done in C as well if you use the C ImGui wrapper
*    https://github.com/cimgui/cimgui
*
*   Copyright (c) 2021 Jeffery Myers
*
********************************************************************************************/

#include "raylib.h"
#include "extras/IconsFontAwesome6.h"
#include "adjust.h"

#include "imgui.h"
#include "rlImGui.h"


// DPI scaling functions
float ScaleToDPIF(float value) {
    return GetWindowScaleDPI().x * value;
}

int ScaleToDPII(int value) {
    return int(GetWindowScaleDPI().x * value);
}

int main(int argc, char* argv[])
{
    // Initialization
    int screenWidth = 1280;
    int screenHeight = 800;

    adjust_init();


    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "Perihelion");
    SetTargetFPS(144);
    rlImGuiSetup(true);

    Texture image = LoadTexture("resources/parrots.png");

    ADJUST_CONST_INT(num, 8);

    // Main game loop
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(DARKGRAY);

        // start ImGui Conent
        rlImGuiBegin();

        // show ImGui Content
        bool open = true;
        ImGui::ShowDemoWindow(&open);

        open = true;
        if (ImGui::Begin("Test Window", &open)) {
            ImGui::TextUnformatted(ICON_FA_JEDI);
            ImGui::TextUnformatted(ICON_FA_ARROW_DOWN);
	    ImGui::Text("Hello #%d", num);

            rlImGuiImage(&image);
        }
        ImGui::End();

	adjust_update();

        // end ImGui Content
        rlImGuiEnd();

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            DrawText("Prssed", 0, 0, 20, RED);

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
            DrawText("Down", 0, 20, 20, GREEN);

        if (IsWindowFocused())
            DrawText("Focused", 100, 20, 20, WHITE);

        EndDrawing();
    }

    // De-Initialization
    rlImGuiShutdown();
    UnloadTexture(image);
    CloseWindow();        // Close window and OpenGL context

    return 0;
}
