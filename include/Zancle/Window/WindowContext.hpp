#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Window/Export.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Vocabulary/Optional.hpp"
#include "Zancle/Vocabulary/PassKey.hpp"
#include "Zancle/Vocabulary/UniquePtr.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za::priv
{
class EglContext;
class GlContext;
class JoystickManager;
class SDLGlContext;
class SDLLayer;
class SDLWindowImpl;
class SensorManager;

} // namespace za::priv


namespace za
{
class Cursor;
class GraphicsContext;
class RenderTarget;
class RenderTexture;
class Shader;
class Texture;
class Utf8String;
class VertexBuffer;
class VideoModeUtils;
class Window;
struct Clipboard;
struct ContextSettings;
struct GLVAOGroup;
struct Joystick;
struct Keyboard;
struct Mouse;
struct Sensor;
struct Touch;
struct WindowContextImpl;
using GlFunctionPointer = void (*)();
} // namespace za

struct TestContext;


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Process-wide window subsystem context
///
/// `WindowContext` is the central object that initializes
/// the underlying SDL/OS layer used by every other type in
/// the Window module. It must be created (via `create()`)
/// before any window, cursor, joystick, sensor, etc. is
/// accessed, and must outlive all of them.
///
/// Only one `WindowContext` can exist at a time. The simplest
/// pattern is to construct one in `main()` and let it live
/// until the program ends:
/// \code
/// int main()
/// {
///     auto windowContext = za::WindowContext::create().value();
///     // ... create windows, run main loop ...
/// }
/// \endcode
///
/// Most of the methods on `WindowContext` are private and
/// only accessible to friend classes inside Zancle; user code
/// will typically only call `create()` and `isInstalled()`.
///
////////////////////////////////////////////////////////////
class [[nodiscard]] ZA_WINDOW_API WindowContext
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Initialize the window subsystem and return a context
    ///
    /// Boots the underlying SDL layer (and any related OS
    /// resources). Returns `za::nullOpt` if initialization
    /// fails.
    ///
    /// \return The newly installed window context on success, `za::nullOpt` on failure
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<WindowContext> create();

    ////////////////////////////////////////////////////////////
    /// \private
    ///
    /// \brief Internal pass-key constructor
    ///
    /// Used by `create()` and by `za::GraphicsContext` to
    /// construct the singleton instance. Not callable from
    /// user code.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit WindowContext(za::PassKey<WindowContext>&&);
    [[nodiscard]] explicit WindowContext(za::PassKey<GraphicsContext>&&);

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    /// Tears down the window subsystem.
    ///
    ////////////////////////////////////////////////////////////
    ~WindowContext();

    // Deleted copy operations
    WindowContext(const WindowContext&)            = delete;
    WindowContext& operator=(const WindowContext&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Move constructor
    ///
    ////////////////////////////////////////////////////////////
    WindowContext(WindowContext&&) noexcept;

    // Deleted move assignment
    WindowContext& operator=(WindowContext&&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Check whether a window context is currently installed
    ///
    /// Installation happens by constructing a `WindowContext`
    /// (typically through `create()`).
    ///
    /// \return `true` if a window context is currently alive, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static bool isInstalled();

    ////////////////////////////////////////////////////////////
    /// \brief Get the currently active OpenGL context's ID
    ///
    /// The context ID is used to identify OpenGL contexts when
    /// managing unshareable OpenGL resources (e.g. VAOs and
    /// FBOs that cannot be shared between contexts).
    ///
    /// \return The active context's ID, or `0` if no context is currently active
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static unsigned int getActiveThreadLocalGlContextId();

    ////////////////////////////////////////////////////////////
    /// \brief Set the app ID for the compositor
    ///
    /// The app ID is used to identify the windows to the compositor
    /// for purposes of assigning icons, grouping and other user-defined rules
    /// depending on the platform.
    ///
    ////////////////////////////////////////////////////////////
    static void setAppId(const Utf8String& id);

    ////////////////////////////////////////////////////////////
    /// \brief Check whether a thread-local OpenGL context is currently active
    ///
    /// \return `true` if an OpenGL context is currently active on the
    ///         calling thread, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static bool hasActiveThreadLocalGlContext();

private:
    ////////////////////////////////////////////////////////////
    /// Friend declarations
    ///
    ////////////////////////////////////////////////////////////
    friend priv::EglContext;      // for `setActiveThreadLocalGlContext` and `cleanupUnsharedFrameBuffers`
    friend priv::SDLGlContext;    // for `cleanupUnsharedFrameBuffers`
    friend priv::JoystickManager; // for `getSDLLayer`
    friend priv::SDLWindowImpl;   // for `getJoystickManager` and `getSDLLayer`
    friend Clipboard;             // for `getSDLLayer`
    friend Cursor;                // for `getSDLLayer`
    friend GLVAOGroup;            // for `[un]registerUnsharedVAO`
    friend GraphicsContext;       // for `setActiveThreadLocalGlContext`
    friend Joystick;              // for `getJoystickManager`
    friend Keyboard;              // for `getSDLLayer`
    friend Mouse;                 // for `getSDLLayer`
    friend RenderTarget;          // for `getActiveThreadLocalGlContextId`
    friend RenderTexture;         // for `[un]registerUnsharedFrameBuffer`
    friend Sensor;                // for `getSensorManager`
    friend Shader;                // for `hasActiveThreadLocalGlContext`
    friend TestContext;           // for `createGlContext`
    friend Texture;               // for `hasActiveThreadLocalGlContext`
    friend Touch;                 // for `getSDLLayer`
    friend VertexBuffer;          // for `hasActiveThreadLocalGlContext`
    friend VideoModeUtils;        // for `getSDLLayer`
    friend Window;                // for `createGlContext`
    friend WindowContextImpl;

    ////////////////////////////////////////////////////////////
    /// \brief Create a new context, not associated to a window
    ///
    /// This function automatically chooses the specialized class
    /// to use according to the OS.
    ///
    /// \return Pointer to the created context
    ///
    ////////////////////////////////////////////////////////////
    template <typename... GLContextArgs>
    [[nodiscard]] static za::UniquePtr<priv::GlContext> createGlContextImpl(const ContextSettings& contextSettings,
                                                                            GLContextArgs&&... args);

    ////////////////////////////////////////////////////////////
    /// \brief Create a new context, not associated to a window
    ///
    /// This function automatically chooses the specialized class
    /// to use according to the OS.
    ///
    /// \return Pointer to the created context
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::UniquePtr<priv::GlContext> createGlContext(const ContextSettings& contextSettings);

    ////////////////////////////////////////////////////////////
    /// \brief Create a new context attached to a window
    ///
    /// This function automatically chooses the specialized class
    /// to use according to the OS.
    ///
    /// \param settings     Creation parameters
    /// \param owner        Pointer to the owner window
    /// \param bitsPerPixel Pixel depth (in bits per pixel)
    ///
    /// \return Pointer to the created context
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::UniquePtr<priv::GlContext> createGlContext(const ContextSettings&     contextSettings,
                                                                        const priv::SDLWindowImpl& owner,
                                                                        unsigned int               bitsPerPixel);

    ////////////////////////////////////////////////////////////
    /// \brief Notify unshared resources of context destruction
    ///
    ////////////////////////////////////////////////////////////
    static void cleanupUnsharedFrameBuffers(priv::GlContext& glContext);


    ////////////////////////////////////////////////////////////
    /// \brief Register a framebuffer object that cannot be shared between contexts
    ///
    /// Used by `za::RenderTexture` to track FBOs that need to
    /// be torn down when the owning OpenGL context is destroyed.
    ///
    ////////////////////////////////////////////////////////////
    static void registerUnsharedFrameBuffer(unsigned int glContextId, unsigned int frameBufferId);

    ////////////////////////////////////////////////////////////
    /// \brief Unregister a previously registered unshared FBO
    ///
    ////////////////////////////////////////////////////////////
    static void unregisterUnsharedFrameBuffer(unsigned int glContextId, unsigned int frameBufferId);

    ////////////////////////////////////////////////////////////
    /// \brief Register a VAO that cannot be shared between contexts
    ///
    /// Used by `za::GLVAOGroup` to track VAOs that need to be
    /// torn down when the owning OpenGL context is destroyed.
    ///
    ////////////////////////////////////////////////////////////
    static void registerUnsharedVAO(unsigned int glContextId, unsigned int vaoId);

    ////////////////////////////////////////////////////////////
    /// \brief Unregister a previously registered unshared VAO
    ///
    ////////////////////////////////////////////////////////////
    static void unregisterUnsharedVAO(unsigned int glContextId, unsigned int vaoId);

    ////////////////////////////////////////////////////////////
    /// \brief Check whether the given OpenGL context is the global shared context
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static bool isSharedContext(priv::GlContext& glContext);

    ////////////////////////////////////////////////////////////
    /// \brief Check whether a given OpenGL extension is available
    ///
    /// \param name Name of the extension to check for
    ///
    /// \return `true` if available, false if unavailable
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static bool isExtensionAvailable(const char* name);

    ////////////////////////////////////////////////////////////
    /// \brief Get the address of an OpenGL function
    ///
    /// On Windows when not using OpenGL ES, a context must be
    /// active for this function to succeed.
    ///
    /// \param name Name of the function to get the address of
    ///
    /// \return Address of the OpenGL function, 0 on failure
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static GlFunctionPointer getFunction(const char* name);

    ////////////////////////////////////////////////////////////
    /// \brief Activate or deactivate an OpenGL context on the current thread
    ///
    /// \return `true` on success, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static bool setActiveThreadLocalGlContext(priv::GlContext& glContext, bool active);

    ////////////////////////////////////////////////////////////
    /// \brief Make the global shared OpenGL context active on the current thread
    ///
    /// \return `true` on success, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static bool setActiveThreadLocalGlContextToSharedContext();

    ////////////////////////////////////////////////////////////
    /// \brief Check whether the currently active GL context is the shared context
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static bool isActiveGlContextSharedContext();

    ////////////////////////////////////////////////////////////
    /// \brief Deactivate the shared OpenGL context on the current thread, if any
    ///
    ////////////////////////////////////////////////////////////
    static void disableSharedGlContext();

    ////////////////////////////////////////////////////////////
    /// \brief Load OpenGL or OpenGL ES entry points using GLAD
    ///
    ////////////////////////////////////////////////////////////
    static void loadGLEntryPointsViaGLAD();

    ////////////////////////////////////////////////////////////
    /// \brief Return the currently active context or a null pointer if none is active
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static priv::GlContext* getActiveThreadLocalGlContextPtr();

    ////////////////////////////////////////////////////////////
    /// \brief Return a reference to the shared OpenGL context
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static priv::GlContext& getSharedGlContext();

    ////////////////////////////////////////////////////////////
    /// \brief Get the SDL layer instance
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static priv::SDLLayer& getSDLLayer();

    ////////////////////////////////////////////////////////////
    /// \brief Get the "global" joystick manager
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static priv::JoystickManager& getJoystickManager();

    ////////////////////////////////////////////////////////////
    /// \brief Get the "global" sensor manager
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static priv::SensorManager& getSensorManager();

    ////////////////////////////////////////////////////////////
    /// \brief Get a reusable scratch framebuffer for read operations
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static unsigned int getTransferScratchReadFramebuffer();

    ////////////////////////////////////////////////////////////
    /// \brief Get a reusable scratch framebuffer for draw operations
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static unsigned int getTransferScratchDrawFramebuffer();

    ////////////////////////////////////////////////////////////
    /// \brief Ensure the flip FBO has its scratch texture attached and is verified complete
    ///
    /// Like calling `getTransferScratchFlipFramebuffer` followed by
    /// `ensureTransferScratchFlipTexture` and a one-shot
    /// `glFramebufferTexture2D` + `glCheckFramebufferStatus`, but
    /// performs the attach + status check only on first use (or after
    /// the texture is recreated). Subsequent calls with the same
    /// `size` and `sRgb` return the cached FBO immediately, avoiding a
    /// per-frame `glCheckFramebufferStatus` (which forces a GPU
    /// pipeline sync on Chrome's WebGL implementation, ~1.6 ms per
    /// call).
    ///
    /// \return The flip FBO ID, or `0` on failure
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static unsigned int ensureTransferScratchFlipFramebufferReady(Vec2u size, bool sRgb);

    ////////////////////////////////////////////////////////////
    /// \brief Copy framebuffer contents with vertical flipping
    ///
    /// Copies source framebuffer contents to destination while vertically flipping
    /// the image. On OpenGL ES this uses a reusable intermediate texture/FBO pair,
    /// while on desktop OpenGL this is done via a direct blit with inverted coordinates.
    ///
    /// \param sRgb   Whether the scratch texture should use sRGB (only used for OpenGL ES)
    /// \param size   Dimensions of the region to copy
    /// \param srcFBO Source framebuffer ID
    /// \param dstFBO Destination framebuffer ID
    /// \param srcPos Source region starting position (default: 0,0)
    /// \param dstPos Destination region starting position (default: 0,0)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static bool copyFlippedFramebuffer(
        bool         sRgb,
        Vec2u        size,
        unsigned int srcFBO,
        unsigned int dstFBO,
        Vec2u        srcPos = {0u, 0u},
        Vec2u        dstPos = {0u, 0u});
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::WindowContext
/// \ingroup window
///
/// `za::WindowContext` is the lifetime root of Zancle's
/// window subsystem. Constructing one (via `create()`)
/// initializes SDL and any OS-level resources used by
/// windows, cursors, joysticks, sensors, the clipboard, and
/// the OpenGL context tracker. Destroying it tears all of
/// that down.
///
/// Only one `WindowContext` may exist at a time, and every
/// other Window-module API requires that one is alive when
/// it is called. The recommended pattern is to construct a
/// `WindowContext` early in `main()` and let it live until
/// the end of the program.
///
/// \see `za::Window`, `za::WindowBase`, `za::GraphicsContext`
///
////////////////////////////////////////////////////////////
