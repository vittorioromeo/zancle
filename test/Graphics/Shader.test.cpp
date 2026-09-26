#include "Zancle/Graphics/Shader.hpp"

// Other 1st party headers
#include "Tst/Tst.hpp"

#include "Zancle/GLUtils/GLCheck.hpp"
#include "Zancle/GLUtils/Glad.hpp"

#include "Zancle/Graphics/Glsl.hpp"
#include "Zancle/Graphics/GraphicsContext.hpp"

#include "Zancle/IO/FileInputStream.hpp"
#include "Zancle/IO/Path.hpp"

#include "Zancle/Base/Macros.hpp"

#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsDefaultConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"


namespace
{
constexpr auto vertexSource = R"glsl(

layout(location = 0) uniform vec3 za_u_mvpRow0;
layout(location = 1) uniform vec3 za_u_mvpRow1;
layout(location = 3) uniform vec2 za_u_invTextureSize;

layout(location = 4) uniform vec2 storm_position;
layout(location = 5) uniform float storm_total_radius;
layout(location = 6) uniform float storm_inner_radius;

layout(location = 0) in vec2 za_a_position;
layout(location = 1) in vec4 za_a_color;
layout(location = 2) in vec2 za_a_texCoord;

out vec4 za_v_color;
out vec2 za_v_texCoord;

void main()
{
    vec2 newPosition = za_a_position;

    vec2 offset = newPosition.xy - storm_position;

    float len = length(offset);
    if (len < storm_total_radius)
    {
        float push_distance = storm_inner_radius + len / storm_total_radius * (storm_total_radius - storm_inner_radius);
        newPosition.xy      = storm_position + normalize(offset) * push_distance;
    }

    gl_Position   = vec4(dot(za_u_mvpRow0, vec3(newPosition, 1.0)), dot(za_u_mvpRow1, vec3(newPosition, 1.0)), 0.0, 1.0);
    za_v_texCoord = za_a_texCoord * za_u_invTextureSize;
    za_v_color    = za_a_color;
}

)glsl";

constexpr auto geometrySource = R"glsl(

// The render target's resolution (used for scaling)
layout(location = 8) uniform vec2 resolution;

// The billboards' size
layout(location = 9) uniform vec2 size;

// Input is the passed point cloud
layout(points) in;

// The output will consist of triangle strips with four vertices each
layout(triangle_strip, max_vertices = 4) out;

// Output texture coordinates
out vec2 za_v_texCoord;

out vec4 za_v_color; // Pass through to fragment

// Main entry point
void main()
{
    // Calculate the half width/height of the billboards
    vec2 half_size = size / 2.f;

    // Scale the size based on resolution (1 would be full width/height)
    half_size /= resolution;

    // Iterate over all vertices
    for (int i = 0; i < gl_in.length(); ++i)
    {
        // Retrieve the passed vertex position
        vec2 pos = gl_in[i].gl_Position.xy;

        // Bottom left vertex
        gl_Position   = vec4(pos - half_size, 0.f, 1.f);
        za_v_texCoord = vec2(1.f, 1.f);
        EmitVertex();

        // Bottom right vertex
        gl_Position   = vec4(pos.x + half_size.x, pos.y - half_size.y, 0.f, 1.f);
        za_v_texCoord = vec2(0.f, 1.f);
        EmitVertex();

        // Top left vertex
        gl_Position   = vec4(pos.x - half_size.x, pos.y + half_size.y, 0.f, 1.f);
        za_v_texCoord = vec2(1.f, 0.f);
        EmitVertex();

        // Top right vertex
        gl_Position   = vec4(pos + half_size, 0.f, 1.f);
        za_v_texCoord = vec2(0.f, 0.f);
        EmitVertex();

        // And finalize the primitive
        EndPrimitive();
    }
}

)glsl";

constexpr auto fragmentSource = R"glsl(

layout(location = 2) uniform sampler2D za_u_texture;
layout(location = 7) uniform float     blink_alpha;

in vec4 za_v_color;
in vec2 za_v_texCoord;

layout(location = 0) out vec4 za_fragColor;

void main()
{
    vec4 pixel = za_v_color;
    pixel.a    = blink_alpha;

    za_fragColor = pixel;
}

)glsl";

#ifdef ZA_RUN_DISPLAY_TESTS
constexpr bool skipShaderFullTest = false;
#else
constexpr bool skipShaderFullTest = true;
#endif

} // namespace

TEST_CASE("[Graphics] za::Shader" * tst::skip(skipShaderFullTest))
{
    [[maybe_unused]] auto& graphicsContext = TST_CASE_SHARED(za::GraphicsContext::create().value());

    SECTION("Type traits")
    {
        STATIC_CHECK(!ZA_IS_DEFAULT_CONSTRUCTIBLE(za::Shader));
        STATIC_CHECK(!ZA_IS_COPY_CONSTRUCTIBLE(za::Shader));
        STATIC_CHECK(!ZA_IS_COPY_ASSIGNABLE(za::Shader));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(za::Shader));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_ASSIGNABLE(za::Shader));
    }

    SECTION("Move semantics")
    {
        SECTION("Construction")
        {
            auto       movedShader = za::Shader::loadFromFile({.vertexPath = "shader.vert"}).value();
            const auto shader      = ZA_MOVE(movedShader);
            CHECK(shader.getNativeHandle() != 0);
        }

        SECTION("Assignment")
        {
            auto movedShader = za::Shader::loadFromFile({.vertexPath = "shader.vert"}).value();
            auto shader      = za::Shader::loadFromFile({.fragmentPath = "shader.frag"}).value();
            shader           = ZA_MOVE(movedShader);
            CHECK(shader.getNativeHandle() != 0);
        }
    }

    SECTION("loadFromFile()")
    {
        SECTION("One shader")
        {
            CHECK(!za::Shader::loadFromFile({.vertexPath = "does-not-exist.vert"}).hasValue());

            const auto vertexShader = za::Shader::loadFromFile({.vertexPath = "shader.vert"});
            CHECK(vertexShader.hasValue());
            if (vertexShader.hasValue())
                CHECK(static_cast<bool>(vertexShader->getNativeHandle()));

            const auto fragmentShader = za::Shader::loadFromFile({.fragmentPath = "shader.frag"});
            CHECK(fragmentShader.hasValue());
            if (fragmentShader.hasValue())
                CHECK(static_cast<bool>(fragmentShader->getNativeHandle()));
        }

        SECTION("Two shaders")
        {
            CHECK(!za::Shader::loadFromFile({.vertexPath = "does-not-exist.vert", .fragmentPath = "shader.frag"}).hasValue());

            CHECK(!za::Shader::loadFromFile({.vertexPath = "shader.vert", .fragmentPath = "does-not-exist.frag"}).hasValue());

            const auto shader = za::Shader::loadFromFile({.vertexPath = "shader.vert", .fragmentPath = "shader.frag"});

            CHECK(shader.hasValue());
            if (shader.hasValue())
                CHECK(static_cast<bool>(shader->getNativeHandle()));
        }

        SECTION("Three shaders")
        {
            CHECK(!za::Shader::loadFromFile(
                       {.vertexPath = "does-not-exist.vert", .fragmentPath = "shader.frag", .geometryPath = "shader.geom"})
                       .hasValue());

            CHECK(!za::Shader::loadFromFile(
                       {.vertexPath = "shader.vert", .fragmentPath = "shader.frag", .geometryPath = "does-not-exist.geom"})
                       .hasValue());

            CHECK(!za::Shader::loadFromFile(
                       {.vertexPath = "shader.vert", .fragmentPath = "does-not-exist.frag", .geometryPath = "shader.geom"})
                       .hasValue());

            const auto shader = za::Shader::loadFromFile(
                {.vertexPath = "shader.vert", .fragmentPath = "shader.frag", .geometryPath = "shader.geom"});

            CHECK(shader.hasValue() == za::Shader::isGeometryAvailable());
            if (shader.hasValue())
                CHECK(static_cast<bool>(shader->getNativeHandle()) == za::Shader::isGeometryAvailable());
        }

        SECTION("One shader with non-ASCII filename")
        {
            CHECK(za::Shader::loadFromFile({.vertexPath = U"shader-ń.vert"}).hasValue());
            CHECK(za::Shader::loadFromFile({.vertexPath = U"shader-🐌.vert"}).hasValue());
        }
    }

    SECTION("loadFromMemory()")
    {
        CHECK(za::Shader::loadFromMemory({.vertexCode = vertexSource}).hasValue());
        CHECK(za::Shader::loadFromMemory({.geometryCode = geometrySource}).hasValue() == za::Shader::isGeometryAvailable());
        CHECK(za::Shader::loadFromMemory({.fragmentCode = fragmentSource}).hasValue());
        CHECK(za::Shader::loadFromMemory({.vertexCode = vertexSource, .fragmentCode = fragmentSource}).hasValue());

        const auto shader = za::Shader::loadFromMemory(
            {.vertexCode = vertexSource, .fragmentCode = fragmentSource, .geometryCode = geometrySource});
        CHECK(shader.hasValue() == za::Shader::isGeometryAvailable());
        if (shader.hasValue())
            CHECK(static_cast<bool>(shader->getNativeHandle()));
    }

    SECTION("loadFromStream()")
    {
        auto vertexShaderStream   = za::FileInputStream::open("shader.vert").value();
        auto fragmentShaderStream = za::FileInputStream::open("shader.frag").value();
        auto geometryShaderStream = za::FileInputStream::open("shader.geom").value();

        auto emptyStream = za::FileInputStream::open("invalid_shader.vert").value();

        SECTION("One shader")
        {
            CHECK(!za::Shader::loadFromStream({.vertexStream = &emptyStream}).hasValue());
            CHECK(za::Shader::loadFromStream({.vertexStream = &vertexShaderStream}).hasValue());
            CHECK(za::Shader::loadFromStream({.fragmentStream = &fragmentShaderStream}).hasValue());
        }

        SECTION("Two shaders")
        {
            CHECK(!za::Shader::loadFromStream({.vertexStream = &emptyStream, .fragmentStream = &fragmentShaderStream})
                       .hasValue());
            CHECK(!za::Shader::loadFromStream({.vertexStream = &vertexShaderStream, .fragmentStream = &emptyStream}).hasValue());
            CHECK(za::Shader::loadFromStream({.vertexStream = &vertexShaderStream, .fragmentStream = &fragmentShaderStream})
                      .hasValue());
        }

        SECTION("Three shaders")
        {
            CHECK(!za::Shader::loadFromStream({.vertexStream   = &emptyStream,
                                               .fragmentStream = &fragmentShaderStream,
                                               .geometryStream = &geometryShaderStream})
                       .hasValue());
            CHECK(!za::Shader::loadFromStream({.vertexStream   = &vertexShaderStream,
                                               .fragmentStream = &fragmentShaderStream,
                                               .geometryStream = &emptyStream})
                       .hasValue());
            CHECK(!za::Shader::loadFromStream({.vertexStream   = &vertexShaderStream,
                                               .fragmentStream = &emptyStream,
                                               .geometryStream = &geometryShaderStream})
                       .hasValue());

            const auto shader = za::Shader::loadFromStream(
                {.vertexStream   = &vertexShaderStream,
                 .fragmentStream = &fragmentShaderStream,
                 .geometryStream = &geometryShaderStream});

            CHECK(shader.hasValue() == za::Shader::isGeometryAvailable());
            if (shader.hasValue())
                CHECK(static_cast<bool>(shader->getNativeHandle()) == za::Shader::isGeometryAvailable());
        }
    }

#ifndef ZA_SYSTEM_EMSCRIPTEN // see https://github.com/emscripten-core/emscripten/issues/26844
    SECTION("setUniformArray() readback")
    {
        // Fragment shader with uniform arrays at known layout locations.
        // All uniforms are referenced in main() to prevent driver elimination.
        constexpr auto uniformArrayFragSource = R"glsl(

layout(location = 10) uniform float u_floats[3];
layout(location = 13) uniform vec2  u_vec2s[2];
layout(location = 15) uniform vec3  u_vec3s[2];
layout(location = 17) uniform vec4  u_vec4s[2];
layout(location = 19) uniform mat3  u_mat3s[2];
layout(location = 25) uniform mat4  u_mat4s[2];

layout(location = 0) out vec4 za_fragColor;

void main()
{
    za_fragColor = vec4(u_floats[0] + u_floats[1] + u_floats[2])
                 + vec4(u_vec2s[0], u_vec2s[1])
                 + vec4(u_vec3s[0], u_vec3s[1].x)
                 + u_vec4s[0] + u_vec4s[1]
                 + vec4(u_mat3s[0][0], u_mat3s[1][0][0])
                 + vec4(u_mat4s[0][0]) + vec4(u_mat4s[1][0]);
}

)glsl";

        auto       shader  = za::Shader::loadFromMemory({.fragmentCode = uniformArrayFragSource}).value();
        const auto program = static_cast<GLuint>(shader.getNativeHandle());

        SECTION("float[]")
        {
            const float data[] = {1.5f, 2.5f, 3.5f};
            shader.setUniformArray(shader.getUniformLocation("u_floats").value(), data, 3);

            for (int i = 0; i < 3; ++i)
            {
                const char* names[] = {"u_floats[0]", "u_floats[1]", "u_floats[2]"};

                float readback = 0.f;
                glCheck(glGetUniformfv(program, shader.getUniformLocation(names[i]).value().getNativeHandle(), &readback));
                CHECK(readback == data[i]);
            }
        }

        SECTION("vec2[]")
        {
            const za::Glsl::Vec2 data[] = {{1.0f, 2.0f}, {3.0f, 4.0f}};
            shader.setUniformArray(shader.getUniformLocation("u_vec2s").value(), data, 2);

            for (int i = 0; i < 2; ++i)
            {
                const char* names[] = {"u_vec2s[0]", "u_vec2s[1]"};

                float readback[2]{};
                glCheck(glGetUniformfv(program, shader.getUniformLocation(names[i]).value().getNativeHandle(), readback));
                CHECK(readback[0] == data[i].x);
                CHECK(readback[1] == data[i].y);
            }
        }

        SECTION("vec3[]")
        {
            const za::Glsl::Vec3 data[] = {{1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}};
            shader.setUniformArray(shader.getUniformLocation("u_vec3s").value(), data, 2);

            for (int i = 0; i < 2; ++i)
            {
                const char* names[] = {"u_vec3s[0]", "u_vec3s[1]"};

                float readback[3]{};
                glCheck(glGetUniformfv(program, shader.getUniformLocation(names[i]).value().getNativeHandle(), readback));
                CHECK(readback[0] == data[i].x);
                CHECK(readback[1] == data[i].y);
                CHECK(readback[2] == data[i].z);
            }
        }

        SECTION("vec4[]")
        {
            const za::Glsl::Vec4 data[] = {{1.0f, 2.0f, 3.0f, 4.0f}, {5.0f, 6.0f, 7.0f, 8.0f}};
            shader.setUniformArray(shader.getUniformLocation("u_vec4s").value(), data, 2);

            for (int i = 0; i < 2; ++i)
            {
                const char* names[] = {"u_vec4s[0]", "u_vec4s[1]"};

                float readback[4]{};
                glCheck(glGetUniformfv(program, shader.getUniformLocation(names[i]).value().getNativeHandle(), readback));
                CHECK(readback[0] == data[i].x);
                CHECK(readback[1] == data[i].y);
                CHECK(readback[2] == data[i].z);
                CHECK(readback[3] == data[i].w);
            }
        }

        SECTION("mat3[]")
        {
            const float raw0[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
            const float raw1[9] = {10, 20, 30, 40, 50, 60, 70, 80, 90};

            const za::Glsl::Mat3 data[] = {za::Glsl::Mat3(raw0), za::Glsl::Mat3(raw1)};
            shader.setUniformArray(shader.getUniformLocation("u_mat3s").value(), data, 2);

            float readback0[9]{};
            glCheck(glGetUniformfv(program, shader.getUniformLocation("u_mat3s[0]").value().getNativeHandle(), readback0));
            for (int j = 0; j < 9; ++j)
                CHECK(readback0[j] == raw0[j]);

            float readback1[9]{};
            glCheck(glGetUniformfv(program, shader.getUniformLocation("u_mat3s[1]").value().getNativeHandle(), readback1));
            for (int j = 0; j < 9; ++j)
                CHECK(readback1[j] == raw1[j]);
        }

        SECTION("mat4[]")
        {
            const float raw0[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
            const float raw1[16] = {16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1};

            const za::Glsl::Mat4 data[] = {za::Glsl::Mat4(raw0), za::Glsl::Mat4(raw1)};
            shader.setUniformArray(shader.getUniformLocation("u_mat4s").value(), data, 2);

            float readback0[16]{};
            glCheck(glGetUniformfv(program, shader.getUniformLocation("u_mat4s[0]").value().getNativeHandle(), readback0));
            for (int j = 0; j < 16; ++j)
                CHECK(readback0[j] == raw0[j]);

            float readback1[16]{};
            glCheck(glGetUniformfv(program, shader.getUniformLocation("u_mat4s[1]").value().getNativeHandle(), readback1));
            for (int j = 0; j < 16; ++j)
                CHECK(readback1[j] == raw1[j]);
        }
    }
#endif
}
