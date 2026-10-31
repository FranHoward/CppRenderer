#pragma once

#include <iostream>
#include <random>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/ext.hpp>

using Vector2f = glm::vec2;
using Vector3f = glm::vec3;
using Vector4f = glm::vec4;

using Vector2i = glm::ivec2;
using Vector3i = glm::ivec3;
using Vector4i = glm::ivec4;

using Matrix3x3 = glm::mat3;
using Matrix4x4 = glm::mat4;

using Color = glm::vec3;

const float PI = glm::pi<float>();
const float INV_PI = 1.0f / PI;

// Print a Vector3f to the console
inline void PrintVector(const Vector3f& v) {
    std::cout << "Vector3f: (" << v.x << ", " << v.y << ", " << v.z << ")" << std::endl;
}

// Construct a transition matrix from a translation vector
inline Matrix4x4 MakeTransition(const Vector3f& t)
{
    return Matrix4x4(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        t.x, t.y, t.z, 1.0f
    );
}

// Construct a rotation matrix from Euler angles (in radians)
inline Matrix4x4 MakeRotation(const Vector3f& euler)
{
    // Rotate x first, then y, then z
    float cx = cosf(euler.x);
    float sx = sinf(euler.x);
    float cy = cosf(euler.y);
    float sy = sinf(euler.y);
    float cz = cosf(euler.z);
    float sz = sinf(euler.z);

    Matrix4x4 rx(
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, cx, sx, 0.0f,
        0.0f, -sx, cx, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );

    Matrix4x4 ry(
        cy, 0.0f, -sy, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        sy, 0.0f, cy, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );

    Matrix4x4 rz(
        cz, sz, 0.0f, 0.0f,
        -sz, cz, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );

    return rz * ry * rx;
}

// Construct a scale matrix from a uniform scale factor
inline Matrix4x4 MakeScale(float s)
{
    return Matrix4x4(
        s, 0.0f, 0.0f, 0.0f,
        0.0f, s, 0.0f, 0.0f,
        0.0f, 0.0f, s, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    );
}

// Construct a world transformation matrix from position, rotation, and scale
inline Matrix4x4 MakeWorldTransform(const Vector3f& position, const Vector3f& rotation, float s)
{
    Matrix4x4 T = MakeTransition(position);
    Matrix4x4 R = MakeRotation(rotation);
    Matrix4x4 S = MakeScale(s); 

    return T * R * S;
}

// Construct a new coordinate system to the world coordinate system based on a given vector w
inline Matrix3x3 MakeCoordinateSystem(const Vector3f& w)
{
    Vector3f u(1.0f, 0.0f, 0.0f);

    if (glm::dot(w, u) > 0.999f)
    {
        u = Vector3f(0.0f, 1.0f, 0.0f);
    }

    Vector3f v = glm::cross(w, u);
    u = glm::cross(v, w);

    u = glm::normalize(u);
    v = glm::normalize(v);

    return Matrix3x3(u, v, w);
}

inline float Random01()
{
    thread_local static std::mt19937 generator(std::random_device{}());
    thread_local static std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
    return distribution(generator);
}

inline float Random(float a, float b)
{
    return a + (b - a) * Random01();
}

inline Vector3f GetSphericalCoordinate(float theta, float phi)
{
    float sinTheta = sinf(theta);
    return Vector3f(
        sinTheta * cosf(phi),
        sinTheta * sinf(phi),
        cosf(theta)
    );
}
