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

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif


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

// Every piece of state the frame callback needs. It has to outlive each frame
// (under Emscripten the browser calls back into update_and_draw one frame at a
// time and main() has already returned), so it lives on the heap and is
// threaded through as a single argument instead of being locals in main().
struct AppData {
    Camera3D camera = { 0 };
    bool camera_pan_enabled = true;
    AppState app_state = AppState::Simulation;
    SimulationScreen simulation {presets.at(0)};
    Skybox skybox {"./assets/images/myersalex216-space-2638158.jpg"};

    std::optional<Vector2> right_click_location = std::nullopt;
    // set by handle_events, consumed by draw_simulation: input handling runs
    // before the frame is drawn (see update_and_draw) but ImGui::OpenPopup can
    // only be called from inside the ImGui frame, so the right click is
    // recorded here and the popup is opened during the draw
    bool right_click_menu_should_open = false;
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

    // the settings a transient action (placing an object, dragging a velocity
    // arrow) is overriding right now, or the persisted ones if no such action
    // is in progress
    SimulationSettings& get_settings_state() {
        return temp_state ? *temp_state : settings;
    }

    void stop_changing_velocity() {
        changing_velocity_of_obj = false;
        temp_state = std::nullopt;
        changing_velocity_axis_info = std::nullopt;
    }

    // NOTE: add_object can reallocate the object vector, invalidating every
    // Object& taken from the simulation earlier in the frame -- only call this
    // once those references are dead.
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
        // using camera_offset_from_selected instead of the real camera
        // because the real camera loses decimal points of precision when
        // far away from the origin (see camera_offset_from_selected's comment).
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
        // horizontal_speed/vertical_speed already carry the pixels-per-world-unit
        // scale (see capture site below), so this dot product yields world
        // units directly -- no separate sensitivity constant needed.
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

// All input handling, run before anything is drawn this frame.
//
// The ordering matters on the web: raylib does not reliably report
// edge-triggered input (IsKeyPressed, IsMouseButtonPressed) that is read after
// the frame has been drawn, so every such check has to happen here, ahead of
// the draw. Level-triggered state (IsKeyDown, the mouse position) survives
// either way, which is why ImGui's own input keeps working from inside the
// draw while raw raylib checks placed there silently do nothing.
// See https://github.com/raysan5/raylib/wiki/Working-for-Web-(HTML5).
//
// Two consequences of running ahead of the draw:
//   - imgui_wants_io() reports what ImGui decided during the *previous*
//     frame's NewFrame. That is the value ImGui intends callers to poll input
//     against, so this is fine, just one frame behind.
//   - velocity_cone is last frame's. It is only used as an "is there an arrow
//     on screen" gate, so a frame of lag costs nothing.
//
// The order of the branches in here is load-bearing; see the comments on the
// individual ones.
void handle_events(AppData& app) {
    app.camera_pan_enabled = IsCursorHidden();
    if (app.camera_pan_enabled)
        update_camera(&app.camera, app.simulation.current_selected_object == -1 ? CAMERA_FREE : CAMERA_CUSTOM);

    if (app.app_state != AppState::Simulation)
        return;

    if (app.adding_object && app.camera_pan_enabled)
        DisableCursor();

    // one read per frame so every branch below agrees on who owns the input
    const bool imgui_has_focus = imgui_wants_io();

    if (!imgui_has_focus) {
        // only records the click; draw_simulation opens the popup from inside
        // the ImGui frame, which is the only place OpenPopup can be called
        if (!IsCursorHidden() && IsCursorOnScreen() && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !app.adding_object) {
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
                // NOTE: GetKeyPressed pops from raylib's keypress queue rather
                // than querying state like IsKeyPressed -- calling it on more
                // frames, or from more than one place, eats keys other
                // handlers are waiting on.
                std::optional<Axis> changing_velocity_axis = get_axis_from_key((KeyboardKey)GetKeyPressed());
                if (changing_velocity_axis) {
                    app.temp_state->paused = true;
                    const auto& selected = app.simulation.get_object(app.simulation.current_selected_object);

                    Vector3Double original_pos = selected.position;
                    Vector2 original_pos_screen = GetWorldToScreen(original_pos.to_vector3(), app.camera);

                    Vector3Double test_pos = original_pos + axis_unit_vector(*changing_velocity_axis);
                    Vector2 test_pos_screen = GetWorldToScreen(test_pos.to_vector3(), app.camera);

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

    // Deliberately outside the focus guard -- a drag whose arrow went away
    // (object deselected, cone gone) has to be cancelled even while a widget
    // has focus. It also has to stay ahead of the click handling below: it
    // clears changing_velocity_of_obj, which is what decides whether a click
    // on this frame ends the drag or re-grabs the cursor for camera panning.
    if ((!app.velocity_cone || app.simulation.current_selected_object == -1) && app.changing_velocity_of_obj)
        app.stop_changing_velocity();

    if (!imgui_has_focus) {
        // The two left-click branches are mutually exclusive on
        // changing_velocity_of_obj, and both have to stay *after* the velocity
        // handling above -- that is what sets the flag (on a V press), so the
        // click that ends a drag gets consumed by stop_changing_velocity
        // instead of also re-locking the cursor.
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

        // move the placement plane itself, so an object can be placed off y=0
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

        // Last, because stop_adding_object reallocates the object vector and
        // invalidates any Object& still live from earlier in the frame.
        if (app.adding_object && IsKeyPressed(KEY_ENTER))
            app.stop_adding_object();
    }
}

// Steps the simulation and draws one frame of it. Assumes it is called between
// BeginDrawing and EndDrawing, with handle_events already run for this frame.
//
// Nothing in here may use IsKeyPressed/IsMouseButtonPressed: edge-triggered
// raylib input read after the draw has begun is unreliable on the web. Input
// belongs in handle_events.
void draw_simulation(AppData& app) {
    app.simulation.simulate_physics(app.get_settings_state());
    app.simulation.update_camera(app.camera, app.get_settings_state(), app.camera_pan_enabled);

    BeginMode3D(app.camera);
    app.skybox.draw();
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
            Vector3Double axis_dir = axis_unit_vector(app.changing_velocity_axis_info->axis);
            double guide_line_extent = Vector3Double{app.camera.position}.distance(end) * 4.0 + 1.0;
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
        app.adding_object->draw_outline(app.get_settings_state().objects_scale, app.camera);
        app.adding_object->draw_label(app.camera);
        DrawText("Press Enter to confirm", 10, 10, 24, WHITE);
    }

    auto selection = app.simulation.draw_object_selection_ui(app.camera, app.settings);
    if (selection && !app.adding_object && !app.changing_velocity_of_obj)
        app.simulation.select_object(*selection, app.settings);

    if (app.simulation.current_selected_object != -1)
        ObjectEditor(app.simulation.current_selected_object, app.simulation.get_object(app.simulation.current_selected_object), app.simulation.get_objects());

    if (app.right_click_location) {
        // OpenPopup resolves its string id through ImGui's current window, so
        // it can only run inside the ImGui frame -- handle_events, which
        // detects the click, runs before the frame exists and just sets the flag
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
//
// handle_events must stay ahead of the drawing -- see the comment on it.
void update_and_draw(AppData& app) {
    handle_events(app);

    BeginDrawing();
    ClearBackground(DARKGRAY);

    switch (app.app_state) {
        case AppState::PresetSelection:
            // TODO: draw the preset selection screen and transition to
            // AppState::Simulation once the user picks one. Its input handling
            // belongs in handle_events, which currently returns early for
            // every state but Simulation.
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

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "Perihelion");
#if !defined(__EMSCRIPTEN__)
    // on the web the browser paces us via requestAnimationFrame (the 0 passed
    // to emscripten_set_main_loop_arg); a target FPS on top of that just makes
    // EndDrawing burn the leftover time in a wait
    SetTargetFPS(144);
#endif
    rlImGuiSetup(true);
    IMGUI_CHECKVERSION();

    // after InitWindow: constructing the simulation and skybox loads textures,
    // models and shaders, which all need a live GL context
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
    // De-Initialization
    // AppData owns textures, models and shaders (the objects' and the
    // skybox's), so it has to be destroyed while the GL context is still up --
    // i.e. before CloseWindow.
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
    // The browser drives the loop; this call does not return, so there is no
    // teardown path on the web (the tab closing is the teardown).
    emscripten_set_main_loop_arg(emscripten_frame, app_data, 0, 1);
#else
    // Main game loop
    while (!WindowShouldClose())
        update_and_draw(*app_data);

    quit(app_data);
#endif

    return 0;
}
