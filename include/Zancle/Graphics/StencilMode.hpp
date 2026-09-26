#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Export.hpp"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"

namespace za
{
////////////////////////////////////////////////////////
/// \brief Enumeration of the stencil test comparisons that can be performed
///
/// The comparisons are mapped directly to their OpenGL equivalents,
/// specified by `glStencilFunc()`.
///
////////////////////////////////////////////////////////
enum class [[nodiscard]] StencilComparison : unsigned char
{
    Never = 0u,     //!< The stencil test never passes
    Less  = 1u,     //!< The stencil test passes if the new value is less than the value in the stencil buffer
    LessEqual = 2u, //!< The stencil test passes if the new value is less than or equal to the value in the stencil buffer
    Greater = 3u,   //!< The stencil test passes if the new value is greater than the value in the stencil buffer
    GreaterEqual = 4u, //!< The stencil test passes if the new value is greater than or equal to the value in the stencil buffer
    Equal    = 5u, //!< The stencil test passes if the new value is strictly equal to the value in the stencil buffer
    NotEqual = 6u, //!< The stencil test passes if the new value is strictly unequal to the value in the stencil buffer
    Always   = 7u  //!< The stencil test always passes
};

////////////////////////////////////////////////////////
/// \brief Enumeration of the stencil buffer update operations
///
/// The update operations are mapped directly to their OpenGL equivalents,
/// specified by `glStencilOp()`.
///
////////////////////////////////////////////////////////
enum class [[nodiscard]] StencilUpdateOperation : unsigned char
{
    Keep    = 0u, //!< If the stencil test passes, the value in the stencil buffer is not modified
    Zero    = 1u, //!< If the stencil test passes, the value in the stencil buffer is set to zero
    Replace = 2u, //!< If the stencil test passes, the value in the stencil buffer is set to the new value
    Increment = 3u, //!< If the stencil test passes, the value in the stencil buffer is incremented and if required clamped
    Decrement = 4u, //!< If the stencil test passes, the value in the stencil buffer is decremented and if required clamped
    Invert = 5u     //!< If the stencil test passes, the value in the stencil buffer is bitwise inverted
};

////////////////////////////////////////////////////////
/// \brief Stencil value type (also used as a mask)
///
////////////////////////////////////////////////////////
struct [[nodiscard]] ZA_GRAPHICS_API StencilValue
{
    ////////////////////////////////////////////////////////////
    /// \brief Construct a stencil value from an unsigned integer
    ///
    /// \param theValue Unsigned integer value to use
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] constexpr explicit StencilValue(unsigned int theValue) : value(theValue)
    {
    }

    ////////////////////////////////////////////////////////////
    /// \brief Disable construction from any other type
    ///
    ////////////////////////////////////////////////////////////
    template <typename T>
    StencilValue(T) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Overload of the `operator==`
    ///
    /// \param rhs Right operand
    ///
    /// \return `true` if stencil values are equal, `false` if they are different
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] ZA_GRAPHICS_API constexpr bool operator==(const StencilValue& rhs) const = default;

    unsigned int value{}; //!< The stored stencil value
};

////////////////////////////////////////////////////////////
/// \brief Stencil modes for drawing
///
////////////////////////////////////////////////////////////
struct [[nodiscard]] ZA_GRAPHICS_API StencilMode
{
    StencilComparison stencilComparison
        : 3 {StencilComparison::Always}; //!< The comparison we're performing the stencil test with

    StencilUpdateOperation stencilUpdateOperation
        : 3 {StencilUpdateOperation::Keep}; //!< The update operation to perform if the stencil test passes

    bool stencilOnly : 1 {false}; //!< Whether we should update only the stencil buffer, leaving the color buffer untouched

    StencilValue stencilReference{0u}; //!< The reference value we're performing the stencil test with

    StencilValue stencilMask{~0u}; //!< The mask to apply to both the reference value and the value in the stencil buffer

    ////////////////////////////////////////////////////////////
    /// \brief Overload of the `operator==`
    ///
    /// \param rhs Right operand
    ///
    /// \return `true` if stencil modes are equal, `false` if they are different
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] ZA_GRAPHICS_API constexpr bool operator==(const StencilMode& rhs) const = default;
};

} // namespace za

#pragma GCC diagnostic pop


////////////////////////////////////////////////////////////
/// \class za::StencilMode
/// \ingroup graphics
///
/// `za::StencilMode` is a class that controls stencil testing.
///
/// In addition to drawing to the visible portion of a render target,
/// there is the possibility to "draw" to a so-called stencil buffer.
/// The stencil buffer is a special non-visible buffer that can contain
/// a single value per pixel that is drawn. This can be thought of as a
/// fifth value in addition to red, green, blue and alpha values. The maximum
/// value that can be represented depends on what is supported by the system.
/// Typically support for a 8-bit stencil buffer should always be available.
/// This will also have to be requested when creating a render target via
/// the `za::ContextSettings` that is passed during creation. Stencil testing
/// will not work if there is no stencil buffer available in the target
/// that is being drawn to.
///
/// Initially, just like with the visible color buffer, the stencil value of
/// each pixel is set to an undefined value. Calling `za::RenderTarget::clear`
/// will set each pixel's stencil value to 0. `za::RenderTarget::clear` can be
/// called at any time to reset the stencil values back to 0.
///
/// When drawing an object, before each pixel of the color buffer is updated
/// with its new color value, the stencil test is performed. During this test
/// 2 values are compared with each other: the reference value that is passed
/// via `za::StencilMode` and the value that is currently in the stencil buffer.
/// The arithmetic comparison that is performed on the 2 values can also be
/// controlled via `za::StencilMode`. Depending on whether the test passes i.e.
/// the comparison yields `true`, the color buffer is updated with its new RGBA
/// value and if set in `za::StencilMode` the stencil buffer is updated
/// accordingly. The new stencil value will be used during stencil testing the
/// next time the pixel is drawn to.
///
/// The class is composed of 5 components, each of which has its
/// own public member variable:
/// \li Stencil Comparison (\ref stencilComparison)
/// \li Stencil Update Operation (\ref stencilUpdateOperation)
/// \li Stencil Reference Value (\ref stencilReference)
/// \li Stencil Mask Value (\ref stencilMask)
/// \li Stencil Only Update (\ref stencilOnly)
///
/// The stencil comparison specifies the comparison that is performed between
/// the reference value of the currently active `za::StencilMode` and the value
/// that is currently in the stencil buffer. This comparison determines whether
/// the stencil test passes or fails.
///
/// The stencil update operation specifies how the stencil buffer is updated if
/// the stencil test passes. If the stencil test fails, neither the color or
/// stencil buffers will be modified. If incrementing or decrementing the
/// stencil value, the new value will be clamped to the range from 0 to the
/// maximum representable value given the bit width of the stencil buffer
/// e.g. 255 if an 8-bit stencil buffer is being used.
///
/// The reference value is used both during the comparison with the current
/// stencil buffer value and as the new value to be written when the operation
/// is set to Replace.
///
/// The mask value is used to mask the bits of both the reference value and
/// the value in the stencil buffer during the comparison and when updating.
/// The mask can be used to e.g. segment the stencil value bits into separate
/// regions that are used for different purposes.
///
/// In certain situations, it might make sense to only write to the stencil
/// buffer and not the color buffer during a draw. The written stencil buffer
/// value can then be used in subsequent draws as a masking region.
///
/// A stencil mode can be specified every time you draw something
/// to a render target. It is part of the `za::RenderStates`
/// compound that is passed to `za::RenderTarget::draw`.
///
/// Usage example:
/// \code
/// // Make sure to create the render target with a stencil buffer.
/// auto window = za::RenderWindow::create({
///     .size            = {250u, 200u},
///     .title           = "Stencil Window",
///     .contextSettings = {.depthBits = 0, .stencilBits = 8},
/// }).value();
///
/// const za::CircleShape left  {{.position = { 0.f, 0.f}, .fillColor = za::Color::Green,  .radius = 100.f}};
/// const za::CircleShape middle{{.position = {25.f, 0.f}, .fillColor = za::Color::Yellow, .radius = 100.f}};
/// const za::CircleShape right {{.position = {50.f, 0.f}, .fillColor = za::Color::Red,    .radius = 100.f}};
///
/// // Clear the color and stencil buffers at the start of every frame.
/// window.clear(za::Color::Black, za::StencilValue{0u});
///
/// // Draw the middle circle in a stencil-only pass and write the value 1
/// // to the stencil buffer for every pixel the circle would have affected.
/// window.draw(middle, /* render states */ {.stencilMode = za::StencilMode{
///     .stencilComparison      = za::StencilComparison::Always,
///     .stencilUpdateOperation = za::StencilUpdateOperation::Replace,
///     .stencilOnly            = true,
///     .stencilReference       = za::StencilValue{1u},
/// }});
///
/// // Draw the left and right circles, allowing only pixels whose stencil
/// // value is not 1 (i.e. those not written when drawing the middle circle).
/// const za::StencilMode notEqualOne{
///     .stencilComparison      = za::StencilComparison::NotEqual,
///     .stencilUpdateOperation = za::StencilUpdateOperation::Keep,
///     .stencilReference       = za::StencilValue{1u},
/// };
///
/// window.draw(left,  /* render states */ {.stencilMode = notEqualOne});
/// window.draw(right, /* render states */ {.stencilMode = notEqualOne});
/// \endcode
///
/// \see `za::RenderStates`, `za::RenderTarget`
///
////////////////////////////////////////////////////////////
