#include "Aliases.hpp"
#include "BubbleIdleHelpers.hpp"
#include "BubbleIdleMain.hpp"
#include "Cat.hpp"
#include "CatConstants.hpp"
#include "CatType.hpp"
#include "IconsFontAwesome6.h"
#include "MainShaders.hpp"
#include "ParticleData.hpp"
#include "ParticleType.hpp"
#include "Playthrough.hpp"
#include "Profile.hpp"
#include "UIState.hpp"

#include "Zancle/Graphics/Color.hpp"
#include "Zancle/Graphics/DrawableBatch.hpp"
#include "Zancle/Graphics/RectangleShapeData.hpp"
#include "Zancle/Graphics/RenderWindow.hpp"

#include "Zancle/ImGui/IncludeImGui.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Math/Clamp.hpp"
#include "Zancle/Math/Constants.hpp"

#include "Zancle/Base/Limits.hpp"
#include "Zancle/Base/SizeT.hpp"

#include <cstdio>

////////////////////////////////////////////////////////////
void Main::uiDrawCloudWindowBackground()
{
    const ImVec2 winPos  = ImGui::GetWindowPos();
    const ImVec2 winSize = ImGui::GetWindowSize();

    const za::Vec2f pMin{winPos.x, winPos.y};
    const za::Vec2f pMax{winPos.x + winSize.x, winPos.y + winSize.y};

    cpuCloudUiDrawableBatch.add(za::RectangleShapeData{
        .position  = pMin,
        .fillColor = za::Color::White,
        .size      = pMax - pMin,
    });

    const int xSteps = za::clamp(static_cast<int>(winSize.x / 28.f), 3, 24);
    const int ySteps = za::clamp(static_cast<int>(winSize.y / 28.f), 3, 24);

    drawCloudFrame({
        .time              = shaderTime,
        .mins              = pMin,
        .maxs              = pMax,
        .xSteps            = xSteps,
        .ySteps            = ySteps,
        .scaleMult         = 1.6f,
        .outwardOffsetMult = 1.f,
        .batch             = &cpuCloudUiDrawableBatch,
    });
}


////////////////////////////////////////////////////////////
bool Main::uiDrawQuickbarIconButton(const char* label, const bool selected, const float scaleMult)
{
    constexpr TabButtonPalette palette{
        .idle    = za::Color::fromFloats(0.15f, 0.35f, 0.60f, 1.0f),
        .hovered = za::Color::fromFloats(0.25f, 0.45f, 0.80f, 1.0f),
        .active  = za::Color::fromFloats(0.35f, 0.55f, 0.95f, 1.0f),
    };

    ImGui::PushStyleColor(ImGuiCol_Button, (selected ? palette.active : palette.idle).toVec4<ImVec4>());
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (selected ? palette.active : palette.hovered).toVec4<ImVec4>());
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, palette.active.toVec4<ImVec4>());
    ImGui::PushStyleColor(ImGuiCol_Border, (selected ? palette.active : palette.hovered).toVec4<ImVec4>());
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(ImGui::GetStyle().FramePadding.x + 1.f, -6.f * scaleMult));

    ImGui::SetCursorPosY(ImGui::GetCursorStartPos().y - 10.f * scaleMult);

    const auto outcome = uiAnimatedButton(txCloudBtnSquare2,
                                          label,
                                          {36.f * profile.uiScale * scaleMult, 22.f * profile.uiScale * scaleMult},
                                          /* fontScale */ 1.35f * scaleMult,
                                          /* fontScaleMult */ 1.0f,
                                          /* btnSizeMult */ 1.3125f * scaleMult,
                                          /* forceHovered */ selected);

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);

    return outcome == Main::AnimatedButtonOutcome::Clicked;
}


////////////////////////////////////////////////////////////
void Main::uiDrawQuickbarCopyCat(const za::Vec2f quickBarPos, Cat& copyCat)
{
    const bool asWitchAndBusy = pt->copycatCopiedCatType == CatType::Witch && !pt->copyHexSessions.empty();

    const bool asWizardAndBusy = pt->copycatCopiedCatType == CatType::Wizard && isWizardBusy();

    const bool mustDisable = asWitchAndBusy || asWizardAndBusy;

    ImGui::BeginDisabled(mustDisable);

    constexpr const char* popupLabel = "CopyCatSelectorPopup";

    const bool pressed = uiDrawQuickbarIconButton(ICON_FA_MASKS_THEATER "##800", ImGui::IsPopupOpen(popupLabel));

    std::sprintf(uiState.uiTooltipBuffer, "Select Copycat mask");
    uiMakeTooltip(/* small */ true);

    ImGui::SameLine();

    if (pressed)
    {
        ImGui::OpenPopup(popupLabel);
        playSound(sounds.uitab);
    }

    ImGui::SetNextWindowPos(ImVec2(quickBarPos) - ImVec2{0.f, 64.f}, 0, {0.f, 1.f});
    if (ImGui::BeginPopup(popupLabel, ImGuiWindowFlags_NoBackground))
    {
        uiDrawCloudWindowBackground();

        ImGui::SetNextItemWidth(210.f * profile.uiScale);

        if (ImGui::BeginCombo("##copycatsel", CatConstants::typeNamesLong[asIdx(pt->copycatCopiedCatType)]))
        {
            for (SizeT i = asIdx(CatType::Normal); i < nCatTypes; ++i)
            {
                if (static_cast<CatType>(i) == CatType::Duck)
                    continue;

                if (!isUniqueCatType(static_cast<CatType>(i)))
                    continue;

                if (i == asIdx(CatType::Copy))
                    continue;

                if (findFirstCatByType(static_cast<CatType>(i)) == nullptr)
                    continue;

                const bool isSelected = pt->copycatCopiedCatType == static_cast<CatType>(i);
                if (ImGui::Selectable(CatConstants::typeNamesLong[i], isSelected))
                {
                    statDisguise();
                    pt->copycatCopiedCatType = static_cast<CatType>(i);

                    copyCat.cooldown.time = pt->getComputedCooldownByCatType(pt->copycatCopiedCatType);
                    copyCat.hits          = 0u;

                    sounds.smokebomb.settings.position = {copyCat.position.x, copyCat.position.y};
                    sounds.smokebomb.settings.position = {copyCat.position.x, copyCat.position.y};
                    playSound(sounds.smokebomb);

                    for (za::SizeT iP = 0u; iP < 8u; ++iP)
                        spawnParticle(ParticleData{.position   = copyCat.position,
                                                   .velocity   = {rngFast.getF(-0.15f, 0.15f), rngFast.getF(0.f, 0.1f)},
                                                   .scale      = rngFast.getF(0.75f, 1.f),
                                                   .scaleDecay = -0.0005f,
                                                   .accelerationY = -0.00017f,
                                                   .opacity       = 1.f,
                                                   .opacityDecay  = rngFast.getF(0.00065f, 0.00075f),
                                                   .rotation      = rngFast.getF(0.f, za::tau),
                                                   .torque        = rngFast.getF(-0.002f, 0.002f)},
                                      0.f,
                                      ParticleType::Smoke);

                    playSound(sounds.uitab);
                    ImGui::CloseCurrentPopup();
                }

                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }

        ImGui::EndPopup();
    }

    ImGui::EndDisabled();
}

////////////////////////////////////////////////////////////
void Main::uiDrawQuickbarBackgroundSelector(const za::Vec2f quickBarPos)
{
    constexpr const char* popupLabel = "BackgroundSelectorPopup";

    const bool pressed = uiDrawQuickbarIconButton(ICON_FA_PALETTE "##801", ImGui::IsPopupOpen(popupLabel));

    std::sprintf(uiState.uiTooltipBuffer, "Select background");
    uiMakeTooltip(/* small */ true);

    ImGui::SameLine();

    if (pressed)
    {
        ImGui::OpenPopup(popupLabel);
        playSound(sounds.uitab);
    }

    ImGui::SetNextWindowPos(ImVec2(quickBarPos) - ImVec2{0.f, 64.f}, 0, {0.f, 1.f});
    if (ImGui::BeginPopup(popupLabel, ImGuiWindowFlags_NoBackground))
    {
        uiDrawCloudWindowBackground();

        auto& [entries, selectedIndex] = getBackgroundSelectorData();

        ImGui::SetNextItemWidth(210.f * profile.uiScale);

        if (ImGui::BeginCombo("##backgroundsel", entries[static_cast<za::SizeT>(selectedIndex)].name))
        {
            for (SizeT i = 0u; i < entries.size(); ++i)
            {
                const bool isSelected = selectedIndex == static_cast<int>(i);
                if (ImGui::Selectable(entries[i].name, isSelected))
                {
                    selectedIndex = static_cast<int>(i);

                    selectBackground(entries, static_cast<int>(i));

                    playSound(sounds.uitab);
                    ImGui::CloseCurrentPopup();
                }

                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }

        ImGui::EndPopup();
    }
}

////////////////////////////////////////////////////////////
void Main::uiDrawQuickbarBGMSelector(const za::Vec2f quickBarPos)
{
    constexpr const char* popupLabel = "MusicSelectorPopup";

    const bool pressed = uiDrawQuickbarIconButton(ICON_FA_MUSIC "##802", ImGui::IsPopupOpen(popupLabel));

    std::sprintf(uiState.uiTooltipBuffer, "Select music");
    uiMakeTooltip(/* small */ true);

    ImGui::SameLine();

    if (pressed)
    {
        ImGui::OpenPopup(popupLabel);
        playSound(sounds.uitab);
    }

    ImGui::SetNextWindowPos(ImVec2(quickBarPos) - ImVec2{0.f, 64.f}, 0, {0.f, 1.f});
    if (ImGui::BeginPopup(popupLabel, ImGuiWindowFlags_NoBackground))
    {
        uiDrawCloudWindowBackground();

        auto& [entries, selectedIndex] = getBGMSelectorData();

        ImGui::SetNextItemWidth(210.f * profile.uiScale);

        if (ImGui::BeginCombo("##musicsel", entries[static_cast<za::SizeT>(selectedIndex)].name))
        {
            for (SizeT i = 0u; i < entries.size(); ++i)
            {
                const bool isSelected = selectedIndex == static_cast<int>(i);
                if (ImGui::Selectable(entries[i].name, isSelected))
                {
                    selectedIndex = static_cast<int>(i);

                    selectBGM(entries, static_cast<int>(i));
                    switchToBGM(static_cast<za::SizeT>(profile.selectedBGM), /* force */ false);

                    playSound(sounds.uitab);
                    ImGui::CloseCurrentPopup();
                }

                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }

        ImGui::EndPopup();
    }
}

////////////////////////////////////////////////////////////
void Main::uiDrawQuickbarQuickSettings(const za::Vec2f quickBarPos)
{
    constexpr const char* popupLabel = "QuickSettingsPopup";

    const bool pressed = uiDrawQuickbarIconButton(ICON_FA_SLIDERS "##803", ImGui::IsPopupOpen(popupLabel));

    std::sprintf(uiState.uiTooltipBuffer, "Quick settings");
    uiMakeTooltip(/* small */ true);

    ImGui::SameLine();

    if (pressed)
    {
        ImGui::OpenPopup(popupLabel);
        playSound(sounds.uitab);
    }

    ImGui::SetNextWindowPos(ImVec2(quickBarPos) - ImVec2{0.f, 64.f}, 0, {0.f, 1.f});
    if (ImGui::BeginPopup(popupLabel, ImGuiWindowFlags_NoBackground))
    {
        uiDrawCloudWindowBackground();

        uiCheckbox("Enable tips", &profile.tipsEnabled);
        uiCheckbox("Enable notifications", &profile.enableNotifications);

        ImGui::Separator();

        uiCheckbox("Enable $/s meter", &profile.showDpsMeter);

        ImGui::Separator();

        uiCheckbox("Show cat range", &profile.showCatRange);
        uiCheckbox("Show ranges only on hover", &profile.showRangesOnlyOnHover);
        uiCheckbox("Show cat text", &profile.showCatText);

        ImGui::Separator();

        uiCheckbox("Show particles", &profile.showParticles);

        ImGui::BeginDisabled(!profile.showParticles);
        uiCheckbox("Show coin particles", &profile.showCoinParticles);
        ImGui::EndDisabled();

        uiCheckbox("Show text particles", &profile.showTextParticles);

        ImGui::Separator();

        uiCheckbox("Enable screen shake", &profile.enableScreenShake);

        ImGui::Separator();

        ImGui::SetNextItemWidth(210.f * profile.uiScale);
        ImGui::SliderFloat("Background Opacity", &profile.backgroundOpacity, 0.f, 100.f, "%.f%%");

        uiCheckbox("Always show drawings", &profile.alwaysShowDrawings);

        ImGui::Separator();

        ImGui::SetNextItemWidth(210.f * profile.uiScale);
        ImGui::SliderFloat("Minimap Scale", &profile.minimapScale, 5.f, 40.f, "%.2f");

        ImGui::SetNextItemWidth(210.f * profile.uiScale);
        ImGui::SliderFloat("HUD Scale", &profile.hudScale, 0.5f, 2.f, "%.2f");

        ImGui::AlignTextToFramePadding();
        ImGui::Text("UI Scale");

        const auto makeUIScaleButton = [&](const char* label, const float scaleFactor)
        {
            ImGui::SameLine();
            if (ImGui::Button(label, ImVec2{46.f * profile.uiScale, 0.f}))
            {
                playSound(sounds.buy);
                profile.uiScale = scaleFactor;
            }
        };

        makeUIScaleButton("XXL", 1.75f);
        makeUIScaleButton("XL", 1.5f);
        makeUIScaleButton("L", 1.25f);
        makeUIScaleButton("M", 1.f);
        makeUIScaleButton("S", 0.75f);
        makeUIScaleButton("XS", 0.5f);

        ImGui::Separator();

        uiCheckbox("Bubble shaders.shader", &profile.useBubbleShader);

        ImGui::BeginDisabled(!profile.useBubbleShader);
        ImGui::SetNextItemWidth(210.f * profile.uiScale);
        ImGui::SliderFloat("Bubble Lightness", &profile.bsBubbleLightness, -1.f, 1.f, "%.2f");
        ImGui::EndDisabled();

        ImGui::Separator();

        if (uiCheckbox("VSync", &profile.vsync))
            window.setVerticalSyncEnabled(profile.vsync);

        ImGui::EndPopup();
    }
}

////////////////////////////////////////////////////////////
void Main::uiDrawQuickbarVolumeControls(const za::Vec2f quickBarPos)
{
    constexpr const char* popupLabel = "VolumeSelectorPopup";

    const bool pressed = uiDrawQuickbarIconButton(ICON_FA_VOLUME_HIGH "##804", ImGui::IsPopupOpen(popupLabel));

    std::sprintf(uiState.uiTooltipBuffer, "Volume settings");
    uiMakeTooltip(/* small */ true);

    ImGui::SameLine();

    if (pressed)
    {
        ImGui::OpenPopup(popupLabel);
        playSound(sounds.uitab);
    }

    ImGui::SetNextWindowPos(ImVec2(quickBarPos) - ImVec2{0.f, 64.f}, 0, {0.f, 1.f});
    if (ImGui::BeginPopup(popupLabel, ImGuiWindowFlags_NoBackground))
    {
        uiDrawCloudWindowBackground();

        ImGui::SetNextItemWidth(210.f * profile.uiScale);
        ImGui::SliderFloat("Master##popupmastervolume", &profile.masterVolume, 0.f, 100.f, "%.f%%");

        ImGui::SetNextItemWidth(210.f * profile.uiScale);
        if (ImGui::SliderFloat("SFX##popupsfxvolume", &profile.sfxVolume, 0.f, 100.f, "%.f%%"))
            sounds.setupSounds(/* volumeOnly */ true, profile.sfxVolume / 100.f);

        ImGui::SetNextItemWidth(210.f * profile.uiScale);
        ImGui::SliderFloat("Music##popupmusicvolume", &profile.musicVolume, 0.f, 100.f, "%.f%%");

        ImGui::EndPopup();
    }
}

////////////////////////////////////////////////////////////
void Main::uiDrawMinimapZoomButtons()
{
    if (!pt->mapPurchased)
    {
        uiState.minimapZoomButtonsRect = {};
        return;
    }

    const float     hudScale     = profile.hudScale;
    const za::Vec2f mmScreenPos  = uiState.minimapRect.position * hudScale;
    const za::Vec2f mmScreenSize = uiState.minimapRect.size * hudScale;

    constexpr float tolerance = 48.f;

    const ImVec2 mp        = ImGui::GetMousePos();
    const bool inHoverArea = mp.x >= mmScreenPos.x - tolerance && mp.x <= mmScreenPos.x + mmScreenSize.x + tolerance &&
                             mp.y >= mmScreenPos.y - tolerance && mp.y <= mmScreenPos.y + mmScreenSize.y + tolerance;

    if (!inHoverArea)
    {
        uiState.minimapZoomButtonsRect = {};
        return;
    }

    ImGui::SetNextWindowPos({2.f, 14.f}, 0, {0.f, 0.f});
    ImGui::SetNextWindowSizeConstraints(ImVec2(0.f, 0.f), ImVec2(ZA_FLOAT_MAX, ZA_FLOAT_MAX));

    ImGui::Begin("##minimapzoom",
                 nullptr,
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);

    constexpr float zoomBtnScale = 0.85f;

    if (uiDrawQuickbarIconButton(ICON_FA_MAGNIFYING_GLASS_PLUS "##700", false, zoomBtnScale))
    {
        profile.minimapScale = za::clamp(profile.minimapScale - 2.5f, 5.f, 40.f);
        playSound(sounds.uitab);
    }

    std::sprintf(uiState.uiTooltipBuffer, "Zoom minimap in");
    uiMakeTooltip(/* small */ true);

    ImGui::SameLine();

    if (uiDrawQuickbarIconButton(ICON_FA_MAGNIFYING_GLASS_MINUS "##701", false, zoomBtnScale))
    {
        profile.minimapScale = za::clamp(profile.minimapScale + 2.5f, 5.f, 40.f);
        playSound(sounds.uitab);
    }

    std::sprintf(uiState.uiTooltipBuffer, "Zoom minimap out");
    uiMakeTooltip(/* small */ true);

    const ImVec2 winPos            = ImGui::GetWindowPos();
    const ImVec2 winSize           = ImGui::GetWindowSize();
    uiState.minimapZoomButtonsRect = {{winPos.x, winPos.y}, {winSize.x, winSize.y}};

    ImGui::End();
}


////////////////////////////////////////////////////////////
void Main::uiDrawQuickbar()
{
    const za::Vec2f quickBarPos{10.f, getResolution().y - 15.f};

    ImGui::SetNextWindowPos({quickBarPos.x, quickBarPos.y}, 0, {0.f, 1.f});
    ImGui::SetNextWindowSizeConstraints(ImVec2(0.f, 0.f), ImVec2(ZA_FLOAT_MAX, ZA_FLOAT_MAX));

    ImGui::Begin("##quickmenu",
                 nullptr,
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize |
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground);

    {
        const ImVec2 qbPos  = ImGui::GetWindowPos();
        const ImVec2 qbSize = ImGui::GetWindowSize();

        const za::Vec2f mins{qbPos.x - 20.f, qbPos.y - 6.f};
        const za::Vec2f maxs{qbPos.x + qbSize.x - 10.f, qbPos.y + qbSize.y + 20.f};

        drawCloudFrame({
            .time              = shaderTime,
            .mins              = mins,
            .maxs              = maxs,
            .xSteps            = 8,
            .ySteps            = 8,
            .scaleMult         = 1.6f,
            .outwardOffsetMult = 1.f,
            .batch             = &cpuCloudUiDrawableBatch,
        });
    }

    if (Cat* copyCat = getCopyCat(); copyCat != nullptr)
        uiDrawQuickbarCopyCat(quickBarPos, *copyCat);

    if (getBackgroundSelectorData().entries.size() > 1u)
        uiDrawQuickbarBackgroundSelector(quickBarPos);

    if (getBGMSelectorData().entries.size() > 1u)
        uiDrawQuickbarBGMSelector(quickBarPos);

    uiDrawQuickbarVolumeControls(quickBarPos);

    uiDrawQuickbarQuickSettings(quickBarPos);

    ImGui::End();
}
