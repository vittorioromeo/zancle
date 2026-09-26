////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/GraphicsContext.hpp"
#include "Zancle/Graphics/RenderWindow.hpp"
#include "Zancle/Graphics/Texture.hpp"

#include "Zancle/IO/Path.hpp"

#include "Zancle/Chrono/Clock.hpp"
#include "Zancle/Chrono/Time.hpp"

#include "Zancle/Geometry/Angle.hpp"
#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Math/Cos.hpp"

#include "Zancle/Base/WindowsHeader.hpp"


namespace
{
HWND button;

////////////////////////////////////////////////////////////
/// Function called whenever one of our windows receives a message
///
////////////////////////////////////////////////////////////
LRESULT CALLBACK onEvent(HWND handle, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        // Quit when we close the main window
        case WM_CLOSE:
        {
            PostQuitMessage(0);
            return 0;
        }

        // Quit when we click the "quit" button
        case WM_COMMAND:
        {
            if (reinterpret_cast<HWND>(lParam) == button)
            {
                PostQuitMessage(0);
                return 0;
            }
        }
    }

    return DefWindowProc(handle, message, wParam, lParam);
}
} // namespace


////////////////////////////////////////////////////////////
/// Entry point of application
///
/// \param Instance: Instance of the application
///
/// \return Error code
///
////////////////////////////////////////////////////////////
int main()
{
    HINSTANCE instance = GetModuleHandle(nullptr);

    // Define a class for our main window
    WNDCLASS windowClass;
    windowClass.style         = 0;
    windowClass.lpfnWndProc   = &onEvent;
    windowClass.cbClsExtra    = 0;
    windowClass.cbWndExtra    = 0;
    windowClass.hInstance     = instance;
    windowClass.hIcon         = nullptr;
    windowClass.hCursor       = nullptr;
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BACKGROUND);
    windowClass.lpszMenuName  = nullptr;
    windowClass.lpszClassName = TEXT("Zancle App");
    RegisterClass(&windowClass);

    // Create the main window
    HWND window = CreateWindow(TEXT("Zancle App"),
                               TEXT("Zancle Win32"),
                               WS_SYSMENU | WS_VISIBLE,
                               200,
                               200,
                               660,
                               520,
                               nullptr,
                               nullptr,
                               instance,
                               nullptr);

    // Add a button for exiting
    button = CreateWindow(TEXT("BUTTON"), TEXT("Quit"), WS_CHILD | WS_VISIBLE, 560, 440, 80, 40, window, nullptr, instance, nullptr);

    // Create two Zancle views
    HWND view1 = CreateWindow(TEXT("STATIC"),
                              nullptr,
                              WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                              20,
                              20,
                              300,
                              400,
                              window,
                              nullptr,
                              instance,
                              nullptr);
    HWND view2 = CreateWindow(TEXT("STATIC"),
                              nullptr,
                              WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                              340,
                              20,
                              300,
                              400,
                              window,
                              nullptr,
                              instance,
                              nullptr);

    auto graphicsContext = za::GraphicsContext::create().value();

    {
        auto zancleView1 = za::RenderWindow::create(view1).value();
        auto zancleView2 = za::RenderWindow::create(view2).value();

        // Load some textures to display
        const auto texture1 = za::Texture::loadFromFile("resources/image1.jpg").value();
        const auto texture2 = za::Texture::loadFromFile("resources/image2.jpg").value();

        // Create a clock for measuring elapsed time
        const za::Clock clock;

        // Loop until a WM_QUIT message is received
        MSG message;
        message.message = static_cast<UINT>(~WM_QUIT);
        while (message.message != WM_QUIT)
        {
            if (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
            {
                // If a message was waiting in the message queue, process it
                TranslateMessage(&message);
                DispatchMessage(&message);
            }
            else
            {
                const float time = clock.getElapsedTime().asSeconds();

                // Clear views
                zancleView1.clear();
                zancleView2.clear();

                // Draw sprite 1 on view 1
                zancleView1.draw(texture1,
                                 {.position = texture1.getSize().toVec2f() / 2.f,
                                  .origin   = texture1.getSize().toVec2f() / 2.f,
                                  .rotation = za::degrees(time * 100).wrapUnsigned()});

                // Draw sprite 2 on view 2
                zancleView2.draw(texture2, {.position = {za::cos(time) * 100.f, 0.f}});

                // Display each view on screen
                zancleView1.display();
                zancleView2.display();
            }
        }
    }

    // Destroy the main window (all its child controls will be destroyed)
    DestroyWindow(window);

    // Don't forget to unregister the window class
    UnregisterClass(TEXT("Zancle App"), instance);
}
