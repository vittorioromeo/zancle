#include "Zancle/Graphics/CircleShape.hpp"
#include "Zancle/Graphics/Color.hpp"
#include "Zancle/Graphics/GraphicsContext.hpp"
#include "Zancle/Graphics/Image.hpp"
#include "Zancle/Graphics/PrimitiveType.hpp"
#include "Zancle/Graphics/RenderTexture.hpp"
#include "Zancle/Graphics/RenderWindow.hpp"
#include "Zancle/Graphics/Texture.hpp"
#include "Zancle/Graphics/Vertex.hpp"

#include "Zancle/ImGui/ImGuiContext.hpp"
#include "Zancle/ImGui/IncludeImGui.hpp"

#include "Zancle/Window/Event.hpp"
#include "Zancle/Window/EventUtils.hpp"
#include "Zancle/Window/Mouse.hpp"
#include "Zancle/Window/VideoMode.hpp"
#include "Zancle/Window/VideoModeUtils.hpp"

#include "Zancle/Fmt/Fmt.hpp"
#include "Zancle/Fmt/FmtNumeric.hpp"

#include "Zancle/IO/IO.hpp"

#include "Zancle/Chrono/Clock.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

int main()
{
    auto graphicsContext = za::GraphicsContext::create().value();

    auto window = za::RenderWindow::create({.size{1024u, 768u}, .title = "ImGui + Zancle = <3", .vsync = true}).value();

    za::ImGuiContext imGuiContext;

    const za::CircleShape shape{{.fillColor = za::Color::Green, .radius = 100.f}};

    const float width     = 128.f;
    const float height    = 64.f;
    const float halfWidth = width / 2.f;

    const za::Vec2u size{static_cast<unsigned int>(width), static_cast<unsigned int>(height)};

    auto baseRenderTexture = za::RenderTexture::create(size, {.sampleCount = 0}).value();

    auto leftInnerRT = za::RenderTexture::create(size, {.sampleCount = 4}).value();

    const za::Vertex leftVertexArray[6]{{{0.f, 0.f}, za::Color::Red, {0.f, 0.f}},
                                        {{halfWidth, 0.f}, za::Color::Red, {halfWidth, 0.f}},
                                        {{0.f, height}, za::Color::Red, {0.f, height}},
                                        {{0.f, height}, za::Color::Green, {0.f, height}},
                                        {{halfWidth, 0.f}, za::Color::Green, {halfWidth, 0.f}},
                                        {{halfWidth, height}, za::Color::Green, {halfWidth, height}}};

    leftInnerRT.clear();

    auto image   = za::Image::create(size, za::Color::White).value();
    auto texture = za::Texture::loadFromImage(image).value();

    leftInnerRT.draw(texture);
    leftInnerRT.display();

    baseRenderTexture.clear();
    baseRenderTexture.draw(leftVertexArray, za::PrimitiveType::Triangles, {.texture = &leftInnerRT.getTexture()});
    baseRenderTexture.display();

    za::Clock deltaClock;
    za::Vec2i eventMousePosition;

    // TODO P0:
    const auto modes = za::VideoModeUtils::getFullscreenModes();
    for (const auto& mode : modes)
        za::printLn("Fullscreen mode: {}x{}; {}bpp; {}x pixel density; {}Hz",
                    mode.size.x,
                    mode.size.y,
                    mode.bitsPerPixel,
                    mode.pixelDensity,
                    mode.refreshRate);

    while (true)
    {
        while (const za::Optional event = window.pollEvent())
        {
            imGuiContext.processEvent(window, *event);

            if (za::EventUtils::isClosedOrEscapeKeyPressed(*event))
                return 0;

            if (const auto* eResized = event->getIf<za::Event::Resized>())
                za::printLn("Resized event: {}, {}", eResized->size.x, eResized->size.y);

            if (const auto* eMouseMoved = event->getIf<za::Event::MouseMoved>())
                eventMousePosition = eMouseMoved->position;
        }

        imGuiContext.update(window, deltaClock.restart());

        ImGui::ShowDemoWindow();

        ImGui::Begin("Hello, world!");

        const auto globalMousePosition = za::Mouse::getPosition();
        ImGui::Text("Global mouse position: %d, %d", globalMousePosition.x, globalMousePosition.y);

        const auto relativeMousePosition = za::Mouse::getPosition(window);
        ImGui::Text("Relative mouse position: %d, %d", relativeMousePosition.x, relativeMousePosition.y);

        ImGui::Text("Event mouse position: %d, %d", eventMousePosition.x, eventMousePosition.y);

        if (ImGui::Button("Set Window Size to 400x400"))
            window.setSize({400u, 400u});

        if (ImGui::Button("Set Window Size to 800x800"))
            window.setSize({800u, 800u});

        if (ImGui::Button("Recreate Window"))
            window = za::RenderWindow::create({.size{1024u, 768u}, .title = "ImGui + Zancle = <3", .vsync = true})
                         .value(); // TODO P0: doesn't work on emscripten

        ImGui::Button("Look at this pretty button");
        imGuiContext.image(baseRenderTexture, size.toVec2f());

        ImGui::End();

        window.clear();
        imGuiContext.render(window);
        window.display();
    }
}
