#pragma once
#include "Vector3Double.hpp"
#include <optional>
#include <raylib.h>
#include <raymath.h>
#include <string>

void draw_text_centered(const std::string& text, Vector2 pos, int font_size, Color color);
bool is_object_in_camera(Vector3 objectPos, const Camera3D& camera);

constexpr float light_seconds_meters_conversion_factor = 299'792'458.0f;

template<typename T>
T convert_light_seconds_to_meters(T light_minutes) {
    return light_minutes * light_seconds_meters_conversion_factor;
}
template<typename T>
T convert_meters_to_light_seconds(T meters) {
    return meters / light_seconds_meters_conversion_factor;
}

float get_screen_radius_of_sphere(const Camera3D& camera, Vector3 sphere_position, float radius);

// get the point where the ray intersects the y=0 plane
std::optional<Vector3> ray_y_plane_intersection(Ray ray);

void draw_3d_arrow(Vector3Double start, Vector3Double end, double cone_height);

