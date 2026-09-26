#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Export.hpp"

#include "Zancle/Graphics/Priv/GlslFwd.hpp"

#include "Zancle/Container/Array.hpp"

#include "Zancle/Vocabulary/InPlacePImpl.hpp"
#include "Zancle/Vocabulary/PassKey.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/SizeT.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
class InputStream;
class RenderTarget;
class Texture;
} // namespace za

namespace za
{
template <typename>
class Optional;

class StringView;
} // namespace za


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Shader class (vertex, geometry and fragment)
///
////////////////////////////////////////////////////////////
class ZA_GRAPHICS_API Shader
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Special type that can be passed to setUniform(),
    ///        and that represents the texture of the object being drawn
    ///
    /// \see `setUniform(UniformLocation, CurrentTextureType)`
    ///
    ////////////////////////////////////////////////////////////
    struct CurrentTextureType
    {
    };

    ////////////////////////////////////////////////////////////
    /// \brief Represents the texture of the object being drawn
    ///
    /// \see `setUniform(UniformLocation, CurrentTextureType)`
    ///
    ////////////////////////////////////////////////////////////
    // NOLINTNEXTLINE(readability-identifier-naming)
    static inline constexpr CurrentTextureType CurrentTexture{};

    ////////////////////////////////////////////////////////////
    /// \brief Type-safe wrapper over a non-null shader uniform location
    ///
    ////////////////////////////////////////////////////////////
    class [[nodiscard]] UniformLocation
    {
        friend Shader;

    private:
        [[nodiscard]] explicit UniformLocation(int location);
        int m_value;

    public:
        [[nodiscard, gnu::always_inline, gnu::pure]] int getNativeHandle() const
        {
            return m_value;
        }
    };

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    ////////////////////////////////////////////////////////////
    ~Shader();

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy constructor
    ///
    ////////////////////////////////////////////////////////////
    Shader(const Shader&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy assignment
    ///
    ////////////////////////////////////////////////////////////
    Shader& operator=(const Shader&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Move constructor
    ///
    ////////////////////////////////////////////////////////////
    Shader(Shader&& source) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Move assignment
    ///
    ////////////////////////////////////////////////////////////
    Shader& operator=(Shader&& rhs) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Settings for `loadFromFile`
    ///
    /// Each path is optional: leave any field empty to skip that
    /// shader stage. The default vertex and/or fragment shader will
    /// be used in place of any stage that is not provided.
    ///
    /// \note Zancle automatically prepends a `#version` and default
    ///       `precision` preamble to every source; the provided sources
    ///       must not contain their own `#version` or global `precision`
    ///       directives, or compilation will fail.
    ///
    /// \see `loadFromFile`
    ///
    ////////////////////////////////////////////////////////////
    struct LoadFromFileSettings;

    ////////////////////////////////////////////////////////////
    /// \brief Load the vertex, geometry and fragment shaders from files
    ///
    /// This function loads the vertex, geometry and fragment
    /// shaders specified by `settings`. If one of them fails to
    /// load, this function returns `za::nullOpt`. Any stage
    /// whose path is left empty is replaced by the corresponding
    /// default shader.
    ///
    /// The sources must be text files containing valid shaders
    /// in GLSL language. GLSL is a C-like language dedicated to
    /// OpenGL shaders; you'll probably need to read a good documentation
    /// for it before writing your own shaders.
    ///
    /// Example:
    /// \code
    /// auto shader = za::Shader::loadFromFile({
    ///     .vertexPath   = "resources/wave.vert",
    ///     .fragmentPath = "resources/blur.frag",
    /// });
    /// \endcode
    ///
    /// \param settings Paths of the shader stages to load
    ///
    /// \return Shader if loading succeeded, `za::nullOpt` if it failed
    ///
    /// \see `loadFromMemory`, `loadFromStream`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<Shader> loadFromFile(const LoadFromFileSettings& settings);

    ////////////////////////////////////////////////////////////
    /// \brief Settings for `loadFromMemory`
    ///
    /// Each source view is optional: leave any field empty to skip
    /// that shader stage. The default vertex and/or fragment shader
    /// will be used in place of any stage that is not provided.
    ///
    /// \note Zancle automatically prepends a `#version` and default
    ///       `precision` preamble to every source; the provided sources
    ///       must not contain their own `#version` or global `precision`
    ///       directives, or compilation will fail.
    ///
    /// \see `loadFromMemory`
    ///
    ////////////////////////////////////////////////////////////
    struct LoadFromMemorySettings;

    ////////////////////////////////////////////////////////////
    /// \brief Load the vertex, geometry and fragment shaders from source codes in memory
    ///
    /// This function loads the vertex, geometry and fragment
    /// shaders specified by `settings`. If one of them fails to
    /// load, this function returns `za::nullOpt`. Any stage
    /// whose source code is left empty is replaced by the
    /// corresponding default shader.
    ///
    /// The sources must be valid shaders in GLSL language. GLSL is
    /// a C-like language dedicated to OpenGL shaders; you'll
    /// probably need to read a good documentation for it before
    /// writing your own shaders.
    ///
    /// \param settings Source code of the shader stages to load
    ///
    /// \return Shader if loading succeeded, `za::nullOpt` if it failed
    ///
    /// \see `loadFromFile`, `loadFromStream`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<Shader> loadFromMemory(const LoadFromMemorySettings& settings);

    ////////////////////////////////////////////////////////////
    /// \brief Settings for `loadFromStream`
    ///
    /// Each stream pointer is optional: leave any field as
    /// `nullptr` to skip that shader stage. The default vertex
    /// and/or fragment shader will be used in place of any stage
    /// that is not provided.
    ///
    /// \see `loadFromStream`
    ///
    ////////////////////////////////////////////////////////////
    struct LoadFromStreamSettings
    {
        InputStream* vertexStream{nullptr};   //!< Stream to read the vertex shader from (`nullptr` to skip)
        InputStream* fragmentStream{nullptr}; //!< Stream to read the fragment shader from (`nullptr` to skip)
        InputStream* geometryStream{nullptr}; //!< Stream to read the geometry shader from (`nullptr` to skip)
    };

    ////////////////////////////////////////////////////////////
    /// \brief Load the vertex, geometry and fragment shaders from custom streams
    ///
    /// This function loads the vertex, geometry and fragment
    /// shaders specified by `settings`. If one of them fails to
    /// load, this function returns `za::nullOpt`. Any stage
    /// whose stream is `nullptr` is replaced by the corresponding
    /// default shader.
    ///
    /// The source codes must be valid shaders in GLSL language.
    /// GLSL is a C-like language dedicated to OpenGL shaders;
    /// you'll probably need to read a good documentation for
    /// it before writing your own shaders.
    ///
    /// \note The streams must report a known, non-zero size via
    ///       `getSize()`; unsized streams are not supported.
    ///
    /// \param settings Streams of the shader stages to load
    ///
    /// \return Shader if loading succeeded, `za::nullOpt` if it failed
    ///
    /// \see `loadFromFile`, `loadFromMemory`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<Shader> loadFromStream(const LoadFromStreamSettings& settings);

    ////////////////////////////////////////////////////////////
    /// \brief Get the location of a shader uniform
    ///
    /// The returned `UniformLocation` can be cached and passed
    /// to the various `setUniform` overloads to avoid the cost of
    /// looking the uniform up by name on every call.
    ///
    /// \param uniformName Name of the uniform variable to search
    ///
    /// \return Location of the uniform, or `za::nullOpt` if not found
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Optional<UniformLocation> getUniformLocation(za::StringView uniformName) const;

    ////////////////////////////////////////////////////////////
    /// \brief Specify value for \p float uniform
    ///
    /// \param location Location of the uniform variable in GLSL
    /// \param x        Value of the float scalar
    ///
    ////////////////////////////////////////////////////////////
    void setUniform(UniformLocation location, float x) const;

    ////////////////////////////////////////////////////////////
    /// \brief Specify value for \p vec2 uniform
    ///
    /// \param location Location of the uniform variable in GLSL
    /// \param vec      Value of the vec2
    ///
    ////////////////////////////////////////////////////////////
    void setUniform(UniformLocation location, Glsl::Vec2 vec) const;

    ////////////////////////////////////////////////////////////
    /// \brief Specify value for \p vec3 uniform
    ///
    /// \param location Location of the uniform variable in GLSL
    /// \param vec      Value of the vec3
    ///
    ////////////////////////////////////////////////////////////
    void setUniform(UniformLocation location, const Glsl::Vec3& vec) const;

    ////////////////////////////////////////////////////////////
    /// \brief Specify value for \p vec4 uniform
    ///
    /// To pass a `za::Color`, convert it explicitly, as in
    /// `setUniform(location, za::Glsl::Vec4(color))`. A plain
    /// `setUniform(location, color)` call is ambiguous between the
    /// `Glsl::Vec4` and `Glsl::Ivec4` overloads and does not compile.
    ///
    /// It is important to note that the components of the color are
    /// normalized before being passed to the shader. Therefore,
    /// they are converted from range [0 .. 255] to range [0 .. 1].
    /// For example, a `za::Color(255, 127, 0, 255)` will be transformed
    /// to a `vec4(1.0, 0.5, 0.0, 1.0)` in the shader.
    ///
    /// \param location Location of the uniform variable in GLSL
    /// \param vec      Value of the vec4
    ///
    ////////////////////////////////////////////////////////////
    void setUniform(UniformLocation location, const Glsl::Vec4& vec) const;

    ////////////////////////////////////////////////////////////
    /// \brief Specify value for \p int uniform
    ///
    /// \param location Location of the uniform variable in GLSL
    /// \param x        Value of the int scalar
    ///
    ////////////////////////////////////////////////////////////
    void setUniform(UniformLocation location, int x) const;

    ////////////////////////////////////////////////////////////
    /// \brief Specify value for \p ivec2 uniform
    ///
    /// \param location Location of the uniform variable in GLSL
    /// \param vec      Value of the ivec2
    ///
    ////////////////////////////////////////////////////////////
    void setUniform(UniformLocation location, Glsl::Ivec2 vec) const;

    ////////////////////////////////////////////////////////////
    /// \brief Specify value for \p ivec3 uniform
    ///
    /// \param location Location of the uniform variable in GLSL
    /// \param vec      Value of the ivec3
    ///
    ////////////////////////////////////////////////////////////
    void setUniform(UniformLocation location, const Glsl::Ivec3& vec) const;

    ////////////////////////////////////////////////////////////
    /// \brief Specify value for \p ivec4 uniform
    ///
    /// To pass a `za::Color`, convert it explicitly, as in
    /// `setUniform(location, za::Glsl::Ivec4(color))` -- a plain
    /// `setUniform(location, color)` call is ambiguous and does not compile.
    ///
    /// If color conversions are used, the ivec4 uniform in GLSL
    /// will hold the same values as the original `za::Color`
    /// instance. For example, `za::Color(255, 127, 0, 255)` is
    /// mapped to `ivec4(255, 127, 0, 255)`.
    ///
    /// \param location Location of the uniform variable in GLSL
    /// \param vec      Value of the ivec4
    ///
    ////////////////////////////////////////////////////////////
    void setUniform(UniformLocation location, const Glsl::Ivec4& vec) const;

    ////////////////////////////////////////////////////////////
    /// \brief Specify value for \p bool uniform
    ///
    /// \param location Location of the uniform variable in GLSL
    /// \param x        Value of the bool scalar
    ///
    ////////////////////////////////////////////////////////////
    void setUniform(UniformLocation location, bool x) const;

    ////////////////////////////////////////////////////////////
    /// \brief Specify value for \p bvec2 uniform
    ///
    /// \param location Location of the uniform variable in GLSL
    /// \param vec      Value of the bvec2
    ///
    ////////////////////////////////////////////////////////////
    void setUniform(UniformLocation location, Glsl::Bvec2 vec) const;

    ////////////////////////////////////////////////////////////
    /// \brief Specify value for \p bvec3 uniform
    ///
    /// \param location Location of the uniform variable in GLSL
    /// \param vec      Value of the bvec3
    ///
    ////////////////////////////////////////////////////////////
    void setUniform(UniformLocation location, const Glsl::Bvec3& vec) const;

    ////////////////////////////////////////////////////////////
    /// \brief Specify value for \p bvec4 uniform
    ///
    /// \param location Location of the uniform variable in GLSL
    /// \param vec      Value of the bvec4
    ///
    ////////////////////////////////////////////////////////////
    void setUniform(UniformLocation location, const Glsl::Bvec4& vec) const;

    ////////////////////////////////////////////////////////////
    /// \brief Specify value for \p mat3 matrix
    ///
    /// \param location Location of the uniform variable in GLSL
    /// \param matrix   Value of the mat3 matrix
    ///
    ////////////////////////////////////////////////////////////
    void setUniform(UniformLocation location, const Glsl::Mat3& matrix) const;

    ////////////////////////////////////////////////////////////
    /// \brief Specify value for \p mat4 matrix
    ///
    /// \param location Location of the uniform variable in GLSL
    /// \param matrix   Value of the mat4 matrix
    ///
    ////////////////////////////////////////////////////////////
    void setUniform(UniformLocation location, const Glsl::Mat4& matrix) const;

    ////////////////////////////////////////////////////////////
    /// \brief Specify a texture as \p sampler2D uniform
    ///
    /// The corresponding parameter in the shader must be a 2D
    /// texture (\p sampler2D GLSL type).
    ///
    /// Example:
    /// \code
    /// uniform sampler2D the_texture; // this is the variable in the shader
    /// \endcode
    /// \code
    /// za::Texture texture;
    /// ...
    /// const auto loc = shader.getUniformLocation("the_texture").value();
    /// shader.setUniform(loc, texture);
    /// \endcode
    /// It is important to note that `texture` must remain alive as long
    /// as the shader uses it, no copy is made internally.
    ///
    /// To use the texture of the object being drawn, which cannot be
    /// known in advance, you can pass the special value
    /// `za::Shader::CurrentTexture`:
    /// \code
    /// shader.setUniform(loc, za::Shader::CurrentTexture);
    /// \endcode
    ///
    /// \param location Location of the texture uniform in the shader
    /// \param texture  Texture to assign
    ///
    /// \return `true` on success, `false` if no free texture units are available
    ///
    /// \warning Updates an internal CPU-side map; not thread-safe. `za::Shader`
    ///          must only be mutated and bound from a single thread.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool setUniform(UniformLocation location, const Texture& texture) const;

    ////////////////////////////////////////////////////////////
    /// \brief Disallow setting from a temporary texture
    ///
    ////////////////////////////////////////////////////////////
    void setUniform(UniformLocation location, const Texture&& texture) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Specify current texture as \p sampler2D uniform
    ///
    /// This overload maps a shader texture variable to the
    /// texture of the object being drawn, which cannot be
    /// known in advance. The second argument must be
    /// `za::Shader::CurrentTexture`.
    /// The corresponding parameter in the shader must be a 2D texture
    /// (\p sampler2D GLSL type).
    ///
    /// Example:
    /// \code
    /// uniform sampler2D current; // this is the variable in the shader
    /// \endcode
    /// \code
    /// const auto loc = shader.getUniformLocation("current").value();
    /// shader.setUniform(loc, za::Shader::CurrentTexture);
    /// \endcode
    ///
    /// \param location Location of the texture uniform in the shader
    ///
    ////////////////////////////////////////////////////////////
    void setUniform(UniformLocation location, CurrentTextureType);

    ////////////////////////////////////////////////////////////
    /// \brief Specify values for \p float[] array uniform
    ///
    /// \param location    Location of the uniform variable in GLSL
    /// \param scalarArray Pointer to array of \p float values
    /// \param length      Number of elements in the array
    ///
    ////////////////////////////////////////////////////////////
    void setUniformArray(UniformLocation location, const float* scalarArray, za::SizeT length);

    ////////////////////////////////////////////////////////////
    /// \brief Specify values for \p vec2[] array uniform
    ///
    /// \param location Location of the uniform variable in GLSL
    /// \param vecArray Pointer to array of \p vec2 values
    /// \param length   Number of elements in the array
    ///
    ////////////////////////////////////////////////////////////
    void setUniformArray(UniformLocation location, const Glsl::Vec2* vecArray, za::SizeT length);

    ////////////////////////////////////////////////////////////
    /// \brief Specify values for \p vec3[] array uniform
    ///
    /// \param location Location of the uniform variable in GLSL
    /// \param vecArray Pointer to array of \p vec3 values
    /// \param length   Number of elements in the array
    ///
    ////////////////////////////////////////////////////////////
    void setUniformArray(UniformLocation location, const Glsl::Vec3* vecArray, za::SizeT length);

    ////////////////////////////////////////////////////////////
    /// \brief Specify values for \p vec4[] array uniform
    ///
    /// \param location Location of the uniform variable in GLSL
    /// \param vecArray Pointer to array of \p vec4 values
    /// \param length   Number of elements in the array
    ///
    ////////////////////////////////////////////////////////////
    void setUniformArray(UniformLocation location, const Glsl::Vec4* vecArray, za::SizeT length);

    ////////////////////////////////////////////////////////////
    /// \brief Specify values for \p mat3[] array uniform
    ///
    /// \param location    Location of the uniform variable in GLSL
    /// \param matrixArray Pointer to array of \p mat3 values
    /// \param length      Number of elements in the array
    ///
    ////////////////////////////////////////////////////////////
    void setUniformArray(UniformLocation location, const Glsl::Mat3* matrixArray, za::SizeT length);

    ////////////////////////////////////////////////////////////
    /// \brief Specify values for \p mat4[] array uniform
    ///
    /// \param location    Location of the uniform variable in GLSL
    /// \param matrixArray Pointer to array of \p mat4 values
    /// \param length      Number of elements in the array
    ///
    ////////////////////////////////////////////////////////////
    void setUniformArray(UniformLocation location, const Glsl::Mat4* matrixArray, za::SizeT length);

    ////////////////////////////////////////////////////////////
    /// \brief Get the underlying OpenGL handle of the shader.
    ///
    /// You shouldn't need to use this function, unless you have
    /// very specific stuff to implement that Zancle doesn't support,
    /// or implement a temporary workaround until a bug is fixed.
    ///
    /// \return OpenGL handle of the shader or 0 if not yet loaded
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] unsigned int getNativeHandle() const;

    ////////////////////////////////////////////////////////////
    /// \brief Bind this shader for rendering
    ///
    /// This function is not part of the graphics API, it mustn't be
    /// used when drawing Zancle entities. It must be used only if you
    /// mix `za::Shader` with raw OpenGL code.
    ///
    /// Call `Shader::unbind()` to revert to using no shader.
    ///
    /// \code
    /// za::Shader s1 = ...;
    /// za::Shader s2 = ...;
    ///
    /// s1.bind();
    /// // draw OpenGL stuff that uses s1...
    /// s2.bind();
    /// // draw OpenGL stuff that uses s2...
    /// za::Shader::unbind();
    /// // draw OpenGL stuff that uses no shader...
    /// \endcode
    ///
    /// \see `unbind`
    ///
    ////////////////////////////////////////////////////////////
    void bind() const;

    ////////////////////////////////////////////////////////////
    /// \brief Unbind any bound shader
    ///
    /// \see `bind`
    ///
    ////////////////////////////////////////////////////////////
    static void unbind();

    ////////////////////////////////////////////////////////////
    /// \brief Tell whether or not the system supports geometry shaders
    ///
    /// This function should always be called before using
    /// the geometry shader features. If it returns `false`, then
    /// any attempt to use `za::Shader` geometry shader features will fail.
    ///
    /// Note: The first call to this function, whether by your
    /// code or Zancle will result in a context switch.
    ///
    /// \return `true` if geometry shaders are supported, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static bool isGeometryAvailable();

    ////////////////////////////////////////////////////////////
    /// \private
    ///
    /// \brief Construct from shader program
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit Shader(za::PassKey<Shader>&&, unsigned int shaderProgram);

private:
    friend RenderTarget;

    ////////////////////////////////////////////////////////////
    /// \brief More efficient but less safe way of setting 4x4 matrix uniform
    ///
    ////////////////////////////////////////////////////////////
    void setMat4Uniform(UniformLocation location, const float* matrixPtr) const;

    ////////////////////////////////////////////////////////////
    /// \brief Compile the shader(s) and create the program
    ///
    /// If one of the arguments is empty, the corresponding stage is skipped:
    /// the built-in default shader is used for vertex/fragment, and the
    /// geometry stage is simply omitted.
    ///
    /// \param vertexShaderCode   Source code of the vertex shader
    /// \param geometryShaderCode Source code of the geometry shader
    /// \param fragmentShaderCode Source code of the fragment shader
    ///
    /// \return Shader on success, `za::nullOpt` if any error happened
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<Shader> compile(za::StringView vertexShaderCode,
                                                      za::StringView geometryShaderCode,
                                                      za::StringView fragmentShaderCode);

    ////////////////////////////////////////////////////////////
    /// \brief Bind all the textures used by the shader
    ///
    /// This function each texture to a different unit, and
    /// updates the corresponding variables in the shader accordingly.
    ///
    ////////////////////////////////////////////////////////////
    void bindTextures() const;

    ////////////////////////////////////////////////////////////
    /// \brief RAII object to save and restore the program
    ///        binding while uniforms are being set
    ///
    /// Implementation is private in the .cpp file.
    ///
    ////////////////////////////////////////////////////////////
    class [[nodiscard]] UniformBinder;

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    struct Impl;
    za::InPlacePImpl<Impl, 192> m_impl; //!< Implementation details

    mutable za::U32 m_uniformGeneration{0}; //!< Bumped on every uniform mutation (autobatch invalidation)

    mutable za::U32 m_textureBindingsGeneration{0}; //!< Bumped on every texture uniform (re)assignment (draw-path rebind)

    // Shadows of the last uploaded built-in uniform values, used by
    // `RenderTarget` to skip redundant uploads. Uniform values are stored in
    // the program object itself (shared across GL contexts and render
    // targets), so this cache must live here: a per-render-target cache goes
    // stale as soon as another render target draws with the same program.
    // Zero-initialized to match the GL-mandated post-link uniform values.
    mutable za::Array<float, 3> m_lastUploadedMVPRow0{};        //!< Last uploaded `za_u_mvpRow0` value
    mutable za::Array<float, 3> m_lastUploadedMVPRow1{};        //!< Last uploaded `za_u_mvpRow1` value
    mutable za::Array<float, 2> m_lastUploadedInvTextureSize{}; //!< Last uploaded `za_u_invTextureSize` value

    bool m_hasBuiltInUniformMVPRow0;        //!< Whether the shader has the built-in `za_u_mvpRow0` uniform
    bool m_hasBuiltInUniformMVPRow1;        //!< Whether the shader has the built-in `za_u_mvpRow1` uniform
    bool m_hasBuiltInUniformInvTextureSize; //!< Whether the shader has the built-in `za_u_invTextureSize` uniform
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::Shader
/// \ingroup graphics
///
/// Shaders are programs written using a specific language,
/// executed directly by the graphics card and allowing
/// to apply real-time operations to the rendered entities.
///
/// There are three kinds of shaders:
/// \li %Vertex shaders, that process vertices
/// \li Geometry shaders, that process primitives
/// \li Fragment (pixel) shaders, that process pixels
///
/// A `za::Shader` can be composed of either a vertex shader
/// alone, a geometry shader alone, a fragment shader alone,
/// or any combination of them. (see the variants of the
/// load functions).
///
/// Shaders are written in GLSL, which is a C-like
/// language dedicated to OpenGL shaders. You'll probably
/// need to learn its basics before writing your own shaders
/// for Zancle.
///
/// Like any C/C++ program, a GLSL shader has its own variables
/// called _uniforms_ that you can set from your C++ application.
/// `za::Shader` handles different types of uniforms:
/// \li scalars: \p float, \p int, \p bool
/// \li vectors (2, 3 or 4 components)
/// \li matrices (3x3 or 4x4)
/// \li samplers (textures)
///
/// Some Zancle-specific types can be converted:
/// \li `za::Color` as a 4D vector (\p vec4)
/// \li `za::Transform` as matrices (\p mat3 or \p mat4)
///
/// Every uniform variable in a shader can be set through one of the
/// `setUniform()` or `setUniformArray()` overloads. The overloads take
/// a `Shader::UniformLocation`, which is obtained by calling
/// `getUniformLocation()` with the name of the uniform variable.
/// Locations should be cached and reused rather than queried on every
/// frame, as the lookup has a non-trivial cost.
///
/// For example, if you have a shader with the following uniforms:
/// \code
/// uniform float offset;
/// uniform vec3 point;
/// uniform vec4 color;
/// uniform mat4 matrix;
/// uniform sampler2D overlay;
/// uniform sampler2D current;
/// \endcode
/// You can set their values from C++ code as follows, using the types
/// defined in the `za::Glsl` namespace:
/// \code
/// const auto ulOffset  = shader.getUniformLocation("offset").value();
/// const auto ulPoint   = shader.getUniformLocation("point").value();
/// const auto ulColor   = shader.getUniformLocation("color").value();
/// const auto ulMatrix  = shader.getUniformLocation("matrix").value();
/// const auto ulOverlay = shader.getUniformLocation("overlay").value();
/// const auto ulCurrent = shader.getUniformLocation("current").value();
///
/// shader.setUniform(ulOffset,  2.f);
/// shader.setUniform(ulPoint,   za::Vec3f(0.5f, 0.8f, 0.3f));
/// shader.setUniform(ulColor,   za::Glsl::Vec4(color));          // color is a za::Color
/// shader.setUniform(ulMatrix,  za::Glsl::Mat4(transform));      // transform is a za::Transform
/// shader.setUniform(ulOverlay, texture);                        // texture is a za::Texture
/// shader.setUniform(ulCurrent, za::Shader::CurrentTexture);
/// \endcode
///
/// The special `Shader::CurrentTexture` argument maps the
/// given \p sampler2D uniform to the current texture of the
/// object being drawn (which cannot be known in advance).
///
/// To apply a shader to a drawable, you must pass it as an
/// additional parameter to the `RenderWindow::draw` function:
/// \code
/// window.draw(sprite, {.shader = &shader});
/// \endcode
///
/// In the code above we pass a pointer to the shader, because it may
/// be null (which means "no shader").
///
/// Shaders can be used on any drawable, but some combinations are
/// not interesting. For example, using a vertex shader on a `za::Sprite`
/// is limited because there are only 4 vertices, the sprite would
/// have to be subdivided in order to apply wave effects.
/// Another bad example is a fragment shader with `za::Text`: the texture
/// of the text is not the actual text that you see on screen, it is
/// a big texture containing all the characters of the font in an
/// arbitrary order; thus, texture lookups on pixels other than the
/// current one may not give you the expected result.
///
/// Shaders can also be used to apply global post-effects to the
/// current contents of the target.
/// This can be done in two different ways:
/// \li draw everything to a `za::RenderTexture`, then draw it to
///     the main target using the shader
/// \li draw everything directly to the main target, then use
///     `za::Texture::update(Window&)` to copy its contents to a texture
///     and draw it to the main target using the shader
///
/// The first technique is more optimized because it doesn't involve
/// retrieving the target's pixels to system memory, but the
/// second one doesn't impact the rendering process and can be
/// easily inserted anywhere without impacting all the code.
///
/// Like `za::Texture` that can be used as a raw OpenGL texture,
/// `za::Shader` can also be used directly as a raw shader for
/// custom OpenGL geometry.
/// \code
/// shader.bind();
/// ... render OpenGL geometry ...
/// za::Shader::unbind();
/// \endcode
///
/// \see `za::Glsl`
///
////////////////////////////////////////////////////////////
