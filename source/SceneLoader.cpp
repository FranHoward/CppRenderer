#include "Scene.h"
#include "Disk.h"
#include "Sphere.h"
#include "Triangle.h"
#include <tinyxml2.h>

#include <cmath>
#include <initializer_list>
#include <locale>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

namespace
{
    using tinyxml2::XMLElement;

    [[noreturn]] void Fail(const XMLElement& element, const std::string& message)
    {
        throw std::runtime_error("line " + std::to_string(element.GetLineNum())
            + " <" + element.Name() + ">: " + message);
    }

    void CheckAttributes(const XMLElement& element,
        std::initializer_list<const char*> allowed)
    {
        for (auto* attribute = element.FirstAttribute(); attribute; attribute = attribute->Next())
        {
            bool known = false;
            for (const char* name : allowed)
                known = known || std::string(attribute->Name()) == name;
            if (!known)
                Fail(element, "unknown attribute '" + std::string(attribute->Name()) + "'");
        }
    }

    const char* RequireAttribute(const XMLElement& element, const char* name)
    {
        const char* value = element.Attribute(name);
        if (!value)
            Fail(element, "missing attribute '" + std::string(name) + "'");
        return value;
    }

    float ReadFloat(const XMLElement& element, const char* name)
    {
        std::istringstream input(RequireAttribute(element, name));
        input.imbue(std::locale::classic());
        float value = 0.0f;
        if (!(input >> value) || !std::isfinite(value))
            Fail(element, "attribute '" + std::string(name) + "' must be a finite number");
        input >> std::ws;
        if (!input.eof())
            Fail(element, "unexpected characters in attribute '" + std::string(name) + "'");
        return value;
    }

    Vector3f ReadVector(const XMLElement& element, const char* name)
    {
        std::istringstream input(RequireAttribute(element, name));
        input.imbue(std::locale::classic());
        Vector3f value(0.0f);
        if (!(input >> value.x >> value.y >> value.z)
            || !std::isfinite(value.x) || !std::isfinite(value.y) || !std::isfinite(value.z))
        {
            Fail(element, "attribute '" + std::string(name) + "' must contain three finite numbers");
        }
        input >> std::ws;
        if (!input.eof())
            Fail(element, "unexpected characters in attribute '" + std::string(name) + "'");
        return value;
    }

    void RequireLeaf(const XMLElement& element)
    {
        if (element.FirstChildElement())
            Fail(element, "child elements are not supported");
    }

    Camera LoadCamera(const XMLElement& element, int w, int h)
    {
        CheckAttributes(element, {"position", "target", "up", "fov", "near", "far"});
        RequireLeaf(element);
        const Vector3f position = ReadVector(element, "position");
        const Vector3f target = ReadVector(element, "target");
        const Vector3f up = ReadVector(element, "up");
        const float fov = ReadFloat(element, "fov");
        const float nearPlane = ReadFloat(element, "near");
        const float farPlane = ReadFloat(element, "far");
        const Vector3f direction = target - position;
        const Vector3f right = glm::cross(direction, up);
        if (glm::dot(direction, direction) <= 0.0f || glm::dot(right, right) <= 0.0f)
            Fail(element, "position, target and up must define a valid camera basis");
        if (fov <= 0.0f || fov >= 180.0f || nearPlane <= 0.0f || farPlane <= nearPlane)
            Fail(element, "require 0 < fov < 180 and 0 < near < far");

        Camera camera;
        camera.Initialize(position, target, up, fov, nearPlane, farPlane,
            static_cast<float>(w), static_cast<float>(h));
        return camera;
    }

    void LoadLight(Scene& scene, const XMLElement& element)
    {
        const std::string type = element.Name();
        RequireLeaf(element);
        if (type == "DirectionalLight")
            CheckAttributes(element, {"direction", "radiance"});
        else if (type == "PointLight")
            CheckAttributes(element, {"position", "intensity", "attenuations"});
        else if (type == "SpotLight")
            CheckAttributes(element, {"position", "direction", "intensity", "attenuations", "innerAngle", "outerAngle"});
        else
            Fail(element, "unsupported light type");

        const Vector3f color = ReadVector(element, type == "DirectionalLight" ? "radiance" : "intensity");
        if (color.x < 0.0f || color.y < 0.0f || color.z < 0.0f)
            Fail(element, "light color components must be nonnegative");

        Vector3f direction(0.0f);
        if (type != "PointLight")
        {
            direction = ReadVector(element, "direction");
            const float lengthSquared = glm::dot(direction, direction);
            if (!std::isfinite(lengthSquared) || lengthSquared <= 0.0f)
                Fail(element, "direction must have a finite, positive length");
        }
        if (type == "DirectionalLight")
        {
            scene.CreateLight<DirectionalLight>(direction, color);
            return;
        }

        const Vector3f position = ReadVector(element, "position");
        // Coefficients are ordered quadratic, linear, constant, matching GetRadiance.
        const Vector3f attenuations = ReadVector(element, "attenuations");
        if (attenuations.x < 0.0f || attenuations.y < 0.0f || attenuations.z < 0.0f
            || (attenuations.x == 0.0f && attenuations.y == 0.0f && attenuations.z == 0.0f))
            Fail(element, "attenuations must be nonnegative and not all zero");
        if (type == "PointLight")
            scene.CreateLight<PointLight>(position, color, attenuations);
        else
        {
            const float innerAngle = ReadFloat(element, "innerAngle");
            const float outerAngle = ReadFloat(element, "outerAngle");
            if (innerAngle < 0.0f || innerAngle >= outerAngle || outerAngle >= 180.0f
                || std::cos(glm::radians(innerAngle)) <= std::cos(glm::radians(outerAngle)))
                Fail(element, "require 0 <= innerAngle < outerAngle < 180 with distinct cone cosines");
            scene.CreateLight<SpotLight>(position, direction, color, attenuations, innerAngle, outerAngle);
        }
    }

    void LoadMaterial(Scene& scene, const XMLElement& element)
    {
        if (std::string(element.Name()) != "LambertMaterial")
            Fail(element, "unsupported material type");
        CheckAttributes(element, {"name", "albedo"});
        RequireLeaf(element);
        const std::string name = RequireAttribute(element, "name");
        if (name.find_first_not_of(" \t\r\n") == std::string::npos)
            Fail(element, "material name must not be empty");
        if (scene.GetMaterial(name))
            Fail(element, "duplicate material name '" + name + "'");
        const Color albedo = ReadVector(element, "albedo");
        if (albedo.x < 0.0f || albedo.x > 1.0f || albedo.y < 0.0f || albedo.y > 1.0f
            || albedo.z < 0.0f || albedo.z > 1.0f)
            Fail(element, "albedo components must be in [0, 1]");
        scene.CreateMaterial<LambertMaterial>(name, albedo);
    }

    void LoadObject(Scene& scene, const XMLElement& element)
    {
        CheckAttributes(element, {"position", "rotation", "scale", "material"});
        const Vector3f position = element.Attribute("position")
            ? ReadVector(element, "position") : Vector3f(0.0f);
        // XML rotations use degrees; MakeWorldTransform expects radians.
        const Vector3f rotation = element.Attribute("rotation")
            ? glm::radians(ReadVector(element, "rotation")) : Vector3f(0.0f);
        const float scale = element.Attribute("scale") ? ReadFloat(element, "scale") : 1.0f;
        if (scale <= 0.0f)
            Fail(element, "scale must be positive");
        if (!element.FirstChildElement())
            Fail(element, "at least one primitive is required");

        SceneObject* object = scene.CreateSceneObject(position, rotation, scale);
        if (const char* name = element.Attribute("material"))
        {
            Material* material = scene.GetMaterial(name);
            if (!material)
                Fail(element, "unknown material '" + std::string(name) + "'");
            object->SetMaterial(material);
        }
        for (const XMLElement* primitive = element.FirstChildElement(); primitive;
            primitive = primitive->NextSiblingElement())
        {
            const std::string type = primitive->Name();
            RequireLeaf(*primitive);
            if (type == "Triangle")
            {
                CheckAttributes(*primitive, {"v0", "v1", "v2"});
                const Vector3f v0 = ReadVector(*primitive, "v0");
                const Vector3f v1 = ReadVector(*primitive, "v1");
                const Vector3f v2 = ReadVector(*primitive, "v2");
                const Vector3f normal = glm::cross(v1 - v0, v2 - v0);
                if (glm::dot(normal, normal) <= 0.0f)
                    Fail(*primitive, "triangle vertices must not be collinear");
                object->CreatePrimitive<Triangle>(v0, v1, v2);
            }
            else if (type == "Sphere" || type == "Disk")
            {
                CheckAttributes(*primitive, {"radius"});
                const float radius = ReadFloat(*primitive, "radius");
                if (radius <= 0.0f)
                    Fail(*primitive, "radius must be positive");
                if (type == "Sphere")
                    object->CreatePrimitive<Sphere>(radius);
                else
                    object->CreatePrimitive<Disk>(radius);
            }
            else
            {
                Fail(*primitive, "unsupported primitive type");
            }
        }
    }
}

// Implement the existing Scene factory here to keep XML parsing out of Scene.cpp.
Scene* Scene::LoadSceneFromXML(const char* filepath, int w, int h)
{
    if (!filepath || !*filepath)
        throw std::invalid_argument("Scene filepath must not be empty");
    if (w <= 0 || h <= 0)
        throw std::invalid_argument("Scene viewport dimensions must be positive");

    try
    {
        tinyxml2::XMLDocument document;
        if (document.LoadFile(filepath) != tinyxml2::XML_SUCCESS)
            throw std::runtime_error(document.ErrorStr());

        const XMLElement* root = document.RootElement();
        if (!root || std::string(root->Name()) != "Scene")
            throw std::runtime_error("expected a <Scene> root element");
        if (root->NextSiblingElement())
            Fail(*root, "only one root element is allowed");
        CheckAttributes(*root, {});

        // Clean up partially built scenes if parsing fails.
        auto scene = std::make_unique<Scene>();
        // Resolve all material definitions before binding objects, including forward references.
        for (const XMLElement* element = root->FirstChildElement(); element;
            element = element->NextSiblingElement())
        {
            const std::string type = element->Name();
            if (type == "LambertMaterial")
                LoadMaterial(*scene, *element);
            else if (type == "Materials")
            {
                CheckAttributes(*element, {});
                for (const XMLElement* child = element->FirstChildElement(); child;
                    child = child->NextSiblingElement())
                    LoadMaterial(*scene, *child);
            }
        }
        bool hasCamera = false;
        for (const XMLElement* element = root->FirstChildElement(); element;
            element = element->NextSiblingElement())
        {
            const std::string type = element->Name();
            if (type == "Materials" || type == "LambertMaterial")
                continue; // Already loaded in the material pass.
            if (type == "Camera")
            {
                if (hasCamera)
                    Fail(*element, "only one camera is allowed");
                scene->SetCamera(LoadCamera(*element, w, h));
                hasCamera = true;
            }
            else if (type == "SceneObject")
            {
                LoadObject(*scene, *element);
            }
            else if (type == "SceneObjects" || type == "Lights")
            {
                CheckAttributes(*element, {});
                for (const XMLElement* child = element->FirstChildElement(); child;
                    child = child->NextSiblingElement())
                {
                    if (type == "Lights")
                        LoadLight(*scene, *child);
                    else if (std::string(child->Name()) == "SceneObject")
                        LoadObject(*scene, *child);
                    else
                        Fail(*child, "expected <SceneObject>");
                }
            }
            else if (type == "DirectionalLight" || type == "PointLight" || type == "SpotLight")
            {
                LoadLight(*scene, *element);
            }
            else
            {
                Fail(*element, "expected camera, objects, lights or materials");
            }
        }
        if (!hasCamera)
            Fail(*root, "a <Camera> is required");
        return scene.release();
    }
    catch (const std::runtime_error& error)
    {
        throw std::runtime_error("Failed to load scene '" + std::string(filepath) + "': " + error.what());
    }
}
