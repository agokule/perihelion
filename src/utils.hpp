#pragma once
#include <raylib.h>
#include <raymath.h>
#include <string>

void draw_text_centered(const std::string& text, Vector2 pos, int font_size, Color color);
bool IsObjectInCamera(Vector3 objectPos, const Camera3D& camera);

constexpr float light_minutes_meters_conversion_factor = 17'987'547'480.0f;

float convert_light_minutes_to_meters(float light_minutes);
Vector3 convert_light_minutes_to_meters(Vector3 light_minutes);
float convert_meters_to_light_minutes(float light_minutes);
Vector3 convert_meters_to_light_minutes(Vector3 light_minutes);

