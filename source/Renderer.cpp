#include "Renderer.h"
#include "SceneObject.h"
#include "Material.h"
#include <MiniFB.h>
#include <thread>
#include <vector>
#include <algorithm>
#include <memory>


Renderer::Renderer(int viewportWidth, int viewportHeight, int minDepth, int maxDepth, int samplesPerPixel, 
    const char* filepath)
    : mViewportWidth(viewportWidth), mViewportHeight(viewportHeight), mSamplesPerPixel(samplesPerPixel), 
    mMaxDepth(maxDepth), mMinDepth(minDepth)
{
    mCurrentPixelIndex = 0;

    mScene = Scene::LoadSceneFromXML(filepath, mViewportWidth, mViewportHeight);
}

Renderer::~Renderer()
{
    if (mScene)
        delete mScene;
}

void Renderer::Run()
{
    // Buffer ownership stays on the calling thread until every worker has joined.
    auto buffer = std::make_unique<uint32_t[]>(static_cast<size_t>(mViewportWidth) * mViewportHeight);
    struct mfb_window* window = mfb_open_ex("C++ Renderer", mViewportWidth, mViewportHeight, MFB_WF_RESIZABLE);

    if (window == NULL)
        return;

    // Color of every pixel in the window is expressed as a 32-bit unsigned integer in the format 0xAARRGGBB
    // where AA is the alpha channel, RR is the red channel, GG is the green channel, and BB is the blue channel.
    mBuffer = buffer.get();
    mCurrentPixelIndex = 0;
    mStopRequested = false;
    std::vector<std::thread> workers;
    auto stopWorkers = [&]()
    {
        mStopRequested = true;
        for (auto& worker : workers)
            if (worker.joinable()) worker.join();
        mBuffer = nullptr;
    };
    auto closeWindow = [&]()
    {
        if (window)
        {
            mfb_close(window);
            mfb_update_events(window);
            window = nullptr;
        }
    };
    try
    {
        const unsigned numThreads = std::max(1u, std::thread::hardware_concurrency());
        workers.reserve(numThreads);
        for (unsigned i = 0; i < numThreads; ++i)
            workers.emplace_back(&Renderer::RunRenderThread, this);
        do
        {
            const auto state = mfb_update_ex(window, mBuffer, mViewportWidth, mViewportHeight);
            if (state == MFB_STATE_EXIT) { window = nullptr; break; }
            if (state != MFB_STATE_OK) break;
            // MiniFB destroys the window itself when either event call detects close.
            if (!mfb_wait_sync(window)) { window = nullptr; break; }
        }
        while (true);
    }
    catch (...)
    {
        stopWorkers();
        closeWindow();
        throw;
    }
    stopWorkers();
    closeWindow();
}

Color Renderer::RenderPixel(int x, int y)
{
    // SSAA
    const int samplesPerPixel = mSamplesPerPixel;
    Color resultColor = Color(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < samplesPerPixel; i++)
    {
        if (mStopRequested) break;
        float px = x + glm::linearRand(0.0f, 1.0f);
        float py = y + glm::linearRand(0.0f, 1.0f);

        Color color = RenderSubPixel(px, py);

        resultColor += (color / (float)samplesPerPixel); // Accumulate the color for each sample
    }

    return resultColor;
}

Color Renderer::RenderSubPixel(float x, float y)
{
    Ray ray = mScene->GetCamera().GetRay(x, y);
    Color color = GetRadiance(ray, 0); // Start with depth 0 for the initial ray
    return color;
}

Color Renderer::GetIrradiance(const Ray& ray)
{
    Intersection hit;
    SceneObject* object = mScene->Intersect(ray, hit);
    if (!object)
        return Color(0.0f, 0.0f, 0.0f); // Return black if no intersection
    
    Color color(0.0f, 0.0f, 0.0f);
    for (Light* pLight : mScene->GetLights())
    {
        // Calculate the irradiance from each light
        Vector3f sourcePos;
        Color l = pLight->GetRadiance(hit.position, sourcePos);

        Ray shadowRay;
        shadowRay.o = hit.position;
        shadowRay.d = glm::normalize(sourcePos - hit.position);
        shadowRay.mint = 0.001f; // Avoid self-intersection
        shadowRay.maxt = glm::length(sourcePos - hit.position);

        Intersection shadowHit;
        if (mScene->Intersect(shadowRay, shadowHit))
            continue; // If the shadow ray intersects an object, skip this light
        
        float cosTheta = glm::dot(hit.normal, shadowRay.d);
        color += l * glm::max(cosTheta, 0.0f);
    }

    return color;
}

Color Renderer::GetRadiance(const Ray& ray, int depth)
{
    if (mStopRequested) return Color(0.0f);
    if (depth > mMaxDepth)
        return Color(0.0f, 0.0f, 0.0f); // Return black if the maximum recursion depth is exceeded

    static const float survivalProbability = 0.8f; // Probability of continuing the path
    float rewardFactor = 1.0f;
    if (depth >= mMinDepth)
    {
        float k = Random01();
        if (k > survivalProbability)
            return Color(0.0f, 0.0f, 0.0f); // Terminate the path with a probability of 1 - survivalProbability
        rewardFactor = 1.0f / survivalProbability; // Scale the color by the survival probability
    }
    
    Intersection hit;
    SceneObject* object = mScene->Intersect(ray, hit);
    if (!object)
        return Color(0.0f, 0.0f, 0.0f); // Return black if no intersection
    
    Material* pMaterial = object->GetMaterial();
    Color color(0.0f, 0.0f, 0.0f);

    Matrix3x3 localToWorld = MakeCoordinateSystem(hit.normal);
    Matrix3x3 worldToLocal = glm::transpose(localToWorld);

    Vector3f wo = worldToLocal * (-ray.d); // Outgoing direction in local space


    for (Light* pLight : mScene->GetLights())
    {
        // Calculate the irradiance from each light
        Vector3f sourcePos;
        Color l = pLight->GetRadiance(hit.position, sourcePos);

        Ray shadowRay;
        shadowRay.o = hit.position;
        shadowRay.d = glm::normalize(sourcePos - hit.position);
        shadowRay.mint = 0.001f; // Avoid self-intersection
        shadowRay.maxt = glm::length(sourcePos - hit.position);

        Intersection shadowHit;
        if (mScene->Intersect(shadowRay, shadowHit))
            continue; // If the shadow ray intersects an object, skip this light
        
        Vector3f wi = worldToLocal * shadowRay.d; // Incoming direction in local space
        float cosTheta = glm::dot(hit.normal, shadowRay.d);
        Color brdf = pMaterial->brdf(wi, wo); // Assuming the BRDF returns a Color, we take the red channel as a representative value

        color += brdf * l * glm::max(cosTheta, 0.0f);
    }
    
    {
        const float theta = Random(0.0f, PI * 0.5f); // Sample within the hemisphere
        const float phi = Random(0.0f, 2.0f * PI);
        const Vector3f wi = GetSphericalCoordinate(theta, phi); // Sampled incoming direction in local space
        
        Color brdf = pMaterial->brdf(wo, wi); // Assuming the BRDF returns a Color, we take the red channel as a representative value
        Ray r;
        r.d = localToWorld * wi;
        r.o = hit.position;
        r.mint = 0.001f; // Avoid self-intersection
        Color li = GetRadiance(r, depth + 1); // Recursive call to get the radiance from the sampled direction

        color += brdf * li * cosf(theta) * sinf(theta) * PI * PI; // Multiply by the PDF of the sampling method (cosine-weighted hemisphere sampling)
    }

    return color * rewardFactor; // Scale the color by the survival probability to account for Russian roulette termination
}

void Renderer::RunRenderThread()
{
    // Read the next pixel on the current viewport
    while (!mStopRequested)
    {
        int pixelIndex = mCurrentPixelIndex.fetch_add(1);
        if (pixelIndex >= mViewportWidth * mViewportHeight)
            break;

        int x = pixelIndex % mViewportWidth;
        int y = pixelIndex / mViewportWidth;

        Color color = RenderPixel(x, y);
        uint8_t r = glm::clamp((uint32_t)std::round(color.r * 255.0f), 0u, 255u);
        uint8_t g = glm::clamp((uint32_t)std::round(color.g * 255.0f), 0u, 255u);
        uint8_t b = glm::clamp((uint32_t)std::round(color.b * 255.0f), 0u, 255u);

        mBuffer[y * mViewportWidth + x] = (r << 16) | (g << 8) | b;
    }
}
