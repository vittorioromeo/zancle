////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Shader.hpp"

#include "ExampleUtils/Scaling.hpp"

#include "Zancle/Graphics/Color.hpp"
#include "Zancle/Graphics/Font.hpp"
#include "Zancle/Graphics/GraphicsContext.hpp"
#include "Zancle/Graphics/PrimitiveType.hpp"
#include "Zancle/Graphics/RenderStates.hpp"
#include "Zancle/Graphics/RenderTarget.hpp"
#include "Zancle/Graphics/RenderTexture.hpp"
#include "Zancle/Graphics/RenderWindow.hpp"
#include "Zancle/Graphics/Sprite.hpp"
#include "Zancle/Graphics/Text.hpp"
#include "Zancle/Graphics/Texture.hpp"
#include "Zancle/Graphics/Vertex.hpp"

#include "Zancle/Window/Event.hpp"
#include "Zancle/Window/EventUtils.hpp"
#include "Zancle/Window/Keyboard.hpp"
#include "Zancle/Window/Mouse.hpp"

#include "Zancle/IO/Path.hpp"

#include "Zancle/Random/FastNonCryptoRng.hpp"

#include "Zancle/String/String.hpp"
#include "Zancle/String/Utf8String.hpp"

#include "Zancle/Chrono/Clock.hpp"
#include "Zancle/Chrono/Time.hpp"

#include "Zancle/Container/Array.hpp"
#include "Zancle/Container/Vector.hpp"

#include "Zancle/Geometry/Angle.hpp"
#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Math/Clamp.hpp"
#include "Zancle/Math/Cos.hpp"
#include "Zancle/Math/Fabs.hpp"
#include "Zancle/Math/Sin.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/Macros.hpp"
#include "Zancle/Base/SizeT.hpp"


namespace
{
////////////////////////////////////////////////////////////
za::FastNonCryptoRng rng(/* seed */ 1234);


////////////////////////////////////////////////////////////
// Base class for effects
////////////////////////////////////////////////////////////
struct Effect
{
    virtual ~Effect() = default;

    virtual void draw(za::RenderTarget& target, za::RenderStates states) const = 0;
    virtual void update(float time, float x, float y)                          = 0;
};


////////////////////////////////////////////////////////////
// "Pixelate" fragment shader
////////////////////////////////////////////////////////////
class Pixelate : public Effect
{
public:
    explicit Pixelate(za::Texture&& texture, za::Shader&& shader) :
        m_texture(ZA_MOVE(texture)),
        m_shader(ZA_MOVE(shader)),
        m_ulTexture(m_shader.getUniformLocation("za_u_texture").value()),
        m_ulPixelThreshold(m_shader.getUniformLocation("pixel_threshold").value())
    {
        m_shader.setUniform(m_ulTexture, za::Shader::CurrentTexture);
    }

    void update(float /* time */, float x, float y) override
    {
        m_shader.setUniform(m_ulPixelThreshold, (x + y) / 30);
    }

    void draw(za::RenderTarget& target, za::RenderStates states) const override
    {
        states.shader = &m_shader;
        target.draw(m_texture, states);
    }

private:
    za::Texture                 m_texture;
    za::Shader                  m_shader;
    za::Shader::UniformLocation m_ulTexture;
    za::Shader::UniformLocation m_ulPixelThreshold;
};


////////////////////////////////////////////////////////////
// "Wave" vertex shader + "blur" fragment shader
////////////////////////////////////////////////////////////
class WaveBlur : public Effect
{
public:
    void update(float time, float x, float y) override
    {
        m_shader.setUniform(m_ulWavePhase, time);
        m_shader.setUniform(m_ulWaveAmplitude, za::Vec2f(x * 40, y * 40));
        m_shader.setUniform(m_ulBlurRadius, (x + y) * 0.008f);
    }

    void draw(za::RenderTarget& target, za::RenderStates states) const override
    {
        states.shader = &m_shader;
        target.draw(m_text, states);
    }

    explicit WaveBlur(const za::Font& font, za::Shader&& shader) :
        m_text(font,
               {.position = {30.f, 20.f},
                .string = "Praesent suscipit augue in velit pulvinar hendrerit varius purus aliquam.\n"
                          "Mauris mi odio, bibendum quis fringilla a, laoreet vel orci. Proin vitae vulputate tortor.\n"
                          "Praesent cursus ultrices justo, ut feugiat ante vehicula quis.\n"
                          "Donec fringilla scelerisque mauris et viverra.\n"
                          "Maecenas adipiscing ornare scelerisque. Nullam at libero elit.\n"
                          "Pellentesque habitant morbi tristique senectus et netus et malesuada fames ac turpis "
                          "egestas.\n"
                          "Nullam leo urna, tincidunt id semper eget, ultricies sed mi.\n"
                          "Morbi mauris massa, commodo id dignissim vel, lobortis et elit.\n"
                          "Fusce vel libero sed neque scelerisque venenatis.\n"
                          "Integer mattis tincidunt quam vitae iaculis.\n"
                          "Vivamus fringilla sem non velit venenatis fermentum.\n"
                          "Vivamus varius tincidunt nisi id vehicula.\n"
                          "Integer ullamcorper, enim vitae euismod rutrum, massa nisl semper ipsum,\n"
                          "vestibulum sodales sem ante in massa.\n"
                          "Vestibulum in augue non felis convallis viverra.\n"
                          "Mauris ultricies dolor sed massa convallis sed aliquet augue fringilla.\n"
                          "Duis erat eros, porta in accumsan in, blandit quis sem.\n"
                          "In hac habitasse platea dictumst. Etiam fringilla est id odio dapibus sit amet semper dui "
                          "laoreet.\n",
                .characterSize = 22u}),
        m_shader(ZA_MOVE(shader)),
        m_ulWavePhase(m_shader.getUniformLocation("wave_phase").value()),
        m_ulWaveAmplitude(m_shader.getUniformLocation("wave_amplitude").value()),
        m_ulBlurRadius(m_shader.getUniformLocation("blur_radius").value())
    {
    }

private:
    za::Text                    m_text;
    za::Shader                  m_shader;
    za::Shader::UniformLocation m_ulWavePhase;
    za::Shader::UniformLocation m_ulWaveAmplitude;
    za::Shader::UniformLocation m_ulBlurRadius;
};


////////////////////////////////////////////////////////////
// "Storm" vertex shader + "blink" fragment shader
////////////////////////////////////////////////////////////
class StormBlink : public Effect
{
public:
    void update(float time, float x, float y) override
    {
        const float radius = 200 + za::cos(time) * 150;

        m_shader.setUniform(m_ulStormPosition, za::Vec2f(x * 800, y * 600));
        m_shader.setUniform(m_ulStormInnerRadius, radius / 3);
        m_shader.setUniform(m_ulStormTotalRadius, radius);
        m_shader.setUniform(m_ulBlinkAlpha, 0.5f + za::cos(time * 3) * 0.25f);
    }

    void draw(za::RenderTarget& target, za::RenderStates states) const override
    {
        states.shader = &m_shader;
        target.draw(m_points, za::PrimitiveType::Points, states);
    }

    explicit StormBlink(za::Shader&& shader) :
        m_shader(ZA_MOVE(shader)),
        m_ulStormPosition(m_shader.getUniformLocation("storm_position").value()),
        m_ulStormInnerRadius(m_shader.getUniformLocation("storm_inner_radius").value()),
        m_ulStormTotalRadius(m_shader.getUniformLocation("storm_total_radius").value()),
        m_ulBlinkAlpha(m_shader.getUniformLocation("blink_alpha").value())
    {
        // Create the points
        for (int i = 0; i < 40'000; ++i)
        {
            const auto x = rng.getF(0.f, 800.f);
            const auto y = rng.getF(0.f, 600.f);

            const auto r = rng.getI<za::U8>(0u, 255u);
            const auto g = rng.getI<za::U8>(0u, 255u);
            const auto b = rng.getI<za::U8>(0u, 255u);

            m_points.emplaceBack(za::Vec2f{x, y}, za::Color{r, g, b});
        }
    }

private:
    za::Vector<za::Vertex>      m_points;
    za::Shader                  m_shader;
    za::Shader::UniformLocation m_ulStormPosition;
    za::Shader::UniformLocation m_ulStormInnerRadius;
    za::Shader::UniformLocation m_ulStormTotalRadius;
    za::Shader::UniformLocation m_ulBlinkAlpha;
};


////////////////////////////////////////////////////////////
// "Edge" post-effect fragment shader
////////////////////////////////////////////////////////////
class Edge : public Effect
{
public:
    void update(float time, float x, float y) override
    {
        m_shader.setUniform(m_ulEdgeThreshold, za::clamp(1.f - (x + y) / 2.f, 0.f, 1.f));

        // Render the updated scene to the off-screen surface
        m_surface.clear(za::Color::White);
        m_surface.draw(m_backgroundTexture, {.position = {135.f, 100.f}});

        // Update the position of the moving entities
        constexpr int numEntities = 6;

        for (int i = 0; i < 6; ++i)
        {
            za::Sprite entity{.textureRect = {{96.f * static_cast<float>(i), 0.f}, {96.f, 96.f}}};

            entity.position =
                {za::cos(0.25f * (time * static_cast<float>(i) + static_cast<float>(numEntities - i))) * 300 + 350,
                 za::sin(0.25f * (time * static_cast<float>(numEntities - i) + static_cast<float>(i))) * 200 + 250};

            m_surface.draw(entity, {.texture = &m_entityTexture});
        }

        m_surface.display();
    }

    void draw(za::RenderTarget& target, za::RenderStates states) const override
    {
        const za::Texture& texture = m_surface.getTexture();

        states.shader = &m_shader;
        target.draw(texture, states);
    }

    explicit Edge(za::RenderTexture&& surface, za::Texture&& backgroundTexture, za::Texture&& entityTexture, za::Shader&& shader) :
        m_surface(ZA_MOVE(surface)),
        m_backgroundTexture(ZA_MOVE(backgroundTexture)),
        m_entityTexture(ZA_MOVE(entityTexture)),
        m_shader(ZA_MOVE(shader)),
        m_ulEdgeThreshold(m_shader.getUniformLocation("edge_threshold").value())
    {
    }

private:
    za::RenderTexture           m_surface;
    za::Texture                 m_backgroundTexture;
    za::Texture                 m_entityTexture;
    za::Shader                  m_shader;
    za::Shader::UniformLocation m_ulEdgeThreshold;
};


////////////////////////////////////////////////////////////
// "Geometry" geometry shader example
////////////////////////////////////////////////////////////
class Geometry : public Effect
{
public:
    void update(float /* time */, float x, float y) override
    {
        // Reset our transformation matrix
        m_transform = za::Transform::Identity;

        // Move to the center of the window
        m_transform.translate({400.f, 300.f});

        // Rotate everything based on cursor position
        m_transform.rotate(za::degrees(x * 360.f));

        // Adjust billboard size to scale between 25 and 75
        const float size = 25 + za::fabs(y) * 50;

        // Update the shader parameter
        m_shader.setUniform(m_ulSize, za::Vec2f{size, size});
    }

    void draw(za::RenderTarget& target, za::RenderStates states) const override
    {
        // Prepare the render state
        states.shader    = &m_shader;
        states.texture   = &m_logoTexture;
        states.transform = m_transform;

        // Draw the point cloud
        target.draw(m_pointCloud, za::PrimitiveType::Points, states);
    }

    explicit Geometry(za::Texture&& logoTexture, za::Shader&& shader) :
        m_logoTexture(ZA_MOVE(logoTexture)),
        m_shader(ZA_MOVE(shader)),
        m_ulSize(m_shader.getUniformLocation("size").value()),
        m_pointCloud(10'000)
    {
        // Move the points in the point cloud to random positions
        for (za::SizeT i = 0; i < 10'000; ++i)
        {
            // Spread the coordinates from -480 to +480 so they'll always fill the viewport at 800x600
            m_pointCloud[i].position = {rng.getF(-480.f, 480.f), rng.getF(-480.f, 480.f)};
        }
    }

private:
    za::Texture                 m_logoTexture;
    za::Transform               m_transform;
    za::Shader                  m_shader;
    za::Shader::UniformLocation m_ulSize;
    za::Vector<za::Vertex>      m_pointCloud;
};


////////////////////////////////////////////////////////////
// Effect loading factory functions
////////////////////////////////////////////////////////////
za::Optional<Pixelate> tryLoadPixelate()
{
    auto texture = za::Texture::loadFromFile("resources/background.jpg");
    if (!texture.hasValue())
        return za::nullOpt;

    auto shader = za::Shader::loadFromFile(
        {.vertexPath = "resources/billboard.vert", .fragmentPath = "resources/pixelate.frag"});
    if (!shader.hasValue())
        return za::nullOpt;

    return za::makeOptional<Pixelate>(ZA_MOVE(*texture), ZA_MOVE(*shader));
}

za::Optional<WaveBlur> tryLoadWaveBlur(const za::Font& font)
{
    auto shader = za::Shader::loadFromFile({.vertexPath = "resources/wave.vert", .fragmentPath = "resources/blur.frag"});
    if (!shader.hasValue())
        return za::nullOpt;

    return za::makeOptional<WaveBlur>(font, ZA_MOVE(*shader));
}

za::Optional<StormBlink> tryLoadStormBlink()
{
    auto shader = za::Shader::loadFromFile({.vertexPath = "resources/storm.vert", .fragmentPath = "resources/blink.frag"});
    if (!shader.hasValue())
        return za::nullOpt;

    return za::makeOptional<StormBlink>(ZA_MOVE(*shader));
}

za::Optional<Edge> tryLoadEdge()
{
    // Create the off-screen surface
    auto surface = za::RenderTexture::create({800, 600}, {.smooth = true});
    if (!surface.hasValue())
        return za::nullOpt;

    // Load the background texture
    auto backgroundTexture = za::Texture::loadFromFile("resources/zancle.png", {.smooth = true});
    if (!backgroundTexture.hasValue())
        return za::nullOpt;

    // Load the entity texture
    auto entityTexture = za::Texture::loadFromFile("resources/devices.png", {.smooth = true});
    if (!entityTexture.hasValue())
        return za::nullOpt;

    // Load the shader
    auto shader = za::Shader::loadFromFile(
        {.vertexPath = "resources/billboard.vert", .fragmentPath = "resources/edge.frag"});
    if (!shader.hasValue())
        return za::nullOpt;

    shader->setUniform(shader->getUniformLocation("za_u_texture").value(), za::Shader::CurrentTexture);

    return za::makeOptional<Edge>(ZA_MOVE(*surface), ZA_MOVE(*backgroundTexture), ZA_MOVE(*entityTexture), ZA_MOVE(*shader));
}

za::Optional<Geometry> tryLoadGeometry()
{
    // Check if geometry shaders are supported
    if (!za::Shader::isGeometryAvailable())
        return za::nullOpt;

    // Load the logo texture
    auto logoTexture = za::Texture::loadFromFile("resources/logo.png");
    if (!logoTexture.hasValue())
        return za::nullOpt;

    logoTexture->setSmooth(true);

    // Load the shader
    auto shader = za::Shader::loadFromFile({.vertexPath   = "resources/billboard.vert",
                                            .fragmentPath = "resources/billboard.frag",
                                            .geometryPath = "resources/billboard.geom"});
    if (!shader.hasValue())
        return za::nullOpt;

    shader->setUniform(shader->getUniformLocation("za_u_texture").value(), za::Shader::CurrentTexture);

    // Set the render resolution (used for proper scaling)
    shader->setUniform(shader->getUniformLocation("resolution").value(), za::Vec2f{800, 600});

    return za::makeOptional<Geometry>(ZA_MOVE(*logoTexture), ZA_MOVE(*shader));
}

} // namespace


////////////////////////////////////////////////////////////
/// Main
///
////////////////////////////////////////////////////////////
int main()
{
    // Create the graphics context
    auto graphicsContext = za::GraphicsContext::create().value();

    // Open the application font
    const auto font = za::Font::openFromFile("resources/tuffy.ttf").value();

    // Create the effects
    za::Optional pixelateEffect   = tryLoadPixelate();
    za::Optional waveBlurEffect   = tryLoadWaveBlur(font);
    za::Optional stormBlinkEffect = tryLoadStormBlink();
    za::Optional edgeEffect       = tryLoadEdge();
    za::Optional geometryEffect   = tryLoadGeometry();

    const za::Array<Effect*, 5> effects{pixelateEffect.asPtr(),
                                        waveBlurEffect.asPtr(),
                                        stormBlinkEffect.asPtr(),
                                        edgeEffect.asPtr(),
                                        geometryEffect.asPtr()};

    const za::Array<za::String, 5>
        effectNames{"Pixelate", "Wave + Blur", "Storm + Blink", "Edge Post-effect", "Geometry Shader Billboards"};

    // Index of currently selected effect
    za::SizeT current = 0u;

    // Create the messages background
    const auto textBackgroundTexture = za::Texture::loadFromFile("resources/text-background.png").value();

    // Create the description text
    za::Text description(font,
                         {.position         = {10.f, 530.f},
                          .string           = "Current effect: " + effectNames[current],
                          .characterSize    = 20u,
                          .fillColor        = {80, 80, 80},
                          .outlineColor     = za::Color::White,
                          .outlineThickness = 1.5f});


    // Create the instructions text
    za::Text instructions(font,
                          {.position         = {280.f, 555.f},
                           .string           = "Press left and right arrows to change the current shader",
                           .characterSize    = 20u,
                           .fillColor        = {80, 80, 80},
                           .outlineColor     = za::Color::White,
                           .outlineThickness = 1.5f});

    // Create the main window
    constexpr za::Vec2f windowSize{800.f, 600.f};

    auto window = makeDPIScaledRenderWindow(
                      {
                          .size      = windowSize.toVec2u(),
                          .title     = "Zancle Shader",
                          .resizable = true,
                          .vsync     = true,
                      })
                      .value();

    auto windowView = computeAspectRatioAwareView(window.getSize().toVec2f(), windowSize);

    // Start the game loop
    const za::Clock clock;

    while (true)
    {
        // Process events
        while (const za::Optional event = window.pollEvent())
        {
            if (za::EventUtils::isClosedOrEscapeKeyPressed(*event))
                return 0;

            if (handleAspectRatioAwareResize(*event, windowSize, windowView))
                continue;

            if (const auto* keyPressed = event->getIf<za::Event::KeyPressed>())
            {
                switch (keyPressed->code)
                {
                    // Left arrow key: previous shader
                    case za::Keyboard::Key::Left:
                    {
                        if (current == 0)
                            current = effects.size() - 1;
                        else
                            --current;

                        break;
                    }

                    // Right arrow key: next shader
                    case za::Keyboard::Key::Right:
                    {
                        if (current == effects.size() - 1)
                            current = 0;
                        else
                            ++current;

                        break;
                    }

                    default:
                        break;
                }

                description.setString("Current effect: " + effectNames[current]);
            }
        }

        // If the current example was loaded successfully...
        if (Effect* currentEffect = effects[current])
        {
            // Update the current example
            const auto [x, y] = za::Mouse::getPosition(window).toVec2f().componentWiseDiv(window.getSize().toVec2f());
            currentEffect->update(clock.getElapsedTime().asSeconds(), x, y);

            // Clear the window
            window.clear(currentEffect == edgeEffect.asPtr() ? za::Color::White : za::Color(50, 50, 50));

            // Draw the current example
            window.draw(*currentEffect, {.view = windowView});
        }
        else
        {
            // Clear the window to grey to make sure the text is always readable
            window.clear(za::Color(50, 50, 50));
            window.draw(za::Text{font, {.position = {320.f, 200.f}, .string = "Shader not\nsupported", .characterSize = 36u}},
                        {.view = windowView});
        }

        // Draw the text
        window.withRenderStates({.view = windowView})
            .draw(textBackgroundTexture, {.position = {0.f, 520.f}, .color = {255, 255, 255, 200}})
            .drawAll(instructions, description);

        // Finally, display the rendered frame on screen
        window.display();
    }
}
