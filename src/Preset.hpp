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
            Object(ObjectType::Star, "The Sun", 1.989e30, 2.32061, {}, {}),
            Object(ObjectType::Planet, "Earth", 5.972e24, 0.0003545866667, { -1.49, 0, 8.041667 }, { -0.0000016275, 0, -0.000000027125 })
        }
    }
};

