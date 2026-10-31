#pragma once
#include "Ray.h"
#include "Primitive.h"

class SceneObject;

class Primitive
{
    public:
        Primitive(SceneObject* pSceneObject) : m_pSceneObject(pSceneObject) {}
        virtual bool Intersect(const Ray& ray, Intersection& hit) const = 0;
        virtual ~Primitive() {}

    protected:
        SceneObject* m_pSceneObject = nullptr;
};