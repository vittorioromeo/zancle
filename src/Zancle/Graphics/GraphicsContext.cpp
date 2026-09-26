// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/GraphicsContext.hpp"

#include "Zancle/Graphics/Color.hpp"
#include "Zancle/Graphics/DefaultShader.hpp"
#include "Zancle/Graphics/Image.hpp"
#include "Zancle/Graphics/Shader.hpp"
#include "Zancle/Graphics/Texture.hpp"

#include "Zancle/Window/WindowContext.hpp"

#include "Zancle/Err/Err.hpp"

#include "Zancle/Concurrency/Atomic.hpp"

#include "Zancle/Vocabulary/Optional.hpp"
#include "Zancle/Vocabulary/PassKey.hpp"

#include "Zancle/Base/Abort.hpp"
#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/Macros.hpp"


namespace za
{
namespace
{
///////////////////////////////////////////////////////////
struct GraphicsContextImpl
{
    Shader  builtInShader;
    Texture builtInWhiteDotTexture;
};


///////////////////////////////////////////////////////////
constinit za::Optional<GraphicsContextImpl> installedGraphicsContext;
constinit za::Atomic<unsigned int>          graphicsContextRC{0u};


////////////////////////////////////////////////////////////
GraphicsContextImpl& ensureInstalled()
{
    if (!installedGraphicsContext.hasValue()) [[unlikely]]
    {
        priv::errMsg("`za::GraphicsContext` not installed -- did you forget to create one in `main`?");
        za::abort();
    }

    return *installedGraphicsContext;
}

} // namespace

////////////////////////////////////////////////////////////
struct GraphicsContext::Impl
{
    explicit Impl(WindowContext&& theWindowContext) : windowContext(ZA_MOVE(theWindowContext))
    {
    }

    WindowContext windowContext;
};

////////////////////////////////////////////////////////////
za::Optional<GraphicsContext> GraphicsContext::create()
{
    const auto fail = [](const char* what)
    {
        priv::errMsg("Error creating `za::GraphicsContext`: {}", what);
        return za::nullOpt;
    };

    //
    // Ensure graphics context is not already installed.
    // NOTE: this exists-check and the `emplace` below are not atomic; two
    // threads racing `create()` could both pass the check. Acceptable, as a
    // `GraphicsContext` is expected to be created once in `main` before any
    // other thread touches the library.
    if (installedGraphicsContext.hasValue())
        return fail("a `za::GraphicsContext` object already exists");

    //
    // Install window context if necessary
    auto windowContext = WindowContext::isInstalled() ? za::makeOptional<WindowContext>(za::PassKey<GraphicsContext>{})
                                                      : WindowContext::create();

    if (!windowContext.hasValue())
        return fail("window context initialization failure");

    //
    // Initialize built-in shader
    auto shader = DefaultShader::create();
    if (!shader.hasValue())
        return fail("built-in shader initialization failure");

    //
    // Initialize built-in texture
    auto texture = Texture::loadFromImage(*Image::create({2u, 2u}, Color::White));
    if (!texture.hasValue())
        return fail("built-in texture initialization failure");

    //
    // Install graphics context
    installedGraphicsContext.emplace(*ZA_MOVE(shader), *ZA_MOVE(texture));

    return za::makeOptional<GraphicsContext>(za::PassKey<GraphicsContext>{}, ZA_MOVE(*windowContext));
}


////////////////////////////////////////////////////////////
GraphicsContext::GraphicsContext(za::PassKey<GraphicsContext>&&, WindowContext&& windowContext) :
    m_impl(ZA_MOVE(windowContext))
{
    graphicsContextRC.fetchAddRelaxed(1u);
}


////////////////////////////////////////////////////////////
GraphicsContext::GraphicsContext(GraphicsContext&& rhs) noexcept :
    m_impl(static_cast<WindowContext&&>(rhs.m_impl->windowContext))
{
    graphicsContextRC.fetchAddRelaxed(1u);
}


////////////////////////////////////////////////////////////
GraphicsContext::~GraphicsContext()
{
    if (graphicsContextRC.fetchSubRelaxed(1u) > 1u)
        return;

    // Need to activate shared context during destruction to avoid GL errors when destroying texture and shader
    [[maybe_unused]] const bool rc = WindowContext::setActiveThreadLocalGlContextToSharedContext();
    ZA_ASSERT(rc);

    installedGraphicsContext.reset();
}


////////////////////////////////////////////////////////////
Shader& GraphicsContext::getBuiltInShader()
{
    return ensureInstalled().builtInShader;
}


////////////////////////////////////////////////////////////
Texture& GraphicsContext::getBuiltInWhiteDotTexture()
{
    return ensureInstalled().builtInWhiteDotTexture;
}


////////////////////////////////////////////////////////////
unsigned int GraphicsContext::getActiveThreadLocalGlContextId()
{
    return WindowContext::getActiveThreadLocalGlContextId();
}


////////////////////////////////////////////////////////////
bool GraphicsContext::hasActiveThreadLocalGlContext()
{
    return WindowContext::hasActiveThreadLocalGlContext();
}


////////////////////////////////////////////////////////////
void GraphicsContext::registerUnsharedFrameBuffer(const unsigned int glContextId, const unsigned int frameBufferId)
{
    WindowContext::registerUnsharedFrameBuffer(glContextId, frameBufferId);
}


////////////////////////////////////////////////////////////
void GraphicsContext::unregisterUnsharedFrameBuffer(const unsigned int glContextId, const unsigned int frameBufferId)
{
    WindowContext::unregisterUnsharedFrameBuffer(glContextId, frameBufferId);
}


////////////////////////////////////////////////////////////
void GraphicsContext::registerUnsharedVAO(const unsigned int glContextId, const unsigned int vaoId)
{
    WindowContext::registerUnsharedVAO(glContextId, vaoId);
}


////////////////////////////////////////////////////////////
void GraphicsContext::unregisterUnsharedVAO(const unsigned int glContextId, const unsigned int vaoId)
{
    WindowContext::unregisterUnsharedVAO(glContextId, vaoId);
}


////////////////////////////////////////////////////////////
bool GraphicsContext::isInstalled()
{
    return installedGraphicsContext.hasValue();
}


////////////////////////////////////////////////////////////
Shader& GraphicsContext::getInstalledBuiltInShader()
{
    ZA_ASSERT(installedGraphicsContext.hasValue());
    return installedGraphicsContext->builtInShader;
}


////////////////////////////////////////////////////////////
Texture& GraphicsContext::getInstalledBuiltInWhiteDotTexture()
{
    ZA_ASSERT(installedGraphicsContext.hasValue());
    return installedGraphicsContext->builtInWhiteDotTexture;
}

} // namespace za
