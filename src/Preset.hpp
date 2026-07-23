#pragma once

#include "Object.hpp"
#include <filesystem>
#include <vector>
#include <raylib.h>
#include <string>

struct Preset {
    std::string name;
    std::vector<Object> objects;
};

const std::vector<Preset> presets {
    {
        "The Solar System",
        {
            // THE SUN
            // Mass: 1.989e30 kg | Radius: 2.32 light-seconds
            Object(ObjectType::Star, "The Sun", 1.989e30f, 2.3216f, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, "./assets/textures/2k_sun.jpg"),

            // MERCURY (Near perihelion)
            // Mass: 3.301e23 kg | Radius: ~0.008 light-seconds
            Object(ObjectType::Planet, "Mercury", 3.301e23f, 0.00814f, 
                   {-55.5f, 0.0f, 152.0f}, 
                   {-0.0001815f, 0.0f, -0.0000664f}, "./assets/textures/2k_mercury.jpg"),

            // VENUS (Very circular orbit)
            // Mass: 4.867e24 kg | Radius: ~0.020 light-seconds
            Object(ObjectType::Planet, "Venus", 4.867e24f, 0.02019f, 
                   {-328.0f, 0.0f, 267.5f}, 
                   {-0.0000843f, 0.0f, -0.0001034f}, "./assets/textures/2k_venus_surface.jpg"),

            // EARTH (Near perihelion)
            // Mass: 5.972e24 kg | Radius: ~0.021 light-seconds
            Object(ObjectType::Planet, "Earth", 5.972e24f, 0.02125f, 
                   {-89.4f, 0.0f, 482.5f}, 
                   {-0.00009765f, 0.0f, -0.00001808f}, "./assets/textures/2k_earth_daymap.jpg"),

            // Moon (Position offset +1.282 ls on X, Velocity offset +0.000003409 ls/s on Z)
            Object(ObjectType::Moon, "The Moon", 7.349e22f, 0.005795f, 
                   { -88.118f, 0.0f, 482.5f }, 
                   { -0.00009765f, 0.0f, -0.000014671f }, "./assets/textures/2k_moon.jpg"),

            // MARS
            // Mass: 6.417e23 kg | Radius: ~0.011 light-seconds
            Object(ObjectType::Planet, "Mars", 6.417e23f, 0.01131f, 
                   {658.0f, 0.0f, 381.0f}, 
                   {-0.0000401f, 0.0f, 0.0000692f}, "./assets/textures/2k_mars.jpg"),

            // JUPITER
            // Mass: 1.898e27 kg | Radius: ~0.233 light-seconds
            Object(ObjectType::Planet, "Jupiter", 1.898e27f, 0.23320f, 
                   {-1520.0f, 0.0f, 2125.0f}, 
                   {-0.0000355f, 0.0f, -0.0000254f}, "./assets/textures/2k_jupiter.jpg"),

            // SATURN
            // Mass: 5.685e26 kg | Radius: ~0.194 light-seconds
            Object(ObjectType::Planet, "Saturn", 5.685e26f, 0.19424f, 
                   {4420.0f, 0.0f, -1810.0f}, 
                   {0.0000122f, 0.0f, 0.0000298f}, "./assets/textures/2k_saturn.jpg"),

            // URANUS
            // Mass: 8.682e25 kg | Radius: ~0.084 light-seconds
            Object(ObjectType::Planet, "Uranus", 8.682e25f, 0.08460f, 
                   {-8840.0f, 0.0f, -3710.0f}, 
                   {0.0000088f, 0.0f, -0.0000209f}, "./assets/textures/2k_uranus.jpg"),

            // NEPTUNE
            // Mass: 1.024e26 kg | Radius: ~0.082 light-seconds
            Object(ObjectType::Planet, "Neptune", 1.024e26f, 0.08213f, 
                   {2110.0f, 0.0f, -14910.0f}, 
                   {0.0000180f, 0.0f, 0.0000025f}, "./assets/textures/2k_neptune.jpg")
        }
    }
};
