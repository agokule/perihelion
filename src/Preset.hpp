#pragma once

#include "Object.hpp"
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
            // 1. THE SUN
            // Mass: 1.989e30 kg | Radius: 2.32 light-seconds
            Object(ObjectType::Star, "The Sun", 1.989e30f, 2.3216f, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}),

            // 2. MERCURY (Near perihelion)
            // Mass: 3.301e23 kg | Radius: ~0.008 light-seconds
            Object(ObjectType::Planet, "Mercury", 3.301e23f, 0.00814f, 
                   {-55.5f, 0.0f, 152.0f}, 
                   {-0.0001815f, 0.0f, -0.0000664f}),

            // 3. VENUS (Very circular orbit)
            // Mass: 4.867e24 kg | Radius: ~0.020 light-seconds
            Object(ObjectType::Planet, "Venus", 4.867e24f, 0.02019f, 
                   {-328.0f, 0.0f, 267.5f}, 
                   {-0.0000843f, 0.0f, -0.0001034f}),

            // 4. EARTH (Near perihelion)
            // Mass: 5.972e24 kg | Radius: ~0.021 light-seconds
            Object(ObjectType::Planet, "Earth", 5.972e24f, 0.02125f, 
                   {-89.4f, 0.0f, 482.5f}, 
                   {-0.00009765f, 0.0f, -0.00001808f}),

            // 5. MARS
            // Mass: 6.417e23 kg | Radius: ~0.011 light-seconds
            Object(ObjectType::Planet, "Mars", 6.417e23f, 0.01131f, 
                   {658.0f, 0.0f, 381.0f}, 
                   {-0.0000401f, 0.0f, 0.0000692f}),

            // 6. JUPITER
            // Mass: 1.898e27 kg | Radius: ~0.233 light-seconds
            Object(ObjectType::Planet, "Jupiter", 1.898e27f, 0.23320f, 
                   {-1520.0f, 0.0f, 2125.0f}, 
                   {-0.0000355f, 0.0f, -0.0000254f}),

            // 7. SATURN
            // Mass: 5.685e26 kg | Radius: ~0.194 light-seconds
            Object(ObjectType::Planet, "Saturn", 5.685e26f, 0.19424f, 
                   {4420.0f, 0.0f, -1810.0f}, 
                   {0.0000122f, 0.0f, 0.0000298f}),

            // 8. URANUS
            // Mass: 8.682e25 kg | Radius: ~0.084 light-seconds
            Object(ObjectType::Planet, "Uranus", 8.682e25f, 0.08460f, 
                   {-8840.0f, 0.0f, -3710.0f}, 
                   {0.0000088f, 0.0f, -0.0000209f}),

            // 9. NEPTUNE
            // Mass: 1.024e26 kg | Radius: ~0.082 light-seconds
            Object(ObjectType::Planet, "Neptune", 1.024e26f, 0.08213f, 
                   {2110.0f, 0.0f, -14910.0f}, 
                   {0.0000180f, 0.0f, 0.0000025f})
        }
    }
};
