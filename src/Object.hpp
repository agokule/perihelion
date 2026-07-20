#pragma once

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

    Object(ObjectType type, const std::string& name, float mass, float radius, Vector3Double position, Vector3Double starting_velocity):
        type(type),
        name(name),
        mass(mass),
        radius(radius),
        position(position),
        velocity(starting_velocity) {}

    void accelerate(Vector3Double acceleration, double delta_time) {
        velocity += acceleration * delta_time;
    }

    void update_pos(double delta_time) {
        position += velocity * delta_time;
    }

    void draw() const {
        DrawSphere(position.to_vector3(), radius, YELLOW);
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

        if (!IsObjectInCamera(position.to_vector3(), camera))
            return;

        draw_text_centered(name, text_pos, font_size, GREEN);
    }
};

