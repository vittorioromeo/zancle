// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Texture.hpp"

#include "Zancle/GLUtils/BlitFramebuffer.hpp"
#include "Zancle/GLUtils/FramebufferSaver.hpp"
#include "Zancle/GLUtils/GLCheck.hpp"
#include "Zancle/GLUtils/GLSharedContextGuard.hpp"
#include "Zancle/GLUtils/GLUtils.hpp"
#include "Zancle/GLUtils/Glad.hpp"
#include "Zancle/GLUtils/TextureSaver.hpp"

#include "Zancle/Graphics/Color.hpp"
#include "Zancle/Graphics/GraphicsContext.hpp"
#include "Zancle/Graphics/Image.hpp"
#include "Zancle/Graphics/TextureWrapMode.hpp"

#include "Zancle/Window/Window.hpp"
#include "Zancle/Window/WindowContext.hpp"

#include "Zancle/Err/Err.hpp"

#include "Zancle/IO/Path.hpp"

#include "Zancle/Concurrency/Atomic.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Vocabulary/Optional.hpp"
#include "Zancle/Vocabulary/PassKey.hpp"

#include "Zancle/Math/MinMax.hpp"

#include "Zancle/Base/Abort.hpp"
#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/Exchange.hpp"
#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/Macros.hpp"
#include "Zancle/Base/SizeT.hpp"
#include "Zancle/Base/Swap.hpp"


namespace
{
// A nested named namespace is used here to allow unity builds of Zancle.
namespace TextureImpl
{
////////////////////////////////////////////////////////////
// Thread-safe unique identifier generator, is used for states cache (see RenderTarget)
constinit za::Atomic<unsigned int> nextUniqueId{1u}; // start at 1, zero is "no texture"

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten]] inline unsigned int getUniqueId() noexcept
{
    return nextUniqueId.fetchAddRelaxed(1u);
}

////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] inline constexpr GLint wrapModeToGl(za::TextureWrapMode wrapMode) noexcept
{
    return wrapMode == za::TextureWrapMode::Clamp ? GL_CLAMP_TO_EDGE
           : wrapMode == za::TextureWrapMode::Repeat
               ? GL_REPEAT
               : GL_MIRRORED_REPEAT;
}

} // namespace TextureImpl
} // namespace


namespace za
{
////////////////////////////////////////////////////////////
Texture::Texture(za::PassKey<Texture>&&, Vec2u size, unsigned int texture, bool sRgb) :
    m_size(size),
    m_texture(texture),
    m_sRgb(sRgb),
    m_cacheId(TextureImpl::getUniqueId())
{
}


////////////////////////////////////////////////////////////
Texture::Texture(const Texture& rhs) : m_cacheId(0u) // every member is overwritten by the move-assignment below
{
    za::Optional texture = create(rhs.getSize(),
                                  {
                                      .sRgb     = rhs.isSrgb(),
                                      .smooth   = rhs.isSmooth(),
                                      .wrapMode = rhs.getWrapMode(),
                                  });

    if (!texture.hasValue())
    {
        priv::errMsg("Failed to copy texture, failed to create new texture");
        za::abort();
    }

    *this = ZA_MOVE(*texture);

    if (!update(rhs))
    {
        priv::errMsg("Failed to copy texture, failed to update from new texture");
        za::abort();
    }
}


////////////////////////////////////////////////////////////
Texture::~Texture()
{
    destroyGlTexture();
}


////////////////////////////////////////////////////////////
void Texture::destroyGlTexture()
{
    // Destroy the OpenGL texture
    if (!m_texture)
        return;

    ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());

    {
        // Always destroy the texture on the shared context
        priv::GLSharedContextGuard guard;

        const GLuint texture = m_texture;
        glCheck(glDeleteTextures(1, &texture));
    }

    if (priv::getGLInteger(GL_TEXTURE_BINDING_2D) == static_cast<GLint>(m_texture))
    {
        // Unbind the texture if it was bound
        glCheck(glBindTexture(GL_TEXTURE_2D, 0u));
    }
}


////////////////////////////////////////////////////////////
Texture::Texture(Texture&& rhs) noexcept :
    m_size(za::exchange(rhs.m_size, {})),
    m_texture(za::exchange(rhs.m_texture, 0u)),
    m_isSmooth(za::exchange(rhs.m_isSmooth, false)),
    m_sRgb(za::exchange(rhs.m_sRgb, false)),
    m_wrapMode(za::exchange(rhs.m_wrapMode, TextureWrapMode::Clamp)),
    m_fboAttachment(za::exchange(rhs.m_fboAttachment, false)),
    m_hasMipmap(za::exchange(rhs.m_hasMipmap, false)),
    m_cacheId(za::exchange(rhs.m_cacheId, 0u))
{
}


////////////////////////////////////////////////////////////
Texture& Texture::operator=(Texture&& rhs) noexcept
{
    if (&rhs == this)
        return *this;

    // Destroy the OpenGL texture
    destroyGlTexture();

    // Move old to new.
    m_size          = za::exchange(rhs.m_size, {});
    m_texture       = za::exchange(rhs.m_texture, 0u);
    m_isSmooth      = za::exchange(rhs.m_isSmooth, false);
    m_sRgb          = za::exchange(rhs.m_sRgb, false);
    m_wrapMode      = za::exchange(rhs.m_wrapMode, TextureWrapMode::Clamp);
    m_fboAttachment = za::exchange(rhs.m_fboAttachment, false);
    m_hasMipmap     = za::exchange(rhs.m_hasMipmap, false);
    m_cacheId       = za::exchange(rhs.m_cacheId, 0u);

    // Both textures' state was just replaced wholesale (`rhs` is now empty);
    // bump on each so that any in-flight batched draw referencing either one
    // is detected and flushed with an actionable error (mirrors `swap`).
    ++m_destructiveGeneration;
    ++rhs.m_destructiveGeneration;

    return *this;
}


////////////////////////////////////////////////////////////
za::Optional<Texture> Texture::create(Vec2u size, const TextureCreateSettings& settings)
{
    za::Optional<Texture> result; // Use a single local variable for NRVO

    // Check if texture parameters are valid before creating it
    if ((size.x == 0) || (size.y == 0))
    {
        priv::errMsg("Failed to create texture, invalid size ({}x{})", size.x, size.y);
        return result; // Empty optional
    }

    ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());

    // Check the maximum texture size
    const unsigned int maxSize = getMaximumSize();
    if ((size.x > maxSize) || (size.y > maxSize))
    {
        priv::errMsg("Failed to create texture, its internal size is too high ({}x{}, maximum is {}x{})",
                     size.x,
                     size.y,
                     maxSize,
                     maxSize);

        return result; // Empty optional
    }

    // Create the OpenGL texture
    GLuint glTexture = 0u;

    {
        // Always create textures on the shared context
        priv::GLSharedContextGuard guard;

        glCheck(glGenTextures(1, &glTexture));
        ZA_ASSERT(glTexture);
    }

    // All the validity checks passed, we can store the new texture settings
    result.emplace(za::PassKey<Texture>{}, size, glTexture, settings.sRgb);

    // Make sure that the current texture binding will be preserved
    const priv::TextureSaver save;

    priv::bindAndInitializeTexture(result->m_texture,
                                   result->m_sRgb,
                                   size,
                                   static_cast<unsigned int>(TextureImpl::wrapModeToGl(settings.wrapMode)));

    result->setSmooth(settings.smooth);
    result->setWrapMode(settings.wrapMode);

    return result;
}


////////////////////////////////////////////////////////////
za::Optional<Texture> Texture::loadFromFile(const Path& filename, const TextureLoadSettings& settings)
{
    if (const za::Optional image = za::Image::loadFromFile(filename))
        return loadFromImage(*image, settings);

    priv::errMsg("Failed to load texture from file");
    return za::nullOpt;
}


////////////////////////////////////////////////////////////
za::Optional<Texture> Texture::loadFromMemory(const void* data, za::SizeT size, const TextureLoadSettings& settings)
{
    if (const za::Optional image = za::Image::loadFromMemory(data, size))
        return loadFromImage(*image, settings);

    priv::errMsg("Failed to load texture from memory");
    return za::nullOpt;
}


////////////////////////////////////////////////////////////
za::Optional<Texture> Texture::loadFromStream(InputStream& stream, const TextureLoadSettings& settings)
{
    if (const za::Optional image = za::Image::loadFromStream(stream))
        return loadFromImage(*image, settings);

    priv::errMsg("Failed to load texture from stream");
    return za::nullOpt;
}


////////////////////////////////////////////////////////////
za::Optional<Texture> Texture::loadFromImage(const Image& image, const TextureLoadSettings& settings)
{
    za::Optional<Texture> result; // Use a single local variable for NRVO

    // Retrieve the image size
    const auto size = image.getSize().toVec2i();

    // Load the entire image if the source area is either empty or contains the whole image
    if (settings.area.size.x == 0 || (settings.area.size.y == 0) ||
        ((settings.area.position.x <= 0) && (settings.area.position.y <= 0) && (settings.area.size.x >= size.x) &&
         (settings.area.size.y >= size.y)))
    {
        // Load the entire image
        if ((result = za::Texture::create(image.getSize(),
                                          {.sRgb = settings.sRgb, .smooth = settings.smooth, .wrapMode = settings.wrapMode})))
        {
            result->update(image);
            return result;
        }

        // Error message generated in called function.
        return result; // Empty optional
    }

    // Load a sub-area of the image
    ZA_ASSERT(settings.area.size.x > 0);
    ZA_ASSERT(settings.area.size.y > 0);
    ZA_ASSERT(settings.area.position.x < size.x);
    ZA_ASSERT(settings.area.position.y < size.y);

    // Adjust the rectangle to the size of the image
    Rect2i rectangle     = settings.area;
    rectangle.position.x = za::max(rectangle.position.x, 0);
    rectangle.position.y = za::max(rectangle.position.y, 0);
    rectangle.size.x     = za::min(rectangle.size.x, size.x - rectangle.position.x);
    rectangle.size.y     = za::min(rectangle.size.y, size.y - rectangle.position.y);

    // Create the texture and upload the pixels
    if ((result = za::Texture::create(rectangle.size.toVec2u(),
                                      {.sRgb = settings.sRgb, .smooth = settings.smooth, .wrapMode = settings.wrapMode})))
    {
        ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());

        // Make sure that the current texture binding will be preserved
        const priv::TextureSaver save;

        // Copy the pixels to the texture
        const za::U8* pixels = image.getPixelsPtr() +
                               4u * (static_cast<za::SizeT>(rectangle.position.x) +
                                     static_cast<za::SizeT>(size.x) * static_cast<za::SizeT>(rectangle.position.y));

        glCheck(glBindTexture(GL_TEXTURE_2D, result->m_texture));

        glCheck(glPixelStorei(GL_UNPACK_ROW_LENGTH, size.x)); // restore after
        glCheck(
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, rectangle.size.x, rectangle.size.y, GL_RGBA, GL_UNSIGNED_BYTE, pixels));
        glCheck(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0));

        glCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, result->m_isSmooth ? GL_LINEAR : GL_NEAREST));
        result->m_hasMipmap = false;

        // Force an OpenGL flush, so that the texture will appear updated
        // in all contexts immediately (solves problems in multi-threaded apps)
        glCheck(glFlush());
    }

    // Error message generated in called function.
    return result;
}


////////////////////////////////////////////////////////////
Vec2u Texture::getSize() const
{
    return m_size;
}


////////////////////////////////////////////////////////////
bool Texture::clear()
{
    return clear(Color::Transparent);
}


////////////////////////////////////////////////////////////
bool Texture::clear(const Color color)
{
    ZA_ASSERT(m_texture);
    ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());

    // Clear on the GPU by attaching the texture to the scratch framebuffer
    // (cheaper than uploading a same-size pixel buffer)
    const auto frameBuffer = static_cast<GLuint>(WindowContext::getTransferScratchDrawFramebuffer());

    if (frameBuffer == 0u)
    {
        priv::errMsg("Failed to clear texture, could not get scratch framebuffer");
        return false;
    }

    {
        const priv::FramebufferSaver    framebufferSaver;
        const priv::ScissorDisableGuard scissorDisableGuard; // `glClearBufferfv` honors the scissor test

        glCheck(glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer));
        glCheck(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0));

        const float clearColor[4]{color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f};
        glCheck(glClearBufferfv(GL_COLOR, 0, clearColor));

        // Detach so the scratch framebuffer doesn't keep referencing the texture
        glCheck(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0u, 0));
    }

    // Make sure that the current texture binding will be preserved
    const priv::TextureSaver save;

    glCheck(glBindTexture(GL_TEXTURE_2D, m_texture));
    glCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_isSmooth ? GL_LINEAR : GL_NEAREST));
    m_hasMipmap = false;
    m_cacheId   = TextureImpl::getUniqueId();

    // Force an OpenGL flush, so that the texture data will appear updated
    // in all contexts immediately (solves problems in multi-threaded apps)
    glCheck(glFlush());

    // Full-texture overwrite: every UV samples new content, so invalidate
    // in-flight batched draws (see `update`)
    ++m_destructiveGeneration;

    return true;
}


////////////////////////////////////////////////////////////
Image Texture::copyToImage() const
{
    // Easy case: empty texture
    ZA_ASSERT(m_texture && "Texture::copyToImage Cannot copy empty texture to image");

    ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());

    // Make sure that the current texture binding will be preserved
    const priv::TextureSaver save;

    // Create an array of pixels
    za::Vector<za::U8> pixels(za::SizeT{m_size.x} * za::SizeT{m_size.y} * 4);

    // OpenGL ES doesn't have the glGetTexImage function, the only way to read
    // from a texture is to bind it to a FBO and use glReadPixels
    const auto frameBuffer = static_cast<GLuint>(WindowContext::getTransferScratchReadFramebuffer());
    if (frameBuffer != 0u)
    {
        const priv::FramebufferSaver framebufferSaver;

        glCheck(glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer));
        glCheck(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0));
        glCheck(glReadPixels(0,
                             0,
                             static_cast<GLsizei>(m_size.x),
                             static_cast<GLsizei>(m_size.y),
                             GL_RGBA,
                             GL_UNSIGNED_BYTE,
                             pixels.data()));
    }
    else
    {
        priv::errMsg("Failed to copy texture to image, failed to create frame buffer object");
        za::abort();
    }

    auto result = za::Image::create(m_size, pixels.data());
    ZA_ASSERT(result.hasValue());
    return ZA_MOVE(*result);
}


////////////////////////////////////////////////////////////
void Texture::update(const za::U8* pixels)
{
    // Update the whole texture (the sub-rect overload detects the full-size
    // update and bumps `m_destructiveGeneration`)
    update(pixels, m_size, {0, 0});
}


////////////////////////////////////////////////////////////
void Texture::update(const za::U8* pixels, Vec2u size, Vec2u dest)
{
    ZA_ASSERT(dest.x + size.x <= m_size.x && "Destination x coordinate is outside of texture");
    ZA_ASSERT(dest.y + size.y <= m_size.y && "Destination y coordinate is outside of texture");

    ZA_ASSERT(pixels != nullptr);

    ZA_ASSERT(m_texture);
    ZA_ASSERT(glCheck(glIsTexture(m_texture)));

    ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());

    // Make sure that the current texture binding will be preserved
    const priv::TextureSaver save;

    // Copy pixels from the given array to the texture
    glCheck(glBindTexture(GL_TEXTURE_2D, m_texture));
    glCheck(glTexSubImage2D(GL_TEXTURE_2D,
                            0,
                            static_cast<GLint>(dest.x),
                            static_cast<GLint>(dest.y),
                            static_cast<GLsizei>(size.x),
                            static_cast<GLsizei>(size.y),
                            GL_RGBA,
                            GL_UNSIGNED_BYTE,
                            pixels));
    glCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_isSmooth ? GL_LINEAR : GL_NEAREST));
    m_hasMipmap = false;
    m_cacheId   = TextureImpl::getUniqueId();

    // Force an OpenGL flush, so that the texture data will appear updated
    // in all contexts immediately (solves problems in multi-threaded apps)
    glCheck(glFlush());

    // Intentionally not bumping `m_destructiveGeneration` here.
}


////////////////////////////////////////////////////////////
bool Texture::update(const Texture& texture, Vec2u dest)
{
    ZA_ASSERT(dest.x + texture.m_size.x <= m_size.x && "Destination x coordinate is outside of texture");
    ZA_ASSERT(dest.y + texture.m_size.y <= m_size.y && "Destination y coordinate is outside of texture");

    ZA_ASSERT(&texture != this && "Cannot update a texture from itself (GL framebuffer feedback loop)");

    ZA_ASSERT(m_texture);
    ZA_ASSERT(glCheck(glIsTexture(m_texture)));

    ZA_ASSERT(texture.m_texture);
    ZA_ASSERT(glCheck(glIsTexture(texture.m_texture)));

    ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());

    const auto sourceFrameBuffer = static_cast<GLuint>(WindowContext::getTransferScratchReadFramebuffer());
    if (sourceFrameBuffer == 0u)
    {
        priv::errMsg("Cannot copy texture, failed to acquire source frame buffer object");
        return false;
    }

    const auto destFrameBuffer = static_cast<GLuint>(WindowContext::getTransferScratchDrawFramebuffer());
    if (destFrameBuffer == 0u)
    {
        priv::errMsg("Cannot copy texture, failed to acquire destination frame buffer object");
        return false;
    }

    bool success = true;

    {
        // Save the current bindings so we can restore them after we are done
        const priv::FramebufferSaver framebufferSaver;

        // Link the source texture to the source frame buffer
        glCheck(glBindFramebuffer(GL_READ_FRAMEBUFFER, sourceFrameBuffer));
        glCheck(glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.m_texture, 0));

        // Link the destination texture to the destination frame buffer
        glCheck(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, destFrameBuffer));
        glCheck(glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0));

        // A final check, just to be sure...
        const GLenum sourceStatus = glCheck(glCheckFramebufferStatus(GL_READ_FRAMEBUFFER));
        const GLenum destStatus   = glCheck(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER));

        if ((sourceStatus == GL_FRAMEBUFFER_COMPLETE) && (destStatus == GL_FRAMEBUFFER_COMPLETE))
        {
            // Scissor testing affects framebuffer blits as well
            // Since we don't want scissor testing to interfere with our copying, we temporarily disable it for the blit if it is enabled
            const priv::ScissorDisableGuard scissorDisableGuard;

            // Blit the texture contents from the source to the destination texture
            priv::blitFramebuffer(/* invertYAxis */ false, texture.m_size, {0u, 0u}, dest);
        }
        else
        {
            priv::errMsg("Cannot copy texture, failed to link texture to frame buffer");
            success = false;
        }
    }

    // Make sure that the current texture binding will be preserved
    const priv::TextureSaver save;

    // Set the parameters of this texture
    glCheck(glBindTexture(GL_TEXTURE_2D, m_texture));
    glCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_isSmooth ? GL_LINEAR : GL_NEAREST));
    m_hasMipmap = false;
    m_cacheId   = TextureImpl::getUniqueId();

    // Force an OpenGL flush, so that the texture data will appear updated
    // in all contexts immediately (solves problems in multi-threaded apps)
    glCheck(glFlush());
    return success;
}


////////////////////////////////////////////////////////////
void Texture::update(const Image& image, Vec2u dest)
{
    update(image.getPixelsPtr(), image.getSize(), dest);
}


////////////////////////////////////////////////////////////
bool Texture::update(const Window& window, Vec2u dest)
{
    ZA_ASSERT(dest.x + window.getSize().x <= m_size.x && "Destination x coordinate is outside of texture");
    ZA_ASSERT(dest.y + window.getSize().y <= m_size.y && "Destination y coordinate is outside of texture");

    ZA_ASSERT(m_texture);
    ZA_ASSERT(glCheck(glIsTexture(m_texture)));

    if (!window.setActive(true))
    {
        priv::errMsg("Failed to activate window in `Texture::update`");
        return false;
    }

    ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());

    const auto destFrameBuffer = static_cast<GLuint>(WindowContext::getTransferScratchDrawFramebuffer());
    if (destFrameBuffer == 0u)
    {
        priv::errMsg("Cannot copy texture, failed to acquire a frame buffer object");
        return false;
    }

    bool success = true;

    {
        // Save the current bindings so we can restore them after we are done
        const priv::FramebufferSaver framebufferSaver;

        // Link the source texture to the source frame buffer
        glCheck(glBindFramebuffer(GL_READ_FRAMEBUFFER, 0u /* default FBO */));

        // Link the destination texture to the destination frame buffer
        glCheck(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, destFrameBuffer));
        glCheck(glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0));

        // A final check, just to be sure...
        const GLenum sourceStatus = glCheck(glCheckFramebufferStatus(GL_READ_FRAMEBUFFER));
        const GLenum destStatus   = glCheck(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER));

        if ((sourceStatus == GL_FRAMEBUFFER_COMPLETE) && (destStatus == GL_FRAMEBUFFER_COMPLETE))
        {
            // Scissor testing affects framebuffer blits as well
            // Since we don't want scissor testing to interfere with our copying, we temporarily disable it for the blit if it is enabled
            const priv::ScissorDisableGuard scissorDisableGuard;

            if (!WindowContext::copyFlippedFramebuffer(m_sRgb, window.getSize(), 0u /* default FBO */, destFrameBuffer, {0u, 0u}, dest))
            {
                priv::errMsg("Cannot copy texture, failed to copy flipped framebuffer");
                success = false;
            }
        }
        else
        {
            priv::errMsg("Cannot copy texture, failed to link texture to frame buffer");
            success = false;
        }
    }

    // Make sure that the current texture binding will be preserved
    const priv::TextureSaver save;

    // Set the parameters of this texture
    glCheck(glBindTexture(GL_TEXTURE_2D, m_texture));
    glCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_isSmooth ? GL_LINEAR : GL_NEAREST));
    m_hasMipmap = false;
    m_cacheId   = TextureImpl::getUniqueId();

    // Force an OpenGL flush, so that the texture will appear updated
    // in all contexts immediately (solves problems in multi-threaded apps)
    glCheck(glFlush());

    return success;
}


////////////////////////////////////////////////////////////
void Texture::setSmooth(bool smooth)
{
    ZA_ASSERT(m_texture);

    if (smooth == m_isSmooth)
        return;

    m_isSmooth = smooth;
    ++m_destructiveGeneration; // sampler state change visible to any in-flight batched draw

    ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());

    // Make sure that the current texture binding will be preserved
    const priv::TextureSaver save;

    glCheck(glBindTexture(GL_TEXTURE_2D, m_texture));
    glCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, m_isSmooth ? GL_LINEAR : GL_NEAREST));

    if (m_hasMipmap)
    {
        glCheck(glTexParameteri(GL_TEXTURE_2D,
                                GL_TEXTURE_MIN_FILTER,
                                m_isSmooth ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR));
    }
    else
    {
        glCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_isSmooth ? GL_LINEAR : GL_NEAREST));
    }
}


////////////////////////////////////////////////////////////
bool Texture::isSmooth() const
{
    return m_isSmooth;
}


////////////////////////////////////////////////////////////
bool Texture::isSrgb() const
{
    return m_sRgb;
}


////////////////////////////////////////////////////////////
void Texture::setWrapMode(TextureWrapMode wrapMode)
{
    ZA_ASSERT(m_texture);

    if (wrapMode == m_wrapMode)
        return;

    m_wrapMode = wrapMode;
    ++m_destructiveGeneration; // sampler state change visible to any in-flight batched draw

    ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());

    // Make sure that the current texture binding will be preserved
    const priv::TextureSaver save;

    glCheck(glBindTexture(GL_TEXTURE_2D, m_texture));
    glCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, TextureImpl::wrapModeToGl(wrapMode)));
    glCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, TextureImpl::wrapModeToGl(wrapMode)));
}


////////////////////////////////////////////////////////////
TextureWrapMode Texture::getWrapMode() const
{
    return m_wrapMode;
}


////////////////////////////////////////////////////////////
void Texture::generateMipmap()
{
    ZA_ASSERT(m_texture);
    ZA_ASSERT(glCheck(glIsTexture(m_texture)));

    ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());

    // Make sure that the current texture binding will be preserved
    const priv::TextureSaver save;

    glCheck(glBindTexture(GL_TEXTURE_2D, m_texture));
    glCheck(glGenerateMipmap(GL_TEXTURE_2D));
    glCheck(glTexParameteri(GL_TEXTURE_2D,
                            GL_TEXTURE_MIN_FILTER,
                            m_isSmooth ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR));

    m_hasMipmap = true;
    ++m_destructiveGeneration; // MIN_FILTER changed; in-flight batched draws would sample with the old filter
}


////////////////////////////////////////////////////////////
void Texture::invalidateMipmap()
{
    if (!m_hasMipmap)
        return;

    ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());

    // Make sure that the current texture binding will be preserved
    const priv::TextureSaver save;

    glCheck(glBindTexture(GL_TEXTURE_2D, m_texture));
    glCheck(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, m_isSmooth ? GL_LINEAR : GL_NEAREST));

    m_hasMipmap = false;
    ++m_destructiveGeneration; // MIN_FILTER changed; in-flight batched draws would sample with the old filter
}


////////////////////////////////////////////////////////////
void Texture::bind() const
{
    ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());
    ZA_ASSERT(m_texture);

    glCheck(glBindTexture(GL_TEXTURE_2D, m_texture));
}


////////////////////////////////////////////////////////////
void Texture::unbind()
{
    ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());
    glCheck(glBindTexture(GL_TEXTURE_2D, 0));
}


////////////////////////////////////////////////////////////
unsigned int Texture::getMaximumSize()
{
    ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());

    static const auto size = static_cast<unsigned int>(priv::getGLInteger(GL_MAX_TEXTURE_SIZE));
    return size;
}


////////////////////////////////////////////////////////////
Texture& Texture::operator=(const Texture& rhs)
{
    Texture temp(rhs);

    swap(temp);

    return *this;
}


////////////////////////////////////////////////////////////
void Texture::swap(Texture& rhs) noexcept
{
    za::genericSwap(m_size, rhs.m_size);
    za::genericSwap(m_texture, rhs.m_texture);
    za::genericSwap(m_isSmooth, rhs.m_isSmooth);
    za::genericSwap(m_sRgb, rhs.m_sRgb);
    za::genericSwap(m_wrapMode, rhs.m_wrapMode);
    za::genericSwap(m_fboAttachment, rhs.m_fboAttachment);
    za::genericSwap(m_hasMipmap, rhs.m_hasMipmap);
    za::genericSwap(m_cacheId, rhs.m_cacheId);

    // Both textures' content/state was just replaced wholesale; bump on each.
    ++m_destructiveGeneration;
    ++rhs.m_destructiveGeneration;
}


////////////////////////////////////////////////////////////
unsigned int Texture::getNativeHandle() const
{
    return m_texture;
}


////////////////////////////////////////////////////////////
Rect2f Texture::getRect() const
{
    return {{0, 0}, getSize().toVec2f()};
}


////////////////////////////////////////////////////////////
void swap(Texture& lhs, Texture& rhs) noexcept
{
    lhs.swap(rhs);
}

} // namespace za
