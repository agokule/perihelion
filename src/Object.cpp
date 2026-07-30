#include "Object.hpp"

#include <iterator>
#include <optional>
#include <variant>

#include "imgui.h"
#include "raylib.h"
#include "utils.hpp"

using std::holds_alternative;
using std::get;

ObjectTextureInfo::ObjectTextureInfo(ObjectTextureInfo&& other) noexcept:
    texture_path(other.texture_path),
    texture(other.texture),
    model(other.model) {
        other.texture = std::nullopt;
        other.model = std::nullopt;
    }

ObjectTextureInfo::~ObjectTextureInfo() {
    if (texture)
        UnloadTexture(*texture);
    if (model)
        UnloadModel(*model);
}

ObjectTextureInfo::ObjectTextureInfo(const ObjectTextureInfo& other):
    texture_path(other.texture_path),
    texture(std::nullopt),
    model(std::nullopt) {}

ObjectTextureInfo& ObjectTextureInfo::operator=(const ObjectTextureInfo& other) {
    if (this == &other)
        return *this;

    if (texture)
        UnloadTexture(*texture);
    if (model)
        UnloadModel(*model);

    texture_path = other.texture_path;
    texture = std::nullopt;
    model = std::nullopt;

    return *this;
}

ObjectTextureInfo& ObjectTextureInfo::operator=(ObjectTextureInfo&& other) noexcept {
    if (texture)
        UnloadTexture(*texture);
    if (model)
        UnloadModel(*model);

    texture_path = other.texture_path;
    texture = other.texture;
    model = other.model;

    other.texture = std::nullopt;
    other.model = std::nullopt;

    return *this;
}

void ObjectTextureInfo::load_model(double radius) {
    if (texture)
        UnloadTexture(*texture);
    if (model)
        UnloadModel(*model);

    Image image = LoadImage(texture_path.c_str());
    ImageRotateCCW(&image);
    ImageFlipHorizontal(&image);
    texture = LoadTextureFromImage(image);

    Mesh sphere = GenMeshSphere(radius, 32, 32);
    model = LoadModelFromMesh(sphere);

    model->materials[0].maps[MATERIAL_MAP_ALBEDO].texture = *texture;
    UnloadImage(image);
}

Object::Object(ObjectType type, const std::string& name, double mass, double radius, Vector3Double position, Vector3Double starting_velocity, std::variant<std::string_view, Color> drawing_data):
    type(type),
    name(name),
    mass(mass),
    radius(radius),
    position(position),
    velocity(starting_velocity),
    previous_positions(),
    drawing_info(WHITE) {
        if (holds_alternative<std::string_view>(drawing_data))
            drawing_info.emplace<ObjectTextureInfo>(get<std::string_view>(drawing_data));
        else
            drawing_info.emplace<Color>(get<Color>(drawing_data));
    }

void Object::accelerate(Vector3Double acceleration, double delta_time) {
    velocity += acceleration * delta_time;
}

void Object::update_pos(double delta_time) {
    if (previous_positions.empty() || position.distance(previous_positions.back()) > 0.05f)
        previous_positions.push_back(position.to_vector3());
    if (previous_positions.size() > 150)
        previous_positions.pop_front();

    position += velocity * delta_time;
}

void Object::load_model() {
    if (!holds_alternative<ObjectTextureInfo>(drawing_info))
        return;
    auto& info {get<ObjectTextureInfo>(drawing_info)};
    info.load_model(radius);
}

void Object::draw(float scale) const {
    if (holds_alternative<Color>(drawing_info))
        DrawSphere(position.to_vector3(), radius * scale, get<Color>(drawing_info));
    else
        DrawModelEx(*get<ObjectTextureInfo>(drawing_info).model, position.to_vector3(), {1, 0, 0}, 90.0f, Vector3Ones * scale, WHITE);
}

bool Object::draw_outline(float scale, const Camera3D& camera) const {
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

void Object::draw_trail() const {
    auto sz = previous_positions.size();
    if (sz == 0)
        return;

    for (auto it = std::next(previous_positions.cbegin()); it != previous_positions.cend(); it++) {
        auto idx = it - previous_positions.cbegin();
        float fadedness = (float)idx / sz;
        DrawLine3D(*std::prev(it), *it, Fade(WHITE, fadedness));
    }
}

void Object::draw_label(Camera3D& camera) const {
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
