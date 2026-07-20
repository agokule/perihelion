#include "Vector3Double.hpp"
#include <cmath>

double Vector3Double::distance(const Vector3Double& other) const {
    double dx = x - other.x;
    double dy = y - other.y;
    double dz = z - other.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

double Vector3Double::lengthSqr() const {
    return x*x + y*y + z*z;
}

double Vector3Double::length() const {
    return sqrtf(lengthSqr());
}

Vector3Double Vector3Double::normalize() const {
    return *this / length();
}

