#include "Object.hpp"
#include "Vector3Double.hpp"
#include "raylib.h"
#include "extras/IconsFontAwesome6.h"
#include "adjust.h"

#include "imgui.h"
#include "raymath.h"
#include "rlImGui.h"
#include "AppState.hpp"
#include "SimulationScreen.hpp"
#include "ui/ObjectEditor.hpp"
#include "ui/RightClickMenu.hpp"
#include "utils.hpp"
#include <format>
#include <iostream>
#include <optional>


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
    ADJUST_CONST_FLOAT(velocity_arrow_scale, 5e3f);
    bool paused = false;
    bool camera_pan_enabled = true;

    size_t frame_counter = 0;

    DisableCursor();

    SetExitKey(KEY_NULL);

    AppState app_state = AppState::Simulation;
    SimulationScreen simulation(presets.at(0));
    std::optional<Vector2> right_click_location = std::nullopt;
    std::optional<Object> adding_object = std::nullopt;
    std::optional<Cone> velocity_cone = std::nullopt;

    // Main game loop
    while (!WindowShouldClose()) {
        SimulationSettings settings {
            delta_time, substeps_per_frame,
            selected_sensitivity, objects_scale,
            spacetime_curve_factor, paused,
            velocity_arrow_scale
        };

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
                if (adding_object && camera_pan_enabled)
                    DisableCursor();

                simulation.simulate_physics(settings);
                simulation.update_camera(camera, settings, camera_pan_enabled, frame_counter);

                BeginMode3D(camera);
                simulation.draw_world(camera, settings);

                if (adding_object) {
                    Ray ray = GetScreenToWorldRay(GetMousePosition(), camera);
                    auto pos = ray_y_plane_intersection(ray);
                    if (!pos.has_value())
                        pos = ray.position + ray.direction * 15;
                    adding_object->position = *pos;
                    adding_object->draw(settings.objects_scale);
                    adding_object->draw_trail();
                }
                if (simulation.current_selected_object != -1) {
                    const auto& selected = simulation.get_object(simulation.current_selected_object);

                    auto velocity_radius_length = selected.velocity.normalize() * selected.radius * settings.objects_scale;
                    auto start = selected.position + velocity_radius_length;
                    auto end = start + selected.velocity * settings.velocity_arrow_scale;

                    auto cone_height = selected.radius * settings.objects_scale / 20;

                    velocity_cone = draw_3d_arrow(start, end, cone_height);
                }

                EndMode3D();

                // draw 2d ui
                DrawFPS(10, 10);
                // start ImGui Conent
                rlImGuiBegin();

                if (adding_object) {
                    adding_object->draw_outline(settings.objects_scale, camera);
                    adding_object->draw_label(camera);
                    DrawText("Press Enter to confirm", 10, 10, 24, WHITE);
                }

                if (auto selection = simulation.draw_object_selection_ui(camera, settings)) {
                    simulation.select_object(*selection, settings, frame_counter);
                    if (!selection.has_value())
                        velocity_cone = std::nullopt;
                }

                
                if (!ImGui::GetIO().WantCaptureKeyboard && !ImGui::GetIO().WantCaptureMouse) {
                    if (!IsCursorHidden() && IsCursorOnScreen() && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !adding_object) {
                        simulation.select_object({-1, 0}, settings, frame_counter);
                        right_click_location = GetMousePosition();
                        ImGui::OpenPopup("Right Click Menu");
                    }
                }

                if (simulation.current_selected_object != -1)
                    ObjectEditor(simulation.get_object(simulation.current_selected_object));

                if (right_click_location) {
                    auto action = RightClickMenu(*right_click_location);
                    if (action) {
                        std::cout << (int)*action << '\n';
                        switch (*action) {
                            case RightClickActionSelected::CreateObject:
                            {
                                Ray ray = GetScreenToWorldRay(*right_click_location, camera);
                                Vector3Double pos = ray.position + ray.direction * 15;
                                adding_object = Object {
                                    ObjectType::Planet,
                                    std::format("New Object ({})", simulation.num_objects()),
                                    1e20,
                                    0.3,
                                    pos,
                                    Vector3Zero(),
                                    WHITE
                                };
                                paused = true;
                                break;
                            }
                            case RightClickActionSelected::EditObject:
                                // TODO: implement this
                                break;
                            case RightClickActionSelected::FocusOnObject:
                                // TODO: implement this
                                break;
                        }
                        right_click_location = std::nullopt;
                    }
                }

                // end ImGui Content
                rlImGuiEnd();
                break;
        }

        if (!ImGui::GetIO().WantCaptureKeyboard && !ImGui::GetIO().WantCaptureMouse) {
            if (adjust_live || IsKeyPressed(KEY_R))
                adjust_update();

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                DisableCursor();
                camera_pan_enabled = true;
            }
            if (IsKeyPressed(KEY_ESCAPE)) {
                EnableCursor();
                camera_pan_enabled = false;
            }

            if (adding_object && IsKeyPressed(KEY_ENTER)) {
                simulation.add_object(*adding_object);
                paused = false;
                adding_object = std::nullopt;
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
