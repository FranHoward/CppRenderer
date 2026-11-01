#pragma once
#include <string>

struct OpenGLRunOptions
{
    int maxFrames = 0; // Zero means run until the window is closed.
    bool hidden = false; // Hidden context for automated smoke tests.
    bool shadows = true;
    std::string screenshotPath; // Optional PPM capture of the last frame.
};

// Independent backend. The original Renderer interface and implementation are unchanged.
class OpenGLRenderer
{
public:
    OpenGLRenderer(int viewportWidth, int viewportHeight, const char* filepath);
    void Run(const OpenGLRunOptions& options = {});
private:
    int mWidth;
    int mHeight;
    std::string mFilepath;
};
