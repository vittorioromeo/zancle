#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Export.hpp"

#include "Zancle/Graphics/Transform.hpp"

#include "Zancle/Geometry/Angle.hpp"
#include "Zancle/Geometry/AutoWrapAngle.hpp"
#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Math/ClampMacro.hpp"
#include "Zancle/Math/Lround.hpp"
#include "Zancle/Math/SinCosLookup.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/RequireDesignatedInitializers.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief 2D camera that defines what region is shown on screen
///
////////////////////////////////////////////////////////////
struct [[nodiscard]] ZA_GRAPHICS_API View
{
    ZA_REQUIRE_DESIGNATED_INITIALIZERS;

    ////////////////////////////////////////////////////////////
    /// \brief Normalized scissor rectangle, expressed as a fraction of the render target
    ///
    /// `ScissorRect` is a thin wrapper around `za::Rect2f` that
    /// asserts (in debug builds) that all four edges lie within
    /// `[0, 1]`. Coordinates are interpreted as a ratio of the
    /// render target's size, so that the scissor automatically
    /// follows resize events.
    ///
    /// Use `fromRectClamped` if your input may fall outside the
    /// allowed range and you want it silently clamped instead of
    /// triggering an assertion.
    ///
    ////////////////////////////////////////////////////////////
    struct [[nodiscard]] ScissorRect : Rect2f
    {
        ////////////////////////////////////////////////////////////
        /// \brief Construct from explicit position and size
        ///
        /// \param thePosition Top-left corner, in `[0, 1]` along each axis
        /// \param theSize     Size, in `[0, 1]` along each axis
        ///
        /// Asserts in debug builds that the resulting rectangle is
        /// fully contained in `[0, 1] x [0, 1]`.
        ///
        ////////////////////////////////////////////////////////////
        [[nodiscard, gnu::always_inline]] constexpr ScissorRect(Vec2f thePosition, Vec2f theSize) :
            Rect2f{thePosition, theSize}
        {
            ZA_ASSERT(position.x >= 0.f && position.x <= 1.f && "position.x must lie within [0, 1]");
            ZA_ASSERT(position.y >= 0.f && position.y <= 1.f && "position.y must lie within [0, 1]");

            ZA_ASSERT(size.x >= 0.f && "size.x must lie within [0, 1]");
            ZA_ASSERT(size.y >= 0.f && "size.y must lie within [0, 1]");

            ZA_ASSERT(position.x + size.x <= 1.f && "position.x + size.x must lie within [0, 1]");
            ZA_ASSERT(position.y + size.y <= 1.f && "position.y + size.y must lie within [0, 1]");
        }

        ////////////////////////////////////////////////////////////
        /// \brief Implicit conversion from a `Rect2f`
        ///
        /// Allows passing a regular `za::Rect2f` anywhere a
        /// `ScissorRect` is expected. The same `[0, 1]` assertions
        /// apply.
        ///
        ////////////////////////////////////////////////////////////
        [[nodiscard, gnu::always_inline]] constexpr /* implicit */ ScissorRect(const Rect2f& rect) :
            ScissorRect{rect.position, rect.size}
        {
        }

        ////////////////////////////////////////////////////////////
        /// \brief Build a `ScissorRect` by clamping a (possibly out-of-range) rectangle
        ///
        /// Unlike the constructors, this factory does not assert.
        /// Position is clamped to `[0, 1]` and size is clamped so
        /// that `position + size` never exceeds `1`. The result is
        /// always a valid `ScissorRect`.
        ///
        /// \param rect Source rectangle (any values allowed)
        ///
        /// \return Clamped `ScissorRect`
        ///
        ////////////////////////////////////////////////////////////
        [[nodiscard]] static constexpr ScissorRect fromRectClamped(za::Rect2f rect)
        {
            // Clamp the position to the range `[0, 1]`
            rect.position.x = ZA_CLAMP(rect.position.x, 0.f, 1.f);
            rect.position.y = ZA_CLAMP(rect.position.y, 0.f, 1.f);

            // Ensure the size is non-negative and so that `position + size` doesn't exceed `1`
            rect.size.x = ZA_CLAMP(rect.size.x, 0.f, 1.f - rect.position.x);
            rect.size.y = ZA_CLAMP(rect.size.y, 0.f, 1.f - rect.position.y);

            return ScissorRect{rect};
        }
    };


    ////////////////////////////////////////////////////////////
    /// \brief Create a view from a rectangle
    ///
    /// \param rectangle Rectangle defining the zone to display
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::const]] static constexpr View fromRect(const Rect2f& rectangle)
    {
        return {.center = rectangle.position + rectangle.size / 2.f, .size = rectangle.size};
    }


    ////////////////////////////////////////////////////////////
    /// \brief Create a view that exactly covers a render target of the given size
    ///
    /// The resulting view is centered on `size / 2` and has a size
    /// of `size`, so that the world-to-screen mapping is the
    /// identity (one world unit equals one pixel). This matches the
    /// "default view" that `za::RenderTarget` uses on creation.
    ///
    /// \param size Size of the target the view should cover, in pixels
    ///
    /// \return A view spanning `[0, 0, size.x, size.y]`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::const]] static constexpr View fromScreenSize(const Vec2f& size)
    {
        return {.center = size / 2.f, .size = size};
    }


    ////////////////////////////////////////////////////////////
    /// \brief Get the projection transform of the view
    ///
    /// This function is meant for internal use only.
    ///
    /// \return Projection transform defining the view
    ///
    /// \see `getInverseTransform`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::pure]] constexpr Transform getTransform() const
    {
        ZA_ASSERT(size.x != 0.f && "size.x must be non-zero");
        ZA_ASSERT(size.y != 0.f && "size.y must be non-zero");

        const float a = 2.f / size.x;
        const float b = -2.f / size.y;

        const auto [sine, cosine] = za::sinCosLookup(rotation.asRadians());

        // Analytically derived matrix: Scale_proj * Rot_-theta * Trans_-center
        return {a * cosine,
                a * sine,
                -a * (center.x * cosine + center.y * sine),
                -b * sine,
                b * cosine,
                b * (center.x * sine - center.y * cosine)};
    }


    ////////////////////////////////////////////////////////////
    /// \brief Get the inverse projection transform of the view
    ///
    /// This function is meant for internal use only.
    ///
    /// \return Inverse of the projection transform defining the view
    ///
    /// \see `getTransform`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::pure]] constexpr Transform getInverseTransform() const
    {
        const auto [sine, cosine] = za::sinCosLookup(rotation.asRadians());

        const float hw = size.x * 0.5f;
        const float hh = size.y * 0.5f;

        // Analytically derived inverse: Trans_center * Rot_theta * Scale_proj^-1
        return {cosine * hw, sine * hh, center.x, sine * hw, -cosine * hh, center.y};
    }


    ////////////////////////////////////////////////////////////
    /// \brief Transform a 2D point from world coordinates to target (pixel) coordinates.
    ///
    /// This function is the forward projection of a camera. It takes a
    /// point in the 2D world and finds its corresponding location in
    /// absolute pixels on the render target.
    ///
    /// This can be useful for tasks such as attaching a UI element
    /// (e.g., a name tag or health bar) to a sprite in the world,
    /// by calculating its screen position every frame.
    ///
    /// This function is the inverse of `screenToWorld`.
    ///
    /// \param point The point to transform, in world coordinates.
    /// \param targetSize The size of the render target the view is applied to.
    ///
    /// \return The transformed point, in target (pixel) coordinates.
    ///
    /// \see screenToWorld
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] Vec2f worldToScreen(const Vec2f point, const Vec2f targetSize) const
    {
        // 1. Transform to NDC [-1, 1]
        const Vec2f normalized = getTransform().transformPoint(point);

        // 2. Map from NDC to [0, 1] space and flip Y
        const Vec2f relativePos = Vec2f(normalized.x + 1.f, 1.f - normalized.y) * 0.5f;

        // 3. Map into the same rounded pixel viewport that rendering uses
        //    (see `computePixelViewport`), so results match the rasterized output
        const Rect2i pixelViewport = computePixelViewport(targetSize);
        return pixelViewport.position.toVec2f() + relativePos.componentWiseMul(pixelViewport.size.toVec2f());
    }


    ////////////////////////////////////////////////////////////
    /// \brief Transform a 2D point from target (pixel) coordinates to world coordinates.
    ///
    /// This function is the inverse projection, also known as "unprojection".
    /// It takes a pixel coordinate on the render target and finds its
    /// corresponding location in the 2D world.
    ///
    /// This is most commonly used to translate a mouse click on the screen
    /// into a position in the game world, for object selection, character
    /// movement, etc.
    ///
    /// This function is the inverse of `worldToScreen`.
    ///
    /// Usage example:
    /// \code
    /// // A view centered at (100, 100), showing 200x150 world units.
    /// const za::View      gameView{.center = {100.f, 100.f}, .size = {200.f, 150.f}};
    /// const za::Vec2f     targetSize{800.f, 600.f};
    ///
    /// // Simulate a mouse click at pixel (400, 300), the center of the screen.
    /// const za::Vec2f     mousePixelPos{400.f, 300.f};
    ///
    /// // Find out where that click corresponds to in the game world.
    /// const za::Vec2f worldPos = gameView.screenToWorld(mousePixelPos, targetSize);
    /// // 'worldPos' will be approximately (100, 100), the center of the view.
    ///
    /// // Now go the other way to confirm: where does the world origin (0, 0)
    /// // appear on the screen?
    /// const za::Vec2f originPixelPos = gameView.worldToScreen({0.f, 0.f}, targetSize);
    /// \endcode
    ///
    /// \param point The point to transform, in target (pixel) coordinates.
    /// \param targetSize The size of the render target the view is applied to.
    ///
    /// \return The transformed point, in world coordinates.
    ///
    /// \see worldToScreen
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] Vec2f screenToWorld(const Vec2f point, const Vec2f targetSize) const
    {
        // 1. Localize to the same rounded pixel viewport that rendering uses
        //    (see `computePixelViewport`), so results match the rasterized output
        const Rect2i pixelViewport = computePixelViewport(targetSize);

        ZA_ASSERT(pixelViewport.size.x != 0 && "viewport pixel width must be non-zero");
        ZA_ASSERT(pixelViewport.size.y != 0 && "viewport pixel height must be non-zero");

        const Vec2f relativePos = (point - pixelViewport.position.toVec2f()).componentWiseDiv(pixelViewport.size.toVec2f());

        // 2. Map from [0, 1] space back to NDC [-1, 1] space and flip Y
        const Vec2f normalized = relativePos.componentWiseMul({2.f, -2.f}) + Vec2f{-1.f, 1.f};

        // 3. Transform using the inverse
        return getInverseTransform().transformPoint(normalized);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Get the viewport of this view in pixels, applied to a specific size
    ///
    /// The viewport is defined in the view as a ratio, this function
    /// simply applies this ratio to the current dimensions of the
    /// render target to calculate the pixels rectangle that the viewport
    /// actually covers in the target.
    ///
    /// \param targetSize The size of the render target
    ///
    /// \return Viewport rectangle, expressed in pixels
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] Rect2i computePixelViewport(const Vec2f targetSize) const
    {
        return Rect2<long>({ZA_MATH_LROUNDF(targetSize.x * viewport.position.x),
                            ZA_MATH_LROUNDF(targetSize.y * viewport.position.y)},
                           {ZA_MATH_LROUNDF(targetSize.x * viewport.size.x),
                            ZA_MATH_LROUNDF(targetSize.y * viewport.size.y)})
            .toRect2i();
    }


    ////////////////////////////////////////////////////////////
    /// \brief Get the scissor rectangle of a view, applied to a specific size
    ///
    /// The scissor rectangle is defined in the view as a ratio. This
    /// function simply applies this ratio to the current dimensions
    /// of the render target to calculate the pixels rectangle
    /// that the scissor rectangle actually covers in the target.
    ///
    /// \param targetSize The size of the render target
    ///
    /// \return Scissor rectangle, expressed in pixels
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] Rect2i computePixelScissor(const Vec2f targetSize) const
    {
        return Rect2<long>({ZA_MATH_LROUNDF(targetSize.x * scissor.position.x),
                            ZA_MATH_LROUNDF(targetSize.y * scissor.position.y)},
                           {ZA_MATH_LROUNDF(targetSize.x * scissor.size.x), ZA_MATH_LROUNDF(targetSize.y * scissor.size.y)})
            .toRect2i();
    }


    ////////////////////////////////////////////////////////////
    /// \brief Compare strict equality between two `View` objects
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] ZA_GRAPHICS_API constexpr bool operator==(const View& rhs) const = default;


    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    Vec2f center; //!< Center of the view, in scene coordinates
    Vec2f size;   //!< Size of the view, in scene coordinates

    // NOLINTNEXTLINE(readability-redundant-member-init)
    AutoWrapAngle rotation{}; //!< Angle of rotation of the view rectangle

    Rect2f viewport{{0.f, 0.f}, {1.f, 1.f}}; //!< Viewport rectangle, expressed as a factor of the render-target's size
    ScissorRect scissor{{0.f, 0.f}, {1.f, 1.f}}; //!< Scissor rectangle, expressed as a factor of the render-target's size
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::View
/// \ingroup graphics
///
/// `za::View` defines a camera in the 2D scene. This is a
/// very powerful concept: you can scroll, rotate or zoom
/// the entire scene without altering the way that your
/// drawable objects are drawn.
///
/// A view is composed of a source rectangle, which defines
/// what part of the 2D scene is shown, and a target viewport,
/// which defines where the contents of the source rectangle
/// will be displayed on the render target (window or texture).
///
/// The viewport allows to map the scene to a custom part
/// of the render target, and can be used for split-screen
/// or for displaying a minimap, for example. If the source
/// rectangle doesn't have the same size as the viewport, its
/// contents will be stretched to fit in.
///
/// The scissor rectangle allows for specifying regions of the
/// render target to which modifications can be made by draw
/// operations. Only pixels that are within the region will be
/// modified by draws. Note that clear operations are *not*
/// scissored: the renderer deliberately disables scissor testing
/// before every clear, so `clear`/`clearStencil` always affect
/// the entire render target.
///
/// Certain effects can be created by either using the viewport or
/// scissor rectangle. While the results appear identical, there
/// can be times where one method should be preferred over the other.
/// Viewport transformations are applied during the vertex processing
/// stage of the graphics pipeline, before the primitives are
/// rasterized into fragments for fragment processing. Since
/// viewport processing has to be performed and cannot be disabled,
/// effects that are performed using the viewport transform are
/// basically free performance-wise. Scissor testing is performed in
/// the per-sample processing stage of the graphics pipeline, after
/// fragment processing has been performed. Because per-sample
/// processing is performed at the last stage of the pipeline,
/// fragments that are discarded at this stage will cause the
/// highest waste of GPU resources compared to any method that
/// would have discarded vertices or fragments earlier in the
/// pipeline. There are situations in which scissor testing has
/// to be used to control whether fragments are discarded or not.
/// An example of such a situation is when performing the viewport
/// transform on vertices is necessary but a subset of the generated
/// fragments should not have an effect on the stencil buffer or
/// blend with the color buffer.
///
/// `za::View` is an aggregate, so it is constructed and modified
/// directly through its public members. To apply a view to a draw
/// call, pass it as part of the `za::RenderStates` (`.view = ...`).
/// Subsequent draw calls that do not specify a view will use
/// whatever view the render target was last given via
/// `setView` (the "default view" if none was set).
///
/// Usage example:
/// \code
/// auto window = za::RenderWindow::create(/* ... */).value();
///
/// // A view rectangle located at (100, 100) with a size of 400x200.
/// za::View view = za::View::fromRect({{100.f, 100.f}, {400.f, 200.f}});
///
/// // Rotate it by 45 degrees.
/// view.rotation += za::degrees(45.f);
///
/// // Restrict its target viewport to the left half of the window.
/// view.viewport = {{0.f, 0.f}, {0.5f, 1.f}};
///
/// // Render 'someSprite' through the custom view.
/// window.draw(someSprite, {.view = view});
///
/// // Render 'someText' through whatever view is currently active on the target.
/// window.draw(someText);
/// \endcode
///
/// See also the note on coordinates and undistorted rendering in `za::Transformable`.
///
/// \see `za::RenderWindow`, `za::RenderTexture`, `za::RenderStates`
///
////////////////////////////////////////////////////////////
