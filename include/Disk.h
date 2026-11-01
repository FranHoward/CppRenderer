#pragma once
#include "Primitive.h"

class Disk : public Primitive
{
    public:
        Disk(SceneObject* pSceneObject, float radius);
        virtual bool Intersect(const Ray& ray, Intersection& hit) const override;
        float GetRadius() const { return mRadius; }

    private:
        Matrix4x4 mObjectToWorld; // Transformation matrix from object space to world space
        Matrix4x4 mWorldToObject; // Transformation matrix from world space to object space
        float mRadius;
};
