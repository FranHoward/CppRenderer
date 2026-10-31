#include "Common.h"
#include <atomic>
#include "Camera.h"
#include "Sphere.h"
#include "Disk.h"
#include "Triangle.h"
#include "Scene.h"

class Renderer
{
    public:
        Renderer(int viewportWidth, int viewportHeight, int samplesPerPixel, const char* filepath);
        virtual ~Renderer();
        void Run();

    private:
        Color RenderPixel(int x, int y);
        Color RenderSubPixel(float x, float y);
        Color GetIrradiance(const Ray& ray);
        Color GetRadiance(const Ray& ray);

        void RunRenderThread();

        int mViewportWidth = 800;
        int mViewportHeight = 600;
        int mSamplesPerPixel = 100;
        uint32_t* mBuffer = nullptr;
        
        std::atomic<int> mCurrentPixelIndex = 0;

        Scene* mScene = nullptr;
};