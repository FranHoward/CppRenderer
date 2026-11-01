#include "Renderer.h"
#include <exception>
#include <charconv>
#include <stdexcept>
#include <string>
#ifdef RENDERER_ENABLE_OPENGL
#include "OpenGLRenderer.h"
#endif

namespace
{
    int ReadInt(const std::string& value, const std::string& option, int minimum)
    {
        int result = 0;
        const auto parsed = std::from_chars(value.data(), value.data() + value.size(), result);
        if (parsed.ec != std::errc() || parsed.ptr != value.data() + value.size() || result < minimum)
            throw std::invalid_argument("Invalid value for " + option + ": " + value);
        return result;
    }
}

int main(int argc, char** argv)
{
    try
    {
        // No arguments preserve the original CPU renderer and its settings.
        std::string backend = "cpu", scenePath = "scene/CornellBox.xml", screenshot;
        int width = 1920, height = 1080, minDepth = 3, maxDepth = 15, samples = 1000;
        [[maybe_unused]] int frames = 0;
        [[maybe_unused]] bool hidden = false, shadows = true;
        bool glOption = false;
        for (int i = 1; i < argc; ++i)
        {
            const std::string option = argv[i];
            auto value = [&]() -> std::string
            {
                if (++i >= argc) throw std::invalid_argument("Missing value for " + option);
                return argv[i];
            };
            if (option == "--help" || option == "-h")
            {
                std::cout << "Usage: C++Renderer [--renderer cpu|opengl] [--scene file.xml]\n"
                    "  --width N --height N       Viewport size (default 1920x1080)\n"
                    "  --samples N               CPU samples per pixel (default 1000)\n"
                    "  --min-depth N --max-depth N CPU recursion limits (default 3/15)\n"
                    "  --frames N                OpenGL frame limit (default interactive)\n"
                    "  --hidden                  Hidden OpenGL window; requires --frames N\n"
                    "  --screenshot file.ppm     Save last OpenGL frame; requires --frames N\n"
                    "  --no-shadows              Disable OpenGL shadow maps\n"
                    "OpenGL controls: RMB look, WASD move, Q/E down/up, Shift faster, R reset, Esc close\n";
                return 0;
            }
            else if (option == "--renderer") backend = value();
            else if (option == "--scene") scenePath = value();
            else if (option == "--width") width = ReadInt(value(), option, 1);
            else if (option == "--height") height = ReadInt(value(), option, 1);
            else if (option == "--samples") samples = ReadInt(value(), option, 1);
            else if (option == "--min-depth") minDepth = ReadInt(value(), option, 0);
            else if (option == "--max-depth") maxDepth = ReadInt(value(), option, 0);
            else if (option == "--frames") { frames = ReadInt(value(), option, 1); glOption = true; }
            else if (option == "--hidden") { hidden = true; glOption = true; }
            else if (option == "--screenshot") { screenshot = value(); glOption = true; }
            else if (option == "--no-shadows") { shadows = false; glOption = true; }
            else throw std::invalid_argument("Unknown option: " + option);
        }
        if (width > 16384 || height > 16384)
            throw std::invalid_argument("Viewport dimensions must not exceed 16384");
        if (minDepth > maxDepth) throw std::invalid_argument("--min-depth must not exceed --max-depth");
        if (backend == "cpu")
        {
            if (glOption) throw std::invalid_argument("--frames/--hidden/--screenshot/--no-shadows require --renderer opengl");
            Renderer renderer(width, height, minDepth, maxDepth, samples, scenePath.c_str());
            renderer.Run();
        }
        else if (backend == "opengl")
        {
#ifdef RENDERER_ENABLE_OPENGL
            OpenGLRenderer renderer(width, height, scenePath.c_str());
            renderer.Run({frames, hidden, shadows, screenshot});
#else
            throw std::runtime_error("OpenGL backend was disabled at build time; configure with -DRENDERER_ENABLE_OPENGL=ON");
#endif
        }
        else throw std::invalid_argument("Unknown renderer: " + backend + "; expected cpu or opengl");
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << std::endl;
        return 1;
    }

    return 0;
}
