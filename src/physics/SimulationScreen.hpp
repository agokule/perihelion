#pragma once

#include <chrono>
#include <cstddef>
#include <optional>
#include <raylib.h>
#include <vector>

#include "Preset.hpp"
#include "Vector3Double.hpp"
#include "physics/settings.hpp"

class SimulationScreen {
public:
    explicit SimulationScreen(const Preset& initial_preset);

    void load_preset(const Preset& new_preset);

    // advances the n-body gravity simulation by one frame
    void simulate_physics(const SimulationSettings& settings);

    void update_camera(Camera3D& camera, const SimulationSettings& settings,
                        bool camera_pan_enabled, Vector2 mouse_delta, float mouse_wheel_move);

    // draws the grid, trails and objects, wraps its own Begin/EndMode3D
    void draw_world(const Camera3D& camera, const SimulationSettings& settings) const;

    // draws the ImGui object picker and returns the object the user picked this frame, if any
    std::optional<ObjectSelection> draw_object_selection_ui(Camera3D& camera, const SimulationSettings& settings,
                                                             std::optional<Vector2> left_click_pos);

    // makes idx the selected object and starts the camera lerp toward it
    void select_object(ObjectSelection selection, const SimulationSettings& settings);

    // the camera's offset from the object it's currently following
    // this is needed when the camera is far away from (0, 0, 0)
    // and you still want high decimal preceision
    Vector3Double camera_offset_from_selected() const;

    void add_object(const Object& obj);

    size_t num_objects() const { return scene.objects.size(); }

    Object& get_object(size_t idx) { return scene.objects.at(idx); }

    std::vector<Object>& get_objects() { return scene.objects; }

    int current_selected_object = -1;

private:
    Preset scene;

    float alpha = 0.0f; // horizontal camera rotation (yaw)
    float beta = 0.5f;  // vertical camera rotation (pitch)
    float distance = 1.0f;
    float wheel_sensitivity = 0.1f;

    float camera_position_lerp = -1.0f;
    float camera_target_lerp = -1.0f;
    std::chrono::time_point<std::chrono::steady_clock> camera_lerp_start;
    Vector3 camera_target_lerp_start_target{};
    Vector3 camera_position_lerp_start_position{};

    // scratch buffer for the spacetime curvature grid
    mutable std::vector<double> grid_y_values;

    // height of Flamm's paraboloid (more well known as the curvature of spacetime) at (x, z)
    double curvature_at(double x, double z, const GridSettings& settings) const;

    // must run inside Begin/EndMode3D Raylib functions
    void draw_grid(const Camera3D& camera, const GridSettings& settings) const;
};
