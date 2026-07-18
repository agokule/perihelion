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
#include <iostream>
#include <raymath.h>
#include <string>


// DPI scaling functions
float ScaleToDPIF(float value) {
    return GetWindowScaleDPI().x * value;
}

int ScaleToDPII(int value) {
    return int(GetWindowScaleDPI().x * value);
}

enum class ObjectType {
    Planet,
    Star,
    BlackHole,
    Moon
};

void draw_text_centered(const std::string& text, Vector2 pos, int font_size, Color color) {
    auto [width, height] = MeasureTextEx(GetFontDefault(), text.c_str(), font_size, 2);

    Vector2 new_pos = { pos.x - width / 2, pos.y - height / 2 };
    DrawText(text.c_str(), new_pos.x, new_pos.y, font_size, color);
}

struct Object {
    ObjectType type;
    std::string name;
    // in kg
    float mass;
    // in km
    float radius;
    Vector3 position;

    void draw() const {
        DrawSphere(position, radius, YELLOW);
    }

    void draw_label(Camera3D& camera) const {
        int font_size = 15;

        float distance = Vector3Distance(position, camera.position);

        if (distance > 30)
            return;
        if (distance > 15)
            font_size = 5;
        if (distance > 10)
            font_size = 10;

        std::cout << distance << ',' << font_size << '\n';

        auto text_pos = GetWorldToScreen(position, camera);
        draw_text_centered(name, text_pos, font_size, GREEN);
    }
};

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

    // Define the camera to look into our 3d world
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 10.0f, 10.0f, 10.0f }; // Camera position
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type

    ADJUST_CONST_STRING(text, "Perihelion");
    ADJUST_CONST_BOOL(adjust_live, false);

    DisableCursor();
    Object sun { ObjectType::Star, "The Sun", 1, 1, { 0, 0 } };

    SetExitKey(KEY_NULL);

    // Main game loop
    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_FREE);

        BeginDrawing();
        ClearBackground(DARKGRAY);

        BeginMode3D(camera);

        sun.draw();
        DrawGrid(100, 1.0f);

        EndMode3D();

        sun.draw_label(camera);

        // start ImGui Conent
        rlImGuiBegin();

        if (ImGui::Begin("Test Window")) {
	    ImGui::Text("%s", text);
        }
        ImGui::End();

        if (adjust_live || IsKeyPressed(KEY_R))
            adjust_update();

        // end ImGui Content
        rlImGuiEnd();

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            DrawText("Prssed", 0, 0, 20, RED);
            DisableCursor();
        }
        if (IsKeyPressed(KEY_ESCAPE))
            EnableCursor();


        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
            DrawText("Down", 0, 20, 20, GREEN);

        if (IsWindowFocused())
            DrawText("Focused", 100, 20, 20, WHITE);

        EndDrawing();
    }

    // De-Initialization
    rlImGuiShutdown();
    CloseWindow();        // Close window and OpenGL context

    return 0;
}
