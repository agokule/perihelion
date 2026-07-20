#pragma once

#include <string>
#include <raylib.h>
#include <raymath.h>

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
    float mass;
    // in light seconds
    float radius;
    Vector3 position;
    Vector3 velocity;

    Object(ObjectType type, const std::string& name, float mass, float radius, Vector3 position, Vector3 starting_velocity):
        type(type),
        name(name),
        mass(mass),
        radius(radius),
        position(position),
        velocity(starting_velocity) {}

    void accelerate(Vector3 acceleration, float delta_time) {
        velocity += acceleration * delta_time;
    }

    void update_pos(float delta_time) {
        position += velocity * delta_time;
    }

    void draw() const {
        DrawSphere(position, radius, YELLOW);
    }

    void draw_label(Camera3D& camera) const {
        int font_size = 15;

        float distance = Vector3Distance(position, camera.position);

        
        // if (distance > 30)
        //     return;
        if (distance > 15)
            font_size = 5;
        if (distance > 10)
            font_size = 10;

        auto text_pos = GetWorldToScreen(position, camera);

        if (!IsObjectInCamera(position, camera))
            return;

        draw_text_centered(name, text_pos, font_size, GREEN);
    }
};

