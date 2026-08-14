#pragma once

enum class GridType {
    SpacetimeCurved,
    Flat,
    None
    // maybe a 3D spacetime curvature visualization in the future?
};

struct GridSettings {
    int curved_grid_slices = 50;
    int flat_grid_slices = 100;

    float flat_grid_y = 0;

    GridType type = GridType::SpacetimeCurved;

    int spacing_between_slices = 5;
    int space_time_curve_factor = 600;
};

// tunable values that come from adjust.h and may be re-read every frame
struct SimulationSettings {
    float& delta_time;
    int& substeps_per_frame;
    float& selected_sensitivity;
    float& objects_scale;
    bool& paused;
    float& velocity_arrow_scale;
    GridSettings& grid;
};

// an object the user picked this frame, either via the radio button list or
// by clicking its outline
struct ObjectSelection {
    int idx;
    double radius;
};

