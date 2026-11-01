#include "Renderer.h"
#include <MiniFB.h>
#include <chrono>
#include <thread>
#include <stdexcept>

// A deterministic window substitute: close while an expensive CPU render is
// active. Exercise both MiniFB APIs which can consume a close event, without UI.
struct mfb_window {};
namespace
{
    mfb_window window;
    bool closeDuringUpdate = true;
}

extern "C" mfb_window* mfb_open_ex(const char*, unsigned, unsigned, unsigned) { return &window; }
extern "C" mfb_update_state mfb_update_ex(mfb_window*, void*, unsigned, unsigned)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    return closeDuringUpdate ? MFB_STATE_EXIT : MFB_STATE_OK;
}
extern "C" bool mfb_wait_sync(mfb_window*) { return false; }
extern "C" void mfb_close(mfb_window*) {}
extern "C" mfb_update_state mfb_update_events(mfb_window*) { return MFB_STATE_EXIT; }

int main(int argc, char** argv)
{
    if (argc != 2) return 2;
    try
    {
        for (bool update : {true, false})
        {
            closeDuringUpdate = update;
            // Much more work than can finish before the simulated close event.
            // Joining without cancellation would take far beyond the test timeout.
            Renderer renderer(256, 256, 3, 15, 2000000, argv[1]);
            for (int run = 0; run < 2; ++run)
            {
                const auto start = std::chrono::steady_clock::now();
                renderer.Run();
                const auto elapsed = std::chrono::steady_clock::now() - start;
                if (elapsed > std::chrono::seconds(5))
                    throw std::runtime_error("Closing a render did not promptly cancel workers");
            }
        }
        // A detached worker would still be using the destroyed scene/buffer here.
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "Renderer shutdown tests passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
