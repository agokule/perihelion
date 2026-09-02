#pragma once

#include <chrono>
#include <cstddef>
#include <optional>
#include <raylib.h>
#include <vector>

#include "Preset.hpp"
#include "Vector3Double.hpp"
#include "physics/settings.hpp"

// owns the currently loaded preset and everything needed to simulate and
// draw it; main.cpp only calls into this when AppState::Simulation is active
class SimulationScreen {
public:
    explicit SimulationScreen(const Preset& initial_preset);

    void load_preset(const Preset& new_preset);

    // advances the n-body simulation by one frame
    void simulate_physics(const SimulationSettings& settings);

    // follows the selected object, if any; must run before draw_world so the
    // camera is up to date for this frame's render
    void update_camera(Camera3D& camera, const SimulationSettings& settings,
                        bool camera_pan_enabled);

    // draws the grid, trails and objects; wraps its own Begin/EndMode3D
    void draw_world(const Camera3D& camera, const SimulationSettings& settings) const;

    // draws the ImGui object picker; call between rlImGuiBegin/rlImGuiEnd.
    // returns the object the user picked this frame, if any, so the caller
    // can decide to call select_object
    std::optional<ObjectSelection> draw_object_selection_ui(Camera3D& camera, const SimulationSettings& settings);

    // makes idx the selected object and starts the camera lerp toward it
    void select_object(ObjectSelection selection, const SimulationSettings& settings);

    // the camera's offset from the object it's currently following, kept in
    // double precision. camera.position/camera.target (raylib's float32
    // Vector3) are computed as the object's position plus this same offset,
    // and once the followed object is far from the world origin that sum
    // only has a couple thousandths of a unit of precision left -- fine for
    // rendering, but not for pixel-accurate mouse-ray math done relative to
    // the object (e.g. dragging the velocity arrow). Callers needing that
    // extra double precision should build their own object-relative ray from
    // this offset instead of reading camera.position.
    Vector3Double camera_offset_from_selected() const;

    void add_object(const Object& obj);

    size_t num_objects() const { return scene.objects.size(); }

    Object& get_object(size_t idx) { return scene.objects.at(idx); }

    const std::vector<Object>& get_objects() const { return scene.objects; }

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
    // camera.target captured the instant a target lerp starts, so the turn
    // has a fixed start direction to slerp away from instead of re-deriving
    // it from a target that's already mid-lerp
    Vector3 camera_target_lerp_start_target{};
    // camera.position captured the instant a position lerp starts, so the
    // move has a fixed start point to lerp away from. Without this, each
    // frame re-lerps from the previous frame's already-moved position toward
    // the (moving, since the followed object keeps orbiting) goal using the
    // same eased t again -- as t approaches 1 the effective per-frame blend
    // weight explodes, so any shift in the goal position near the end of the
    // animation snaps the camera almost fully onto it, looking like a jitter/teleport
    Vector3 camera_position_lerp_start_position{};

    // scratch buffer for the spacetime curvature grid; not logical state,
    // so draw_spacetime_curvature can stay const
    mutable std::vector<double> grid_y_values;

    // height of Flamm's paraboloid at (x, z) on the grid plane: the sum of
    // every object's contribution to how far the sheet is stretched there
    double curvature_at(double x, double z, const GridSettings& settings) const;

    // draws the spacetime curvature grid, or normal grid depending on grid settings;
    // must run inside Begin/EndMode3D
    void draw_grid(const Camera3D& camera, const GridSettings& settings) const;
};
