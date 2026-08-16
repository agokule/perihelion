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

