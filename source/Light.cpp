#include "Light.h"

Color DirectionalLight::GetRadiance(const Vector3f& p, Vector3f& sourcePos) const
{
    sourcePos = p - mDirection * 100000.0f;
    return mRadiance;
}

Color PointLight::GetRadiance(const Vector3f& p, Vector3f& sourcePos) const
{
    sourcePos = mPosition;
    float r = glm::length(p - mPosition);
    float attenuation = 1.0f / (mAttenuations.z + mAttenuations.y * r + mAttenuations.x * r * r);
    return mIntensity * attenuation;
}

Color SpotLight::GetRadiance(const Vector3f& p, Vector3f& sourcePos) const
{
    sourcePos = mPosition;
    float r = glm::length(p - mPosition);
    float k1 = 1.0f / (mAttenuations.z + mAttenuations.y * r + mAttenuations.x * r * r);

    Vector3f l = glm::normalize(p - mPosition);
    float cosTheta = glm::dot(l, mDirection);

    float k2 = (cosTheta - mCosOuterAngle) / (mCosInnerAngle - mCosOuterAngle);

    return mIntensity * k1 * glm::clamp(k2, 0.0f, 1.0f);
}
