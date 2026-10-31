#pragma once
#include "Common.h"

struct Ray
{
    Vector3f o;
    Vector3f d;

    float mint = 0.0f;
    float maxt = FLT_MAX;
};

struct Intersection
{
    float t;
    Vector3f position;
    Vector3f normal;
};

inline Ray operator*(const Matrix4x4& transform, const Ray& ray)
{
    Ray transformedRay;
    transformedRay.o = Vector3f(transform * Vector4f(ray.o, 1.0f));
    transformedRay.d = Vector3f(transform * Vector4f(ray.d, 0.0f));
    transformedRay.mint = ray.mint;
    transformedRay.maxt = ray.maxt;
    return transformedRay;
}