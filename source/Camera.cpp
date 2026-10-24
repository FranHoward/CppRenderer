#include "Camera.h"

void Camera::Initialize(const Vector3f& position, const Vector3f& target,
    const Vector3f& up, float fov, float n, float f, float w, float h)
{
    mPosition = position;

    // Find the left-handed view matrix using the position, target, and up vector
    Matrix4x4 viewMatrix = glm::lookAtLH(mPosition, target, up);

    // Find the perspective projection matrix using the field of view,
    // near and far planes, and viewport dimensions
    Matrix4x4 projectionMatrix = glm::perspectiveFovLH_ZO(glm::radians(fov), (float)w, (float)h, n, f);

    // Find the viewport matrix using the viewport dimensions
    Matrix4x4 viewportMatrix = glm::mat4(
        w / 2.0f, 0.0f, 0.0f, 0.0f,
        0.0f, h / 2.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        w / 2.0f, h / 2.0f, 0.0f, 1.0f
    );

    // Combine the matrices to get the final transformation matrix
    Matrix4x4 combinedMatrix = viewportMatrix * projectionMatrix * viewMatrix;
    Matrix4x4 inverseCombinedMatrix = glm::inverse(combinedMatrix);

    mCombinedMatrix = combinedMatrix;
    mInverseCombinedMatrix = inverseCombinedMatrix;
 }

Ray Camera::GetRay(int x, int y) const
{
    Ray ray;
    ray.o = mPosition;

    Vector4f screenPos(x, y, 0.0f, 1.0f);
    Vector4f worldPos = mInverseCombinedMatrix * screenPos;
    worldPos /= worldPos.w; // Normalize the homogeneous coordinate
    ray.d = glm::normalize(Vector3f(worldPos) - mPosition);

    return ray;
}
