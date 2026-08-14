#include "physics/SimulationScreen.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <imgui.h>
#include <limits>
#include <raymath.h>
#include <reasings.h>
#include <vector>

#include "physics/Constants.hpp"
#include "physics/Object.hpp"
#include "Vector3Double.hpp"
#include "raylib.h"
#include "ui/ObjectSelector.hpp"
#include "rlgl.h"

using namespace std::chrono;
using namespace std::chrono_literals;

namespace {
    // rotates `start` towards `end` by fraction t (0..1) along the shortest
    // great-circle arc, so a direction turn happens at constant angular speed
    Vector3 slerp_direction(Vector3 start, Vector3 end, float t) {
        start = Vector3Normalize(start);
        end = Vector3Normalize(end);
        float dot = std::clamp(Vector3DotProduct(start, end), -1.0f, 1.0f);

        // start/end are (near) opposite: the great-circle arc between them
        // isn't unique, so pick an arbitrary axis to rotate around instead
        // of falling into the degenerate (0-length) case below
        if (dot < -0.9999f) {
            Vector3 arbitrary = std::abs(start.x) < 0.9f ? Vector3{1, 0, 0} : Vector3{0, 1, 0};
            Vector3 axis = Vector3Normalize(Vector3CrossProduct(start, arbitrary));
            float theta = PI * t;
            return start * cosf(theta) + axis * sinf(theta);
        }

        float theta = acosf(dot) * t;
        Vector3 relative = Vector3Normalize(end - start * dot);
        return start * cosf(theta) + relative * sinf(theta);
    }
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

                double distance = (obj.position.distance(obj2.position));
                double distanceSqr = distance * distance;
                double gravity_acceleration = gravitational_constant * obj2.mass / distanceSqr;

                Vector3Double direction_of_acceleration = (obj2.position - obj.position).normalize();

                Vector3Double acceleration = direction_of_acceleration * gravity_acceleration;
                obj.accelerate((acceleration), settings.delta_time);
            }
        }
        for (Object& obj : scene.objects) {
            obj.update_pos(settings.delta_time);
        }
    }
}

void SimulationScreen::select_object(ObjectSelection selection, const SimulationSettings& settings) {
    if (current_selected_object == selection.idx)
        return;

    current_selected_object = selection.idx;
    distance = std::clamp(selection.radius * 5 * settings.objects_scale, 0.1, 1e100);
    camera_target_lerp = camera_position_lerp = 0.0f;
    camera_lerp_start = steady_clock::now();
}

Vector3Double SimulationScreen::camera_offset_from_selected() const {
    return Vector3Double {
        distance * std::cos(beta) * std::sin(alpha),
        distance * std::sin(beta),
        distance * std::cos(beta) * std::cos(alpha)
    };
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
                                      bool camera_pan_enabled) {
    if (current_selected_object == -1)
        return;

    const Object& obj = scene.objects.at(current_selected_object);
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
        if (camera_target_lerp == 0.0f)
            camera_target_lerp_start_target = camera.target;

        auto now = steady_clock::now();

        camera_target_lerp = camera_target_lerp != 1
            ? EaseSineInOut(duration_cast<milliseconds>(now - camera_lerp_start).count(), 0.0f, 1.0f, 1000.0f)
            : 1;
        if (now - camera_lerp_start >= 1s) {
            if (camera_target_lerp != 1.0f)
                camera_target_lerp = 1.0f;
            else if (camera_position_lerp != 1)
                camera_position_lerp = 1;
            camera_lerp_start = now;
        }
        // camera doesn't move during the target-turn phase, so keep capturing
        // its (static) position as the position-lerp's start point right up
        // until the position lerp actually begins
        if (camera_target_lerp != 1.0f)
            camera_position_lerp_start_position = camera.position;

        if (camera_target_lerp == 1.0f && camera_position_lerp != 1.0f)
            camera_position_lerp = EaseCircInOut(duration_cast<milliseconds>(now - camera_lerp_start).count(), 0.0f, 1.0f, 1000.0f);
        camera.position = Vector3Lerp(camera_position_lerp_start_position, camera_position, camera_position_lerp);

        // turn towards the new target by rotating the look direction
        // (slerp) around the camera
        Vector3 start_offset = camera_target_lerp_start_target - camera.position;
        Vector3 end_offset = obj.position.to_vector3() - camera.position;
        float target_distance = Lerp(Vector3Length(start_offset), Vector3Length(end_offset), camera_target_lerp);
        Vector3 direction = slerp_direction(start_offset, end_offset, camera_target_lerp);
        camera.target = camera.position + direction * target_distance;

        if (camera_position_lerp == 1.0f)
            camera_target_lerp = camera_position_lerp = -1.0f;
    }
}

void SimulationScreen::draw_grid(const Camera3D& camera, const GridSettings& settings) const {
    if (settings.type == GridType::None)
        return;

    int slices = settings.type == GridType::SpacetimeCurved ? settings.curved_grid_slices : settings.flat_grid_slices;
    int spacing_between_slices = settings.spacing_between_slices;
    int grid_dimensions = slices * 2 + 1;
    double radius = slices * spacing_between_slices;
    Vector3 center { camera.position.x, 0, camera.position.z };

    if (grid_y_values.size() != grid_dimensions * grid_dimensions)
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
    double difference = abs(vertical_shift - previous_vertical_shift);
    if (difference >= 0.2) {
        center.y = -vertical_shift;
        previous_vertical_shift = vertical_shift;
    } else
        center.y = -previous_vertical_shift;
    max_y_val = -std::numeric_limits<double>::infinity();

    if (settings.type == GridType::Flat)
        center.y = settings.flat_grid_y;

    // floor (not truncate) the camera offset so adjacent grid indices always
    // map to x/z values exactly spacing_between_slices apart, even across 0
    int horiz_offset = static_cast<int>(std::floor(center.x / spacing_between_slices));
    int depth_offset = static_cast<int>(std::floor(center.z / spacing_between_slices));
    
    rlBegin(RL_LINES);

    rlColor4ub(255, 255, 255, 200);

    for (int horiz = -slices; horiz <= slices; horiz++) {
        int adj_horiz = horiz + horiz_offset;
        const int x = adj_horiz * spacing_between_slices;
        int horiz_vector_idx = horiz + slices;
        for (int depth = -slices; depth <= slices; depth++) {
            int adj_depth = depth + depth_offset;
            const int z = adj_depth * spacing_between_slices;
            int depth_vector_idx = depth + slices;

            double y = 0;
            if (settings.type == GridType::SpacetimeCurved) {
                for (const Object& obj: scene.objects) {
                    double r_s = (2 * gravitational_constant * obj.mass) / (speed_of_light * speed_of_light);
                    double distance = obj.position.distance({(double)x, 0, (double)z});
                    distance = std::max(distance, r_s); // Flamm's paraboloid is only defined for distance >= r_s
                    y += settings.space_time_curve_factor * sqrt(r_s * (distance - r_s));
                }
                max_y_val = std::max(max_y_val, y);
            }
            y += center.y;
            grid_y_values[horiz_vector_idx * grid_dimensions + depth_vector_idx] = y;

            Vector3 point = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)};

            if (horiz_vector_idx > 0) {
                Vector3 left_point = point;
                left_point.x -= spacing_between_slices;
                left_point.y = grid_y_values[(horiz_vector_idx - 1) * grid_dimensions + depth_vector_idx];

                rlVertex3f(left_point.x, left_point.y, left_point.z);
                rlVertex3f(point.x, point.y, point.z);
            }
            if (depth_vector_idx > 0) {
                Vector3 up_point = point;
                up_point.z -= spacing_between_slices;
                up_point.y = grid_y_values[horiz_vector_idx * grid_dimensions + depth_vector_idx - 1];

                rlVertex3f(up_point.x, up_point.y, up_point.z);
                rlVertex3f(point.x, point.y, point.z);
            }
        }
    }

    rlEnd();
}

void SimulationScreen::draw_world(const Camera3D& camera, const SimulationSettings& settings) const {
    draw_grid(camera, settings.grid);

    for (const Object& obj : scene.objects) {
        obj.draw_trail();
        obj.draw(settings.objects_scale);
    }
}

std::optional<ObjectSelection> SimulationScreen::draw_object_selection_ui(Camera3D& camera,
                                                                           const SimulationSettings& settings) {
    std::optional<ObjectSelection> selection = std::nullopt;

    int idx = 0;
    for (const Object& obj : scene.objects) {
        obj.draw_label(camera);

        if (obj.draw_outline(settings.objects_scale, camera))
            selection = ObjectSelection{idx, obj.radius};

        idx++;
    }

    int new_selected_object = ObjectSelector(scene.objects, current_selected_object);
    if (new_selected_object != current_selected_object) {
        if (new_selected_object == -1)
            current_selected_object = -1;
        else
            selection = ObjectSelection{new_selected_object, scene.objects[new_selected_object].radius};
    }

    return selection;
}

