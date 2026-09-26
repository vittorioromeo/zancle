// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "ExampleUtils/Scaling.hpp"

#include "Zancle/Graphics/RenderTexture.hpp"
#include "Zancle/Graphics/RenderTextureCreateSettings.hpp"
#include "Zancle/Graphics/RenderWindow.hpp"
#include "Zancle/Graphics/View.hpp"

#include "Zancle/Window/Event.hpp"
#include "Zancle/Window/VideoModeUtils.hpp"
#include "Zancle/Window/WindowSettings.hpp"

#include "Zancle/Fmt/Fmt.hpp"
#include "Zancle/Fmt/FmtNumeric.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Math/Fabs.hpp"
#include "Zancle/Math/Floor.hpp"
#include "Zancle/Math/MinMax.hpp"

#include "Zancle/Base/Assert.hpp"


////////////////////////////////////////////////////////////
za::Rect2f getAspectRatioAwareViewport(const za::Vec2f newSize, const za::Vec2f originalSize)
{
    ZA_ASSERT(newSize.x > 0.f && newSize.y > 0.f);
    ZA_ASSERT(originalSize.x > 0.f && originalSize.y > 0.f);

    const float contentAspectRatio = originalSize.x / originalSize.y; // Assume old size has correct ratio
    const float windowAspectRatio  = newSize.x / newSize.y;

    if (ZA_MATH_FABSF(windowAspectRatio - contentAspectRatio) < 0.01f)
    {
        // Aspect ratios match, viewport covers the whole window
        return {{0.f, 0.f}, {1.f, 1.f}};
    }

    if (windowAspectRatio > contentAspectRatio)
    {
        // Window is wider than content (pillarboxing)
        // Viewport height is 100%, width is scaled
        const float width = contentAspectRatio / windowAspectRatio;
        return {{(1.f - width) / 2.f, 0.f}, {width, 1.f}}; // Center horizontally
    }

    // Window is taller than content (letterboxing)
    // Viewport width is 100%, height is scaled
    const float height = windowAspectRatio / contentAspectRatio;
    return {{0.f, (1.f - height) / 2.f}, {1.f, height}}; // Center vertically
}


////////////////////////////////////////////////////////////
float getPixelPerfectScale(const za::Vec2f windowSize, const za::Vec2f nativeResolution)
{
    ZA_ASSERT(windowSize.x > 0.f && windowSize.y > 0.f);
    ZA_ASSERT(nativeResolution.x > 0.f && nativeResolution.y > 0.f);

    const auto scaleRatio = windowSize.componentWiseDiv(nativeResolution);

    // The final scale is the smaller of the two, floored to the nearest integer
    // This ensures that the scaled content fits entirely within the window
    return za::max(1.f, za::floor(za::min(scaleRatio.x, scaleRatio.y)));
}


////////////////////////////////////////////////////////////
za::Rect2f getPixelPerfectViewport(const za::Vec2f windowSize, const za::Vec2f nativeResolution)
{
    const float scale = getPixelPerfectScale(windowSize, nativeResolution);

    // Calculate the size of the viewport in normalized coordinates [0, 1]
    const float viewWidth  = (nativeResolution.x * scale) / windowSize.x;
    const float viewHeight = (nativeResolution.y * scale) / windowSize.y;

    // Calculate the position to center the viewport
    const float viewX = (1.f - viewWidth) / 2.f;
    const float viewY = (1.f - viewHeight) / 2.f;

    return {{viewX, viewY}, {viewWidth, viewHeight}};
}


namespace
{
////////////////////////////////////////////////////////////
[[nodiscard]] za::View computeViewImpl(const za::Vec2f windowSize, const za::Vec2f originalSize, auto&& fnViewport)
{
    return {
        .center   = originalSize / 2.f,
        .size     = originalSize,
        .viewport = fnViewport(windowSize, originalSize),
    };
}


////////////////////////////////////////////////////////////
[[nodiscard]] bool handleResizeImpl(const za::Event& event, const za::Vec2f originalSize, za::View& view, auto&& fnViewport)
{
    const auto* eResized = event.getIf<za::Event::Resized>();
    if (eResized == nullptr)
        return false;

    const auto newSize = eResized->size.toVec2f();

    if (newSize.x <= 0.f || newSize.y <= 0.f)
        return true;

    view.center   = originalSize / 2.f;
    view.size     = originalSize;
    view.viewport = fnViewport(newSize, originalSize);

    return true;
}

} // namespace


////////////////////////////////////////////////////////////
za::View computeAspectRatioAwareView(const za::Vec2f windowSize, const za::Vec2f originalSize)
{
    return computeViewImpl(windowSize, originalSize, &getAspectRatioAwareViewport);
}


////////////////////////////////////////////////////////////
za::View computePixelPerfectView(const za::Vec2f windowSize, const za::Vec2f nativeResolution)
{
    return computeViewImpl(windowSize, nativeResolution, &getPixelPerfectViewport);
}


////////////////////////////////////////////////////////////
bool handleNonScalingResize(const za::Event& event, [[maybe_unused]] const za::Vec2f originalSize, za::View& view)
{
    const auto* eResized = event.getIf<za::Event::Resized>();
    if (eResized == nullptr)
        return false;

    const auto newSize = eResized->size.toVec2f();

    if (newSize.x <= 0.f || newSize.y <= 0.f)
        return true;

    view.center = newSize / 2.f;
    view.size   = newSize;

    return true;
}


////////////////////////////////////////////////////////////
bool handleAspectRatioAwareResize(const za::Event& event, const za::Vec2f originalSize, za::View& view)
{
    return handleResizeImpl(event, originalSize, view, &getAspectRatioAwareViewport);
}


////////////////////////////////////////////////////////////
bool handlePixelPerfectResize(const za::Event& event, const za::Vec2f nativeResolution, za::View& view)
{
    return handleResizeImpl(event, nativeResolution, view, &getPixelPerfectViewport);
}


////////////////////////////////////////////////////////////
za::Optional<za::RenderWindow> makeDPIScaledRenderWindow(const za::WindowSettings& windowSettings)
{
    const auto  fSize                      = windowSettings.size.toVec2f();
    const float primaryDisplayContentScale = za::VideoModeUtils::getPrimaryDisplayContentScale();

    auto adjustedWindowSettings = windowSettings;
    adjustedWindowSettings.size = (fSize * primaryDisplayContentScale).toVec2u();

    return za::RenderWindow::create(adjustedWindowSettings);
}


////////////////////////////////////////////////////////////
za::Optional<za::RenderTexture> makeAARenderTexture(const za::Vec2u resolution, za::RenderTextureCreateSettings rtCreateSettings)
{
    const auto maxAALevel = za::RenderTexture::getMaximumSampleCount();

    if (rtCreateSettings.sampleCount > maxAALevel)
    {
        za::printLn("Desired AA level {} higher than supported {}, falling back to maximum",
                    rtCreateSettings.sampleCount,
                    maxAALevel);

        rtCreateSettings.sampleCount = maxAALevel;
    }

    return za::RenderTexture::create(resolution.toVec2u(), rtCreateSettings);
}
