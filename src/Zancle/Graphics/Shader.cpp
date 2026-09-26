// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Shader.hpp"

#include "Zancle/GLUtils/GLCheck.hpp"
#include "Zancle/GLUtils/GLSharedContextGuard.hpp"
#include "Zancle/GLUtils/GLUtils.hpp"
#include "Zancle/GLUtils/Glad.hpp"

#include "Zancle/Graphics/DefaultShader.hpp"
#include "Zancle/Graphics/Glsl.hpp"
#include "Zancle/Graphics/GraphicsContext.hpp"
#include "Zancle/Graphics/ShaderUtils.hpp"
#include "Zancle/Graphics/Texture.hpp"

#include "Zancle/Err/Err.hpp"

#include "Zancle/IO/IO.hpp"
#include "Zancle/IO/InputStream.hpp"
#include "Zancle/IO/Path.hpp"
#include "Zancle/IO/PathUtils.hpp"

#include "Zancle/String/StringView.hpp"

#include "Zancle/Container/AnkerlUnorderedDense.hpp"
#include "Zancle/Container/Vector.hpp"

#include "Zancle/Vocabulary/Optional.hpp"
#include "Zancle/Vocabulary/PassKey.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/Exchange.hpp"
#include "Zancle/Base/Macros.hpp"
#include "Zancle/Base/Memcpy.hpp"
#include "Zancle/Base/SizeT.hpp"


using GLhandle = GLuint;


namespace
{
////////////////////////////////////////////////////////////
// Retrieve the maximum number of texture units available
[[nodiscard]] za::SizeT getMaxTextureUnits()
{
    static const auto maxUnits = static_cast<za::SizeT>(za::priv::getGLInteger(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS));
    return maxUnits;
}


////////////////////////////////////////////////////////////
// Pair of indices into thread-local buffer
struct [[nodiscard]] BufferSlice
{
    za::SizeT beginIdx;
    za::SizeT count;

    [[nodiscard]] explicit BufferSlice(za::SizeT b, za::SizeT c) : beginIdx(b), count(c)
    {
    }

    [[nodiscard]] za::StringView toView(const za::Vector<char>& buffer) const
    {
        return {buffer.data() + beginIdx, count};
    }
};


////////////////////////////////////////////////////////////
// Read the contents of a file and append them (followed by a null terminator)
// to `buffer`, returning the slice of `buffer` that holds the new contents.
// The returned slice EXCLUDES the trailing null terminator: its length flows
// into `glShaderSource`, and NUL is outside the GLSL source character set.
[[nodiscard]] za::Optional<BufferSlice> appendFileContentsToVector(const za::Path& filename, za::Vector<char>& buffer)
{
    const za::SizeT bufferSizeBeforeRead = buffer.size();

    if (!za::appendFromFile(filename, buffer))
        return za::nullOpt;

    buffer.pushBack('\0');
    return za::makeOptional<BufferSlice>(bufferSizeBeforeRead, buffer.size() - bufferSizeBeforeRead - 1u);
}


////////////////////////////////////////////////////////////
// Read the contents of a stream into an array of char
[[nodiscard]] za::Optional<BufferSlice> appendStreamContentsToVector(za::InputStream& stream, za::Vector<char>& buffer)
{
    const za::Optional<za::SizeT> size = stream.getSize();

    if (!size.hasValue() || size.value() == 0)
        return za::nullOpt;

    if (!stream.seek(0).hasValue())
        return za::nullOpt;

    const za::SizeT bufferSizeBeforeRead = buffer.size();
    buffer.reserve(bufferSizeBeforeRead + *size + 1u);
    buffer.unsafeSetSize(bufferSizeBeforeRead + *size);

    // `InputStream::read` may legally return fewer bytes than requested
    // ("up to `size`" contract) -- keep reading until all `*size` bytes are
    // accumulated. `nullOpt` (I/O error) or `0` (premature end) is a failure.
    za::SizeT totalRead = 0u;
    while (totalRead < *size)
    {
        const za::Optional<za::SizeT> read = stream.read(buffer.data() + bufferSizeBeforeRead + totalRead, *size - totalRead);

        if (!read.hasValue() || *read == 0u)
        {
            // Roll back the size grow so `buffer` is left as the caller saw it.
            buffer.unsafeSetSize(bufferSizeBeforeRead);
            return za::nullOpt;
        }

        totalRead += *read;
    }

    buffer.pushBack('\0');
    return za::makeOptional<BufferSlice>(bufferSizeBeforeRead, buffer.size() - bufferSizeBeforeRead - 1u);
}


////////////////////////////////////////////////////////////
// Return a thread-local vector used as the per-call concatenation buffer for
// the vertex/geometry/fragment shader sources. Disjoint from
// `za::getThreadLocalScratchCharBuffer` (which is the I/O staging area used
// inside `readFromFile` / `appendFromFile`); this one survives across the
// individual file reads and lives until `compile()` returns.
//
// Non-reentrant by the same contract as the I/O scratch.
[[nodiscard]] za::Vector<char>& getThreadLocalShaderConcatBuffer()
{
    static thread_local za::Vector<char> result;
    return result;
}


// Vec/Matrix types are standard-layout with no padding, so an array of N
// elements is exactly N * componentCount contiguous floats. This lets us
// pass the storage directly to glUniform*v without an intermediate copy.
static_assert(sizeof(za::Glsl::Vec2) == 2 * sizeof(float));
static_assert(sizeof(za::Glsl::Vec3) == 3 * sizeof(float));
static_assert(sizeof(za::Glsl::Vec4) == 4 * sizeof(float));
static_assert(sizeof(za::Glsl::Mat3) == 9 * sizeof(float));
static_assert(sizeof(za::Glsl::Mat4) == 16 * sizeof(float));


////////////////////////////////////////////////////////////
// Returns the static `#version`+precision preamble appropriate for the build.
//
// The trailing `#line 1` resets the GLSL compiler's line counter so that any
// error message the driver reports refers to the user's source line numbers,
// not preamble-relative ones. The preamble is constant per build, so callers
// pass it as a separate source string to `glShaderSource(count=2)` and avoid
// the per-compile concatenation.
//
// Note: `#line 1` assumes the common C-style driver behavior ("the NEXT line
// is line 1"). A spec-literal GLSL compiler ("behaves as if compiling at line
// `line`+1") would report user line N as N+1 -- accepted as a cosmetic
// off-by-one in diagnostics on such drivers.
[[nodiscard]] constexpr za::StringView getShaderPreamble()
{
    return
#if defined(ZA_SYSTEM_EMSCRIPTEN)
        // Emscripten/WebGL always requires `#version 300 es` and precision
        "#version 300 es\n\nprecision highp float;\n\n#line 1\n"
#elif defined(ZA_OPENGL_ES)
        // Desktop/mobile GLES can use `#version 310 es` and precision
        "#version 310 es\n\nprecision highp float;\n\n#line 1\n"
#else
        // Desktop GL can use `#version 430 core`
        "#version 430 core\n\n#line 1\n"
#endif
        ;
}


////////////////////////////////////////////////////////////
// Slow-path helper for compile-error reporting: concatenates the preamble and
// user source into a thread-local buffer so `printLinesWithNumbers` can show
// the full source as the driver saw it (preamble included).
[[nodiscard]] za::StringView buildConcatenatedShaderSource(za::StringView preamble, za::StringView src)
{
    static thread_local za::Vector<char> buffer; // Cannot reuse the other buffer here
    buffer.clear();

    buffer.emplaceRange(preamble.data(), preamble.size());
    buffer.emplaceRange(src.data(), src.size());

    return {buffer.data(), buffer.size()};
}


////////////////////////////////////////////////////////////
void printLinesWithNumbers(za::StringView text)
{
    za::priv::ErrMsgScope scope;
    scope.disableTrailing();
    scope.append("\n\n");

    constexpr za::StringView lineDirectivePrefix{"#line "};
    constexpr za::StringView beginIncludePrefix{"// >>> begin included from \""};
    constexpr za::StringView endIncludePrefix{"// <<< end included from \""};

    za::SizeT lineStart  = 0u;
    za::SizeT lineNumber = 1u;

    while (lineStart < text.size())
    {
        // Find the position of the next newline character.
        za::SizeT  newlinePos = text.find('\n', lineStart);
        const bool lastLine   = (newlinePos == za::StringView::nPos);

        const za::StringView line = lastLine ? text.substrByPosLen(lineStart)
                                             : text.substrByPosLen(lineStart, newlinePos - lineStart);

        // Check if this line is a #line directive and update the tracked line number
        if (line.size() > lineDirectivePrefix.size() &&
            line.substrByPosLen(0, lineDirectivePrefix.size()) == lineDirectivePrefix)
        {
            // Parse the line number from the directive
            unsigned int parsedLineNumber = 0;

            for (za::SizeT i = lineDirectivePrefix.size(); i < line.size(); ++i)
            {
                const char c = line[i];
                if (c < '0' || c > '9')
                    break;

                parsedLineNumber = parsedLineNumber * 10 + static_cast<unsigned int>(c - '0');
            }

            if (parsedLineNumber > 0)
                lineNumber = parsedLineNumber;
        }
        // Check for include begin/end markers -- print as separator lines
        else if ((line.size() > beginIncludePrefix.size() &&
                  line.substrByPosLen(0, beginIncludePrefix.size()) == beginIncludePrefix) ||
                 (line.size() > endIncludePrefix.size() &&
                  line.substrByPosLen(0, endIncludePrefix.size()) == endIncludePrefix))
        {
            scope.fmt("      | {}\n", line);
        }
        else
        {
            scope.fmt("{:>5} | {}\n", lineNumber, line);
            ++lineNumber;
        }

        if (lastLine)
            break;

        lineStart = newlinePos + 1;
    }
}


////////////////////////////////////////////////////////////
// Per-thread cache of the currently-bound shader program.
//
// All in-library `glUseProgram` calls go through `useProgram`, which keeps
// `currentProgramCacheValue` in sync with GL. `UniformBinder` reads from
// the cache instead of querying `GL_CURRENT_PROGRAM`.
//
// The cache is tagged with the GL context id that produced it. On read, if
// the active context id differs from the tag we fall back to a one-time query
// and re-tag.
//
// Stale-cache risks the tag does NOT cover:
//
// * Raw user `glUseProgram` -- caller must follow with `resetGLStates()`, same contract as every other Zancle state cache.
// * Program-handle reuse after deletion -- handled below by clearing the cache in `destroyProgramIfNeeded`.
//
thread_local unsigned int currentProgramCacheValue     = 0u;
thread_local unsigned int currentProgramCacheContextId = 0u;


////////////////////////////////////////////////////////////
void useProgram(const unsigned int program)
{
    glCheck(glUseProgram(program));

    currentProgramCacheValue     = program;
    currentProgramCacheContextId = za::GraphicsContext::getActiveThreadLocalGlContextId();
}


////////////////////////////////////////////////////////////
[[nodiscard]] unsigned int readCurrentProgramOrQuery()
{
    const unsigned int activeContextId = za::GraphicsContext::getActiveThreadLocalGlContextId();

    if (currentProgramCacheContextId == activeContextId)
        return currentProgramCacheValue;

    // Cache was tagged for a different context -- the value is meaningless
    // for the current one. Fall back to a one-shot query and re-tag.
    unsigned int current = 0u;
    glCheck(glGetIntegerv(GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>(&current)));

    currentProgramCacheValue     = current;
    currentProgramCacheContextId = activeContextId;

    return current;
}


////////////////////////////////////////////////////////////
void destroyProgramIfNeeded(const unsigned int program)
{
    if (!program)
        return;

    // Always delete programs and shaders on the shared context
    za::priv::GLSharedContextGuard guard;

    ZA_ASSERT(za::GraphicsContext::hasActiveThreadLocalGlContext());
    ZA_ASSERT(glCheck(glIsProgram(program)));
    glCheck(glDeleteProgram(program));

    // GL handles can be reused after deletion. If the cache still names this
    // handle, a future `UniformBinder{reusedId}` would compare against it,
    // conclude the (new, unrelated) program is already bound, and skip the
    // bind. Clear the cache to force a real bind on the next operation.
    if (currentProgramCacheValue == program)
        currentProgramCacheValue = 0u;
}


////////////////////////////////////////////////////////////
// `RenderTarget` uploads the built-in uniforms to hardcoded layout locations
// rather than by the queried location. A user shader declaring one of the
// built-in names at a different location would get an unrelated uniform silently
// corrupted by those uploads -- detect that here and warn.
//
// On Emscripten these queries also double as the lazy uniform-location-table
// warm-up workaround for `-sGL_EXPLICIT_UNIFORM_LOCATION=1` (see CLEAN-2 note
// on the constructor).
[[nodiscard]] bool checkBuiltInUniform(const unsigned int shaderProgram, const char* name, const GLint expectedLocation)
{
    const GLint location = glCheck(glGetUniformLocation(shaderProgram, name));

    if (location != -1 && location != expectedLocation) [[unlikely]]
        za::priv::errMsg(
            "Shader declares built-in uniform '{}' at location {} (expected layout location {}) -- built-in "
            "uniform uploads from the render target will write to the wrong location",
            name,
            location,
            expectedLocation);

    return location != -1;
}

} // namespace


namespace za
{
////////////////////////////////////////////////////////////
struct Shader::Impl
{
    unsigned int shaderProgram; //!< OpenGL identifier for the program

    // NOLINTNEXTLINE(cppcoreguidelines-use-default-member-init, modernize-use-default-member-init)
    int currentTexture; //!< Location of the current texture in the shader

    // Plain unsynchronized map: `za::Shader` is single-thread-affine. The program object is a
    // shared GL resource, but the map is CPU-side bookkeeping mutated by `setUniform(loc, Texture&)`
    // and read by `bindTextures()`. Sharing a Shader across threads would race on this container.
    mutable ankerl::unordered_dense::map<int, const Texture*> textures; //!< Texture variables in the shader, mapped to their location

    explicit Impl(unsigned int theShaderProgram) : shaderProgram{theShaderProgram}, currentTexture{-1}
    {
    }

    explicit Impl(Impl&& rhs) noexcept :
        shaderProgram(za::exchange(rhs.shaderProgram, 0u)),
        currentTexture(za::exchange(rhs.currentTexture, -1)),
        textures(ZA_MOVE(rhs.textures))
    {
    }

    Impl& operator=(Impl&& rhs) noexcept
    {
        if (&rhs == this)
            return *this;

        destroyProgramIfNeeded(shaderProgram);

        shaderProgram  = za::exchange(rhs.shaderProgram, 0u);
        currentTexture = za::exchange(rhs.currentTexture, -1);
        textures       = ZA_MOVE(rhs.textures);

        return *this;
    }
};


////////////////////////////////////////////////////////////
Shader::UniformLocation::UniformLocation(int location) : m_value(location)
{
    ZA_ASSERT(m_value != -1);
}


////////////////////////////////////////////////////////////
class [[nodiscard]] Shader::UniformBinder
{
public:
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] explicit UniformBinder(unsigned int currentProgramInt)
    {
        ZA_ASSERT(currentProgramInt != 0u);

        m_savedProgram = readCurrentProgramOrQuery();
        m_needsRestore = (currentProgramInt != m_savedProgram);

        if (m_needsRestore)
            useProgram(currentProgramInt);
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] ~UniformBinder()
    {
        if (m_needsRestore)
            useProgram(m_savedProgram);
    }


    ////////////////////////////////////////////////////////////
    UniformBinder(const UniformBinder&)            = delete;
    UniformBinder& operator=(const UniformBinder&) = delete;

private:
    GLhandle m_savedProgram{};
    bool     m_needsRestore{};
};


////////////////////////////////////////////////////////////
Shader::~Shader()
{
    destroyProgramIfNeeded(m_impl->shaderProgram);
}


////////////////////////////////////////////////////////////
Shader::Shader(Shader&& source) noexcept = default;


////////////////////////////////////////////////////////////
Shader& Shader::operator=(Shader&& rhs) noexcept = default;


////////////////////////////////////////////////////////////
za::Optional<Shader> Shader::loadFromFile(const LoadFromFileSettings& settings)
{
    // Prepare thread-local buffer
    za::Vector<char>& buffer = getThreadLocalShaderConcatBuffer();
    buffer.clear();

    // Helper function
    const auto readIntoBufferSlice = [&](const char* typeStr, const Path& optPath, za::Optional<BufferSlice>& optBufferSlice)
    {
        if (optPath.empty())
            return true;

        optBufferSlice = appendFileContentsToVector(optPath, buffer);
        if (!optBufferSlice.hasValue())
        {
            priv::errMsg("Failed to open {} shader file\n{}", typeStr, priv::PathDebugFormatter{optPath});
            return false;
        }

        return true;
    };

    // Read the vertex shader file (if path provided)
    za::Optional<BufferSlice> vertexShaderSlice;
    if (!readIntoBufferSlice("vertex", settings.vertexPath, vertexShaderSlice))
        return za::nullOpt;

    // Read the geometry shader file (if path provided)
    za::Optional<BufferSlice> geometryShaderSlice;
    if (!readIntoBufferSlice("geometry", settings.geometryPath, geometryShaderSlice))
        return za::nullOpt;

    // Read the fragment shader file (if path provided)
    za::Optional<BufferSlice> fragmentShaderSlice;
    if (!readIntoBufferSlice("fragment", settings.fragmentPath, fragmentShaderSlice))
        return za::nullOpt;

    // Get source views
    auto vertexView   = vertexShaderSlice.hasValue() ? vertexShaderSlice->toView(buffer) : za::StringView{};
    auto geometryView = geometryShaderSlice.hasValue() ? geometryShaderSlice->toView(buffer) : za::StringView{};
    auto fragmentView = fragmentShaderSlice.hasValue() ? fragmentShaderSlice->toView(buffer) : za::StringView{};

    // Preprocess #include directives if present
    za::Vector<char> ppVertexBuf;
    za::Vector<char> ppGeometryBuf;
    za::Vector<char> ppFragmentBuf;

    const auto preprocessIfNeeded = [](za::StringView& source, const Path& shaderPath, za::Vector<char>& ppBuf) -> bool
    {
        if (source.data() == nullptr || source.find("#include") == za::StringView::nPos)
            return true;

        if (!ShaderUtils::preprocessGlslIncludes(source, shaderPath, ppBuf))
            return false;

        source = {ppBuf.data(), ppBuf.size()};
        return true;
    };

    if (!preprocessIfNeeded(vertexView, settings.vertexPath, ppVertexBuf))
        return za::nullOpt;

    if (!preprocessIfNeeded(geometryView, settings.geometryPath, ppGeometryBuf))
        return za::nullOpt;

    if (!preprocessIfNeeded(fragmentView, settings.fragmentPath, ppFragmentBuf))
        return za::nullOpt;

    return compile(vertexView, geometryView, fragmentView);
}


////////////////////////////////////////////////////////////
za::Optional<Shader> Shader::loadFromMemory(const LoadFromMemorySettings& settings)
{
    return compile(settings.vertexCode, settings.geometryCode, settings.fragmentCode);
}


////////////////////////////////////////////////////////////
za::Optional<Shader> Shader::loadFromStream(const LoadFromStreamSettings& settings)
{
    // Prepare thread-local buffer
    za::Vector<char>& buffer = getThreadLocalShaderConcatBuffer();
    buffer.clear();

    // Helper function
    const auto readIntoBufferSlice =
        [&](const char* typeStr, InputStream* optStream, za::Optional<BufferSlice>& optBufferSlice)
    {
        if (optStream == nullptr)
            return true;

        optBufferSlice = appendStreamContentsToVector(*optStream, buffer);
        if (!optBufferSlice.hasValue())
        {
            priv::errMsg("Failed to read {} shader from stream (I/O error, empty stream, or unknown stream size)", typeStr);
            return false;
        }

        return true;
    };

    // Read the vertex shader code from the stream
    za::Optional<BufferSlice> vertexShaderSlice;
    if (!readIntoBufferSlice("vertex", settings.vertexStream, vertexShaderSlice))
        return za::nullOpt;

    // Read the geometry shader code from the stream
    za::Optional<BufferSlice> geometryShaderSlice;
    if (!readIntoBufferSlice("geometry", settings.geometryStream, geometryShaderSlice))
        return za::nullOpt;

    // Read the fragment shader code from the stream
    za::Optional<BufferSlice> fragmentShaderSlice;
    if (!readIntoBufferSlice("fragment", settings.fragmentStream, fragmentShaderSlice))
        return za::nullOpt;

    return compile(vertexShaderSlice.hasValue() ? vertexShaderSlice->toView(buffer) : za::StringView{},
                   geometryShaderSlice.hasValue() ? geometryShaderSlice->toView(buffer) : za::StringView{},
                   fragmentShaderSlice.hasValue() ? fragmentShaderSlice->toView(buffer) : za::StringView{});
}


////////////////////////////////////////////////////////////
za::Optional<Shader::UniformLocation> Shader::getUniformLocation(za::StringView uniformName) const
{
    enum : za::SizeT
    {
        maxUniformNameLength = 256
    };

    if (uniformName.size() >= maxUniformNameLength) [[unlikely]]
    {
        priv::errMsg("Uniform name too long ({} characters, maximum is {})", uniformName.size(), maxUniformNameLength - 1);
        return za::nullOpt;
    }

    // To get a a null-terminated string
    char uniformNameBuffer[maxUniformNameLength];
    ZA_MEMCPY(uniformNameBuffer, uniformName.data(), uniformName.size());
    uniformNameBuffer[uniformName.size()] = '\0';

    // Request the location from OpenGL
    const int location = glCheck(glGetUniformLocation(m_impl->shaderProgram, uniformNameBuffer));
    return location == -1 ? za::nullOpt : za::makeOptional(UniformLocation{location});
}


////////////////////////////////////////////////////////////
// Note that `glProgramUniform` is not supported on Emscripten.


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, float x) const
{
    ++m_uniformGeneration;
    const UniformBinder binder{m_impl->shaderProgram};
    glCheck(glUniform1f(location.m_value, x));
}


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, Glsl::Vec2 v) const
{
    ++m_uniformGeneration;
    const UniformBinder binder{m_impl->shaderProgram};
    glCheck(glUniform2f(location.m_value, v.x, v.y));
}


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, const Glsl::Vec3& v) const
{
    ++m_uniformGeneration;
    const UniformBinder binder{m_impl->shaderProgram};
    glCheck(glUniform3f(location.m_value, v.x, v.y, v.z));
}


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, const Glsl::Vec4& v) const
{
    ++m_uniformGeneration;
    const UniformBinder binder{m_impl->shaderProgram};
    glCheck(glUniform4f(location.m_value, v.x, v.y, v.z, v.w));
}


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, int x) const
{
    ++m_uniformGeneration;
    const UniformBinder binder{m_impl->shaderProgram};
    glCheck(glUniform1i(location.m_value, x));
}


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, Glsl::Ivec2 v) const
{
    ++m_uniformGeneration;
    const UniformBinder binder{m_impl->shaderProgram};
    glCheck(glUniform2i(location.m_value, v.x, v.y));
}


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, const Glsl::Ivec3& v) const
{
    ++m_uniformGeneration;
    const UniformBinder binder{m_impl->shaderProgram};
    glCheck(glUniform3i(location.m_value, v.x, v.y, v.z));
}


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, const Glsl::Ivec4& v) const
{
    ++m_uniformGeneration;
    const UniformBinder binder{m_impl->shaderProgram};
    glCheck(glUniform4i(location.m_value, v.x, v.y, v.z, v.w));
}


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, bool x) const
{
    setUniform(location, static_cast<int>(x));
}


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, Glsl::Bvec2 v) const
{
    setUniform(location, v.to<Glsl::Ivec2>());
}


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, const Glsl::Bvec3& v) const
{
    setUniform(location, v.to<Glsl::Ivec3>());
}


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, const Glsl::Bvec4& v) const
{
    setUniform(location, Glsl::Ivec4(v));
}


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, const Glsl::Mat3& matrix) const
{
    ++m_uniformGeneration;
    const UniformBinder binder{m_impl->shaderProgram};
    glCheck(glUniformMatrix3fv(location.m_value, 1, GL_FALSE, matrix.array));
}


////////////////////////////////////////////////////////////
void Shader::setMat4Uniform(UniformLocation location, const float* matrixPtr) const
{
    ++m_uniformGeneration;
    const UniformBinder binder{m_impl->shaderProgram};
    glCheck(glUniformMatrix4fv(location.m_value, 1, GL_FALSE, matrixPtr));
}


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, const Glsl::Mat4& matrix) const
{
    setMat4Uniform(location, matrix.array);
}


////////////////////////////////////////////////////////////
bool Shader::setUniform(UniformLocation location, const Texture& texture) const
{
    ++m_uniformGeneration;
    ++m_textureBindingsGeneration;

    ZA_ASSERT(m_impl->shaderProgram);
    ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());

    // Mutates `m_impl->textures`; must be called from the Shader's owning thread.
    // Store the location -> texture mapping
    if (auto* const it = m_impl->textures.find(location.m_value); it != m_impl->textures.end())
    {
        // Location already used, just replace the texture
        it->second = &texture;
        return true;
    }

    // New entry, make sure there are enough texture units
    if (m_impl->textures.size() + 1 >= getMaxTextureUnits())
    {
        priv::errMsg("Impossible to use texture at location {} for shader: all available texture units are used",
                     location.m_value);

        return false;
    }

    m_impl->textures[location.m_value] = &texture;
    return true;
}


////////////////////////////////////////////////////////////
void Shader::setUniform(UniformLocation location, CurrentTextureType)
{
    ++m_uniformGeneration;
    ++m_textureBindingsGeneration;

    ZA_ASSERT(m_impl->shaderProgram);
    ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());

    // Find the location of the variable in the shader
    m_impl->currentTexture = location.m_value;
}


////////////////////////////////////////////////////////////
void Shader::setUniformArray(UniformLocation location, const float* scalarArray, za::SizeT length)
{
    ++m_uniformGeneration;
    const UniformBinder binder{m_impl->shaderProgram};
    glCheck(glUniform1fv(location.m_value, static_cast<GLsizei>(length), scalarArray));
}


////////////////////////////////////////////////////////////
void Shader::setUniformArray(UniformLocation location, const Glsl::Vec2* vecArray, za::SizeT length)
{
    ++m_uniformGeneration;
    const UniformBinder binder{m_impl->shaderProgram};
    glCheck(glUniform2fv(location.m_value, static_cast<GLsizei>(length), reinterpret_cast<const float*>(vecArray)));
}


////////////////////////////////////////////////////////////
void Shader::setUniformArray(UniformLocation location, const Glsl::Vec3* vecArray, za::SizeT length)
{
    ++m_uniformGeneration;
    const UniformBinder binder{m_impl->shaderProgram};
    glCheck(glUniform3fv(location.m_value, static_cast<GLsizei>(length), reinterpret_cast<const float*>(vecArray)));
}


////////////////////////////////////////////////////////////
void Shader::setUniformArray(UniformLocation location, const Glsl::Vec4* vecArray, za::SizeT length)
{
    ++m_uniformGeneration;
    const UniformBinder binder{m_impl->shaderProgram};
    glCheck(glUniform4fv(location.m_value, static_cast<GLsizei>(length), reinterpret_cast<const float*>(vecArray)));
}


////////////////////////////////////////////////////////////
void Shader::setUniformArray(UniformLocation location, const Glsl::Mat3* matrixArray, za::SizeT length)
{
    ++m_uniformGeneration;
    const UniformBinder binder{m_impl->shaderProgram};
    glCheck(glUniformMatrix3fv(location.m_value,
                               static_cast<GLsizei>(length),
                               GL_FALSE,
                               reinterpret_cast<const float*>(matrixArray)));
}


////////////////////////////////////////////////////////////
void Shader::setUniformArray(UniformLocation location, const Glsl::Mat4* matrixArray, za::SizeT length)
{
    ++m_uniformGeneration;
    const UniformBinder binder{m_impl->shaderProgram};
    glCheck(glUniformMatrix4fv(location.m_value,
                               static_cast<GLsizei>(length),
                               GL_FALSE,
                               reinterpret_cast<const float*>(matrixArray)));
}


////////////////////////////////////////////////////////////
unsigned int Shader::getNativeHandle() const
{
    return m_impl->shaderProgram;
}


////////////////////////////////////////////////////////////
void Shader::bind() const
{
    ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());
    ZA_ASSERT(m_impl->shaderProgram != 0u);

    // Enable the program
    ZA_ASSERT(glCheck(glIsProgram(m_impl->shaderProgram)));
    useProgram(m_impl->shaderProgram);

    // Bind the textures (including the special `CurrentTexture` sampler)
    bindTextures();
}


////////////////////////////////////////////////////////////
void Shader::unbind()
{
    ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());
    useProgram(0u);
}


////////////////////////////////////////////////////////////
bool Shader::isGeometryAvailable()
{
#ifdef ZA_OPENGL_ES
    return false;
#else
    ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());
    // `GLAD_GL_VERSION_3_2` is the runtime flag set by glad after `gladLoadGL`.
    // (Not `GL_VERSION_3_2`, which is a `#define` that always expands to 1.)
    return GLAD_GL_VERSION_3_2 != 0;
#endif
}


////////////////////////////////////////////////////////////
// Note: the `glGetUniformLocation` calls below also serve as the workaround
// for an Emscripten bug with `-sGL_EXPLICIT_UNIFORM_LOCATION=1`: Emscripten
// lazily populates its internal uniform location table (`uniformLocsById`)
// only on `glGetUniformLocation`, NOT on `glUniform*` -- without at least one
// query here, `glUniform*` on a newly linked program would silently no-op.
// See: https://github.com/emscripten-core/emscripten/issues/26672
Shader::Shader(za::PassKey<Shader>&&, unsigned int shaderProgram) :
    m_impl(
        [&]
{
    ZA_ASSERT(shaderProgram != 0);
    return shaderProgram;
}()),
    m_hasBuiltInUniformMVPRow0(checkBuiltInUniform(shaderProgram, "za_u_mvpRow0", 0)),
    m_hasBuiltInUniformMVPRow1(checkBuiltInUniform(shaderProgram, "za_u_mvpRow1", 1)),
    m_hasBuiltInUniformInvTextureSize(checkBuiltInUniform(shaderProgram, "za_u_invTextureSize", 3))
{
}


////////////////////////////////////////////////////////////
za::Optional<Shader> Shader::compile(za::StringView vertexShaderCode,
                                     za::StringView geometryShaderCode,
                                     za::StringView fragmentShaderCode)
{
    ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());

    // Make sure we can use geometry shaders
    if (!geometryShaderCode.empty() && !isGeometryAvailable())
    {
        priv::errMsg(
            "Failed to create a shader: your system doesn't support geometry shaders (you should test "
            "Shader::isGeometryAvailable() before trying to use geometry shaders)");

        return za::nullOpt;
    }

    // Always create programs and shaders on the shared context
    priv::GLSharedContextGuard guard;

    // Create the program
    const GLhandle shaderProgram = glCheck(glCreateProgram());
    ZA_ASSERT(glCheck(glIsProgram(shaderProgram)));

    const auto makeShader = [&](GLenum type, const char* typeStr, za::StringView shaderCode)
    {
        // Pass `#version` (+ precision + `#line 1`) and the user source as two
        // separate source strings -- the preamble is build-time constant, no
        // per-compile concatenation needed.
        constexpr za::StringView preamble = getShaderPreamble();

        const GLhandle shader = glCheck(glCreateShader(type));

        const GLchar* sources[2]{preamble.data(), shaderCode.data()};
        const GLint   lengths[2]{static_cast<GLint>(preamble.size()), static_cast<GLint>(shaderCode.size())};

        glCheck(glShaderSource(shader, 2, sources, lengths));
        glCheck(glCompileShader(shader));
        ZA_ASSERT(glCheck(glIsShader(shader)));

        // Check the compile log
        GLint success = 0;
        glCheck(glGetShaderiv(shader, GL_COMPILE_STATUS, &success));
        if (success == GL_FALSE)
        {
            char log[1024]{};
            glCheck(glGetShaderInfoLog(shader, sizeof(log), nullptr, log));

            priv::errMsg("Failed to compile {} shader:{}{}\n\nSource code:\n", typeStr, '\n', static_cast<const char*>(log));

            // Build the concatenated source on the slow (error) path so the
            // numbered listing shows what the GLSL compiler actually saw.
            printLinesWithNumbers(buildConcatenatedShaderSource(preamble, shaderCode));

            glCheck(glDeleteShader(shader));
            glCheck(glDeleteProgram(shaderProgram));

            return false;
        }

        // Attach the shader to the program, and delete it (not needed anymore)
        glCheck(glAttachShader(shaderProgram, shader));
        glCheck(glDeleteShader(shader));

        return true;
    };

    if (vertexShaderCode.empty())
        vertexShaderCode = DefaultShader::srcVertex;

    if (!makeShader(GL_VERTEX_SHADER, "vertex", vertexShaderCode))
        return za::nullOpt;


    // Create the geometry shader if needed
    if (!geometryShaderCode.empty())
    {
        if (!makeShader(GL_GEOMETRY_SHADER, "geometry", geometryShaderCode))
            return za::nullOpt;
    }

    if (fragmentShaderCode.empty())
        fragmentShaderCode = DefaultShader::srcFragment;

    // Create the fragment shader
    if (!makeShader(GL_FRAGMENT_SHADER, "fragment", fragmentShaderCode))
        return za::nullOpt;

    // Link the program
    glCheck(glLinkProgram(shaderProgram));

    // Check the link log
    GLint success = 0;
    glCheck(glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success));
    if (success == GL_FALSE)
    {
        char log[1024]{};
        glCheck(glGetProgramInfoLog(shaderProgram, sizeof(log), nullptr, log));

        priv::errMsg("Failed to link shader:{}{}VERTEX SOURCE:\n{}\n\nFRAGMENT SOURCE:\n{}\n\nGEOMETRY SOURCE:\n{}",
                     '\n',
                     static_cast<const char*>(log),
                     vertexShaderCode,
                     fragmentShaderCode,
                     geometryShaderCode);

        glCheck(glDeleteProgram(shaderProgram));
        return za::nullOpt;
    }

    // Force an OpenGL flush, so that the shader will appear updated
    // in all contexts immediately (solves problems in multi-threaded apps)
    glCheck(glFlush());

    return za::makeOptional<Shader>(za::PassKey<Shader>{}, shaderProgram);
}


////////////////////////////////////////////////////////////
// Reads `m_impl->textures`; must be called from the Shader's owning thread.
void Shader::bindTextures() const
{
    auto* it = m_impl->textures.begin();

    for (za::SizeT i = 0u; i < m_impl->textures.size(); ++i)
    {
        const auto index = static_cast<GLsizei>(i + 1);

        glCheck(glUniform1i(it->first, index));
        glCheck(glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(index)));

        it->second->bind();
        ++it;
    }

    // Make sure that the texture unit which is left active is the number 0
    glCheck(glActiveTexture(GL_TEXTURE0));

    // Bind the special `CurrentTexture` sampler to texture unit 0, if set
    if (m_impl->currentTexture != -1)
        glCheck(glUniform1i(m_impl->currentTexture, 0));
}

} // namespace za
