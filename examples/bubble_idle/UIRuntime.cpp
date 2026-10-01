#include "Aliases.hpp"
#include "BubbleIdleHelpers.hpp"
#include "BubbleIdleMain.hpp"
#include "Constants.hpp"
#include "ImGuiNotify.hpp"
#include "Milestones.hpp"
#include "Playthrough.hpp"
#include "Profile.hpp"
#include "UIState.hpp"
#include "Version.hpp"

#include "ExampleUtils/Easing.hpp"
#include "ExampleUtils/NinePatchUtils.hpp"

#include "Zancle/Graphics/Color.hpp"
#include "Zancle/Graphics/DrawableBatch.hpp"
#include "Zancle/Graphics/RectangleShapeData.hpp"
#include "Zancle/Graphics/Texture.hpp"
#include "Zancle/Graphics/View.hpp"

#include "Zancle/ImGui/IncludeImGui.hpp"

#include "Zancle/Chrono/Clock.hpp"
#include "Zancle/Chrono/Time.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Math/Clamp.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/Limits.hpp"
#include "Zancle/Base/SizeT.hpp"

#include <cstdio>
#include <cstring>

namespace
{
static_assert(sizeof(unsigned int) <= sizeof(ImTextureID), "ImTextureID is not large enough to fit unsigned int.");

[[nodiscard]] ImTextureID toImTextureIDUiRuntime(const unsigned int nativeHandle)
{
    ImTextureID textureID{};
    std::memcpy(&textureID, &nativeHandle, sizeof(nativeHandle));
    return textureID;
}

void drawNinePatchInImGui(ImDrawList&            drawList,
                          const za::Texture&     texture,
                          const za::Vec2f        position,
                          const za::Vec2f        size,
                          const za::Rect2f       textureRect,
                          const NinePatchBorders borders,
                          const ImU32            color)
{
    if (size.x <= 0.f || size.y <= 0.f)
        return;

    const za::Rect2f sourceRect = textureRect == za::Rect2f{} ? texture.getRect() : textureRect;

    if (sourceRect.size.x <= 0.f || sourceRect.size.y <= 0.f)
        return;

    const auto srcX    = makeNinePatchSlices(sourceRect.size.x, borders.left, borders.right);
    const auto srcY    = makeNinePatchSlices(sourceRect.size.y, borders.top, borders.bottom);
    const auto dstX    = makeNinePatchSlices(size.x, borders.left, borders.right);
    const auto dstY    = makeNinePatchSlices(size.y, borders.top, borders.bottom);
    const auto srcPosX = makeNinePatchPositions(sourceRect.position.x, srcX);
    const auto srcPosY = makeNinePatchPositions(sourceRect.position.y, srcY);
    const auto dstPosX = makeNinePatchPositions(position.x, dstX);
    const auto dstPosY = makeNinePatchPositions(position.y, dstY);

    const za::Vec2f   textureSize = texture.getSize().toVec2f();
    const ImTextureID textureID   = toImTextureIDUiRuntime(texture.getNativeHandle());

    drawList.PushClipRectFullScreen();

    for (za::SizeT iy = 0; iy < 3u; ++iy)
    {
        for (za::SizeT ix = 0; ix < 3u; ++ix)
        {
            if (srcX[ix] <= 0.f || srcY[iy] <= 0.f || dstX[ix] <= 0.f || dstY[iy] <= 0.f)
                continue;

            const ImVec2 pMin{dstPosX[ix], dstPosY[iy]};
            const ImVec2 pMax{dstPosX[ix] + dstX[ix], dstPosY[iy] + dstY[iy]};
            const ImVec2 uvMin{srcPosX[ix] / textureSize.x, srcPosY[iy] / textureSize.y};
            const ImVec2 uvMax{(srcPosX[ix] + srcX[ix]) / textureSize.x, (srcPosY[iy] + srcY[iy]) / textureSize.y};

            drawList.AddImage(textureID, pMin, pMax, uvMin, uvMax, color);
        }
    }

    drawList.PopClipRect();
}

struct ImGuiStyleScales
{
    const ImVec2 windowPadding;
    const float  windowRounding;
    const ImVec2 windowMinSize;
    const float  childRounding;
    const float  popupRounding;
    const ImVec2 framePadding;
    const float  frameRounding;
    const ImVec2 itemSpacing;
    const ImVec2 itemInnerSpacing;
    const ImVec2 cellPadding;
    const ImVec2 touchExtraPadding;
    const float  indentSpacing;
    const float  columnsMinSpacing;
    const float  scrollbarSize;
    const float  scrollbarRounding;
    const float  grabMinSize;
    const float  grabRounding;
    const float  logSliderDeadzone;
    const float  tabRounding;
    const float  tabCloseButtonMinWidthUnselected;
    const float  tabBarOverlineSize;
    const ImVec2 separatorTextPadding;
    const ImVec2 displayWindowPadding;
    const ImVec2 displaySafeAreaPadding;
    const float  mouseCursorScale;

    void applyWithScale(ImGuiStyle& style, const float scale, const bool steamDeck) const
    {
        style.WindowPadding                    = windowPadding * scale;
        style.WindowRounding                   = windowRounding * scale;
        style.WindowMinSize                    = windowMinSize * scale;
        style.ChildRounding                    = childRounding * scale;
        style.PopupRounding                    = popupRounding * scale;
        style.FramePadding                     = framePadding * scale;
        style.FrameRounding                    = frameRounding * scale;
        style.ItemSpacing                      = itemSpacing * scale;
        style.ItemInnerSpacing                 = itemInnerSpacing * scale;
        style.CellPadding                      = cellPadding * scale;
        style.TouchExtraPadding                = touchExtraPadding * scale;
        style.IndentSpacing                    = indentSpacing * scale;
        style.ColumnsMinSpacing                = columnsMinSpacing * scale;
        style.ScrollbarSize                    = scrollbarSize * scale * (steamDeck ? 2.f : 0.65f);
        style.ScrollbarRounding                = scrollbarRounding * scale;
        style.GrabMinSize                      = grabMinSize * scale;
        style.GrabRounding                     = grabRounding * scale;
        style.LogSliderDeadzone                = logSliderDeadzone * scale;
        style.TabRounding                      = tabRounding * scale;
        style.TabCloseButtonMinWidthUnselected = tabCloseButtonMinWidthUnselected * scale;
        style.TabBarOverlineSize               = tabBarOverlineSize * scale;
        style.SeparatorTextPadding             = separatorTextPadding * scale;
        style.DisplayWindowPadding             = displayWindowPadding * scale;
        style.DisplaySafeAreaPadding           = displaySafeAreaPadding * scale;
        style.MouseCursorScale                 = mouseCursorScale * scale;
    }
};
} // namespace

////////////////////////////////////////////////////////////
void Main::uiDraw(const za::Vec2f mousePos)
{
    ImGuiStyle& style = ImGui::GetStyle();

    style.FrameBorderSize = 2.f;
    style.FrameRounding   = 10.f;
    style.WindowRounding  = 5.f;

    static const auto initialStyleScales = [&] -> ImGuiStyleScales
    {
        return {
            .windowPadding                    = style.WindowPadding,
            .windowRounding                   = style.WindowRounding,
            .windowMinSize                    = style.WindowMinSize,
            .childRounding                    = style.ChildRounding,
            .popupRounding                    = style.PopupRounding,
            .framePadding                     = style.FramePadding,
            .frameRounding                    = style.FrameRounding,
            .itemSpacing                      = style.ItemSpacing,
            .itemInnerSpacing                 = style.ItemInnerSpacing,
            .cellPadding                      = style.CellPadding,
            .touchExtraPadding                = style.TouchExtraPadding,
            .indentSpacing                    = style.IndentSpacing,
            .columnsMinSpacing                = style.ColumnsMinSpacing,
            .scrollbarSize                    = style.ScrollbarSize,
            .scrollbarRounding                = style.ScrollbarRounding,
            .grabMinSize                      = style.GrabMinSize,
            .grabRounding                     = style.GrabRounding,
            .logSliderDeadzone                = style.LogSliderDeadzone,
            .tabRounding                      = style.TabRounding,
            .tabCloseButtonMinWidthUnselected = style.TabCloseButtonMinWidthUnselected,
            .tabBarOverlineSize               = style.TabBarOverlineSize,
            .separatorTextPadding             = style.SeparatorTextPadding,
            .displayWindowPadding             = style.DisplayWindowPadding,
            .displaySafeAreaPadding           = style.DisplaySafeAreaPadding,
            .mouseCursorScale                 = style.MouseCursorScale,
        };
    }();

    uiState.uiWidgetId = 0u;

    ImGui::PushFont(fontImGuiSuperBakery);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.f);

    style.Colors[ImGuiCol_WindowBg]     = ImVec4(0.f, 0.f, 0.f, 0.65f);
    style.Colors[ImGuiCol_PopupBg]      = ImVec4(0.96f, 0.98f, 1.00f, 0.97f);
    style.Colors[ImGuiCol_Border]       = colorBlueOutline.toVec4<ImVec4>();
    style.Colors[ImGuiCol_Text]         = za::Color{50u, 84u, 135u};
    style.Colors[ImGuiCol_TextDisabled] = za::Color{50u, 84u, 135u}.withLightness(0.35f).withSaturation(0.25f);

    style.Colors[ImGuiCol_FrameBg]        = ImVec4(0.82f, 0.88f, 0.96f, 0.85f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.72f, 0.82f, 0.95f, 0.95f);
    style.Colors[ImGuiCol_FrameBgActive]  = ImVec4(0.65f, 0.78f, 0.95f, 1.00f);

    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.20f, 0.45f, 0.85f, 1.00f);

    style.Colors[ImGuiCol_SliderGrab]       = ImVec4(0.30f, 0.55f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.20f, 0.45f, 0.85f, 1.00f);

    style.Colors[ImGuiCol_Button]        = ImVec4(0.72f, 0.82f, 0.95f, 0.90f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.60f, 0.75f, 0.95f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive]  = ImVec4(0.45f, 0.65f, 0.92f, 1.00f);

    style.Colors[ImGuiCol_Header]        = ImVec4(0.72f, 0.82f, 0.95f, 0.75f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.60f, 0.75f, 0.95f, 0.85f);
    style.Colors[ImGuiCol_HeaderActive]  = ImVec4(0.45f, 0.65f, 0.92f, 0.95f);

    style.Colors[ImGuiCol_Separator]        = ImVec4(0.50f, 0.65f, 0.85f, 0.55f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.40f, 0.58f, 0.85f, 0.80f);
    style.Colors[ImGuiCol_SeparatorActive]  = ImVec4(0.30f, 0.50f, 0.85f, 1.00f);

    style.Colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.88f, 0.93f, 0.98f, 0.60f);
    style.Colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.65f, 0.78f, 0.92f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.50f, 0.68f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.35f, 0.58f, 0.88f, 1.00f);

    const float     newScalingFactor = profile.uiScale;
    const auto      resolution       = getResolution();
    const float     deltaTime        = ImGui::GetIO().DeltaTime;
    const ImVec2    imguiMousePos    = ImGui::GetMousePos();
    const za::Vec2f windowMousePos{imguiMousePos.x, imguiMousePos.y};

    constexpr float uiMenuAutoHideDelaySeconds = 1.25f;
    constexpr float uiMenuRevealDuration       = 0.7f;
    constexpr float uiMenuHiddenPeekWidth      = 32.f;
    constexpr float uiMenuHotspotWidth         = 128.f;

    initialStyleScales.applyWithScale(style, newScalingFactor, onSteamDeck);

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_IsTouchScreen;
    ImGui::GetIO().FontGlobalScale = newScalingFactor * 0.975f;

    (void)&Main::uiDpsMeter;

    if (inSpeedrunPlaythrough())
        uiSpeedrunning();

    if (!uiState.debugHideUI)
        uiDrawQuickbar();

    if (!uiState.debugHideUI)
        uiDrawMinimapZoomButtons();

    const za::Vec2f  uiOpenWindowPos = uiGetWindowPos();
    const za::Rect2f gameViewBounds  = getViewportPixelBounds(gameView, resolution);
    const float      gameViewRightX  = gameViewBounds.position.x + gameViewBounds.size.x;
    const float playableRightScreenX = gameView.worldToScreen({pt->getMapLimit(), gameView.center.y}, resolution).x;
    const bool  uiMenuDoesNotCoverPlayableSpace = uiOpenWindowPos.x >= gameViewRightX ||
                                                  playableRightScreenX <= uiOpenWindowPos.x;

    const float menuHiddenX   = resolution.x - uiMenuHiddenPeekWidth * newScalingFactor;
    const float hotspotHeight = uiState.uiMenuLastDrawSize.y > 1.f ? uiState.uiMenuLastDrawSize.y
                                                                   : uiGetMaxWindowHeight() * newScalingFactor;

    const za::Rect2f menuHoverHotspot{
        {resolution.x - uiMenuHotspotWidth * newScalingFactor, uiOpenWindowPos.y},
        {uiMenuHotspotWidth * newScalingFactor, hotspotHeight},
    };

    if (uiState.uiMenuLocked)
    {
        uiState.uiMenuHideTimer = uiMenuAutoHideDelaySeconds;
    }
    else if (uiMenuDoesNotCoverPlayableSpace)
    {
        uiState.uiMenuHideTimer = uiMenuAutoHideDelaySeconds;
    }
    else if (menuHoverHotspot.contains(windowMousePos) ||
             (uiState.uiMenuLastDrawSize.x > 1.f && uiState.uiMenuLastDrawSize.y > 1.f &&
              za::Rect2f{uiState.uiMenuLastDrawPos, uiState.uiMenuLastDrawSize}.contains(windowMousePos)))
    {
        uiState.uiMenuHideTimer = uiMenuAutoHideDelaySeconds;
    }
    else
    {
        uiState.uiMenuHideTimer = za::clamp(uiState.uiMenuHideTimer - deltaTime, 0.f, uiMenuAutoHideDelaySeconds);
    }

    const float uiMenuRevealTarget = uiState.uiMenuHideTimer > 0.f ? 1.f : 0.f;
    const float uiMenuRevealStep   = uiMenuRevealDuration > 0.f ? deltaTime / uiMenuRevealDuration : 1.f;

    if (uiState.uiMenuRevealT < uiMenuRevealTarget)
        uiState.uiMenuRevealT = za::clamp(uiState.uiMenuRevealT + uiMenuRevealStep, 0.f, 1.f);
    else if (uiState.uiMenuRevealT > uiMenuRevealTarget)
        uiState.uiMenuRevealT = za::clamp(uiState.uiMenuRevealT - uiMenuRevealStep, 0.f, 1.f);

    const float  uiMenuRevealEased = easeInOutBack(uiState.uiMenuRevealT);
    const float  uiMenuDrawX       = menuHiddenX + (uiOpenWindowPos.x - menuHiddenX) * uiMenuRevealEased;
    const ImVec2 uiMenuSize{uiWindowWidth * newScalingFactor, uiGetMaxWindowHeight() * newScalingFactor};

    ImGui::SetNextWindowPos({uiMenuDrawX, uiOpenWindowPos.y}, 0, {0.f, 0.f});
    ImGui::SetNextWindowSize(uiMenuSize, ImGuiCond_Always);

    ImGui::Begin("##menu",
                 nullptr,
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoBackground);

    ImU32 col_top = ImColor(25, 65, 125, 220);
    ImU32 col_bot = ImColor(5, 20, 45, 240);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2      p_min     = ImGui::GetWindowPos();
    ImVec2      p_max     = ImVec2(p_min.x + ImGui::GetWindowWidth(), p_min.y + ImGui::GetWindowHeight());

    (void)col_top;
    (void)col_bot;

    cpuCloudUiDrawableBatch.add(za::RectangleShapeData{
        .position  = p_min,
        .fillColor = za::Color::White,
        .size      = p_max - p_min,
    });

    p_min.x += 35.f;

    drawCloudFrame({
        .time              = shaderTime,
        .mins              = p_min,
        .maxs              = p_max,
        .xSteps            = 6,
        .ySteps            = 12,
        .scaleMult         = 4.f,
        .outwardOffsetMult = 1.f,
        .batch             = &cpuCloudUiDrawableBatch,
    });

    uiTabBar();

    const bool uiMenuHovered = ImGui::IsWindowHovered(
        ImGuiHoveredFlags_RootAndChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    const bool uiMenuActiveInteraction = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
                                         ImGui::IsAnyItemActive();

    if (uiMenuHovered || uiMenuActiveInteraction)
    {
        uiState.uiMenuHideTimer = uiMenuAutoHideDelaySeconds;
    }

    if (!ImGui::GetIO().WantCaptureMouse && particleCullingBoundaries.isInside(mousePos))
        uiMakeShrineOrCatTooltip(mousePos);

    const za::Vec2f windowDrawPos  = ImGui::GetWindowPos();
    const za::Vec2f windowDrawSize = ImGui::GetWindowSize();

    uiState.uiMenuLastDrawPos  = windowDrawPos;
    uiState.uiMenuLastDrawSize = windowDrawSize;

    if (0)
    {
        if (windowDrawSize.y > 64.f)
        {
            const float offset = 15.f;

            drawNinePatchInImGui(*draw_list,
                                 txFrame,
                                 windowDrawPos - za::Vec2f{offset, offset} + za::Vec2f{2.f, 1.f},
                                 windowDrawSize + za::Vec2f{offset * 2.f, offset * 2.f} - za::Vec2f{3.f, 3.f},
                                 txFrame.getRect(),
                                 NinePatchBorders::all(64.f),
                                 IM_COL32_WHITE);
        }
        else
        {
            const float offset = 4.f * profile.uiScale;

            drawNinePatchInImGui(*draw_list,
                                 txFrameTiny,
                                 windowDrawPos - za::Vec2f{offset, offset} + za::Vec2f{2.f, 1.f},
                                 windowDrawSize + za::Vec2f{offset * 2.f, offset * 2.f} - za::Vec2f{3.f, 3.f},
                                 txFrameTiny.getRect(),
                                 NinePatchBorders::all(18.f),
                                 IM_COL32_WHITE);
        }
    }

    ImGui::End();

    uiDrawDebugWindow();
    uiDrawExitPopup(newScalingFactor);

    if (profile.enableNotifications)
        ImGui::RenderNotifications(/* paddingY */ (profile.showDpsMeter ? (15.f + 60.f + 15.f) : 15.f) * profile.uiScale,
                                   [&](const float opacity)
        {
            ImGui::PushFont(fontImGuiMouldyCheese);
            uiSetFontScale(uiToolTipFontScale);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, opacity);
        },
                                   [&](const float opacity)
        {
            const ImVec2 winPos  = ImGui::GetWindowPos();
            const ImVec2 winSize = ImGui::GetWindowSize();

            const za::Vec2f pMin{winPos.x - 8.f, winPos.y - 8.f};
            const za::Vec2f pMax{winPos.x + winSize.x + 8.f, winPos.y + winSize.y + 8.f};

            const auto tintColor = za::Color::whiteWithAlpha(static_cast<za::U8>(opacity * 255.f));

            cpuCloudUiDrawableBatch.add(za::RectangleShapeData{
                .position  = pMin,
                .fillColor = tintColor,
                .size      = pMax - pMin,
            });

            const int xSteps = za::clamp(static_cast<int>((pMax.x - pMin.x) / 28.f), 3, 24);
            const int ySteps = za::clamp(static_cast<int>((pMax.y - pMin.y) / 28.f), 3, 24);

            drawCloudFrame({
                .time              = shaderTime,
                .mins              = pMin,
                .maxs              = pMax,
                .xSteps            = xSteps,
                .ySteps            = ySteps,
                .scaleMult         = 1.6f,
                .outwardOffsetMult = 1.f,
                .color             = tintColor,
                .batch             = &cpuCloudUiDrawableBatch,
            });

            ImGui::PopStyleVar();
            uiSetFontScale(uiNormalFontScale);
            ImGui::PopFont();
        });

    ImGui::PopStyleVar();
    ImGui::PopFont();
}

////////////////////////////////////////////////////////////
void Main::uiDpsMeter()
{
    const auto resolution = getResolution();

    const float  dpsMeterScale = profile.uiScale;
    const ImVec2 dpsMeterSize(240.f * dpsMeterScale, 65.f * dpsMeterScale);

    ImGui::SetNextWindowPos({15.f, resolution.y - 15.f}, 0, {0.f, 1.f});
    ImGui::SetNextWindowSizeConstraints(dpsMeterSize, dpsMeterSize);

    const auto oldRounding            = ImGui::GetStyle().FrameRounding;
    const auto oldBorderSize          = ImGui::GetStyle().FrameBorderSize;
    ImGui::GetStyle().FrameRounding   = 0.f;
    ImGui::GetStyle().FrameBorderSize = 0.f;

    ImGui::Begin("##dpsmeter",
                 nullptr,
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMouseInputs);

    ImDrawList* const drawList = ImGui::GetWindowDrawList();

    {
        const float offset = 8.f * profile.uiScale;

        const ImVec2 pMin = ImGui::GetWindowPos();
        const ImVec2 pMax = ImVec2(pMin.x + ImGui::GetWindowWidth(), pMin.y + ImGui::GetWindowHeight());

        drawList->AddRectFilledMultiColor(pMin + ImVec2{offset, offset},
                                          pMax - ImVec2{offset, offset},
                                          ImColor(25, 65, 125, 220),
                                          ImColor(25, 65, 125, 220),
                                          ImColor(5, 20, 45, 240),
                                          ImColor(5, 20, 45, 240));
    }

    uiSetFontScale(0.75f);

    static thread_local za::Vector<float> sampleBuffer(60);
    samplerMoneyPerSecond.writeSamplesInOrder(sampleBuffer.data());

    const auto average = static_cast<MoneyType>(samplerMoneyPerSecond.getAverageAs<double>());

    static thread_local char avgBuffer[64];
    std::sprintf(avgBuffer, "%s $/s", toStringWithSeparators(average));

    ImGui::PlotLines("##dpsmeter",
                     sampleBuffer.data(),
                     static_cast<int>(sampleBuffer.size()),
                     0,
                     avgBuffer,
                     0.f,
                     ZA_FLOAT_MAX,
                     ImVec2(dpsMeterSize.x - 15.f * dpsMeterScale, dpsMeterSize.y - 17.f * dpsMeterScale));

    const auto windowDrawPos  = za::Vec2f(ImGui::GetWindowPos());
    const auto windowDrawSize = za::Vec2f(ImGui::GetWindowSize());

    {
        const float offset = -2.f * profile.uiScale;

        drawNinePatchInImGui(*drawList,
                             txFrameTiny,
                             windowDrawPos - za::Vec2f{offset, offset} + za::Vec2f{1.f, 1.f},
                             windowDrawSize + za::Vec2f{offset * 2.f, offset * 2.f} - za::Vec2f{1.f, 3.f},
                             txFrameTiny.getRect(),
                             NinePatchBorders::all(18.f),
                             IM_COL32_WHITE);
    }

    ImGui::GetStyle().FrameRounding   = oldRounding;
    ImGui::GetStyle().FrameBorderSize = oldBorderSize;

    ImGui::End();
}

////////////////////////////////////////////////////////////
void Main::uiSpeedrunning()
{
    const auto resolution = getResolution();

    const float  dpsMeterScale = profile.uiScale;
    const ImVec2 dpsMeterSize(240.f * dpsMeterScale, 65.f * dpsMeterScale);
    const ImVec2 speedrunningWindowSize(240.f * 2.f * dpsMeterScale - 45.f, 1.f * 65.f * dpsMeterScale);

    ImGui::SetNextWindowPos({dpsMeterSize.x + 15.f + 15.f, resolution.y - 15.f}, 0, {0.f, 1.f});
    ImGui::SetNextWindowSizeConstraints(speedrunningWindowSize, speedrunningWindowSize);

    ImGui::Begin("##speedrunning",
                 nullptr,
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoTitleBar);

    uiSetFontScale(0.55f);

    ImGui::Text("Game Version: %s", BUBBLEBYTE_VERSION_STR);

    ImGui::SetCursorPosY(21.f * profile.uiScale);
    uiSetFontScale(1.40f);

    if (pt->speedrunStartTime.hasValue())
    {
        const auto [hours, mins, secs, millis] = formatSpeedrunTime(za::Clock::now() - *(pt->speedrunStartTime));
        ImGui::Text("%02llu:%02llu:%02llu:%03llu", hours, mins, secs, millis);
    }
    else
    {
        ImGui::Text("POP TO START");
    }

    uiSetFontScale(0.75f);

    const ImVec4 textColorLocked{1.f, 1.f, 1.f, 0.5f};
    const ImVec4 textColorUnlocked{1.f, 1.f, 1.f, 1.f};

    const auto textSplit = [&](const char* title, const MilestoneTimestamp split)
    {
        if (split == maxMilestone)
        {
            ImGui::TextColored(textColorLocked, "%s: XX:XX:XX:XXX", title);
        }
        else
        {
            const auto [hours, mins, secs, millis] = formatSpeedrunTime(za::microseconds(static_cast<za::I64>(split)));
            ImGui::TextColored(textColorUnlocked, "%s: %02llu:%02llu:%02llu:%03llu", title, hours, mins, secs, millis);
        }
    };

    ImGui::SetCursorPosY(10.f * profile.uiScale);
    ImGui::SetCursorPosX(205.f * profile.uiScale);

    ImGui::BeginGroup();
    textSplit("Prestige Lv.2", pt->speedrunSplits.prestigeLevel2);
    textSplit("Prestige Lv.3", pt->speedrunSplits.prestigeLevel3);
    ImGui::EndGroup();

    ImGui::End();
}
