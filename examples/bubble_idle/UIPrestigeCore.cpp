#include "BubbleIdleMain.hpp"
#include "CatType.hpp"
#include "MainAtlasRects.hpp"
#include "Playthrough.hpp"
#include "UIState.hpp"

#include "Zancle/ImGui/IncludeImGui.hpp"

#include "Zancle/Base/Limits.hpp"

#include <cstdio>

void Main::uiPrestigeDrawCoreUpgrades()
{
    uiBeginColumns();

    uiState.uiButtonHueMod = 190.f;

    if (checkUiUnlock(47u, pt->psvBubbleValue.nPurchases >= 3u))
    {
        uiImgsep2(atlasRects.txrPrestigeSeparator4, "faster beginning");

        uiSetUnlockLabelY(47u);
        (void)uiMakePrestigeOneTimeButton("Starter pack",
                                          1u,
                                          pt->perm.starterPackPurchased,
                                          "Begin your next prestige with $1000.");
    }

    if (checkUiUnlock(48u, pt->psvBubbleValue.nPurchases >= 1u))
    {
        uiImgsep2(atlasRects.txrPrestigeSeparator2, "clicking tools");

        uiSetUnlockLabelY(48u);
        if (uiMakePrestigeOneTimeButton("Multipop click",
                                        0u,
                                        pt->perm.multiPopPurchased,
                                        "Manually popping a bubble now also pops nearby bubbles automatically!\n\n"
                                        "(Note: combo multiplier still only increases once per successful click.)\n\n"
                                        "(Note: this effect can be toggled at will.)"))
            doTip("Popping a bubble now also pops\nnearby bubbles automatically!",
                  /* maxPrestigeLevel */ ZA_UNSIGNED_INT_MAX);

        if (checkUiUnlock(49u, pt->perm.multiPopPurchased))
        {
            uiSetUnlockLabelY(49u);

            if (pt->psvBubbleValue.nPurchases >= 2u)
            {
                const float currentRange = pt->psvPPMultiPopRange.currentValue();
                const float nextRange    = pt->psvPPMultiPopRange.nextValue();

                uiMakePrestigePsvButtonValue("  range",
                                             pt->psvPPMultiPopRange,
                                             "%.2fpx",
                                             static_cast<double>(currentRange),
                                             "Increase the range of the multipop effect from %.2fpx to %.2fpx.",
                                             static_cast<double>(currentRange),
                                             static_cast<double>(nextRange),
                                             "Increase the range of the multipop effect (MAX).");
            }

            uiSetFontScale(uiSubBulletFontScale);
            uiCheckbox("enable ##multipop", &pt->multiPopEnabled);
            if (getMouseCat() != nullptr)
            {
                ImGui::SameLine();
                uiCheckbox("mousecat##multipopmousecat", &pt->multiPopMouseCatEnabled);
            }
            uiSetFontScale(uiNormalFontScale);
            ImGui::NextColumn();
            ImGui::NextColumn();
        }
    }

    uiImgsep2(atlasRects.txrPrestigeSeparator3, "wind effects");

    if (uiMakePrestigeOneTimeButton("Giant fan",
                                    6u,
                                    pt->perm.windPurchased,
                                    "A giant fan (off-screen) will produce an intense wind, making bubbles move and "
                                    "flow much faster.\n\n(Note: this effect can be toggled at will.)"))
        doTip("Hold onto something!", /* maxPrestigeLevel */ ZA_UNSIGNED_INT_MAX);

    if (checkUiUnlock(50u, pt->perm.windPurchased))
    {
        uiSetFontScale(uiSubBulletFontScale);
        ImGui::Columns(1);

        uiSetUnlockLabelY(50u);
        uiRadio("off##windOff", &pt->windStrength, 0);
        ImGui::SameLine();
        uiRadio("slow##windSlow", &pt->windStrength, 1);
        ImGui::SameLine();
        uiRadio("medium##windMed", &pt->windStrength, 2);
        ImGui::SameLine();
        uiRadio("fast##windFast", &pt->windStrength, 3);

        uiBeginColumns();
        uiSetFontScale(uiNormalFontScale);
        ImGui::NextColumn();
        ImGui::NextColumn();
    }

    // TODO P0: use a dedicated "power nap" separator image; reusing
    // atlasRects.txrPrestigeSeparator7 (currently unused) as a placeholder.
    if (checkUiUnlock(68u, pt->psvBubbleValue.nPurchases >= 1u))
    {
        uiImgsep2(atlasRects.txrPrestigeSeparator7, "power nap");

        uiBeginColumns();

        uiSetUnlockLabelY(68u);
        (void)uiMakePrestigeOneTimeButton("Power nap",
                                          4u,
                                          pt->perm.powerNapPurchased,
                                          "Forcibly waking a cat from a nap -- by shaking it, or via a wardencat bonk "
                                          "-- grants that cat a temporary cooldown-reduction boost. The boost stacks "
                                          "multiplicatively with other buffs (engicat, inspired, ...).\n\n(Note: cats "
                                          "that wake naturally from a full nap get no boost.)");

        if (checkUiUnlock(69u, pt->perm.powerNapPurchased))
        {
            uiSetUnlockLabelY(69u);

            const float currentDuration = pt->psvPPPowerNapDuration.currentValue();
            const float nextDuration    = pt->psvPPPowerNapDuration.nextValue();

            uiMakePrestigePsvButtonValue("Boost duration",
                                         pt->psvPPPowerNapDuration,
                                         "%.2fs",
                                         static_cast<double>(currentDuration / 1000.f),
                                         "Increase the Power Nap boost duration from %.2fs to %.2fs.",
                                         static_cast<double>(currentDuration / 1000.f),
                                         static_cast<double>(nextDuration / 1000.f),
                                         "Increase the Power Nap boost duration (MAX).");

            const float currentStrength    = pt->psvPPPowerNapStrength.currentValue();
            const float nextStrength       = pt->psvPPPowerNapStrength.nextValue();
            const float currentCooldownMul = 1.f + currentStrength;
            const float nextCooldownMul    = 1.f + nextStrength;

            uiMakePrestigePsvButtonValue("Boost strength",
                                         pt->psvPPPowerNapStrength,
                                         "%.2fx",
                                         static_cast<double>(currentCooldownMul),
                                         "Increase the Power Nap cooldown speed-up from %.2fx to %.2fx.",
                                         static_cast<double>(currentCooldownMul),
                                         static_cast<double>(nextCooldownMul),
                                         "Increase the Power Nap cooldown speed-up (MAX).");
        }
    }

    uiImgsep2(atlasRects.txrPrestigeSeparator5, "cats");

    uiBeginColumns();

    if (uiMakePrestigeOneTimeButton("Smart cats",
                                    1u,
                                    pt->perm.smartCatsPurchased,
                                    "Cats have graduated!\n\nThey still cannot resist their popping insticts, but "
                                    "they will go for star bubbles and bombs first, ensuring they are not wasted!"))
        doTip("Cats will now prioritize popping\nspecial bubbles over basic ones!",
              /* maxPrestigeLevel */ ZA_UNSIGNED_INT_MAX);

    if (checkUiUnlock(51u, pt->perm.smartCatsPurchased))
    {
        uiSetUnlockLabelY(51u);
        if (uiMakePrestigeOneTimeButton("genius cats",
                                        8u,
                                        pt->perm.geniusCatsPurchased,
                                        "Embrace the glorious evolution!\n\nCats have ascended beyond their primal "
                                        "insticts and will now prioritize bombs, then star bubbles, then normal "
                                        "bubbles!\n\nThey will also ignore any bubble type of your choosing.\n\n"
                                        "Through the sheer power of their intellect, they also get a x2 multiplier "
                                        "on all bubble values.\n\n(Note: this effect can be toggled at will.)"))
            doTip("Genius cats prioritize bombs and\ncan be instructed to ignore certain bubbles!",
                  /* maxPrestigeLevel */ ZA_UNSIGNED_INT_MAX);
    }

    if (checkUiUnlock(52u, pt->perm.geniusCatsPurchased))
    {
        ImGui::Columns(1);
        uiSetFontScale(uiSubBulletFontScale);

        uiSetUnlockLabelY(52u);
        ImGui::AlignTextToFramePadding();
        ImGui::Text("  ignore: ");
        ImGui::SameLine();

        auto& [ignoreNormal, ignoreStar, ignoreBomb] = pt->geniusCatIgnoreBubbles;

        uiCheckbox("normal##genius", &ignoreNormal);
        ImGui::SameLine();
        uiCheckbox("star##genius", &ignoreStar);
        ImGui::SameLine();
        uiCheckbox("bombs##genius", &ignoreBomb);

        uiSetFontScale(uiNormalFontScale);
        uiBeginColumns();
    }

    if (checkUiUnlock(53u, pt->psvBubbleValue.nPurchases >= 3))
    {
        uiImgsep2(atlasRects.txrPrestigeSeparator6, "unicats");

        uiBeginColumns();

        uiSetUnlockLabelY(53u);
        if (uiMakePrestigeOneTimeButton("transcendence",
                                        96u,
                                        pt->perm.unicatTranscendencePurchased,
                                        "Unicats transcend their physical form, becoming a higher entity that "
                                        "transforms bubbles into nova bubbles, worth x50."))
            doTip("Are you ready for that sweet x50?", /* maxPrestigeLevel */ ZA_UNSIGNED_INT_MAX);

        if (checkUiUnlock(54u, pt->perm.unicatTranscendencePurchased))
        {
            uiSetUnlockLabelY(54u);
            if (uiMakePrestigeOneTimeButton("nova expanse",
                                            128u,
                                            pt->perm.unicatTranscendenceAOEPurchased,
                                            "Unicats can now transform all bubbles in range at once. Also unlocks "
                                            "Unicat range upgrades."))
                doTip("It's about to get crazy...", /* maxPrestigeLevel */ ZA_UNSIGNED_INT_MAX);

            if (pt->perm.unicatTranscendencePurchased)
            {
                ImGui::Columns(1);
                uiCheckbox("enable transcendent unicats", &pt->perm.unicatTranscendenceEnabled);
                uiBeginColumns();
            }
        }
    }

    if (checkUiUnlock(55u, pt->psvBubbleValue.nPurchases >= 3))
    {
        uiImgsep2(atlasRects.txrPrestigeSeparator7, "devilcats");

        uiBeginColumns();

        uiSetUnlockLabelY(55u);
        if (uiMakePrestigeOneTimeButton("hellsinged",
                                        192u,
                                        pt->perm.devilcatHellsingedPurchased,
                                        "Devilcats become touched by the flames of hell, opening stationary portals "
                                        "that teleport bubbles into the abyss, with a x50 multiplier. Also unlocks "
                                        "Devilcat range upgrades."))
            doTip("I'm starting to get a bit scared...", /* maxPrestigeLevel */ ZA_UNSIGNED_INT_MAX);

        if (pt->perm.devilcatHellsingedPurchased)
        {
            ImGui::Columns(1);
            uiCheckbox("enable hellsinged devilcats", &pt->perm.devilcatHellsingedEnabled);
            uiBeginColumns();
        }
    }

    if (checkUiUnlock(56u, pt->getCatCountByType(CatType::Astro) >= 1u || pt->psvBubbleValue.nPurchases >= 3))
    {
        uiImgsep2(atlasRects.txrPrestigeSeparator8, "astrocats");

        uiBeginColumns();

        uiCheckbox("disable flight", &pt->disableAstrocatFlight);

        uiSetUnlockLabelY(56u);
        if (uiMakePrestigeOneTimeButton("Space propaganda",
                                        16u,
                                        pt->perm.astroCatInspirePurchased,
                                        "Astrocats are now equipped with fancy patriotic flags, inspiring cats "
                                        "watching them fly by to work faster!"))
            doTip("Astrocats will inspire other cats\nto work faster when flying by!",
                  /* maxPrestigeLevel */ ZA_UNSIGNED_INT_MAX);

        if (checkUiUnlock(57u, pt->perm.astroCatInspirePurchased))
        {
            const float currentDuration = pt->getComputedInspirationDuration();
            const float nextDuration    = pt->getComputedNextInspirationDuration();

            uiSetUnlockLabelY(57u);
            uiMakePrestigePsvButtonValue("inspire duration",
                                         pt->psvPPInspireDurationMult,
                                         "%.2fs",
                                         static_cast<double>(currentDuration / 1000.f),
                                         "Increase the duration of the inspiration effect from %.2fs to %.2fs.",
                                         static_cast<double>(currentDuration / 1000.f),
                                         static_cast<double>(nextDuration / 1000.f),
                                         "Increase the duration of the inspiration effect (MAX).");
        }
    }
}
