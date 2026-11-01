#include "SceneGpuAdapter.h"
#include "Triangle.h"
#include "Sphere.h"
#include "Disk.h"
#include <limits>
#include <stdexcept>

GpuSceneData BuildGpuScene(const Scene& scene, int sphereSlices, int sphereStacks, int diskSegments)
{
    if (sphereSlices < 3 || sphereStacks < 2 || diskSegments < 3)
        throw std::invalid_argument("Invalid GPU mesh tessellation");
    GpuSceneData result;
    for (const SceneObject* object : scene.GetSceneObjects())
    {
        Color albedo(0.8f); // Legacy XML objects may have no material.
        if (const Material* material = object->GetMaterial())
        {
            const auto* lambert = dynamic_cast<const LambertMaterial*>(material);
            if (!lambert)
                throw std::runtime_error("OpenGL currently supports LambertMaterial only");
            albedo = lambert->GetAlbedo();
        }
        const Matrix4x4 model = object->GetObjectToWorld();
        const Matrix3x3 normalMatrix = glm::transpose(glm::inverse(Matrix3x3(model)));
        auto append = [&](const Vector3f& p, const Vector3f& n)
        {
            result.vertices.push_back({Vector3f(model * Vector4f(p, 1)),
                glm::normalize(normalMatrix * n), albedo});
        };
        for (const Primitive* primitive : object->GetPrimitives())
        {
            if (const auto* triangle = dynamic_cast<const Triangle*>(primitive))
            {
                // The CPU Triangle constructor has already applied the model matrix.
                for (int i = 0; i < 3; ++i)
                    result.vertices.push_back({triangle->GetVertex(i), triangle->GetNormal(), albedo});
            }
            else if (const auto* sphere = dynamic_cast<const Sphere*>(primitive))
            {
                auto normal = [&](int row, int col)
                {
                    const float theta = PI * static_cast<float>(row) / sphereStacks;
                    const float phi = 2 * PI * static_cast<float>(col) / sphereSlices;
                    return Vector3f(std::sin(theta) * std::cos(phi), std::cos(theta),
                        std::sin(theta) * std::sin(phi));
                };
                auto vertex = [&](int row, int col)
                {
                    const Vector3f n = normal(row, col);
                    append(n * sphere->GetRadius(), n);
                };
                for (int row = 0; row < sphereStacks; ++row)
                    for (int col = 0; col < sphereSlices; ++col)
                    {
                        // Omit degenerate triangles at the poles; outward winding.
                        if (row != 0)
                        {
                            vertex(row, col); vertex(row, col + 1); vertex(row + 1, col);
                        }
                        if (row != sphereStacks - 1)
                        {
                            vertex(row, col + 1); vertex(row + 1, col + 1); vertex(row + 1, col);
                        }
                    }
            }
            else if (const auto* disk = dynamic_cast<const Disk*>(primitive))
            {
                for (int i = 0; i < diskSegments; ++i)
                {
                    const float a = 2 * PI * static_cast<float>(i) / diskSegments;
                    const float b = 2 * PI * static_cast<float>(i + 1) / diskSegments;
                    append(Vector3f(0), Vector3f(0, 0, 1));
                    append(Vector3f(std::cos(a), std::sin(a), 0) * disk->GetRadius(), Vector3f(0, 0, 1));
                    append(Vector3f(std::cos(b), std::sin(b), 0) * disk->GetRadius(), Vector3f(0, 0, 1));
                }
            }
            else
                throw std::runtime_error("Unsupported primitive in OpenGL scene");
        }
    }
    if (!result.vertices.empty())
    {
        result.boundsMin = Vector3f(std::numeric_limits<float>::max());
        result.boundsMax = Vector3f(std::numeric_limits<float>::lowest());
        for (const auto& vertex : result.vertices)
        {
            result.boundsMin = glm::min(result.boundsMin, vertex.position);
            result.boundsMax = glm::max(result.boundsMax, vertex.position);
        }
    }
    for (const Light* light : scene.GetLights())
    {
        GpuLight data;
        if (const auto* directional = dynamic_cast<const DirectionalLight*>(light))
        {
            data.type = GpuLightType::Directional;
            data.direction = directional->GetDirection();
            data.color = directional->GetColor();
        }
        else if (const auto* point = dynamic_cast<const PointLight*>(light))
        {
            data.type = GpuLightType::Point;
            data.position = point->GetPosition();
            data.color = point->GetColor();
            data.attenuations = point->GetAttenuations();
        }
        else if (const auto* spot = dynamic_cast<const SpotLight*>(light))
        {
            data.type = GpuLightType::Spot;
            data.position = spot->GetPosition();
            data.direction = spot->GetDirection();
            data.color = spot->GetColor();
            data.attenuations = spot->GetAttenuations();
            data.cosInner = spot->GetCosInnerAngle();
            data.cosOuter = spot->GetCosOuterAngle();
        }
        else
            throw std::runtime_error("Unsupported light in OpenGL scene");
        result.lights.push_back(data);
    }
    return result;
}
