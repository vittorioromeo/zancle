////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "ExampleUtils/Scaling.hpp"

#include "Zancle/Graphics/Color.hpp"
#include "Zancle/Graphics/Font.hpp"
#include "Zancle/Graphics/GraphicsContext.hpp"
#include "Zancle/Graphics/PrimitiveType.hpp"
#include "Zancle/Graphics/RenderStates.hpp"
#include "Zancle/Graphics/RenderWindow.hpp"
#include "Zancle/Graphics/Shader.hpp"
#include "Zancle/Graphics/Text.hpp"
#include "Zancle/Graphics/Vertex.hpp"
#include "Zancle/Graphics/VertexBuffer.hpp"

#include "Zancle/Window/Event.hpp"
#include "Zancle/Window/EventUtils.hpp"
#include "Zancle/Window/Keyboard.hpp"

#include "Zancle/Fmt/Fmt.hpp"
#include "Zancle/Fmt/FmtNumeric.hpp"

#include "Zancle/IO/Path.hpp"

#include "Zancle/Concurrency/Atomic.hpp"
#include "Zancle/Concurrency/Thread.hpp"
#include "Zancle/Concurrency/ThreadPool.hpp"

#include "Zancle/String/Utf8String.hpp"

#include "Zancle/Chrono/Clock.hpp"
#include "Zancle/Chrono/Time.hpp"

#include "Zancle/Container/Array.hpp"
#include "Zancle/Container/Vector.hpp"

#include "Zancle/Geometry/Vec2.hpp"
#include "Zancle/Geometry/Vec3.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Math/Clamp.hpp"
#include "Zancle/Math/MinMax.hpp"
#include "Zancle/Math/Pow.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/SizeT.hpp"

#define STB_PERLIN_IMPLEMENTATION
#include <stb_perlin.h>


namespace
{
// Width and height of the application window
constexpr za::Vec2u windowSize(800, 600);

// Resolution of the generated terrain
constexpr za::Vec2u resolution(800, 600);

// Thread pool parameters
constexpr unsigned int blockCount   = 32;
constexpr unsigned int rowBlockSize = (resolution.y / blockCount) + 1;

// Worker status
constinit bool                     bufferUploadPending = false;
constinit za::Atomic<unsigned int> pendingTasks{0u};

struct Setting
{
    const char* name{};
    float*      value{};
};

// Terrain noise parameters
constexpr int perlinOctaves = 3;

float perlinFrequency     = 7.f;
float perlinFrequencyBase = 4.f;

// Terrain generation parameters
float heightBase          = 0.f;
float edgeFactor          = 0.9f;
float edgeDropoffExponent = 1.5f;

float snowcapHeight = 0.6f;

// Terrain lighting parameters
float heightFactor  = static_cast<float>(windowSize.y) / 2.f;
float heightFlatten = 3.f;
float lightFactor   = 0.7f;


////////////////////////////////////////////////////////////
/// Get the terrain elevation at the given coordinates.
///
////////////////////////////////////////////////////////////
float getElevation(za::Vec2u position)
{
    const za::Vec2f normalized = position.toVec2f().componentWiseDiv(resolution.toVec2f()) - za::Vec2f(0.5f, 0.5f);

    float elevation = 0.f;

    for (int i = 0; i < perlinOctaves; ++i)
    {
        const za::Vec2f scaled = normalized * perlinFrequency *
                                 static_cast<float>(za::pow(perlinFrequencyBase, static_cast<float>(i)));
        elevation += stb_perlin_noise3(scaled.x, scaled.y, 0, 0, 0, 0) *
                     static_cast<float>(za::pow(perlinFrequencyBase, -static_cast<float>(i)));
    }

    elevation = (elevation + 1.f) / 2.f;

    const float distance = 2.f * normalized.length();
    elevation            = (elevation + heightBase) * (1.f - edgeFactor * za::pow(distance, edgeDropoffExponent));
    elevation            = za::clamp(elevation, 0.f, 1.f);

    return elevation;
}


////////////////////////////////////////////////////////////
/// Get the terrain moisture at the given coordinates.
///
////////////////////////////////////////////////////////////
float getMoisture(za::Vec2u position)
{
    const za::Vec2f normalized  = position.toVec2f().componentWiseDiv(resolution.toVec2f()) - za::Vec2f(0.5f, 0.5f);
    const za::Vec2f transformed = normalized * 4.f + za::Vec2f(0.5f, 0.5f);

    const float moisture = stb_perlin_noise3(transformed.x, transformed.y, 0, 0, 0, 0);

    return (moisture + 1.f) / 2.f;
}


////////////////////////////////////////////////////////////
/// Get the lowlands terrain color for the given moisture.
///
////////////////////////////////////////////////////////////
za::Color colorFromFloats(float r, float g, float b)
{
    return {static_cast<za::U8>(r), static_cast<za::U8>(g), static_cast<za::U8>(b)};
}

za::Color getLowlandsTerrainColor(float moisture)
{
    if (moisture < 0.27f)
        return colorFromFloats(240, 240, 180);

    if (moisture < 0.3f)
        return colorFromFloats(240 - (240 * (moisture - 0.27f) / 0.03f),
                               240 - (40 * (moisture - 0.27f) / 0.03f),
                               180 - (180 * (moisture - 0.27f) / 0.03f));

    if (moisture < 0.4f)
        return colorFromFloats(0, 200, 0);

    if (moisture < 0.48f)
        return colorFromFloats(0, 200 - (40 * (moisture - 0.4f) / 0.08f), 0);

    if (moisture < 0.6f)
        return colorFromFloats(0, 160, 0);

    if (moisture < 0.7f)
        return colorFromFloats((34 * (moisture - 0.6f) / 0.1f),
                               160 - (60 * (moisture - 0.6f) / 0.1f),
                               (34 * (moisture - 0.6f) / 0.1f));

    return colorFromFloats(34, 100, 34);
}


////////////////////////////////////////////////////////////
/// Get the highlands terrain color for the given elevation
/// and moisture.
///
////////////////////////////////////////////////////////////
za::Color getHighlandsTerrainColor(float elevation, float moisture)
{
    const za::Color lowlandsColor = getLowlandsTerrainColor(moisture);

    const za::Color color = moisture < 0.6f ? za::Color(112, 128, 144)
                                            : colorFromFloats(112 + (110 * (moisture - 0.6f) / 0.4f),
                                                              128 + (56 * (moisture - 0.6f) / 0.4f),
                                                              144 - (9 * (moisture - 0.6f) / 0.4f));

    const float factor = za::min((elevation - 0.4f) / 0.1f, 1.f);

    return colorFromFloats(lowlandsColor.r * (1.f - factor) + color.r * factor,
                           lowlandsColor.g * (1.f - factor) + color.g * factor,
                           lowlandsColor.b * (1.f - factor) + color.b * factor);
}


////////////////////////////////////////////////////////////
/// Get the snowcap terrain color for the given elevation
/// and moisture.
///
////////////////////////////////////////////////////////////
za::Color getSnowcapTerrainColor(float elevation, float moisture)
{
    const za::Color highlandsColor = getHighlandsTerrainColor(elevation, moisture);

    const float factor = za::min((elevation - snowcapHeight) / 0.05f, 1.f);

    return {static_cast<za::U8>(highlandsColor.r * (1.f - factor) + 255 * factor),
            static_cast<za::U8>(highlandsColor.g * (1.f - factor) + 255 * factor),
            static_cast<za::U8>(highlandsColor.b * (1.f - factor) + 255 * factor)};
}


////////////////////////////////////////////////////////////
/// Get the terrain color for the given elevation and
/// moisture.
///
////////////////////////////////////////////////////////////
za::Color getTerrainColor(float elevation, float moisture)
{
    if (elevation < 0.11f)
        return {0, 0, static_cast<za::U8>(elevation / 0.11f * 74.f + 181.f)};

    if (elevation < 0.14f)
        return {static_cast<za::U8>(za::pow((elevation - 0.11f) / 0.03f, 0.3f) * 48.f),
                static_cast<za::U8>(za::pow((elevation - 0.11f) / 0.03f, 0.3f) * 48.f),
                255};

    if (elevation < 0.16f)
        return {static_cast<za::U8>((elevation - 0.14f) * 128.f / 0.02f + 48.f),
                static_cast<za::U8>((elevation - 0.14f) * 128.f / 0.02f + 48.f),
                static_cast<za::U8>(127.f + (0.16f - elevation) * 128.f / 0.02f)};

    if (elevation < 0.17f)
        return {240, 230, 140};

    if (elevation < 0.4f)
        return getLowlandsTerrainColor(moisture);

    if (elevation < snowcapHeight)
        return getHighlandsTerrainColor(elevation, moisture);

    return getSnowcapTerrainColor(elevation, moisture);
}


////////////////////////////////////////////////////////////
/// Compute a compressed representation of the surface
/// normal based on the given coordinates, and the elevation
/// of the 4 adjacent neighbours.
///
////////////////////////////////////////////////////////////
za::Vec2f computeNormal(float left, float right, float bottom, float top)
{
    const za::Vec3f deltaX(1, 0, (za::pow(right, heightFlatten) - za::pow(left, heightFlatten)) * heightFactor);
    const za::Vec3f deltaY(0, 1, (za::pow(top, heightFlatten) - za::pow(bottom, heightFlatten)) * heightFactor);

    za::Vec3f crossProduct = deltaX.cross(deltaY);

    // Scale cross product to make z component 1.f so we can drop it
    crossProduct /= crossProduct.z;

    // Return "compressed" normal
    return {crossProduct.x, crossProduct.y};
}


////////////////////////////////////////////////////////////
/// Compute the vertex representing the terrain at the given
/// coordinates.
///
////////////////////////////////////////////////////////////
za::Vertex computeVertex(za::Vec2u position)
{
    static constexpr auto scalingFactors = windowSize.toVec2f().componentWiseDiv(resolution.toVec2f());

    return {.position  = position.toVec2f().componentWiseMul(scalingFactors),
            .color     = getTerrainColor(getElevation(position), getMoisture(position)),
            .texCoords = computeNormal(getElevation(position - za::Vec2u(1, 0)),
                                       getElevation(position + za::Vec2u(1, 0)),
                                       getElevation(position + za::Vec2u(0, 1)),
                                       getElevation(position - za::Vec2u(0, 1)))};
}


////////////////////////////////////////////////////////////
/// Process a terrain generation work item. Use the vector
/// of vertices as scratch memory and upload the data to
/// the vertex buffer when done.
///
////////////////////////////////////////////////////////////
void processWorkItem(za::Vertex* vertices, const unsigned int index)
{
    const unsigned int rowStart = rowBlockSize * index;

    if (rowStart >= resolution.y)
        return;

    const unsigned int rowEnd = za::min(rowStart + rowBlockSize, resolution.y);

    for (unsigned int y = rowStart; y < rowEnd; ++y)
    {
        for (unsigned int x = 0; x < resolution.x; ++x)
        {
            const unsigned int arrayIndexBase = ((y - rowStart) * resolution.x + x) * 6;

            // Top left corner (first triangle)
            if (x > 0)
                vertices[arrayIndexBase + 0] = vertices[arrayIndexBase - 6 + 5];
            else if (y > rowStart)
                vertices[arrayIndexBase + 0] = vertices[arrayIndexBase - resolution.x * 6 + 1];
            else
                vertices[arrayIndexBase + 0] = computeVertex({x, y});

            // Bottom left corner (first triangle)
            if (x > 0)
                vertices[arrayIndexBase + 1] = vertices[arrayIndexBase - 6 + 2];
            else
                vertices[arrayIndexBase + 1] = computeVertex({x, y + 1});

            // Bottom right corner (first triangle)
            vertices[arrayIndexBase + 2] = computeVertex({x + 1, y + 1});

            // Top left corner (second triangle)
            vertices[arrayIndexBase + 3] = vertices[arrayIndexBase + 0];

            // Bottom right corner (second triangle)
            vertices[arrayIndexBase + 4] = vertices[arrayIndexBase + 2];

            // Top right corner (second triangle)
            if (y > rowStart)
                vertices[arrayIndexBase + 5] = vertices[arrayIndexBase - resolution.x * 6 + 2];
            else
                vertices[arrayIndexBase + 5] = computeVertex({x + 1, y});
        }
    }
}


////////////////////////////////////////////////////////////
/// Terrain generation entry point. This queues up the
/// generation work items which the worker threads dequeue
/// and process.
///
////////////////////////////////////////////////////////////
void generateTerrain(za::ThreadPool& threadPool, za::Vertex* buffer)
{
    bufferUploadPending = true;

    // Make sure the work queue is empty before queuing new work
    while (pendingTasks.loadAcquire() > 0u)
        za::ThisThread::sleepFor(za::milliseconds(10));

    // Count the work items before queuing them, so that the counter never underflows
    pendingTasks.fetchAddRelease(blockCount);

    // Queue all the new work items at once
    za::ThreadPool::Task tasks[blockCount];

    for (unsigned int i = 0u; i < blockCount; ++i)
        tasks[i] = [buffer, i]
        {
            const unsigned int rowStart = rowBlockSize * i;
            processWorkItem(buffer + (resolution.x * rowStart * 6), i);
            pendingTasks.fetchSubRelease(1u);
        };

    threadPool.postBulk(tasks, blockCount);
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

    // Load the terrain shader
    auto terrainShader = za::Shader::loadFromFile(
                             {.vertexPath = "resources/terrain.vert", .fragmentPath = "resources/terrain.frag"})
                             .value();

    const auto ulLightFactor = terrainShader.getUniformLocation("lightFactor").value();

    // Load the font
    const auto font = za::Font::openFromFile("resources/tuffy.ttf").value();

    // Create the window of the application
    auto window = makeDPIScaledRenderWindow(
                      {
                          .size      = windowSize,
                          .title     = "Zancle Island",
                          .resizable = true,
                          .vsync     = true,
                      })
                      .value();

    auto windowView = computeAspectRatioAwareView(window.getSize().toVec2f(), windowSize.toVec2f());

    // Create all of our graphics resources
    za::Text hudText(font,
                     {.position         = {5.f, 5.f},
                      .string           = "",
                      .characterSize    = 14,
                      .fillColor        = za::Color::White,
                      .outlineColor     = za::Color::Black,
                      .outlineThickness = 2.f});

    za::Text statusText(font,
                        {.string           = "Generating Terrain...",
                         .characterSize    = 28,
                         .fillColor        = za::Color::White,
                         .outlineColor     = za::Color::Black,
                         .outlineThickness = 2.f});

    za::VertexBuffer terrain(za::PrimitiveType::Triangles, za::VertexBuffer::Usage::Static);

    // Staging buffer for our terrain data that we will upload to our VertexBuffer
    za::Vector<za::Vertex> terrainStagingBuffer;

    // Create a thread pool
    za::ThreadPool threadPool{za::ThreadPool::getHardwareWorkerCount()};

    // Create our VertexBuffer with enough space to hold all the terrain geometry
    if (!terrain.create(resolution.x * resolution.y * 6))
    {
        za::printErrLn("Failed to create vertex buffer");
        return 1;
    }

    // Resize the staging buffer to be able to hold all the terrain geometry
    terrainStagingBuffer.resize(resolution.x * resolution.y * 6);

    // Generate the initial terrain
    generateTerrain(threadPool, terrainStagingBuffer.data());

    // Center the status text
    statusText.position = (windowSize.toVec2f() - statusText.getLocalBounds().size) / 2.f;

    // Set up an array of pointers to our settings for arrow navigation
    constexpr za::Array<Setting, 9> settings = {
        {{"perlinFrequency", &perlinFrequency},
         {"perlinFrequencyBase", &perlinFrequencyBase},
         {"heightBase", &heightBase},
         {"edgeFactor", &edgeFactor},
         {"edgeDropoffExponent", &edgeDropoffExponent},
         {"snowcapHeight", &snowcapHeight},
         {"heightFactor", &heightFactor},
         {"heightFlatten", &heightFlatten},
         {"lightFactor", &lightFactor}}};

    za::SizeT currentSetting = 0;

    za::Utf8String hudBuf;
    za::Clock      clock;

    while (true)
    {
        // Handle events
        while (const za::Optional event = window.pollEvent())
        {
            if (za::EventUtils::isClosedOrEscapeKeyPressed(*event))
                return 0;

            if (handleAspectRatioAwareResize(*event, windowSize.toVec2f(), windowView))
                continue;

            // Arrow key pressed:
            if (event->is<za::Event::KeyPressed>())
            {
                switch (event->getIf<za::Event::KeyPressed>()->code)
                {
                    case za::Keyboard::Key::Enter:
                        generateTerrain(threadPool, terrainStagingBuffer.data());
                        break;
                    case za::Keyboard::Key::Down:
                        currentSetting = (currentSetting + 1) % settings.size();
                        break;
                    case za::Keyboard::Key::Up:
                        currentSetting = (currentSetting + settings.size() - 1) % settings.size();
                        break;
                    case za::Keyboard::Key::Left:
                        *(settings[currentSetting].value) -= 0.1f;
                        break;
                    case za::Keyboard::Key::Right:
                        *(settings[currentSetting].value) += 0.1f;
                        break;
                    default:
                        break;
                }
            }
        }

        // Clear, draw graphics objects and display
        window.clear();

        window.draw(statusText, {.view = windowView});

        // Don't bother updating/drawing the VertexBuffer while terrain is being regenerated
        if (pendingTasks.loadAcquire() == 0u)
        {
            // If there is new data pending to be uploaded to the VertexBuffer, do it now
            if (bufferUploadPending)
            {
                if (!terrain.update(terrainStagingBuffer.data()))
                {
                    za::printErrLn("Failed to update vertex buffer");
                    return 0;
                }

                bufferUploadPending = false;
            }

            terrainShader.setUniform(ulLightFactor, lightFactor);
            window.draw(terrain, {.view = windowView, .shader = &terrainShader});
        }

        // Update and draw the HUD text
        hudBuf.clear();
        (void)za::fmtTo(hudBuf,
                        "Frame:  {}ms\n"
                        "perlinOctaves:  {}\n\n"
                        "Use the arrow keys to change the values.\nUse the return key to regenerate the "
                        "terrain.\n\n",
                        clock.restart().asMilliseconds(),
                        perlinOctaves);

        for (za::SizeT i = 0; i < settings.size(); ++i)
            (void)za::fmtTo(hudBuf,
                            "{}{}:  {}\n",
                            (i == currentSetting) ? ">>  " : "       ",
                            settings[i].name,
                            *(settings[i].value));

        hudText.setString(hudBuf);

        window.draw(hudText, {.view = windowView});

        // Display things on screen
        window.display();
    }
}
