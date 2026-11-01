#pragma once
#include "SceneObject.h"
#include "Camera.h"
#include "Light.h"
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

class Scene
{
    public:
        // Returns an owned Scene; throws on file/schema errors.
        static Scene* LoadSceneFromXML(const char* filepath, int w, int h);
        
        void SetCamera(const Camera& camera) { mCamera = camera; }
        const Camera& GetCamera() const { return mCamera; }

        SceneObject* Intersect(Ray ray, Intersection& hit) const;
        SceneObject* CreateSceneObject(const Vector3f& position, const Vector3f& euler, float scale);
        ~Scene();

        template<typename T, typename...Args>
        T* CreateLight(Args&&...args)
        {
            Light* light = new T(std::forward<Args>(args)...);
            mLights.push_back(light);
            return (T*) light;
        }

        template<typename T, typename...Args>
        T* CreateMaterial(const std::string& name, Args&&...args)
        {
            if (name.empty() || mMaterials.count(name))
                throw std::invalid_argument("Material name must be nonempty and unique: " + name);
            auto material = std::make_unique<T>(std::forward<Args>(args)...);
            mMaterials.emplace(name, material.get());
            return material.release();
        }

        Material* GetMaterial(const std::string& name) const
        {
            const auto found = mMaterials.find(name);
            return found == mMaterials.end() ? nullptr : found->second;
        }

        const std::vector<Light*>& GetLights() const { return mLights; }
        const std::vector<SceneObject*>& GetSceneObjects() const { return mSceneObjects; }

    private: 
        Camera mCamera;
        std::vector<SceneObject*> mSceneObjects;
        std::vector<Light*> mLights;
        std::map<std::string, Material*> mMaterials;
};
