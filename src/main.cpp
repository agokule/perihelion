#include "raylib.h"
#include "extras/IconsFontAwesome6.h"
#include "adjust.h"

#include "imgui.h"
#include "rlImGui.h"
#include "AppState.hpp"
#include "SimulationScreen.hpp"


int main(int argc, char* argv[]) {
    // Initialization
    int screenWidth = 1280;
    int screenHeight = 800;

    adjust_init();

    ADJUST_CONST_INT(delta_time, 1);
    ADJUST_CONST_INT(substeps_per_frame, 500);

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "Perihelion");
    SetTargetFPS(144);
    rlImGuiSetup(true);

    // Define the camera to look into our 3d world
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 50.0f, 50.0f, 50.0f }; // Camera position
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type

    ADJUST_CONST_STRING(text, "Perihelion");
    ADJUST_CONST_BOOL(adjust_live, false);
    ADJUST_CONST_FLOAT(selected_sensitivity, 0.005f);
    ADJUST_CONST_FLOAT(objects_scale, 5.0f);
    ADJUST_CONST_INT(spacetime_curve_factor, 600);
    bool camera_pan_enabled = true;

    size_t frame_counter = 0;

    DisableCursor();

    SetExitKey(KEY_NULL);

    AppState app_state = AppState::Simulation;
    SimulationScreen simulation(presets.at(0));

    // Main game loop
    while (!WindowShouldClose()) {
        SimulationSettings settings{delta_time, substeps_per_frame, selected_sensitivity, objects_scale, spacetime_curve_factor};

        camera_pan_enabled = IsCursorHidden();
        if (camera_pan_enabled)
            UpdateCamera(&camera, simulation.current_selected_object == -1 ? CAMERA_FREE : CAMERA_CUSTOM);

        BeginDrawing();
        ClearBackground(DARKGRAY);

        switch (app_state) {
            case AppState::PresetSelection:
                // TODO: draw the preset selection screen and transition to
                // AppState::Simulation once the user picks one
                break;

            case AppState::Simulation:
                simulation.simulate_physics(settings);
                simulation.update_camera(camera, settings, camera_pan_enabled, frame_counter);

                simulation.draw_world(camera, settings);

                // draw 2d ui
                DrawFPS(10, 10);
                // start ImGui Conent
                rlImGuiBegin();

                if (auto selection = simulation.draw_object_selection_ui(camera, settings))
                    simulation.select_object(*selection, settings, frame_counter);

                // end ImGui Content
                rlImGuiEnd();
                break;
        }

        if (!ImGui::GetIO().WantCaptureKeyboard && !ImGui::GetIO().WantCaptureMouse) {
            if (adjust_live || IsKeyPressed(KEY_R))
                adjust_update();

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                DrawText("Prssed", 0, 0, 20, RED);
                DisableCursor();
                camera_pan_enabled = true;
            }
            if (IsKeyPressed(KEY_ESCAPE)) {
                EnableCursor();
                camera_pan_enabled = false;
            }
        }

        EndDrawing();
        frame_counter++;
    }

    // De-Initialization
    rlImGuiShutdown();
    CloseWindow();        // Close window and OpenGL context

    return 0;
}
