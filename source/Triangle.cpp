#include "Triangle.h"
#include "SceneObject.h"

Triangle::Triangle(SceneObject* pSceneObject, const Vector3f& v0, const Vector3f& v1, const Vector3f& v2)
    : Primitive(pSceneObject)
{
    Matrix4x4 worldMatrix = pSceneObject->GetObjectToWorld();

    mVertices[0] = Vector3f(worldMatrix * Vector4f(v0, 1.0f));
    mVertices[1] = Vector3f(worldMatrix * Vector4f(v1, 1.0f));
    mVertices[2] = Vector3f(worldMatrix * Vector4f(v2, 1.0f));

    // Calculate the normal of the triangle
    mNormal = glm::normalize(glm::cross(mVertices[1] - mVertices[0], mVertices[2] - mVertices[0]));
}

bool Triangle::Intersect(const Ray& ray, Intersection& hit) const
{
    Vector3f p0 = mVertices[0];
    Vector3f p1 = mVertices[1];
    Vector3f p2 = mVertices[2];

    Vector3f e1 = p1 - p0;
    Vector3f e2 = p2 - p0;
    Vector3f s = ray.o - p0;

    Vector3f s1 = glm::cross(ray.d, e2);
    Vector3f s2 = glm::cross(s, e1);

    float det = glm::dot(s1, e1);
    if (fabs(det) < 1e-6f)
        return false; // Ray is parallel to the triangle
    
    float invDet = 1.0f / glm::dot(s1, e1);
    float b1 = glm::dot(s1, s) * invDet;
    float b2 = glm::dot(s2, ray.d) * invDet;
    float t = glm::dot(s2, e2) * invDet;

    if (t < ray.mint || t > ray.maxt || b1 < 0.0f || b2 < 0.0f || (b1 + b2) > 1.0f)
        return false; // Intersection is outside the triangle or outside the valid range

    hit.position = ray.o + t * ray.d;
    hit.normal = mNormal;
    hit.t = t;

    return true;
}