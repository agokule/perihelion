#pragma once

#include <raylib.h>

struct Vector3Double {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    Vector3Double(double x, double y, double z): x {x}, y {y}, z {z} {}
    Vector3Double(Vector3 v): x {v.x}, y {v.y}, z {v.z} {}

    // Helper to convert to Raylib's float Vector3 when rendering
    Vector3 to_vector3() const {
        return Vector3{ (float)x, (float)y, (float)z };
    }

    double distance(const Vector3Double& other) const;
    double length() const;
    double lengthSqr() const;
    Vector3Double normalize() const;

    Vector3Double operator+(const Vector3Double& v) const { return { x + v.x, y + v.y, z + v.z }; }
    Vector3Double operator-(const Vector3Double& v) const { return { x - v.x, y - v.y, z - v.z }; }

    Vector3Double operator+(double s) const { return { x + s, y + s, z + s }; }
    Vector3Double operator-(double s) const { return { x - s, y - s, z - s }; }
    Vector3Double operator*(double s) const { return { x * s, y * s, z * s }; }
    Vector3Double operator/(double s) const { return { x / s, y / s, z / s }; }

    Vector3Double& operator+=(const Vector3Double& v) { x += v.x; y += v.y; z += v.z; return *this; }
    Vector3Double& operator/=(double s) { x /= s; y /= s; z /= s; return *this; }
};

