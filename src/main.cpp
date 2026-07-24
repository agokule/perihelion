#include "Vector3Double.hpp"
#include "raylib.h"
#include "extras/IconsFontAwesome6.h"
#include "adjust.h"

#include "imgui.h"
#include "rlImGui.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <reasings.h>
#include <raymath.h>
#include <string>
#include "Object.hpp"
#include "Preset.hpp"
#include "utils.hpp"

static Preset scene;
constexpr double gravitational_constant = 6.6743e-11;


void load_preset(const Preset& new_preset) {
    scene = new_preset;

    for (Object& obj : scene.objects)
        obj.load_model();
}


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
    camera.position = (Vector3){ 10.0f, 10.0f, 10.0f }; // Camera position
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };      // Camera looking at point
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                                // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;             // Camera projection type

    ADJUST_CONST_STRING(text, "Perihelion");
    ADJUST_CONST_BOOL(adjust_live, false);
    ADJUST_VAR_INT(current_selected_object, -1);
    ADJUST_CONST_FLOAT(selected_sensitivity, 0.005f);
    ADJUST_CONST_FLOAT(objects_scale, 5.0f);
    bool camera_pan_enabled = true;
    float alpha = 0.0f;          // Horizontal rotation angle (Yaw)
    float beta = 0.5f;           // Vertical rotation angle (Pitch)
    float distance = 1.0f;
    float wheel_sensitivity = 0.1f;


    float camera_position_lerp = -1.0f;
    float camera_target_lerp = camera_position_lerp;
    size_t camera_lerp_start = 0;

    size_t frame_counter = 0;

    DisableCursor();

    SetExitKey(KEY_NULL);

    load_preset(presets.at(0));

    // Main game loop
    while (!WindowShouldClose()) {
        camera_pan_enabled = IsCursorHidden();
        if (camera_pan_enabled)
            UpdateCamera(&camera, current_selected_object == -1 ? CAMERA_FREE : CAMERA_CUSTOM);

        // simulate gravity
        for (int i = 0; i < substeps_per_frame; i++) {
            for (Object& obj: scene.objects) {
                for (Object& obj2 : scene.objects) {
                    if (obj2.name == obj.name)
                        continue;

                    double distance = convert_light_seconds_to_meters(obj.position.distance(obj2.position));
                    double distanceSqr = distance * distance;
                    double gravity_acceleration = gravitational_constant * obj2.mass / distanceSqr;

                    Vector3Double direction_of_acceleration = (obj2.position - obj.position).normalize();

                    Vector3Double acceleration = direction_of_acceleration * gravity_acceleration;
                    obj.accelerate(convert_meters_to_light_seconds(acceleration), delta_time);
                }
            }
            for (Object& obj : scene.objects) {
                obj.update_pos(delta_time);
            }
        }

        BeginDrawing();
        ClearBackground(DARKGRAY);

        int idx = 0;
        for (const Object& obj : scene.objects) {
            if (current_selected_object == idx) {
                Vector2 mouseDelta = camera_pan_enabled ? GetMouseDelta() : Vector2{0, 0};
                alpha -= mouseDelta.x * selected_sensitivity;
                beta  += mouseDelta.y * selected_sensitivity;

                // Clamp vertical angle to prevent flipping upside down
                if (beta >  1.4f) beta =  1.4f;
                if (beta < -1.4f) beta = -1.4f;

                // 3. Zoom with mouse scroll wheel
                auto mouse_wheel_move = camera_pan_enabled ? GetMouseWheelMove() : 0;
                distance -= mouse_wheel_move * wheel_sensitivity;
                if (distance < obj.radius) distance = obj.radius; // Prevent going inside the object

                // 4. Calculate camera position using spherical trigonometry
                Vector3 camera_position {};
                camera_position.x = obj.position.x + distance * cosf(beta) * sinf(alpha);
                camera_position.y = obj.position.y + distance * sinf(beta);
                camera_position.z = obj.position.z + distance * cosf(beta) * cosf(alpha);

                if (camera_position_lerp == -1.0f) {
                    camera.position = camera_position;
                    camera.target = obj.position.to_vector3();
                } else {
                    camera_target_lerp = camera_target_lerp != 1
                        ? EaseSineIn(frame_counter - camera_lerp_start, 0.0f, 1.0f, ImGui::GetIO().Framerate * 2)
                        : 1;
                    if (frame_counter - camera_lerp_start >= ImGui::GetIO().Framerate) {
                        camera_target_lerp = 1;
                        camera_lerp_start = frame_counter;
                    }
                    if (camera_target_lerp == 1.0f)
                        camera_position_lerp = std::clamp(camera_position_lerp + 0.01f, 0.01f, 1.0f);
                    camera.position = Vector3Lerp(camera.position, camera_position, camera_position_lerp);
                    camera.target = Vector3Lerp(camera.target, obj.position.to_vector3(), camera_target_lerp);
                    if (camera_position_lerp == 1.0f)
                        camera_target_lerp = camera_position_lerp = -1.0f;
                }
            }
            idx++;
        }

        BeginMode3D(camera);

        DrawGrid(1500, 10);

        for (const Object& obj : scene.objects) {
            obj.draw_trail();
            obj.draw(objects_scale);
        }

        EndMode3D();

        // draw 2d ui
        DrawFPS(10, 10);
        // start ImGui Conent
        rlImGuiBegin();

        ImGui::Begin("Select an Object");
        idx = 0;
        for (const Object& obj : scene.objects) {
            obj.draw_label(camera);
            if (obj.position.distance(Vector3Double{camera.position}) > 50.0f)
                obj.draw_outline(objects_scale, camera);

            if (ImGui::RadioButton(obj.name.c_str(), &current_selected_object, idx)) {
                current_selected_object = idx;
                distance = std::clamp(obj.radius * 5 * objects_scale, 0.1, 1e100);
                camera_target_lerp = camera_position_lerp = 0.0f;
                camera_lerp_start = frame_counter;
            }
            idx++;
        }
        if (ImGui::RadioButton("None", &current_selected_object, -1))
            current_selected_object = -1;

        ImGui::End();

        // end ImGui Content
        rlImGuiEnd();

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
