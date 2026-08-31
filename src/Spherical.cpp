#include "Spherical.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace {
constexpr double pi = std::numbers::pi;
constexpr double tau = 2.0 * pi;
}

Spherical to_spherical(const Vector3Double& v) {
    double radius = v.length();
    return {
        radius,
        atan2(v.z, v.x),
        radius > 0.0 ? acos(std::clamp(v.y / radius, -1.0, 1.0)) : 0.0,
    };
}

Vector3Double to_cartesian(const Spherical& s) {
    return {
        s.radius * sin(s.colatitude) * cos(s.longitude),
        s.radius * cos(s.colatitude),
        s.radius * sin(s.colatitude) * sin(s.longitude),
    };
}

double wrap_angle(double angle) {
    double wrapped = std::fmod(angle + pi, tau);
    if (wrapped < 0.0)
        wrapped += tau;
    return wrapped - pi;
}

bool fold_colatitude(double& colatitude) {
    double folded = std::fmod(colatitude, tau);
    if (folded < 0.0)
        folded += tau;

    if (folded > pi) {
        colatitude = tau - folded;
        return true;
    }

    colatitude = folded;
    return false;
}
