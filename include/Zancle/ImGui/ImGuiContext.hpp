#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/ImGui/Export.hpp"

#include "Zancle/Graphics/Color.hpp"

#include "Zancle/Window/Joystick.hpp"

#include "Zancle/Chrono/Time.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Vocabulary/UniquePtr.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
struct ImFontAtlas;

namespace za
{
class Event;
class RenderTarget;
class RenderTexture;
class RenderWindow;
class Texture;
class Window;
struct Sprite;
} // namespace za


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Per-window Dear ImGui context bound to an Zancle rendering pipeline
///
////////////////////////////////////////////////////////////
class ZA_IMGUI_API ImGuiContext
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Construct a new ImGui context and initialize its OpenGL backend
    ///
    /// Creates the underlying Dear ImGui context, sets it as the current
    /// one, initializes the bundled OpenGL3 renderer backend, loads the
    /// system mouse cursors, and applies a sensible default joystick
    /// mapping (face buttons, shoulders, sticks and triggers).
    ///
    /// A graphics context (`za::GraphicsContext`) must already exist
    /// before constructing an `ImGuiContext`, otherwise the OpenGL
    /// backend cannot be initialized.
    ///
    /// One `ImGuiContext` instance should generally be used per window;
    /// switching between contexts is handled automatically by the
    /// member functions of this class.
    ///
    /// This overload creates a context that owns its own font atlas.
    ///
    /// \param loadDefaultFont If `true`, load the default Dear ImGui font
    ///                        into the owned atlas. Set to `false` if you
    ///                        intend to load your own fonts manually via
    ///                        `ImGui::GetIO().Fonts`.
    ///
    ////////////////////////////////////////////////////////////
    explicit ImGuiContext(bool loadDefaultFont = true);

    ////////////////////////////////////////////////////////////
    /// \brief Create a context that drives a caller-owned shared font atlas
    ///
    /// Use this factory to construct the **first** `ImGuiContext` for a
    /// given shared atlas: this context is the one that runs ImGui's
    /// per-frame atlas updates (`ImFontAtlasUpdateNewFrame`).
    ///
    /// The caller owns the atlas and is responsible for populating it
    /// (e.g. `atlas.AddFontDefault()`) **before** calling this factory.
    /// The factory does **not** add the default font and does **not**
    /// transfer ownership of the atlas; the atlas must outlive every
    /// `ImGuiContext` that references it.
    ///
    /// Pair with `createSharingAtlas` for additional contexts on the
    /// same atlas. Exactly one driving context per atlas is allowed:
    /// constructing a second driving context for the same atlas
    /// triggers an assertion. The driving context's `update()` must
    /// be called before any sharing context's in each frame.
    ///
    /// \param atlas Externally-owned, pre-populated font atlas. Must
    ///              not already have a driver assigned.
    ///
    /// \return A driving context bound to the supplied atlas.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static ImGuiContext createOwningAtlas(ImFontAtlas& atlas);

    ////////////////////////////////////////////////////////////
    /// \brief Create a context that consumes a shared font atlas driven by another context
    ///
    /// Use this factory to construct **additional** `ImGuiContext`
    /// instances after a driving context has been created via
    /// `createOwningAtlas`. The sharing context inherits the driver's
    /// font atlas state and (once the driver has rendered once)
    /// adopts the same GL font texture, saving ~1 MB of GPU memory
    /// per extra context.
    ///
    /// The atlas must already have a driving context; calling this
    /// factory before `createOwningAtlas` triggers an assertion. The
    /// driving context must outlive every sharing context that
    /// references the same atlas.
    ///
    /// \param atlas Externally-owned, already-driven font atlas.
    ///
    /// \return A sharing context bound to the supplied atlas.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static ImGuiContext createSharingAtlas(ImFontAtlas& atlas);

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    /// Shuts down the OpenGL backend and destroys the underlying
    /// Dear ImGui context.
    ///
    ////////////////////////////////////////////////////////////
    ~ImGuiContext();

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy constructor
    ///
    ////////////////////////////////////////////////////////////
    ImGuiContext(const ImGuiContext&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Deleted copy assignment
    ///
    ////////////////////////////////////////////////////////////
    ImGuiContext& operator=(const ImGuiContext&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Move constructor
    ///
    ////////////////////////////////////////////////////////////
    ImGuiContext(ImGuiContext&&) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Move assignment
    ///
    ////////////////////////////////////////////////////////////
    ImGuiContext& operator=(ImGuiContext&&) noexcept;

    ////////////////////////////////////////////////////////////
    /// \brief Make this context's underlying Dear ImGui context the current one
    ///
    /// Equivalent to calling `ImGui::SetCurrentContext` on the
    /// underlying Dear ImGui context. The other member functions of
    /// this class call this automatically before interacting with
    /// Dear ImGui, so it only needs to be invoked manually before
    /// direct `ImGui::*` calls when more than one `za::ImGuiContext`
    /// instance is alive.
    ///
    ////////////////////////////////////////////////////////////
    void setCurrent();

    ////////////////////////////////////////////////////////////
    /// \brief Check whether this context's underlying Dear ImGui context is the current one
    ///
    /// \return `true` if `ImGui::GetCurrentContext()` matches the
    ///         underlying Dear ImGui context, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isCurrent() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the underlying Dear ImGui context
    ///
    /// Returns the opaque `ImGuiContext*` owned by this instance, for
    /// interoperability with code that needs to call Dear ImGui APIs
    /// taking an explicit context (e.g. `ImGui::SetCurrentContext`).
    /// The returned pointer is owned by this instance and remains
    /// valid until the `za::ImGuiContext` is destroyed.
    ///
    /// \return Pointer to the underlying `ImGuiContext`, never null
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] void* getNativeImGuiContext();

    ////////////////////////////////////////////////////////////
    /// \brief Get the underlying Dear ImGui context (const overload)
    ///
    /// \return Pointer to the underlying `ImGuiContext`, never null
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const void* getNativeImGuiContext() const;

    ////////////////////////////////////////////////////////////
    /// \brief Set which joystick provides input to ImGui's gamepad navigation
    ///
    /// By default, the first connected joystick (if any) is used.
    /// When the active joystick is disconnected, the next connected
    /// joystick is selected automatically.
    ///
    /// \param joystickId Index of the joystick to use, in `[0, Joystick::MaxCount)`
    ///
    ////////////////////////////////////////////////////////////
    void setActiveJoystickId(unsigned int joystickId);

    ////////////////////////////////////////////////////////////
    /// \brief Set the activation threshold for the gamepad D-pad axes
    ///
    /// The threshold defines how far the configured D-pad axis must be
    /// pushed away from the center before the corresponding ImGui
    /// gamepad direction is considered pressed.
    ///
    /// \param threshold Threshold in `[0, 100]`
    ///
    ////////////////////////////////////////////////////////////
    void setJoystickDPadThreshold(float threshold);

    ////////////////////////////////////////////////////////////
    /// \brief Set the activation threshold for the left analog stick axes
    ///
    /// \param threshold Threshold in `[0, 100]`
    ///
    ////////////////////////////////////////////////////////////
    void setJoystickLStickThreshold(float threshold);

    ////////////////////////////////////////////////////////////
    /// \brief Set the activation threshold for the right analog stick axes
    ///
    /// \param threshold Threshold in `[0, 100]`
    ///
    ////////////////////////////////////////////////////////////
    void setJoystickRStickThreshold(float threshold);

    ////////////////////////////////////////////////////////////
    /// \brief Set the activation threshold for the left trigger
    ///
    /// Some drivers report triggers in `[-100, 100]` (`-100` = released),
    /// hence the wider range compared to the stick thresholds.
    ///
    /// \param threshold Threshold in `[-100, 100]`
    ///
    ////////////////////////////////////////////////////////////
    void setJoystickLTriggerThreshold(float threshold);

    ////////////////////////////////////////////////////////////
    /// \brief Set the activation threshold for the right trigger
    ///
    /// \param threshold Threshold in `[-100, 100]`
    ///
    ////////////////////////////////////////////////////////////
    void setJoystickRTriggerThreshold(float threshold);

    ////////////////////////////////////////////////////////////
    /// \brief Map a joystick button to an ImGui gamepad key
    ///
    /// Used to translate physical joystick button indices into the
    /// virtual `ImGuiKey_Gamepad*` keys that drive ImGui's gamepad
    /// navigation.
    ///
    /// \param key            Target ImGui key, must be a named key
    ///                       (`ImGuiKey_NamedKey_BEGIN <= key < ImGuiKey_NamedKey_END`)
    /// \param joystickButton Source joystick button index, in `[0, Joystick::ButtonCount)`
    ///
    ////////////////////////////////////////////////////////////
    void setJoystickMapping(int key, unsigned int joystickButton);

    ////////////////////////////////////////////////////////////
    /// \brief Configure the joystick axis used for the D-pad X direction
    ///
    /// \param dPadXAxis Joystick axis to use
    /// \param inverted  If `true`, the axis sign is flipped before threshold testing
    ///
    ////////////////////////////////////////////////////////////
    void setDPadXAxis(Joystick::Axis dPadXAxis, bool inverted = false);

    ////////////////////////////////////////////////////////////
    /// \brief Configure the joystick axis used for the D-pad Y direction
    ///
    /// \param dPadYAxis Joystick axis to use
    /// \param inverted  If `true`, the axis sign is flipped before threshold testing
    ///
    ////////////////////////////////////////////////////////////
    void setDPadYAxis(Joystick::Axis dPadYAxis, bool inverted = false);

    ////////////////////////////////////////////////////////////
    /// \brief Configure the joystick axis used for the left stick X direction
    ///
    /// \param lStickXAxis Joystick axis to use
    /// \param inverted    If `true`, the axis sign is flipped before threshold testing
    ///
    ////////////////////////////////////////////////////////////
    void setLStickXAxis(Joystick::Axis lStickXAxis, bool inverted = false);

    ////////////////////////////////////////////////////////////
    /// \brief Configure the joystick axis used for the left stick Y direction
    ///
    /// \param lStickYAxis Joystick axis to use
    /// \param inverted    If `true`, the axis sign is flipped before threshold testing
    ///
    ////////////////////////////////////////////////////////////
    void setLStickYAxis(Joystick::Axis lStickYAxis, bool inverted = false);

    ////////////////////////////////////////////////////////////
    /// \brief Configure the joystick axis used for the right stick X direction
    ///
    /// \param rStickXAxis Joystick axis to use
    /// \param inverted    If `true`, the axis sign is flipped before threshold testing
    ///
    ////////////////////////////////////////////////////////////
    void setRStickXAxis(Joystick::Axis rStickXAxis, bool inverted = false);

    ////////////////////////////////////////////////////////////
    /// \brief Configure the joystick axis used for the right stick Y direction
    ///
    /// \param rStickYAxis Joystick axis to use
    /// \param inverted    If `true`, the axis sign is flipped before threshold testing
    ///
    ////////////////////////////////////////////////////////////
    void setRStickYAxis(Joystick::Axis rStickYAxis, bool inverted = false);

    ////////////////////////////////////////////////////////////
    /// \brief Configure the joystick axis used for the left trigger
    ///
    /// \param lTriggerAxis Joystick axis to use
    ///
    ////////////////////////////////////////////////////////////
    void setLTriggerAxis(Joystick::Axis lTriggerAxis);

    ////////////////////////////////////////////////////////////
    /// \brief Configure the joystick axis used for the right trigger
    ///
    /// \param rTriggerAxis Joystick axis to use
    ///
    ////////////////////////////////////////////////////////////
    void setRTriggerAxis(Joystick::Axis rTriggerAxis);

    ////////////////////////////////////////////////////////////
    /// \brief Forward an Zancle event to the ImGui context
    ///
    /// Translates Zancle window, mouse, keyboard, text, touch and
    /// joystick events into the corresponding ImGui input state.
    /// Call this from inside the event polling loop, before
    /// dispatching the event to your own input handlers.
    ///
    /// \param window Window the event originates from
    /// \param event  Event to translate and forward
    ///
    ////////////////////////////////////////////////////////////
    void processEvent(const Window& window, const Event& event);

    ////////////////////////////////////////////////////////////
    /// \brief Synchronize the OS mouse cursor with the cursor requested by ImGui
    ///
    /// Updates the window's hardware cursor based on what ImGui
    /// currently wants to display (arrow, text caret, resize, etc.).
    /// Hidden when ImGui is drawing its own software cursor or when
    /// the cursor type cannot be loaded.
    ///
    /// This is called automatically by `update(Window&, RenderTarget&, Time)`
    /// and `update(RenderWindow&, Time)`; it only needs to be invoked
    /// manually when using the low-level `update(Vec2i, Vec2f, Time)` overload.
    ///
    /// \param window Window whose cursor should be updated
    ///
    ////////////////////////////////////////////////////////////
    void updateMouseCursor(Window& window);

    ////////////////////////////////////////////////////////////
    /// \brief Begin a new ImGui frame for a render window
    ///
    /// Convenience overload that uses `renderWindow` as both the
    /// input window and the render target. Equivalent to calling
    /// `update(renderWindow, renderWindow, dt)`.
    ///
    /// Must be called once per frame, after `processEvent` has been
    /// called for every pending event and before any `ImGui::*`
    /// widget calls.
    ///
    /// \param renderWindow Render window providing both input and target size
    /// \param dt           Time elapsed since the previous call
    ///
    ////////////////////////////////////////////////////////////
    void update(RenderWindow& renderWindow, Time dt);

    ////////////////////////////////////////////////////////////
    /// \brief Begin a new ImGui frame for a window/target pair
    ///
    /// Updates the mouse cursor, polls real-time mouse and gamepad
    /// state, sets the ImGui display size to match `renderTarget`,
    /// and starts a new ImGui frame.
    ///
    /// Use this overload when the input window and render target
    /// differ (for example, rendering ImGui to an off-screen
    /// `za::RenderTexture`).
    ///
    /// \param window       Window providing input (mouse position, focus, etc.)
    /// \param renderTarget Render target whose size becomes the ImGui display size
    /// \param dt           Time elapsed since the previous call
    ///
    ////////////////////////////////////////////////////////////
    void update(Window& window, RenderTarget& renderTarget, Time dt);

    ////////////////////////////////////////////////////////////
    /// \brief Begin a new ImGui frame from raw values
    ///
    /// Low-level overload that does not touch any window. Use this
    /// when there is no `za::Window` available, or to drive ImGui
    /// in a fully custom way. The caller is responsible for
    /// computing the mouse position relative to the display and for
    /// calling `updateMouseCursor` if a hardware cursor is desired.
    ///
    /// \param mousePos    Mouse position in display-space pixels
    /// \param displaySize Display size in pixels
    /// \param dt          Time elapsed since the previous call
    ///
    ////////////////////////////////////////////////////////////
    void update(Vec2i mousePos, Vec2f displaySize, Time dt);

    ////////////////////////////////////////////////////////////
    /// \brief Render the current ImGui frame to a render window
    ///
    /// Convenience overload, equivalent to `render(static_cast<RenderTarget&>(window))`.
    ///
    /// \param window Render window to draw the ImGui frame into
    ///
    ////////////////////////////////////////////////////////////
    void render(RenderWindow& window);

    ////////////////////////////////////////////////////////////
    /// \brief Render the current ImGui frame to a render target
    ///
    /// Finalizes the ImGui frame (`ImGui::Render()`) and submits its
    /// draw data to the bundled OpenGL backend. Call this after all
    /// `ImGui::*` widget calls for the current frame and before
    /// `window.display()`.
    ///
    /// The current OpenGL texture and shader bindings of the target
    /// are saved and restored around the call so that ImGui rendering
    /// does not disturb subsequent Zancle draw calls.
    ///
    /// \param target Render target to draw the ImGui frame into
    ///
    ////////////////////////////////////////////////////////////
    void render(RenderTarget& target);

    ////////////////////////////////////////////////////////////
    /// \brief Embed an Zancle texture as an ImGui image widget
    ///
    /// Inserts an `ImGui::Image` widget into the current ImGui
    /// window using `texture` at its native size.
    ///
    /// \param texture     Texture to display
    /// \param tintColor   Color multiplied with the texture sampling
    /// \param borderColor Border color drawn around the image
    ///
    ////////////////////////////////////////////////////////////
    void image(const Texture& texture, Color tintColor = Color::White, Color borderColor = Color::Transparent);

    ////////////////////////////////////////////////////////////
    /// \brief Embed an Zancle texture as an ImGui image widget with a custom size
    ///
    /// \param texture     Texture to display
    /// \param size        Display size of the widget, in pixels
    /// \param tintColor   Color multiplied with the texture sampling
    /// \param borderColor Border color drawn around the image
    ///
    ////////////////////////////////////////////////////////////
    void image(const Texture& texture, Vec2f size, Color tintColor = Color::White, Color borderColor = Color::Transparent);

    ////////////////////////////////////////////////////////////
    /// \brief Embed an Zancle render texture as an ImGui image widget
    ///
    /// \param texture     Render texture to display, at its native size
    /// \param tintColor   Color multiplied with the texture sampling
    /// \param borderColor Border color drawn around the image
    ///
    ////////////////////////////////////////////////////////////
    void image(const RenderTexture& texture, Color tintColor = Color::White, Color borderColor = Color::Transparent);

    ////////////////////////////////////////////////////////////
    /// \brief Embed an Zancle render texture as an ImGui image widget with a custom size
    ///
    /// \param texture     Render texture to display
    /// \param size        Display size of the widget, in pixels
    /// \param tintColor   Color multiplied with the texture sampling
    /// \param borderColor Border color drawn around the image
    ///
    ////////////////////////////////////////////////////////////
    void image(const RenderTexture& texture, Vec2f size, Color tintColor = Color::White, Color borderColor = Color::Transparent);

    ////////////////////////////////////////////////////////////
    /// \brief Embed a sub-region of an Zancle texture as an ImGui image widget
    ///
    /// Uses `sprite.textureRect` to compute UV coordinates so that
    /// only the sub-region described by the sprite is shown. The
    /// widget is sized to match the sprite's global bounds.
    ///
    /// \param sprite      Sprite providing the texture sub-region (and size)
    /// \param texture     Texture the sprite samples from
    /// \param tintColor   Color multiplied with the texture sampling
    /// \param borderColor Border color drawn around the image
    ///
    ////////////////////////////////////////////////////////////
    void image(const Sprite&  sprite,
               const Texture& texture,
               Color          tintColor   = Color::White,
               Color          borderColor = Color::Transparent);

    ////////////////////////////////////////////////////////////
    /// \brief Embed a sub-region of an Zancle texture as an ImGui image widget with a custom size
    ///
    /// \param sprite      Sprite providing the texture sub-region
    /// \param texture     Texture the sprite samples from
    /// \param size        Display size of the widget, in pixels
    /// \param tintColor   Color multiplied with the texture sampling
    /// \param borderColor Border color drawn around the image
    ///
    ////////////////////////////////////////////////////////////
    void image(const Sprite&  sprite,
               const Texture& texture,
               Vec2f          size,
               Color          tintColor   = Color::White,
               Color          borderColor = Color::Transparent);

    ////////////////////////////////////////////////////////////
    /// \brief Create a clickable ImGui image button from an Zancle texture
    ///
    /// \param id        ImGui identifier string for the button
    /// \param texture   Texture used as the button's image
    /// \param size      Display size of the button, in pixels
    /// \param bgColor   Background color drawn behind the image
    /// \param tintColor Color multiplied with the texture sampling
    ///
    /// \return `true` if the button was clicked this frame, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool imageButton(const char*    id,
                                   const Texture& texture,
                                   Vec2f          size,
                                   Color          bgColor   = Color::Transparent,
                                   Color          tintColor = Color::White);

    ////////////////////////////////////////////////////////////
    /// \brief Create a clickable ImGui image button from an Zancle render texture
    ///
    /// \param id        ImGui identifier string for the button
    /// \param texture   Render texture used as the button's image
    /// \param size      Display size of the button, in pixels
    /// \param bgColor   Background color drawn behind the image
    /// \param tintColor Color multiplied with the texture sampling
    ///
    /// \return `true` if the button was clicked this frame, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool imageButton(const char*          id,
                                   const RenderTexture& texture,
                                   Vec2f                size,
                                   Color                bgColor   = Color::Transparent,
                                   Color                tintColor = Color::White);

    ////////////////////////////////////////////////////////////
    /// \brief Create a clickable ImGui image button from a sub-region of an Zancle texture
    ///
    /// Uses `sprite.textureRect` to compute UV coordinates so that
    /// only the sub-region described by the sprite is shown.
    ///
    /// \param id        ImGui identifier string for the button
    /// \param sprite    Sprite providing the texture sub-region
    /// \param texture   Texture the sprite samples from
    /// \param size      Display size of the button, in pixels
    /// \param bgColor   Background color drawn behind the image
    /// \param tintColor Color multiplied with the texture sampling
    ///
    /// \return `true` if the button was clicked this frame, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool imageButton(const char*    id,
                                   const Sprite&  sprite,
                                   const Texture& texture,
                                   Vec2f          size,
                                   Color          bgColor   = Color::Transparent,
                                   Color          tintColor = Color::White);

    ////////////////////////////////////////////////////////////
    /// \brief Draw a line on the current ImGui window's draw list
    ///
    /// Both endpoints are interpreted relative to the current ImGui
    /// cursor screen position, so the line moves with the surrounding
    /// layout in the same way an `ImGui::Image` would.
    ///
    /// \param a         First endpoint, relative to the cursor position
    /// \param b         Second endpoint, relative to the cursor position
    /// \param col       Line color
    /// \param thickness Line thickness in pixels
    ///
    ////////////////////////////////////////////////////////////
    void drawLine(Vec2f a, Vec2f b, Color col, float thickness = 1.f);

    ////////////////////////////////////////////////////////////
    /// \brief Draw a rectangle outline on the current ImGui window's draw list
    ///
    /// `rect` is interpreted relative to the current ImGui cursor
    /// screen position.
    ///
    /// \param rect            Rectangle to draw, relative to the cursor position
    /// \param color           Outline color
    /// \param rounding        Corner rounding radius in pixels (`0` for sharp corners)
    /// \param roundingCorners Bitmask of `ImDrawFlags_RoundCorners*` flags
    ///                        selecting which corners to round (defaults to all four)
    /// \param thickness       Outline thickness in pixels
    ///
    ////////////////////////////////////////////////////////////
    void drawRect(const Rect2f& rect, Color color, float rounding = 0.f, int roundingCorners = 0x0F, float thickness = 1.f);

    ////////////////////////////////////////////////////////////
    /// \brief Draw a filled rectangle on the current ImGui window's draw list
    ///
    /// `rect` is interpreted relative to the current ImGui cursor
    /// screen position.
    ///
    /// \param rect            Rectangle to draw, relative to the cursor position
    /// \param color           Fill color
    /// \param rounding        Corner rounding radius in pixels (`0` for sharp corners)
    /// \param roundingCorners Bitmask of `ImDrawFlags_RoundCorners*` flags
    ///                        selecting which corners to round (defaults to all four)
    ///
    ////////////////////////////////////////////////////////////
    void drawRectFilled(const Rect2f& rect, Color color, float rounding = 0.f, int roundingCorners = 0x0F);

private:
    ////////////////////////////////////////////////////////////
    /// \brief Internal constructor used by the shared-atlas static factories
    ///
    /// `claimOwnership` selects between the driver / consumer roles for
    /// the supplied shared atlas (see `createOwningAtlas` /
    /// `createSharingAtlas` for the semantics).
    ///
    ////////////////////////////////////////////////////////////
    explicit ImGuiContext(ImFontAtlas& sharedFontAtlas, bool claimOwnership);

    ////////////////////////////////////////////////////////////
    /// \brief Apply a sensible default joystick mapping
    ///
    /// Maps standard face buttons, shoulders, sticks and triggers to
    /// the `ImGuiKey_Gamepad*` keys, and configures default axis
    /// thresholds. Called from the constructor.
    ///
    ////////////////////////////////////////////////////////////
    void initDefaultJoystickMapping();

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    struct Impl;
    za::UniquePtr<Impl> m_impl; //!< Implementation details
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::ImGuiContext
/// \ingroup imgui
///
/// `za::ImGuiContext` is the main entry point of the ImGui module.
/// It owns a Dear ImGui context together with the bundled OpenGL3
/// renderer backend, and exposes the small set of glue functions
/// needed to drive ImGui from a Zancle application: forwarding
/// events, starting a new frame, rendering, and a few helpers to
/// embed Zancle textures and sprites into ImGui windows.
///
/// One `za::ImGuiContext` instance should generally be created per
/// `za::RenderWindow`. Multiple instances are supported (see the
/// `imgui_multiple_windows` example): the class makes the matching
/// underlying ImGui context current on every member call, so users
/// do not need to call `ImGui::SetCurrentContext` themselves when
/// switching between windows.
///
/// A `za::GraphicsContext` must already exist before constructing
/// an `ImGuiContext`, otherwise the OpenGL backend cannot be
/// initialized.
///
/// Typical usage is a four-step loop:
/// 1. Forward each polled event with `processEvent`.
/// 2. Start a new frame with `update`, passing the elapsed time.
/// 3. Issue any `ImGui::*` widget calls.
/// 4. Render the frame with `render`, then call `window.display()`.
///
/// Usage example:
/// \code
/// auto graphicsContext = za::GraphicsContext::create().value();
/// auto window          = za::RenderWindow::create({.size{1024u, 768u}, .title = "ImGui + Zancle"}).value();
///
/// za::ImGuiContext imGuiContext;
/// za::Clock        deltaClock;
///
/// while (true)
/// {
///     while (const za::Optional event = window.pollEvent())
///     {
///         imGuiContext.processEvent(window, *event);
///
///         if (za::EventUtils::isClosedOrEscapeKeyPressed(*event))
///             return 0;
///     }
///
///     imGuiContext.update(window, deltaClock.restart());
///
///     ImGui::Begin("Hello, world!");
///     ImGui::Button("Look at this pretty button");
///     ImGui::End();
///
///     window.clear();
///     imGuiContext.render(window);
///     window.display();
/// }
/// \endcode
///
/// \see `za::Event`, `za::RenderWindow`, `za::GraphicsContext`
///
////////////////////////////////////////////////////////////
