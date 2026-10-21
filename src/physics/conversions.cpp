#include "physics/conversions.hpp"
#include "physics/Spherical.hpp"
#include "physics/Vector3Double.hpp"
#include <algorithm>
#include <cmath>

Spherical vector3_to_spherical(Vector3Double v) {
    double len = v.length();
    return {
        .radius = len,
        .longitude = atan2(v.z, v.x),
        .colatitude = (len != 0) ? acos(std::clamp(v.y / v.length(), -1.0, 1.0)) : 0.0
    };
}

Vector3Double spherical_to_vector3(Spherical s) {
    return {
        s.radius * cos(s.longitude) * sin(s.colatitude),
        s.radius * cos(s.colatitude),
        s.radius * sin(s.longitude) * sin(s.colatitude)
    };
}

