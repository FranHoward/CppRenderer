#pragma once
#include "Ray.h"

class Material
{
    public:
        virtual ~Material() = default;
        virtual Color brdf(const Vector3f wi, const Vector3f& wo) const = 0;
};

class LambertMaterial : public Material
{
    public:
        virtual Color brdf(const Vector3f, const Vector3f&) const override
        {
            return mAlbedo * INV_PI;
        }

    public:
        LambertMaterial(const Color& albedo) : mAlbedo(albedo) {}
        const Color& GetAlbedo() const { return mAlbedo; }

    private:
        Color mAlbedo;
};
