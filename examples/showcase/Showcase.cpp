#include "ShowcaseAudio.hpp"
#include "ShowcaseBunnyMark.hpp"
#include "ShowcaseExample.hpp"
#include "ShowcaseIndividualShape.hpp"
#include "ShowcaseShapes.hpp"
#include "ShowcaseTextComparison.hpp"

#include "ExampleUtils/Sampler.hpp"
#include "ExampleUtils/Scaling.hpp"

#include "Zancle/Audio/AudioContext.hpp"

#include "Zancle/Graphics/Font.hpp"
#include "Zancle/Graphics/GraphicsContext.hpp"
#include "Zancle/Graphics/Image.hpp"
#include "Zancle/Graphics/RenderTexture.hpp"
#include "Zancle/Graphics/RenderWindow.hpp"
#include "Zancle/Graphics/TextureAtlas.hpp"

#include "Zancle/ImGui/ImGuiContext.hpp"
#include "Zancle/ImGui/IncludeImGui.hpp"

#include "Zancle/Window/Event.hpp"
#include "Zancle/Window/EventUtils.hpp"
#include "Zancle/Window/Keyboard.hpp"

#include "Zancle/IO/Path.hpp"

#include "Zancle/String/String.hpp"
#include "Zancle/String/ToString.hpp"

#include "Zancle/Chrono/Clock.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Base/SizeT.hpp"


namespace
{
////////////////////////////////////////////////////////////
class Game
{
private:
    ////////////////////////////////////////////////////////////
    za::RenderWindow m_window = makeDPIScaledRenderWindow(
                                    {
                                        .size           = resolution.toVec2u(),
                                        .title          = "Showcase",
                                        .resizable      = true,
                                        .vsync          = true,
                                        .frametimeLimit = 144u,
                                    })
                                    .value();

    ////////////////////////////////////////////////////////////
    za::RenderTexture m_rtGame = makeAARenderTexture(resolution.toVec2u(), {.smooth = true, .sampleCount = 8u}).value();

    ////////////////////////////////////////////////////////////
    za::ImGuiContext m_imGuiContext;

    ////////////////////////////////////////////////////////////
    za::View m_worldView  = za::View::fromScreenSize(resolution);
    za::View m_windowView = computeAspectRatioAwareView(m_window.getSize().toVec2f(), resolution);

    ////////////////////////////////////////////////////////////
    za::Clock m_clock;
    za::Clock m_fpsClock;

    ////////////////////////////////////////////////////////////
    Sampler<float> m_samplesEventMs{/* capacity */ 64u};
    Sampler<float> m_samplesUpdateMs{/* capacity */ 64u};
    Sampler<float> m_samplesImGuiMs{/* capacity */ 64u};
    Sampler<float> m_samplesDrawMs{/* capacity */ 64u};
    Sampler<float> m_samplesDisplayMs{/* capacity */ 64u};
    Sampler<float> m_samplesFPS{/* capacity */ 64u};

    ////////////////////////////////////////////////////////////
    unsigned int m_lastFrameDrawCallCount = 0u;
    za::SizeT    m_lastFrameDrawnVertices = 0u;

    ////////////////////////////////////////////////////////////
    za::TextureAtlas m_textureAtlas{za::Texture::create({1024u, 1024u}, {.smooth = true}).value()};

    ////////////////////////////////////////////////////////////
    ImFont*        m_imGuiFont{ImGui::GetIO().Fonts->AddFontFromFileTTF("resources/Born2bSportyFS.ttf", 18.f)};
    const za::Font m_font = za::Font::openFromFile("resources/tuffy.ttf", &m_textureAtlas).value();

    ////////////////////////////////////////////////////////////
    GameDependencies m_deps{&m_window, &m_rtGame, &m_worldView, &m_font};

    ////////////////////////////////////////////////////////////
    ExampleShapes          m_exampleShapes{m_deps};
    ExampleBunnyMark       m_exampleBunnyMark{m_deps, m_textureAtlas};
    ExampleAudio           m_exampleAudio;
    ExampleIndividualShape m_exampleIndividualShape{m_deps};
    ExampleTextComparison  m_exampleTextComparison{m_deps};

    ////////////////////////////////////////////////////////////
    static constexpr int exampleCount = 5;

    ////////////////////////////////////////////////////////////
    ShowcaseExample* m_examples[exampleCount]{
        &m_exampleShapes,
        &m_exampleBunnyMark,
        &m_exampleAudio,
        &m_exampleIndividualShape,
        &m_exampleTextComparison,
    };

    ////////////////////////////////////////////////////////////
    int m_activeExample = 3;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] ShowcaseExample& getActiveExample()
    {
        return *m_examples[m_activeExample];
    }


    ////////////////////////////////////////////////////////////
    void clearSamples()
    {
        m_samplesEventMs.clear();
        m_samplesUpdateMs.clear();
        m_samplesImGuiMs.clear();
        m_samplesDrawMs.clear();
        m_samplesDisplayMs.clear();
        m_samplesFPS.clear();
    }

    ////////////////////////////////////////////////////////////
    void plotSamples(const char* label, const char* unit, const Sampler<float>& samples, float upperBound)
    {
        ImGui::PlotLines(label,
                         samples.data(),
                         static_cast<int>(samples.capacity()),
                         static_cast<int>(samples.insertionIndex()),
                         (za::toString(samples.getAverageAs<double>()) + unit).cStr(),
                         0.f,
                         upperBound,
                         ImVec2{256.f, 32.f});
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Rect2f addImgToAtlas(const za::Path& path)
    {
        return m_textureAtlas.add(za::Image::loadFromFile(path).value()).value();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool runEventHandling()
    {
        m_clock.restart();

        while (za::Optional event = m_window.pollEvent())
        {
            m_imGuiContext.processEvent(m_window, *event);

            if (za::EventUtils::isClosedOrEscapeKeyPressed(*event))
                return true;

            if (handleAspectRatioAwareResize(*event, resolution, m_windowView))
                continue;

            if (ImGui::GetIO().WantCaptureKeyboard)
                continue;

            if (auto* eKeyPressed = event->getIf<za::Event::KeyPressed>())
                if (eKeyPressed->code == za::Keyboard::Key::Space)
                {
                    ++m_activeExample;

                    if (m_activeExample >= exampleCount)
                        m_activeExample = 0;
                }
        }

        m_samplesEventMs.record(m_clock.getElapsedTime().asSeconds() * 1000.f);

        return false;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Time runUpdate()
    {
        m_clock.restart();

        const za::Time deltaTime   = m_fpsClock.restart();
        const float    deltaTimeMs = deltaTime.asSeconds() * 1000.f;

        getActiveExample().update(deltaTimeMs * 0.01f);

        m_samplesUpdateMs.record(m_clock.getElapsedTime().asSeconds() * 1000.f);

        return deltaTime;
    }

    ////////////////////////////////////////////////////////////
    void runImGui(za::Time deltaTime)
    {
        m_clock.restart();

        m_imGuiContext.update(m_window, deltaTime);

        ImGui::PushFont(m_imGuiFont);
        ImGui::Begin("Granita Showcase", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::SetNextItemWidth(120.f);
        {
            const char* names[exampleCount];
            for (za::SizeT i = 0u; i < exampleCount; ++i)
                names[i] = m_examples[i]->name;

            ImGui::Combo("Example", &m_activeExample, names, exampleCount);
        }

        plotSamples("Update", " ms", m_samplesUpdateMs, 64.f);
        plotSamples("Draw", " ms", m_samplesDrawMs, 64.f);
        plotSamples("FPS", " FPS", m_samplesFPS, 360.f);
        // plotSamples("Events", " ms", m_samplesEventMs, 64.f);
        // plotSamples("ImGui", " ms", m_samplesImGuiMs, 64.f);
        plotSamples("Display", " ms", m_samplesDisplayMs, 64.f);

        ImGui::Spacing();
        ImGui::Text("Draw calls: %u", m_lastFrameDrawCallCount);
        ImGui::Text("Drawn vertices: %zu", m_lastFrameDrawnVertices);

        ImGui::End();

        getActiveExample().imgui();

        ImGui::PopFont();

        m_samplesImGuiMs.record(m_clock.getElapsedTime().asSeconds() * 1000.f);
    }

    ////////////////////////////////////////////////////////////
    void runDraw()
    {
        m_clock.restart();

        m_rtGame.clear();
        getActiveExample().draw();

        m_samplesDrawMs.record(m_clock.getElapsedTime().asSeconds() * 1000.f);
    }

    ////////////////////////////////////////////////////////////
    void runDisplay()
    {
        m_clock.restart();

        const auto [drawCalls, drawnVertices] = m_rtGame.display();

        m_lastFrameDrawCallCount = drawCalls;
        m_lastFrameDrawnVertices = drawnVertices;

        m_window.clear();
        m_window.draw(m_rtGame.getTexture(), {.view = m_windowView});
        m_imGuiContext.render(m_window);
        m_window.display();

        m_samplesDisplayMs.record(m_clock.getElapsedTime().asSeconds() * 1000.f);
    }

public:
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool run()
    {
        while (true)
        {
            if (runEventHandling())
                return true;

            const za::Time deltaTime = runUpdate();

            runImGui(deltaTime);
            runDraw();
            runDisplay();

            m_samplesFPS.record(1.f / m_fpsClock.getElapsedTime().asSeconds());
        }

        return true;
    }
};

} // namespace


////////////////////////////////////////////////////////////
int main()
{
    auto audioContext    = za::AudioContext::create().value();
    auto graphicsContext = za::GraphicsContext::create().value();

    Game game;

    if (!game.run())
        return 1;

    return 0;
}
