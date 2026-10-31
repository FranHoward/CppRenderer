#include "Renderer.h"
#include <exception>

int main()
{
    try
    {
        Renderer renderer(1920, 1080, 100, "scene/CornellBox.xml"); // Set the desired viewport width and height
        renderer.Run();
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << std::endl;
        return 1;
    }

    return 0;
}
