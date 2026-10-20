#include "Spherical.hpp"
#include <cmath>
#include <numbers>

using std::numbers::pi;

bool Spherical::fold_colatitude() {
    colatitude = std::fmod(colatitude, 2 * pi);
    return colatitude < 0.0 || colatitude > pi;
}

double wrap_angle(double angle) {
    angle = std::fmod(angle + pi, 2 * pi);
    if (angle < 0)
        angle += 2 * pi;

    return angle - pi;
}

