#include "SceneGpuAdapter.h"
#include "Triangle.h"
#include "Sphere.h"
#include "Disk.h"
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace
{
    void Require(bool value, const char* message)
    { if (!value) throw std::runtime_error(message); }
    bool Near(float a, float b) { return std::abs(a - b) < 0.0001f; }
    bool Near(const Vector3f& a, const Vector3f& b) { return glm::length(a - b) < 0.0001f; }
}

int main(int argc, char** argv)
{
    try
    {
        Require(argc == 2, "Expected CornellBox scene path");
        Scene scene;
        Camera camera;
        camera.Initialize(Vector3f(2, 3, -5), Vector3f(2, 3, 0), Vector3f(0, 1, 0), 55, 0.2f, 80, 800, 600);
        scene.SetCamera(camera);
        Require(Near(camera.GetPosition(), Vector3f(2, 3, -5)) && Near(camera.GetFov(), 55), "Camera metadata mismatch");
        Require(Near(camera.GetRay(400, 300).d, Vector3f(0, 0, 1)), "Legacy camera ray changed");
        Require(Near(camera.GetNearPlane(), 0.2f) && Near(camera.GetFarPlane(), 80), "Camera clip metadata mismatch");
        const Matrix4x4 vp = glm::perspectiveLH_NO(glm::radians(camera.GetFov()), 800.0f / 600,
            camera.GetNearPlane(), camera.GetFarPlane()) *
            glm::lookAtLH(camera.GetPosition(), camera.GetTarget(), camera.GetUp());
        for (const Vector2f pixel : {Vector2f(0, 0), Vector2f(800, 600), Vector2f(173, 429)})
        {
            const Ray sample = camera.GetRay(pixel.x, pixel.y);
            const Vector4f clip = vp * Vector4f(sample.o + sample.d * 5.0f, 1);
            const Vector2f screen((clip.x / clip.w + 1) * 400, (1 - clip.y / clip.w) * 300);
            Require(glm::length(screen - pixel) < 0.02f, "OpenGL projection disagrees with CPU camera rays");
        }
        // Compare actual CPU intersections to uploaded world positions; catches double transforms.
        auto* triangleObject = scene.CreateSceneObject(Vector3f(3, 4, 5), Vector3f(0, PI / 4, 0), 2);
        triangleObject->CreatePrimitive<Triangle>(Vector3f(0), Vector3f(1, 0, 0), Vector3f(0, 1, 0));
        auto* material = scene.CreateMaterial<LambertMaterial>("red", Color(0.7f, 0.1f, 0.05f));
        triangleObject->SetMaterial(material);
        auto* sphereObject = scene.CreateSceneObject(Vector3f(-3, 1, 2), Vector3f(0), 2);
        sphereObject->CreatePrimitive<Sphere>(0.5f);
        auto* diskObject = scene.CreateSceneObject(Vector3f(1, -2, 3), Vector3f(PI / 2, 0, 0), 3);
        diskObject->CreatePrimitive<Disk>(0.5f);
        auto* direction = scene.CreateLight<DirectionalLight>(Vector3f(1, -2, 3), Color(2, 3, 4));
        auto* point = scene.CreateLight<PointLight>(Vector3f(3, 5, 7), Color(20), Vector3f(1, 2, 3));
        auto* spot = scene.CreateLight<SpotLight>(Vector3f(-2, 3, 0), Vector3f(0, -1, 0), Color(10), Vector3f(1, 0, 1), 20, 40);
        const auto gpu = BuildGpuScene(scene, 16, 8, 16);
        const size_t sphereVertices = 6 * 16 * (8 - 1);
        Require(gpu.vertices.size() == 3 + sphereVertices + 3 * 16, "Mesh vertex count mismatch");
        const auto model = triangleObject->GetObjectToWorld();
        Require(Near(gpu.vertices[0].position, Vector3f(model * Vector4f(0, 0, 0, 1))), "Triangle transformed twice");
        Require(Near(gpu.vertices[1].position, Vector3f(model * Vector4f(1, 0, 0, 1))), "Triangle transform mismatch");
        Require(Near(gpu.vertices[0].albedo, material->GetAlbedo()), "Material color not preserved");
        const Vector3f centroid = (gpu.vertices[0].position + gpu.vertices[1].position + gpu.vertices[2].position) / 3.0f;
        const Vector3f normal = gpu.vertices[0].normal;
        Ray ray;
        ray.o = centroid + normal * 2.0f;
        ray.d = -normal;
        Intersection hit;
        Require(triangleObject->Intersect(ray, hit) && Near(hit.position, centroid) && Near(hit.normal, normal),
            "GPU triangle disagrees with CPU intersection");
        for (size_t i = 3; i < 3 + sphereVertices; ++i)
        {
            const auto& vertex = gpu.vertices[i];
            const Vector3f radial = vertex.position - Vector3f(-3, 1, 2);
            Require(Near(glm::length(radial), 1), "Sphere radius/scale mismatch");
            Require(Near(vertex.normal, glm::normalize(radial)), "Sphere normal mismatch");
        }
        for (size_t i = 3; i < 3 + sphereVertices; i += 3)
        {
            const auto a = gpu.vertices[i], b = gpu.vertices[i + 1], c = gpu.vertices[i + 2];
            Require(glm::dot(glm::cross(b.position - a.position, c.position - a.position), a.normal + b.normal + c.normal) > 0,
                "Sphere triangle is degenerate or wound inward");
        }
        for (size_t i = 3 + sphereVertices; i < gpu.vertices.size(); ++i)
        {
            const auto& vertex = gpu.vertices[i];
            Require(Near(vertex.position.y, -2), "Disk plane/rotation mismatch");
            Require(glm::length(vertex.position - Vector3f(1, -2, 3)) <= 1.5001f, "Disk radius/scale mismatch");
            Require(Near(vertex.normal, Vector3f(0, -1, 0)), "Disk normal transform mismatch");
        }
        for (const auto& vertex : gpu.vertices)
            Require(Near(glm::length(vertex.normal), 1), "Non-unit mesh normal");
        Require(gpu.lights.size() == 3 && gpu.lights[0].type == GpuLightType::Directional &&
            gpu.lights[1].type == GpuLightType::Point && gpu.lights[2].type == GpuLightType::Spot, "Light type/order mismatch");
        Require(Near(gpu.lights[0].direction, direction->GetDirection()), "Directional light direction mismatch");
        Vector3f source;
        const Vector3f p(0, 0, 0);
        const float distance = glm::length(gpu.lights[1].position - p);
        const auto att = gpu.lights[1].attenuations;
        Require(Near(gpu.lights[1].color / (att.x * distance * distance + att.y * distance + att.z),
            point->GetRadiance(p, source)), "Point light attenuation order mismatch");
        Require(Near(gpu.lights[2].cosInner, spot->GetCosInnerAngle()) &&
            Near(gpu.lights[2].cosOuter, spot->GetCosOuterAngle()), "Spotlight cone mismatch");
        const auto bundled = std::unique_ptr<Scene>(Scene::LoadSceneFromXML(argv[1], 800, 600));
        const auto cornell = BuildGpuScene(*bundled);
        Require(!cornell.vertices.empty() && cornell.lights.size() == 1, "CornellBox conversion failed");
        Scene empty;
        Require(BuildGpuScene(empty).vertices.empty(), "Empty scene conversion failed");
        bool rejected = false;
        try { BuildGpuScene(empty, 2); } catch (const std::invalid_argument&) { rejected = true; }
        Require(rejected, "Invalid tessellation accepted");
        std::cout << "Scene GPU adapter tests passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
