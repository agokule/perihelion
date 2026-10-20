#pragma once

#include "physics/Spherical.hpp"
#include "physics/Vector3Double.hpp"

enum class VelocityType : int {
    Cartesian,
    Polar,
};

void DragVelocity(Vector3Double& velocity);
void DragVelocity(Spherical& velocity);
