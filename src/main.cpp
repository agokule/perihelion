#include "FontIcons.hpp"
#include "physics/Object.hpp"
#include "Vector3Double.hpp"
#include "physics/settings.hpp"
#include "raylib.h"
#include "adjust.h"
#include "imgui.h"
#include "raymath.h"
#include "ui/GridTypeEdit.hpp"
#include "ui/PlaybackControls.hpp"
#include "ui/SettingsEdit.hpp"
#include "ui/imgui_ui_utils.hpp"
#include "viewport/camera.hpp"

// disable rlImGui's font awesom
#define NO_FONT_AWESOME
#include "rlImGui.h"

#include "AppState.hpp"
#include "physics/SimulationScreen.hpp"
#include "viewport/Skybox.hpp"
#include "ui/ObjectEditor.hpp"
#include "ui/RightClickMenu.hpp"
#include "viewport/utils.hpp"
#include <format>
#include <iostream>
#include <optional>
#include <utility>

enum class Axis {
    X, Y, Z
};

struct AxisChangeInfo {
    Axis axis;

    float horizontal_speed;
    float vertical_speed;

    Vector2 original_mouse_pos;

    // This is the object-relative offset the arrow tip had when the
    // axis lock engaged (same frame as the non-locked branch below), not
    // the object's absolute world position -- otherwise the two
    // non-edited axes carry light-seconds-scale world coordinates into
    // the pos_local/normalize() math below and the velocity flies off.
    Vector3 original_pos;
};

Vector3Double axis_unit_vector(Axis axis) {
    switch (axis) {
        case Axis::X: return Vector3Double{1.0, 0.0, 0.0};
        case Axis::Y: return Vector3Double{0.0, 1.0, 0.0};
        case Axis::Z: return Vector3Double{0.0, 0.0, 1.0};
    }
    std::unreachable();
}

std::optional<Axis> get_axis_from_key(KeyboardKey key) {
    switch (key) {
        case KEY_X:
            return Axis::X;
        case KEY_Z:
            return Axis::Z;
        case KEY_Y:
            return Axis::Y;
        default:
            return std::nullopt;
    }
};

void handle_right_click_menu_action(
    std::optional<RightClickActionSelected> action,
    const Camera& camera,
    const SimulationScreen& simulation,
    std::optional<Object>& adding_object,
    const SimulationSettings& settings,
    std::optional<SimulationSettings>& temp_state
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
                0.005,
                pos,
                Vector3Zero(),
                WHITE
            };
            temp_state = settings;
            temp_state->paused = true;
            temp_state->grid.type = GridType::Flat;
            temp_state->grid.flat_grid_y = 0.0f;
            temp_state->grid.flat_grid_slices = 100;
            temp_state->grid.spacing_between_slices = 1;
            temp_state->grid.grid_color = GREEN;
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

bool change_velocity_using_cone(
        const std::optional<Cone>& velocity_cone,
        const Camera3D& camera,
        bool& changing_velocity_of_obj,
        SimulationScreen& simulation,
        const SimulationSettings& settings,
        std::optional<SimulationSettings>& temp_state,
        const std::optional<AxisChangeInfo>& axis_lock
) {
    changing_velocity_of_obj = true;
    auto& selected = simulation.get_object(simulation.current_selected_object);

    if (!temp_state) {
        temp_state = settings;
        temp_state->grid = {
            .flat_grid_slices = 100,
            .flat_grid_y = static_cast<float>(selected.position.y + selected.velocity.y),
            .type = GridType::Flat,
            .spacing_between_slices = 1,
            .grid_color = ORANGE,
        };
    }

    std::optional<Vector3> pos = std::nullopt;
    auto mouse_pos = GetMousePosition();

    if (!axis_lock) {
        // using camera_offset_from_selected instead of the real camera
        // because the real camera loses decimal points of precision when
        // far away from the origin (see camera_offset_from_selected's comment).
        Camera3D local_camera = camera;
        local_camera.position = simulation.camera_offset_from_selected().to_vector3();
        local_camera.target = Vector3Zero();

        Ray ray = GetScreenToWorldRay(mouse_pos, local_camera);
        pos = ray_y_plane_intersection(ray, 0.0f);
    } else {
        pos = axis_lock->original_pos;

        float* to_edit = nullptr;
        const float* original = nullptr;
        switch (axis_lock->axis) {
            case Axis::X:
                to_edit = &pos->x;
                original = &axis_lock->original_pos.x;
                break;
            case Axis::Y:
                to_edit = &pos->y;
                original = &axis_lock->original_pos.y;
                break;
            case Axis::Z:
                to_edit = &pos->z;
                original = &axis_lock->original_pos.z;
                break;
        }

        auto mouse_change = mouse_pos - axis_lock->original_mouse_pos;
        // horizontal_speed/vertical_speed already carry the pixels-per-world-unit
        // scale (see capture site below), so this dot product yields world
        // units directly -- no separate sensitivity constant needed.
        float change_along_axis = mouse_change.x * axis_lock->horizontal_speed + mouse_change.y * axis_lock->vertical_speed;
        *to_edit = *original + change_along_axis;
    }

    if (pos) {
        double surface_radius = selected.radius * settings.objects_scale;
        auto velocity = arrow_offset_to_velocity(Vector3Double{*pos}, surface_radius, settings.velocity_arrow_scale);
        if (velocity)
            selected.velocity = *velocity;
        return false;
    } else
        return true;
}

int main(int argc, char* argv[]) {
    // Initialization
    int screenWidth = 1280;
    int screenHeight = 800;

    adjust_init();

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

    ADJUST_CONST_BOOL(adjust_live, false);
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
    std::optional<AxisChangeInfo> changing_velocity_axis_info = std::nullopt;
    GridSettings grid_settings {};

    bool demo_shown = false;
    bool metrics_shown = false;
    bool settings_window_shown = false;

    SimulationSettings settings {
        .delta_time = 1.0f,
        .substeps_per_frame = 500,
        .selected_sensitivity = 0.005f,
        .objects_scale = 10.0f,
        .paused = false,
        .velocity_arrow_scale = 5e3f,
        .grid = GridSettings {}
    };
    std::optional<SimulationSettings> temp_state = std::nullopt;
    auto get_settings_state = [&settings, &temp_state]() -> SimulationSettings& {
        return temp_state ? *temp_state : settings;
    };

    auto calculate_starting_point_of_velocity_line = [](const Object& selected, const SimulationSettings& settings) {
        auto velocity_radius_length = selected.velocity.normalize() * selected.radius * settings.objects_scale;
        auto start = selected.position + velocity_radius_length;
        return start;
    };

    auto stop_changing_velocity = [&changing_velocity_of_obj, &temp_state, &changing_velocity_axis_info]() {
        changing_velocity_of_obj = false;
        temp_state = std::nullopt;
        changing_velocity_axis_info = std::nullopt;
    };

    auto stop_adding_object = [&simulation, &adding_object, &temp_state]() {
        simulation.add_object(*adding_object);
        temp_state = std::nullopt;
        adding_object = std::nullopt;
    };

    auto imgui_wants_io = []() { return ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantCaptureMouse; };

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
            update_camera(&camera, simulation.current_selected_object == -1 ? CAMERA_FREE : CAMERA_CUSTOM);

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

                simulation.simulate_physics(get_settings_state());
                simulation.update_camera(camera, get_settings_state(), camera_pan_enabled);

                BeginMode3D(camera);
                skybox.draw();
                simulation.draw_world(camera, get_settings_state());

                if (adding_object) {
                    Ray ray = GetScreenToWorldRay(GetMousePosition(), camera);
                    auto pos = ray_y_plane_intersection(ray, temp_state->grid.flat_grid_y);
                    if (!pos.has_value())
                        pos = ray.position + ray.direction * 15;
                    adding_object->position = *pos;
                    adding_object->draw(get_settings_state().objects_scale);
                    adding_object->draw_trail();
                }

                if (simulation.current_selected_object != -1) {
                    // drawing the velocity vector of the current object
                    const auto& selected = simulation.get_object(simulation.current_selected_object);

                    auto start = calculate_starting_point_of_velocity_line(selected, get_settings_state());
                    auto end = selected.position + velocity_to_arrow_offset(selected.velocity, selected.radius * get_settings_state().objects_scale, get_settings_state().velocity_arrow_scale);

                    auto cone_height = selected.radius * get_settings_state().objects_scale / 20;

                    velocity_cone = draw_3d_arrow(start, end, cone_height);

                    if (changing_velocity_axis_info) {
                        // Draw a guide line through the arrow's tip along the
                        // locked axis so it's clear which axis the velocity
                        // drag is currently constrained to. Sized relative to
                        // the camera's distance from the tip (just far enough
                        // to fill the view) rather than a large fixed
                        // constant -- DrawLine3D takes float endpoints, and
                        // adding a huge offset to `end` before the
                        // double->float cast destroys the precision of the
                        // (much smaller) tip position, so the line visibly
                        // wobbles as it rounds to different nearby floats
                        // each frame. Same class of issue
                        // camera_offset_from_selected works around for the
                        // mouse-ray math above.
                        Vector3Double axis_dir = axis_unit_vector(changing_velocity_axis_info->axis);
                        double guide_line_extent = Vector3Double{camera.position}.distance(end) * 4.0 + 1.0;
                        DrawLine3D(
                            (end - axis_dir * guide_line_extent).to_vector3(),
                            (end + axis_dir * guide_line_extent).to_vector3(),
                            SKYBLUE
                        );
                    }
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

                get_settings_state().paused = !PlaybackControls(!get_settings_state().paused, get_settings_state().delta_time);
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
                    adding_object->draw_outline(get_settings_state().objects_scale, camera);
                    adding_object->draw_label(camera);
                    DrawText("Press Enter to confirm", 10, 10, 24, WHITE);
                }

                auto selection = simulation.draw_object_selection_ui(camera, settings);
                if (selection && !adding_object && !changing_velocity_of_obj)
                    simulation.select_object(*selection, settings);
                
                if (!imgui_wants_io()) {
                    if (!IsCursorHidden() && IsCursorOnScreen() && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !adding_object) {
                        simulation.select_object({-1, 0}, settings);
                        right_click_location = GetMousePosition();
                        ImGui::OpenPopup("Right Click Menu");
                    }
                    if (velocity_cone &&
                        (is_object_in_camera(velocity_cone->base.to_vector3(), camera) ||
                        is_object_in_camera(velocity_cone->tip.to_vector3(), camera)) &&
                        !camera_pan_enabled
                    ) {
                        // velocity cone is visible on screen
                        auto mouse_pos = GetMousePosition();
                        auto cone_tip_pos = GetWorldToScreen(velocity_cone->tip.to_vector3(), camera);
                        auto cone_base_pos = GetWorldToScreen(velocity_cone->base.to_vector3(), camera);

                        float cone_screen_height = Vector2Length(cone_tip_pos - cone_base_pos);

                        bool is_mouse_on_cone =
                            Vector2Distance(cone_tip_pos, mouse_pos) < 9 ||
                            Vector2Distance(cone_base_pos, mouse_pos) < 9;

                        if (changing_velocity_of_obj || (IsKeyPressed(KEY_V))) {
                            bool should_stop = change_velocity_using_cone(velocity_cone, camera, changing_velocity_of_obj, simulation, settings, temp_state, changing_velocity_axis_info);
                            if (should_stop)
                                stop_changing_velocity();
                        }

                        if (changing_velocity_of_obj) {
                            std::optional<Axis> changing_velocity_axis = get_axis_from_key((KeyboardKey)GetKeyPressed());
                            if (changing_velocity_axis) {
                                temp_state->paused = true;
                                const auto& selected = simulation.get_object(simulation.current_selected_object);

                                Vector3Double original_pos = selected.position;
                                Vector2 original_pos_screen = GetWorldToScreen(original_pos.to_vector3(), camera);

                                Vector3Double test_pos = original_pos + axis_unit_vector(*changing_velocity_axis);
                                Vector2 test_pos_screen = GetWorldToScreen(test_pos.to_vector3(), camera);

                                // Raw (non-normalized) screen delta for a 1 world-unit
                                // step along the axis -- its length is how many screen
                                // pixels correspond to 1 world unit at the current zoom.
                                // Dividing by its squared length (instead of just
                                // normalizing) bakes that scale into the projection
                                // below, so dragging tracks the mouse consistently
                                // whether zoomed in or far out.
                                Vector2 screen_delta = test_pos_screen - original_pos_screen;
                                float pixels_per_unit_sqr = Vector2LengthSqr(screen_delta);
                                Vector2 speed = pixels_per_unit_sqr > 1e-6f
                                    ? screen_delta / pixels_per_unit_sqr
                                    : Vector2Zero();

                                changing_velocity_axis_info = AxisChangeInfo {
                                    .axis = *changing_velocity_axis,
                                    .horizontal_speed = speed.x,
                                    .vertical_speed = speed.y,
                                    .original_mouse_pos = GetMousePosition(),
                                    .original_pos = velocity_to_arrow_offset(selected.velocity, selected.radius * settings.objects_scale, settings.velocity_arrow_scale).to_vector3()
                                };
                            }
                        }
                    }

                    if (IsKeyPressed(KEY_P))
                        demo_shown = !demo_shown;
                    if (IsKeyPressed(KEY_O))
                        metrics_shown = !metrics_shown;
                }
                if ((!velocity_cone || simulation.current_selected_object == -1) && changing_velocity_of_obj)
                    stop_changing_velocity();

                if (simulation.current_selected_object != -1)
                    ObjectEditor(simulation.current_selected_object, simulation.get_object(simulation.current_selected_object), simulation.get_objects());

                if (right_click_location) {
                    auto action = RightClickMenu(*right_click_location);
                    if (action) {
                        std::cout << (int)*action << '\n';
                        handle_right_click_menu_action(action, camera, simulation, adding_object, settings, temp_state);
                        right_click_location = std::nullopt;
                    }
                }

                // end ImGui Content
                rlImGuiEnd();
                break;
        }

        if (!imgui_wants_io()) {
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

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && changing_velocity_of_obj)
                stop_changing_velocity();

            if (adding_object) {
                float* to_change = nullptr;
                if (adding_object)
                    to_change = &temp_state->grid.flat_grid_y;

                float multiplier = 6;
                if (IsKeyDown(KEY_LEFT_SHIFT))
                    multiplier = 2;

                if (IsKeyDown(KEY_UP))
                    *to_change += multiplier * GetFrameTime();
                else if (IsKeyDown(KEY_DOWN))
                    *to_change -= multiplier * GetFrameTime();
            }

            if (IsKeyPressed(KEY_K))
                settings.paused = !settings.paused;

            if (adding_object && IsKeyPressed(KEY_ENTER))
                stop_adding_object();
        }

        EndDrawing();
    }

    // De-Initialization
    rlImGuiShutdown();
    CloseWindow();        // Close window and OpenGL context

    return 0;
}
