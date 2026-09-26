// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/RenderTexture.hpp"

#include "Zancle/GLUtils/GLCheck.hpp"
#include "Zancle/GLUtils/GLRenderBufferObject.hpp"
#include "Zancle/GLUtils/GLUniqueResource.hpp"
#include "Zancle/GLUtils/GLUtils.hpp"
#include "Zancle/GLUtils/Glad.hpp"

#include "Zancle/Graphics/DepthStencilFormat.hpp"
#include "Zancle/Graphics/GraphicsContext.hpp"
#include "Zancle/Graphics/RenderTarget.hpp"
#include "Zancle/Graphics/RenderTextureCreateSettings.hpp"
#include "Zancle/Graphics/Texture.hpp"
#include "Zancle/Graphics/TextureWrapMode.hpp"

#include "Zancle/Window/WindowContext.hpp"

#include "Zancle/Err/Err.hpp"

#include "Zancle/Container/AnkerlUnorderedDense.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Vocabulary/Optional.hpp"
#include "Zancle/Vocabulary/PassKey.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/Macros.hpp"


namespace
{
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] inline constexpr GLenum getGLInternalFormat(const za::DepthStencilFormat format)
{
    switch (format)
    {
        case za::DepthStencilFormat::Depth16:
            return GL_DEPTH_COMPONENT16;

        case za::DepthStencilFormat::Depth24:
            return GL_DEPTH_COMPONENT24;

        case za::DepthStencilFormat::Stencil8:
            return GL_STENCIL_INDEX8;

        case za::DepthStencilFormat::Depth24Stencil8:
            return GL_DEPTH24_STENCIL8;

        case za::DepthStencilFormat::None:
            break;
    }

    ZA_ASSERT(false);
    return {};
}


////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] inline constexpr bool hasDepthComponent(const za::DepthStencilFormat format)
{
    return format == za::DepthStencilFormat::Depth16 || format == za::DepthStencilFormat::Depth24 ||
           format == za::DepthStencilFormat::Depth24Stencil8;
}


////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] inline constexpr bool hasStencilComponent(const za::DepthStencilFormat format)
{
    return format == za::DepthStencilFormat::Stencil8 || format == za::DepthStencilFormat::Depth24Stencil8;
}


////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] inline constexpr const char* getBufferTypeStr(const bool multisample,
                                                                                const bool stencil,
                                                                                const bool depth)
{
    if (stencil && depth)
        return multisample ? "multisample depth/stencil buffer" : "depth/stencil buffer";

    if (stencil)
        return multisample ? "multisample stencil buffer" : "stencil buffer";

    if (depth)
        return multisample ? "multisample depth buffer" : "depth buffer";

    ZA_ASSERT(false);
    return {};
}


////////////////////////////////////////////////////////////
void linkStencilDepthBuffer(const za::Optional<za::GLRenderBufferObject>& stencilDepthBuffer, const bool stencil, const bool depth)
{
    if (!stencilDepthBuffer.hasValue())
        return;

    if (stencil)
        glCheck(
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, stencilDepthBuffer->getId()));

    if (depth)
        glCheck(
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, stencilDepthBuffer->getId()));
}


////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] inline bool isBoundFramebufferComplete()
{
    return glCheck(glCheckFramebufferStatus(GL_FRAMEBUFFER)) == GL_FRAMEBUFFER_COMPLETE;
}


} // namespace


namespace za
{
////////////////////////////////////////////////////////////
struct RenderTexture::Impl
{
    using FramebufferIdMap = ankerl::unordered_dense::map<unsigned int, unsigned int>;

    Texture               texture;    //!< Target texture to draw on
    za::Optional<Texture> tmpTexture; //!< Temporary texture used for Y-axis flipping fallback or non multisample FBOs

    FramebufferIdMap framebuffers;    //!< Per-context OpenGL FBOs
    FramebufferIdMap auxFramebuffers; //!< Per-context auxiliary OpenGL FBOs (either multisample or temp for Y-flipping)

    za::Optional<GLRenderBufferObject> stencilDepthBuffer; //!< Optional depth/stencil buffer attached to the framebuffer
    za::Optional<GLRenderBufferObject> colorBuffer; //!< Optional multisample color buffer attached to the framebuffer

    bool multisample{}; //!< Must create a multisample framebuffer as well
    bool stencil{};     //!< Has stencil attachment
    bool depth{};       //!< Has depth attachment
    bool sRgb{};        //!< Must encode drawn pixels into sRGB color space

    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit Impl(Texture&& theTexture) : texture(ZA_MOVE(theTexture))
    {
    }

    Impl(const Impl&)            = delete;
    Impl& operator=(const Impl&) = delete;

    Impl(Impl&&) noexcept            = default;
    Impl& operator=(Impl&&) noexcept = default;

    ////////////////////////////////////////////////////////////
    void cleanup()
    {
        ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());

        stencilDepthBuffer.reset();
        colorBuffer.reset();

        // Unregister FBOs with the contexts if they haven't already been destroyed
        for (const auto& [glContextId, framebufferId] : framebuffers)
            GraphicsContext::unregisterUnsharedFrameBuffer(glContextId, framebufferId);

        for (const auto& [glContextId, auxFramebufferId] : auxFramebuffers)
            GraphicsContext::unregisterUnsharedFrameBuffer(glContextId, auxFramebufferId);

        framebuffers.clear();
        auxFramebuffers.clear();
    }


private:
    ////////////////////////////////////////////////////////////
    static void replaceFramebuffer(const unsigned int glContextId,
                                   FramebufferIdMap&  framebuffersByContext,
                                   const unsigned int framebufferId)
    {
        if (auto* it = framebuffersByContext.find(glContextId); it != framebuffersByContext.end())
        {
            if (it->second == framebufferId)
                return;

            GraphicsContext::unregisterUnsharedFrameBuffer(glContextId, it->second);
            it->second = framebufferId;
        }
        else
        {
            framebuffersByContext.emplace(glContextId, framebufferId);
        }

        GraphicsContext::registerUnsharedFrameBuffer(glContextId, framebufferId);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] Texture* ensureTmpTexture()
    {
        const auto size = texture.getSize();

        if (!tmpTexture.hasValue() || tmpTexture->getSize() != size || tmpTexture->isSrgb() != texture.isSrgb())
        {
            tmpTexture = Texture::create(size, {.sRgb = texture.isSrgb(), .smooth = texture.isSmooth()});
            return tmpTexture.asPtr();
        }

        if (tmpTexture->isSmooth() != texture.isSmooth())
            tmpTexture->setSmooth(texture.isSmooth());

        return tmpTexture.asPtr();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool createFail(const char* what)
    {
        glCheck(glBindFramebuffer(GL_FRAMEBUFFER, 0u));
        priv::errMsg("Impossible to create render texture ({})", what);
        return false;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool completeAuxFramebufferCreation(GLuint auxFramebufferId)
    {
        linkStencilDepthBuffer(stencilDepthBuffer, stencil, depth);

        if (!isBoundFramebufferComplete())
        {
            const GLuint fboToDelete = auxFramebufferId;
            glCheck(glDeleteFramebuffers(1, &fboToDelete));

            return createFail("failed to link the render buffers to the auxiliary framebuffer");
        }

        // Register the FBO in our map and with the current context so it is automatically destroyed
        const unsigned int glContextId = GraphicsContext::getActiveThreadLocalGlContextId();
        replaceFramebuffer(glContextId, auxFramebuffers, auxFramebufferId);

        return true;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool createAuxMultisampleFramebuffer()
    {
        ZA_ASSERT(multisample);

        // Create the multisample framebuffer object
        const GLuint multisampleFramebufferId = priv::generateAndBindFramebuffer();
        if (!multisampleFramebufferId)
            return createFail("failed to create the multisample framebuffer object");

        // Link the multisample color buffer to the framebuffer
        colorBuffer->bind();
        glCheck(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorBuffer->getId()));

        return completeAuxFramebufferCreation(multisampleFramebufferId);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool createAuxTempFramebuffer()
    {
        ZA_ASSERT(!multisample);

        // Create the framebuffer object
        const GLuint tempFramebufferId = priv::generateAndBindFramebuffer();
        if (!tempFramebufferId)
            return createFail("failed to create the temp framebuffer object");

        Texture* const currentTmpTexture = ensureTmpTexture();
        if (currentTmpTexture == nullptr)
        {
            glCheck(glDeleteFramebuffers(1, &tempFramebufferId));
            return createFail("failed to create the aux FBO texture");
        }

        // Link the texture to the framebuffer
        glCheck(
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, currentTmpTexture->getNativeHandle(), 0));

        return completeAuxFramebufferCreation(tempFramebufferId);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool createFramebuffer()
    {
        // Create the framebuffer object
        const GLuint framebufferId = priv::generateAndBindFramebuffer();
        if (!framebufferId)
            return createFail("failed to create the framebuffer object");

        // Link the texture to the framebuffer
        glCheck(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture.getNativeHandle(), 0));

        // Link the depth/stencil renderbuffer to the framebuffer
        if (!multisample)
            linkStencilDepthBuffer(stencilDepthBuffer, stencil, depth);

        if (!isBoundFramebufferComplete())
        {
            glCheck(glDeleteFramebuffers(1, &framebufferId));
            return createFail("failed to link the target texture to the framebuffer");
        }

        // Register the FBO in our map and with the current context so it is automatically destroyed
        const unsigned int glContextId = GraphicsContext::getActiveThreadLocalGlContextId();
        replaceFramebuffer(glContextId, framebuffers, framebufferId);

        return multisample ? createAuxMultisampleFramebuffer() : createAuxTempFramebuffer();
    }

public:
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool create(const RenderTextureCreateSettings& rtCreateSettings)
    {
        // OpenGL ES requires that all attachments have identical sizes

        const auto fail = [&](const auto&... what)
        {
            za::priv::ErrMsgScope scope;
            scope.append("Impossible to create render texture (");
            (scope.fmt("{}", what), ...);
            scope.append(")");

            return false;
        };

        ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());

        const auto size = texture.getSize();

        sRgb = rtCreateSettings.sRgbCapable;

        // Check if the requested anti-aliasing level is supported
        if (const auto samples = getMaximumSampleCount(); rtCreateSettings.sampleCount > samples)
            return fail("unsupported anti-aliasing level ", rtCreateSettings.sampleCount, ", maximum supported is ", samples);

        const auto bindRenderbufferAndSetFormat =
            [&size](GLRenderBufferObject& rbo, const unsigned int sampleCount, const GLenum internalFormat)
        {
            rbo.bind();

            glCheck(glRenderbufferStorageMultisample(GL_RENDERBUFFER,
                                                     static_cast<GLsizei>(sampleCount),
                                                     internalFormat,
                                                     static_cast<GLsizei>(size.x),
                                                     static_cast<GLsizei>(size.y)));
        };

        // `sampleCount` values of 0 and 1 both mean "multisampling
        // disabled" (see `RenderTextureCreateSettings::sampleCount`);
        // normalize so no 1-sample multisample storage is ever created.
        const unsigned int sampleCount = rtCreateSettings.sampleCount <= 1u ? 0u : rtCreateSettings.sampleCount;

        const DepthStencilFormat dsFormat = rtCreateSettings.depthStencilFormat;

        depth       = hasDepthComponent(dsFormat);
        stencil     = hasStencilComponent(dsFormat);
        multisample = sampleCount != 0u;

        // Create the (possibly multisample) depth/stencil buffer if requested
        if (dsFormat != DepthStencilFormat::None)
        {
            stencilDepthBuffer = tryCreateGLUniqueResource<GLRenderBufferObject>();
            if (!stencilDepthBuffer.hasValue())
                return fail("failed to create the attached ", getBufferTypeStr(multisample, stencil, depth));

            bindRenderbufferAndSetFormat(*stencilDepthBuffer, sampleCount, getGLInternalFormat(dsFormat));
        }

        // Create the multisample color buffer if needed
        if (multisample)
        {
            colorBuffer = tryCreateGLUniqueResource<GLRenderBufferObject>();
            if (!colorBuffer.hasValue())
                return fail("failed to create the attached multisample color buffer");

            bindRenderbufferAndSetFormat(*colorBuffer, sampleCount, sRgb ? GL_SRGB8_ALPHA8 : GL_RGBA8);
        }

        // Save the current bindings so we can restore them after we are done
        const auto readFramebuffer = priv::getGLInteger(GL_READ_FRAMEBUFFER_BINDING);
        const auto drawFramebuffer = priv::getGLInteger(GL_DRAW_FRAMEBUFFER_BINDING);

        if (!createFramebuffer())
            return false;

        // Restore previously bound framebuffers
        glCheck(glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(readFramebuffer)));
        glCheck(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(drawFramebuffer)));

        return true;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool activate(const bool active)
    {
        // Unbind the FBO if requested
        if (!active)
        {
            glCheck(glBindFramebuffer(GL_FRAMEBUFFER, 0u));
            return true;
        }

        ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());
        const unsigned int glContextId = GraphicsContext::getActiveThreadLocalGlContextId();

        // Lookup the FBO corresponding to the currently active context
        // If none is found, there is no FBO corresponding to the
        // currently active context so we will have to create a new FBO

        if (const auto* it = auxFramebuffers.find(glContextId); it != auxFramebuffers.end())
        {
            glCheck(glBindFramebuffer(GL_FRAMEBUFFER, it->second));
            return true;
        }

        return createFramebuffer();
    }

    ////////////////////////////////////////////////////////////
    void updateTexture(const bool scissorEnabledCached)
    {
        // If multisampling is enabled, we need to resolve by blitting from our FBO with multisample
        // renderbuffer attachments to our FBO to which our target texture is attached

        // In case of multisampling, make sure both FBOs are already available within the current context

        const auto size = texture.getSize();

        if (size.x == 0u || size.y == 0u || !activate(true))
            return;

        const unsigned int glContextId = GraphicsContext::getActiveThreadLocalGlContextId();

        const auto* framebufferIt = framebuffers.find(glContextId);
        if (framebufferIt == framebuffers.end())
            return;

        const auto* auxFramebufferIt = auxFramebuffers.find(glContextId);
        if (auxFramebufferIt == auxFramebuffers.end())
            return;

        // Since we don't want scissor testing to interfere with blits, temporarily disable it if needed.
        // The cached state is passed in by the caller to avoid a per-frame `glGetBooleanv` (which forces
        // a GPU sync on some WebGL implementations -- ~1 ms per call).

        if (scissorEnabledCached)
            glCheck(glDisable(GL_SCISSOR_TEST));

        // Blit from the auxiliary (multisample or temp) FBO to the main FBO, flipping Y axis
        if (!WindowContext::copyFlippedFramebuffer(texture.isSrgb(), size, auxFramebufferIt->second, framebufferIt->second))
            priv::errMsg("Error flipping render texture during FBO copy");

        if (scissorEnabledCached)
            glCheck(glEnable(GL_SCISSOR_TEST));
    }
};


////////////////////////////////////////////////////////////
RenderTexture::~RenderTexture()
{
    m_impl->cleanup();
}


////////////////////////////////////////////////////////////
RenderTexture::RenderTexture(RenderTexture&&) noexcept = default;


////////////////////////////////////////////////////////////
RenderTexture& RenderTexture::operator=(RenderTexture&& rhs) noexcept
{
    if (this == &rhs)
        return *this;

    m_impl->cleanup();

    RenderTarget::operator=(ZA_MOVE(rhs));
    m_impl = ZA_MOVE(rhs.m_impl);

    return *this;
}


////////////////////////////////////////////////////////////
za::Optional<RenderTexture> RenderTexture::create(const Vec2u size)
{
    return create(size, {});
}


////////////////////////////////////////////////////////////
za::Optional<RenderTexture> RenderTexture::create(const Vec2u size, const RenderTextureCreateSettings& rtCreateSettings)
{
    za::Optional<RenderTexture> result; // Use a single local variable for NRVO

    // Create the texture
    auto texture = za::Texture::create(size, {.sRgb = rtCreateSettings.sRgbCapable});
    if (!texture.hasValue())
    {
        priv::errMsg("Impossible to create render texture (failed to create the target texture)");
        return result; // Empty optional
    }

    // Use frame-buffer object (FBO)
    result.emplace(za::PassKey<RenderTexture>{}, ZA_MOVE(*texture));

    // Mark the texture as being a framebuffer object attachment
    result->m_impl->texture.m_fboAttachment = true;

    result->setSmooth(rtCreateSettings.smooth);
    result->setWrapMode(rtCreateSettings.wrapMode);

    // Initialize the render texture
    if (!result->m_impl->create(rtCreateSettings))
    {
        priv::errMsg("Impossible to create render texture (failed to create render texture renderTextureImpl)");

        result.reset();
        return result; // Empty optional
    }

    return result;
}


////////////////////////////////////////////////////////////
unsigned int RenderTexture::getMaximumSampleCount()
{
    ZA_ASSERT(GraphicsContext::hasActiveThreadLocalGlContext());
    return static_cast<unsigned int>(priv::getGLInteger(GL_MAX_SAMPLES));
}


////////////////////////////////////////////////////////////
void RenderTexture::setSmooth(bool smooth)
{
    m_impl->texture.setSmooth(smooth);
}


////////////////////////////////////////////////////////////
bool RenderTexture::isSmooth() const
{
    return m_impl->texture.isSmooth();
}


////////////////////////////////////////////////////////////
void RenderTexture::setWrapMode(TextureWrapMode wrapMode)
{
    m_impl->texture.setWrapMode(wrapMode);
}


////////////////////////////////////////////////////////////
TextureWrapMode RenderTexture::getWrapMode() const
{
    return m_impl->texture.getWrapMode();
}


////////////////////////////////////////////////////////////
void RenderTexture::generateMipmap()
{
    m_impl->texture.generateMipmap();
}


////////////////////////////////////////////////////////////
bool RenderTexture::setActive(bool active)
{
    // Update RenderTarget tracking
    if (m_impl->activate(active))
        return RenderTarget::setActive(active);

    return false;
}


////////////////////////////////////////////////////////////
RenderTarget::DrawStatistics RenderTexture::display()
{
    RenderTarget::DrawStatistics result{}; // Use a single local variable for NRVO

    // Perform a RenderTarget-only activation (we are using FBOs)
    if (!RenderTarget::setActive())
        return result;

    result = RenderTarget::flush();
    RenderTarget::syncGPUEndFrame();

    // Update the target texture. Pass the cached scissor-test state so the internal scissor disabling guard doesn't have
    // to issue a synchronous `glGetBooleanv(GL_SCISSOR_TEST)` (which can cost ~1 ms per frame on some WebGL implementations).
    m_impl->updateTexture(isScissorEnabledCached());
    m_impl->texture.invalidateMipmap();

    // Pixel content visible to samplers has just been replaced. Bumping this lets the auto-batch
    // detect "user re-rendered into a texture I have outstanding draw refs to" and flush early
    // (or assert under `withLockedRenderStates`).
    ++m_impl->texture.m_destructiveGeneration;

    return result;
}


////////////////////////////////////////////////////////////
Vec2u RenderTexture::getSize() const
{
    return m_impl->texture.getSize();
}


////////////////////////////////////////////////////////////
const Texture& RenderTexture::getTexture() const
{
    return m_impl->texture;
}


////////////////////////////////////////////////////////////
RenderTexture::RenderTexture(za::PassKey<RenderTexture>&&, Texture&& texture) :
    RenderTarget{texture.isSrgb()},
    m_impl(ZA_MOVE(texture))
{
}

} // namespace za
