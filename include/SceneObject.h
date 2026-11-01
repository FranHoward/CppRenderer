#pragma once
#include "Ray.h"
#include "Primitive.h"
#include "Material.h"
#include <vector>

class SceneObject
{
    public:
        SceneObject(const Vector3f& position, const Vector3f& euler, float scale)
        {
            mObjectToWorld = MakeWorldTransform(position, euler, scale);
            mWorldToObject = glm::inverse(mObjectToWorld);
        }

        bool Intersect(Ray ray, Intersection& hit) const;

        template<typename T, typename...Args>
        T* CreatePrimitive(Args&&...args)
        {
            T* primitive = new T(this, std::forward<Args>(args)...);
            mPrimitives.push_back(primitive);
            return primitive;
        }

        virtual ~SceneObject();

        Matrix4x4 GetObjectToWorld() const { return mObjectToWorld; }
        Matrix4x4 GetWorldToObject() const { return mWorldToObject; }

        void SetMaterial(Material* material) { mMaterial = material; }
        Material* GetMaterial() const { return mMaterial; }
        const std::vector<Primitive*>& GetPrimitives() const { return mPrimitives; }

    private:
        Matrix4x4 mObjectToWorld;
        Matrix4x4 mWorldToObject;

        Material* mMaterial = nullptr;

        std::vector<Primitive*> mPrimitives;
};
