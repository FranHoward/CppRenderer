#pragma once
#include "Ray.h"

class Light
{
    public:
        virtual ~Light() = default;
        virtual Color GetRadiance(const Vector3f& p, Vector3f& sourcePos) const = 0;
};

class DirectionalLight : public Light
{
    public:
        DirectionalLight(const Vector3f& direction, const Color& radiance)
            : mDirection(glm::normalize(direction)), mRadiance(radiance) {}

        virtual Color GetRadiance(const Vector3f& p, Vector3f& sourcePos) const override;

    private:
        Vector3f mDirection;
        Color mRadiance;
};

class PointLight : public Light
{
    public:
        PointLight(const Vector3f& position, const Color& intensity, const Vector3f& attenuations)
            : mPosition(position), mIntensity(intensity), mAttenuations(attenuations) {}

        virtual Color GetRadiance(const Vector3f& p, Vector3f& sourcePos) const override;

    private:
        Vector3f mPosition;
        Color mIntensity;
        Vector3f mAttenuations;
};

class SpotLight : public Light
{
    public:
        SpotLight(const Vector3f& position, const Vector3f& direction, const Color& intensity, 
            const Vector3f& attenuations, float innerAngle, float outerAngle)
            : mDirection(glm::normalize(direction)), mPosition(position), mIntensity(intensity),
            mCosInnerAngle(std::cos(glm::radians(innerAngle))),
            mCosOuterAngle(std::cos(glm::radians(outerAngle))), mAttenuations(attenuations) {}

        virtual Color GetRadiance(const Vector3f& p, Vector3f& sourcePos) const override;

    private:
        Vector3f mDirection; 
        Vector3f mPosition;
        Color mIntensity;

        float mCosInnerAngle;
        float mCosOuterAngle; 

        Vector3f mAttenuations;
};
