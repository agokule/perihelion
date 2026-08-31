#pragma once

#include "Vector3Double.hpp"

// Spherical coordinates in raylib's y-up convention:
//   longitude   heading in the horizontal x-z plane, 0 along +x, increasing towards +z
//   colatitude  measured down from +y: 0 = straight up, pi = straight down
//
// Deliberately a distinct type from Vector3Double rather than a tagged one: the
// two can't be mixed in arithmetic by accident, and the components get names
// instead of pretending to be x/y/z.
struct Spherical {
    double radius = 0.0;
    double longitude = 0.0;
    double colatitude = 0.0;
};

Spherical to_spherical(const Vector3Double& v);
Vector3Double to_cartesian(const Spherical& s);

// Wrap an angle into [-pi, pi].
double wrap_angle(double angle);

// Fold an unbounded colatitude back into [0, pi], as an editing widget may drive
// it past a pole. Returns true when a net-odd number of poles was crossed, in
// which case the caller must flip longitude by pi to name the same direction.
bool fold_colatitude(double& colatitude);
