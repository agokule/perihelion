#pragma once
#include "Vector3Double.hpp"
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

// The velocity arrow's tip, relative to the object it belongs to: offset
// out to the object's surface in the velocity's direction, then further out
// by velocity * velocity_arrow_scale. This is also the object-relative
// frame used to interactively drag the arrow (see main.cpp).
Vector3Double velocity_to_arrow_offset(Vector3Double velocity, double surface_radius, double velocity_arrow_scale);

// Inverse of velocity_to_arrow_offset: recovers the velocity that a given
// arrow-tip offset (relative to the object) represents. nullopt if the
// offset is at or inside the object's own surface, i.e. not a valid drag.
std::optional<Vector3Double> arrow_offset_to_velocity(Vector3Double offset, double surface_radius, double velocity_arrow_scale);

enum class Axis {
    X, Y, Z
};

Vector3Double axis_unit_vector(Axis axis);

// Whether the cursor is currently captured for camera panning (i.e.
// DisableCursor() is in effect). Use this instead of raylib's
// IsCursorHidden(): on desktop the two agree, but on web DisableCursor() only
// *requests* pointer lock and never sets the flag IsCursorHidden() reads, so
// that stays false forever there. This asks the browser directly instead,
// which also picks up the lock being released out from under us -- while
// locked, the browser handles Escape itself, as it does switching tabs.
bool is_cursor_locked();

// A drop-in replacement for raylib's built-in default shader, but with the
// vertex stage's position math forced to highp float instead of raylib's
// actual mediump default on GRAPHICS_API_OPENGL_ES2 (see
// assets/shaders/glsl100/default_highp.vs for why that matters on web).
// Lazily loaded on first call and cached for the life of the app; only
// meaningful once there's a live GL context (i.e. after InitWindow), and a
// no-op to call on native -- callers should guard use with
// `#if defined(__EMSCRIPTEN__)`, since native GL33 has no precision
// qualifiers to work around and doesn't need this.
Shader get_highp_default_shader();

