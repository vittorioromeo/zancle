#include "Aliases.hpp"
#include "BGMStorage.hpp"
#include "Bubble.hpp"
#include "BubbleIdleMain.hpp"
#include "BubbleIdleMainInline.hpp"
#include "BubbleType.hpp"
#include "Cat.hpp"
#include "CatConstants.hpp"
#include "CatNames.hpp"
#include "CatType.hpp"
#include "Collision.hpp"
#include "ComboState.hpp"
#include "Constants.hpp"
#include "Doll.hpp"
#include "HexSession.hpp"
#include "InputHelper.hpp"
#include "MainBombStorage.hpp"
#include "NotificationState.hpp"
#include "Particle.hpp"
#include "ParticleData.hpp"
#include "ParticleType.hpp"
#include "Playthrough.hpp"
#include "Profile.hpp"
#include "RNGSeedType.hpp"
#include "Serialization.hpp"
#include "Shrine.hpp"
#include "ShrineType.hpp"
#include "Stats.hpp"
#include "TextParticle.hpp"
#include "UIState.hpp"
#include "Version.hpp"

#include "ExampleUtils/ControlFlow.hpp"
#include "ExampleUtils/HueColor.hpp"
#include "ExampleUtils/LoadedSound.hpp"
#include "ExampleUtils/MathUtils.hpp"
#include "ExampleUtils/Progress.hpp"
#include "ExampleUtils/RNGFast.hpp"
#include "ExampleUtils/SoundManager.hpp"

#include "Zancle/Audio/Music.hpp"
#include "Zancle/Audio/MusicReader.hpp"

#include "Zancle/Graphics/DrawableBatch.hpp"
#include "Zancle/Graphics/Image.hpp"
#include "Zancle/Graphics/RenderStates.hpp"
#include "Zancle/Graphics/RenderTarget.hpp"
#include "Zancle/Graphics/RenderTexture.hpp"
#include "Zancle/Graphics/RenderWindow.hpp"
#include "Zancle/Graphics/Text.hpp" // IWYU pragma: keep
#include "Zancle/Graphics/TextureAtlas.hpp"
#include "Zancle/Graphics/View.hpp"

#include "Zancle/ImGui/IncludeImGui.hpp"

#include "Zancle/Window/Keyboard.hpp"
#include "Zancle/Window/Mouse.hpp"
#include "Zancle/Window/VideoMode.hpp"
#include "Zancle/Window/VideoModeUtils.hpp"

#include "Zancle/Fmt/Fmt.hpp"
#include "Zancle/Fmt/FmtNumeric.hpp"

#include "Zancle/IO/IO.hpp"
#include "Zancle/IO/Path.hpp"

#include "Zancle/Concurrency/ThreadPool.hpp"

#include "Zancle/Algorithm/Erase.hpp"

#include "Zancle/Chrono/Clock.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Geometry/Angle.hpp"
#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Math/Ceil.hpp"
#include "Zancle/Math/Clamp.hpp"
#include "Zancle/Math/Constants.hpp"
#include "Zancle/Math/FloatMax.hpp"
#include "Zancle/Math/MinMax.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/SizeT.hpp"

#include <cstdarg>
#include <cstdio>
#include <ctime>


////////////////////////////////////////////////////////////
bool handleBubbleCollision(const float deltaTimeMs, Bubble& iBubble, Bubble& jBubble)
{
    const auto result = handleCollision(deltaTimeMs,
                                        iBubble.position,
                                        jBubble.position,
                                        iBubble.velocity,
                                        jBubble.velocity,
                                        iBubble.radius,
                                        jBubble.radius,
                                        iBubble.type == BubbleType::Bomb ? 5.f : 1.f,
                                        jBubble.type == BubbleType::Bomb ? 5.f : 1.f);

    if (!result.hasValue())
        return false;

    iBubble.position += result->iDisplacement;
    jBubble.position += result->jDisplacement;
    iBubble.velocity += result->iVelocityChange;
    jBubble.velocity += result->jVelocityChange;

    return true;
}


////////////////////////////////////////////////////////////
bool handleCatCollision(const float deltaTimeMs, Cat& iCat, Cat& jCat)
{
    const auto
        result = handleCollision(deltaTimeMs, iCat.position, jCat.position, {}, {}, iCat.getRadius(), jCat.getRadius(), 1.f, 1.f);

    if (!result.hasValue())
        return false;

    if (!iCat.isHexedOrCopyHexed())
        iCat.position += result->iDisplacement;

    if (!jCat.isHexedOrCopyHexed())
        jCat.position += result->jDisplacement;

    return true;
}


////////////////////////////////////////////////////////////
bool handleCatShrineCollision(const float deltaTimeMs, Cat& cat, Shrine& shrine)
{
    const auto result = handleCollision(deltaTimeMs, cat.position, shrine.position, {}, {}, cat.getRadius(), 64.f, 1.f, 1.f);

    if (!result.hasValue())
        return false;

    cat.position += result->iDisplacement;
    return true;
}


////////////////////////////////////////////////////////////
void Main::pushNotification(const char* const title, const char* const format, ...)
{
    if (!profile.enableNotifications)
        return;

    char fmtBuffer[1024]{};

    std::va_list args;
    va_start(args, format);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#pragma GCC diagnostic ignored "-Wformat-security"
    std::vsnprintf(fmtBuffer, sizeof(fmtBuffer), format, args);
#pragma GCC diagnostic pop

    va_end(args);

    notificationState.queue.emplaceBack(title, za::String{fmtBuffer});
}


////////////////////////////////////////////////////////////
void Main::refreshWindowAutoBatchModeFromProfile() // TODO P1: check if this solves flickering
{
    window.setAutoBatchMode(profile.autobatchMode == 0   ? za::RenderTarget::AutoBatchMode::Disabled
                            : profile.autobatchMode == 1 ? za::RenderTarget::AutoBatchMode::CPUStorage
                                                         : za::RenderTarget::AutoBatchMode::GPUStorage);
}


////////////////////////////////////////////////////////////
void Main::drawBatch(const za::CPUDrawableBatch& batch, const za::RenderStates& states)
{
    rtGame.draw(batch, states);

    if (flushAfterEveryBatch)
        rtGame.invokeGlFlush();

    if (finishAfterEveryBatch)
        rtGame.invokeGlFinish();
}


////////////////////////////////////////////////////////////
void Main::saveMainPlaythroughToFile()
{
    if constexpr (isDemoVersion)
    {
        if (ptMain.fullVersion)
        {
            za::printLn("Cannot save non-demo playthrough in demo version!");
            return;
        }
    }

    ptMain.fullVersion = !isDemoVersion;
    savePlaythroughToFile(ptMain, "userdata/playthrough.json");
}


////////////////////////////////////////////////////////////
void Main::log(const char* format, ...) // NOLINT(modernize-avoid-variadic-functions)
{
    if (!logFile)
        return;

    const std::time_t currentTime = std::time(nullptr);
    const auto        localTime   = *std::localtime(&currentTime);

    char timeBuffer[100]; // Buffer for the timestamp string
    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &localTime);

    char messageBuffer[1024];

    va_list args{};
    va_start(args, format);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
    vsnprintf(messageBuffer, sizeof(messageBuffer), format, args);
#pragma GCC diagnostic pop
    va_end(args);

    za::printLn(*logFile, "{} - {}", static_cast<const char*>(timeBuffer), static_cast<const char*>(messageBuffer));
    (void)logFile->flush();
}


////////////////////////////////////////////////////////////
void Main::addMoney(const MoneyType reward)
{
    pt->money += reward;
    moneyGainedLastSecond += reward;
}


////////////////////////////////////////////////////////////
za::Vector<za::Vector<za::StringView>> Main::makeShuffledCatNames(RNGFast& rng)
{
    za::Vector<za::Vector<za::StringView>> result(nCatTypes);

    for (SizeT i = 0u; i < nCatTypes; ++i)
        result[i] = getShuffledCatNames(static_cast<CatType>(i), rng);

    return result;
}


////////////////////////////////////////////////////////////
unsigned int Main::getTPWorkerCount()
{
    const auto numThreads = static_cast<unsigned int>(za::ThreadPool::getHardwareWorkerCount());
    return (numThreads == 0u) ? 3u : numThreads - 1u;
}


////////////////////////////////////////////////////////////
SizeT Main::getNextCatNameIdx(const CatType catType)
{
    return pt->nextCatNamePerType[asIdx(catType)]++ % shuffledCatNamesPerType[asIdx(catType)].size();
}


////////////////////////////////////////////////////////////
Particle& Main::implEmplaceParticle(const za::Vec2f    position,
                                    const ParticleType particleType,
                                    const float        scaleMult,
                                    const float        speedMult,
                                    const float        opacity)
{
    return particles.emplaceBack(
        ParticleData{
            .position      = position,
            .velocity      = rngFast.getVec2f({-0.75f, -0.75f}, {0.75f, 0.75f}) * speedMult,
            .scale         = rngFast.getF(0.08f, 0.27f) * scaleMult,
            .scaleDecay    = 0.f,
            .accelerationY = 0.002f,
            .opacity       = opacity,
            .opacityDecay  = rngFast.getF(0.00025f, 0.0015f),
            .rotation      = rngFast.getF(0.f, za::tau),
            .torque        = rngFast.getF(-0.002f, 0.002f),
        },
        za::U8{0u},
        particleType);
}


////////////////////////////////////////////////////////////
bool Main::spawnSpentCoinParticle(const ParticleData& particleData)
{
    if (!profile.showParticles || !hudCullingBoundaries.isInside(particleData.position))
        return false;

    spentCoinParticles.emplaceBack(particleData, za::U8{0u}, ParticleType::Coin);
    return true;
}


////////////////////////////////////////////////////////////
void Main::spawnHUDTopParticle(const ParticleData& particleData, const float hue, const ParticleType particleType)
{
    if (!profile.showParticles || !hudCullingBoundaries.isInside(particleData.position))
        return;

    hudTopParticles.emplaceBack(particleData, hueToByte(hue), particleType);
}


////////////////////////////////////////////////////////////
void Main::spawnHUDBottomParticle(const ParticleData& particleData, const float hue, const ParticleType particleType)
{
    if (!profile.showParticles || !hudCullingBoundaries.isInside(particleData.position))
        return;

    hudBottomParticles.emplaceBack(particleData, hueToByte(hue), particleType);
}


////////////////////////////////////////////////////////////
bool Main::spawnEarnedCoinParticle(const za::Vec2f startPosition)
{
    if (!profile.showParticles || !profile.showCoinParticles || !hudCullingBoundaries.isInside(startPosition))
        return false;

    earnedCoinParticles.emplaceBack(startPosition);
    return true;
}


////////////////////////////////////////////////////////////
void Main::spawnParticle(const ParticleData& particleData, const float hue, const ParticleType particleType)
{
    if (!profile.showParticles || !particleCullingBoundaries.isInside(particleData.position))
        return;

    // TODO P2: consider optimizing this pattern by just returning the emplaced particle and having the caller set the data
    particles.emplaceBack(particleData, hueToByte(hue), particleType);
}


////////////////////////////////////////////////////////////
void Main::statBubblePopped(const BubbleType bubbleType, const bool byHand, const MoneyType reward)
{
    withAllStats([&](Stats& stats)
    {
        stats.nBubblesPoppedByType[asIdx(bubbleType)] += 1u;
        stats.revenueByType[asIdx(bubbleType)] += reward;
    });

    if (byHand)
    {
        withAllStats([&](Stats& stats)
        {
            stats.nBubblesHandPoppedByType[asIdx(bubbleType)] += 1u;
            stats.revenueHandByType[asIdx(bubbleType)] += reward;
        });
    }
}


////////////////////////////////////////////////////////////
void Main::statExplosionRevenue(const MoneyType reward)
{
    withAllStats([&](Stats& stats) { stats.explosionRevenue += reward; });
}


////////////////////////////////////////////////////////////
void Main::statFlightRevenue(const MoneyType reward)
{
    withAllStats([&](Stats& stats) { stats.flightRevenue += reward; });
}


////////////////////////////////////////////////////////////
void Main::statHellPortalRevenue(const MoneyType reward)
{
    withAllStats([&](Stats& stats) { stats.hellPortalRevenue += reward; });
}


////////////////////////////////////////////////////////////
void Main::statSecondsPlayed()
{
    withAllStats([&](Stats& stats) { stats.secondsPlayed += 1u; });
}


////////////////////////////////////////////////////////////
void Main::statHighestStarBubblePopCombo(const za::U64 comboValue)
{
    withAllStats([&](Stats& stats)
    { stats.highestStarBubblePopCombo = za::max(stats.highestStarBubblePopCombo, comboValue); });
}


////////////////////////////////////////////////////////////
void Main::statHighestNovaBubblePopCombo(const za::U64 comboValue)
{
    withAllStats([&](Stats& stats)
    { stats.highestNovaBubblePopCombo = za::max(stats.highestNovaBubblePopCombo, comboValue); });
}


////////////////////////////////////////////////////////////
void Main::statAbsorbedStarBubble()
{
    withAllStats([&](Stats& stats) { stats.nAbsorbedStarBubbles += 1u; });
}


////////////////////////////////////////////////////////////
void Main::statSpellCast(const SizeT spellIndex)
{
    withAllStats([&](Stats& stats) { stats.nSpellCasts[spellIndex] += 1u; });
}


////////////////////////////////////////////////////////////
void Main::statMaintenance(const SizeT nCatsHit)
{
    withAllStats([&](Stats& stats) { stats.nMaintenances += nCatsHit; });
}


////////////////////////////////////////////////////////////
void Main::statDisguise()
{
    withAllStats([&](Stats& stats) { stats.nDisguises += 1u; });
}


////////////////////////////////////////////////////////////
void Main::statDollCollected()
{
    withAllStats([&](Stats& stats) { stats.nWitchcatDollsCollected += 1u; });
}


////////////////////////////////////////////////////////////
void Main::statRitual(const CatType catType)
{
    withAllStats([&](Stats& stats) { stats.nWitchcatRitualsPerCatType[asIdx(catType)] += 1u; });
}


////////////////////////////////////////////////////////////
void Main::statHighestSimultaneousMaintenances(const za::U64 value)
{
    withAllStats([&](Stats& stats)
    { stats.highestSimultaneousMaintenances = za::max(stats.highestSimultaneousMaintenances, value); });
}


////////////////////////////////////////////////////////////
void Main::statHighestDPS(const za::U64 value)
{
    withAllStats([&](Stats& stats) { stats.highestDPS = za::max(stats.highestDPS, value); });
}


////////////////////////////////////////////////////////////
bool Main::keyDown(const za::Keyboard::Key key) const
{
    return inputHelper.isKeyDown(key);
}


////////////////////////////////////////////////////////////
bool Main::mBtnDown(const za::Mouse::Button button, const bool penetrateUI) const
{
    if (ImGui::GetIO().WantCaptureMouse && !penetrateUI)
        return false;

    return inputHelper.isMouseButtonDown(button);
}


////////////////////////////////////////////////////////////
za::Rect2f Main::addImgResourceToAtlas(const za::Path& path)
{
    return textureAtlas.add(za::Image::loadFromFile("resources" / path).value()).value();
}


////////////////////////////////////////////////////////////
za::Rect2f Main::addImgResourceToUIAtlas(const za::Path& path)
{
    return uiTextureAtlas.add(za::Image::loadFromFile("resources" / path).value()).value();
}


////////////////////////////////////////////////////////////
void Main::playSound(const LoadedSound& ls, const za::SizeT maxOverlap)
{
#ifndef BUBBLEBYTE_NO_AUDIO
    soundManager.playPooled(playbackDevice, ls, maxOverlap);
#else
    (void)ls;
    (void)maxOverlap;
#endif
}


////////////////////////////////////////////////////////////
Bubble* Main::pickRandomBubbleInRadius(const za::Vec2f center, const float radius)
{
    return pickRandomBubbleInRadiusMatching(center, radius, [] [[gnu::always_inline, gnu::flatten]] (const Bubble&) {
        return true;
    });
}


////////////////////////////////////////////////////////////
za::Vec2f Main::getResolution() const
{
    return window.getSize().toVec2f();
}


////////////////////////////////////////////////////////////
za::Vec2f Main::getViewCenter() const
{
    const za::Vec2f currentViewSize = getCurrentGameViewSize();
    return {clampGameViewCenterX(currentViewSize.x / 2.f + playerInputState.actualScroll * 2.f, currentViewSize.x),
            currentViewSize.y / 2.f};
}


////////////////////////////////////////////////////////////
za::Vec2f Main::getViewCenterWithoutScroll() const
{
    const za::Vec2f currentViewSize = getCurrentGameViewSize();
    return {clampGameViewCenterX(currentViewSize.x / 2.f, currentViewSize.x), currentViewSize.y / 2.f};
}


////////////////////////////////////////////////////////////
Main::CullingBoundaries Main::getViewCullingBoundaries(const float offset) const
{
    const za::Vec2f viewSize{getCurrentGameViewSize()};
    const za::Vec2f viewCenter{getViewCenter()};

    return {viewCenter.x - viewSize.x / 2.f + offset,
            viewCenter.x + viewSize.x / 2.f - offset,
            viewCenter.y - viewSize.y / 2.f + offset,
            viewCenter.y + viewSize.y / 2.f - offset};
}


////////////////////////////////////////////////////////////
za::Vec2f Main::getCatRangeCenter(const Cat& cat)
{
    return cat.position + CatConstants::rangeOffsets[asIdx(cat.type)];
}


////////////////////////////////////////////////////////////
Cat& Main::spawnCat(const za::Vec2f pos, const CatType catType, const float hue)
{
    const auto meowPitch = [&] -> float
    {
        switch (catType)
        {
            case CatType::Uni:
                return rng.getF(1.2f, 1.3f);
            case CatType::Devil:
                return rng.getF(0.7f, 0.8f);
            default:
                return rng.getF(0.9f, 1.1f);
        }
    }();

    sounds.purrmeow.settings.position = {pos.x, pos.y};
    sounds.purrmeow.settings.pitch    = meowPitch;
    playSound(sounds.purrmeow);

    spawnParticles(32, pos, ParticleType::Star, 0.5f, 0.75f);

    playerInputState.catToPlace = nullptr;

    Cat& newCat = pt->cats.emplaceBack(Cat{
        .position    = pos,
        .cooldown    = {.time = getComputedCooldownByCatTypeOrCopyCat(catType)},
        .pawPosition = pos,
        .hue         = hue,
        .nameIdx     = getNextCatNameIdx(catType),
        .type        = catType,
    });

    return newCat;
}


////////////////////////////////////////////////////////////
Cat& Main::spawnCatCentered(const CatType catType, const float hue, const bool placeInHand)
{
    const auto pos = gameView.screenToWorld(getResolution() / 2.f, window.getSize().toVec2f());

    Cat& newCat = spawnCat(pos, catType, hue);

    if (placeInHand)
    {
        playerInputState.catToPlace = &newCat;
        newCat.dragTime             = 1000.f;

        playerInputState.draggedCats.clear();
        playerInputState.draggedCats.pushBack(&newCat);
        playerInputState.draggedCatsStartedWithTouch        = false;
        playerInputState.draggedCatsStartedFromAOESelection = false;

        newCat.position    = playerInputState.lastMousePos;
        newCat.pawPosition = playerInputState.lastMousePos;
    }

    return newCat;
}


////////////////////////////////////////////////////////////
Cat& Main::spawnSpecialCat(const za::Vec2f pos, const CatType catType)
{
    ++pt->psvPerCatType[static_cast<SizeT>(catType)].nPurchases;
    return spawnCat(pos, catType, /* hue */ 0.f);
}


////////////////////////////////////////////////////////////
void Main::resetTipState()
{
    tipTCByte.reset();
    tipTCBackground.reset();
    tipTCBytePreEnd.reset();
    tipTCByteEnd.reset();
    tipTCBackgroundEnd.reset();
    tipCountdownChar.time = 0.f;
    tipString             = "";
    tipCharIdx            = 0u;
}


////////////////////////////////////////////////////////////
void Main::doTip(const za::String& str, const SizeT maxPrestigeLevel)
{
    if (!profile.tipsEnabled || pt->psvBubbleValue.nPurchases > maxPrestigeLevel || inSpeedrunPlaythrough())
        return;

    playSound(sounds.byteMeow, /* maxOverlap */ 1u);

    resetTipState();

    tipTCByte.emplace(TimedCountdown{.duration = 500.f});
    tipTCByte->restart();

    tipString = str + "\t\t\t\t\t";
}


////////////////////////////////////////////////////////////
float Main::getAspectRatioScalingFactor(const za::Vec2f originalSize, const za::Vec2f windowSize) const
{
    const float scaleX = windowSize.x / originalSize.x;
    const float scaleY = windowSize.y / originalSize.y;

    // Use the smaller scale factor to maintain aspect ratio
    return za::min(scaleX, scaleY);
}


////////////////////////////////////////////////////////////
float Main::getCappedGameViewAspectRatio(const za::Vec2f originalSize, const za::Vec2f windowSize) const
{
    const float originalAspect = originalSize.x / originalSize.y;
    const float windowAspect   = windowSize.x / windowSize.y;
    // const float configuredMaxAspect = za::max(maxGameViewAspectRatio, originalAspect);
    // const float unlockedMapAspect   = pt != nullptr ? pt->getMapLimit() / originalSize.y : originalAspect;
    // const float clampedMaxViewAspect = za::min(configuredMaxAspect, unlockedMapAspect);

    return za::max(windowAspect, originalAspect);
}


////////////////////////////////////////////////////////////
za::Vec2f Main::getExpandedGameViewSize(const za::Vec2f originalSize, const za::Vec2f windowSize) const
{
    return {originalSize.y * getCappedGameViewAspectRatio(originalSize, windowSize), originalSize.y};
}


////////////////////////////////////////////////////////////
za::View Main::createScaledGameView(const za::Vec2f originalSize, const za::Vec2f windowSize) const
{
    const float     originalAspect = originalSize.x / originalSize.y;
    const float     windowAspect   = windowSize.x / windowSize.y;
    const float     viewAspect     = getCappedGameViewAspectRatio(originalSize, windowSize);
    const za::Vec2f expandedSize   = getExpandedGameViewSize(originalSize, windowSize);
    const float     viewportWidth  = viewAspect < windowAspect ? viewAspect / windowAspect : 1.f;
    const float     viewportHeight = windowAspect < originalAspect ? windowAspect / originalAspect : 1.f;

    return {.center   = originalSize / 2.f,
            .size     = expandedSize,
            .viewport = {{0.f, (1.f - viewportHeight) * 0.5f}, {viewportWidth, viewportHeight}}};
}


////////////////////////////////////////////////////////////
za::View Main::createScaledTopGameView(const za::Vec2f originalSize, const za::Vec2f windowSize) const
{
    const za::View scaledGameView = createScaledGameView(originalSize, windowSize);

    return {.center   = scaledGameView.center,
            .size     = scaledGameView.size.componentWiseDiv(scaledGameView.viewport.size),
            .viewport = {{0.f, 0.f}, {1.f, 1.f}}};
}


////////////////////////////////////////////////////////////
za::Vec2f Main::getCurrentGameViewSize() const
{
    return getExpandedGameViewSize(gameScreenSize, getResolution());
}


////////////////////////////////////////////////////////////
float Main::clampGameViewCenterX(const float desiredCenterX, const float viewWidth) const
{
    if (viewWidth >= boundaries.x)
        return boundaries.x / 2.f;

    const float halfWidth = viewWidth / 2.f;
    return za::clamp(desiredCenterX, halfWidth, boundaries.x - halfWidth);
}


////////////////////////////////////////////////////////////
za::View Main::makeScaledHUDView(const za::Vec2f resolution, float scale) const
{
    return {.center = {resolution.x / (2.f * scale), resolution.y / (2.f * scale)}, .size = resolution / scale};
}


////////////////////////////////////////////////////////////
za::Vec2f Main::getHUDMousePos() const
{
    return nonScaledHUDView.screenToWorld(za::Mouse::getPosition(window).toVec2f(), window.getSize().toVec2f());
}


////////////////////////////////////////////////////////////
void Main::switchToBGM(const za::SizeT index, const bool force)
{
#ifndef BUBBLEBYTE_NO_AUDIO
    if (!force && lastPlayedMusic == bgmPaths[index])
        return;

    lastPlayedMusic    = bgmPaths[index];
    bgmTransition.time = 1000.f;

    auto& optNextMusic = getNextBGMBuffer();
    optNextMusic.emplace(playbackDevice, za::MusicReader::openFromFile(bgmPaths[index]).value());

    optNextMusic->music.setVolume(0.f);
    optNextMusic->music.setLooping(true);
    optNextMusic->music.setAttenuation(0.f);
    optNextMusic->music.setSpatializationEnabled(false);
    optNextMusic->music.play();
#else
    (void)index;
    (void)force;
#endif
}


////////////////////////////////////////////////////////////
za::Optional<BGMBuffer>& Main::getCurrentBGMBuffer()
{
    return bgm.bgmBuffers[currentBGMBufferIdx % 2u];
}


////////////////////////////////////////////////////////////
za::Optional<BGMBuffer>& Main::getNextBGMBuffer()
{
    return bgm.bgmBuffers[(currentBGMBufferIdx + 1u) % 2u];
}


////////////////////////////////////////////////////////////
float Main::getHueByCatType(const CatType catType)
{
    if (catType == CatType::Normal)
        return rng.getF(-20.f, 20.f);

    if (catType == CatType::Uni)
        return rng.getF(0.f, 360.f);

    if (catType == CatType::Devil)
        return rng.getF(-25.f, 25.f);

    if (catType == CatType::Astro)
        return rng.getF(-20.f, 20.f);

    return 0;
}


////////////////////////////////////////////////////////////
void Main::beginPrestigeTransition(const PrestigePointsType ppReward)
{
    playSound(sounds.prestige);

    buyReminder = 0;

    inPrestigeTransition    = true;
    playerInputState.scroll = 0.f;

    resetAllDraggedCats();
    pt->onPrestige(ppReward);

    profile.selectedBackground = 0;
    profile.selectedBGM        = 0;

    updateSelectedBackgroundSelectorIndex();
    updateSelectedBGMSelectorIndex();

    switchToBGM(static_cast<za::SizeT>(profile.selectedBGM), /* force */ true);
}


////////////////////////////////////////////////////////////
za::Optional<za::Rect2f> Main::getAoEDragRect(const za::Vec2f mousePos) const
{
    if (!playerInputState.catDragOrigin.hasValue())
        return za::nullOpt;

    return za::makeOptional<za::Rect2f>(*playerInputState.catDragOrigin, mousePos - *playerInputState.catDragOrigin);
}


////////////////////////////////////////////////////////////
void Main::resetAllDraggedCats()
{
    playerInputState.catDragPressDuration = 0.f;
    playerInputState.catDragOrigin.reset();
    playerInputState.draggedCats.clear();
    playerInputState.draggedCatsStartedWithTouch        = false;
    playerInputState.draggedCatsStartedFromAOESelection = false;
    playerInputState.catToPlace                         = nullptr;
}


////////////////////////////////////////////////////////////
za::SizeT Main::pickDragPivotCatIndex() const
{
    ZA_ASSERT(!playerInputState.draggedCats.empty());

    if (playerInputState.draggedCats.size() <= 2u)
        return 0u;

    za::Vec2f centroid;

    for (const Cat* cat : playerInputState.draggedCats)
        centroid += cat->position;

    centroid /= static_cast<float>(playerInputState.draggedCats.size());

    // Find the position closest to the centroid
    za::SizeT closestIndex       = 0u;
    float     minDistanceSquared = ZA_FLOAT_MAX;

    for (za::SizeT i = 0u; i < playerInputState.draggedCats.size(); ++i)
    {
        const float distSquared = (playerInputState.draggedCats[i]->position - centroid).lengthSquared();

        if (minDistanceSquared - distSquared < 64.f)
        {
            minDistanceSquared = distSquared;
            closestIndex       = i;
        }
    }

    return closestIndex;
}


////////////////////////////////////////////////////////////
bool Main::isCatBeingDragged(const Cat& cat) const
{
    for (const Cat* c : playerInputState.draggedCats)
        if (c == &cat)
            return true;

    return false;
}


////////////////////////////////////////////////////////////
void Main::stopDraggingCat(const Cat& cat)
{
    za::vectorEraseIf(playerInputState.draggedCats, [&](const Cat* c) { return c == &cat; });
}


////////////////////////////////////////////////////////////
bool Main::isWizardBusy() const
{
    const Cat* wizardCat = getWizardCat();

    if (wizardCat == nullptr)
        return false;

    return pt->absorbingWisdom || wizardCat->cooldown.time != 0.f || wizardCat->isHexedOrCopyHexed() ||
           isCatBeingDragged(*wizardCat);
}


////////////////////////////////////////////////////////////
Cat* Main::findFirstCatByType(const CatType catType) const
{
    for (Cat& cat : pt->cats)
        if (cat.type == catType)
            return &cat;

    return nullptr;
}


////////////////////////////////////////////////////////////
Cat* Main::getWitchCat() const
{
    return findFirstCatByType(CatType::Witch);
}


////////////////////////////////////////////////////////////
Cat* Main::getWizardCat() const
{
    return findFirstCatByType(CatType::Wizard);
}


////////////////////////////////////////////////////////////
Cat* Main::getMouseCat() const
{
    return findFirstCatByType(CatType::Mouse);
}


////////////////////////////////////////////////////////////
Cat* Main::getEngiCat() const
{
    return findFirstCatByType(CatType::Engi);
}


////////////////////////////////////////////////////////////
Cat* Main::getRepulsoCat() const
{
    return findFirstCatByType(CatType::Repulso);
}


////////////////////////////////////////////////////////////
Cat* Main::getAttractoCat() const
{
    return findFirstCatByType(CatType::Attracto);
}


////////////////////////////////////////////////////////////
Cat* Main::getCopyCat() const
{
    return findFirstCatByType(CatType::Copy);
}


////////////////////////////////////////////////////////////
void Main::addCombo(int& xCombo, Countdown& xComboCountdown) const
{
    if (xCombo == 0)
    {
        xCombo               = 1;
        xComboCountdown.time = pt->psvComboStartTime.currentValue() * 1000.f;
    }
    else
    {
        xCombo += 1;
        xComboCountdown.time += 150.f - za::clamp(static_cast<float>(xCombo) * 10.f, 0.f, 100.f);
    }
}


////////////////////////////////////////////////////////////
bool Main::checkComboEnd(const float deltaTimeMs, int& xCombo, Countdown& xComboCountdown)
{
    if (xComboCountdown.tick(deltaTimeMs) != TickResult::JustFinished)
        return false;

    xCombo = 0;
    return true;
}


////////////////////////////////////////////////////////////
void Main::turnBubbleInto(Bubble& bubble, const BubbleType newType)
{
    // Drop the credit of a bomb's creator along with the bomb, whatever it turns into: otherwise
    // the next bomb in this slot (e.g. after recycling) would pay out to the wrong cat
    if (bubble.type == BubbleType::Bomb)
        bombStorage->bombIdxToCatIdx.erase(static_cast<za::SizeT>(&bubble - pt->bubbles.data()));

    bubble.type               = newType;
    bubble.absorptionProgress = 0.f;

    if (newType == BubbleType::Normal)
    {
        bubble.rotation = 0.f;
        bubble.torque   = 0.f;
        bubble.hueMod   = 0.f;

        return;
    }

    if (newType == BubbleType::Star || newType == BubbleType::Nova)
    {
        bubble.hueMod = rng.getF(0.f, 360.f);
        bubble.torque = 0.001f * rng.getSignF();

        return;
    }

    if (newType == BubbleType::Bomb)
    {
        bubble.torque = 0.004f * rng.getSignF();
        bubble.hueMod = 0.f;

        return;
    }
}


////////////////////////////////////////////////////////////
void Main::doWizardSpellStarpawConversion(Cat& wizardCat)
{
    const auto range       = getComputedRangeByCatTypeOrCopyCat(wizardCat.type);
    const auto maxCooldown = getComputedCooldownByCatTypeOrCopyCat(wizardCat.type);

    sounds.cast0.settings.position = {wizardCat.position.x, wizardCat.position.y};
    playSound(sounds.cast0);

    spawnParticlesNoGravity(256, wizardCat.position, ParticleType::Star, rngFast.getF(0.25f, 1.25f), rngFast.getF(0.5f, 3.f));

    forEachBubbleInRadius(wizardCat.position,
                          range,
                          [&](Bubble& bubble)
    {
        if (bubble.type == BubbleType::Combo)
            return ControlFlow::Continue;

        if (pt->perm.starpawConversionIgnoreBombs && bubble.type != BubbleType::Normal)
            return ControlFlow::Continue;

        if (rng.getF(0.f, 99.f) > pt->psvStarpawPercentage.currentValue())
            return ControlFlow::Continue;


        turnBubbleInto(bubble, pt->perm.starpawNova ? BubbleType::Nova : BubbleType::Star);
        bubble.velocity.y -= rng.getF(0.025f, 0.05f);

        spawnParticles(1, bubble.position, ParticleType::Star, 0.5f, 0.35f);

        return ControlFlow::Continue;
    });

    ++wizardCat.hits;
    wizardCat.cooldown.time = maxCooldown * 2.f;
}


////////////////////////////////////////////////////////////
void Main::doWizardSpellMewltiplierAura(Cat& wizardCat)
{
    const auto maxCooldown = getComputedCooldownByCatTypeOrCopyCat(wizardCat.type);

    sounds.cast0.settings.position = {wizardCat.position.x, wizardCat.position.y};
    playSound(sounds.cast0);

    spawnParticlesNoGravity(256, wizardCat.position, ParticleType::Star, rngFast.getF(0.25f, 1.25f), rngFast.getF(0.5f, 3.f));

    ++wizardCat.hits;
    wizardCat.cooldown.time = maxCooldown * 2.f;
}


////////////////////////////////////////////////////////////
void Main::doWizardSpellDarkUnion(Cat& wizardCat)
{
    const auto range       = getComputedRangeByCatTypeOrCopyCat(wizardCat.type);
    const auto maxCooldown = getComputedCooldownByCatTypeOrCopyCat(wizardCat.type);

    Cat* witchCat = getWitchCat();

    const bool castSuccessful = pt->hexSessions.empty() && witchCat != nullptr &&
                                (witchCat->position - wizardCat.position).lengthSquared() <= range * range;

    if (castSuccessful)
    {
        sounds.cast0.settings.position = {wizardCat.position.x, wizardCat.position.y};
        playSound(sounds.cast0);

        spawnParticlesNoGravity(256, wizardCat.position, ParticleType::Star, rngFast.getF(0.25f, 1.25f), rngFast.getF(0.5f, 3.f));

        spawnParticlesNoGravity(256, witchCat->position, ParticleType::Star, rngFast.getF(0.25f, 1.25f), rngFast.getF(0.5f, 3.f));

        witchCat->cooldown.time -= witchCat->cooldown.time * (pt->psvDarkUnionPercentage.currentValue() / 100.f);
    }
    else
    {
        sounds.failcast.settings.position = {wizardCat.position.x, wizardCat.position.y};
        playSound(sounds.failcast);
    }

    ++wizardCat.hits;
    wizardCat.cooldown.time = maxCooldown * 4.f;
}


////////////////////////////////////////////////////////////
void Main::doWizardSpellStasisField(Cat& wizardCat)
{
    const auto maxCooldown = getComputedCooldownByCatTypeOrCopyCat(wizardCat.type);

    sounds.cast0.settings.position = {wizardCat.position.x, wizardCat.position.y};
    playSound(sounds.cast0);

    spawnParticlesNoGravity(256, wizardCat.position, ParticleType::Star, rngFast.getF(0.25f, 1.25f), rngFast.getF(0.5f, 3.f));

    ++wizardCat.hits;
    wizardCat.cooldown.time = maxCooldown * 2.f;
}


////////////////////////////////////////////////////////////
void Main::castSpellByIndex(const za::SizeT index, Cat* wizardCat, Cat* copyCat)
{
    ZA_ASSERT(index < 4u);

    const bool copyCatMustCast = copyCat != nullptr && pt->copycatCopiedCatType == CatType::Wizard;

    wizardcatSpin.time = za::tau;
    statSpellCast(index);

    if (index == 0u) // Starpaw Conversion
    {
        doWizardSpellStarpawConversion(*wizardCat);

        if (copyCatMustCast)
            doWizardSpellStarpawConversion(*copyCat);

        return;
    }

    if (index == 1u) // Mewltiplier Aura
    {
        pt->mewltiplierAuraTimer += pt->perm.wizardCatDoubleMewltiplierDuration ? 12'000.f : 6000.f;

        doWizardSpellMewltiplierAura(*wizardCat);

        if (copyCatMustCast)
            doWizardSpellMewltiplierAura(*copyCat);

        return;
    }

    if (index == 2u) // Dark Union
    {
        doWizardSpellDarkUnion(*wizardCat);

        if (copyCatMustCast)
            doWizardSpellDarkUnion(*copyCat);

        return;
    }

    if (index == 3u) // Stasis Field
    {
        pt->stasisFieldTimer += pt->perm.wizardCatDoubleStasisFieldDuration ? 12'000.f : 6000.f;

        doWizardSpellStasisField(*wizardCat);

        if (copyCatMustCast)
            doWizardSpellStasisField(*copyCat);

        return;
    }
}


////////////////////////////////////////////////////////////
bool Main::mustApplyMewltiplierAura(const za::Vec2f bubblePosition) const
{
    if (pt->mewltiplierAuraTimer <= 0.f)
        return false;

    const Cat* wizardCat = getWizardCat();
    if (wizardCat == nullptr)
        return false;

    const float wizardCatRangeSquared = pt->getComputedSquaredRangeByCatType(CatType::Wizard);

    if ((wizardCat->position - bubblePosition).lengthSquared() <= wizardCatRangeSquared)
        return true;

    const Cat* copyCat = getCopyCat();
    if (copyCat == nullptr || pt->copycatCopiedCatType != CatType::Wizard)
        return false;

    if ((copyCat->position - bubblePosition).lengthSquared() <= wizardCatRangeSquared)
        return true;

    return false;
}


////////////////////////////////////////////////////////////
MoneyType Main::computeFinalReward(const Bubble& bubble, const float multiplier, const float comboMult, const Cat* popperCat) const
{
    const bool byPlayerClick = popperCat == nullptr;

    const bool popperCatIsMousecat = //
        !byPlayerClick && (popperCat->type == CatType::Mouse ||
                           (popperCat->type == CatType::Copy && pt->copycatCopiedCatType == CatType::Mouse));

    const bool popperCatIsNormal = !byPlayerClick && popperCat->type == CatType::Normal;

    const bool mustApplyHandMult = byPlayerClick || popperCatIsMousecat; // mousecat benefits from click and cat mults
    const bool mustApplyCatMult  = !byPlayerClick;

    const bool nearShrineOfClicking = byPlayerClick && ([&]
    {
        for (const Shrine& shrine : pt->shrines)
            if (shrine.type == ShrineType::Clicking && shrine.isInRange(bubble.position))
                return true;

        return false;
    })();

    // Base reward: bubble value by type multiplied by static multiplier (e.g. x10 for bombs, x20 for astro)
    float result = static_cast<float>(pt->getComputedRewardByBubbleType(bubble.type)) * multiplier;

    // Combo mult: applied for player clicks or mousecat clicks
    result *= comboMult;

    // Wizard spells: mewltiplier aura
    if (mustApplyMewltiplierAura(bubble.position))
        result *= pt->psvMewltiplierMult.currentValue();

    // Global bonus -- mousecat (applies to clicks)
    const bool isMouseBeingCopied = getCopyCat() != nullptr && pt->copycatCopiedCatType == CatType::Mouse;
    if (mustApplyHandMult && getMouseCat() != nullptr)
        result *= pt->psvPPMouseCatGlobalBonusMult.currentValue() * (isMouseBeingCopied ? 2.f : 1.f);

    // Global bonus -- engicat (applies to cats)
    const bool isEngiBeingCopied = getCopyCat() != nullptr && pt->copycatCopiedCatType == CatType::Engi;
    if (mustApplyCatMult && getEngiCat() != nullptr)
        result *= pt->psvPPEngiCatGlobalBonusMult.currentValue() * (isEngiBeingCopied ? 2.f : 1.f);

    // Shrine of clicking: x5 reward for clicks
    if (mustApplyHandMult && nearShrineOfClicking)
        result *= 5.f;

    // Ritual buff -- normalcat: x5 reward for cats
    if (mustApplyCatMult && pt->buffCountdownsPerType[asIdx(CatType::Normal)].time > 0.f)
        result *= 5.f;

    // Ritual buff -- mousecat: x10 reward for clicks
    if (mustApplyHandMult && pt->buffCountdownsPerType[asIdx(CatType::Mouse)].time > 0.f)
        result *= 10.f;

    // Genius cats: x2 reward for normal cats only
    if (!byPlayerClick && popperCatIsNormal && pt->perm.geniusCatsPurchased)
        result *= 2.f;

    // Repulsocat: x2 reward for repelled bubbles
    if (!bubble.repelledCountdown.isDone())
        result *= 2.f;

    // Attractocat: x2 reward for attracted bubbles
    if (!bubble.attractedCountdown.isDone())
        result *= 2.f;

    return static_cast<MoneyType>(za::ceil(result));
}


////////////////////////////////////////////////////////////
za::Vec2u Main::getReasonableWindowSize(const float scalingFactorMult)
{
    constexpr float gameRatio = gameScreenSize.x / gameScreenSize.y;

    const auto fullscreenSize = za::VideoModeUtils::getDesktopMode().size.toVec2f();

    const float aspectRatio = fullscreenSize.x / fullscreenSize.y;

    const bool isUltrawide = aspectRatio >= 2.f;
    const bool isWide      = aspectRatio >= 1.6f && aspectRatio < 2.f;

    const float scalingFactor = isUltrawide ? 0.9f : isWide ? 0.8f : 0.7f;

    const auto windowSize = fullscreenSize * scalingFactor * scalingFactorMult;

    const auto windowedWidth = windowSize.y * gameRatio + (uiWindowWidth + 35.f);

    return za::Vec2f{windowedWidth + 12.f, windowSize.y}.toVec2u();
}


////////////////////////////////////////////////////////////
int Main::pickSelectedIndex(const za::Vector<SelectorEntry>& entries, const int selectedIndex)
{
    const auto selectedIndexU = static_cast<za::SizeT>(selectedIndex);
    return selectedIndexU < entries.size() ? entries[selectedIndexU].index : 0;
}


////////////////////////////////////////////////////////////
void Main::selectBackground(const za::Vector<SelectorEntry>& entries, const int selectedIndex)
{
    profile.selectedBackground = pickSelectedIndex(entries, selectedIndex);
}


////////////////////////////////////////////////////////////
void Main::selectBGM(const za::Vector<SelectorEntry>& entries, const int selectedIndex)
{
    profile.selectedBGM = pickSelectedIndex(entries, selectedIndex);
}


////////////////////////////////////////////////////////////
void Main::updateSelectedBackgroundSelectorIndex() const
{
    auto& [entries, selectedIndex] = getBackgroundSelectorData();

    for (za::SizeT i = 0u; i < entries.size(); ++i)
        if (profile.selectedBackground == entries[i].index)
            selectedIndex = static_cast<int>(i);
}


////////////////////////////////////////////////////////////
void Main::updateSelectedBGMSelectorIndex() const
{
    auto& [entries, selectedIndex] = getBGMSelectorData();

    for (za::SizeT i = 0u; i < entries.size(); ++i)
        if (profile.selectedBGM == entries[i].index)
            selectedIndex = static_cast<int>(i);
}


////////////////////////////////////////////////////////////
Main::SelectorData& Main::getBGMSelectorData() const
{
    static thread_local SelectorData data;
    data.entries.clear();

    data.entries.emplaceBack(0, "Default");

    if (pt->perm.shrineCompletedOnceByCatType[asIdx(CatType::Witch)])
        data.entries.emplaceBack(1, "Ritual Circle");

    if (pt->perm.shrineCompletedOnceByCatType[asIdx(CatType::Wizard)])
        data.entries.emplaceBack(2, "The Wise One");

    if (pt->perm.shrineCompletedOnceByCatType[asIdx(CatType::Mouse)])
        data.entries.emplaceBack(3, "Click N Chill");

    if (pt->perm.shrineCompletedOnceByCatType[asIdx(CatType::Engi)])
        data.entries.emplaceBack(4, "Use More Cat");

    if (data.selectedIndex == -1)
    {
        data.selectedIndex = [&]
        {
            for (za::SizeT i = 0u; i < data.entries.size(); ++i)
                if (profile.selectedBGM == data.entries[i].index)
                    return static_cast<int>(i);

            return 0;
        }();
    }

    return data;
}


////////////////////////////////////////////////////////////
Main::SelectorData& Main::getBackgroundSelectorData() const
{
    static thread_local SelectorData data;
    data.entries.clear();

    data.entries.emplaceBack(0, "Default");

    if (pt->perm.shrineCompletedOnceByCatType[asIdx(CatType::Witch)])
        data.entries.emplaceBack(1, "Swamp");

    if (pt->perm.shrineCompletedOnceByCatType[asIdx(CatType::Wizard)])
        data.entries.emplaceBack(2, "Observatory");

    if (pt->perm.shrineCompletedOnceByCatType[asIdx(CatType::Mouse)])
        data.entries.emplaceBack(3, "Aim Labs");

    if (pt->perm.shrineCompletedOnceByCatType[asIdx(CatType::Engi)])
        data.entries.emplaceBack(4, "Factory");

    if (pt->perm.shrineCompletedOnceByCatType[asIdx(CatType::Repulso)])
        data.entries.emplaceBack(5, "Wind Tunnel");

    if (pt->perm.shrineCompletedOnceByCatType[asIdx(CatType::Attracto)])
        data.entries.emplaceBack(6, "Magnetosphere");

    if (pt->perm.shrineCompletedOnceByCatType[asIdx(CatType::Copy)])
        data.entries.emplaceBack(7, "Auditorium");

    if (data.selectedIndex == -1)
    {
        data.selectedIndex = [&]
        {
            for (za::SizeT i = 0u; i < data.entries.size(); ++i)
                if (profile.selectedBackground == data.entries[i].index)
                    return static_cast<int>(i);

            return 0;
        }();
    }

    return data;
}


////////////////////////////////////////////////////////////
void Main::reseedRNGs(const RNGSeedType newSeed)
{
    seed    = newSeed;
    rng     = RNGFast{seed};
    rngFast = RNGFast{seed};
}


////////////////////////////////////////////////////////////
void Main::forceResetGame(const bool goToShopTab)
{
    soundManager.stopPlayingAll(sounds.ritual);
    soundManager.stopPlayingAll(sounds.copyritual);

    delayedActions.clear();

    reseedRNGs(static_cast<RNGSeedType>(za::Clock::now().asMicroseconds()));

    shuffledCatNamesPerType = makeShuffledCatNames(rng);

    *pt      = Playthrough{};
    pt->seed = seed;

    wasPrestigeAvailableLastFrame = false;
    buyReminder                   = 0u;

    resetAllDraggedCats();
    resetTipState();

    particles.clear();
    textParticles.clear();
    spentCoinParticles.clear();
    hudBottomParticles.clear();
    hudTopParticles.clear();
    earnedCoinParticles.clear();

    inPrestigeTransition = false;

    comboState.combo            = 0u;
    comboState.laserCursorCombo = 0;

    comboState.comboNStars         = 0;
    comboState.comboNOthers        = 0;
    comboState.comboAccReward      = 0;
    comboState.comboAccStarReward  = 0;
    comboState.iComboAccReward     = 0;
    comboState.iComboAccStarReward = 0;

    playerInputState.scroll = 0.f;

    screenShakeAmount = 0.f;
    screenShakeTimer  = 0.f;

    spentMoney            = 0u;
    moneyGainedLastSecond = 0u;
    samplerMoneyPerSecond.clear();

    bombStorage->bombIdxToCatIdx.clear();
    uiState.purchaseUnlockedEffects.clear();
    uiState.btnWasDisabled.clear();
    undoPPPurchase.clear();

    if (goToShopTab)
        uiState.shopSelectOnce = ImGuiTabItemFlags_SetSelected;

    profile.selectedBackground = 0;
    profile.selectedBGM        = 0;

    updateSelectedBackgroundSelectorIndex();
    updateSelectedBGMSelectorIndex();

    switchToBGM(static_cast<za::SizeT>(profile.selectedBGM), /* force */ true);
}


////////////////////////////////////////////////////////////
void Main::forceResetProfile()
{
    profile = Profile{};
    forceResetGame();
}


////////////////////////////////////////////////////////////
TextParticle& Main::makeRewardTextParticle(const za::Vec2f position)
{
    return textParticles.emplaceBack(TextParticle{
        {.position      = {position.x, position.y - 10.f},
         .velocity      = rngFast.getVec2f({-0.1f, -1.65f}, {0.1f, -1.35f}) * 0.395f,
         .scale         = za::clamp(1.f + 0.1f * static_cast<float>(comboState.combo + 1) / 1.75f, 1.f, 3.f) * 0.5f,
         .scaleDecay    = 0.f,
         .accelerationY = 0.0035f,
         .opacity       = 1.f,
         .opacityDecay  = 0.0015f,
         .rotation      = 0.f,
         .torque        = rngFast.getF(-0.002f, 0.002f)}});
}


////////////////////////////////////////////////////////////
void Main::shrineCollectReward(Shrine& shrine, const MoneyType reward, const Bubble& bubble)
{
    shrine.collectedReward += reward;
    shrine.textStatusShakeEffect.bump(rngFast, 1.5f);

    spawnParticlesWithHue(wrapHue(shrine.getHue() + 40.f),
                          6,
                          shrine.getDrawPosition(),
                          ParticleType::Fire,
                          rngFast.getF(0.25f, 0.6f),
                          0.75f);

    spawnParticlesWithHue(shrine.getHue(), 6, shrine.getDrawPosition(), ParticleType::Shrine, rngFast.getF(0.6f, 1.f), 0.5f);

    const auto diff = bubble.position - shrine.position;

    spawnParticle({.position      = bubble.position,
                   .velocity      = -diff.normalized() * 0.5f,
                   .scale         = 1.5f,
                   .scaleDecay    = 0.f,
                   .accelerationY = 0.f,
                   .opacity       = 1.f,
                   .opacityDecay  = 0.00135f + (shrine.getRange() - diff.length()) / 22000.f,
                   .rotation      = 0.f,
                   .torque        = 0.f},
                  /* hue */ 0.f,
                  ParticleType::Bubble);
}


////////////////////////////////////////////////////////////
void Main::doExplosion(Bubble& bubble)
{
    sounds.explosion.settings.position = {bubble.position.x, bubble.position.y};
    playSound(sounds.explosion);

    for (za::SizeT iP = 0u; iP < 16u; ++iP)
    {
        spawnParticle(
            ParticleData{
                .position      = bubble.position,
                .velocity      = rngFast.getVec2f({-0.75f, -1.75f}, {0.75f, -0.75f}) * 0.3f,
                .scale         = rngFast.getF(0.08f, 0.27f) * 2.5f,
                .scaleDecay    = 0.0002f,
                .accelerationY = 0.002f,
                .opacity       = 0.65f,
                .opacityDecay  = rngFast.getF(0.00025f, 0.0015f),
                .rotation      = rngFast.getF(0.f, za::tau),
                .torque        = rngFast.getF(-0.002f, 0.002f) * 5.f,
            },
            0.f,
            ParticleType::Fire);

        spawnParticle(ParticleData{.position      = bubble.position,
                                   .velocity      = za::Vec2f::fromAngle(rngFast.getF(0.4f, 0.8f),
                                                                         za::radians(za::tau / static_cast<float>(16u) *
                                                                                     static_cast<float>(iP))),
                                   .scale         = rngFast.getF(0.08f, 0.27f) * 2.75f,
                                   .scaleDecay    = -0.0025f,
                                   .accelerationY = 0.000001f,
                                   .opacity       = 0.35f,
                                   .opacityDecay  = rngFast.getF(0.001f, 0.002f) * 0.6f,
                                   .rotation      = rngFast.getF(0.f, za::tau),
                                   .torque        = rngFast.getF(-0.001f, 0.001f)},
                      0.f,
                      ParticleType::Explosion);
    }

    for (za::SizeT iP = 0u; iP < 8u; ++iP)
        spawnParticle(ParticleData{.position      = bubble.position,
                                   .velocity      = {rngFast.getF(-0.15f, 0.15f), rngFast.getF(-0.15f, 0.05f)},
                                   .scale         = rngFast.getF(0.65f, 1.f) * 1.25f,
                                   .scaleDecay    = -0.0005f,
                                   .accelerationY = -0.00017f,
                                   .opacity       = rngFast.getF(0.5f, 0.75f) * 0.7f,
                                   .opacityDecay  = rngFast.getF(0.00035f, 0.00055f) * 0.8f,
                                   .rotation      = rngFast.getF(0.f, za::tau),
                                   .torque        = rngFast.getF(-0.002f, 0.002f)},
                      0.f,
                      ParticleType::Smoke);

    // TODO P2: cleanup
    const auto  bubbleIdx  = static_cast<za::SizeT>(&bubble - pt->bubbles.data());
    const auto* bombIdxItr = bombStorage->bombIdxToCatIdx.find(bubbleIdx);

    Cat* catWhoMadeBomb = bombIdxItr != bombStorage->bombIdxToCatIdx.end() ? pt->cats.data() + bombIdxItr->second : nullptr;

    const float explosionRadius = pt->getComputedBombExplosionRadius();

    forEachBubbleInRadius(bubble.position,
                          explosionRadius,
                          [&](Bubble& otherBubble)
    {
        if (otherBubble.type == BubbleType::Bomb)
            return ControlFlow::Continue;

        if (otherBubble.type == BubbleType::Combo)
            return ControlFlow::Continue;

        const MoneyType otherReward = computeFinalReward(/* bubble     */ otherBubble,
                                                         /* multiplier */ 10.f,
                                                         /* comboMult  */ 1.f,
                                                         /* popperCat  */ catWhoMadeBomb);

        statExplosionRevenue(otherReward);

        popWithRewardAndReplaceBubble({
            .reward          = otherReward,
            .bubble          = otherBubble,
            .xCombo          = 1,
            .popSoundOverlap = false,
            .popperCat       = catWhoMadeBomb,
            .multiPop        = false,
        });

        return ControlFlow::Continue;
    });

    if (pt->perm.witchCatBuffFlammableDolls)
    {
        for (HexSession& session : pt->hexSessions)
            for (Doll& doll : session.dolls)
                if ((doll.position - bubble.position).length() <= explosionRadius && !doll.tcDeath.hasValue())
                    collectDoll(doll, session);

        for (HexSession& session : pt->copyHexSessions)
            for (Doll& copyDoll : session.dolls)
                if ((copyDoll.position - bubble.position).length() <= explosionRadius && !copyDoll.tcDeath.hasValue())
                    collectCopyDoll(copyDoll, session);
    }

    if (catWhoMadeBomb != nullptr)
        bombStorage->bombIdxToCatIdx.erase(bombIdxItr);
}


////////////////////////////////////////////////////////////
za::Vec2f Main::fromWorldToHud(const za::Vec2f point) const
{
    // From game coordinates to screen coordinates
    const za::Vec2f screenPos = gameView.worldToScreen(point, window.getSize().toVec2f());

    // From screen coordinates to HUD view coordinates
    return scaledHUDView.screenToWorld(screenPos, window.getSize().toVec2f());
}


////////////////////////////////////////////////////////////
bool Main::isBubbleInStasisField(const Bubble& bubble) const
{
    if (pt->stasisFieldTimer <= 0.f)
        return false;

    const auto rangeSquared = pt->getComputedSquaredRangeByCatType(CatType::Wizard);

    if (const Cat* wizardCat = getWizardCat(); wizardCat != nullptr)
        if ((bubble.position - wizardCat->position).lengthSquared() <= rangeSquared)
            return true;

    if (const Cat* copyCat = getCopyCat(); copyCat != nullptr && pt->copycatCopiedCatType == CatType::Wizard)
        if ((bubble.position - copyCat->position).lengthSquared() <= rangeSquared)
            return true;

    return false;
}


////////////////////////////////////////////////////////////
void Main::popWithRewardAndReplaceBubble(const BubblePopData& data)
{
    const auto& [reward, bubble, xCombo, popSoundOverlap, popperCat, multiPop] = data;

    const bool byPlayerClick = popperCat == nullptr;

    // Combo bubbles are interactive only with player clicks: cats can't
    // pop them (silently ignore the swing) and they never get destroyed
    // or replaced -- they fall off the bottom on their own (ephemeral).
    if (bubble.type == BubbleType::Combo && !byPlayerClick)
        return;

    // Bubble-transform (Unicat star / Devilcat bomb / ...) in progress:
    // the bubble is frozen mid-air and inert until the animation resolves.
    // Silently ignore any pop attempt so the transformation can complete.
    if (bubble.pendingTransformMs > 0.f)
        return;

    statBubblePopped(bubble.type, byPlayerClick, reward);

    if (byPlayerClick && bubble.type == BubbleType::Star)
        statHighestStarBubblePopCombo(static_cast<za::U64>(comboState.combo));

    if (byPlayerClick && bubble.type == BubbleType::Nova)
        statHighestNovaBubblePopCombo(static_cast<za::U64>(comboState.combo));

    const Shrine* collectorShrine = nullptr;
    for (Shrine& shrine : pt->shrines)
    {
        if ((bubble.position - shrine.position).lengthSquared() > shrine.getRangeSquared())
            continue;

        collectorShrine = &shrine;
        shrineCollectReward(shrine, reward, bubble);
    }

    const bool      collectedByShrine = collectorShrine != nullptr;
    const za::Vec2f tpPosition        = collectedByShrine ? collectorShrine->getDrawPosition() : bubble.position;

    if (profile.showTextParticles)
    {
        auto& tp = makeRewardTextParticle(tpPosition);
        std::snprintf(tp.buffer, sizeof(tp.buffer), "+$%llu", reward);
    }

    if (profile.accumulatingCombo && !multiPop && byPlayerClick && pt->comboPurchased && !collectedByShrine)
    {
        if (bubble.type == BubbleType::Star || bubble.type == BubbleType::Nova)
            comboState.comboNStars += 1;
        else
            comboState.comboNOthers += 1;
    }

    if (profile.showCoinParticles)
    {
        if (!collectedByShrine && profile.showCoinParticles)
            spawnSpentCoinParticle(
                {.position      = moneyText.getGlobalCenterRight() + za::Vec2f{32.f, rngFast.getF(-12.f, 12.f)},
                 .velocity      = {-0.25f, 0.f},
                 .scale         = 0.25f,
                 .scaleDecay    = 0.f,
                 .accelerationY = 0.f,
                 .opacity       = 0.f,
                 .opacityDecay  = -0.003f,
                 .rotation      = rngFast.getF(0.f, za::tau),
                 .torque        = 0.f});


        const za::Vec2f hudPos = fromWorldToHud(bubble.position);

        if ((!profile.accumulatingCombo || !pt->comboPurchased || !byPlayerClick) && !collectedByShrine &&
            spawnEarnedCoinParticle(hudPos))
        {
            const za::Vec2f viewSize           = getCurrentGameViewSize();
            const za::Vec2f viewCenter         = getViewCenter();
            sounds.coindelay.settings.position = {viewCenter.x - viewSize.x / 2.f + 25.f,
                                                  viewCenter.y - viewSize.y / 2.f + 25.f};

            sounds.coindelay.settings.pitch  = 1.f;
            sounds.coindelay.settings.volume = profile.sfxVolume / 100.f * 0.5f;

            playSound(sounds.coindelay, /* maxOverlap */ 64);
        }
    }

    sounds.pop.settings.position = {bubble.position.x, bubble.position.y};
    sounds.pop.settings.pitch    = remap(static_cast<float>(xCombo), 1, 10, 1.f, 2.f);

    playSound(sounds.pop, popSoundOverlap ? 64u : 1u);

    spawnParticles(32, bubble.position, ParticleType::Bubble, 0.5f, 0.5f);
    spawnParticles(8, bubble.position, ParticleType::Bubble, 1.2f, 0.25f);

    if (bubble.type == BubbleType::Star || bubble.type == BubbleType::Nova)
        spawnParticles(16, bubble.position, ParticleType::Star, 0.5f, 0.35f);
    else if (bubble.type == BubbleType::Bomb)
        doExplosion(bubble);

    if (popperCat != nullptr)
    {
        popperCat->moneyEarned += reward;
        popperCat->textMoneyShakeEffect.bump(rngFast, 1.25f);
    }

    if (!collectedByShrine)
    {
        addMoney(reward);
        moneyTextShakeEffect.bump(rngFast, 1.f + static_cast<float>(comboState.combo) * 0.1f);
    }

    if (bubble.type != BubbleType::Combo && !isBubbleInStasisField(bubble))
    {
        bubble = makeRandomBubble(*pt, rng, pt->getMapLimit(), 0.f);
        bubble.position.y -= bubble.radius;
    }
}


////////////////////////////////////////////////////////////
bool Main::isDebugModeEnabled() const
{
    return debugMode;
}


////////////////////////////////////////////////////////////
void Main::gameLoopCheats() const
{
    if (!isDebugModeEnabled())
        return;

    if (keyDown(za::Keyboard::Key::F4))
    {
        pt->comboPurchased = true;
        pt->mapPurchased   = true;
    }
    else if (keyDown(za::Keyboard::Key::F5))
    {
        pt->money = 1'000'000'000u;
    }
    else if (keyDown(za::Keyboard::Key::F6))
    {
        pt->money += 15u;
    }
    else if (keyDown(za::Keyboard::Key::F7))
    {
        pt->prestigePoints += 15u;
    }
}


////////////////////////////////////////////////////////////
za::Mouse::Button Main::getLMB() const
{
    return profile.invertMouseButtons ? za::Mouse::Button::Right : za::Mouse::Button::Left;
}


////////////////////////////////////////////////////////////
za::Mouse::Button Main::getRMB() const
{
    return profile.invertMouseButtons ? za::Mouse::Button::Left : za::Mouse::Button::Right;
}
