

#include "Aliases.hpp"
#include "BubbleIdleMain.hpp"
#include "BubbleIdleMainInline.hpp" // IWYU pragma: keep
#include "Cat.hpp"
#include "CatType.hpp"
#include "Constants.hpp"
#include "ExampleProfiler/Profiler.hpp"
#include "ImGuiNotify.hpp"
#include "MainShaders.hpp"
#include "NotificationState.hpp"
#include "ParticleType.hpp"
#include "Playthrough.hpp"
#include "Profile.hpp"
#include "PurchasableScalingValue.hpp"
#include "Shrine.hpp"
#include "ShrineConstants.hpp"
#include "UIState.hpp"

#include "ExampleUtils/Easing.hpp"
#include "ExampleUtils/HueColor.hpp"
#include "ExampleUtils/Progress.hpp"

#include "Zancle/Graphics/Color.hpp"
#include "Zancle/Graphics/DrawTextureSettings.hpp"
#include "Zancle/Graphics/RenderTexture.hpp"
#include "Zancle/Graphics/Sprite.hpp"
#include "Zancle/Graphics/Texture.hpp"
#include "Zancle/Graphics/TextureAtlas.hpp"

#include "Zancle/ImGui/ImGuiContext.hpp"
#include "Zancle/ImGui/IncludeImGui.hpp"
#include "Zancle/ImGui/IncludeImGuiInternal.hpp"

#include "Zancle/String/String.hpp"
#include "Zancle/String/ToString.hpp"

#include "Zancle/Algorithm/AnyOf.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Vocabulary/UniquePtr.hpp"

#include "Zancle/Math/Clamp.hpp"
#include "Zancle/Math/Constants.hpp"
#include "Zancle/Math/Cos.hpp"
#include "Zancle/Math/MinMax.hpp"
#include "Zancle/Math/Pow.hpp"
#include "Zancle/Math/Sin.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/GetArraySize.hpp"
#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/Limits.hpp"
#include "Zancle/Base/SizeT.hpp"
#include "Zancle/Base/Strlen.hpp"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace
{
static_assert(sizeof(unsigned int) <= sizeof(ImTextureID), "ImTextureID is not large enough to fit unsigned int.");


////////////////////////////////////////////////////////////
[[nodiscard]] ImTextureID toImTextureID(const unsigned int nativeHandle)
{
    ImTextureID textureID{};
    std::memcpy(&textureID, &nativeHandle, sizeof(nativeHandle));
    return textureID;
}

} // namespace

////////////////////////////////////////////////////////////
bool Main::drawTabButton(const float             scaleMult,
                         const char*             label,
                         const bool              selected,
                         const TabButtonPalette& palette,
                         const za::Vec2f         size,
                         const bool              square)
{
    ImGui::PushStyleColor(ImGuiCol_Button, (selected ? palette.active : palette.idle).toVec4<ImVec4>());
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (selected ? palette.active : palette.hovered).toVec4<ImVec4>());
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, palette.active.toVec4<ImVec4>());
    ImGui::PushStyleColor(ImGuiCol_Border, (selected ? palette.active : palette.hovered).toVec4<ImVec4>());
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 10.f);

    auto outcome = uiAnimatedButton(square ? txCloudBtnSquare2 : txCloudBtnSmall,
                                    label,
                                    size,
                                    1.f * scaleMult,
                                    1.0f,
                                    1.75f * scaleMult,
                                    selected);

    const bool pressed = outcome == Main::AnimatedButtonOutcome::Clicked;

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);

    return pressed;
}

////////////////////////////////////////////////////////////
float Main::uiGetMaxWindowHeight() const
{
    return za::max(getResolution().y - 19.f, (getResolution().y - 19.f) / profile.uiScale);
}

////////////////////////////////////////////////////////////
void Main::uiSetFontScale(const float scale)
{
    uiState.lastFontScale = scale;
    ImGui::SetWindowFontScale(scale);
}

////////////////////////////////////////////////////////////
void Main::uiMakeButtonLabels(const char* label, const char* xLabelBuffer)
{
    const char* readableLabelEnd = ImGui::FindRenderedTextEnd(label);

    const bool subBullet = label[0] == ' ' && label[1] == ' ';

    const char* labelStart = subBullet ? label + 2 : label;
    const auto  labelSize  = readableLabelEnd - labelStart;

    // button label
    uiSetFontScale((subBullet ? uiSubBulletFontScale : uiNormalFontScale) * 1.15f);
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%.*s", static_cast<int>(labelSize), labelStart);
    ImGui::SameLine();

    // button top label
    uiSetFontScale(0.5f);
    ImGui::Text("%s", xLabelBuffer);
    uiSetFontScale(subBullet ? uiSubBulletFontScale : uiNormalFontScale);
    ImGui::SameLine();

    ImGui::NextColumn();
}

////////////////////////////////////////////////////////////
void Main::uiPushButtonColors()
{
    const auto convertColorWithHueMod = [&](const auto colorId)
    {
        return za::Color::fromVec4(ImGui::GetStyleColorVec4(colorId))
            .withRotatedHue(uiState.uiButtonHueMod)
            .template toVec4<ImVec4>();
    };

    ImGui::PushStyleColor(ImGuiCol_Button, convertColorWithHueMod(ImGuiCol_Button));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, convertColorWithHueMod(ImGuiCol_ButtonHovered));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, convertColorWithHueMod(ImGuiCol_ButtonActive));
    ImGui::PushStyleColor(ImGuiCol_Border, colorBlueOutline.withRotatedHue(uiState.uiButtonHueMod).toVec4<ImVec4>());
}

////////////////////////////////////////////////////////////
void Main::uiPopButtonColors()
{
    ImGui::PopStyleColor(4);
}

////////////////////////////////////////////////////////////
void Main::uiBeginTooltip(const float width)
{
    ImGui::SetNextWindowSizeConstraints(ImVec2(width, 0), ImVec2(width, ZA_FLOAT_MAX));

    ImGui::BeginTooltip();
    ImGui::PushFont(fontImGuiMouldyCheese);
    uiSetFontScale(uiToolTipFontScale);
}

////////////////////////////////////////////////////////////
void Main::uiEndTooltip()
{
    uiSetFontScale(uiNormalFontScale);
    ImGui::PopFont();
    ImGui::EndTooltip();
}

////////////////////////////////////////////////////////////
void Main::uiMakeTooltip(const bool small)
{
    if (!ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) || std::strlen(uiState.uiTooltipBuffer) == 0u)
        return;

    const float width = small ? 176.f : uiTooltipWidth;

    const ImVec2 mousePos = ImGui::GetMousePos();
    const float  tooltipX = mousePos.x - width < 8.f ? mousePos.x + 16.f : mousePos.x - width;

    ImGui::SetNextWindowPos(ImVec2(tooltipX, mousePos.y + (small ? -40.f : 20.f)));

    uiBeginTooltip(width);
    ImGui::TextWrapped("%s", uiState.uiTooltipBuffer);
    uiEndTooltip();
}

////////////////////////////////////////////////////////////
void Main::uiMakeShrineOrCatTooltip(const za::Vec2f mousePos)
{
    const auto* hoveredShrine = [&] -> const Shrine*
    {
        for (Shrine& shrine : pt->shrines)
            if ((mousePos - shrine.position).lengthSquared() <= shrine.getRadiusSquared())
                return &shrine;

        return nullptr;
    }();

    const auto* hoveredCat = [&] -> const Cat*
    {
        if (hoveredShrine == nullptr)
            for (const Cat& cat : pt->cats)
                if ((mousePos - cat.position).lengthSquared() <= cat.getRadiusSquared())
                    return &cat;

        return nullptr;
    }();

    if ((hoveredShrine == nullptr && hoveredCat == nullptr) || std::strlen(uiState.uiTooltipBuffer) == 0u)
        return;

    ImGui::SetNextWindowPos(ImVec2(getResolution().x - 15.f, getResolution().y - 15.f), 0, ImVec2(1, 1));
    uiBeginTooltip(uiTooltipWidth);

    if (hoveredShrine != nullptr)
    {
        std::sprintf(uiState.uiTooltipBuffer, "%s", shrineTooltipsByType[static_cast<SizeT>(hoveredShrine->type)] + 1);
    }
    else
    {
        const char* catNormalTooltip = R"(
~~ Cat ~~

Pops bubbles or bombs, whatever comes first. Not the brightest, despite not being orange.

Prestige points can be spent for their college tuition, making them more cleverer.)";

        if (pt->perm.geniusCatsPurchased)
        {
            catNormalTooltip =
                R"(
~~ Genius Cat ~~

A truly intelligent being: prioritizes popping bombs first, then star bubbles, then normal bubbles. Can be instructed to ignore specific bubble types.

Through the sheer power of their intellect, they also get a x2 multiplier on all bubble values.

We do not speak of the origin of the large brain attached to their body.)";
        }
        else if (pt->perm.smartCatsPurchased)
        {
            catNormalTooltip =
                R"(
~~ Smart Cat ~~

Pops bubbles or bombs. Smart enough to prioritize bombs and star bubbles over normal ones, but can't quite tell those two apart.

We do not speak of the tuition fees.)";
        }

        const char* catUniTooltip = R"(
~~ Unicat ~~

Imbued with the power of stars and rainbows, transforms bubbles into star bubbles worth x15 more.

Must have eaten something they weren't supposed to, because they keep changing color.
)";

        if (isUnicatTranscendenceActive())
            catUniTooltip = R"(
~~ Transcended Unicat ~~

Imbued with the power of the void, transforms bubbles into nova bubbles, worth x50 more.

Radiates a somewhat creepy energy.
)";

        const char* catDevilTooltip = R"(
~~ Devilcat ~~

Hired diplomat of the NB (NOBUBBLES) political party. Convinces bubbles to turn into bombs and explode for the rightful cause.

Collateral bubbles caught in the blast are worth x10 more.)";

        if (isDevilcatHellsingedActive())
            catDevilTooltip = R"(
~~ Hellsinged Devilcat ~~

Sold their soul to the devil. Opens up portals to hells that absorb bubbles with a x50 multiplier.

From politician to demon... the NB (NOBUBBLES) party is truly a mystery.)";

        const char* catAstroTooltip = R"(
~~ Astrocat ~~

Pride of the NCSA, a highly trained feline astronaut that continuously flies across the map, popping bubbles with a x20 multiplier.

Desperately trying to get funding from the government for a mission to the cheese moon. Perhaps some prestige points could help?)";

        if (pt->perm.astroCatInspirePurchased)
        {
            catAstroTooltip =
                R"(
~~ Propagandist Astrocat ~~

Pride of the NCSA, a highly trained feline astronaut that continuously flies across the map, popping bubbles with a x20 multiplier.

Finally financed by the NB (NOBUBBLES) political party to inspire other cats to work faster when flying by.)";
        }

        const char* catTooltipsByType[]{
            catNormalTooltip,
            catUniTooltip,
            catDevilTooltip,
            catAstroTooltip,
            R"(
~~ Wardencat ~~

The night-shift cat: gently wakes up sleeping cats nearby.

Will not sleep on the job. Probably. )",
            R"(
~~ Witchcat ~~
(unique cat)

Loves to perform rituals on other cats, hexing one at random.

Voodoo dolls of the cursed cat appear around the map - collect all of them to release the hex and trigger a powerful timed effect based on the victim.

Their dark magic is puzzling... but not as puzzling as the sheer number of dolls they carry around.)",
            R"(
~~ Wizardcat ~~
(unique cat)

Arcane feline capable of unleashing powerful spells -- if only they could remember them.

Absorbs the magic of star bubbles to recall past lives and recover lost incantations.

The scriptures say that they "unlock the Magic menu", but nobody knows what that means.

Hex buff: x3.5 faster mana regen buff.)",
            R"(
~~ Mousecat ~~
(unique cat)

Stole a Logicat gaming mouse and is now on the run. And yes, it still works unplugged.

Maintains a combo like manual popping and shares that combo multiplier with nearby cats.

Affected by both cat and click reward multipliers, including their own buff.

Also provides a global click reward multiplier just by existing -- Logicat does know how to make a good mouse.

Hex buff: x10 click reward buff.)",
            R"(
~~ Engicat ~~
(unique cat)

Periodically performs maintenance on all nearby cats, temporarily overclocking them.

Provides a global cat reward value multiplier just by existing. -- guess they're a "10x engineer"?

Hex buff: x2 global faster cat cooldown buff.)",
            R"(
~~ Repulsocat ~~
(unique cat)

Continuously pushes bubbles away with their powerful USB fan, powered by only Dog knows what kind of batteries. (Note: this effect is applied even while Repulsocat is being dragged.)

Bubbles being pushed away by Repulsocat are worth x2 more.

Using prestige points, the fan can be upgraded to filter specific bubble types and/or convert a percentage of bubbles to star bubbles.

Hex buff: x2 bubble count buff and increase wind speed.)",
            R"(
~~ Attractocat ~~
(unique cat)

Continuously attracts bubbles with their huge magnet. And yes, soap is magnetic -- don't ask.

Bubbles being attracted by Attractocat are worth x2 more.

Using prestige points, the magnet can be upgraded to filter specific bubble types.

Hex buff: all bombs or hell portals will attract bubbles.)",
            R"(
~~ Copycat ~~
(unique cat)

Mimics an existing unique cat, gaining their abilities and effects. Change the target via the toolbar at the bottom of the screen.

Identity crisis? Perhaps. But they're very good at it.

Mimicking Witchcat:
- Two separate rituals will be performed.

Mimicking Wizardcat:
- Spells will also be casted by the Copycat.
- Mewliplier Aura's bonus does not stack twice.

Mimicking Mousecat:
- The combo multiplier is shared.
- The global clicking buff multiplier is doubled.

Mimicking Engicat:
- The global clicking buff multiplier is doubled.

Hex buff: grant the same buff as the mimicked cat.)",
            R"(
~~ Duck ~~

It's a duck.)",
        };

        static_assert(za::getArraySize(catTooltipsByType) == nCatTypes);

        ZA_ASSERT(hoveredCat != nullptr);
        std::sprintf(uiState.uiTooltipBuffer, "%s", catTooltipsByType[static_cast<SizeT>(hoveredCat->type)] + 1);
    }

    ImGui::TextWrapped("%s", uiState.uiTooltipBuffer);
    uiState.uiTooltipBuffer[0] = '\0';

    uiEndTooltip();
}

////////////////////////////////////////////////////////////
Main::AnimatedButtonOutcome Main::uiAnimatedButton(
    const za::Texture& tx,
    const char*        label,
    const za::Vec2f    btnSize,
    const float        fontScale,
    const float        fontScaleMult,
    const float        btnSizeMult,
    const bool         forceHovered)
{
    ImGuiWindow* imGuiWindow = ImGui::GetCurrentWindow();

    if (imGuiWindow->SkipItems)
        return AnimatedButtonOutcome::None;

    const char* labelEnd = ImGui::FindRenderedTextEnd(label);

    const auto id = static_cast<ImGuiID>(std::strtoul(labelEnd + 2, nullptr, 10));

    const ImVec2 labelSize = ImGui::CalcTextSize(label, labelEnd, true);

    const ImVec2 size = ImGui::CalcItemSize(ImVec2{btnSize.x, btnSize.y},
                                            labelSize.x + ImGui::GetStyle().FramePadding.x * 2.f,
                                            labelSize.y + ImGui::GetStyle().FramePadding.y * 2.f);

    const ImRect bb(imGuiWindow->DC.CursorPos, imGuiWindow->DC.CursorPos + size);
    ImGui::ItemSize(bb);

    if (!ImGui::ItemAdd(bb, id))
        return AnimatedButtonOutcome::None;

    // Store animation state in window data
    struct AnimState
    {
        float hoverAnim;     // 0.f to 1.f for tilt
        float clickAnim;     // 0.f to 1.f for scale
        float lastClickTime; // in seconds
    };

    // Get or create animation state
    ImGuiStorage* storage     = imGuiWindow->DC.StateStorage;
    const ImGuiID animStateId = id + 1; // Use a different ID for the state

    auto* animState = static_cast<AnimState*>(storage->GetVoidPtr(animStateId));
    if (animState == nullptr)
    {
        static za::Vector<za::UniquePtr<AnimState>> animStateStorage;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
        animState = animStateStorage.emplaceBack(za::makeUnique<AnimState>(0.f, 0.f, -1.f)).get();
#pragma GCC diagnostic pop

        storage->SetVoidPtr(animStateId, animState);
    }

    const bool isCurrentlyDisabled  = (ImGui::GetItemFlags() & ImGuiItemFlags_Disabled) != 0;
    const bool hovered              = ImGui::ItemHoverable(bb, id, ImGuiItemFlags_None);
    const bool pressed              = !isCurrentlyDisabled && hovered && ImGui::IsMouseDown(0);
    const bool clicked              = !isCurrentlyDisabled && hovered && ImGui::IsMouseReleased(0);
    const bool clickedWhileDisabled = isCurrentlyDisabled && hovered && ImGui::IsMouseReleased(0);

    // Update animations
    const float deltaTime = ImGui::GetIO().DeltaTime;

    // Hover animation (tilt)
    animState->hoverAnim = hovered ? ImMin(animState->hoverAnim + deltaTime * 5.f, 1.f) : 0.f;

    // Click animation (scale)
    if (pressed)
        animState->lastClickTime = static_cast<float>(ImGui::GetTime());

    const float timeSinceClick = static_cast<float>(ImGui::GetTime()) - animState->lastClickTime;

    const float clickAnimDir = timeSinceClick > 0.f && timeSinceClick < 0.2f ? 1.f : -1.f;
    animState->clickAnim     = za::clamp(animState->clickAnim + deltaTime * 4.5f * clickAnimDir, 0.f, 1.f);

    // Save current cursor pos
    const ImVec2 originalPos = imGuiWindow->DC.CursorPos;
    ImDrawList*  drawList    = imGuiWindow->DrawList;

    // Calculate center point for transformations
    const ImVec2 center = bb.Min + size * 0.5f;

    // Apply transformations (with a small extra clip so nothing gets cut off)
    const ImVec2 clipMin{bb.Min.x - 20.f, bb.Min.y - 20.f};
    const ImVec2 clipMax{bb.Max.x + 20.f, bb.Max.y + 20.f};
    //    drawList->PushClipRect(clipMin, clipMax, true);
    drawList->PushClipRect(ImVec2(0, 0), ImGui::GetIO().DisplaySize, false);

    // Scale transform: shrink by up to 10% when clicked.
    const float scale = 1.f - easeInOutBack(animState->clickAnim) * 0.35f;

    // Tilt transform: rotate by up to 0.05 radians (≈2.9°). (Use ~0.0873f for 5°.)
    const float tiltAngle  = za::sin(easeInOutSine(animState->hoverAnim) * za::tau) * 0.1f;
    const float tiltCos    = za::cos(tiltAngle);
    const float tiltSin    = za::sin(tiltAngle);
    const float tiltCosOpp = za::cos(-tiltAngle);
    const float tiltSinOpp = za::sin(-tiltAngle);

    // Helper lambda: apply scale & rotation about the button center.
    const auto transformPoint = [&](const ImVec2& p) -> ImVec2
    {
        // Translate so that the center is at (0,0)
        const ImVec2 centered = p - center;

        // Apply scale
        const ImVec2 scaled = centered * scale;

        // Apply rotation
        const ImVec2 rotated{scaled.x * tiltCos - scaled.y * tiltSin, scaled.x * tiltSin + scaled.y * tiltCos};

        // Translate back
        return center + rotated;
    };

    const auto transformPointOpposite = [&](const ImVec2& p) -> ImVec2
    {
        // Translate so that the center is at (0,0)
        const ImVec2 centered = p - center;

        // Apply scale
        const ImVec2 scaled = centered * scale;

        // Apply rotation
        const ImVec2 rotated{scaled.x * tiltCosOpp - scaled.y * tiltSinOpp, scaled.x * tiltSinOpp + scaled.y * tiltCosOpp};

        // Translate back
        return center + rotated;
    };

    // ── Draw Button Background as a Rounded Rectangle ──
    /*
    const ImU32 btnBgColor = ImGui::GetColorU32(
        pressed   ? ImGuiCol_ButtonActive
        : hovered ? ImGuiCol_ButtonHovered
                  : ImGuiCol_Button);
*/
    const auto btnBgColor = isCurrentlyDisabled         ? ImColor(185, 185, 185, 255)
                            : (hovered || forceHovered) ? ImColor(255, 255, 255, 255)
                                                        : ImColor(235, 235, 235, 255);

    /*
        drawList->PathClear();
        drawList->PathRect(bb.Min, bb.Max, rounding);

        for (int i = 0; i < drawList->_Path.Size; ++i)
            drawList->_Path[i] = transformPoint(drawList->_Path[i]);

        drawList->PathFillConvex(btnBgColor);

        if (ImGui::GetStyle().FrameBorderSize > 0.f)
        {
            // Recreate the path for the border.
            drawList->PathClear();
            drawList->PathRect(bb.Min, bb.Max, rounding);

            for (int i = 0; i < drawList->_Path.Size; ++i)
                drawList->_Path[i] = transformPoint(drawList->_Path[i]);

            drawList->PathStroke(ImGui::GetColorU32(ImGuiCol_Border), ImDrawFlags_Closed, ImGui::GetStyle().FrameBorderSize);
        }
    */

    const ImTextureID textureID = toImTextureID(tx.getNativeHandle());

    // transformed points as quad
    auto offset = ImVec2{4.5f, 10.f} * btnSizeMult;

    if (hovered || forceHovered)
        offset *= 1.7f;

    const auto p0 = transformPointOpposite(ImVec2{bb.Min.x, bb.Min.y} - offset);
    const auto p1 = transformPointOpposite(ImVec2{bb.Max.x + offset.x, bb.Min.y - offset.y});
    const auto p2 = transformPointOpposite(ImVec2{bb.Max.x, bb.Max.y} + offset);
    const auto p3 = transformPointOpposite(ImVec2{bb.Min.x - offset.x, bb.Max.y + offset.y});

    const auto uv0 = ImVec2(0, 0);
    const auto uv1 = ImVec2(1, 0);
    const auto uv2 = ImVec2(1, 1);
    const auto uv3 = ImVec2(0, 1);


    if (id % 2 == 0)
    {
        drawList->AddImageQuad(textureID, p0, p1, p2, p3, uv0, uv1, uv2, uv3, btnBgColor);
    }
    else
    {
        // flip horizontally
        drawList->AddImageQuad(textureID, p1, p0, p3, p2, uv0, uv1, uv2, uv3, btnBgColor);
    }

    // ── Draw Text with the Same Transformation ──

    // First, compute the untransformed text position.

    // Record the current vertex buffer size...
    const int vtxBufferSizeBeforeTransformation = drawList->VtxBuffer.Size;

    // ...and add the text at its normal (unrotated/unscaled) position.
    uiSetFontScale(fontScale * fontScaleMult);


    const auto textSize = ImGui::CalcTextSize(label, labelEnd, true);

    // center text in button
    const ImVec2 textPos = bb.Min + (size - textSize) * 0.5f + ImVec2(0, ImGui::GetStyle().FramePadding.y * 0.5f);

    drawList->AddText(textPos, ImGui::GetColorU32(isCurrentlyDisabled ? ImGuiCol_TextDisabled : ImGuiCol_Text), label, labelEnd);

    // Then, apply our transform to only the new text vertices.
    for (int i = vtxBufferSizeBeforeTransformation; i < drawList->VtxBuffer.Size; ++i)
        drawList->VtxBuffer[i].pos = transformPoint(drawList->VtxBuffer[i].pos);

    // Restore the previous clip rect and cursor position.
    drawList->PopClipRect();
    imGuiWindow->DC.CursorPos = originalPos;

    return clicked                ? AnimatedButtonOutcome::Clicked
           : clickedWhileDisabled ? AnimatedButtonOutcome::ClickedWhileDisabled
                                  : AnimatedButtonOutcome::None;
}

////////////////////////////////////////////////////////////
bool Main::uiMakeButtonImpl(const char* label, const char* xBuffer)
{
    const bool  subBullet = label[0] == ' ' && label[1] == ' ';
    const float fontScale = subBullet ? uiSubBulletFontScale : uiNormalFontScale;

    float fontScaleMult = 1.f;

    const auto xBufferLen = ZA_STRLEN(xBuffer);

    if (xBufferLen > 20)
        fontScaleMult = 0.65f;
    else if (xBufferLen > 19)
        fontScaleMult = 0.7f;
    else if (xBufferLen > 18)
        fontScaleMult = 0.8f;
    else if (xBufferLen > 16)
        fontScaleMult = 0.9f;
    else
        fontScaleMult = 1.f;

    if (fontScale == uiSubBulletFontScale)
        fontScaleMult = za::pow(fontScaleMult, 0.4f);

    const float scaledButtonWidth = uiButtonWidth * profile.uiScale;

    ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - scaledButtonWidth - 2.5f, 0.f)); // Push to right
    ImGui::SameLine();

    uiPushButtonColors();

    bool clicked = false;
    if (const auto outcome = uiAnimatedButton(txCloudBtn, xBuffer, {scaledButtonWidth, 0.f}, fontScale, fontScaleMult);
        outcome == AnimatedButtonOutcome::Clicked)
    {
        playSound(sounds.buy);
        clicked = true;

        for (SizeT i = 0u; i < 24u; ++i)
            spawnHUDTopParticle({.position      = getHUDMousePos(),
                                 .velocity      = rngFast.getVec2f({-0.75f, -0.75f}, {0.75f, 0.75f}),
                                 .scale         = rngFast.getF(0.08f, 0.27f) * 0.7f,
                                 .scaleDecay    = 0.f,
                                 .accelerationY = 0.002f,
                                 .opacity       = 1.f,
                                 .opacityDecay  = rngFast.getF(0.00025f, 0.0015f),
                                 .rotation      = rngFast.getF(0.f, za::tau),
                                 .torque        = rngFast.getF(-0.002f, 0.002f)},
                                /* hue */ wrapHue(165.f + uiState.uiButtonHueMod + currentBackgroundHue.asDegrees()),
                                ParticleType::Star);
    }
    else if (outcome == AnimatedButtonOutcome::ClickedWhileDisabled)
    {
        playSound(sounds.failpopui);

        for (SizeT i = 0u; i < 6u; ++i)
            spawnHUDTopParticle({.position      = getHUDMousePos(),
                                 .velocity      = rngFast.getVec2f({-0.75f, -0.75f}, {0.75f, 0.75f}) * 0.5f,
                                 .scale         = rngFast.getF(0.08f, 0.27f),
                                 .scaleDecay    = 0.f,
                                 .accelerationY = 0.002f * 0.75f,
                                 .opacity       = 1.f,
                                 .opacityDecay  = rngFast.getF(0.00025f, 0.0015f),
                                 .rotation      = rngFast.getF(0.f, za::tau),
                                 .torque        = rngFast.getF(-0.002f, 0.002f)},
                                /* hue */ 0.f,
                                ParticleType::Bubble);
    }

    uiPopButtonColors();

    uiMakeTooltip();

    if (label[0] == ' ')
        uiSetFontScale(uiNormalFontScale);

    ImGui::NextColumn();
    return clicked;
}

////////////////////////////////////////////////////////////
bool Main::makePSVButtonExByCurrency(
    const char*              label,
    PurchasableScalingValue& psv,
    const SizeT              times,
    const MoneyType          cost,
    MoneyType&               availability,
    const char*              currencyFmt)
{
    const bool maxedOut = psv.nPurchases == psv.data->nMaxPurchases;

    if (profile.hideMaxedOutPurchasables && maxedOut)
        return false;

    bool result = false;

    if (maxedOut)
        std::sprintf(uiState.uiBuffer, "MAX##%u", uiState.uiWidgetId++);
    else if (cost == 0u || times == 0u)
        std::sprintf(uiState.uiBuffer, "N/A##%u", uiState.uiWidgetId++);
    else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
        std::sprintf(uiState.uiBuffer, currencyFmt, toStringWithSeparators(cost), uiState.uiWidgetId++);
#pragma GCC diagnostic pop

    ImGui::BeginDisabled(uiCheckPurchasability(label, maxedOut || availability < cost || cost == 0u));

    uiMakeButtonLabels(label, uiState.uiLabelBuffer);
    if (uiMakeButtonImpl(label, uiState.uiBuffer))
    {
        result = true;
        availability -= cost;

        if (&availability == &pt->money)
            spentMoney += cost;

        psv.nPurchases += times;

        if (&availability == &pt->prestigePoints && times == 1u)
        {
            undoPPPurchase.emplaceBack([&psv, &availability, times, cost]
            {
                psv.nPurchases -= times;
                availability += cost;
            });

            undoPPPurchaseTimer.time = 10000.f;
        }
    }

    ImGui::EndDisabled();
    return result;
}

////////////////////////////////////////////////////////////
bool Main::makePSVButtonEx(const char* label, PurchasableScalingValue& psv, const SizeT times, const MoneyType cost)
{
    return makePSVButtonExByCurrency(label, psv, times, cost, pt->money, "$%s##%u");
}

////////////////////////////////////////////////////////////
bool Main::makePSVButton(const char* label, PurchasableScalingValue& psv)
{
    return makePSVButtonEx(label, psv, 1u, static_cast<MoneyType>(psv.nextCost()));
}

////////////////////////////////////////////////////////////
bool Main::uiCheckPurchasability(const char* label, const bool disabled)
{
    uiState.uiLabelToY[label] = ImGui::GetCursorScreenPos().y;

    if (disabled)
    {
        uiState.btnWasDisabled[label] = true;
    }
    else if (uiState.btnWasDisabled[label] && !disabled)
    {
        uiState.btnWasDisabled[label] = false;

        const bool anyPurchaseUnlockedEffectWithSameLabel = za::anyOf(uiState.purchaseUnlockedEffects.begin(),
                                                                      uiState.purchaseUnlockedEffects.end(),
                                                                      [&](const PurchaseUnlockedEffect& effect)
        { return effect.widgetLabel == label; });

        const bool anyPurchaseUnlockedEffectWithSameY = za::anyOf(uiState.purchaseUnlockedEffects.begin(),
                                                                  uiState.purchaseUnlockedEffects.end(),
                                                                  [&](const PurchaseUnlockedEffect& effect)
        {
            const auto* it = uiState.uiLabelToY.find(effect.widgetLabel);
            return it != uiState.uiLabelToY.end() && it->second == uiState.uiLabelToY[label];
        });

        if (!anyPurchaseUnlockedEffectWithSameLabel && !anyPurchaseUnlockedEffectWithSameY)
        {
            uiState.purchaseUnlockedEffects.pushBack({
                .widgetLabel    = label,
                .countdown      = Countdown{.time = 1000.f},
                .arrowCountdown = Countdown{.time = 2000.f},
                .hue            = uiState.uiButtonHueMod,
                .type           = 1, // now purchasable
            });

            playSound(sounds.purchasable, /* maxOverlap */ 1u);
            playSound(sounds.shimmer, /* maxOverlap */ 1u);
        }
    }

    return disabled;
}

////////////////////////////////////////////////////////////
bool Main::makePurchasableButtonOneTime(const char* label, const MoneyType cost, bool& done)
{
    return makePurchasableButtonOneTimeByCurrency(label, done, cost, pt->money, "$%s##%u");
}

////////////////////////////////////////////////////////////
bool Main::makePurchasableButtonOneTimeByCurrency(
    const char*     label,
    bool&           done,
    const MoneyType cost,
    MoneyType&      availability,
    const char*     currencyFmt)
{
    bool result = false;

    if (done)
        std::sprintf(uiState.uiBuffer, "DONE##%u", uiState.uiWidgetId++);
    else if (cost == 0u)
        std::sprintf(uiState.uiBuffer, "FREE##%u", uiState.uiWidgetId++);
    else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
        std::sprintf(uiState.uiBuffer, currencyFmt, toStringWithSeparators(cost), uiState.uiWidgetId++);
#pragma GCC diagnostic pop

    ImGui::BeginDisabled(uiCheckPurchasability(label, done || availability < cost));

    uiMakeButtonLabels(label, uiState.uiLabelBuffer);
    if (uiMakeButtonImpl(label, uiState.uiBuffer))
    {
        result = true;
        availability -= cost;

        if (&availability == &pt->money)
            spentMoney += cost;

        done = true;

        if (&availability == &pt->prestigePoints && cost > 0u)
        {
            undoPPPurchase.emplaceBack([&availability, &done, cost]
            {
                done = false;
                availability += cost;
            });

            undoPPPurchaseTimer.time = 10000.f;
        }
    }

    ImGui::EndDisabled();
    return result;
}

////////////////////////////////////////////////////////////
bool Main::makePurchasablePPButtonOneTime(const char* label, const PrestigePointsType prestigePointsCost, bool& done)
{
    return makePurchasableButtonOneTimeByCurrency(label, done, prestigePointsCost, pt->prestigePoints, "%s PPs##%u");
}

////////////////////////////////////////////////////////////
bool Main::makePrestigePurchasablePPButtonPSV(const char* label, PurchasableScalingValue& psv)
{
    return makePSVButtonExByCurrency(label,
                                     psv,
                                     /* times */ 1u,
                                     /* cost */ static_cast<PrestigePointsType>(psv.nextCost()),
                                     /* availability */ pt->prestigePoints,
                                     "%s PPs##%u");
}

////////////////////////////////////////////////////////////
void Main::uiBeginColumns() const
{
    ImGui::Columns(2, "twoColumns", false);
    ImGui::SetColumnWidth(0, (uiWindowWidth - uiButtonWidth - 40.f) * profile.uiScale);
    ImGui::SetColumnWidth(1, (uiButtonWidth + 10.f) * profile.uiScale);
}

////////////////////////////////////////////////////////////
void Main::uiCenteredText(const char* str, const float offsetX, const float offsetY)
{
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(str).x) * 0.5f + offsetX);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offsetY);

    ImGui::Text("%s", str);
}

////////////////////////////////////////////////////////////
void Main::uiCenteredTextColored(const za::Color color, const char* str, const float offsetX, const float offsetY)
{
    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(str).x) * 0.5f + offsetX);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + offsetY);

    ImGui::TextColored(color.toVec4<ImVec4>(), "%s", str);
}

////////////////////////////////////////////////////////////
za::Vec2f Main::uiGetWindowPos() const
{
    const float scaledWindowWidth = uiWindowWidth * profile.uiScale;
    const float rightAnchorX      = getResolution().x - scaledWindowWidth - 15.f * profile.uiScale;

    return {rightAnchorX - 8.f, 0.f};
}

////////////////////////////////////////////////////////////
void Main::uiClearLabel()
{
    uiState.uiLabelBuffer[0] = '\0';
}

////////////////////////////////////////////////////////////
void Main::uiSetLabel(const char* const fmt, ...)
{
    std::va_list args;
    va_start(args, fmt);
    std::vsnprintf(uiState.uiLabelBuffer, UIState::uiLabelBufferSize, fmt, args);
    va_end(args);
}

////////////////////////////////////////////////////////////
void Main::uiSetTooltip(const char* const fmt, ...)
{
    std::va_list args;
    va_start(args, fmt);
    std::vsnprintf(uiState.uiTooltipBuffer, UIState::uiTooltipBufferSize, fmt, args);
    va_end(args);
}

////////////////////////////////////////////////////////////
void Main::uiSetTooltipOnly(const char* const fmt, ...)
{
    std::va_list args;
    va_start(args, fmt);
    std::vsnprintf(uiState.uiTooltipBuffer, UIState::uiTooltipBufferSize, fmt, args);
    va_end(args);

    uiClearLabel();
}

////////////////////////////////////////////////////////////
bool Main::uiMakePrestigeOneTimeButton(const char* const        buttonLabel,
                                       const PrestigePointsType cost,
                                       bool&                    done,
                                       const char* const        tooltipFmt,
                                       ...)
{
    std::va_list args;
    va_start(args, tooltipFmt);
    std::vsnprintf(uiState.uiTooltipBuffer, UIState::uiTooltipBufferSize, tooltipFmt, args);
    va_end(args);

    uiClearLabel();
    return makePurchasablePPButtonOneTime(buttonLabel, cost, done);
}

////////////////////////////////////////////////////////////
void Main::uiDrawExitPopup(const float newScalingFactor)
{
    if (!playerInputState.escWasPressed)
        return;

    constexpr float scaleMult = 1.25f;

    ImGui::SetNextWindowPos({getResolution().x / 2.f, getResolution().y / 2.f}, 0, {0.5f, 0.5f});
    ImGui::SetNextWindowSizeConstraints(ImVec2(400.f * scaleMult * newScalingFactor, 0.f),
                                        ImVec2(400.f * scaleMult * newScalingFactor, 300.f * scaleMult * newScalingFactor));

    ImGui::Begin("##exit",
                 nullptr,
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);

    uiDrawCloudWindowBackground();

    uiSetFontScale(scaleMult);

    uiCenteredText("Are you sure you want to exit?");

    ImGui::Dummy(ImVec2(0.f, 10.f * scaleMult));
    const float exitButtonWidth = 60.f * scaleMult * profile.uiScale;

    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - exitButtonWidth * 2.f - 5.f) * 0.5f);

    ImGui::BeginGroup();

    if (ImGui::Button("Yes", ImVec2(exitButtonWidth, 0.f)))
    {
        playSound(sounds.uitab);
        mustExit = true;
    }

    ImGui::SameLine();

    if (ImGui::Button("No", ImVec2(exitButtonWidth, 0.f)))
    {
        playSound(sounds.uitab);
        playerInputState.escWasPressed = false;
    }

    ImGui::EndGroup();

    uiSetFontScale(uiNormalFontScale);

    ImGui::End();
}

////////////////////////////////////////////////////////////
bool Main::uiCheckbox(const char* label, bool* b)
{
    if (!ImGui::Checkbox(label, b))
        return false;

    playSound(sounds.btnswitch);
    return true;
}

////////////////////////////////////////////////////////////
bool Main::uiRadio(const char* label, int* i, const int value)
{
    if (!ImGui::RadioButton(label, i, value))
        return false;

    playSound(sounds.btnswitch);
    return true;
}


////////////////////////////////////////////////////////////
void Main::uiSetUnlockLabelY(const za::SizeT unlockId)
{
    const za::String label    = za::toString(unlockId);
    uiState.uiLabelToY[label] = ImGui::GetCursorScreenPos().y;
}

////////////////////////////////////////////////////////////
bool Main::checkUiUnlock(const za::SizeT unlockId, const bool unlockCondition)
{
    const za::String label = za::toString(unlockId);

    if (!unlockCondition)
    {
        profile.uiUnlocks[unlockId] = false;
        return false;
    }

    if (!profile.uiUnlocks[unlockId])
    {
        profile.uiUnlocks[unlockId] = true;

        const bool anyPurchaseUnlockedEffectWithSameLabel = za::anyOf(uiState.purchaseUnlockedEffects.begin(),
                                                                      uiState.purchaseUnlockedEffects.end(),
                                                                      [&](const PurchaseUnlockedEffect& effect)
        { return effect.widgetLabel == label; });

        const bool anyPurchaseUnlockedEffectWithSameY = za::anyOf(uiState.purchaseUnlockedEffects.begin(),
                                                                  uiState.purchaseUnlockedEffects.end(),
                                                                  [&](const PurchaseUnlockedEffect& effect)
        {
            const auto* it = uiState.uiLabelToY.find(effect.widgetLabel);
            return it != uiState.uiLabelToY.end() && it->second == uiState.uiLabelToY[label];
        });

        if (!anyPurchaseUnlockedEffectWithSameLabel && !anyPurchaseUnlockedEffectWithSameY)
        {
            uiState.purchaseUnlockedEffects.pushBack({
                .widgetLabel    = label,
                .countdown      = Countdown{.time = 1000.f},
                .arrowCountdown = Countdown{.time = 2000.f},
                .hue            = uiState.uiButtonHueMod,
                .type           = 0, // now unlocked
            });

            playSound(sounds.unlock, /* maxOverlap */ 1u);
            playSound(sounds.shimmer, /* maxOverlap */ 1u);
        }
    }

    return true;
}


////////////////////////////////////////////////////////////
void Main::uiImageFromAtlas(const za::Rect2f& txr, const za::DrawTextureSettings& drawParams)
{
    imGuiContext.image(
        za::Sprite{
            .position    = drawParams.position,
            .scale       = drawParams.scale * profile.uiScale,
            .origin      = txr.size,
            .textureRect = txr,
        },
        uiTextureAtlas.getTexture(),
        za::Color{50u, 84u, 135u});
}

////////////////////////////////////////////////////////////
void Main::uiImgsep(const za::Rect2f& txr, const char* sepLabel, const bool first)
{
    if (profile.hideCategorySeparators)
        return;

    if (!first)
    {
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::Spacing();
    }

    ImGui::Columns(1);
    uiImageFromAtlas(txr, {.scale = {0.46f, 0.5f}});

    const auto oldFontScale = uiState.lastFontScale;
    uiSetFontScale(0.75f);
    uiCenteredText(sepLabel, -8.f * profile.uiScale, -8.f * profile.uiScale);
    uiSetFontScale(oldFontScale);

    // ImGui::Separator();
    ImGui::Spacing();

    uiBeginColumns();
}

////////////////////////////////////////////////////////////
void Main::uiImgsep2(const za::Rect2f& txr, const char* sepLabel)
{
    if (profile.hideCategorySeparators)
        return;

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Spacing();

    ImGui::Columns(1);

    uiImageFromAtlas(txr, {.scale = {0.46f, 0.5f}});

    const auto oldFontScale = uiState.lastFontScale;
    uiSetFontScale(0.75f);
    uiCenteredText(sepLabel, -8.f * profile.uiScale, -6.f * profile.uiScale);
    uiSetFontScale(oldFontScale);

    uiBeginColumns();
}

////////////////////////////////////////////////////////////

void Main::gameLoopDrawImGui(const za::U8 shouldDrawUIAlpha)
{
    SFEX_PROFILE_SCOPE_AUTOLABEL();

    rtImGui.clear(za::Color::Transparent);
    imGuiContext.render(rtImGui);
    rtImGui.display();

    rtGame.draw(rtImGui.getTexture(),
                {.scale = {1.f / profile.hudScale, 1.f / profile.hudScale},
                 .color = hueColor(currentBackgroundHue.asDegrees(), shouldDrawUIAlpha)},
                {.view = scaledHUDView, .shader = &shaders.shader});
}

////////////////////////////////////////////////////////////
void Main::gameLoopUpdateNotificationQueue(const float deltaTimeMs)
{
    if (tipTCByte.hasValue())
        return;

    if (notificationState.queue.empty())
        return;

    if (notificationState.countdown.tick(deltaTimeMs) == TickResult::Running)
        return;

    notificationState.countdown.restart();

    const auto& notification = notificationState.queue.front();

    ImGuiToast toast{ImGuiToastType::None, 4500};
    toast.setWindowFlags(NOTIFY_DEFAULT_TOAST_FLAGS | ImGuiWindowFlags_NoBackground);
    toast.setTitle("%s", notification.title);
    toast.setContent("%s", notification.content.cStr());

    ImGui::InsertNotification(toast);
    playSound(sounds.notification);

    // pop front
    notificationState.queue.erase(notificationState.queue.begin());
}
