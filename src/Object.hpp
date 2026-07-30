#pragma once

#include <deque>
#include <filesystem>
#include <iterator>
#include <optional>
#include <string>
#include <raylib.h>
#include <raymath.h>
#include <string_view>
#include <variant>

#include "Vector3Double.hpp"
#include "imgui.h"
#include "utils.hpp"

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

    Object(ObjectType type, const std::string& name, double mass, double radius, Vector3Double position, Vector3Double starting_velocity, std::variant<std::string_view, Color> drawing_data):
        type(type),
        name(name),
        mass(mass),
        radius(radius),
        position(position),
        velocity(starting_velocity),
        previous_positions(),
        drawing_info(WHITE) {
            if (std::holds_alternative<std::string_view>(drawing_data))
                drawing_info.emplace<ObjectTextureInfo>(std::get<std::string_view>(drawing_data));
            else
                drawing_info.emplace<Color>(std::get<Color>(drawing_data));
        }

    ~Object() {
        if (!std::holds_alternative<ObjectTextureInfo>(drawing_info))
            return;
            
        auto& info {std::get<ObjectTextureInfo>(drawing_info)};
        if (info.texture)
            UnloadTexture(*info.texture);
        if (info.model)
            UnloadModel(*info.model);
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
        if (!std::holds_alternative<ObjectTextureInfo>(drawing_info))
            return;
        auto& info {std::get<ObjectTextureInfo>(drawing_info)};

        Image image = LoadImage(info.texture_path.c_str());
        ImageRotateCCW(&image);
        ImageFlipHorizontal(&image);
        info.texture = LoadTextureFromImage(image);

        Mesh sphere = GenMeshSphere(radius, 32, 32);
        info.model = LoadModelFromMesh(sphere);

        info.model->materials[0].maps[MATERIAL_MAP_ALBEDO].texture = *info.texture;
        UnloadImage(image);
    }

    void draw(float scale) const {
        if (std::holds_alternative<Color>(drawing_info))
            DrawSphere(position.to_vector3(), radius * scale, std::get<Color>(drawing_info));
        else
            DrawModelEx(*std::get<ObjectTextureInfo>(drawing_info).model, position.to_vector3(), {1, 0, 0}, 90.0f, Vector3Ones * scale, WHITE);
    }

    // returns true if object was selected
    bool draw_outline(float scale, const Camera3D& camera) const {
        if (!is_object_in_camera(position.to_vector3(), camera))
            return false;

        float screen_radius = get_screen_radius_of_sphere(camera, position.to_vector3(), radius * scale);
        screen_radius += 5;
        Vector2 circle_pos = GetWorldToScreen(position.to_vector3(), camera);

        if (position.distance(Vector3Double{camera.position}) > 50.0f)
            DrawCircleLinesV(circle_pos, screen_radius, WHITE);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 mouse_pos = GetMousePosition();
            return !ImGui::GetIO().WantCaptureMouse
                && CheckCollisionPointCircle(mouse_pos, circle_pos, screen_radius);
        }
        return false;
    }

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

