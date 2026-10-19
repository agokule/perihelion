#include "FontIcons.hpp"
#include "Preset.hpp"
#include "physics/Object.hpp"
#include "Vector3Double.hpp"
#include "physics/settings.hpp"
#include "raylib.h"
#include "imgui.h"
#include "raymath.h"
#include "ui/GridTypeEdit.hpp"
#include "ui/PlaybackControls.hpp"
#include "ui/SettingsEdit.hpp"
#include "ui/imgui_ui_utils.hpp"
#include "viewport/camera.hpp"

// disable rlImGui's font awesome
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

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>

// disable a bunch of annoying browser keyboard shortcuts
EM_JS(void, disable_browser_shortcuts, (), {
    document.addEventListener('keydown', function(e) {
        // Block Ctrl+S, Ctrl+W, Ctrl+D, Ctrl+E
        if ((e.ctrlKey && (e.key === 's' || e.key === 'w' || e.key === 'd' || e.key === 'e'))) {
            e.preventDefault();
        }
    }, { passive: false });
});

// just in case disabling Ctrl+w is not allowed in the specific browser
EM_JS(void, warn_before_closing_browser_tab, (), {
    window.addEventListener('beforeunload', function (e) {
        e.preventDefault();
        e.returnValue = ''; // Chrome requires returnValue to show prompt
    });
})

#endif


struct AxisChangeInfo {
    Axis axis;

    float horizontal_speed;
    float vertical_speed;

    Vector2 original_mouse_pos;

    // the object-relative offset the arrow tip had when axis lock was engaged
    Vector3 original_pos;
};

struct AppData {
    Camera3D camera = { 0 };
    bool camera_pan_enabled = true;
    AppState app_state = AppState::Simulation;
    SimulationScreen simulation {presets.at(0)};
    Skybox skybox {"./assets/images/myersalex216-space-2638158.jpg"};

    std::optional<Vector2> right_click_location = std::nullopt;
    bool right_click_menu_should_open = false;
    std::optional<Vector2> left_click_pos = std::nullopt;
    Vector2 mouse_delta = {0, 0};
    float mouse_wheel_move = 0.0f;
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

    SimulationSettings& get_settings_state() {
        return temp_state ? *temp_state : settings;
    }

    void stop_changing_velocity() {
        changing_velocity_of_obj = false;
        temp_state = std::nullopt;
        changing_velocity_axis_info = std::nullopt;
    }

    // note that this can re-allocate memory, so it's best to call this sparingly since Object& are common in the codebase...
    void stop_adding_object() {
        simulation.add_object(*adding_object);
        temp_state = std::nullopt;
        adding_object = std::nullopt;
    }
};

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

bool imgui_wants_io() {
    return ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantCaptureMouse;
}

Vector3Double calculate_starting_point_of_velocity_line(const Object& selected, const SimulationSettings& settings) {
    auto velocity_radius_length = selected.velocity.normalize() * selected.radius * settings.objects_scale;
    auto start = selected.position + velocity_radius_length;
    return start;
}

void handle_right_click_menu_action(AppData& app, RightClickActionSelected action) {
    switch (action) {
        case RightClickActionSelected::CreateObject:
        {
            Ray ray = GetScreenToWorldRay({0, 0}, app.camera);
            Vector3Double pos = ray.position + ray.direction * 15;
            app.adding_object = Object {
                ObjectType::Planet,
                std::format("New Object ({})", app.simulation.num_objects()),
                1e20,
                0.005,
                pos,
                Vector3Zero(),
                WHITE
            };
            app.temp_state = app.settings;
            app.temp_state->paused = true;
            app.temp_state->grid.type = GridType::Flat;
            app.temp_state->grid.flat_grid_y = 0.0f;
            app.temp_state->grid.flat_grid_slices = 100;
            app.temp_state->grid.spacing_between_slices = 1;
            app.temp_state->grid.grid_color = GREEN;
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

bool change_velocity_using_cone(AppData& app) {
    app.changing_velocity_of_obj = true;
    auto& selected = app.simulation.get_object(app.simulation.current_selected_object);

    if (!app.temp_state) {
        app.temp_state = app.settings;
        app.temp_state->grid = {
            .flat_grid_slices = 100,
            .flat_grid_y = static_cast<float>(selected.position.y + selected.velocity.y),
            .type = GridType::Flat,
            .spacing_between_slices = 1,
            .grid_color = ORANGE,
        };
    }

    std::optional<Vector3> pos = std::nullopt;
    auto mouse_pos = GetMousePosition();

    if (!app.changing_velocity_axis_info) {
        Camera3D local_camera = app.camera;
        local_camera.position = app.simulation.camera_offset_from_selected().to_vector3();
        local_camera.target = Vector3Zero();

        Ray ray = GetScreenToWorldRay(mouse_pos, local_camera);
        pos = ray_y_plane_intersection(ray, 0.0f);
    } else {
        const auto& axis_lock = *app.changing_velocity_axis_info;
        pos = axis_lock.original_pos;

        float* to_edit = nullptr;
        const float* original = nullptr;
        switch (axis_lock.axis) {
            case Axis::X:
                to_edit = &pos->x;
                original = &axis_lock.original_pos.x;
                break;
            case Axis::Y:
                to_edit = &pos->y;
                original = &axis_lock.original_pos.y;
                break;
            case Axis::Z:
                to_edit = &pos->z;
                original = &axis_lock.original_pos.z;
                break;
        }

        auto mouse_change = mouse_pos - axis_lock.original_mouse_pos;
        float change_along_axis = mouse_change.x * axis_lock.horizontal_speed + mouse_change.y * axis_lock.vertical_speed;
        *to_edit = *original + change_along_axis;
    }

    if (pos) {
        double surface_radius = selected.radius * app.settings.objects_scale;
        auto velocity = arrow_offset_to_velocity(Vector3Double{*pos}, surface_radius, app.settings.velocity_arrow_scale);
        if (velocity)
            selected.velocity = *velocity;
        return false;
    } else
        return true;
}

void handle_events(AppData& app) {
    app.camera_pan_enabled = is_cursor_locked();
    if (app.camera_pan_enabled)
        update_camera(&app.camera, app.simulation.current_selected_object == -1 ? CAMERA_FREE : CAMERA_CUSTOM);

    // capturing mouse input here because Raylib does not support reading
    // it during drawing a frame on emscripten so that the draw_simulation() function can read it
    // see https://github.com/raysan5/raylib/wiki/Working-for-Web-(HTML5)#faq--common-issues

    app.mouse_delta = get_mouse_delta();
    app.mouse_wheel_move = GetMouseWheelMove();
    app.left_click_pos = std::nullopt;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        app.left_click_pos = GetMousePosition();

    if (app.app_state != AppState::Simulation)
        return;

    if (app.adding_object && app.camera_pan_enabled)
        DisableCursor();

    const bool imgui_has_focus = imgui_wants_io();

    if (!imgui_has_focus) {
        if (!is_cursor_locked() && IsCursorOnScreen() && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !app.adding_object) {
            app.simulation.select_object({-1, 0}, app.settings);
            app.right_click_location = GetMousePosition();
            app.right_click_menu_should_open = true;
        }

        if (app.velocity_cone && !app.camera_pan_enabled) {
            if (app.changing_velocity_of_obj || (IsKeyPressed(KEY_V))) {
                bool should_stop = change_velocity_using_cone(app);
                if (should_stop)
                    app.stop_changing_velocity();
            }

            if (app.changing_velocity_of_obj) {
                std::optional<Axis> changing_velocity_axis = get_axis_from_key(
                        (KeyboardKey)GetKeyPressed() // note that this function deletes keys from a queue, calling it multiple times is not a good idea
                );
                if (changing_velocity_axis) {
                    app.temp_state->paused = true;
                    const auto& selected = app.simulation.get_object(app.simulation.current_selected_object);

                    Vector3Double original_pos = selected.position;
                    Vector2 original_pos_screen = GetWorldToScreen(original_pos.to_vector3(), app.camera);

                    Vector3Double test_pos = original_pos + axis_unit_vector(*changing_velocity_axis);
                    Vector2 test_pos_screen = GetWorldToScreen(test_pos.to_vector3(), app.camera);

                    // calculate how much the velocity arrow should move if the mouse moves one pixel
                    // TODO: this is not accurate since the mouse movement should not be multiplied by a constant factor
                    //       fix it.
                    Vector2 screen_delta = test_pos_screen - original_pos_screen;
                    float pixels_per_unit_sqr = Vector2LengthSqr(screen_delta);
                    Vector2 speed = pixels_per_unit_sqr > 1e-6f
                        ? screen_delta / pixels_per_unit_sqr
                        : Vector2Zero();

                    app.changing_velocity_axis_info = AxisChangeInfo {
                        .axis = *changing_velocity_axis,
                        .horizontal_speed = speed.x,
                        .vertical_speed = speed.y,
                        .original_mouse_pos = GetMousePosition(),
                        .original_pos = velocity_to_arrow_offset(selected.velocity, selected.radius * app.settings.objects_scale, app.settings.velocity_arrow_scale).to_vector3()
                    };
                }
            }
        }

        if (IsKeyPressed(KEY_P))
            app.demo_shown = !app.demo_shown;
        if (IsKeyPressed(KEY_O))
            app.metrics_shown = !app.metrics_shown;
    }

    // this needs to be here, in between the two if statements otherwise things break
    if ((!app.velocity_cone || app.simulation.current_selected_object == -1) && app.changing_velocity_of_obj)
        app.stop_changing_velocity();

    if (!imgui_has_focus) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !app.changing_velocity_of_obj) {
            DisableCursor();
            app.camera_pan_enabled = true;
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            EnableCursor();
            app.camera_pan_enabled = false;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && app.changing_velocity_of_obj)
            app.stop_changing_velocity();

        // move the placement plane itself, so an object can be placed off of y=0
        if (app.adding_object) {
            float multiplier = 6;
            if (IsKeyDown(KEY_LEFT_SHIFT))
                multiplier = 2;

            if (IsKeyDown(KEY_UP))
                app.temp_state->grid.flat_grid_y += multiplier * GetFrameTime();
            else if (IsKeyDown(KEY_DOWN))
                app.temp_state->grid.flat_grid_y -= multiplier * GetFrameTime();
        }

        if (IsKeyPressed(KEY_K))
            app.settings.paused = !app.settings.paused;

        // should be last since it can re-allocate memory which makes it more volatile of function
        if (app.adding_object && IsKeyPressed(KEY_ENTER))
            app.stop_adding_object();
    }
}

void draw_simulation(AppData& app) {
    app.simulation.simulate_physics(app.get_settings_state());
    app.simulation.update_camera(app.camera, app.get_settings_state(), app.camera_pan_enabled, app.mouse_delta, app.mouse_wheel_move);

    BeginMode3D(app.camera);
    app.skybox.draw();
#if defined(__EMSCRIPTEN__)
    BeginShaderMode(get_highp_default_shader());
#endif
    app.simulation.draw_world(app.camera, app.get_settings_state());

    if (app.adding_object) {
        Ray ray = GetScreenToWorldRay(GetMousePosition(), app.camera);
        auto pos = ray_y_plane_intersection(ray, app.temp_state->grid.flat_grid_y);
        if (!pos.has_value())
            pos = ray.position + ray.direction * 15;
        app.adding_object->position = *pos;
        app.adding_object->draw(app.get_settings_state().objects_scale);
        app.adding_object->draw_trail();
    }

    if (app.simulation.current_selected_object != -1) {
        // drawing the velocity vector of the current object
        const auto& selected = app.simulation.get_object(app.simulation.current_selected_object);

        auto start = calculate_starting_point_of_velocity_line(selected, app.get_settings_state());
        auto end = selected.position + velocity_to_arrow_offset(selected.velocity, selected.radius * app.get_settings_state().objects_scale, app.get_settings_state().velocity_arrow_scale);

        auto cone_height = selected.radius * app.get_settings_state().objects_scale / 20;

        app.velocity_cone = draw_3d_arrow(start, end, cone_height);

        if (app.changing_velocity_axis_info) {
            // drawing a guide line along the axis
            Vector3Double axis_dir = axis_unit_vector(app.changing_velocity_axis_info->axis);
            double guide_line_extent = Vector3Double{app.camera.position}.distance(end) * 4.0 + 1.0;
            DrawLine3D(
                (end - axis_dir * guide_line_extent).to_vector3(),
                (end + axis_dir * guide_line_extent).to_vector3(),
                SKYBLUE
            );
        }
    }

#if defined(__EMSCRIPTEN__)
    EndShaderMode();
#endif
    EndMode3D();

    // draw 2d ui
    DrawFPS(10, 10);
    // start ImGui Conent
    rlImGuiBegin();

    if (app.demo_shown)
        ImGui::ShowDemoWindow(&app.demo_shown);
    if (app.metrics_shown)
        ImGui::ShowMetricsWindow(&app.metrics_shown);

    app.get_settings_state().paused = !PlaybackControls(!app.get_settings_state().paused, app.get_settings_state().delta_time);
    GridTypeEdit(app.settings.grid);
    {
        ImGuiSetNextWindowPos({ .top = 10, .right = 10 });
        RAIIStyleVar s {ImGuiStyleVar_WindowPadding, {4.0f, 4.0f}};
        RAIIStyleVar s2 {ImGuiStyleVar_WindowMinSize, {0, 0}};
        RAIIWindow win {"Open Settings", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings};
        if (ImGui::Button(NF_COD_SETTINGS_GEAR))
            app.settings_window_shown = !app.settings_window_shown;
    }
    if (app.settings_window_shown)
        SettingsEdit(app.settings, app.settings_window_shown);

    if (app.adding_object) {
        app.adding_object->draw_outline(app.get_settings_state().objects_scale, app.camera, app.left_click_pos);
        app.adding_object->draw_label(app.camera);
        DrawText("Press Enter to confirm", 10, 10, 24, WHITE);
    }

    auto selection = app.simulation.draw_object_selection_ui(app.camera, app.settings, app.left_click_pos);
    if (selection && !app.adding_object && !app.changing_velocity_of_obj)
        app.simulation.select_object(*selection, app.settings);

    if (app.simulation.current_selected_object != -1)
        ObjectEditor(app.simulation.current_selected_object, app.simulation.get_object(app.simulation.current_selected_object), app.simulation.get_objects());

    if (app.right_click_location) {
        if (app.right_click_menu_should_open) {
            ImGui::OpenPopup("Right Click Menu");
            app.right_click_menu_should_open = false;
        }

        auto action = RightClickMenu(*app.right_click_location);
        if (action) {
            std::cout << (int)*action << '\n';
            handle_right_click_menu_action(app, *action);
            app.right_click_location = std::nullopt;
        }
    }

    // end ImGui Content
    rlImGuiEnd();
}

// One frame: input handling, then internal updates + drawing.
void update_and_draw(AppData& app) {
    handle_events(app);

    BeginDrawing();
    ClearBackground(DARKGRAY);

    switch (app.app_state) {
        case AppState::PresetSelection:
            // TODO: create a preset selection screen and draw it
            break;

        case AppState::Simulation:
            draw_simulation(app);
            break;
    }

    EndDrawing();
}

AppData* initialize() {
    // Initialization
    int screenWidth = 1280;
    int screenHeight = 800;

#ifdef __EMSCRIPTEN__
    disable_browser_shortcuts();
    warn_before_closing_browser_tab();
#endif

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "Perihelion");
#if !defined(__EMSCRIPTEN__)
    SetTargetFPS(144);
#endif
    rlImGuiSetup(true);
    IMGUI_CHECKVERSION();

    AppData* app_data = new AppData();

    // Define the camera to look into our 3d world
    app_data->camera.position = (Vector3){ 50.0f, 50.0f, 50.0f }; // Camera position
    app_data->camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    app_data->camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    app_data->camera.fovy = 45.0f;                                // Camera field-of-view Y
    app_data->camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type

    SetExitKey(KEY_NULL);

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

    return app_data;
}

void quit(AppData* app_data) {
    delete app_data;

    rlImGuiShutdown();
    CloseWindow();        // Close window and OpenGL context
}

#if defined(__EMSCRIPTEN__)
void emscripten_frame(void* arg) {
    update_and_draw(*static_cast<AppData*>(arg));
}
#endif

int main() {
    AppData* app_data = initialize();

#if defined(__EMSCRIPTEN__)
    emscripten_set_main_loop_arg(emscripten_frame, app_data, 0, 1);
#else
    // Main game loop
    while (!WindowShouldClose())
        update_and_draw(*app_data);

    quit(app_data);
#endif

    return 0;
}
