#include "FontIcons.hpp"
#include "Object.hpp"
#include "Vector3Double.hpp"
#include "raylib.h"
#include "adjust.h"
#include "imgui.h"
#include "raymath.h"
#include "ui/GridTypeEdit.hpp"
#include "ui/PlaybackControls.hpp"
#include "ui/SettingsEdit.hpp"
#include "ui/imgui_ui_utils.hpp"

// disable rlImGui's font awesom
#define NO_FONT_AWESOME
#include "rlImGui.h"

#include "AppState.hpp"
#include "SimulationScreen.hpp"
#include "Skybox.hpp"
#include "ui/ObjectEditor.hpp"
#include "ui/RightClickMenu.hpp"
#include "utils.hpp"
#include <format>
#include <iostream>
#include <optional>

void handle_right_click_menu_action(
    std::optional<RightClickActionSelected> action,
    const Camera& camera,
    const SimulationScreen& simulation,
    std::optional<Object>& adding_object,
    bool& paused
) {
    switch (*action) {
        case RightClickActionSelected::CreateObject:
        {
            Ray ray = GetScreenToWorldRay({0, 0}, camera);
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
}

void change_velocity_using_cone(
        const std::optional<Cone>& velocity_cone,
        const Camera3D& camera,
        bool& changing_velocity_of_obj,
        SimulationScreen& simulation,
        const SimulationSettings& settings
) {
    auto mouse_pos = GetMousePosition();
    auto cone_tip_pos = GetWorldToScreen(velocity_cone->tip.to_vector3(), camera);
    auto cone_base_pos = GetWorldToScreen(velocity_cone->base.to_vector3(), camera);

    float cone_screen_height = Vector2Length(cone_tip_pos - cone_base_pos);

    bool is_mouse_on_cone =
        Vector2Distance(cone_tip_pos, mouse_pos) < 9 ||
        Vector2Distance(cone_base_pos, mouse_pos) < 9;

    if (changing_velocity_of_obj || (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && is_mouse_on_cone)) {
        changing_velocity_of_obj = true;

        auto& selected = simulation.get_object(simulation.current_selected_object);

        // using camera_offset_from_selected instead of the real camera
        // because the real camera loses decimal points of precision when
        // far away from the origin (see camera_offset_from_selected's comment).
        Camera3D local_camera = camera;
        local_camera.position = simulation.camera_offset_from_selected().to_vector3();
        local_camera.target = Vector3Zero();

        Ray ray = GetScreenToWorldRay(mouse_pos, local_camera);
        auto pos = ray_y_plane_intersection(ray, 0.0f);

        if (pos) {
            // The rendered tip is surface_radius further out than
            // `velocity` alone would put it, so invert that full
            // equation
            Vector3Double pos_local {*pos};
            double dist_from_center = pos_local.length();
            double surface_radius = selected.radius * settings.objects_scale;

            if (dist_from_center > surface_radius) {
                double magnitude = (dist_from_center - surface_radius) / settings.velocity_arrow_scale;
                selected.velocity = pos_local.normalize() * magnitude;
            }
        } else
            changing_velocity_of_obj = false;
    }
    if (changing_velocity_of_obj && IsMouseButtonUp(MOUSE_BUTTON_LEFT))
        changing_velocity_of_obj = false;
}

int main(int argc, char* argv[]) {
    // Initialization
    int screenWidth = 1280;
    int screenHeight = 800;

    adjust_init();

    ADJUST_CONST_FLOAT(delta_time, 1.0f);
    ADJUST_CONST_INT(substeps_per_frame, 500);

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "Perihelion");
    SetTargetFPS(144);
    rlImGuiSetup(true);
    IMGUI_CHECKVERSION();

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
    ADJUST_CONST_FLOAT(objects_scale, 10.0f);
    ADJUST_CONST_FLOAT(velocity_arrow_scale, 5e3f);
    bool paused = false;
    bool camera_pan_enabled = true;

    DisableCursor();

    SetExitKey(KEY_NULL);

    AppState app_state = AppState::Simulation;
    SimulationScreen simulation(presets.at(0));
    Skybox skybox("./assets/images/myersalex216-space-2638158.jpg");
    std::optional<Vector2> right_click_location = std::nullopt;
    std::optional<Object> adding_object = std::nullopt;
    std::optional<Cone> velocity_cone = std::nullopt;
    bool changing_velocity_of_obj = false;
    GridSettings grid_settings {};

    bool demo_shown = false;
    bool metrics_shown = false;
    bool settings_window_shown = false;

    SimulationSettings settings {
        delta_time, substeps_per_frame,
        selected_sensitivity, objects_scale,
        paused, velocity_arrow_scale,
        grid_settings
    };

    auto calculate_starting_point_of_velocity_line = [](const Object& selected, const SimulationSettings& settings) {
        auto velocity_radius_length = selected.velocity.normalize() * selected.radius * settings.objects_scale;
        auto start = selected.position + velocity_radius_length;
        return start;
    };

    static const ImWchar icon_ranges[] = {
        0xF0000, 0xF0FFF, 
        0
    };

    ImFontConfig icons_config {};
    icons_config.MergeMode = true;
    icons_config.PixelSnapH = true; // Prevents blurry icons
    icons_config.GlyphRanges = icon_ranges;
    icons_config.ExtraSizeScale = 1.2f;
    icons_config.GlyphOffset = {1.0f, 1.0f};
    icons_config.SizePixels = 13.0f;

    auto& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("./assets/fonts/nerdfont/SymbolsNerdFont-Regular.ttf", 0.0f, &icons_config, icon_ranges);

    // Main game loop
    while (!WindowShouldClose()) {
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
                simulation.update_camera(camera, settings, camera_pan_enabled);

                BeginMode3D(camera);
                skybox.draw();
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
                    // drawing the velocity vector of the current object
                    const auto& selected = simulation.get_object(simulation.current_selected_object);

                    auto start = calculate_starting_point_of_velocity_line(selected, settings);
                    auto end = start + selected.velocity * settings.velocity_arrow_scale;

                    auto cone_height = selected.radius * settings.objects_scale / 20;

                    velocity_cone = draw_3d_arrow(start, end, cone_height);
                }

                EndMode3D();

                // draw 2d ui
                DrawFPS(10, 10);
                // start ImGui Conent
                rlImGuiBegin();

                if (demo_shown)
                    ImGui::ShowDemoWindow(&demo_shown);
                if (metrics_shown)
                    ImGui::ShowMetricsWindow(&metrics_shown);

                settings.paused = !PlaybackControls(!settings.paused, settings.delta_time);
                GridTypeEdit(settings.grid);
                {
                    ImGuiSetNextWindowPos({ .top = 10, .right = 10 });
                    RAIIStyleVar s {ImGuiStyleVar_WindowPadding, {4.0f, 4.0f}};
                    RAIIStyleVar s2 {ImGuiStyleVar_WindowMinSize, {0, 0}};
                    RAIIWindow win {"Open Settings", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings};
                    if (ImGui::Button(NF_COD_SETTINGS_GEAR))
                        settings_window_shown = !settings_window_shown;
                }
                if (settings_window_shown)
                    SettingsEdit(settings, settings_window_shown);

                if (adding_object) {
                    adding_object->draw_outline(settings.objects_scale, camera);
                    adding_object->draw_label(camera);
                    DrawText("Press Enter to confirm", 10, 10, 24, WHITE);
                }

                if (auto selection = simulation.draw_object_selection_ui(camera, settings))
                    simulation.select_object(*selection, settings);
                
                if (!ImGui::GetIO().WantCaptureKeyboard && !ImGui::GetIO().WantCaptureMouse) {
                    if (!IsCursorHidden() && IsCursorOnScreen() && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !adding_object) {
                        simulation.select_object({-1, 0}, settings);
                        right_click_location = GetMousePosition();
                        ImGui::OpenPopup("Right Click Menu");
                    }
                    if (velocity_cone &&
                        (is_object_in_camera(velocity_cone->base.to_vector3(), camera) ||
                        is_object_in_camera(velocity_cone->tip.to_vector3(), camera))
                    ) {
                        change_velocity_using_cone(velocity_cone, camera, changing_velocity_of_obj, simulation, settings);
                    }

                    if (IsKeyPressed(KEY_Z))
                        demo_shown = !demo_shown;
                    if (IsKeyPressed(KEY_X))
                        metrics_shown = !metrics_shown;
                }
                if ((!velocity_cone || simulation.current_selected_object == -1) && changing_velocity_of_obj)
                    changing_velocity_of_obj = false;

                if (simulation.current_selected_object != -1)
                    ObjectEditor(simulation.get_object(simulation.current_selected_object));

                if (right_click_location) {
                    auto action = RightClickMenu(*right_click_location);
                    if (action) {
                        std::cout << (int)*action << '\n';
                        handle_right_click_menu_action(action, camera, simulation, adding_object, paused);
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

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !changing_velocity_of_obj) {
                DisableCursor();
                camera_pan_enabled = true;
            }
            if (IsKeyPressed(KEY_ESCAPE)) {
                EnableCursor();
                camera_pan_enabled = false;
            }

            if (IsMouseButtonUp(MOUSE_BUTTON_LEFT) && changing_velocity_of_obj)
                changing_velocity_of_obj = false;

            if (IsKeyPressed(KEY_K))
                paused = !paused;

            if (adding_object && IsKeyPressed(KEY_ENTER)) {
                simulation.add_object(*adding_object);
                paused = false;
                adding_object = std::nullopt;
            }
        }

        EndDrawing();
    }

    // De-Initialization
    rlImGuiShutdown();
    CloseWindow();        // Close window and OpenGL context

    return 0;
}
