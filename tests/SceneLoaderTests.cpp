#include "Scene.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>

namespace
{
    void Require(bool condition, const std::string& message)
    {
        if (!condition)
            throw std::runtime_error(message);
    }

    bool Near(float a, float b) { return std::fabs(a - b) < 0.0001f; }

    void CheckHit(const Scene& scene, const Vector3f& origin,
        const Vector3f& direction, float distance)
    {
        Ray ray;
        ray.o = origin;
        ray.d = glm::normalize(direction);
        Intersection hit;
        Require(scene.Intersect(ray, hit) != nullptr, "Expected a hit");
        Require(Near(hit.t, distance), "Unexpected hit distance");
        Require(Near(glm::length(hit.normal), 1.0f), "Expected a unit normal");
    }

    const std::string camera =
        R"(<Camera position="0 0 -5" target="0 0 1" up="0 1 0" fov="45" near="0.1" far="100"/>)";
}

int main(int argc, char** argv)
{
    if (argc != 5)
        return 2;
    const std::string fixture = argv[2];
    try
    {
        auto scene = std::unique_ptr<Scene>(Scene::LoadSceneFromXML(argv[1], 800, 600));
        const Ray center = scene->GetCamera().GetRay(400.0f, 300.0f);
        Require(Near(center.o.z, -5.0f) && Near(center.d.z, 1.0f), "Camera parameters changed");
        CheckHit(*scene, center.o, center.d, 6.5f);
        CheckHit(*scene, Vector3f(1.5f, -1.5f, -5.0f), Vector3f(0, 0, 1), 10.0f);
        CheckHit(*scene, Vector3f(-1.5f, 1.5f, -5.0f), Vector3f(0, 0, 1), 10.0f);
        Ray miss;
        miss.o = Vector3f(2.1f, 0, -5);
        miss.d = Vector3f(0, 0, 1);
        Intersection hit;
        Require(!scene->Intersect(miss, hit), "Square scale changed");

        auto load = [&](const std::string& xml)
        {
            { std::ofstream output(fixture); output << xml; Require(output.good(), "Cannot write fixture"); }
            return std::unique_ptr<Scene>(Scene::LoadSceneFromXML(fixture.c_str(), 800, 600));
        };
        auto reject = [&](const std::string& xml)
        {
            try { auto invalid = load(xml); }
            catch (const std::runtime_error& error)
            {
                Require(std::string(error.what()).find(fixture) != std::string::npos,
                    "Failure must identify the XML file");
                return;
            }
            throw std::runtime_error("Invalid XML scene was accepted: " + xml);
        };

        // Defaults, degree-based rotation, scaling and the disk primitive.
        scene = load("<Scene>" + camera
            + R"(<SceneObject position="1 0 2" rotation="0 90 0" scale="2"><Disk radius="0.5"/></SceneObject>)"
            + R"(<SceneObject><Sphere radius="0.5"/></SceneObject></Scene>)");
        CheckHit(*scene, Vector3f(5, 0, 2), Vector3f(-1, 0, 0), 4.0f);
        CheckHit(*scene, Vector3f(0, 0, -5), Vector3f(0, 0, 1), 4.5f);
        scene = load("<Scene>" + camera + "</Scene>");
        Require(!scene->Intersect(miss, hit), "Expected an empty scene");
        Require(scene->GetLights().empty(), "Legacy scene should have no lights");

        const std::string lights =
            R"(<DirectionalLight direction="0 -2 0" radiance="1 2 3"/>)"
            R"(<PointLight position="0 2 0" intensity="8 4 2" attenuations="1 0 0"/>)"
            R"(<SpotLight position="0 2 0" direction="0 -2 0" intensity="8 4 2" attenuations="1 0 0" innerAngle="15" outerAngle="45"/>)";
        for (bool grouped : {false, true})
        {
            scene = load("<Scene>" + camera + (grouped ? "<Lights>" : "") + lights
                + (grouped ? "</Lights>" : "") + "</Scene>");
            const auto& loaded = scene->GetLights();
            Require(loaded.size() == 3, "Expected three lights");
            Require(dynamic_cast<DirectionalLight*>(loaded[0])
                && dynamic_cast<PointLight*>(loaded[1]) && dynamic_cast<SpotLight*>(loaded[2]),
                "Light types or ordering changed");
            Vector3f source;
            const Color directional = loaded[0]->GetRadiance(Vector3f(0), source);
            Require(Near(directional.z, 3) && Near(source.y, 100000), "Directional light parameters changed");
            const Color point = loaded[1]->GetRadiance(Vector3f(0), source);
            Require(Near(point.x, 2) && Near(point.y, 1) && Near(source.y, 2), "Point attenuation changed");
            const Color spot = loaded[2]->GetRadiance(Vector3f(0), source);
            Require(Near(spot.x, 2) && Near(spot.z, 0.5f) && Near(source.y, 2), "Spot axis should be illuminated");
            Require(Near(glm::length(loaded[2]->GetRadiance(Vector3f(0, 4, 0), source)), 0),
                "Behind spotlight should be dark");
            const float angle = glm::radians(30.0f);
            const Color penumbra = loaded[2]->GetRadiance(Vector3f(2 * std::sin(angle), 2 - 2 * std::cos(angle), 0), source);
            const float blend = (std::cos(angle) - std::cos(glm::radians(45.0f)))
                / (std::cos(glm::radians(15.0f)) - std::cos(glm::radians(45.0f)));
            Require(Near(penumbra.x, 2 * blend), "Spot cone falloff changed");
        }

        scene.reset(Scene::LoadSceneFromXML(argv[3], 800, 600));
        Require(scene->GetLights().size() == 1
            && dynamic_cast<PointLight*>(scene->GetLights()[0]),
            "Cornell box must contain exactly one point light");
        CheckHit(*scene, Vector3f(0, 0, -6.5f), Vector3f(0, 0, 1), 10.5f);
        CheckHit(*scene, Vector3f(-0.95f, -1.25f, -1), Vector3f(0, 0, 1), 1.3f);
        CheckHit(*scene, Vector3f(0.9f, -1.5f, -1), Vector3f(0, 0, 1), 1.2f);
        CheckHit(*scene, Vector3f(0.65f, 1, 2.8f), Vector3f(0, -1, 0), 1.2f);
        const Vector3f inside(0, 0, 2);
        for (const Vector3f direction : {Vector3f(-1,0,0), Vector3f(1,0,0), Vector3f(0,1,0), Vector3f(0,-1,0), Vector3f(0,0,1)})
        {
            CheckHit(*scene, inside, direction, 2);
            Ray wallRay;
            wallRay.o = inside;
            wallRay.d = direction;
            scene->Intersect(wallRay, hit);
            Require(glm::dot(hit.normal, direction) < -0.99f, "Cornell walls must face inward");
        }

        for (const std::string invalid : {
            R"(<DirectionalLight direction="0 0 0" radiance="1 1 1"/>)",
            R"(<DirectionalLight direction="0 -1 0" radiance="-1 1 1"/>)",
            R"(<PointLight position="0 0 0" intensity="1 1 1" attenuations="0 0 0"/>)",
            R"(<PointLight position="0 0 0" intensity="1 1 1" attenuations="-1 0 1"/>)",
            R"(<PointLight position="0 0 0" intensity="1 1 1"/>)",
            R"(<SpotLight position="0 1 0" direction="0 -1 0" intensity="1 1 1" attenuations="1 0 1" innerAngle="30" outerAngle="30"/>)",
            R"(<SpotLight position="0 1 0" direction="0 -1 0" intensity="1 1 1" attenuations="1 0 1" innerAngle="-1" outerAngle="45"/>)",
            R"(<SpotLight position="0 1 0" direction="0 -1 0" intensity="1 1 1" attenuations="1 0 1" innerAngle="20" outerAngle="180"/>)",
            "<AreaLight/>", "<PointLight><Sphere radius=\"1\"/></PointLight>"})
            reject("<Scene>" + camera + "<Lights>" + invalid + "</Lights></Scene>");
        reject("<Scene>" + camera + "<SceneObjects><PointLight/></SceneObjects></Scene>");

        // Materials are shared by name and may be defined after their objects.
        const std::string red = R"(<LambertMaterial name="red" albedo="0.8 0.1 0.05"/>)";
        for (bool grouped : {false, true})
        {
            scene = load("<Scene>" + camera
                + R"(<SceneObject position="-1 0 1" material="red"><Sphere radius="0.5"/></SceneObject>)"
                + R"(<SceneObject position="1 0 1" material="red"><Sphere radius="0.5"/></SceneObject>)"
                + (grouped ? "<Materials>" : "") + red + (grouped ? "</Materials>" : "") + "</Scene>");
            Material* material = scene->GetMaterial("red");
            Require(dynamic_cast<LambertMaterial*>(material), "Expected a Lambert material");
            Require(!scene->GetMaterial("missing"), "Unknown material lookup must return null");
            for (float x : {-1.0f, 1.0f})
            {
                Ray ray;
                ray.o = Vector3f(x, 0, -5);
                ray.d = Vector3f(0, 0, 1);
                const SceneObject* object = scene->Intersect(ray, hit);
                Require(object && object->GetMaterial() == material, "Material reference must be shared");
            }
            const Color response = material->brdf(Vector3f(0, 0, 1), Vector3f(0, 1, 0));
            Require(Near(response.r, 0.8f * INV_PI) && Near(response.g, 0.1f * INV_PI)
                && Near(response.b, 0.05f * INV_PI), "Lambert BRDF must equal albedo / pi");
        }
        for (const std::string invalid : {
            R"(<LambertMaterial albedo="1 1 1"/>)",
            R"(<LambertMaterial name="" albedo="1 1 1"/>)",
            R"(<LambertMaterial name="   " albedo="1 1 1"/>)",
            R"(<LambertMaterial name="red"/>)",
            R"(<LambertMaterial name="red" albedo="-0.1 0 0"/>)",
            R"(<LambertMaterial name="red" albedo="1.1 0 0"/>)",
            R"(<LambertMaterial name="red" albedo="nan 0 0"/>)",
            R"(<LambertMaterial name="red" albedo="1 0"/>)",
            R"(<LambertMaterial name="red" albedo="1 0 0oops"/>)",
            R"(<LambertMaterial name="red" albedo="1 0 0" roughness="1"/>)",
            R"(<LambertMaterial name="red" albedo="1 0 0"><Sphere radius="1"/></LambertMaterial>)",
            "<MetalMaterial/>"})
            reject("<Scene>" + camera + "<Materials>" + invalid + "</Materials></Scene>");
        reject("<Scene>" + camera + red + "<Materials>" + red + "</Materials></Scene>");
        reject("<Scene>" + camera + R"(<SceneObject material="missing"><Sphere radius="1"/></SceneObject></Scene>)");
        reject("<Scene>" + camera + R"(<Materials typo="1"/></Scene>)");

        scene.reset(Scene::LoadSceneFromXML(argv[4], 800, 600));
        Require(scene->GetLights().size() == 1, "Scene02 must have a test light");
        for (float x : {-1.05f, 1.05f})
        {
            Ray ray;
            ray.o = Vector3f(x, 0, -6);
            ray.d = Vector3f(0, 0, 1);
            const SceneObject* object = scene->Intersect(ray, hit);
            Material* expected = scene->GetMaterial(x < 0 ? "RedLambert" : "BlueLambert");
            Require(expected && object && object->GetMaterial() == expected,
                "Scene02 sphere must use its named material");
            CheckHit(*scene, ray.o, ray.d, 6.15f);
            const Color brdf = expected->brdf(-ray.d, -ray.d);
            Require(x < 0 ? brdf.r > brdf.b : brdf.b > brdf.r,
                "Scene02 spheres must have different red and blue responses");
        }
        Ray gap;
        gap.o = Vector3f(0, 0, -6);
        gap.d = Vector3f(0, 0, 1);
        Require(!scene->Intersect(gap, hit), "Scene02 spheres must be separated");

        reject("<Scene>");
        reject("<Other/>");
        reject("<Scene/>");
        reject("<Scene>" + camera + camera + "</Scene>");
        reject("<Scene>" + camera + "<SceneObject/></Scene>");
        reject("<Scene>" + camera + "<SceneObject><Cube/></SceneObject></Scene>");
        reject("<Scene>" + camera + R"(<SceneObject scale="0"><Sphere radius="1"/></SceneObject></Scene>)");
        reject("<Scene>" + camera + R"(<SceneObject><Sphere radius="-1"/></SceneObject></Scene>)");
        reject("<Scene>" + camera + R"(<SceneObject><Sphere radius="1oops"/></SceneObject></Scene>)");
        reject("<Scene>" + camera + R"(<SceneObject><Sphere/></SceneObject></Scene>)");
        reject("<Scene>" + camera + R"(<SceneObject position="1 2"><Sphere radius="1"/></SceneObject></Scene>)");
        reject("<Scene>" + camera + R"(<SceneObject positon="1 2 3"><Sphere radius="1"/></SceneObject></Scene>)");
        reject("<Scene>" + camera + R"(<SceneObject><Triangle v0="0 0 0" v1="1 0 0" v2="2 0 0"/></SceneObject></Scene>)");
        reject(R"(<Scene><Camera position="0 0 0" target="0 0 0" up="0 1 0" fov="45" near="0.1" far="100"/></Scene>)");
        reject(R"(<Scene><Camera position="0 0 -5" target="0 0 1" up="0 1 0" fov="180" near="0.1" far="100"/></Scene>)");
        reject(R"(<Scene><Camera position="0 0 -5" target="0 0 1" up="0 1 0" fov="45" near="100" far="1"/></Scene>)");
        reject("<Scene>" + camera + "</Scene><Scene/>");

        std::remove(fixture.c_str());
        try { scene.reset(Scene::LoadSceneFromXML(fixture.c_str(), 800, 600)); return 1; }
        catch (const std::runtime_error&) {}
        try { scene.reset(Scene::LoadSceneFromXML(nullptr, 800, 600)); return 1; }
        catch (const std::invalid_argument&) {}
        try { scene.reset(Scene::LoadSceneFromXML(argv[1], 0, 600)); return 1; }
        catch (const std::invalid_argument&) {}
        std::cout << "SceneLoader: geometry, camera, transforms, lights, materials, sample scenes and invalid input checks passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::remove(fixture.c_str());
        std::cerr << error.what() << '\n';
        return 1;
    }
}
