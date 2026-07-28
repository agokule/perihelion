#include "SimulationScreen.hpp"

#include <algorithm>
#include <cmath>
#include <imgui.h>
#include <raymath.h>
#include <reasings.h>

#include "utils.hpp"

namespace {
    constexpr double gravitational_constant = 6.6743e-11;
}

SimulationScreen::SimulationScreen(const Preset& initial_preset) {
    load_preset(initial_preset);
}

void SimulationScreen::load_preset(const Preset& new_preset) {
    scene = new_preset;

    for (Object& obj : scene.objects)
        obj.load_model();
}

void SimulationScreen::simulate_physics(const SimulationSettings& settings) {
    for (int i = 0; i < settings.substeps_per_frame; i++) {
        for (Object& obj : scene.objects) {
            for (Object& obj2 : scene.objects) {
                if (obj2.name == obj.name)
                    continue;

                double distance = convert_light_seconds_to_meters(obj.position.distance(obj2.position));
                double distanceSqr = distance * distance;
                double gravity_acceleration = gravitational_constant * obj2.mass / distanceSqr;

                Vector3Double direction_of_acceleration = (obj2.position - obj.position).normalize();

                Vector3Double acceleration = direction_of_acceleration * gravity_acceleration;
                obj.accelerate(convert_meters_to_light_seconds(acceleration), settings.delta_time);
            }
        }
        for (Object& obj : scene.objects) {
            obj.update_pos(settings.delta_time);
        }
    }
}

void SimulationScreen::select_object(int idx, double radius, const SimulationSettings& settings,
                                     std::size_t frame_counter) {
    current_selected_object = idx;
    distance = std::clamp(radius * 5 * settings.objects_scale, 0.1, 1e100);
    camera_target_lerp = camera_position_lerp = 0.0f;
    camera_lerp_start = frame_counter;
}

void SimulationScreen::update_camera(Camera3D& camera, const SimulationSettings& settings,
                                      bool camera_pan_enabled, std::size_t frame_counter) {
    int idx = -1;
    for (const Object& obj : scene.objects) {
        idx++;
        if (current_selected_object != idx)
            continue;
        Vector2 mouseDelta = camera_pan_enabled ? GetMouseDelta() : Vector2{0, 0};
        alpha -= mouseDelta.x * settings.selected_sensitivity;
        beta  += mouseDelta.y * settings.selected_sensitivity;

        // Clamp vertical angle to prevent flipping upside down
        if (beta >  1.4f) beta =  1.4f;
        if (beta < -1.4f) beta = -1.4f;

        // Zoom with mouse scroll wheel
        auto mouse_wheel_move = camera_pan_enabled ? GetMouseWheelMove() : 0;
        distance -= mouse_wheel_move * wheel_sensitivity;
        if (distance < obj.radius) distance = obj.radius; // Prevent going inside the object

        // Calculate camera position using spherical trigonometry
        Vector3 camera_position{};
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
}

void SimulationScreen::draw_world(const Camera3D& camera, const SimulationSettings& settings) const {
    BeginMode3D(camera);

    DrawGrid(1500, 10);

    for (const Object& obj : scene.objects) {
        obj.draw_trail();
        obj.draw(settings.objects_scale);
    }

    EndMode3D();
}

std::optional<ObjectSelection> SimulationScreen::draw_object_selection_ui(Camera3D& camera,
                                                                           const SimulationSettings& settings) {
    std::optional<ObjectSelection> selection;

    ImGui::Begin("Select an Object");

    int idx = 0;
    for (const Object& obj : scene.objects) {
        obj.draw_label(camera);

        if (ImGui::RadioButton(obj.name.c_str(), &current_selected_object, idx))
            selection = ObjectSelection{idx, obj.radius};

        if (obj.draw_outline(settings.objects_scale, camera))
            selection = ObjectSelection{idx, obj.radius};

        idx++;
    }
    if (ImGui::RadioButton("None", &current_selected_object, -1))
        current_selected_object = -1;

    ImGui::End();

    return selection;
}
