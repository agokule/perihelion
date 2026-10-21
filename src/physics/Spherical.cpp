#include "Spherical.hpp"
#include <cmath>
#include <numbers>

using std::numbers::pi;

bool Spherical::fold_colatitude() {
    colatitude = std::fmod(colatitude, 2 * pi);
    if (colatitude < 0)
        colatitude += 2 * pi;

    if (colatitude > pi) {
        colatitude = 2 * pi - colatitude;
        return true;
    }

    return false;
}

double wrap_angle(double angle) {
    angle = std::fmod(angle + pi, 2 * pi);
    if (angle < 0)
        angle += 2 * pi;

    return angle - pi;
}

