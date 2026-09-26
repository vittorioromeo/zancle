#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/RenderTextureCreateSettings.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Vocabulary/Optional.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
class RenderWindow;
class RenderTexture;
class Event;
struct View;
struct WindowSettings;
} // namespace za


////////////////////////////////////////////////////////////
/// \brief Calculate a viewport that preserves the original aspect ratio.
///
/// Returns the normalized viewport rectangle needed to display
/// content of aspect ratio `originalSize.x / originalSize.y` inside
/// a target of size `newSize`, centered with letterboxing (top/bottom
/// bars when the window is taller than the content) or pillarboxing
/// (left/right bars when the window is wider than the content).
/// When the aspect ratios match within a small tolerance, the full
/// `[0, 1]` rectangle is returned.
///
/// \param newSize      The current size of the window or render target.
/// \param originalSize The original size whose aspect ratio should be preserved.
///
/// \return A `za::Rect2f` defining the viewport in normalized coordinates.
///
////////////////////////////////////////////////////////////
[[nodiscard]] za::Rect2f getAspectRatioAwareViewport(za::Vec2f newSize, za::Vec2f originalSize);

////////////////////////////////////////////////////////////
/// \brief Calculate the largest integer scale factor that fits the native resolution.
///
/// Returns the largest integer `n >= 1` such that
/// `nativeResolution * n` fits inside `windowSize` along both axes.
/// Used to drive pixel-perfect upscaling without fractional pixel
/// distortion.
///
/// \param windowSize       The current size of the window or render target.
/// \param nativeResolution The original, internal resolution of the content.
///
/// \return Integer scale factor as a float, always `>= 1`.
///
////////////////////////////////////////////////////////////
[[nodiscard]] float getPixelPerfectScale(za::Vec2f windowSize, za::Vec2f nativeResolution);

////////////////////////////////////////////////////////////
/// \brief Calculate a viewport for pixel-perfect integer scaling.
///
/// This function calculates the viewport rectangle needed to scale content
/// from its native resolution to the target window size using the largest
/// possible integer scaling factor, preventing pixel distortion. The
/// resulting view is centered within the window, with black bars
/// (letterboxing/pillarboxing) filling the unused space.
///
/// \param windowSize The current size of the window or render target.
/// \param nativeResolution The original, internal resolution of the content.
///
/// \return A `za::Rect2f` defining the viewport in normalized coordinates.
///
////////////////////////////////////////////////////////////
[[nodiscard]] za::Rect2f getPixelPerfectViewport(za::Vec2f windowSize, za::Vec2f nativeResolution);

////////////////////////////////////////////////////////////
/// \brief Compute an aspect-ratio-aware view for a window of `windowSize`.
///
/// Mirrors the view state produced by `handleAspectRatioAwareResize`
/// so the view is correctly sized at startup, before any resize event
/// has fired. The view's size is set to `originalSize`, centered, and
/// its viewport is computed via `getAspectRatioAwareViewport` so the
/// content stays centered with letterboxing or pillarboxing as needed.
///
/// Use this in place of `za::RenderTarget::computeView` when the
/// window may have been created at a different size than the logical
/// world resolution (e.g. via `makeDPIScaledRenderWindow` on a
/// high-DPI display).
///
/// \param windowSize   The current size of the window or render target.
/// \param originalSize The original size whose aspect ratio should be preserved.
///
/// \return A `za::View` with size, center, and viewport set.
///
////////////////////////////////////////////////////////////
[[nodiscard]] za::View computeAspectRatioAwareView(za::Vec2f windowSize, za::Vec2f originalSize);

////////////////////////////////////////////////////////////
/// \brief Compute a pixel-perfect view for a window of `windowSize`.
///
/// Mirrors the view state produced by `handlePixelPerfectResize` so
/// the view is correctly sized at startup, before any resize event
/// has fired. The view's size is set to `nativeResolution`, centered,
/// and its viewport is scaled by the largest integer factor that
/// fits.
///
/// \param windowSize       The current size of the window or render target.
/// \param nativeResolution The original, internal resolution of the content.
///
/// \return A `za::View` with size, center, and viewport set.
///
////////////////////////////////////////////////////////////
[[nodiscard]] za::View computePixelPerfectView(za::Vec2f windowSize, za::Vec2f nativeResolution);

////////////////////////////////////////////////////////////
/// \brief Handles a window resize event without scaling the content.
///
/// Call this inside your event loop. If the event is a resize event,
/// the view's size and center are updated to match the new window
/// size, so one world unit keeps mapping to one pixel and any extra
/// space simply reveals more of the world. The `originalSize`
/// parameter is unused and only kept for signature symmetry with the
/// other resize handlers.
///
/// \param event        The `za::Event` to process.
/// \param originalSize Unused, kept for signature symmetry.
/// \param view         The `za::View` to update.
///
/// \return True if the event was a resize event and was handled, false otherwise.
///
////////////////////////////////////////////////////////////
[[nodiscard]] bool handleNonScalingResize(const za::Event& event, [[maybe_unused]] za::Vec2f originalSize, za::View& view);

////////////////////////////////////////////////////////////
/// \brief Handles a window resize event while preserving the original aspect ratio.
///
/// Call this inside your event loop. If the event is a resize event,
/// the view is reset to display `originalSize` and its viewport is
/// updated via `getAspectRatioAwareViewport` so the content stays
/// centered with letterboxing or pillarboxing as needed.
///
/// \param event        The `za::Event` to process.
/// \param originalSize The original size whose aspect ratio should be preserved.
/// \param view         The `za::View` to update.
///
/// \return True if the event was a resize event and was handled, false otherwise.
///
////////////////////////////////////////////////////////////
[[nodiscard]] bool handleAspectRatioAwareResize(const za::Event& event, za::Vec2f originalSize, za::View& view);

////////////////////////////////////////////////////////////
/// \brief Handles a window resize event to maintain pixel-perfect scaling.
///
/// Call this inside your event loop. If the event is a resize event,
/// it will automatically update the window's view to maintain a
/// centered, integer-scaled viewport.
///
/// \param event The za::Event to process.
/// \param nativeResolution The original, internal resolution of the content.
/// \param renderWindow The za::RenderWindow to update.
///
/// \return True if the event was a resize event and was handled, false otherwise.
///
////////////////////////////////////////////////////////////
[[nodiscard]] bool handlePixelPerfectResize(const za::Event& event, za::Vec2f nativeResolution, za::View& view);

////////////////////////////////////////////////////////////
/// \brief Create a render window scaled by the primary display's DPI content scale.
///
/// Multiplies `windowSettings.size` by the primary display's content
/// scale (as reported by `za::VideoModeUtils::getPrimaryDisplayContentScale`)
/// before forwarding to `za::RenderWindow::create`, so a window
/// requested at e.g. 800x600 logical pixels comes out at 1600x1200
/// on a 2x display.
///
/// \param windowSettings Window settings whose `size` is the logical, unscaled size.
///
/// \return The created render window, or empty on failure.
///
////////////////////////////////////////////////////////////
[[nodiscard]] za::Optional<za::RenderWindow> makeDPIScaledRenderWindow(const za::WindowSettings& windowSettings);

////////////////////////////////////////////////////////////
/// \brief Create a render texture, clamping anti-aliasing to the supported maximum.
///
/// Forwards to `za::RenderTexture::create` after clamping
/// `rtCreateSettings.sampleCount` to the value reported by
/// `za::RenderTexture::getMaximumSampleCount`. A message is
/// printed when clamping occurs.
///
/// \param resolution       Resolution of the render texture in pixels.
/// \param rtCreateSettings Creation settings; `sampleCount` may be clamped.
///
/// \return The created render texture, or empty on failure.
///
////////////////////////////////////////////////////////////
[[nodiscard]] za::Optional<za::RenderTexture> makeAARenderTexture(za::Vec2u                       resolution,
                                                                  za::RenderTextureCreateSettings rtCreateSettings);
