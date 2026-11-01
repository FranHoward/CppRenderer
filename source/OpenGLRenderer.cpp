#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "OpenGLRenderer.h"
#include "SceneGpuAdapter.h"
#include "GLShader.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>

#ifndef RENDERER_SHADER_DIR
#define RENDERER_SHADER_DIR "shaders"
#endif

namespace
{
    constexpr int ShadowSize = 1024;

    struct GlfwSession
    {
        GlfwSession()
        {
            glfwSetErrorCallback([](int code, const char* description)
            { std::cerr << "GLFW " << code << ": " << description << '\n'; });
            if (!glfwInit()) throw std::runtime_error("Cannot initialize GLFW");
        }
        ~GlfwSession() { glfwTerminate(); }
    };

    struct ShadowMap
    {
        GLuint texture = 0;
        Matrix4x4 matrix = Matrix4x4(1);
        float farPlane = 1;
        float bias = 0.002f;
    };

    struct Resources
    {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint fbo = 0;
        std::vector<ShadowMap> shadows;
        ~Resources()
        {
            for (const auto& map : shadows) glDeleteTextures(1, &map.texture);
            glDeleteFramebuffers(1, &fbo);
            glDeleteBuffers(1, &vbo);
            glDeleteVertexArrays(1, &vao);
        }
    };

    void CheckGL(const char* stage)
    {
        const GLenum error = glGetError();
        if (error != GL_NO_ERROR)
            throw std::runtime_error(std::string("OpenGL error at ") + stage + ": " + std::to_string(error));
    }

    void Capture(const std::string& path, int width, int height)
    {
        std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 3);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadBuffer(GL_BACK);
        glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
        CheckGL("screenshot readback");
        std::ofstream output(path, std::ios::binary);
        if (!output) throw std::runtime_error("Cannot write screenshot: " + path);
        output << "P6\n" << width << ' ' << height << "\n255\n";
        for (int row = height - 1; row >= 0; --row)
            output.write(reinterpret_cast<const char*>(pixels.data() + static_cast<size_t>(row) * width * 3),
                static_cast<std::streamsize>(width) * 3);
        if (!output) throw std::runtime_error("Screenshot write failed: " + path);
    }

    Vector3f SafeUp(const Vector3f& direction)
    {
        return std::abs(direction.y) < 0.95f ? Vector3f(0, 1, 0) : Vector3f(0, 0, 1);
    }

    void BuildShadows(Resources& resources, const GpuSceneData& data, const GLShader& shader, GLsizei count)
    {
        glGenFramebuffers(1, &resources.fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, resources.fbo);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glViewport(0, 0, ShadowSize, ShadowSize);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        shader.Use();
        const Vector3f center = (data.boundsMin + data.boundsMax) * 0.5f;
        const float radius = std::max(glm::length(data.boundsMax - data.boundsMin) * 0.5f, 0.1f);
        const std::array<Vector3f, 6> directions = {Vector3f(1, 0, 0), Vector3f(-1, 0, 0),
            Vector3f(0, 1, 0), Vector3f(0, -1, 0), Vector3f(0, 0, 1), Vector3f(0, 0, -1)};
        const std::array<Vector3f, 6> ups = {Vector3f(0, -1, 0), Vector3f(0, -1, 0),
            Vector3f(0, 0, 1), Vector3f(0, 0, -1), Vector3f(0, -1, 0), Vector3f(0, -1, 0)};

        for (const auto& light : data.lights)
        {
            resources.shadows.emplace_back();
            ShadowMap& map = resources.shadows.back();
            glGenTextures(1, &map.texture);
            const bool directional = light.type == GpuLightType::Directional;
            const GLenum target = directional ? GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP;
            glBindTexture(target, map.texture);
            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            if (!directional) glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            const int faces = directional ? 1 : 6;
            for (int face = 0; face < faces; ++face)
                glTexImage2D(directional ? GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                    0, GL_DEPTH_COMPONENT32F, ShadowSize, ShadowSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

            map.farPlane = directional ? 4 * radius : glm::length(light.position - center) + radius + 0.1f;
            map.bias = std::max(radius * 0.0005f, 0.0001f);
            shader.Set("radialDepth", directional ? 0 : 1);
            shader.Set("lightPosition", light.position);
            shader.Set("shadowFar", map.farPlane);
            if (directional)
            {
                map.matrix = glm::orthoRH_NO(-radius, radius, -radius, radius, 0.01f, map.farPlane) *
                    glm::lookAtRH(center - light.direction * (2 * radius), center, SafeUp(light.direction));
            }
            for (int face = 0; face < faces; ++face)
            {
                const Matrix4x4 matrix = directional ? map.matrix :
                    glm::perspectiveRH_NO(PI * 0.5f, 1.0f, std::min(0.01f, map.farPlane * 0.001f), map.farPlane) *
                    glm::lookAtRH(light.position, light.position + directions[face], ups[face]);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                    directional ? GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, map.texture, 0);
                if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                    throw std::runtime_error("Shadow framebuffer is incomplete");
                shader.Set("lightMatrix", matrix);
                glClear(GL_DEPTH_BUFFER_BIT);
                glDrawArrays(GL_TRIANGLES, 0, count);
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        CheckGL("shadow setup");
    }

    class InteractiveCamera
    {
    public:
        explicit InteractiveCamera(const Camera& source) : original(source) { Reset(); }
        void Reset()
        {
            position = original.GetPosition();
            baseForward = glm::normalize(original.GetTarget() - position);
            baseRight = glm::normalize(glm::cross(original.GetUp(), baseForward));
            baseUp = glm::cross(baseForward, baseRight);
            yaw = pitch = 0;
            dragging = false;
        }
        void Update(GLFWwindow* window, float seconds, float speed)
        {
            if (!glfwGetWindowAttrib(window, GLFW_FOCUSED))
            {
                dragging = false;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                return;
            }
            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) Reset();
            double x = 0, y = 0;
            glfwGetCursorPos(window, &x, &y);
            const bool down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
            if (down && dragging)
            {
                yaw += static_cast<float>(x - lastX) * 0.003f;
                pitch = glm::clamp(pitch - static_cast<float>(y - lastY) * 0.003f, -1.55f, 1.55f);
            }
            if (down != dragging)
                glfwSetInputMode(window, GLFW_CURSOR, down ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
            dragging = down;
            lastX = x; lastY = y;
            const Vector3f forward = Forward();
            const Vector3f right = glm::normalize(glm::cross(baseUp, forward));
            Vector3f movement(0);
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) movement += forward;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) movement -= forward;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) movement += right;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) movement -= right;
            if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) movement += baseUp;
            if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) movement -= baseUp;
            if (glm::dot(movement, movement) > 0)
                position += glm::normalize(movement) * seconds * speed *
                    (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? 3.0f : 1.0f);
        }
        Matrix4x4 ViewProjection(int width, int height) const
        {
            // Preserve the existing left-handed camera, but use OpenGL's [-1,1] clip depth.
            return glm::perspectiveLH_NO(glm::radians(original.GetFov()), static_cast<float>(width) / height,
                original.GetNearPlane(), original.GetFarPlane()) *
                glm::lookAtLH(position, position + Forward(), baseUp);
        }
    private:
        Vector3f Forward() const
        {
            return std::cos(pitch) * (std::cos(yaw) * baseForward + std::sin(yaw) * baseRight) +
                std::sin(pitch) * baseUp;
        }
        Camera original;
        Vector3f position, baseForward, baseRight, baseUp;
        float yaw = 0, pitch = 0;
        double lastX = 0, lastY = 0;
        bool dragging = false;
    };
}

OpenGLRenderer::OpenGLRenderer(int viewportWidth, int viewportHeight, const char* filepath)
    : mWidth(viewportWidth), mHeight(viewportHeight), mFilepath(filepath ? filepath : "")
{
    if (mWidth <= 0 || mHeight <= 0 || mFilepath.empty())
        throw std::invalid_argument("OpenGL renderer requires positive dimensions and a scene path");
}

void OpenGLRenderer::Run(const OpenGLRunOptions& options)
{
    if (options.maxFrames < 0 || (options.hidden && options.maxFrames == 0) ||
        (!options.screenshotPath.empty() && options.maxFrames == 0))
        throw std::invalid_argument("Hidden rendering and screenshots require a positive frame limit");
    const std::unique_ptr<Scene> scene(Scene::LoadSceneFromXML(mFilepath.c_str(), mWidth, mHeight));
    const GpuSceneData data = BuildGpuScene(*scene);
    if (data.vertices.size() > static_cast<size_t>(std::numeric_limits<GLsizei>::max()))
        throw std::runtime_error("Scene exceeds OpenGL vertex count limit");
    const GLsizei vertexCount = static_cast<GLsizei>(data.vertices.size());
    GlfwSession session;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_VISIBLE, options.hidden ? GLFW_FALSE : GLFW_TRUE);
    std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> window(
        glfwCreateWindow(mWidth, mHeight, "C++ Renderer | OpenGL", nullptr, nullptr), glfwDestroyWindow);
    if (!window)
        throw std::runtime_error("Cannot create an OpenGL 3.3 window; use --renderer cpu or check GPU drivers");
    glfwMakeContextCurrent(window.get());
    if (!gladLoadGL(glfwGetProcAddress) || !GLAD_GL_VERSION_3_3)
        throw std::runtime_error("OpenGL 3.3 function loading failed");
    glfwSwapInterval(options.hidden ? 0 : 1);
    std::cout << "OpenGL " << glGetString(GL_VERSION) << " | " << glGetString(GL_RENDERER) << '\n';
    std::cout << "RMB: look | WASD: move | Q/E: down/up | Shift: faster | R: reset | Esc: close\n";

    const std::string shaderDir = RENDERER_SHADER_DIR;
    GLShader shader(shaderDir + "/scene.vert", shaderDir + "/scene.frag");
    GLShader shadowShader(shaderDir + "/shadow.vert", shaderDir + "/shadow.frag");
    Resources resources;
    glGenVertexArrays(1, &resources.vao);
    glGenBuffers(1, &resources.vbo);
    glBindVertexArray(resources.vao);
    glBindBuffer(GL_ARRAY_BUFFER, resources.vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(data.vertices.size() * sizeof(GpuVertex)),
        data.vertices.data(), GL_STATIC_DRAW);
    const std::array<size_t, 3> offsets = {offsetof(GpuVertex, position), offsetof(GpuVertex, normal), offsetof(GpuVertex, albedo)};
    for (GLuint i = 0; i < offsets.size(); ++i)
    {
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, 3, GL_FLOAT, GL_FALSE, sizeof(GpuVertex), reinterpret_cast<const void*>(offsets[i]));
    }
    // CPU intersections are two-sided. Keep rasterization two-sided too.
    glDisable(GL_CULL_FACE);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    if (options.shadows) BuildShadows(resources, data, shadowShader, vertexCount);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_MULTISAMPLE);
    glBlendFunc(GL_ONE, GL_ONE);
    shader.Use();
    shader.Set("shadow2D", 0);
    shader.Set("shadowCube", 1);
    shader.Set("shadowsEnabled", options.shadows ? 1 : 0);
    CheckGL("scene upload");

    InteractiveCamera camera(scene->GetCamera());
    const float moveSpeed = std::max(glm::length(data.boundsMax - data.boundsMin) * 0.25f, 0.5f);
    int frames = 0, titleFrames = 0;
    double previous = glfwGetTime(), titleTime = previous;
    while (!glfwWindowShouldClose(window.get()) && (options.maxFrames == 0 || frames < options.maxFrames))
    {
        glfwPollEvents();
        if (glfwGetKey(window.get(), GLFW_KEY_ESCAPE) == GLFW_PRESS) break;
        const double now = glfwGetTime();
        const float delta = static_cast<float>(std::min(now - previous, 0.1));
        previous = now;
        camera.Update(window.get(), delta, moveSpeed);
        int width = 0, height = 0;
        glfwGetFramebufferSize(window.get(), &width, &height);
        if (width == 0 || height == 0) { glfwWaitEventsTimeout(0.1); continue; }
        glViewport(0, 0, width, height);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader.Set("viewProjection", camera.ViewProjection(width, height));
        for (size_t i = 0; i < data.lights.size(); ++i)
        {
            const auto& light = data.lights[i];
            shader.Set("lightType", static_cast<int>(light.type));
            shader.Set("lightPosition", light.position);
            shader.Set("lightDirection", light.direction);
            shader.Set("lightColor", light.color);
            shader.Set("attenuations", light.attenuations);
            shader.Set("cosInner", light.cosInner);
            shader.Set("cosOuter", light.cosOuter);
            if (options.shadows)
            {
                const auto& shadow = resources.shadows[i];
                shader.Set("lightMatrix", shadow.matrix);
                shader.Set("shadowFar", shadow.farPlane);
                shader.Set("shadowBias", shadow.bias);
                const bool directional = light.type == GpuLightType::Directional;
                glActiveTexture(directional ? GL_TEXTURE0 : GL_TEXTURE1);
                glBindTexture(directional ? GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP, shadow.texture);
            }
            // Add one light per pass, avoiding a fixed shader light/sampler limit.
            if (i > 0) { glEnable(GL_BLEND); glDepthMask(GL_FALSE); }
            glDrawArrays(GL_TRIANGLES, 0, vertexCount);
        }
        CheckGL("frame rendering");
        ++frames;
        ++titleFrames;
        if (!options.screenshotPath.empty() && frames == options.maxFrames)
            Capture(options.screenshotPath, width, height);
        glfwSwapBuffers(window.get());
        if (now - titleTime >= 1.0)
        {
            const std::string title = "C++ Renderer | OpenGL | " +
                std::to_string(static_cast<int>(titleFrames / (now - titleTime))) + " FPS | RMB look / WASD move";
            glfwSetWindowTitle(window.get(), title.c_str());
            titleFrames = 0;
            titleTime = now;
        }
    }
    std::cout << "Rendered " << frames << " OpenGL frames\n";
}
