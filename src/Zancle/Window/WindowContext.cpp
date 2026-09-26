// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Window/WindowContext.hpp"

#include "Zancle/GLUtils/CopyFramebuffer.hpp"
#include "Zancle/GLUtils/FramebufferSaver.hpp"
#include "Zancle/GLUtils/GLCheck.hpp"
#include "Zancle/GLUtils/GLContextSaver.hpp"
#include "Zancle/GLUtils/GLDebugCallback.hpp"
#include "Zancle/GLUtils/GLUtils.hpp"
#include "Zancle/GLUtils/GlContext.hpp"
#include "Zancle/GLUtils/GlFuncTypesImpl.hpp"
#include "Zancle/GLUtils/Glad.hpp"
#include "Zancle/GLUtils/TextureSaver.hpp"

#include "Zancle/Window/ContextSettings.hpp"
#include "Zancle/Window/JoystickManager.hpp"
#include "Zancle/Window/SDLGlContext.hpp"
#include "Zancle/Window/SDLLayer.hpp"
#include "Zancle/Window/SensorManager.hpp"

#include "Zancle/Err/Err.hpp"
#include "Zancle/Err/SignalErrHandler.hpp"

#include "Zancle/Concurrency/Atomic.hpp"
#include "Zancle/Concurrency/AtomicMutex.hpp"
#include "Zancle/Concurrency/LockGuard.hpp"

#include "Zancle/String/StringView.hpp"
#include "Zancle/String/Utf8String.hpp"

#include "Zancle/Algorithm/Find.hpp"

#include "Zancle/Container/AnkerlUnorderedDense.hpp"
#include "Zancle/Container/Vector.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Vocabulary/Optional.hpp"
#include "Zancle/Vocabulary/PassKey.hpp"
#include "Zancle/Vocabulary/UniquePtr.hpp"

#include "Zancle/Base/Abort.hpp"
#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/Macros.hpp"

#include <SDL3/SDL_hints.h> // TODO P0: move to SDLLayer


namespace za
{
namespace
{
////////////////////////////////////////////////////////////
/// \brief Load our extensions vector with the supported extensions
///
////////////////////////////////////////////////////////////
[[nodiscard]] za::Vector<za::StringView> loadExtensions(priv::SDLGlContext& glContext)
{
    za::Vector<za::StringView> result; // Use a single local variable for NRVO

    auto glGetErrorFunc    = reinterpret_cast<glGetErrorFuncType>(glContext.getFunction("glGetError"));
    auto glGetIntegervFunc = reinterpret_cast<glGetIntegervFuncType>(glContext.getFunction("glGetIntegerv"));
    auto glGetStringFunc   = reinterpret_cast<glGetStringFuncType>(glContext.getFunction("glGetString"));

    if (!glGetErrorFunc || !glGetIntegervFunc || !glGetStringFunc)
        return result; // Empty vector

    // Drain any pre-existing GL errors (Emscripten leaves errors after context setup)
    while (glGetErrorFunc() != GL_NO_ERROR)
        ;

    // Check whether a >= 3.0 context is available
    int majorVersion = 0;
    glCheckIgnoreWithFunc(glGetErrorFunc, glGetIntegervFunc(GL_MAJOR_VERSION, &majorVersion));

    auto glGetStringiFunc = reinterpret_cast<glGetStringiFuncType>(glContext.getFunction("glGetStringi"));

    // Try to load the >= 3.0 way
    int numExtensions = 0;
    glCheckIgnoreWithFunc(glGetErrorFunc, glGetIntegervFunc(GL_NUM_EXTENSIONS, &numExtensions));

    if (numExtensions == 0)
        return result; // Empty vector

    for (int i = 0; i < numExtensions; ++i)
        if (const auto* extensionString = reinterpret_cast<const char*>(
                glCheckIgnoreWithFunc(glGetErrorFunc, glGetStringiFunc(GL_EXTENSIONS, static_cast<unsigned int>(i)))))
            result.emplaceBack(extensionString);

    return result;
}


////////////////////////////////////////////////////////////
struct UnsharedContextResources
{
    ////////////////////////////////////////////////////////////
    ankerl::unordered_dense::set<unsigned int> frameBufferIds;
    ankerl::unordered_dense::set<unsigned int> vaoIds;

    ////////////////////////////////////////////////////////////
    void clear()
    {
        frameBufferIds.clear();
        vaoIds.clear();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool allEmpty() const
    {
        return frameBufferIds.empty() && vaoIds.empty();
    }
};


////////////////////////////////////////////////////////////
class UnsharedContextResourcesManager
{
private:
    mutable AtomicMutex                                                  m_mutex;
    ankerl::unordered_dense::map<unsigned int, UnsharedContextResources> m_mapping;

    ////////////////////////////////////////////////////////////
    void registerImpl(auto idsPmr, const unsigned int glContextId, const unsigned int id)
    {
        LockGuard lock(m_mutex);

        auto& resources = m_mapping[glContextId];
        auto& idsSet    = (resources.*idsPmr);

        if (idsSet.contains(id))
            return;

        idsSet.emplace(id);
    }

    ////////////////////////////////////////////////////////////
    void unregisterImpl(auto idsPmr, const unsigned int glContextId, const unsigned int id, auto fGlDeleteFunc)
    {
        LockGuard lock(m_mutex);

        auto& resources = m_mapping[glContextId];
        auto& idsSet    = (resources.*idsPmr);

        if (!idsSet.contains(id))
            return;

        glCheck(fGlDeleteFunc(1, &id));
        idsSet.erase(id);
    }

public:
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool allEmpty() const
    {
        LockGuard lock(m_mutex);

        for (const auto& [glContextId, resources] : m_mapping)
            if (!resources.allEmpty())
                return false;

        return true;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool allNonSharedEmpty() const
    {
        LockGuard lock(m_mutex);

        for (const auto& [glContextId, resources] : m_mapping)
            if (glContextId != 1u && !resources.allEmpty())
                return true;

        return false;
    }

    ////////////////////////////////////////////////////////////
    void unregisterAllResources(const unsigned int glContextId)
    {
        LockGuard lock(m_mutex);

        auto& resources = m_mapping[glContextId];
        if (resources.allEmpty())
            return;

        for (const unsigned int frameBufferId : resources.frameBufferIds)
            glCheck(glDeleteFramebuffers(1, &frameBufferId));

        for (const unsigned int vaoId : resources.vaoIds)
            glCheck(glDeleteVertexArrays(1, &vaoId));

        resources.clear();
    }

    ////////////////////////////////////////////////////////////
    void registerFrameBuffer(const unsigned int glContextId, const unsigned int frameBufferId)
    {
        registerImpl(&UnsharedContextResources::frameBufferIds, glContextId, frameBufferId);
    }

    ////////////////////////////////////////////////////////////
    void unregisterFrameBuffer(const unsigned int glContextId, const unsigned int frameBufferId)
    {
        unregisterImpl(&UnsharedContextResources::frameBufferIds, glContextId, frameBufferId, glDeleteFramebuffers);
    }

    ////////////////////////////////////////////////////////////
    void registerVAO(const unsigned int glContextId, const unsigned int vaoId)
    {
        registerImpl(&UnsharedContextResources::vaoIds, glContextId, vaoId);
    }

    ////////////////////////////////////////////////////////////
    void unregisterVAO(const unsigned int glContextId, const unsigned int vaoId)
    {
        unregisterImpl(&UnsharedContextResources::vaoIds, glContextId, vaoId, glDeleteVertexArrays);
    }
};


////////////////////////////////////////////////////////////
struct ContextTransferScratch
{
    unsigned int readFramebuffer{};
    unsigned int drawFramebuffer{};

    unsigned int flipFramebuffer{};
    unsigned int flipTexture{};
    za::Vec2u    flipTextureSize{};
    bool         flipTextureSrgb{};

    // `true` once `flipTexture` has been attached to `flipFramebuffer` and the FBO has been verified complete. Reset
    // whenever either is (re)created. Skipping  the per-frame `glFramebufferTexture2D` + `glCheckFramebufferStatus`
    // saves ~1.6 ms / frame on Chrome's WebGL implementation (the status check forces a GPU pipeline sync).
    bool flipFramebufferAttachmentValidated{};
};


////////////////////////////////////////////////////////////
class TransferScratchManager
{
private:
    AtomicMutex                                                        m_mutex;
    ankerl::unordered_dense::map<unsigned int, ContextTransferScratch> m_byContext;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] static unsigned int getActiveContextId()
    {
        ZA_ASSERT(za::WindowContext::hasActiveThreadLocalGlContext());
        return za::WindowContext::getActiveThreadLocalGlContextId();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] ContextTransferScratch& getOrCreateForActiveContext()
    {
        const auto [it, inserted] = m_byContext.try_emplace(getActiveContextId());
        return it->second;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] static unsigned int ensureFramebuffer(unsigned int& framebufferId, const char* const what)
    {
        if (framebufferId != 0u)
            return framebufferId;

        glCheck(glGenFramebuffers(1, &framebufferId));

        if (framebufferId == 0u)
            priv::errMsg("Failed to create transfer scratch {}", what);

        return framebufferId;
    }

    ////////////////////////////////////////////////////////////
    static void deleteFramebuffer(unsigned int& framebufferId)
    {
        if (framebufferId == 0u)
            return;

        glCheck(glDeleteFramebuffers(1, &framebufferId));
        framebufferId = 0u;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] static unsigned int ensureFlipTexture(ContextTransferScratch& scratch, const za::Vec2u size, const bool sRgb)
    {
        if (scratch.flipTexture != 0u && scratch.flipTextureSize == size && scratch.flipTextureSrgb == sRgb)
            return scratch.flipTexture;

        if (scratch.flipTexture != 0u)
        {
            glCheck(glDeleteTextures(1, &scratch.flipTexture));
            scratch.flipTexture = 0u;
        }

        // Texture handle is about to change -- any prior attachment to the flip FBO is now stale.
        scratch.flipFramebufferAttachmentValidated = false;

        const za::priv::TextureSaver textureSaver;

        glCheck(glGenTextures(1, &scratch.flipTexture));

        if (scratch.flipTexture == 0u)
        {
            priv::errMsg("Failed to create transfer scratch flip texture");
            return 0u;
        }

        za::priv::bindAndInitializeTexture(scratch.flipTexture, sRgb, size, GL_CLAMP_TO_EDGE);

        scratch.flipTextureSize = size;
        scratch.flipTextureSrgb = sRgb;

        return scratch.flipTexture;
    }

    ////////////////////////////////////////////////////////////
    static void deleteTexture(unsigned int& textureId)
    {
        if (textureId == 0u)
            return;

        glCheck(glDeleteTextures(1, &textureId));
        textureId = 0u;
    }

public:
    ////////////////////////////////////////////////////////////
    TransferScratchManager() = default;

    ////////////////////////////////////////////////////////////
    ~TransferScratchManager()
    {
        // Verify that everything is already released for all contexts
        if (!m_byContext.empty())
        {
            priv::errMsg("TransferScratchManager destroyed with unreleased resources for {} contexts", m_byContext.size());

            za::abort();
        }
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] unsigned int getReadFramebuffer()
    {
        const LockGuard lock(m_mutex);

        auto& scratch = getOrCreateForActiveContext();
        return ensureFramebuffer(scratch.readFramebuffer, "read framebuffer");
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] unsigned int getDrawFramebuffer()
    {
        const LockGuard lock(m_mutex);

        auto& scratch = getOrCreateForActiveContext();
        return ensureFramebuffer(scratch.drawFramebuffer, "draw framebuffer");
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] unsigned int ensureFlipTexture(const za::Vec2u size, const bool sRgb)
    {
        const LockGuard lock(m_mutex);

        auto& scratch = getOrCreateForActiveContext();
        return ensureFlipTexture(scratch, size, sRgb);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Get the flip FBO with the scratch texture attached and validated
    ///
    /// Lazily creates the flip framebuffer and the matching scratch
    /// texture, attaches the texture, and runs `glCheckFramebufferStatus`
    /// once. Subsequent calls with the same `size`/`sRgb` skip both the
    /// attach and the status check entirely, returning the cached FBO
    /// directly. The cache is invalidated whenever the texture has to be
    /// recreated (different size, different sRgb, or first call) or when
    /// the FBO itself is freshly created.
    ///
    /// \return The flip FBO ID, or `0` on failure
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] unsigned int ensureFlipFramebufferReady(const za::Vec2u size, const bool sRgb)
    {
        const LockGuard lock(m_mutex);

        auto& scratch = getOrCreateForActiveContext();

        // If the FBO is about to be freshly created, the cached attachment is stale.
        if (scratch.flipFramebuffer == 0u)
            scratch.flipFramebufferAttachmentValidated = false;

        const unsigned int fbo = ensureFramebuffer(scratch.flipFramebuffer, "flip framebuffer");
        if (fbo == 0u)
            return 0u;

        // `ensureFlipTexture` resets `flipFramebufferAttachmentValidated` if the texture is recreated.
        const unsigned int tex = ensureFlipTexture(scratch, size, sRgb);
        if (tex == 0u)
            return 0u;

        if (!scratch.flipFramebufferAttachmentValidated)
        {
            const za::priv::FramebufferSaver fbsaver;

            glCheck(glBindFramebuffer(GL_FRAMEBUFFER, fbo));
            glCheck(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0));

            if (glCheck(glCheckFramebufferStatus(GL_FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE)
            {
                priv::errMsg("Failure to complete intermediate FBO in `copyFlippedFramebuffer`");
                return 0u;
            }

            scratch.flipFramebufferAttachmentValidated = true;
        }

        return fbo;
    }

    ////////////////////////////////////////////////////////////
    void releaseForActiveContext()
    {
        if (!za::WindowContext::isInstalled() || !za::WindowContext::hasActiveThreadLocalGlContext())
            return;

        const LockGuard lock(m_mutex);

        const unsigned int contextId = getActiveContextId();
        auto*              it        = m_byContext.find(contextId);

        if (it == m_byContext.end())
            return;

        deleteFramebuffer(it->second.readFramebuffer);
        deleteFramebuffer(it->second.drawFramebuffer);

        deleteFramebuffer(it->second.flipFramebuffer);
        deleteTexture(it->second.flipTexture);

        m_byContext.erase(it);
    }
};


#ifdef ZA_OPENGL_ES
////////////////////////////////////////////////////////////
/// \brief Copy framebuffer contents with vertical flipping using reusable scratch storage
///
/// Copies source framebuffer contents to destination while vertically flipping
/// the image. On OpenGL ES this uses a reusable intermediate texture/FBO pair.
///
/// \param intermediateFBO The intermediate FBO ID (cannot be 0)
/// \param size            Dimensions of the region to copy
/// \param srcFBO          Source framebuffer ID
/// \param dstFBO          Destination framebuffer ID
/// \param srcPos          Source region starting position (default: 0,0)
/// \param dstPos          Destination region starting position (default: 0,0)
///
////////////////////////////////////////////////////////////
void copyFlippedFramebufferViaTransferScratch(
    const unsigned int intermediateFBO,
    const za::Vec2u    size,
    const unsigned int srcFBO,
    const unsigned int dstFBO,
    const za::Vec2u    srcPos,
    const za::Vec2u    dstPos)
{
    ZA_ASSERT(intermediateFBO != 0u);

    // The intermediate FBO already has the scratch texture attached and verified
    // complete by `TransferScratchManager::ensureFlipFramebufferReady`, so we can
    // go straight to the two blits.
    //
    // Note that we do NOT save+restore `GL_{READ,DRAW}_FRAMEBUFFER_BINDING` here:
    // each `gl.getParameter` query is cheap (~12us cached) but pure overhead, and
    // both call sites tolerate the absence of restoration:
    //
    //   * `RenderTexture::Impl::updateTexture` (the hot path) does no FBO-dependent
    //     work after this returns -- `invalidateMipmap` only touches `GL_TEXTURE_2D`
    //     under a `TextureSaver`, and the next user-initiated draw goes through
    //     `RenderTarget::setActive` which rebinds the appropriate FBO.
    //
    //   * `Texture::update(const Window&)` already wraps this call in its own
    //     outer `FramebufferSaver`, so an inner one would have been strictly
    //     redundant with it.
    //
    za::priv::copyFramebuffer(/* invertYAxis */ false, size, srcFBO, intermediateFBO, srcPos, {});
    za::priv::copyFramebuffer(/* invertYAxis */ true, size, intermediateFBO, dstFBO, {}, dstPos);
}
#endif


#ifndef ZA_OPENGL_ES
////////////////////////////////////////////////////////////
/// \brief Copy framebuffer contents with vertical flipping
///
/// Copies source framebuffer contents to destination while vertically flipping
/// the image.
///
/// \param size   Dimensions of the region to copy
/// \param srcFBO Source framebuffer ID
/// \param dstFBO Destination framebuffer ID
/// \param srcPos Source region starting position (default: 0,0)
/// \param dstPos Destination region starting position (default: 0,0)
///
////////////////////////////////////////////////////////////
void copyFlippedFramebufferViaDirectBlit(
    [[maybe_unused]] const bool sRgb,
    const za::Vec2u             size,
    const unsigned int          srcFBO,
    const unsigned int          dstFBO,
    const za::Vec2u             srcPos,
    const za::Vec2u             dstPos)
{
    const za::priv::FramebufferSaver framebufferSaver;
    za::priv::copyFramebuffer(/* invertYAxis */ true, size, srcFBO, dstFBO, srcPos, dstPos);
}
#endif


} // namespace

////////////////////////////////////////////////////////////
struct WindowContextImpl
{
    ////////////////////////////////////////////////////////////
    priv::SDLLayer sdlLayer; //!< SDL layer instance, must be initialized before GL context

    ////////////////////////////////////////////////////////////
    za::Atomic<unsigned int> nextThreadLocalGlContextId{2u}; // 1 is reserved for shared context

    ////////////////////////////////////////////////////////////
    TransferScratchManager transferScratchManager; //!< Manager for the scratch FBOs and textures used for copy operations

    ////////////////////////////////////////////////////////////
    UnsharedContextResourcesManager unsharedContextResourcesManager;

    ////////////////////////////////////////////////////////////
    priv::SDLGlContext sharedGlContext; //!< The hidden, inactive context that will be shared with all other contexts
    AtomicMutex        sharedGlContextMutex;

    ////////////////////////////////////////////////////////////
    za::Vector<za::StringView> extensions; //!< Supported OpenGL extensions

    ////////////////////////////////////////////////////////////
    za::Optional<priv::JoystickManager> joystickManager;
    za::Optional<priv::SensorManager>   sensorManager;

    ////////////////////////////////////////////////////////////
    template <typename... SharedGlContextArgs>
    explicit WindowContextImpl(SharedGlContextArgs&&... args) : sharedGlContext(sdlLayer, ZA_FORWARD(args)...)
    {
    }
};

namespace
{
////////////////////////////////////////////////////////////
constinit za::Optional<WindowContextImpl> installedWindowContext;
constinit za::Atomic<unsigned int>        windowContextRC{0u};


////////////////////////////////////////////////////////////
void uninstall()
{
    installedWindowContext.reset();
    priv::GlContext::setSharedGlContext(nullptr);
}


////////////////////////////////////////////////////////////
WindowContextImpl& ensureInstalled()
{
    if (!installedWindowContext.hasValue()) [[unlikely]]
    {
        priv::errMsg("`za::WindowContext` not installed -- did you forget to create one in `main`?");
        za::abort();
    }

    return *installedWindowContext;
}

} // namespace

////////////////////////////////////////////////////////////
za::Optional<WindowContext> WindowContext::create()
{
    const auto fail = [](const char* what)
    {
        priv::errMsg("Error creating `za::WindowContext`: {}", what);
        return za::nullOpt;
    };

    //
    // Ensure window context is not already installed
    if (installedWindowContext.hasValue())
        return fail("a `za::WindowContext` object already exists");

    //
    // Shared context settings
    const ContextSettings sharedContextSettings{};

    //
    // Install window context
    auto& wc = installedWindowContext.emplace(/* id */ 1u, /* shared */ nullptr, sharedContextSettings);
    priv::GlContext::setSharedGlContext(&wc.sharedGlContext);

    //
    // Define fatal signal handlers for the user that will display a stack trace
    priv::installSignalErrHandler();

    //
    // Enable shader GL context
    ZA_ASSERT(!hasActiveThreadLocalGlContext());

    if (!setActiveThreadLocalGlContextToSharedContext())
    {
        uninstall();
        return fail("could not enable shared context");
    }

    ZA_ASSERT(isActiveGlContextSharedContext());

    //
    // Try to initialize shared GL context
    if (!wc.sharedGlContext.initialize(wc.sharedGlContext, sharedContextSettings))
    {
        uninstall();
        return fail("could not initialize shared context");
    }

    //
    // Load extensions and entrypoints
    ZA_ASSERT(isActiveGlContextSharedContext());
    wc.extensions = loadExtensions(wc.sharedGlContext);
    loadGLEntryPointsViaGLAD();

#ifndef ZA_SYSTEM_EMSCRIPTEN
    if (sharedContextSettings.isDebug())
        priv::setupGLDebugCallback();

    // Retrieve the context version number
    const auto majorVersion = priv::getGLInteger(GL_MAJOR_VERSION);
    const auto minorVersion = priv::getGLInteger(GL_MINOR_VERSION);

    if ((majorVersion < 1) || ((majorVersion == 1) && (minorVersion < 1)))
    {
        uninstall();
        return fail("support for OpenGL 1.1 or greater required, ensure hardware acceleration is enabled");
    }
#else
    // Need to drain errors here or subsequent assertion will fail on Emscripten, unsure why
    while (glGetError() != GL_NO_ERROR)
        ;
#endif

    ZA_ASSERT(glGetError() == GL_NO_ERROR);

    return za::makeOptional<WindowContext>(za::PassKey<WindowContext>{});
}


////////////////////////////////////////////////////////////
WindowContext::WindowContext(za::PassKey<WindowContext>&&)
{
    windowContextRC.fetchAddRelaxed(1u);
}


////////////////////////////////////////////////////////////
WindowContext::WindowContext(za::PassKey<GraphicsContext>&&) : WindowContext(za::PassKey<WindowContext>{})
{
}


////////////////////////////////////////////////////////////
WindowContext::WindowContext(WindowContext&&) noexcept : WindowContext(za::PassKey<WindowContext>{})
{
}


////////////////////////////////////////////////////////////
WindowContext::~WindowContext()
{
    if (windowContextRC.fetchSubRelaxed(1u) > 1u)
        return;

    ZA_ASSERT(!ensureInstalled().unsharedContextResourcesManager.allNonSharedEmpty());

    // All the FBOs on shared context should be destroyed later

    ZA_ASSERT(hasActiveThreadLocalGlContext());
    ZA_ASSERT(isActiveGlContextSharedContext());

    disableSharedGlContext();
    ZA_ASSERT(!hasActiveThreadLocalGlContext());

    uninstall();
}


////////////////////////////////////////////////////////////
priv::SDLLayer& WindowContext::getSDLLayer()
{
    return ensureInstalled().sdlLayer;
}


////////////////////////////////////////////////////////////
void WindowContext::registerUnsharedFrameBuffer(const unsigned int glContextId, const unsigned int frameBufferId)
{
    auto& wc = ensureInstalled();

    ZA_ASSERT(getActiveThreadLocalGlContextId() == glContextId);
    wc.unsharedContextResourcesManager.registerFrameBuffer(glContextId, frameBufferId);
}


////////////////////////////////////////////////////////////
void WindowContext::unregisterUnsharedFrameBuffer(const unsigned int glContextId, const unsigned int frameBufferId)
{
    auto& wc = ensureInstalled();

    // If we're not on the right context, wait for the cleanup later on
    if (getActiveThreadLocalGlContextId() != glContextId)
        return;

    wc.unsharedContextResourcesManager.unregisterFrameBuffer(glContextId, frameBufferId);
}


////////////////////////////////////////////////////////////
void WindowContext::registerUnsharedVAO(const unsigned int glContextId, const unsigned int vaoId)
{
    auto& wc = ensureInstalled();

    ZA_ASSERT(getActiveThreadLocalGlContextId() == glContextId);
    wc.unsharedContextResourcesManager.registerVAO(glContextId, vaoId);
}


////////////////////////////////////////////////////////////
void WindowContext::unregisterUnsharedVAO(const unsigned int glContextId, const unsigned int vaoId)
{
    auto& wc = ensureInstalled();

    // If we're not on the right context, wait for the cleanup later on
    if (getActiveThreadLocalGlContextId() != glContextId)
        return;

    wc.unsharedContextResourcesManager.unregisterVAO(glContextId, vaoId);
}


////////////////////////////////////////////////////////////
void WindowContext::cleanupUnsharedFrameBuffers(priv::GlContext& glContext)
{
    auto& wc = ensureInstalled();

    const auto doCleanup = [&]
    {
        // Make this context active so resources can be freed
        if (!setActiveThreadLocalGlContext(glContext, true))
            priv::errMsg("Could not enable GL context in GlContext::cleanupUnsharedFrameBuffers()");

        wc.unsharedContextResourcesManager.unregisterAllResources(glContext.getId());
        wc.transferScratchManager.releaseForActiveContext();
    };


    if (&glContext == &wc.sharedGlContext)
    {
        doCleanup();
        ZA_ASSERT(wc.unsharedContextResourcesManager.allEmpty());

        disableSharedGlContext();
        return;
    }

    // Save the current context so we can restore it later
    priv::GLContextSaver glContextSaver;
    doCleanup();
}


////////////////////////////////////////////////////////////
priv::GlContext* WindowContext::getActiveThreadLocalGlContextPtr()
{
    ensureInstalled();
    return priv::GlContext::getActiveThreadLocalGlContextPtr();
}


////////////////////////////////////////////////////////////
priv::GlContext& WindowContext::getSharedGlContext()
{
    return ensureInstalled().sharedGlContext;
}


////////////////////////////////////////////////////////////
priv::JoystickManager& WindowContext::getJoystickManager()
{
    return ensureInstalled().joystickManager.emplaceIfNeeded();
}


////////////////////////////////////////////////////////////
priv::SensorManager& WindowContext::getSensorManager()
{
    return ensureInstalled().sensorManager.emplaceIfNeeded();
}


////////////////////////////////////////////////////////////
unsigned int WindowContext::getActiveThreadLocalGlContextId()
{
    ensureInstalled();
    return priv::GlContext::getActiveThreadLocalGlContextId();
}


////////////////////////////////////////////////////////////
void WindowContext::setAppId(const Utf8String& id)
{
    SDL_SetHint(SDL_HINT_APP_ID, id.cStr());
}


////////////////////////////////////////////////////////////
bool WindowContext::hasActiveThreadLocalGlContext()
{
    ensureInstalled();
    return priv::GlContext::hasActiveThreadLocalGlContext();
}


////////////////////////////////////////////////////////////
bool WindowContext::setActiveThreadLocalGlContext(priv::GlContext& glContext, const bool active)
{
    ensureInstalled();
    return priv::GlContext::setActiveThreadLocalGlContext(glContext, active);
}


////////////////////////////////////////////////////////////
bool WindowContext::setActiveThreadLocalGlContextToSharedContext()
{
    ensureInstalled();
    return priv::GlContext::setActiveThreadLocalGlContextToSharedContext();
}


////////////////////////////////////////////////////////////
[[nodiscard]] bool WindowContext::isSharedContext(priv::GlContext& glContext)
{
    auto& wc = ensureInstalled();
    return &glContext == &wc.sharedGlContext;
}


////////////////////////////////////////////////////////////
bool WindowContext::isActiveGlContextSharedContext()
{
    ensureInstalled();
    return priv::GlContext::isActiveGlContextSharedContext();
}


////////////////////////////////////////////////////////////
void WindowContext::disableSharedGlContext()
{
    ensureInstalled();
    [[maybe_unused]] const bool disabled = priv::GlContext::disableSharedGlContext();
}


////////////////////////////////////////////////////////////
bool WindowContext::isInstalled()
{
    return installedWindowContext.hasValue();
}


////////////////////////////////////////////////////////////
void WindowContext::loadGLEntryPointsViaGLAD()
{
#ifdef ZA_OPENGL_ES
    gladLoadGLES2(&getFunction);
#else
    gladLoadGL(&getFunction);
#endif
}


////////////////////////////////////////////////////////////
template <typename... GLContextArgs>
za::UniquePtr<priv::GlContext> WindowContext::createGlContextImpl(const ContextSettings& contextSettings, GLContextArgs&&... args)
{
    auto& wc = ensureInstalled();

    const LockGuard lock(wc.sharedGlContextMutex);

    if (!setActiveThreadLocalGlContextToSharedContext())
        priv::errMsg("Error enabling shared GL context in WindowContext::createGlContext()");

    auto glContext = za::makeUnique<priv::SDLGlContext>(wc.sdlLayer,
                                                        wc.nextThreadLocalGlContextId.fetchAddSeqCst(1u),
                                                        &wc.sharedGlContext,
                                                        contextSettings,
                                                        ZA_FORWARD(args)...);

    if (!setActiveThreadLocalGlContext(*glContext, true))
    {
        priv::errMsg("Error enabling newly created GL context in GlContext::initialize()");
        return nullptr;
    }

    if (!glContext->initialize(wc.sharedGlContext, contextSettings))
    {
        priv::errMsg("Error initializing newly created GL context in WindowContext::createGlContext()");
        return nullptr;
    }

    if (contextSettings.isDebug())
        priv::setupGLDebugCallback();

    glContext->checkSettings(contextSettings);
    return glContext;
}


////////////////////////////////////////////////////////////
za::UniquePtr<priv::GlContext> WindowContext::createGlContext(const ContextSettings& contextSettings)
{
    return createGlContextImpl(contextSettings);
}


////////////////////////////////////////////////////////////
za::UniquePtr<priv::GlContext> WindowContext::createGlContext(const ContextSettings&     contextSettings,
                                                              const priv::SDLWindowImpl& owner,
                                                              const unsigned int         bitsPerPixel)
{
    return createGlContextImpl(contextSettings, owner, bitsPerPixel);
}


////////////////////////////////////////////////////////////
bool WindowContext::isExtensionAvailable(const char* const name)
{
    auto& wc = ensureInstalled();
    return za::find(wc.extensions.begin(), wc.extensions.end(), name) != wc.extensions.end();
}


////////////////////////////////////////////////////////////
GlFunctionPointer WindowContext::getFunction(const char* const name)
{
    return ensureInstalled().sharedGlContext.getFunction(name);
}


////////////////////////////////////////////////////////////
unsigned int WindowContext::getTransferScratchReadFramebuffer()
{
    return ensureInstalled().transferScratchManager.getReadFramebuffer();
}


////////////////////////////////////////////////////////////
unsigned int WindowContext::getTransferScratchDrawFramebuffer()
{
    return ensureInstalled().transferScratchManager.getDrawFramebuffer();
}


////////////////////////////////////////////////////////////
unsigned int WindowContext::ensureTransferScratchFlipFramebufferReady(const Vec2u size, const bool sRgb)
{
    return ensureInstalled().transferScratchManager.ensureFlipFramebufferReady(size, sRgb);
}


////////////////////////////////////////////////////////////
bool WindowContext::copyFlippedFramebuffer(
    const bool         sRgb,
    const Vec2u        size,
    const unsigned int srcFBO,
    const unsigned int dstFBO,
    const Vec2u        srcPos,
    const Vec2u        dstPos)
{
#ifdef ZA_OPENGL_ES
    // Lazily attach the scratch texture and validate completeness on first call;
    // every subsequent call returns the cached, already-ready FBO.
    const unsigned int intermediateFBO = ensureTransferScratchFlipFramebufferReady(size, sRgb);

    if (intermediateFBO == 0u)
        return false; // error already logged by `ensureTransferScratchFlipFramebufferReady`

    copyFlippedFramebufferViaTransferScratch(intermediateFBO, size, srcFBO, dstFBO, srcPos, dstPos);
    return true;
#else
    copyFlippedFramebufferViaDirectBlit(sRgb, size, srcFBO, dstFBO, srcPos, dstPos);
    return true;
#endif
}

} // namespace za
