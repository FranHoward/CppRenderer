#pragma once
#include "Primitive.h"

class Triangle : public Primitive
{
    public:
        Triangle(SceneObject* pSceneObject, const Vector3f& v0, const Vector3f& v1, const Vector3f& v2);
        virtual bool Intersect(const Ray& ray, Intersection& hit) const override;
        // Triangle vertices are already in world space (see the constructor).
        const Vector3f& GetVertex(int index) const { return mVertices[index]; }
        const Vector3f& GetNormal() const { return mNormal; }

    private:
        Vector3f mVertices[3];
        Vector3f mNormal;
};
