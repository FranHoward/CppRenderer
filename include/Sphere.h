#pragma once
#include "Primitive.h"

class Sphere : public Primitive
{
    public:
        Sphere(SceneObject* pSceneObject, float radius);
        virtual bool Intersect(const Ray& ray, Intersection& hit) const override;
    
    private:
        float mRadius;
};   