#include "physics/conversions.hpp"
#include "physics/Spherical.hpp"
#include "physics/Vector3Double.hpp"
#include <cmath>

Spherical vector3_to_spherical(Vector3Double v) {
    return {
        .radius = v.length(),
        .longitude = atan2(v.z, v.x),
        .colatitude = acos(v.y / v.length())
    };
}

Vector3Double spherical_to_vector3(Spherical s) {
    return {
        s.radius * cos(s.longitude) * sin(s.colatitude),
        s.radius * cos(s.colatitude),
        s.radius * sin(s.longitude) * sin(s.colatitude)
    };
}

