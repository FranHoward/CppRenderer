#include "Sphere.h"
#include "SceneObject.h"

Sphere::Sphere(SceneObject* pSceneObject, float radius)
    : Primitive(pSceneObject), mRadius(radius)
{

}

bool Sphere::Intersect(const Ray& ray, Intersection& hit) const
{
    // Transform the ray to object space
    Ray r = m_pSceneObject->GetWorldToObject() * ray; // Transform the ray to object space
    
    float a = glm::dot(r.d, r.d);
    float b = 2.0f * glm::dot(r.o, r.d);
    float c = glm::dot(r.o, r.o) - mRadius * mRadius;

    float discriminant = b * b - 4 * a * c;
    if (discriminant < 0)
        return false;

    float sqrtDiscriminant = glm::sqrt(discriminant);
    float t1 = (-b - sqrtDiscriminant) / (2.0f * a);
    float t2 = (-b + sqrtDiscriminant) / (2.0f * a);

    if (t1 > r.maxt || t2 < r.mint)
        return false;
    
    float t = t1;
    if (t < r.mint)
    {
        t = t2;
        if (t > r.maxt)
            return false;
    }
    
    Vector3f position = r.o + t * r.d;
    Vector3f normal = glm::normalize(position);

    hit.position = Vector3f(m_pSceneObject->GetObjectToWorld() * Vector4f(position, 1.0f));
    hit.normal = glm::normalize(Vector3f(m_pSceneObject->GetObjectToWorld() * Vector4f(normal, 0.0f)));
    hit.t = t;

    return true;
}