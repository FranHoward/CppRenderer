#include "Disk.h"
#include "SceneObject.h"

Disk::Disk(SceneObject* pSceneObject, float radius)
    : Primitive(pSceneObject), mRadius(radius)
{

}

bool Disk::Intersect(const Ray& ray, Intersection& hit) const
{
    // Transform the ray to object space
    Ray r =  m_pSceneObject->GetWorldToObject() * ray; // Transform the ray to object space

    // Check if the ray intersects the disk's plane (z = 0 in object space)
    if (fabs(r.d.z) < 1e-6f)
        return false; // Ray is parallel to the disk's plane

    float t = -r.o.z / r.d.z;
    if (t < r.mint || t > r.maxt)
        return false; // Intersection is outside the valid range

    Vector3f position = r.o + t * r.d;

    // Check if the intersection point is within the disk's radius
    if (glm::dot(position, position) > mRadius * mRadius)
        return false; // Intersection point is outside the disk's radius

    hit.position = Vector3f( m_pSceneObject->GetObjectToWorld() * Vector4f(position, 1.0f));
    hit.normal = glm::normalize(Vector3f( m_pSceneObject->GetObjectToWorld() * Vector4f(0.0f, 0.0f,1.0f, 0.0f)));
    hit.t = t;

    return true;
}