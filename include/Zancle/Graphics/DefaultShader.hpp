#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
template <typename T>
class Optional;
}

namespace za
{
class Shader;
}


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Provides access to Zancle's built-in default shader sources and creation
///
/// `za::DefaultShader` is a utility struct that serves two main purposes:
///
/// 1. It stores the GLSL source code for Zancle's default vertex (`srcVertex`)
///    and fragment (`srcFragment`) shaders. These shaders are used internally
///    by Zancle when no custom shader is specified, providing basic 2D texturing
///    and coloring.
///
/// 2. It offers a static factory method (`create()`) to easily compile and
///    link these built-in sources into a ready-to-use `za::Shader` object.
///
/// This is useful for users who might want to inspect the default shader code
/// or create an instance of the default shader, perhaps as a base for their
/// own modifications or as a fallback.
///
////////////////////////////////////////////////////////////
struct DefaultShader
{
    ////////////////////////////////////////////////////////////
    /// \brief GLSL source code for the built-in default vertex shader
    ///
    /// This shader handles basic 2D transformations (model-view-projection matrix)
    /// and passes vertex color and texture coordinates to the fragment shader.
    /// Texture coordinates are normalized based on the texture size.
    ///
    /// Inputs (attributes):
    /// - `vec2 za_a_position`: Vertex position
    /// - `vec4 za_a_color`: Vertex color
    /// - `vec2 za_a_texCoord`: Non-normalized vertex texture coordinates
    ///
    /// Uniforms:
    /// - `vec3 za_u_mvpRow0`: First row of the 2D MVP transform (a00, a01, a02)
    /// - `vec3 za_u_mvpRow1`: Second row of the 2D MVP transform (a10, a11, a12)
    /// - `vec2 za_u_invTextureSize`: Inverse of the texture dimensions (1/width, 1/height)
    ///
    /// Outputs (varyings):
    /// - `vec4 za_v_color`: Passed-through vertex color
    /// - `vec2 za_v_texCoord`: Normalized texture coordinates
    ///
    ////////////////////////////////////////////////////////////
    static inline constexpr const char* srcVertex = R"glsl(

layout(location = 0) uniform vec3 za_u_mvpRow0;
layout(location = 1) uniform vec3 za_u_mvpRow1;
layout(location = 2) uniform sampler2D za_u_texture;
layout(location = 3) uniform vec2 za_u_invTextureSize;

layout(location = 0) in vec2 za_a_position;
layout(location = 1) in vec4 za_a_color;
layout(location = 2) in vec2 za_a_texCoord;

out vec4 za_v_color;
out vec2 za_v_texCoord;

void main()
{
    vec3 pos = vec3(za_a_position, 1.0);

    gl_Position = vec4(dot(za_u_mvpRow0, pos), dot(za_u_mvpRow1, pos), 0.0, 1.0);
    za_v_color = za_a_color;
    za_v_texCoord = za_a_texCoord * za_u_invTextureSize;
}

)glsl";

    ////////////////////////////////////////////////////////////
    /// \brief GLSL source code for the built-in default fragment shader
    ///
    /// This shader samples a texture at the given texture coordinates
    /// and multiplies the sampled color by the vertex color to produce
    /// the final fragment color.
    ///
    /// Inputs (varyings):
    /// - `vec4 za_v_color`: Interpolated vertex color
    /// - `vec2 za_v_texCoord`: Interpolated normalized texture coordinates
    ///
    /// Uniforms:
    /// - `sampler2D za_u_texture`: The texture to be sampled
    ///
    /// Outputs (fragment color):
    /// - `vec4 za_fragColor`: Final color of the fragment
    ///
    ////////////////////////////////////////////////////////////
    static inline constexpr const char* srcFragment = R"glsl(

layout(location = 2) uniform sampler2D za_u_texture;

in vec4 za_v_color;
in vec2 za_v_texCoord;

layout(location = 0) out vec4 za_fragColor;

void main()
{
    za_fragColor = za_v_color * texture(za_u_texture, za_v_texCoord);
}

)glsl";

    ////////////////////////////////////////////////////////////
    /// \brief Create an `za::Shader` instance from the default shader sources
    ///
    /// This static method attempts to compile `srcVertex` and `srcFragment`
    /// and link them into a new `za::Shader` object.
    ///
    /// This is a convenient way to get a ready-to-use instance of Zancle's
    /// default shader program.
    ///
    /// \return An `za::Optional<za::Shader>` containing the compiled
    ///         shader if successful, or `za::nullOpt` if compilation
    ///         or linking failed.
    ///
    /// \see za::Shader::loadFromMemory
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<Shader> create();
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::DefaultShader
/// \ingroup graphics
///
/// `za::DefaultShader` provides access to Zancle's built-in
/// default vertex and fragment shader sources. It also offers
/// a convenient way to create an `za::Shader` object from these
/// default sources.
///
/// This can be useful if you need a basic shader for rendering
/// textured and colored 2D objects, or if you want to inspect
/// or extend Zancle's default rendering behavior.
///
/// Example:
/// \code
/// // Attempt to create an instance of the default shader
/// auto defaultShader = za::DefaultShader::create();
///
/// if (defaultShader.hasValue())
/// {
///     // Use the default shader for rendering
///     // renderStates.shader = &(*defaultShader);
/// }
/// else
/// {
///     // Shader creation failed, handle error
/// }
///
/// // Accessing the source code directly
/// const char* vertexShaderSource = za::DefaultShader::srcVertex;
/// const char* fragmentShaderSource = za::DefaultShader::srcFragment;
/// \endcode
///
/// \see za::Shader
///
////////////////////////////////////////////////////////////
