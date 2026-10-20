#pragma once
#include "physics/Vector3Double.hpp"
#include <optional>
#include <raylib.h>
#include <raymath.h>
#include <string>

void draw_text_centered(const std::string& text, Vector2 pos, int font_size, Color color);
bool is_object_in_camera(Vector3 objectPos, const Camera3D& camera);

float get_screen_radius_of_sphere(const Camera3D& camera, Vector3 sphere_position, float radius);

// get the point where the ray intersects the y=0 plane
std::optional<Vector3> ray_y_plane_intersection(Ray ray, float y = 0);

struct Cone {
    Vector3Double base;
    Vector3Double tip;
    double base_radius;
};

std::optional<Cone> draw_3d_arrow(Vector3Double start, Vector3Double end, double cone_height);

// get the position of the velocity arrow's tip that should be drawn
Vector3Double velocity_to_arrow_offset(Vector3Double velocity, double surface_radius, double velocity_arrow_scale);

// inverse of velocity_to_arrow_offset
// ie: x == *arrow_offset_to_velocity(velocity_to_arrow_offset(x))
std::optional<Vector3Double> arrow_offset_to_velocity(Vector3Double offset, double surface_radius, double velocity_arrow_scale);

enum class Axis {
    X, Y, Z
};

Vector3Double axis_unit_vector(Axis axis);

// whether the cursor is locked, used instead of Raylib's IsCursorHidden() because it doesn't work on emscripten
bool is_cursor_locked();

Vector2 get_mouse_delta();

// a shader that is explicitly set to higher resolution, because Raylib by default uses a lower resolution shader on emscripten
// NOTE: should not be run on non-emscripten builds
Shader get_highp_default_shader();

