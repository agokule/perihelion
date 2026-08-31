#pragma once

#include "Spherical.hpp"
#include "Vector3Double.hpp"

enum class VelocityType : int {
    Cartesian,
    Polar,
};

void DragVelocity(Vector3Double& velocity);
void DragVelocity(Spherical& velocity);
