#include "SceneObject.h"

bool SceneObject::Intersect(Ray ray, Intersection& hit) const
{
    bool bHit = false;
    for (const auto& primitive : mPrimitives)
    {
        if (primitive->Intersect(ray, hit))
        {
            ray.maxt = hit.t;
            bHit = true;
        }
    }

    return bHit;
}

SceneObject::~SceneObject()
{
    for (auto& primitive: mPrimitives)
    {
        if (primitive)
            delete primitive;
    }
}