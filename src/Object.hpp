#pragma once

#include <deque>
#include <filesystem>
#include <iterator>
#include <optional>
#include <string>
#include <raylib.h>
#include <raymath.h>

#include "Vector3Double.hpp"
#include "utils.hpp"

enum class ObjectType {
    Planet,
    Star,
    BlackHole,
    Moon
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

    std::optional<std::filesystem::path> texture_path;
    std::optional<Texture> texture;
    std::optional<Model> model;

    Object(ObjectType type, const std::string& name, float mass, float radius, Vector3Double position, Vector3Double starting_velocity, const std::optional<std::filesystem::path>& texture_path):
        type(type),
        name(name),
        mass(mass),
        radius(radius),
        position(position),
        velocity(starting_velocity),
        texture_path(texture_path),
        previous_positions() {
            if (!texture_path) {
                texture = std::nullopt;
                model = std::nullopt;
                return;
            }
        }

    ~Object() {
        if (texture)
            UnloadTexture(*texture);
        if (model)
            UnloadModel(*model);
    }

    void accelerate(Vector3Double acceleration, double delta_time) {
        velocity += acceleration * delta_time;
    }

    void update_pos(double delta_time) {
        if (previous_positions.empty() || position.distance(previous_positions.back()) > 0.05f)
            previous_positions.push_back(position.to_vector3());
        if (previous_positions.size() > 150)
            previous_positions.pop_front();

        position += velocity * delta_time;
    }

    void load_model() {
        if (!texture_path)
            return;
        Image image = LoadImage(texture_path->c_str());
        ImageRotateCCW(&image);
        ImageFlipHorizontal(&image);
        texture = LoadTextureFromImage(image);

        Mesh sphere = GenMeshSphere(radius, 32, 32);
        model = LoadModelFromMesh(sphere);

        model->materials[0].maps[MATERIAL_MAP_ALBEDO].texture = *texture;
        UnloadImage(image);
    }

    void draw(float scale) const {
        if (!texture_path)
            DrawSphere(position.to_vector3(), radius * scale, YELLOW);
        else
            DrawModelEx(*model, position.to_vector3(), {1, 0, 0}, 90.0f, Vector3Ones * scale, WHITE);
    }

    void draw_outline(float scale, const Camera3D& camera) const {
        if (!is_object_in_camera(position.to_vector3(), camera))
            return;

        float screen_radius = get_screen_radius_of_sphere(camera, position.to_vector3(), radius * scale);
        screen_radius += 5;
        DrawCircleLinesV(GetWorldToScreen(position.to_vector3(), camera), screen_radius, WHITE);

    void draw_trail() const {
        auto sz = previous_positions.size();

        for (auto it = std::next(previous_positions.cbegin()); it != previous_positions.cend(); it++) {
            auto idx = it - previous_positions.cbegin();
            float fadedness = (float)idx / sz;
            DrawLine3D(*std::prev(it), *it, Fade(WHITE, fadedness));
        }
    }

    void draw_label(Camera3D& camera) const {
        int font_size = 15;

        float distance = position.distance(camera.position);

        
        // if (distance > 30)
        //     return;
        if (distance > 15)
            font_size = 5;
        if (distance > 10)
            font_size = 10;

        auto text_pos = GetWorldToScreen(position.to_vector3(), camera);
        if (distance > 50)
            text_pos.y -= 10;

        if (!is_object_in_camera(position.to_vector3(), camera))
            return;

        draw_text_centered(name, text_pos, font_size, GREEN);
    }
};

