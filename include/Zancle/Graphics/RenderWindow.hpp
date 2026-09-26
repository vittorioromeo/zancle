#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Export.hpp"

#include "Zancle/Graphics/RenderTarget.hpp"

#include "Zancle/Window/ContextSettings.hpp"
#include "Zancle/Window/Window.hpp"
#include "Zancle/Window/WindowHandle.hpp"
#include "Zancle/Window/WindowSettings.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Vocabulary/Optional.hpp"
#include "Zancle/Vocabulary/PassKey.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
class Image;
} // namespace za


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Window that can serve as a target for 2D drawing
///
////////////////////////////////////////////////////////////
class ZA_GRAPHICS_API RenderWindow : public Window, public RenderTarget
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Window creation settings
    ///
    ////////////////////////////////////////////////////////////
    using Settings = WindowSettings;

    ////////////////////////////////////////////////////////////
    /// \brief Create a new render window
    ///
    /// Creates the render window described by `windowSettings`
    /// (size, title, style, context settings, ...). The OS window
    /// is opened immediately and an associated OpenGL context is
    /// created and made current.
    ///
    /// \param windowSettings Window creation parameters
    ///
    /// \return Render window on success, `za::nullOpt` on failure
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<RenderWindow> create(const Settings& windowSettings);

    ////////////////////////////////////////////////////////////
    /// \brief Construct the window from an existing control
    ///
    /// Use this constructor if you want to create a Zancle
    /// rendering area into an already existing control.
    ///
    /// The second parameter is an optional structure specifying
    /// advanced OpenGL context settings such as anti-aliasing,
    /// depth-buffer bits, etc. You shouldn't care about these
    /// parameters for a regular usage of the graphics module.
    ///
    /// \param handle   Platform-specific handle of the control (\a HWND on
    ///                 Windows, \a %Window on Linux/FreeBSD, \a NSWindow on macOS)
    /// \param settings Additional settings for the underlying OpenGL context
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<RenderWindow> create(WindowHandle handle, const ContextSettings& contextSettings = {});

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy constructor
    ///
    ////////////////////////////////////////////////////////////
    RenderWindow(const RenderWindow&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy assignment
    ///
    ////////////////////////////////////////////////////////////
    RenderWindow& operator=(const RenderWindow&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Move constructor
    ///
    ////////////////////////////////////////////////////////////
    RenderWindow(RenderWindow&&) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Move assignment
    ///
    ////////////////////////////////////////////////////////////
    RenderWindow& operator=(RenderWindow&&) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    /// Closes the underlying OS window and releases the
    /// associated OpenGL context.
    ///
    ////////////////////////////////////////////////////////////
    ~RenderWindow() override;

    ////////////////////////////////////////////////////////////
    /// \brief Get the size of the rendering region of the window
    ///
    /// The size doesn't include the titlebar and borders
    /// of the window.
    ///
    /// \return Size in pixels
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Vec2u getSize() const override;

    ////////////////////////////////////////////////////////////
    /// \brief Change the window's icon
    ///
    /// The OS default icon is used by default.
    ///
    /// \param icon Image to use as the icon. The image is copied,
    ///             so you need not keep the source alive after
    ///             calling this function.
    ///
    ////////////////////////////////////////////////////////////
    void setIcon(const Image& icon);
    using Window::setIcon;

    ////////////////////////////////////////////////////////////
    /// \brief Activate or deactivate the window as the current target
    ///        for OpenGL rendering
    ///
    /// A window is active only on the current thread, if you want to
    /// make it active on another thread you have to deactivate it
    /// on the previous thread first if it was active.
    /// Only one window can be active on a thread at a time, thus
    /// the window previously active (if any) automatically gets deactivated.
    /// This is not to be confused with `requestFocus()`.
    ///
    /// \param active `true` to activate, `false` to deactivate
    ///
    /// \return `true` if operation was successful, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool setActive(bool active = true) override;

    ////////////////////////////////////////////////////////////
    /// \brief Display the rendered frame on screen
    ///
    /// Flushes any pending auto-batched draw calls, swaps the
    /// front and back buffers of the underlying window, and
    /// returns statistics describing the draw calls issued during
    /// the current frame -- the counters are reset by `display`
    /// itself, so each call reports exactly one frame's work.
    /// Call this exactly once at the end of every frame.
    ///
    /// \return Per-frame draw call and vertex statistics
    ///
    ////////////////////////////////////////////////////////////
    RenderTarget::DrawStatistics display();

    ////////////////////////////////////////////////////////////
    /// \private
    ///
    /// \brief Construct a new window
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit RenderWindow(za::PassKey<RenderWindow>&&, Window&& window);

private:
    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    unsigned int m_defaultFrameBuffer{}; //!< Framebuffer to bind when targeting this window
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::RenderWindow
/// \ingroup graphics
///
/// `za::RenderWindow` is the main class of the Graphics module.
/// It defines an OS window that can be painted using the other
/// classes of the graphics module.
///
/// `za::RenderWindow` is derived from `za::Window`, thus it inherits
/// all its features: events, window management, OpenGL rendering,
/// etc. See the documentation of `za::Window` for a more complete
/// description of all these features, as well as code examples.
///
/// On top of that, `za::RenderWindow` adds more features related to
/// 2D drawing with the graphics module (see its base class
/// `za::RenderTarget` for more details).
/// Here is a typical rendering and event loop with a
/// `za::RenderWindow`:
///
/// \code
/// // Declare and create a new render window.
/// auto window = za::RenderWindow::create({.size = {800u, 600u}, .title = "Zancle Window"}).value();
///
/// // Limit the framerate to 60 frames per second (optional).
/// window.setFramerateLimit(60u);
///
/// // The main loop -- ends as soon as the window is closed.
/// while (true)
/// {
///     // Event processing.
///     while (const za::Optional event = window.pollEvent())
///     {
///         // Request for closing the window.
///         if (event->is<za::Event::Closed>())
///             return 0; // break out of both loops
///     }
///
///     // Clear the whole window before rendering a new frame.
///     window.clear();
///
///     // Draw some graphical entities.
///     window.draw(sprite, {.texture = &texture}); // texture passed at draw time in Zancle
///     window.draw(circle);
///     window.draw(text);
///
///     // End the current frame and display its contents on screen.
///     window.display();
/// }
/// \endcode
///
/// Like `za::Window`, `za::RenderWindow` is still able to render
/// direct OpenGL geometry. It is even possible to mix together
/// custom OpenGL calls (using OpenGL ES 3.1 only -- the legacy
/// fixed-function pipeline is not available in Zancle) and
/// regular Zancle drawing commands. When doing so, call
/// `resetGLStates` between the two so that Zancle can re-establish
/// the OpenGL state it expects.
///
/// \code
/// // Create the render window.
/// auto window = za::RenderWindow::create({.size = {800u, 600u}, .title = "Zancle OpenGL"}).value();
///
/// // Load some assets to display.
/// const auto       texture = za::Texture::loadFromFile("circle.png").value();
/// const za::Sprite sprite{.textureRect = texture.getRect()};
///
/// const auto     font = za::Font::openFromFile("arial.ttf").value();
/// const za::Text text{font, {.string = "Hello", .characterSize = 32u}};
///
/// // Perform OpenGL initializations (modern OpenGL only).
/// // ...
///
/// // Start the rendering loop.
/// while (true)
/// {
///     // Process events
///     // ...
///
///     // Draw a background sprite.
///     window.resetGLStates();
///     window.draw(sprite, {.texture = &texture});
///
///     // Draw raw OpenGL ES 3.1 geometry here.
///     // ...
///
///     // Draw text on top of the OpenGL geometry.
///     window.resetGLStates();
///     window.draw(text);
///
///     // Display the rendered frame on screen.
///     window.display();
/// }
/// \endcode
///
/// \see `za::Window`, `za::RenderTarget`, `za::RenderTexture`, `za::View`
///
////////////////////////////////////////////////////////////
