#include "Renderer.h"
#include <MiniFB.h>
#include <thread>
#include <vector>

Renderer::Renderer(int viewportWidth, int viewportHeight)
    : mViewportWidth(viewportWidth), mViewportHeight(viewportHeight)
{
    mCurrentPixelIndex = 0;
}

void Renderer::Run()
{
    struct mfb_window* window = mfb_open_ex("C++ Renderer", mViewportWidth, mViewportHeight, MFB_WF_RESIZABLE);

    if (window == NULL)
        return;

    // Color of every pixel in the window is expressed as a 32-bit unsigned integer in the format 0xAARRGGBB
    // where AA is the alpha channel, RR is the red channel, GG is the green channel, and BB is the blue channel.
    mBuffer = (uint32_t*)malloc(mViewportWidth * mViewportHeight * 4);

    std::thread RenderThread(&Renderer::RunRenderThread, this);
    RenderThread.detach(); // Detach the thread to allow it to run independently

    int numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> RenderThreads(numThreads);

    for (int i = 0; i < numThreads; i++)
    {
        RenderThreads[i] = std::thread(&Renderer::RunRenderThread, this);\
        RenderThreads[i].detach(); // Detach the thread to allow it to run independently
    }
    // Present the buffer to the window
    mfb_update_state state;
    do 
    {
        state = mfb_update_ex(window, mBuffer, mViewportWidth, mViewportHeight);

        if (state != MFB_STATE_OK)
            break;
    }  
    while (mfb_wait_sync(window));

    free(mBuffer);
    mBuffer = NULL;
    window = NULL;
}

Color Renderer::RenderPixel(int x, int y)
{
    Color color;
    color.r = (float) x / mViewportWidth;
    color.g = (float) y / mViewportHeight;
    color.b = 0.0f;

    return color;
}

void Renderer::RunRenderThread()
{
    // Read the next pixel on the current viewport
    while (true)
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