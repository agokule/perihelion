#include "raylib.h"
#include "extras/IconsFontAwesome6.h"
#include "adjust.h"

#include "imgui.h"
#include "rlImGui.h"
#include <cmath>
#include <iostream>
#include <raymath.h>
#include <string>
#include "Object.hpp"
#include "Preset.hpp"
#include "utils.hpp"

static Preset scene = presets.at(0);
constexpr double gravitationalConstant = 6.6743e-11;


void load_preset(const Preset& new_preset) {
    scene = new_preset;
}


int main(int argc, char* argv[]) {
    // Initialization
    int screenWidth = 1280;
    int screenHeight = 800;

    adjust_init();

    ADJUST_CONST_INT(delta_time, 10);
    ADJUST_CONST_INT(substeps_per_frame, 1000);

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

    SetExitKey(KEY_NULL);

    // Main game loop
    while (!WindowShouldClose()) {
        UpdateCamera(&camera, CAMERA_FREE);

        // simulate gravity
        for (int i = 0; i < substeps_per_frame; i++) {
            for (Object& obj: scene.objects) {
                for (Object& obj2 : scene.objects) {
                    if (obj2.name == obj.name)
                        continue;

                    float distance = convert_light_minutes_to_meters(Vector3Distance(obj.position, obj2.position));
                    float distanceSqr = distance * distance;
                    double gravity_acceleration = gravitationalConstant * obj2.mass / distanceSqr;
                    std::cout << gravity_acceleration << '\n';

                    Vector3 direction_of_acceleration = convert_light_minutes_to_meters(obj2.position - obj.position);
                    direction_of_acceleration /= distance;

                    Vector3 acceleration = direction_of_acceleration * gravity_acceleration;
                    obj.accelerate(convert_meters_to_light_minutes(acceleration), delta_time);
                }
            }
            for (Object& obj : scene.objects) {
                obj.update_pos(delta_time);
            }
        }

        BeginDrawing();
        ClearBackground(DARKGRAY);

        DrawFPS(10, 10);

        BeginMode3D(camera);

        for (const Object& obj : scene.objects)
            obj.draw();

        DrawGrid(100, 0.5f);

        EndMode3D();

        for (const Object& obj : scene.objects)
            obj.draw_label(camera);

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
