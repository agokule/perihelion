#pragma once

#include <deque>
#include <filesystem>
#include <optional>
#include <string>
#include <raylib.h>
#include <raymath.h>
#include <string_view>
#include <variant>

#include "Vector3Double.hpp"

enum class ObjectType {
    Planet,
    Star,
    BlackHole,
    Moon
};

struct ObjectTextureInfo {
    std::filesystem::path texture_path;
    std::optional<Texture> texture = std::nullopt;
    std::optional<Model> model = std::nullopt;

    ObjectTextureInfo(std::string_view path):
        texture_path(path) {}
};

struct Object {
    ObjectType type;
    std::string name;
    // in kg
    double mass;
    // in light seconds
    double radius;
    Vector3Double position;
    Vector3Double velocity;

    std::deque<Vector3> previous_positions;

    std::variant<ObjectTextureInfo, Color> drawing_info;

    Object(ObjectType type, const std::string& name, double mass, double radius, Vector3Double position, Vector3Double starting_velocity, std::variant<std::string_view, Color> drawing_data);

    ~Object();

    void accelerate(Vector3Double acceleration, double delta_time);

    void update_pos(double delta_time);

    // create a raylib model and link a texture to it if
    // a texture file was given in the constructor
    void load_model();

    void draw(float scale) const;

    // returns true if object was selected
    bool draw_outline(float scale, const Camera3D& camera) const;

    void draw_trail() const;

    void draw_label(Camera3D& camera) const;
};
