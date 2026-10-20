#pragma once

// basically the same thing as Vector3Double, but using polar coordinates instead to represent a 3D Vector
struct Spherical {
    double radius = 0.0;
    // always between -pi and pi
    double longitude = 0.0;
    // always between 0 and pi
    double colatitude = 0.0;

    // put colatitude back in between 0 and pi, returns true if it happened
    bool fold_colatitude();
};

double wrap_angle(double angle);

