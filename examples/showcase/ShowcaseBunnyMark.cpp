#include "ShowcaseBunnyMark.hpp"
#include "ShowcaseExample.hpp"

#include "Zancle/Graphics/Color.hpp"
#include "Zancle/Graphics/DefaultShader.hpp"
#include "Zancle/Graphics/DrawInstancedIndexedVerticesSettings.hpp"
#include "Zancle/Graphics/Font.hpp"
#include "Zancle/Graphics/InstanceAttributeBinder.hpp"
#include "Zancle/Graphics/InstancedQuad.hpp"
#include "Zancle/Graphics/PrimitiveType.hpp"
#include "Zancle/Graphics/RenderStates.hpp"
#include "Zancle/Graphics/RenderTarget.hpp"
#include "Zancle/Graphics/Sprite.hpp"
#include "Zancle/Graphics/TextData.hpp"
#include "Zancle/Graphics/TextureAtlas.hpp"
#include "Zancle/Graphics/VertexSpan.hpp"

#include "Zancle/ImGui/IncludeImGui.hpp"

#include "Zancle/Window/Keyboard.hpp"

#include "Zancle/String/String.hpp"
#include "Zancle/String/ToString.hpp"

#include "Zancle/Geometry/Angle.hpp"
#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Vocabulary/Span.hpp"

#include "Zancle/Math/Clamp.hpp"
#include "Zancle/Math/Constants.hpp"
#include "Zancle/Math/Sin.hpp"

#include "Zancle/Base/SizeT.hpp"


////////////////////////////////////////////////////////////
constexpr const char* bunnyInstancedVertexShader = R"glsl(

layout(location = 0) uniform vec3 za_u_mvpRow0;
layout(location = 1) uniform vec3 za_u_mvpRow1;
layout(location = 2) uniform sampler2D za_u_texture;
layout(location = 3) uniform vec2 za_u_invTextureSize;

layout(location = 0) in vec2 za_a_position;
layout(location = 1) in vec4 za_a_color;
layout(location = 2) in vec2 za_a_texCoord;

// Per-instance attributes
layout(location = 3) in vec2 instance_position;
layout(location = 4) in float instance_scale;
layout(location = 5) in float instance_rotation;
layout(location = 6) in vec2 instance_texRectPos;
layout(location = 7) in vec2 instance_texRectSize;

out vec4 za_v_color;
out vec2 za_v_texCoord;

void main()
{
    vec2 local = za_a_position * instance_texRectSize;

    float c = cos(instance_rotation);
    float s = sin(instance_rotation);
    float x = local.x * c - local.y * s;
    float y = local.x * s + local.y * c;
    vec2 worldPos = instance_position + instance_scale * vec2(x, y);

    gl_Position = vec4(dot(za_u_mvpRow0, vec3(worldPos, 1.0)),
                       dot(za_u_mvpRow1, vec3(worldPos, 1.0)), 0.0, 1.0);

    za_v_color = vec4(1.0, 1.0, 1.0, 1.0);

    vec2 final_texCoord = instance_texRectPos + (za_a_texCoord * instance_texRectSize);
    za_v_texCoord = final_texCoord * za_u_invTextureSize;
}

)glsl";


////////////////////////////////////////////////////////////
za::Rect2f ExampleBunnyMark::addImgToAtlasWithRotatedHue(const za::Path& path, const float hueDegrees)
{
    auto img = za::Image::loadFromFile(path).value();
    img.rotateHue(hueDegrees);
    return m_textureAtlas.add(img).value();
}


////////////////////////////////////////////////////////////
za::String ExampleBunnyMark::toDigitSeparatedString(const za::SizeT value)
{
    auto s = za::toString(value);

    for (int i = static_cast<int>(s.size()) - 3; i > 0; i -= 3)
        s.insert(static_cast<za::SizeT>(i), ".");

    return s;
}


////////////////////////////////////////////////////////////
ExampleBunnyMark::ExampleBunnyMark(const GameDependencies& deps, za::TextureAtlas& textureAtlas) :
    ShowcaseExample{"Bunnymark"},
    m_deps{deps},
    m_textureAtlas{textureAtlas},
    m_bunnyTextureRects{
        addImgToAtlasWithRotatedHue("resources/bunny0.png", 0.f),
        addImgToAtlasWithRotatedHue("resources/bunny0.png", 45.f),
        addImgToAtlasWithRotatedHue("resources/bunny0.png", 90.f),
        addImgToAtlasWithRotatedHue("resources/bunny0.png", 135.f),
        addImgToAtlasWithRotatedHue("resources/bunny0.png", 180.f),
        addImgToAtlasWithRotatedHue("resources/bunny0.png", 225.f),
        addImgToAtlasWithRotatedHue("resources/bunny0.png", 270.f),
        addImgToAtlasWithRotatedHue("resources/bunny0.png", 315.f),
    }
{
    // Set up instanced rendering shader
    m_instancedShader.emplace(za::Shader::loadFromMemory({.vertexCode = bunnyInstancedVertexShader}).value());

    m_ulInvTexSize.emplace(m_instancedShader->getUniformLocation("za_u_invTextureSize").value());
    m_instancedShader->setUniform(*m_ulInvTexSize, 1.f / m_textureAtlas.getTexture().getSize().toVec2f());
}


////////////////////////////////////////////////////////////
void ExampleBunnyMark::update(const float deltaTimeMs)
{
    m_time += deltaTimeMs;

    if (za::Keyboard::isKeyPressed(za::Keyboard::Key::Right))
        m_bunnyTargetCount += 5000;
    else if (za::Keyboard::isKeyPressed(za::Keyboard::Key::Left))
        m_bunnyTargetCount -= 5000;

    m_bunnyTargetCount = za::clamp(m_bunnyTargetCount, za::SizeT{5000}, za::SizeT{2'500'000});

    if (m_bunnies.size() < m_bunnyTargetCount)
    {
        m_bunnies.reserve(m_bunnyTargetCount);

        for (za::SizeT i = m_bunnies.size(); i < m_bunnyTargetCount; ++i)
        {
            m_bunnies.emplaceBack(
                /* position */ m_rng.getVec2f(resolution),
                /* velocity */ m_rng.getVec2f({-1.f, -1.f}, {1.f, 1.f}),
                /* rotation */ za::radians(m_rng.getF(0.f, za::tau)),
                /*    scale */ m_rng.getF(0.25f, 0.5f));
        }
    }
    else if (m_bunnies.size() > m_bunnyTargetCount)
    {
        m_bunnies.resize(m_bunnyTargetCount);
    }

    for (auto& [position, velocity, rotation, scale] : m_bunnies)
    {
        position += velocity * deltaTimeMs;

        if (position.x < 0.f)
            position.x = resolution.x;
        else if (position.x > resolution.x)
            position.x = 0.f;

        if (position.y < 0.f)
            position.y = resolution.y;
        else if (position.y > resolution.y)
            position.y = 0.f;

        rotation += za::radians(0.05f * deltaTimeMs);

        if (rotation >= za::Angle::Full)
            rotation -= za::Angle::Full;
    }
}


////////////////////////////////////////////////////////////
void ExampleBunnyMark::imgui()
{
    ImGui::Begin("BunnyMark", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

    constexpr const char* modeNames[] = {"Normal", "Instanced"};
    int                   modeIndex   = static_cast<int>(m_drawMode);

    if (ImGui::Combo("Draw mode", &modeIndex, modeNames, 2))
        m_drawMode = static_cast<DrawMode>(modeIndex);

    ImGui::SliderFloat("Scale multiplier", &m_scaleMultiplier, 0.01f, 2.5f, "%.3f", ImGuiSliderFlags_NoRoundToFormat);

    ImGui::End();
}


////////////////////////////////////////////////////////////
void ExampleBunnyMark::drawInstanced()
{
    const auto  nBunnies = m_bunnies.size();
    const float scaleMul = m_scaleMultiplier;

    m_instanceData.resize(nBunnies);

    for (za::SizeT i = 0u; i < nBunnies; ++i)
    {
        const auto& [position, velocity, rotation, scale] = m_bunnies[i];
        const auto& txr                                   = m_bunnyTextureRects[i % 8u];

        m_instanceData[i] = {position, scale * scaleMul, rotation.asRadians(), txr.position, txr.size};
    }

    auto setupAttribs = [&](za::InstanceAttributeBinder& binder)
    {
        binder.uploadContiguousData(m_instanceVBO, m_instanceData);

        binder.setupField<&BunnyInstanceData::position>(3);
        binder.setupField<&BunnyInstanceData::scale>(4);
        binder.setupField<&BunnyInstanceData::rotation>(5);
        binder.setupField<&BunnyInstanceData::texRectPos>(6);
        binder.setupField<&BunnyInstanceData::texRectSize>(7);
    };

    m_deps.rtGame->drawInstancedIndexedVertices(
        {
            .vaoHandle     = m_vaoHandle,
            .vertexSpan    = za::instancedQuadVertices,
            .indexSpan     = za::instancedQuadIndices,
            .instanceCount = nBunnies,
            .primitiveType = za::PrimitiveType::Triangles,
        },
        setupAttribs,
        {.view = *m_deps.view, .texture = &m_textureAtlas.getTexture(), .shader = &*m_instancedShader});
}


////////////////////////////////////////////////////////////
void ExampleBunnyMark::draw()
{
    if (m_drawMode == DrawMode::Instanced)
    {
        drawInstanced();
    }
    else
    {
        m_deps.rtGame->reserveAutoBatchQuads(m_bunnies.size());

        const auto drawCtx = m_deps.rtGame->withLockedRenderStates(
            {.view = *m_deps.view, .texture = &m_textureAtlas.getTexture()});

        const float scaleMul = m_scaleMultiplier;

        za::SizeT i = 0;

        for (auto& [position, velocity, rotation, scale] : m_bunnies)
        {
            const auto& txr = m_bunnyTextureRects[i % 8u];

            const float s = scale * scaleMul;

            drawCtx.draw(za::Sprite{
                .position    = position,
                .scale       = {s, s},
                .origin      = txr.size / 2.f,
                .rotation    = rotation,
                .textureRect = txr,
            });

            ++i;
        }
    }

    const auto digitSeparatedBunnyCount = toDigitSeparatedString(m_bunnies.size());

    const auto vertices = m_deps.rtGame->draw(*m_deps.font,
                                              za::TextData{
                                                  .position         = {8.f, 8.f},
                                                  .string           = digitSeparatedBunnyCount + " bunnies",
                                                  .characterSize    = 32,
                                                  .outlineColor     = za::Color::Black,
                                                  .outlineThickness = 4.f,
                                              },
                                              {.view = *m_deps.view}); // TODO P1: add a way to prevent flushing

    const auto applyEffect = [&](za::VertexSpan quads)
    {
        for (za::SizeT j = 0u; j < quads.size(); j += 4u)
        {
            if (j >= digitSeparatedBunnyCount.size() * 4u)
            {
                const float offY = za::sin(m_time) * 1.25f;

                quads[j + 0].position.y -= offY;
                quads[j + 1].position.y -= offY;
                quads[j + 2].position.y += offY;
                quads[j + 3].position.y += offY;

                quads[j + 0].position.x -= offY;
                quads[j + 1].position.x += offY;
                quads[j + 2].position.x -= offY;
                quads[j + 3].position.x += offY;
            }
            else
            {
                const float offY = za::sin(m_time + static_cast<float>(j)) * 1.5f;

                quads[j + 0].position.y += offY;
                quads[j + 1].position.y += offY;
                quads[j + 2].position.y += offY;
                quads[j + 3].position.y += offY;
            }
        }
    };

    applyEffect(vertices.outline);
    applyEffect(vertices.fill);

    m_deps.rtGame->draw(*m_deps.font,
                        za::TextData{
                            .position         = {8.f, 48.f},
                            .string           = "Change number of bunnies with arrow keys",
                            .characterSize    = 16,
                            .outlineColor     = za::Color::Black,
                            .outlineThickness = 2.f,
                        },
                        {.view = *m_deps.view});
}
