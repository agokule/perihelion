#include "SimulationScreen.hpp"

#include <algorithm>
#include <cmath>
#include <imgui.h>
#include <iostream>
#include <limits>
#include <raymath.h>
#include <reasings.h>
#include <string>
#include <vector>

#include "Object.hpp"
#include "Vector3Double.hpp"
#include "adjust.h"
#include "raylib.h"
#include "ui/ObjectSelector.hpp"
#include "utils.hpp"

namespace {
    constexpr double gravitational_constant = 6.6743e-11;
    constexpr double speed_of_light = 3e8;
}

SimulationScreen::SimulationScreen(const Preset& initial_preset): scene{initial_preset} {
    load_preset(initial_preset);
}

void SimulationScreen::load_preset(const Preset& new_preset) {
    scene = new_preset;
    // reserve here to avoid a reallocation the first time add_object grows it
    scene.objects.reserve(100);

    for (Object& obj : scene.objects)
        obj.load_model();
}

void SimulationScreen::simulate_physics(const SimulationSettings& settings) {
    if (settings.paused)
        return;

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

void SimulationScreen::select_object(ObjectSelection selection, const SimulationSettings& settings,
                                     std::size_t frame_counter) {
    if (current_selected_object == selection.idx || settings.paused)
        return;

    current_selected_object = selection.idx;
    distance = std::clamp(selection.radius * 5 * settings.objects_scale, 0.1, 1e100);
    camera_target_lerp = camera_position_lerp = 0.0f;
    camera_lerp_start = frame_counter;
}

void SimulationScreen::add_object(const Object& obj) {
    scene.objects.push_back(obj);

    // pushing back may reallocate scene.objects, which copies (rather than
    // moves) the existing Objects and drops their loaded texture/model in
    // the process (see ObjectTextureInfo's copy constructor) — reload them
    for (Object& o : scene.objects)
        o.load_model();
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

void SimulationScreen::draw_spacetime_curvature(const Camera3D& camera, const SimulationSettings& settings) const {
    int slices = 50;
    int spacing_between_slices = 5;
    int grid_dimensions = slices * 2 + 1;
    double radius = slices * spacing_between_slices;
    Vector3 center { camera.position.x, 0, camera.position.z };

    if (grid_y_values.empty())
        grid_y_values.resize(std::pow(grid_dimensions, 2));

    double center_of_mass_y = 0;
    double total_mass = 0;
    for (const Object& obj: scene.objects) {
        if (obj.position.distance(center) > radius)
            continue;
        center_of_mass_y += obj.mass * obj.position.y;
        total_mass += obj.mass;
    }
    if (total_mass != 0)
        center_of_mass_y /= total_mass;

    double vertical_shift = abs(center_of_mass_y - max_y_val);
    center.y = -vertical_shift;
    max_y_val = -std::numeric_limits<double>::infinity();

    // floor (not truncate) the camera offset so adjacent grid indices always
    // map to x/z values exactly spacing_between_slices apart, even across 0
    int horiz_offset = static_cast<int>(std::floor(center.x / spacing_between_slices));
    int depth_offset = static_cast<int>(std::floor(center.z / spacing_between_slices));

    for (int horiz = -slices; horiz <= slices; horiz++) {
        int adj_horiz = horiz + horiz_offset;
        const int x = adj_horiz * spacing_between_slices;
        int horiz_vector_idx = horiz + slices;
        for (int depth = -slices; depth <= slices; depth++) {
            int adj_depth = depth + depth_offset;
            const int z = adj_depth * spacing_between_slices;
            int depth_vector_idx = depth + slices;

            double y = 0;
            for (const Object& obj: scene.objects) {
                double r_s = (2 * gravitational_constant * obj.mass) / (speed_of_light * speed_of_light);
                double distance = convert_light_seconds_to_meters(obj.position.distance({(double)x, 0, (double)z}));
                distance = std::max(distance, r_s); // Flamm's paraboloid is only defined for distance >= r_s
                y += convert_meters_to_light_seconds(settings.space_time_curve_factor * sqrt(r_s * (distance - r_s)));
            }
            max_y_val = std::max(max_y_val, y);
            y += center.y;
            grid_y_values[horiz_vector_idx * grid_dimensions + depth_vector_idx] = y;

            Vector3 point = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)};

            if (horiz_vector_idx > 0) {
                Vector3 left_point = point;
                left_point.x -= spacing_between_slices;
                left_point.y = grid_y_values[(horiz_vector_idx - 1) * grid_dimensions + depth_vector_idx];
                DrawLine3D(left_point, point, WHITE);
            }
            if (depth_vector_idx > 0) {
                Vector3 up_point = point;
                up_point.z -= spacing_between_slices;
                up_point.y = grid_y_values[horiz_vector_idx * grid_dimensions + depth_vector_idx - 1];
                DrawLine3D(up_point, point, WHITE);
            }
        }
    }
}

void SimulationScreen::draw_world(const Camera3D& camera, const SimulationSettings& settings) const {
    draw_spacetime_curvature(camera, settings);

    for (const Object& obj : scene.objects) {
        obj.draw_trail();
        obj.draw(settings.objects_scale);
    }
}

std::optional<ObjectSelection> SimulationScreen::draw_object_selection_ui(Camera3D& camera,
                                                                           const SimulationSettings& settings) {
    std::optional<ObjectSelection> selection = std::nullopt;

    std::vector<std::string> object_names;
    object_names.reserve(scene.objects.size());

    int idx = 0;
    for (const Object& obj : scene.objects) {
        obj.draw_label(camera);
        object_names.push_back(obj.name);

        if (obj.draw_outline(settings.objects_scale, camera))
            selection = ObjectSelection{idx, obj.radius};

        idx++;
    }

    int new_selected_object = ObjectSelector(object_names, current_selected_object);
    if (new_selected_object != current_selected_object) {
        if (new_selected_object == -1)
            current_selected_object = -1;
        else
            selection = ObjectSelection{new_selected_object, scene.objects[new_selected_object].radius};
    }

    return selection;
}

