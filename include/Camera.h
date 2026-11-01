#pragma once
#include "Ray.h"

class Camera
{
    public:
        void Initialize(const Vector3f& position, const Vector3f& target,
            const Vector3f& up, float fov, float n, float f, float w, float h);

        Ray GetRay(float x, float y) const;
        const Vector3f& GetPosition() const { return mPosition; }
        const Vector3f& GetTarget() const { return mTarget; }
        const Vector3f& GetUp() const { return mUp; }
        float GetFov() const { return mFov; }
        float GetNearPlane() const { return mNear; }
        float GetFarPlane() const { return mFar; }

    private:
        Vector3f mPosition;
        Matrix4x4 mCombinedMatrix;
        Matrix4x4 mInverseCombinedMatrix;
        Vector3f mTarget = Vector3f(0, 0, 1);
        Vector3f mUp = Vector3f(0, 1, 0);
        float mFov = 45.0f;
        float mNear = 0.1f;
        float mFar = 100.0f;
};
