#include "GraphicsUtil.hpp"
#include "Tst/Tst.hpp"
#include "WindowUtil.hpp"

#include "Zancle/GLUtils/GLCheck.hpp"
#include "Zancle/GLUtils/GLVAOGroup.hpp"
#include "Zancle/GLUtils/Glad.hpp"

#include "Zancle/Graphics/CircleShape.hpp"
#include "Zancle/Graphics/Color.hpp"
#include "Zancle/Graphics/DepthStencilFormat.hpp"
#include "Zancle/Graphics/DrawInstancedIndexedVerticesSettings.hpp"
#include "Zancle/Graphics/DrawableBatch.hpp"
#include "Zancle/Graphics/Font.hpp"
#include "Zancle/Graphics/Glsl.hpp"
#include "Zancle/Graphics/GraphicsContext.hpp"
#include "Zancle/Graphics/Image.hpp"
#include "Zancle/Graphics/InstanceAttributeBinder.hpp"
#include "Zancle/Graphics/InstancedQuad.hpp"
#include "Zancle/Graphics/PrimitiveType.hpp"
#include "Zancle/Graphics/RectangleShape.hpp"
#include "Zancle/Graphics/RenderStates.hpp"
#include "Zancle/Graphics/RenderTexture.hpp"
#include "Zancle/Graphics/Shader.hpp"
#include "Zancle/Graphics/StencilMode.hpp"
#include "Zancle/Graphics/Text.hpp"
#include "Zancle/Graphics/Texture.hpp"
#include "Zancle/Graphics/VAOHandle.hpp"
#include "Zancle/Graphics/VBOHandle.hpp"
#include "Zancle/Graphics/Vertex.hpp"
#include "Zancle/Graphics/VertexBuffer.hpp"
#include "Zancle/Graphics/View.hpp"

#include "Zancle/Window/WindowContext.hpp"

#include "Zancle/Concurrency/Thread.hpp"

#include "Zancle/Geometry/Angle.hpp"
#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Math/Fabs.hpp"


// Shared multi-context test helper, used by both this file and
// `test/Window/Context.test.cpp`.
#include "TestContext.hpp"


namespace
{
constexpr auto blinkAlphaFragSource = R"glsl(
layout(location = 2) uniform sampler2D za_u_texture;
layout(location = 7) uniform float blink_alpha;

in vec4 za_v_color;
in vec2 za_v_texCoord;

layout(location = 0) out vec4 za_fragColor;

void main()
{
    za_fragColor = za_v_color * blink_alpha;
}
)glsl";

constexpr auto instancedVertexSource = R"glsl(
layout(location = 0) uniform vec3 za_u_mvpRow0;
layout(location = 1) uniform vec3 za_u_mvpRow1;
layout(location = 3) uniform vec2 za_u_invTextureSize;

layout(location = 0) in vec2 za_a_position;
layout(location = 1) in vec4 za_a_color;
layout(location = 2) in vec2 za_a_texCoord;

layout(location = 3) in vec2 instance_offset;
layout(location = 4) in vec4 instance_color;

out vec4 v_color;

void main()
{
    vec2 worldPos = instance_offset + (za_a_position * vec2(20.0, 20.0));

    gl_Position = vec4(dot(za_u_mvpRow0, vec3(worldPos, 1.0)),
                       dot(za_u_mvpRow1, vec3(worldPos, 1.0)),
                       0.0,
                       1.0);

    v_color = instance_color;
}
)glsl";

constexpr auto instancedFragmentSource = R"glsl(
in vec4 v_color;

layout(location = 0) out vec4 za_fragColor;

void main()
{
    za_fragColor = v_color;
}
)glsl";

} // namespace


TEST_CASE("[Graphics] Render Tests" * tst::skip(skipDisplayTests))
{
    [[maybe_unused]] auto& graphicsContext = TST_CASE_SHARED(za::GraphicsContext::create().value());

    SECTION("Stencil Tests")
    {
        auto renderTexture = za::RenderTexture::create({100, 100}, {.depthStencilFormat = za::DepthStencilFormat::Stencil8})
                                 .value();

        renderTexture.clear(za::Color::Red, za::StencilValue{127u});

        const za::RectangleShape shape1{{.fillColor = za::Color::Green, .size = {100.f, 100.f}}};
        const za::RectangleShape shape2{{.fillColor = za::Color::Blue, .size = {100.f, 100.f}}};

        SECTION("Stencil-Only")
        {
            renderTexture.draw(shape1,
                               za::RenderStates{.stencilMode = {
                                                    .stencilComparison      = za::StencilComparison::Always,
                                                    .stencilUpdateOperation = za::StencilUpdateOperation::Keep,
                                                    .stencilOnly            = true,
                                                    .stencilReference       = za::StencilValue{1u},
                                                    .stencilMask            = za::StencilValue{0xFFu},
                                                }});
            renderTexture.display();
            CHECK(renderTexture.getTexture().copyToImage().getPixel({50, 50}) == za::Color::Red);
        }

        SECTION("Comparisons")
        {
            SECTION("Always")
            {
                renderTexture.draw(shape1,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::Always,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Keep,
                                                        .stencilOnly            = false,
                                                        .stencilReference       = za::StencilValue{1u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.display();
                CHECK(renderTexture.getTexture().copyToImage().getPixel({50, 50}) == za::Color::Green);
            }

            SECTION("Equal")
            {
                renderTexture.draw(shape1,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::Equal,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Keep,
                                                        .stencilOnly            = false,
                                                        .stencilReference       = za::StencilValue{126u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.display();
                CHECK(renderTexture.getTexture().copyToImage().getPixel({50, 50}) == za::Color::Red);

                renderTexture.draw(shape1,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::Equal,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Keep,
                                                        .stencilOnly            = false,
                                                        .stencilReference       = za::StencilValue{127u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.display();
                CHECK(renderTexture.getTexture().copyToImage().getPixel({50, 50}) == za::Color::Green);
            }

            SECTION("Greater")
            {
                renderTexture.draw(shape1,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::Greater,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Keep,
                                                        .stencilOnly            = false,
                                                        .stencilReference       = za::StencilValue{126u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.display();
                CHECK(renderTexture.getTexture().copyToImage().getPixel({50, 50}) == za::Color::Red);
                renderTexture.draw(shape1,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::Greater,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Keep,
                                                        .stencilOnly            = false,
                                                        .stencilReference       = za::StencilValue{127u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.display();
                CHECK(renderTexture.getTexture().copyToImage().getPixel({50, 50}) == za::Color::Red);
                renderTexture.draw(shape1,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::Greater,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Keep,
                                                        .stencilOnly            = false,
                                                        .stencilReference       = za::StencilValue{128u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.display();
                CHECK(renderTexture.getTexture().copyToImage().getPixel({50, 50}) == za::Color::Green);
            }

            SECTION("GreaterEqual")
            {
                renderTexture.draw(shape1,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::GreaterEqual,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Keep,
                                                        .stencilOnly            = false,
                                                        .stencilReference       = za::StencilValue{126u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.display();
                CHECK(renderTexture.getTexture().copyToImage().getPixel({50, 50}) == za::Color::Red);
                renderTexture.draw(shape1,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::GreaterEqual,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Keep,
                                                        .stencilOnly            = false,
                                                        .stencilReference       = za::StencilValue{127u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.display();
                CHECK(renderTexture.getTexture().copyToImage().getPixel({50, 50}) == za::Color::Green);
                renderTexture.draw(shape2,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::GreaterEqual,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Keep,
                                                        .stencilOnly            = false,
                                                        .stencilReference       = za::StencilValue{128u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.display();

                CHECK(renderTexture.getTexture().copyToImage().getPixel({50, 50}) == za::Color::Blue);
            }

            SECTION("Less")
            {
                renderTexture.draw(shape1,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::Less,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Keep,
                                                        .stencilOnly            = false,
                                                        .stencilReference       = za::StencilValue{128u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.display();
                CHECK(renderTexture.getTexture().copyToImage().getPixel({50, 50}) == za::Color::Red);
                renderTexture.draw(shape1,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::Less,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Keep,
                                                        .stencilOnly            = false,
                                                        .stencilReference       = za::StencilValue{127u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.display();
                CHECK(renderTexture.getTexture().copyToImage().getPixel({50, 50}) == za::Color::Red);
                renderTexture.draw(shape1,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::Less,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Keep,
                                                        .stencilOnly            = false,
                                                        .stencilReference       = za::StencilValue{126u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.display();
                CHECK(renderTexture.getTexture().copyToImage().getPixel({50, 50}) == za::Color::Green);
            }

            SECTION("LessEqual")
            {
                renderTexture.draw(shape1,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::LessEqual,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Keep,
                                                        .stencilOnly            = false,
                                                        .stencilReference       = za::StencilValue{128u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.display();
                CHECK(renderTexture.getTexture().copyToImage().getPixel({50, 50}) == za::Color::Red);
                renderTexture.draw(shape1,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::LessEqual,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Keep,
                                                        .stencilOnly            = false,
                                                        .stencilReference       = za::StencilValue{127u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.display();
                CHECK(renderTexture.getTexture().copyToImage().getPixel({50, 50}) == za::Color::Green);
                renderTexture.draw(shape2,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::LessEqual,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Keep,
                                                        .stencilOnly            = false,
                                                        .stencilReference       = za::StencilValue{126u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.display();

                CHECK(renderTexture.getTexture().copyToImage().getPixel({50, 50}) == za::Color::Blue);
            }

            SECTION("Never")
            {
                renderTexture.draw(shape1,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::Never,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Keep,
                                                        .stencilOnly            = false,
                                                        .stencilReference       = za::StencilValue{127u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.display();
                CHECK(renderTexture.getTexture().copyToImage().getPixel({50, 50}) == za::Color::Red);
            }

            SECTION("NotEqual")
            {
                renderTexture.draw(shape1,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::NotEqual,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Keep,
                                                        .stencilOnly            = false,
                                                        .stencilReference       = za::StencilValue{127u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.display();
                CHECK(renderTexture.getTexture().copyToImage().getPixel({50, 50}) == za::Color::Red);
                renderTexture.draw(shape1,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::NotEqual,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Keep,
                                                        .stencilOnly            = false,
                                                        .stencilReference       = za::StencilValue{128u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.display();
                CHECK(renderTexture.getTexture().copyToImage().getPixel({50, 50}) == za::Color::Green);
            }
        }

        SECTION("Updating")
        {
            SECTION("Decrement")
            {
                renderTexture.draw(shape1,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::Always,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Decrement,
                                                        .stencilOnly            = true,
                                                        .stencilReference       = za::StencilValue{127u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.draw(shape1,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::Equal,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Decrement,
                                                        .stencilOnly            = false,
                                                        .stencilReference       = za::StencilValue{126u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.display();
                CHECK(renderTexture.getTexture().copyToImage().getPixel({50, 50}) == za::Color::Green);
            }

            SECTION("Increment")
            {
                renderTexture.draw(shape1,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::Always,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Increment,
                                                        .stencilOnly            = true,
                                                        .stencilReference       = za::StencilValue{127u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.draw(shape1,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::Equal,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Increment,
                                                        .stencilOnly            = false,
                                                        .stencilReference       = za::StencilValue{128u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.display();
                CHECK(renderTexture.getTexture().copyToImage().getPixel({50, 50}) == za::Color::Green);
            }

            SECTION("Invert")
            {
                renderTexture.draw(shape1,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::Always,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Invert,
                                                        .stencilOnly            = true,
                                                        .stencilReference       = za::StencilValue{127u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.draw(shape1,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::Equal,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Invert,
                                                        .stencilOnly            = false,
                                                        .stencilReference       = za::StencilValue{0x80u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.display();
                CHECK(renderTexture.getTexture().copyToImage().getPixel({50, 50}) == za::Color::Green);
            }

            SECTION("Keep")
            {
                renderTexture.draw(shape1,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::Always,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Keep,
                                                        .stencilOnly            = true,
                                                        .stencilReference       = za::StencilValue{127u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.draw(shape1,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::Equal,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Keep,
                                                        .stencilOnly            = false,
                                                        .stencilReference       = za::StencilValue{127u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.display();
                CHECK(renderTexture.getTexture().copyToImage().getPixel({50, 50}) == za::Color::Green);
            }

            SECTION("Replace")
            {
                renderTexture.draw(shape1,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::Always,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Replace,
                                                        .stencilOnly            = true,
                                                        .stencilReference       = za::StencilValue{255u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.draw(shape1,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::Equal,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Replace,
                                                        .stencilOnly            = false,
                                                        .stencilReference       = za::StencilValue{255u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.display();
                CHECK(renderTexture.getTexture().copyToImage().getPixel({50, 50}) == za::Color::Green);
            }

            SECTION("Zero")
            {
                renderTexture.draw(shape1,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::Always,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Zero,
                                                        .stencilOnly            = true,
                                                        .stencilReference       = za::StencilValue{127u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.draw(shape1,
                                   za::RenderStates{.stencilMode = {
                                                        .stencilComparison      = za::StencilComparison::Equal,
                                                        .stencilUpdateOperation = za::StencilUpdateOperation::Zero,
                                                        .stencilOnly            = false,
                                                        .stencilReference       = za::StencilValue{0u},
                                                        .stencilMask            = za::StencilValue{0xFFu},
                                                    }});
                renderTexture.display();
                CHECK(renderTexture.getTexture().copyToImage().getPixel({50, 50}) == za::Color::Green);
            }
        }
    }

    SECTION("Auto-batch generation counter tests")
    {
        auto renderTexture = za::RenderTexture::create({100, 100}).value();

        const za::RectangleShape shape{{.fillColor = za::Color::Red, .size = {100.f, 100.f}}};

        SECTION("Shader uniform mutation breaks batch")
        {
            auto shader = za::Shader::loadFromMemory({.fragmentCode = blinkAlphaFragSource}).value();

            const auto loc = shader.getUniformLocation("blink_alpha").value();
            shader.setUniform(loc, 1.f);

            const za::RenderStates states{.shader = &shader};

            // Two draws without mutation: single batch, 1 draw call
            renderTexture.clear();
            renderTexture.draw(shape, states);
            renderTexture.draw(shape, states);
            CHECK(renderTexture.display().drawCalls == 1u);

            // Two draws with uniform mutation in between: batch broken, 2 draw calls
            renderTexture.clear();
            renderTexture.draw(shape, states);
            shader.setUniform(loc, 0.5f);
            renderTexture.draw(shape, states);
            CHECK(renderTexture.display().drawCalls == 2u);
        }

        SECTION("Manual flush before uniform mutation preserves correct rendering")
        {
            auto shader = za::Shader::loadFromMemory({.fragmentCode = blinkAlphaFragSource}).value();

            const auto loc = shader.getUniformLocation("blink_alpha").value();

            const za::RenderStates states{.shader = &shader};

            // Correct order: flush, then mutate, then draw
            // The flush drains the pending batch with the OLD uniform value,
            // so each batch sees its own intended uniform.
            renderTexture.clear();
            shader.setUniform(loc, 1.f);
            renderTexture.draw(shape, states); // batch 1: alpha=1
            renderTexture.flush();             // drain batch 1 with alpha=1
            shader.setUniform(loc, 0.5f);      // now safe to mutate
            renderTexture.draw(shape, states); // batch 2: alpha=0.5
            CHECK(renderTexture.display().drawCalls == 2u);

            // Wrong order: mutate, then flush -- the pending batch is
            // drawn with the ALREADY-MUTATED uniform (corruption).
            // The generation counter must detect this and break the batch.
            renderTexture.clear();
            shader.setUniform(loc, 1.f);
            renderTexture.draw(shape, states); // batch 1: intended alpha=1
            shader.setUniform(loc, 0.5f);      // oops -- mutated before flush
            renderTexture.flush();             // draws batch 1 with alpha=0.5 (wrong!)
            renderTexture.draw(shape, states); // batch 2: alpha=0.5
            CHECK(renderTexture.display().drawCalls == 2u);
        }

        SECTION("Flush before display is redundant (display already flushes)")
        {
            auto shader = za::Shader::loadFromMemory({.fragmentCode = blinkAlphaFragSource}).value();

            const auto loc = shader.getUniformLocation("blink_alpha").value();

            const za::RenderStates states{.shader = &shader};

            // With redundant flush before display: uniform set before draw,
            // then flush (no-op since display would do it), then display.
            renderTexture.clear();
            shader.setUniform(loc, 1.f);
            renderTexture.draw(shape, states);
            renderTexture.flush();
            CHECK(renderTexture.display().drawCalls == 1u);

            // Without flush: display handles it identically.
            renderTexture.clear();
            shader.setUniform(loc, 0.5f);
            renderTexture.draw(shape, states);
            CHECK(renderTexture.display().drawCalls == 1u);
        }

        SECTION("Repeated instanced draws keep base vertex attributes bound to the shared quad VBO")
        {
            struct InstanceData
            {
                za::Vec2f offset;
                za::Color color;
            };

            auto shader = za::Shader::loadFromMemory(
                              {.vertexCode = instancedVertexSource, .fragmentCode = instancedFragmentSource})
                              .value();

            za::VAOHandle vaoHandle;
            za::VBOHandle instanceVBO;

            const auto drawInstance = [&](InstanceData instance)
            {
                auto setupAttribs = [&](za::InstanceAttributeBinder& binder)
                {
                    binder.uploadContiguousData(instanceVBO, &instance);
                    binder.setupField<&InstanceData::offset>(3);
                    binder.setupField<&InstanceData::color>(4);
                };

                renderTexture.drawInstancedIndexedVertices(
                    {
                        .vaoHandle     = vaoHandle,
                        .vertexSpan    = za::instancedQuadVertices,
                        .indexSpan     = za::instancedQuadIndices,
                        .instanceCount = 1u,
                        .primitiveType = za::PrimitiveType::Triangles,
                    },
                    setupAttribs,
                    {.view = za::View::fromScreenSize({100.f, 100.f}), .shader = &shader});
            };

            renderTexture.clear(za::Color::Black);

            drawInstance({.offset = {10.f, 10.f}, .color = za::Color::Green});
            drawInstance({.offset = {60.f, 10.f}, .color = za::Color::Red});

            renderTexture.display();

            const auto image = renderTexture.getTexture().copyToImage();

            CHECK(image.getPixel({15, 15}) == za::Color::Green);
            CHECK(image.getPixel({65, 15}) == za::Color::Red);
        }

        SECTION("SOA instanced draw with separate VBOs per field")
        {
            auto shader = za::Shader::loadFromMemory(
                              {.vertexCode = instancedVertexSource, .fragmentCode = instancedFragmentSource})
                              .value();

            za::VAOHandle vaoHandle;
            za::VBOHandle offsetVBO;
            za::VBOHandle colorVBO;

            const auto drawInstance = [&](const za::Vec2f offset, const za::Color color)
            {
                const za::Vec2f instanceOffsetData[]{offset};
                const za::Color instanceColorData[]{color};

                auto setupAttribs = [&](za::InstanceAttributeBinder& binder)
                {
                    binder.uploadContiguousData(offsetVBO, instanceOffsetData);
                    binder.setupFlat<za::Vec2f>(3);

                    binder.uploadContiguousData(colorVBO, instanceColorData);
                    binder.setupFlat<za::Color>(4);
                };

                renderTexture.drawInstancedIndexedVertices(
                    {
                        .vaoHandle     = vaoHandle,
                        .vertexSpan    = za::instancedQuadVertices,
                        .indexSpan     = za::instancedQuadIndices,
                        .instanceCount = 1u,
                        .primitiveType = za::PrimitiveType::Triangles,
                    },
                    setupAttribs,
                    {.view = za::View::fromScreenSize({100.f, 100.f}), .shader = &shader});
            };

            renderTexture.clear(za::Color::Black);

            drawInstance({10.f, 10.f}, za::Color::Green);
            drawInstance({60.f, 10.f}, za::Color::Red);

            renderTexture.display();

            const auto image = renderTexture.getTexture().copyToImage();

            CHECK(image.getPixel({15, 15}) == za::Color::Green);
            CHECK(image.getPixel({65, 15}) == za::Color::Red);
        }

        SECTION("Mixed instanced and immediate draws preserve GL_ARRAY_BUFFER for the active VAO")
        {
            // Regression: `setupDraw`'s non-rebind branch (taken for
            // consecutive same-VAO draws when the cache is enabled) must
            // ensure GL_ARRAY_BUFFER is bound to the active VAO's shared
            // VBO before `streamVerticesToGPU` runs. The instanced binder
            // path leaves a per-instance VBO bound; without proper
            // restoration, the next same-VAO draw's `glBufferData` would
            // overwrite the per-instance VBO storage instead of the shared
            // mesh VBO, corrupting the next draw and (on desktop where
            // per-instance VBOs are persistently mapped) tripping
            // `GL_INVALID_OPERATION`.
            //
            // The test stresses transitions across all interesting
            // cache-state combinations:
            //   instanced -> instanced (same VAO, non-rebind branch)
            //   instanced -> immediate (different VAO, rebind branch)
            //   immediate -> instanced (different VAO, rebind branch)
            auto rt = za::RenderTexture::create({100, 100}).value();

            auto shader = za::Shader::loadFromMemory(
                              {.vertexCode = instancedVertexSource, .fragmentCode = instancedFragmentSource})
                              .value();

            za::VAOHandle vaoHandle;
            za::VBOHandle instanceVBO;

            struct InstanceData
            {
                za::Vec2f offset;
                za::Color color;
            };

            const auto drawInstanced = [&](za::Vec2f offset, za::Color color)
            {
                const InstanceData instance{offset, color};

                auto setupAttribs = [&](za::InstanceAttributeBinder& binder)
                {
                    binder.uploadContiguousData(instanceVBO, &instance);
                    binder.setupField<&InstanceData::offset>(3);
                    binder.setupField<&InstanceData::color>(4);
                };

                rt.drawInstancedIndexedVertices(
                    {
                        .vaoHandle     = vaoHandle,
                        .vertexSpan    = za::instancedQuadVertices,
                        .indexSpan     = za::instancedQuadIndices,
                        .instanceCount = 1u,
                        .primitiveType = za::PrimitiveType::Triangles,
                    },
                    setupAttribs,
                    {.view = za::View::fromScreenSize({100.f, 100.f}), .shader = &shader});
            };

            rt.clear(za::Color::Black);

            // Two consecutive instanced draws on the same VAOHandle. After
            // the first, GL_ARRAY_BUFFER points at the instance VBO; the
            // second must rebind the VAO's shared mesh VBO before
            // `streamVerticesToGPU` runs.
            drawInstanced({15.f, 15.f}, za::Color::Green);
            drawInstanced({40.f, 15.f}, za::Color::Red);

            // Immediate non-instanced draw (different VAO -> rebind branch).
            rt.draw(za::RectangleShape{{.position = {55.f, 5.f}, .fillColor = za::Color::Blue, .size = {20.f, 20.f}}});

            // Back to instanced (different VAO -> rebind branch).
            drawInstanced({15.f, 40.f}, za::Color::Yellow);

            // Another consecutive instanced draw (same VAO -> non-rebind
            // branch must once again restore GL_ARRAY_BUFFER).
            drawInstanced({40.f, 40.f}, za::Color::Magenta);

            // Final transitions: instanced -> immediate -> instanced again.
            rt.draw(za::RectangleShape{{.position = {55.f, 30.f}, .fillColor = za::Color::Cyan, .size = {20.f, 20.f}}});
            drawInstanced({15.f, 65.f}, za::Color{255u, 128u, 0u, 255u}); // orange

            rt.display();

            const auto img = rt.getTexture().copyToImage();
            CHECK(img.getPixel({15, 15}) == za::Color::Green);
            CHECK(img.getPixel({40, 15}) == za::Color::Red);
            CHECK(img.getPixel({65, 15}) == za::Color::Blue);
            CHECK(img.getPixel({15, 40}) == za::Color::Yellow);
            CHECK(img.getPixel({40, 40}) == za::Color::Magenta);
            CHECK(img.getPixel({65, 40}) == za::Color::Cyan);
            CHECK(img.getPixel({15, 65}) == za::Color{255u, 128u, 0u, 255u});
        }

#ifndef ZA_OPENGL_ES
        SECTION("Persistent GPU batches can reserve before clear when reused across frames")
        {
            auto batchRenderTexture = za::RenderTexture::create({100, 100}).value();

            za::PersistentGPUDrawableBatch batch;
            const za::RectangleShape rect{{.position = {10.f, 10.f}, .fillColor = za::Color::Green, .size = {30.f, 30.f}}};

            batchRenderTexture.clear(za::Color::Black);
            batch.add(rect);
            batchRenderTexture.draw(batch);
            batchRenderTexture.display();

            glCheck(glFinish());

            // Mirrors the examples: reserve for the next frame before the
            // batch is cleared and refilled.
            batch.reserveQuads(64u);
            batch.clear();
            batch.add(rect);

            batchRenderTexture.clear(za::Color::Black);
            batchRenderTexture.draw(batch);
            batchRenderTexture.display();

            const auto image = batchRenderTexture.getTexture().copyToImage();
            CHECK(image.getPixel({20, 20}) == za::Color::Green);
        }

        SECTION("Persistent GPU batches can be refilled from a worker thread after clear")
        {
            auto batchRenderTexture = za::RenderTexture::create({100, 100}).value();

            za::PersistentGPUDrawableBatch batch;
            const za::RectangleShape rect{{.position = {10.f, 10.f}, .fillColor = za::Color::Green, .size = {30.f, 30.f}}};

            batch.add(rect);

            batchRenderTexture.clear(za::Color::Black);
            batchRenderTexture.draw(batch);
            batchRenderTexture.display();

            batch.clear();

            za::Thread worker{[&batch, &rect] { batch.add(rect); }};
            worker.join();

            batchRenderTexture.clear(za::Color::Black);
            batchRenderTexture.draw(batch);
            batchRenderTexture.display();

            const auto image = batchRenderTexture.getTexture().copyToImage();
            CHECK(image.getPixel({20, 20}) == za::Color::Green);
        }

        SECTION("GPU autobatch survives VBO/EBO id cycling across grows (keyboard example regression)")
        {
            // Regression for the bug that crashed `examples/keyboard`.
            // The GPU autobatch's persistent VBO/EBO ring buffers grow as
            // vertices accumulate. Each grow allocates a fresh GL buffer
            // via `glGenBuffers` and destroys the old one via
            // `glDeleteBuffers`. The GL spec permits a sequence of these
            // operations to *cycle* the live vbo/ebo ids back to values
            // that were live earlier in the frame -- Mesa/iris does this
            // aggressively. A cache that keys only on ids then sees a
            // false match at flush 2 and skips the VAO rebind, leaving
            // the VAO's attribute pointers referencing a destroyed-and-
            // recreated buffer object. The next `glDrawElementsBaseVertex`
            // SIGSEGVs inside the driver.
            //
            // The fix is a per-`GLBufferObject` generation counter (see
            // `nextGLBufferObjectGeneration`) that the cache snapshots
            // alongside the ids. Distinct buffer instances always have
            // distinct generations, so id recycling can never produce a
            // false match.
            //
            // The test mirrors the keyboard's exact draw pattern:
            //   1. 1746 raw triangles (`Keyboard::ScancodeCount * 6`),
            //      no texture.
            //   2. 291 textured `Text` labels (`Keyboard::ScancodeCount`).
            //      The first label is a state change from no-texture to
            //      the font atlas; that triggers flush 1.
            //   3. A textured draw with a *custom shader* triggers flush
            //      2 -- exactly how `ShinyText` triggers it in keyboard.
            // If the cache check misses the id cycle, flush 2 SIGSEGVs.
            const auto font = za::Font::openFromFile("tuffy.ttf").value();

            // The custom shader path is what fires flush 2 in keyboard:
            // `ShinyText` uses a uniform-bearing fragment shader, so the
            // first `ShinyText` draw mismatches the labels' shader state
            // and flushes.
            auto       customShader = za::Shader::loadFromMemory({.fragmentCode = blinkAlphaFragSource}).value();
            const auto loc          = customShader.getUniformLocation("blink_alpha").value();
            customShader.setUniform(loc, 1.f);

            auto rt = za::RenderTexture::create({100u, 100u}).value();
            rt.setAutoBatchMode(za::RenderTarget::AutoBatchMode::GPUStorage);
            rt.clear(za::Color::Black);

            // 1746 = `za::Keyboard::ScancodeCount * 6` (mirrors KeyboardView).
            za::Vertex triangles[1746];
            for (auto& v : triangles)
                v = {.position = {-10.f, -10.f}, .color = za::Color::Red};
            rt.draw(triangles, za::PrimitiveType::Triangles);

            // 291 = `za::Keyboard::ScancodeCount`. Each label is a Text
            // draw against the same font atlas, so subsequent labels
            // hit the same `RenderStates` and do NOT re-flush; only the
            // first label flushes (state change from no-texture).
            for (unsigned int i = 0u; i < 291u; ++i)
            {
                za::Text label(font, {.string = "label", .characterSize = 14u});
                label.position = {static_cast<float>(i % 30u) * 3.f, static_cast<float>(i / 30u) * 4.f};
                rt.draw(label);
            }

            // Custom shader -> shader state mismatch -> flush 2 fires here.
            // This is the exact site where keyboard crashes.
            const za::Vertex tri[] = {
                {.position = {-10.f, -10.f}, .color = za::Color::White},
                {.position = {-9.f, -10.f}, .color = za::Color::White},
                {.position = {-10.f, -9.f}, .color = za::Color::White},
            };
            rt.draw(tri, za::PrimitiveType::Triangles, za::RenderStates{.shader = &customShader});

            rt.draw(za::RectangleShape{{.position = {10.f, 10.f}, .fillColor = za::Color::Green, .size = {30.f, 30.f}}});
            rt.display();

            const auto image = rt.getTexture().copyToImage();
            CHECK(image.getPixel({25u, 25u}) != za::Color::Black);
        }

        SECTION(
            "VAO attribute pointers must be re-issued after a buffer is deleted (invariant the keyboard fix relies on)")
        {
            // This is a *contract* test, not a regression test on the
            // cache code: it pins the GL-level semantics that motivate
            // the unconditional cache invalidation in
            // `immediateDrawPersistentMappedIndexedVertices`. Whoever
            // later replaces that invalidation with a smarter mechanism
            // (e.g. a generational counter) needs to preserve the
            // behaviour these four steps assert, so the cache check
            // cannot regress to "ids match -> skip rebind" when the
            // underlying buffer object has been recreated.
            //
            //   1. Create a VAO + VBO. Bind, run `glVertexAttribPointer`,
            //      query `GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING` -> must
            //      match the VBO's id.
            //   2. `glDeleteBuffers` on that VBO. Re-query -> binding is
            //      now 0 (the spec mandates the VAO state demotes any
            //      attribute that referenced the deleted buffer).
            //   3. `glGenBuffers` a fresh buffer and bind it to
            //      `GL_ARRAY_BUFFER`. Re-query -> binding is STILL 0
            //      (binding to the target alone does not patch the VAO).
            //   4. Re-issue `glVertexAttribPointer`. Re-query -> binding
            //      now matches the new buffer's id.
            unsigned int vao = 0u;
            glCheck(glGenVertexArrays(1, &vao));
            glCheck(glBindVertexArray(vao));

            unsigned int vboA = 0u;
            glCheck(glGenBuffers(1, &vboA));
            glCheck(glBindBuffer(GL_ARRAY_BUFFER, vboA));
            glCheck(glBufferData(GL_ARRAY_BUFFER, 64, nullptr, GL_STATIC_DRAW));

            glCheck(glEnableVertexAttribArray(0u));
            glCheck(glVertexAttribPointer(0u, 2, GL_FLOAT, GL_FALSE, 8, nullptr));

            int binding = -1;
            glCheck(glGetVertexAttribiv(0u, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &binding));
            CHECK(binding == static_cast<int>(vboA));

            // Step 2: deleting the buffer demotes the VAO's attribute
            // binding to 0.
            glCheck(glDeleteBuffers(1, &vboA));
            glCheck(glGetVertexAttribiv(0u, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &binding));
            CHECK(binding == 0);

            // Step 3: binding a fresh VBO to `GL_ARRAY_BUFFER` does NOT
            // restore the VAO's attribute binding.
            unsigned int vboB = 0u;
            glCheck(glGenBuffers(1, &vboB));
            glCheck(glBindBuffer(GL_ARRAY_BUFFER, vboB));
            glCheck(glBufferData(GL_ARRAY_BUFFER, 64, nullptr, GL_STATIC_DRAW));
            glCheck(glGetVertexAttribiv(0u, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &binding));
            CHECK(binding == 0);

            // Step 4: only `glVertexAttribPointer` restores the binding.
            glCheck(glVertexAttribPointer(0u, 2, GL_FLOAT, GL_FALSE, 8, nullptr));
            glCheck(glGetVertexAttribiv(0u, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &binding));
            CHECK(binding == static_cast<int>(vboB));

            // Cleanup.
            glCheck(glBindVertexArray(0u));
            glCheck(glDeleteBuffers(1, &vboB));
            glCheck(glDeleteVertexArrays(1, &vao));
        }

        SECTION("PersistentGPUDrawableBatch bumps attribStateGen when reserveVertexCapacity grows the VBO")
        {
            // Direct unit-level pin of the bump in
            // `PersistentGPUStorage::reserveVertexCapacity`. Future
            // refactors that move the grow logic without re-issuing
            // the bump will trip the second `CHECK`. The third `CHECK`
            // proves the bump is grow-conditional rather than
            // unconditional, otherwise the cache fast path is defeated.
            struct TestableBatch : za::PersistentGPUDrawableBatch
            {
                using za::PersistentGPUDrawableBatch::m_storage;
            };
            TestableBatch batch;
            const auto*   vg = static_cast<const za::GLVAOGroup*>(batch.m_storage.getVAOGroup());

            const unsigned int genStart = vg->attribStateGen;

            batch.m_storage.reserveVertexCapacity(4096u);
            const unsigned int genAfterGrow = vg->attribStateGen;
            CHECK(genAfterGrow > genStart);

            // Second reservation that fits in existing capacity must not bump.
            batch.m_storage.reserveVertexCapacity(1u);
            CHECK(vg->attribStateGen == genAfterGrow);
        }

        SECTION("PersistentGPUDrawableBatch bumps attribStateGen when reserveIndexCapacity grows the EBO")
        {
            // Parallel check for the EBO grow path. Symmetrical to the
            // VBO case above.
            struct TestableBatch : za::PersistentGPUDrawableBatch
            {
                using za::PersistentGPUDrawableBatch::m_storage;
            };
            TestableBatch batch;
            const auto*   vg = static_cast<const za::GLVAOGroup*>(batch.m_storage.getVAOGroup());

            const unsigned int genStart = vg->attribStateGen;

            batch.m_storage.reserveIndexCapacity(6144u);
            const unsigned int genAfterGrow = vg->attribStateGen;
            CHECK(genAfterGrow > genStart);

            batch.m_storage.reserveIndexCapacity(1u);
            CHECK(vg->attribStateGen == genAfterGrow);
        }

        SECTION("PersistentGPUStorage::reserveMoreVertices bumps attribStateGen on growth")
        {
            // Most-traveled VBO grow path. Isolated from the EBO side
            // by calling `reserveMoreVertices` directly rather than
            // `add()` which would grow both buffers simultaneously
            // and mask a missing VBO-side bump behind the EBO one.
            struct TestableBatch : za::PersistentGPUDrawableBatch
            {
                using za::PersistentGPUDrawableBatch::m_storage;
            };
            TestableBatch batch;
            const auto*   vg = static_cast<const za::GLVAOGroup*>(batch.m_storage.getVAOGroup());

            const unsigned int genStart = vg->attribStateGen;

            // First call: VBO ring buffer has zero capacity, so this
            // forces a grow.
            [[maybe_unused]] auto* p = batch.m_storage.reserveMoreVertices(3u);
            batch.m_storage.commitMoreVertices(3u);
            CHECK(vg->attribStateGen > genStart);
        }

        SECTION("PersistentGPUStorage::reserveMoreIndices bumps attribStateGen on growth")
        {
            // Most-traveled EBO grow path. Isolated from the VBO side.
            struct TestableBatch : za::PersistentGPUDrawableBatch
            {
                using za::PersistentGPUDrawableBatch::m_storage;
            };
            TestableBatch batch;
            const auto*   vg = static_cast<const za::GLVAOGroup*>(batch.m_storage.getVAOGroup());

            const unsigned int genStart = vg->attribStateGen;

            [[maybe_unused]] auto* p = batch.m_storage.reserveMoreIndices(3u);
            batch.m_storage.commitMoreIndices(3u);
            CHECK(vg->attribStateGen > genStart);
        }

        SECTION("PersistentGPUDrawableBatch::add does not bump attribStateGen on every call (steady state)")
        {
            // Companion to the two isolated grow tests above: this
            // verifies the bump is grow-conditional, not unconditional.
            // If a future refactor accidentally bumps on every reserve
            // regardless of growth, the cache fast path is defeated
            // -- the persistent flush always rebinds, even in steady
            // state.
            struct TestableBatch : za::PersistentGPUDrawableBatch
            {
                using za::PersistentGPUDrawableBatch::m_storage;
            };
            TestableBatch batch;
            const auto*   vg = static_cast<const za::GLVAOGroup*>(batch.m_storage.getVAOGroup());

            const za::RectangleShape rect{{.position = {0.f, 0.f}, .fillColor = za::Color::White, .size = {1.f, 1.f}}};

            // Prime the buffers.
            batch.add(rect);

            // Now pump many more shapes. With geometric growth (1.5x
            // per grow), 64 shapes should produce only a handful of
            // grows -- definitely not 64.
            unsigned int genBefore     = vg->attribStateGen;
            unsigned int bumpsObserved = 0u;
            for (unsigned int i = 0u; i < 64u; ++i)
            {
                batch.add(rect);
                if (vg->attribStateGen != genBefore)
                {
                    ++bumpsObserved;
                    genBefore = vg->attribStateGen;
                }
            }
            CHECK(bumpsObserved < 32u);
        }

        SECTION("GPU autobatch survives many clear/draw/display frame cycles after initial growth")
        {
            // Exercises the autobatch's per-frame-state rotation path
            // including the pre-sizing in `PersistentGPUStorage::clear()`
            // that may itself trigger a grow (and thus an attribStateGen
            // bump). The first frame establishes the high-water mark via
            // growth; subsequent frames should stay growth-free once the
            // pre-sizing has caught up. Any breakage in the cache check
            // or the bump on the clear-time pre-size path would crash
            // here or render the wrong pixel inside the loop.
            auto rt = za::RenderTexture::create({100u, 100u}).value();
            rt.setAutoBatchMode(za::RenderTarget::AutoBatchMode::GPUStorage);

            // Large initial batch on frame 0 -- forces growth.
            // Vertex count must be a multiple of 3 (Triangles primitive).
            za::Vertex triangles[1023];
            for (auto& v : triangles)
                v = {.position = {-10.f, -10.f}, .color = za::Color::Red};

            for (unsigned int frame = 0u; frame < 8u; ++frame)
            {
                rt.clear(za::Color::Black);
                rt.draw(triangles, za::PrimitiveType::Triangles);
                // Visible green marker -- proves the flush produced
                // valid output for this frame.
                rt.draw(za::RectangleShape{{.position = {10.f, 10.f}, .fillColor = za::Color::Green, .size = {30.f, 30.f}}});
                rt.display();

                const auto image = rt.getTexture().copyToImage();
                CHECK(image.getPixel({25u, 25u}) == za::Color::Green);
            }
        }

        SECTION("Instanced draw survives VBOHandle growth across many draws")
        {
            // The instanced draw path relies on
            // `InstanceAttributeBinder::applySetups` unconditionally
            // re-issuing `glVertexAttribPointer` every draw. That
            // protects against `VBOHandle` growth (which can replace
            // the underlying VBO instance) leaving slot-3+ attribute
            // pointers stale. We exercise the property by pumping
            // many sequential instanced draws through a single
            // `VBOHandle`: the persistent ring buffer behind the
            // handle grows multiple times during the loop, and any
            // staleness would corrupt the final draws.
            //
            // If a future optimization caches `applySetups` results
            // across draws without invalidating on VBOHandle grow,
            // the bookend draws would render wrong pixels.
            struct InstanceData
            {
                za::Vec2f offset;
                za::Color color;
            };

            auto shader = za::Shader::loadFromMemory(
                              {.vertexCode = instancedVertexSource, .fragmentCode = instancedFragmentSource})
                              .value();

            auto          rt = za::RenderTexture::create({100u, 100u}).value();
            za::VAOHandle vaoHandle;
            za::VBOHandle instanceVBO;

            const auto drawInstance = [&](const InstanceData instance)
            {
                auto setupAttribs = [&](za::InstanceAttributeBinder& binder)
                {
                    binder.uploadContiguousData(instanceVBO, &instance);
                    binder.setupField<&InstanceData::offset>(3);
                    binder.setupField<&InstanceData::color>(4);
                };

                rt.drawInstancedIndexedVertices(
                    {
                        .vaoHandle     = vaoHandle,
                        .vertexSpan    = za::instancedQuadVertices,
                        .indexSpan     = za::instancedQuadIndices,
                        .instanceCount = 1u,
                        .primitiveType = za::PrimitiveType::Triangles,
                    },
                    setupAttribs,
                    {.view = za::View::fromScreenSize({100.f, 100.f}), .shader = &shader});
            };

            rt.clear(za::Color::Black);

            // First bookend: a green quad at (10, 10). This early
            // draw must still be visible after the loop -- if the
            // VBOHandle growth somehow corrupted earlier writes,
            // this pixel would be wrong.
            drawInstance({.offset = {10.f, 10.f}, .color = za::Color::Green});

            // Pump enough instanced draws to grow the `VBOHandle`'s
            // ring buffer multiple times. Each upload bumps the
            // writeCursor by `sizeof(InstanceData)` (~12 bytes); the
            // ring buffer starts at zero capacity and grows
            // geometrically. ~2000 iterations crosses many growth
            // boundaries.
            for (unsigned int i = 0u; i < 2000u; ++i)
            {
                const float x = 30.f + static_cast<float>(i % 5u) * 0.001f;
                drawInstance({.offset = {x, 50.f}, .color = za::Color::Blue});
            }

            // Second bookend after the growth churn. If the cached
            // attribute pointers had ever gone stale, this draw's
            // colour or position would be wrong.
            drawInstance({.offset = {60.f, 10.f}, .color = za::Color::Red});

            rt.display();

            const auto image = rt.getTexture().copyToImage();
            CHECK(image.getPixel({15u, 15u}) == za::Color::Green);
            CHECK(image.getPixel({65u, 15u}) == za::Color::Red);
        }
#endif
    }

    SECTION("State-cache regression tests")
    {
        // Custom shader that emits the value of a single uniform as the fragment
        // color. Decouples the rendered color from the geometry so we can prove
        // which `setUniform` value the right program actually received.
        constexpr auto solidColorFragSource = R"glsl(
layout(location = 7) uniform vec4 u_color;

in vec4 za_v_color;
in vec2 za_v_texCoord;

layout(location = 0) out vec4 za_fragColor;

void main()
{
    za_fragColor = u_color;
}
)glsl";

        SECTION("resetGLStates reapplies viewport after raw GL viewport mutation")
        {
            // Regression: `resetGLStatesImpl` previously did not reapply
            // `cache.lastView`, so a raw `glViewport` between draws (or an
            // ImGui backend) could leave the GL viewport mismatched with what
            // Zancle's cache claims is bound. The next draw with the same view
            // would skip `applyView` (cache hit on `lastView`) and render at
            // the wrong viewport.
            auto rt = za::RenderTexture::create({100, 100}).value();

            const za::RectangleShape leftRect{
                {.position = {10.f, 10.f}, .fillColor = za::Color::Green, .size = {20.f, 20.f}}};
            const za::RectangleShape rightRect{
                {.position = {60.f, 60.f}, .fillColor = za::Color::Red, .size = {20.f, 20.f}}};

            rt.clear(za::Color::Black);
            rt.draw(leftRect);
            rt.flush(); // commit the draw so cache.lastView is set on the live view

            // Mutate the GL viewport behind Zancle's back. Without the fix, the
            // next `resetGLStates` + draw will inherit this 50x50 viewport.
            glCheck(glViewport(0, 0, 50, 50));

            rt.resetGLStates();

            // Same view as before -- without the fix, `applyView` is skipped
            // because `lastView == usedView`, so the rightRect would land
            // outside the (still-50x50) viewport and not render.
            rt.draw(rightRect);
            rt.display();

            const auto img = rt.getTexture().copyToImage();
            CHECK(img.getPixel({15, 15}) == za::Color::Green);
            CHECK(img.getPixel({65, 65}) == za::Color::Red);
        }

        SECTION("setUniform applies to the right shader when many shaders interleave")
        {
            // Regression: `Shader::UniformBinder` now reads the bound program
            // from a thread_local cache instead of querying GL. Stress the
            // cache by mutating uniforms on multiple shaders interleaved.
            auto shaderA = za::Shader::loadFromMemory({.fragmentCode = solidColorFragSource}).value();
            auto shaderB = za::Shader::loadFromMemory({.fragmentCode = solidColorFragSource}).value();
            auto shaderC = za::Shader::loadFromMemory({.fragmentCode = solidColorFragSource}).value();

            const auto locA = shaderA.getUniformLocation("u_color").value();
            const auto locB = shaderB.getUniformLocation("u_color").value();
            const auto locC = shaderC.getUniformLocation("u_color").value();

            // Many interleaved mutations -- each `setUniform` constructs a
            // `UniformBinder` that reads from / writes to the program cache.
            for (int i = 0; i < 8; ++i)
            {
                shaderA.setUniform(locA, za::Glsl::Vec4{1.f, 0.f, 0.f, 1.f}); // red
                shaderB.setUniform(locB, za::Glsl::Vec4{0.f, 1.f, 0.f, 1.f}); // green
                shaderC.setUniform(locC, za::Glsl::Vec4{0.f, 0.f, 1.f, 1.f}); // blue
            }

            auto                     rt = za::RenderTexture::create({60, 20}).value();
            const za::RectangleShape full{{.size = {20.f, 20.f}}};

            rt.clear(za::Color::Black);
            rt.draw(full, za::RenderStates{.shader = &shaderA});
            rt.draw(za::RectangleShape{{.position = {20.f, 0.f}, .size = {20.f, 20.f}}},
                    za::RenderStates{.shader = &shaderB});
            rt.draw(za::RectangleShape{{.position = {40.f, 0.f}, .size = {20.f, 20.f}}},
                    za::RenderStates{.shader = &shaderC});
            rt.display();

            const auto img = rt.getTexture().copyToImage();
            CHECK(img.getPixel({10, 10}) == za::Color::Red);
            CHECK(img.getPixel({30, 10}) == za::Color::Green);
            CHECK(img.getPixel({50, 10}) == za::Color::Blue);
        }

        SECTION("flush() must not dereference m_lastRenderStates.shader after the shader is destroyed")
        {
            // Regression: `flush()` ran a scope guard at function exit
            // that unconditionally called
            // `updateCachedGenerations(m_lastRenderStates)`, which
            // dereferences `m_lastRenderStates.shader->m_uniformGeneration`.
            // If the user's flow was
            //   1. draw something with a local shader,
            //   2. `display()` (flushes the batch and leaves
            //      `m_lastRenderStates.shader` pointing at the shader),
            //   3. let the shader go out of scope,
            //   4. call `clear()` (which internally calls `flush()`),
            // the scope guard read the dangling pointer.
            //
            // The fix: gate the `updateCachedGenerations` call on
            // `m_numAutoBatchVertices > 0u`. When the batch is empty
            // there's nothing whose generation snapshot matters.
            //
            // The test crashes (or trips ASAN) under the old code; runs
            // cleanly under the fix.
            auto                     rt = za::RenderTexture::create({40u, 40u}).value();
            const za::RectangleShape full{{.size = {40.f, 40.f}}};

            {
                auto shaderInner = za::Shader::loadFromMemory({.fragmentCode = solidColorFragSource}).value();
                shaderInner.setUniform(shaderInner.getUniformLocation("u_color").value(),
                                       za::Glsl::Vec4{1.f, 0.f, 0.f, 1.f});

                rt.clear(za::Color::Black);
                rt.draw(full, za::RenderStates{.shader = &shaderInner});
                rt.display();
                // `m_lastRenderStates.shader` is now `&shaderInner`.
            }
            // shaderInner destroyed -- pointer in `m_lastRenderStates`
            // is now dangling.

            // The next `clear()` triggers `flush()`. With nothing in the
            // batch, the scope guard MUST NOT touch the dangling shader.
            rt.clear(za::Color::Black);

            // If we got here, the scope guard skipped the dereference
            // correctly.
            CHECK(true);
        }

        SECTION("Destroying a shader does not poison the program cache")
        {
            // Regression: `destroyProgramIfNeeded` clears the cache if the
            // destroyed program was the cached one. Without this, GL handle
            // reuse could lead `useProgram(reusedHandle)` to skip the bind
            // on a cache hit and leave the wrong (deleted) program current.
            auto                     rt = za::RenderTexture::create({40, 40}).value();
            const za::RectangleShape full{{.size = {40.f, 40.f}}};

            {
                auto shaderTemp = za::Shader::loadFromMemory({.fragmentCode = solidColorFragSource}).value();
                shaderTemp.setUniform(shaderTemp.getUniformLocation("u_color").value(), za::Glsl::Vec4{1.f, 0.f, 0.f, 1.f});

                rt.clear(za::Color::Black);
                rt.draw(full, za::RenderStates{.shader = &shaderTemp});
                rt.display();
                // shaderTemp's program is now in the cache as the last bound program.
            }
            // shaderTemp destroyed -- cache must be invalidated for that handle.

            // A new shader may receive the same GL handle; either way, the
            // cache must not skip the rebind.
            auto shaderNew = za::Shader::loadFromMemory({.fragmentCode = solidColorFragSource}).value();
            shaderNew.setUniform(shaderNew.getUniformLocation("u_color").value(), za::Glsl::Vec4{0.f, 1.f, 0.f, 1.f});

            rt.clear(za::Color::Black);
            rt.draw(full, za::RenderStates{.shader = &shaderNew});
            rt.display();

            CHECK(rt.getTexture().copyToImage().getPixel({20, 20}) == za::Color::Green);
        }

#ifndef ZA_SYSTEM_EMSCRIPTEN
        // Emscripten/WebGL does not support multiple GL contexts.
        SECTION("Shader cache is invalidated when active GL context changes")
        {
            // Regression: `readCurrentProgramOrQuery` uses `currentProgramCacheContextId`
            // to detect when the cache was populated under a different GL
            // context, falling back to a query in that case.
            //
            // To exercise this, spin up an explicit secondary GL context so
            // the active-context id genuinely changes between calls.
            // RenderTextures alone don't trigger it because they share the
            // active context rather than carrying their own.
            auto rt = za::RenderTexture::create({40, 40}).value();

            auto       shader = za::Shader::loadFromMemory({.fragmentCode = solidColorFragSource}).value();
            const auto loc    = shader.getUniformLocation("u_color").value();

            const za::RectangleShape full{{.size = {40.f, 40.f}}};

            // 1) Prime the cache on the shared graphics context by drawing
            //    once. After this, `currentProgramCacheValue` holds shader's
            //    program id, tagged with the shared context's id.
            shader.setUniform(loc, za::Glsl::Vec4{1.f, 0.f, 0.f, 1.f});
            rt.clear(za::Color::Black);
            rt.draw(full, za::RenderStates{.shader = &shader});
            rt.display();

            // 2) Activate a fresh GL context. In this context, no shader is
            //    bound -- `glGetIntegerv(GL_CURRENT_PROGRAM)` would return 0.
            //    But the cache still holds the shader's program id from the
            //    previous context. Without the per-context tag check, the
            //    next `setUniform` would think shader is already bound and
            //    skip the rebind, calling `glUniform4f` on whatever (or
            //    nothing) is current in this context.
            {
                TestContext fresh;
                CHECK(fresh.getId() != 0u);

                // 3) Touch a uniform in the fresh context. With the fix, the
                //    UniformBinder reads the cache, sees the context tag
                //    mismatch, queries GL (which returns 0), so saves 0 and
                //    binds the shader. Without the fix, it would trust the
                //    stale cache value and skip the bind.
                shader.setUniform(loc, za::Glsl::Vec4{0.f, 1.f, 0.f, 1.f});
            }
            // TestContext destructor reactivates the shared context. Note
            // that on the shared context, the cache value is now stale in
            // a different way -- but that's the same context-tag-mismatch
            // path, so the fix handles it equally.

            // 4) Draw again on the shared context, this time with green.
            //    With the fix: re-tag fires, fallback query runs, shader
            //    rebinds correctly, uniform applies.
            shader.setUniform(loc, za::Glsl::Vec4{0.f, 1.f, 0.f, 1.f});
            rt.clear(za::Color::Black);
            rt.draw(full, za::RenderStates{.shader = &shader});
            rt.display();

            CHECK(rt.getTexture().copyToImage().getPixel({20, 20}) == za::Color::Green);
        }
#endif

        SECTION("Multiple RenderTextures with different views render correctly")
        {
            // Regression: cross-context viewport state. Each RenderTexture
            // has its own view and corresponding viewport. After the fix,
            // `resetGLStatesImpl` reapplies `cache.lastView` per target, so
            // switching contexts mid-frame doesn't leave the GL viewport
            // pointing at the previous target's rectangle.
            auto rtSmall = za::RenderTexture::create({40, 40}).value();
            auto rtLarge = za::RenderTexture::create({80, 80}).value();

            const za::RectangleShape rectSmall{
                {.position = {10.f, 10.f}, .fillColor = za::Color::Green, .size = {20.f, 20.f}}};
            const za::RectangleShape rectLarge{
                {.position = {30.f, 30.f}, .fillColor = za::Color::Red, .size = {20.f, 20.f}}};

            // Interleaved draws: each pass forces a setActive() between the
            // two RTs. If the viewport tracking regresses, one of these
            // produces blank pixels where geometry should be.
            for (int i = 0; i < 3; ++i)
            {
                rtSmall.clear(za::Color::Black);
                rtSmall.draw(rectSmall);
                rtSmall.display();

                rtLarge.clear(za::Color::Black);
                rtLarge.draw(rectLarge);
                rtLarge.display();
            }

            CHECK(rtSmall.getTexture().copyToImage().getPixel({20, 20}) == za::Color::Green);
            CHECK(rtLarge.getTexture().copyToImage().getPixel({40, 40}) == za::Color::Red);
        }

        SECTION("VertexBuffer draws correctly when interleaved with regular draws")
        {
            // Regression: `RenderTarget::draw(VertexBuffer&)` previously
            // force-zeroed `cache.lastVaoGroup` before the DrawGuard,
            // forcing a full VAO rebind. The light cache-hit path in
            // `applyDrawCacheStates` should be sufficient: after
            // `bindGLObjects(m_impl->vaoGroup)` at the end of the function
            // restores the standard VAO's attribs, subsequent draws (regular
            // or another VertexBuffer) must produce correct geometry.
            //
            // Mixed sequence below stresses the handoff:
            //   regular draw -> VB draw -> regular draw -> VB draw -> regular draw
            auto rt = za::RenderTexture::create({120, 30}).value();
            rt.clear(za::Color::Black);

            // 1) Regular draw to prime the cache.
            rt.draw(za::RectangleShape{{.position = {0.f, 0.f}, .fillColor = za::Color::Green, .size = {20.f, 30.f}}});

            // 2) VertexBuffer draw at x=[20, 40).
            za::VertexBuffer vb1{za::PrimitiveType::TriangleStrip};
            REQUIRE(vb1.create(4u));
            const za::Vertex v1[]{
                {{20.f, 0.f}, za::Color::Red, {0.f, 0.f}},
                {{40.f, 0.f}, za::Color::Red, {0.f, 0.f}},
                {{20.f, 30.f}, za::Color::Red, {0.f, 0.f}},
                {{40.f, 30.f}, za::Color::Red, {0.f, 0.f}},
            };
            REQUIRE(vb1.update(v1));
            rt.draw(vb1);

            // 3) Regular draw at x=[40, 60), checks the standard VAO
            //    attribs were restored after the VB draw.
            rt.draw(za::RectangleShape{{.position = {40.f, 0.f}, .fillColor = za::Color::Blue, .size = {20.f, 30.f}}});

            // 4) Another VertexBuffer draw immediately after a regular draw.
            za::VertexBuffer vb2{za::PrimitiveType::TriangleStrip};
            REQUIRE(vb2.create(4u));
            const za::Vertex v2[]{
                {{60.f, 0.f}, za::Color::Yellow, {0.f, 0.f}},
                {{80.f, 0.f}, za::Color::Yellow, {0.f, 0.f}},
                {{60.f, 30.f}, za::Color::Yellow, {0.f, 0.f}},
                {{80.f, 30.f}, za::Color::Yellow, {0.f, 0.f}},
            };
            REQUIRE(vb2.update(v2));
            rt.draw(vb2);

            // 5) Final regular draw at x=[80, 120) -- same standard VAO
            //    must again be in a valid state.
            rt.draw(za::RectangleShape{{.position = {80.f, 0.f}, .fillColor = za::Color::Magenta, .size = {40.f, 30.f}}});

            rt.display();

            const auto img = rt.getTexture().copyToImage();
            CHECK(img.getPixel({10, 15}) == za::Color::Green);
            CHECK(img.getPixel({30, 15}) == za::Color::Red);
            CHECK(img.getPixel({50, 15}) == za::Color::Blue);
            CHECK(img.getPixel({70, 15}) == za::Color::Yellow);
            CHECK(img.getPixel({100, 15}) == za::Color::Magenta);
        }

#ifndef ZA_SYSTEM_EMSCRIPTEN
        // Emscripten/WebGL does not support multiple GL contexts.
        SECTION("Persistent GL states are reset when an RT moves to a context already owned by another RT")
        {
            // Regression: when `setActive(true)` is called on a context whose
            // slot in `contextRenderTargetMap` already holds a *different*
            // RT id (not `invalidId`), `RenderTarget::setActive` clears
            // `cache.enable` but leaves `cache.glStatesSet` untrue. The next
            // `setupDraw` therefore skips `resetGLStatesImpl`, and persistent
            // states owned only by that helper (`GL_CULL_FACE`,
            // `GL_DEPTH_TEST`, `GL_COLOR_MASK`, the active texture unit)
            // inherit whatever the previous occupant or raw GL code left in
            // the context.
            //
            // We exercise the case using `GL_CULL_FACE` + `glCullFace(GL_FRONT_AND_BACK)`,
            // which culls every triangle until something disables it.
            auto rtA = za::RenderTexture::create({40, 40}).value();
            auto rtB = za::RenderTexture::create({40, 40}).value();

            const za::RectangleShape full{{.fillColor = za::Color::Green, .size = {40.f, 40.f}}};

            // 1) Prime rtA on the shared graphics context. After this draw
            //    rtA's `glStatesSet` is true, slot[shared] = rtA.id, and the
            //    shared context's GL state is the one `resetGLStatesImpl`
            //    establishes (cull off, etc.).
            rtA.clear(za::Color::Red);
            rtA.draw(full);
            rtA.display();

            {
                TestContext fresh;
                CHECK(fresh.getId() != 0u);

                // 2) Activate rtB on `fresh`. slot[fresh] starts invalid, so
                //    rtB's `glStatesSet` is forced false → `setupDraw` runs
                //    `resetGLStatesImpl` on `fresh` → `fresh`'s persistent
                //    GL state is sane. After this, slot[fresh] = rtB.id.
                rtB.clear(za::Color::Black);
                rtB.draw(full);
                rtB.display();

                // 3) Corrupt `fresh`'s persistent state behind Zancle's back.
                //    `GL_CULL_FACE` is touched only by `resetGLStatesImpl`
                //    in the Zancle pipeline, so it survives until that helper
                //    runs again. `glCullFace(GL_FRONT_AND_BACK)` makes the
                //    cull discard every triangle regardless of winding.
                glCheck(glEnable(GL_CULL_FACE));
                glCheck(glCullFace(GL_FRONT_AND_BACK));

                // 4) Activate rtA on `fresh`. setActive sees slot[fresh] = rtB.id,
                //    which is neither `invalidId` nor `rtA.id`. With the bug,
                //    only `cache.enable` is reset; rtA's `glStatesSet` stays
                //    true (carried over from step 1), so `setupDraw` skips
                //    `resetGLStatesImpl` and the cull-everything state leaks
                //    into the draw. The expected fix forces `glStatesSet =
                //    false` on this branch (or tags it with the context id),
                //    so the draw resets persistent state and the green quad
                //    actually rasterizes.
                rtA.clear(za::Color::Red);
                rtA.draw(full);
                rtA.display();

                // Restore `fresh`'s GL state before TestContext tears down.
                glCheck(glDisable(GL_CULL_FACE));
                glCheck(glCullFace(GL_BACK));
            }

            // With the fix: green wrote to the FBO. With the bug: every
            // triangle was culled, so the pixel is still the red clear value.
            CHECK(rtA.getTexture().copyToImage().getPixel({20, 20}) == za::Color::Green);
        }
#endif
    }
}


TEST_CASE("[Graphics] Shape vs ShapeData vertex parity" * tst::skip(skipDisplayTests))
{
    [[maybe_unused]] auto& graphicsContext = TST_CASE_SHARED(za::GraphicsContext::create().value());

    // The retained `za::Shape` pipeline and the batch `*ShapeData` pipeline
    // must produce identical geometry for the same aggregate: local-space
    // outlines (thickness scales with the transform, mirroring cannot flip
    // the grow side) and local-space fill/outline texture coordinates.
    const auto checkParity = [](const auto& shape, const auto& shapeData)
    {
        const auto transform = shape.getTransform();

        za::CPUDrawableBatch batch;
        const auto           geo = batch.add(shapeData);

        const auto fillVertices    = shape.getFillVertices();
        const auto outlineVertices = shape.getOutlineVertices();

        REQUIRE(geo.fill.size() == fillVertices.size());
        REQUIRE(geo.outline.size() == outlineVertices.size());

        const auto approxEq = [](const za::Vec2f a, const za::Vec2f b)
        { return ZA_MATH_FABSF(a.x - b.x) < 0.01f && ZA_MATH_FABSF(a.y - b.y) < 0.01f; };

        for (za::SizeT i = 0u; i < fillVertices.size(); ++i)
        {
            CHECK(approxEq(geo.fill[i].position, transform.transformPoint(fillVertices[i].position)));
            CHECK(approxEq(geo.fill[i].texCoords, fillVertices[i].texCoords));
            CHECK(geo.fill[i].color == fillVertices[i].color);
        }

        for (za::SizeT i = 0u; i < outlineVertices.size(); ++i)
        {
            CHECK(approxEq(geo.outline[i].position, transform.transformPoint(outlineVertices[i].position)));
            CHECK(approxEq(geo.outline[i].texCoords, outlineVertices[i].texCoords));
            CHECK(geo.outline[i].color == outlineVertices[i].color);
        }
    };

    // Hostile transform on purpose: rotation + non-uniform scale + mirror +
    // origin offset, textured fill and outline, custom miter limit
    SECTION("CircleShape vs CircleShapeData")
    {
        const za::CircleShapeData
            data{.position           = {30.f, 40.f},
                 .scale              = {2.f, -1.5f},
                 .origin             = {5.f, 6.f},
                 .rotation           = za::degrees(30.f),
                 .textureRect        = {{8.f, 16.f}, {32.f, 32.f}},
                 .outlineTextureRect = {{4.f, 2.f}, {16.f, 8.f}},
                 .fillColor          = za::Color::Green,
                 .outlineColor       = za::Color::Red,
                 .outlineThickness   = 4.f,
                 .miterLimit         = 2.f,
                 .radius             = 25.f,
                 .startAngle         = za::degrees(15.f),
                 .pointCount         = 8u};

        checkParity(za::CircleShape{data}, data);
    }

    SECTION("RectangleShape vs RectangleShapeData")
    {
        const za::RectangleShapeData
            data{.position           = {-10.f, 20.f},
                 .scale              = {-1.25f, 3.f},
                 .origin             = {12.f, 4.f},
                 .rotation           = za::degrees(75.f),
                 .textureRect        = {{0.f, 0.f}, {64.f, 64.f}},
                 .outlineTextureRect = {{2.f, 2.f}, {8.f, 8.f}},
                 .fillColor          = za::Color::Blue,
                 .outlineColor       = za::Color::Yellow,
                 .outlineThickness   = 3.f,
                 .miterLimit         = 1.5f,
                 .size               = {40.f, 20.f}};

        checkParity(za::RectangleShape{data}, data);
    }
}
