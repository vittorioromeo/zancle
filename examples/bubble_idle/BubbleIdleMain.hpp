#pragma once


// #define BUBBLEBYTE_NO_AUDIO 1
#include "Achievements.hpp"
#include "Aliases.hpp"
#include "Bubble.hpp"
#include "BubbleType.hpp"
#include "CatType.hpp"
#include "Constants.hpp"
#include "ExactArray.hpp"
#include "HexSession.hpp"
#include "Particle.hpp"
#include "ParticleData.hpp"
#include "ParticleType.hpp"
#include "PlayerInput.hpp"
#include "PurchasableScalingValue.hpp"
#include "RNGSeedType.hpp"
#include "ShrineType.hpp"
#include "Sounds.hpp"
#include "TextEffectWiggle.hpp"
#include "TextParticle.hpp"
#include "TextShakeEffect.hpp"

#include "ExampleUtils/Progress.hpp"
#include "ExampleUtils/Sampler.hpp"

#include "Zancle/Graphics/Color.hpp"

#include "Zancle/Window/Keyboard.hpp"
#include "Zancle/Window/Mouse.hpp"

#include "Zancle/Concurrency/ThreadPool.hpp"

#include "Zancle/Random/FastNonCryptoRng.hpp"

#include "Zancle/String/String.hpp"
#include "Zancle/String/StringView.hpp"

#include "Zancle/Chrono/Clock.hpp"
#include "Zancle/Chrono/Time.hpp"

#include "Zancle/Container/Array.hpp"
#include "Zancle/Container/Vector.hpp"

#include "Zancle/Geometry/Angle.hpp"
#include "Zancle/Geometry/Rect2.hpp"
#include "Zancle/Geometry/Vec2.hpp"

#include "Zancle/Vocabulary/FixedFunction.hpp"
#include "Zancle/Vocabulary/Optional.hpp"
#include "Zancle/Vocabulary/UniquePtr.hpp"

#include "Zancle/Base/GetArraySize.hpp"
#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/SizeT.hpp"

#if defined(__GNUC__) || defined(__clang__)
    #define BUBBLE_IDLE_PRINTF_FORMAT(fmtIndex, firstArgIndex) __attribute__((format(printf, fmtIndex, firstArgIndex)))
#else
    #define BUBBLE_IDLE_PRINTF_FORMAT(fmtIndex, firstArgIndex)
#endif


////////////////////////////////////////////////////////////
struct ImFont;
struct BGMBuffer;
struct ComboState;
struct LoadedSound;
struct SoundManager;
struct Doll;
struct Shrine;

namespace za
{
class AudioContext;
class CPUDrawableBatch;
class Font;
class GraphicsContext;
class ImGuiContext;
class OutFile;
class Path;
class PlaybackDevice;
class RenderTarget;
class RenderTexture;
class RenderWindow;
class Text;
class Texture;
class TextureAtlas;
struct DrawTextureSettings;
struct Listener;
struct RenderStates;
struct Sprite;
struct TextData;
struct View;
} // namespace za

struct BubbleIgnoreFlags;
struct CloudFrameDrawSettings;
struct FrameViewState;
struct GameConstants;
class InputHelper;
struct MainAtlasRects;
struct MainBGMStorage;
struct MainBombStorage;
struct MainDrawableBatches;
struct MainGameStorage;
struct MainShaders;
struct MainTextureStorage;
struct MainTextStorage;
struct NotificationState;
struct Playthrough;
struct Profile;
struct UIState;

using MainRenderTextureVector = za::Vector<za::RenderTexture>;

template <typename T>
struct MainOwnedDeleter
{
    void operator()(T* ptr) noexcept;
};

template <typename T>
using MainOwnedPtr = za::UniquePtr<T, MainOwnedDeleter<T>>;


////////////////////////////////////////////////////////////
#ifdef BUBBLEBYTE_USE_STEAMWORKS
    #include "Steam.hpp"
#endif


////////////////////////////////////////////////////////////<
extern bool debugMode;


////////////////////////////////////////////////////////////
class SweepAndPrune;


////////////////////////////////////////////////////////////
bool handleBubbleCollision(float deltaTimeMs, Bubble& iBubble, Bubble& jBubble);


////////////////////////////////////////////////////////////
bool handleCatCollision(float deltaTimeMs, Cat& iCat, Cat& jCat);


////////////////////////////////////////////////////////////
bool handleCatShrineCollision(float deltaTimeMs, Cat& cat, Shrine& shrine);


////////////////////////////////////////////////////////////
// Definition in `BubbleIdleMainInline.hpp`
[[nodiscard]] Bubble makeRandomBubble(Playthrough& pt, za::FastNonCryptoRng& rng, float mapLimit, float maxY);

/// Main struct
///
////////////////////////////////////////////////////////////
struct Main
{
////////////////////////////////////////////////////////////
// Audio context and playback device
#ifndef BUBBLEBYTE_NO_AUDIO
    MainOwnedPtr<za::AudioContext>   audioContextStorage;
    za::AudioContext&                audioContext;
    MainOwnedPtr<za::PlaybackDevice> playbackDeviceStorage;
    za::PlaybackDevice&              playbackDevice;
#endif

    ////////////////////////////////////////////////////////////
    // Graphics context
    MainOwnedPtr<za::GraphicsContext> graphicsContextStorage;
    za::GraphicsContext&              graphicsContext;

    ////////////////////////////////////////////////////////////
    // Shaders + their uniform locations -- live in `MainShaders`
    MainOwnedPtr<MainShaders> shadersStorage;
    MainShaders&              shaders;

    float shaderTime = 0.f;

    ////////////////////////////////////////////////////////////
    // Context settings
    const unsigned int aaLevel;

    ////////////////////////////////////////////////////////////
    // Game-state storage: profile, game constants, playthroughs
    MainOwnedPtr<MainGameStorage> gameStorage;

    ///////////////////////////////////////////////////////////
    // Profile (stores settings)
    Profile& profile;

    ///////////////////////////////////////////////////////////
    // Game constants (loaded once on startup)
    GameConstants& gameConstants;

    ////////////////////////////////////////////////////////////
    // Zancle fonts
    MainOwnedPtr<za::Font> fontMouldyCheeseStorage;
    za::Font&              fontMouldyCheese;

    ////////////////////////////////////////////////////////////
    // Render window
    [[nodiscard]] za::RenderWindow makeWindow();
    MainOwnedPtr<za::RenderWindow> windowStorage;
    za::RenderWindow&              window;
    float                          dpiScalingFactor = 1.f;

    ////////////////////////////////////////////////////////////
    void refreshWindowAutoBatchModeFromProfile(); // TODO P1: check if this solves flickering

    ////////////////////////////////////////////////////////////
    bool flushAfterEveryBatch  = false; // TODO P1: check if this solves flickering
    bool finishAfterEveryBatch = false; // TODO P1: check if this solves flickering
    bool flushBeforeDisplay    = false; // TODO P1: check if this solves flickering
    bool finishBeforeDisplay   = false; // TODO P1: check if this solves flickering
    bool flushAfterDisplay     = false; // TODO P1: check if this solves flickering
    bool finishAfterDisplay    = false; // TODO P1: check if this solves flickering

    ////////////////////////////////////////////////////////////
    bool loadingGuard;

    ////////////////////////////////////////////////////////////
    // ImGui context
    MainOwnedPtr<za::ImGuiContext> imGuiContextStorage;
    za::ImGuiContext&              imGuiContext;

    ////////////////////////////////////////////////////////////
    // Exiting status
    PlayerInputState playerInputState;
    bool             mustExit{false};

    ////////////////////////////////////////////////////////////
    // Texture atlas
    MainOwnedPtr<za::TextureAtlas> textureAtlasStorage;
    za::TextureAtlas&              textureAtlas; // TODO P0: make smaller

    ////////////////////////////////////////////////////////////
    // Zancle fonts
    MainOwnedPtr<za::Font> fontSuperBakeryStorage;
    za::Font&              fontSuperBakery;

    ////////////////////////////////////////////////////////////
    // ImGui fonts
    ImFont* fontImGuiMouldyCheese;
    ImFont* fontImGuiSuperBakery;
    ImFont* fontImGuiFA;

    ////////////////////////////////////////////////////////////
    // Music
    static inline constexpr const char* bgmPathNormal = "resources/hibiscus.mp3";
    static inline constexpr const char* bgmPathWitch  = "resources/bgmwitch.mp3";
    static inline constexpr const char* bgmPathWizard = "resources/bgmwizard.mp3";
    static inline constexpr const char* bgmPathMouse  = "resources/bgmmouse.mp3";
    static inline constexpr const char* bgmPathEngi   = "resources/bgmengi.mp3";

    const char* lastPlayedMusic = bgmPathNormal; // to avoid restarting the same song

    za::SizeT currentBGMBufferIdx = 0u; // which one of the two buffers is "current"
    Countdown bgmTransition;            // fade in/out timer

    MainOwnedPtr<MainBGMStorage> bgmStorage;
    MainBGMStorage&              bgm;

    ////////////////////////////////////////////////////////////
    // Sound management
    Sounds sounds{/* volumeMult */ 1.f};

    MainOwnedPtr<SoundManager> soundManagerStorage;
    SoundManager&              soundManager;
    MainOwnedPtr<za::Listener> listenerStorage;
    za::Listener&              listener;

    ////////////////////////////////////////////////////////////
    // Delayed actions
    struct DelayedAction
    {
        Countdown                      delayCountdown;
        za::FixedFunction<void(), 128> action;
    };

    za::Vector<DelayedAction> delayedActions;

    ////////////////////////////////////////////////////////////
    // Background and ImGui render textures
    MainOwnedPtr<za::RenderTexture> rtBackgroundStorage;
    za::RenderTexture&              rtBackground;

    MainOwnedPtr<za::RenderTexture> rtBackgroundProcessedStorage;
    za::RenderTexture&              rtBackgroundProcessed;

    MainOwnedPtr<za::RenderTexture> rtImGuiStorage;
    za::RenderTexture&              rtImGui;

    MainOwnedPtr<za::RenderTexture> rtCloudMaskStorage;
    za::RenderTexture&              rtCloudMask;

    MainOwnedPtr<za::RenderTexture> rtCloudProcessedStorage;
    za::RenderTexture&              rtCloudProcessed;

    ////////////////////////////////////////////////////////////
    // Game render texture (before post-processing)
    MainOwnedPtr<za::RenderTexture> rtGameStorage;
    za::RenderTexture&              rtGame;

    ////////////////////////////////////////////////////////////
    // Hexed cat offscreen render textures (one per concurrent hex, for witch and copy-witch combined)
    static inline constexpr za::Vec2u hexedCatRenderTextureSize{640u, 640u};
    static inline constexpr za::SizeT maxHexedCatRenderTextures = maxConcurrentHexes * 2u;

    MainOwnedPtr<MainRenderTextureVector> hexedCatRenderTexturesStorage;
    MainRenderTextureVector&              hexedCatRenderTextures;

    ////////////////////////////////////////////////////////////
    // Textures (not in atlas)
    MainOwnedPtr<MainTextureStorage> textureStorage;

    za::Texture& txLogo;
    za::Texture& txFixedBg;
    za::Texture& txBackgroundChunk;
    za::Texture& txBackgroundChunkDesaturated;
    za::Texture& txClouds;
    za::Texture& txTintedClouds;
    za::Texture& txBgSwamp;
    za::Texture& txBgObservatory;
    za::Texture& txBgAimTraining;
    za::Texture& txBgFactory;
    za::Texture& txBgWindTunnel;
    za::Texture& txBgMagnetosphere;
    za::Texture& txBgAuditorium;
    za::Texture& txDrawings;
    za::Texture& txTipBg;
    za::Texture& txTipByte;
    za::Texture& txCursor;
    za::Texture& txCursorMultipop;
    za::Texture& txCursorLaser;
    za::Texture& txCursorGrab;
    za::Texture& txArrow;
    za::Texture& txUnlock;
    za::Texture& txPurchasable;
    za::Texture& txLetter;
    za::Texture& txLetterText;
    za::Texture& txFrame;
    za::Texture& txFrameTiny;
    za::Texture& txCloudBtn;
    za::Texture& txCloudBtnSmall;
    za::Texture& txCloudBtnSquare;
    za::Texture& txCloudBtnSquare2;

    ////////////////////////////////////////////////////////////
    // UI texture atlas
    MainOwnedPtr<za::TextureAtlas> uiTextureAtlasStorage;
    za::TextureAtlas&              uiTextureAtlas;

    ////////////////////////////////////////////////////////////
    // Atlas rects + cat animation rect arrays -- live in `MainAtlasRects`
    MainOwnedPtr<MainAtlasRects> atlasRectsStorage;
    MainAtlasRects&              atlasRects;

    ////////////////////////////////////////////////////////////
    // Background hues
    static inline constexpr EXACT_ARRAY(
        float,
        backgroundHues,
        nShrineTypes + 1u,
        {
            0.f,    // Normal
            -140.f, // Voodoo
            47.f,   // Magic
            -15.f,  // Clicking
            180.f,  // Automation
            121.f,  // Repulsion
            -45.f,  // Attraction
            -155.f, // Camouflage
            -80.f,  // Victory
        });

    ////////////////////////////////////////////////////////////
    // Playthrough (game state) -- storage lives in `gameStorage`
    Playthrough& ptMain;
    Playthrough& ptSpeedrun;

    ////////////////////////////////////////////////////////////
    // Currently active playthrough (game state)
    Playthrough* pt;

    ////////////////////////////////////////////////////////////
    // Prestige availability tracking
    bool wasPrestigeAvailableLastFrame = false;

    ////////////////////////////////////////////////////////////
    // Buy combo reminder secret achievement
    int buyReminder = 0;

    ////////////////////////////////////////////////////////////
    // Wasted effort ritual secret achievement
    bool wastedEffort = false;

    ////////////////////////////////////////////////////////////
    // Witchcat animation
    float witchcatWobblePhase{0.f};
    float copyWitchcatWobblePhase{0.f};

    ////////////////////////////////////////////////////////////
    // Wizardcat spin
    Countdown wizardcatSpin;
    float     wizardcatAbsorptionRotation{0.f};

    ////////////////////////////////////////////////////////////
    // Copycat state
    Countdown copycatMaskAnimCd;
    Countdown copycatMaskAnim;

    ////////////////////////////////////////////////////////////
    // Frametime-independent astro/portal proc
    Countdown frameProcCd;
    bool      frameProcThisFrame{false};

    ////////////////////////////////////////////////////////////
    // HUD money text
    MainOwnedPtr<MainTextStorage> textStorage;
    za::Text&                     moneyText;
    TextShakeEffect               moneyTextShakeEffect;

    ////////////////////////////////////////////////////////////
    // Combo state
    static inline constexpr za::Vec2f moneyTextInitialPosition{10.f, 70.f};
    MainOwnedPtr<ComboState>          comboStateStorage;
    ComboState&                       comboState;

    ////////////////////////////////////////////////////////////
    // HUD demo text
    za::Text& demoText;

    ////////////////////////////////////////////////////////////
    // Spatial partitioning
    za::UniquePtr<SweepAndPrune> sweepAndPrune;

    ////////////////////////////////////////////////////////////
    // Particles
    za::Vector<Particle>     particles;          // World space
    za::Vector<TextParticle> textParticles;      // World space
    za::Vector<Particle>     spentCoinParticles; // HUD space
    za::Vector<Particle>     hudBottomParticles; // HUD space, drawn below ImGui
    za::Vector<Particle>     hudTopParticles;    // HUD space, drawn on top of ImGui

    struct EarnedCoinParticle
    {
        za::Vec2f startPosition;
        Progress  progress{};
    };

    za::Vector<EarnedCoinParticle> earnedCoinParticles; // HUD space

    ////////////////////////////////////////////////////////////
    // Combo-bubble payout queue. Each entry holds a swarm of coin particles
    // spawned at the popped bubble. They first burst outward physically (with
    // exponential damping, so they settle to a near-stop), then transition
    // into the rising-pitch collection sequence -- when the collection delay
    // ticks, one settled coin is consumed and a regular `EarnedCoinParticle`
    // is spawned at its position to fly to the money text on the HUD.
    struct [[nodiscard]] BurstingComboCoin
    {
        za::Vec2f position{};
        za::Vec2f velocity{};
        bool      collected{false};
    };

    struct [[nodiscard]] PendingComboBubblePayout
    {
        za::Vector<BurstingComboCoin> coins;
        SizeT                         coinsCollected{0u}; // total collected so far (drives pitch)
        Countdown                     settleCountdown{};  // burst → collect transition
        Countdown                     collectDelay{};
    };

    za::Vector<PendingComboBubblePayout> pendingComboBubblePayouts;

    ////////////////////////////////////////////////////////////
    // Random number generation
    RNGSeedType          seed;
    za::FastNonCryptoRng rng{seed};
    za::FastNonCryptoRng rngFast{seed}; // very fast, low-quality, but good enough for VFXs

    ////////////////////////////////////////////////////////////
    // Cat names
    za::Vector<za::Vector<za::StringView>> shuffledCatNamesPerType;

    ////////////////////////////////////////////////////////////
    // Prestige transition
    bool inPrestigeTransition{false};

    ////////////////////////////////////////////////////////////
    // Timers for transitions
    TimedCountdown bubbleSpawnTimer{.duration = 3.f};
    TimedCountdown catRemoveTimer{.duration = 100.f};

    ////////////////////////////////////////////////////////////
    // Clock and accumulator for played time
    za::Clock playedClock;
    za::I64   playedUsAccumulator{0};
    za::I64   autosaveUsAccumulator{0};
    za::I64   fixedBgSlideAccumulator{0}; // for menu background slide
    float     fixedBgSlideTarget = 0.f;
    float     fixedBgSlide       = 0.f;

    ////////////////////////////////////////////////////////////
    // FPS and delta time clocks
    za::Clock fpsClock;
    za::Clock deltaClock;

    ////////////////////////////////////////////////////////////
    // Batches for drawing
    MainOwnedPtr<MainDrawableBatches> drawableBatchesStorage;

    za::CPUDrawableBatch& bubbleDrawableBatch;
    za::CPUDrawableBatch& starBubbleDrawableBatch;
    za::CPUDrawableBatch& bombBubbleDrawableBatch;
    za::CPUDrawableBatch& cpuCloudDrawableBatch;
    za::CPUDrawableBatch& cpuTopCloudDrawableBatch;
    za::CPUDrawableBatch& cpuCloudHudDrawableBatch;
    za::CPUDrawableBatch& cpuCloudUiDrawableBatch;
    za::CPUDrawableBatch& cpuDrawableBatchBeforeCats;
    za::CPUDrawableBatch& cpuDrawableBatch;
    za::CPUDrawableBatch& cpuDrawableBatchAfterCats;
    za::CPUDrawableBatch& cpuDrawableBatchAdditive;
    za::CPUDrawableBatch& minimapDrawableBatch;
    za::CPUDrawableBatch& catTextDrawableBatch;
    za::CPUDrawableBatch& hudDrawableBatch;
    za::CPUDrawableBatch& hudTopDrawableBatch;     // drawn on top of ImGui
    za::CPUDrawableBatch& hudBottomDrawableBatch;  // drawn below ImGui
    za::CPUDrawableBatch& cpuTopDrawableBatch;     // drawn on top of ImGui
    za::CPUDrawableBatch& catTextTopDrawableBatch; // drawn on top of ImGui
    za::CPUDrawableBatch& tempDrawableBatch;       // for misc one-off draws (hexed cat effect)

    struct HexedCatDrawCommand // NOLINT(cppcoreguidelines-pro-type-member-init)
    {
        za::SizeT renderTextureIndex;
        za::Vec2f position;
        float     phaseSeed;
        float     effectStrength;
        bool      top;
    };

    za::Vector<HexedCatDrawCommand> hexedCatDrawCommands;

    ////////////////////////////////////////////////////////////
    void drawBatch(const za::CPUDrawableBatch& batch, const za::RenderStates& states);

    ////////////////////////////////////////////////////////////
    // Screen shake effect state
    float screenShakeAmount{0.f};
    float screenShakeTimer{0.f};

    ////////////////////////////////////////////////////////////
    // Debug-only multiplier applied to per-frame `deltaTimeMs` in the world update.
    // Always 1.0 outside debug mode.
    float debugTimeScale{1.f};

    ////////////////////////////////////////////////////////////
    // Cached culling boundaries
    struct CullingBoundaries
    {
        float left;
        float right;
        float top;
        float bottom;

        ////////////////////////////////////////////////////////////
        [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr bool isInside(const za::Vec2f point) const noexcept
        {
            return (point.x >= left) && (point.x <= right) && (point.y >= top) && (point.y <= bottom);
        }
    };

    CullingBoundaries hudCullingBoundaries{};
    CullingBoundaries particleCullingBoundaries{};
    CullingBoundaries bubbleCullingBoundaries{};

    ////////////////////////////////////////////////////////////
    // Splash screen state
    TimedCountdown splashCountdown{.duration = 1.f}; // TODO P0: revert to 2500

    ////////////////////////////////////////////////////////////
    // Tip state
    za::Optional<TimedCountdown> tipTCByte;
    za::Optional<TimedCountdown> tipTCBackground;
    za::Optional<TimedCountdown> tipTCBytePreEnd;
    za::Optional<TimedCountdown> tipTCByteEnd;
    za::Optional<TimedCountdown> tipTCBackgroundEnd;
    Countdown                    tipCountdownChar;
    za::String                   tipString;
    TextEffectWiggle             tipStringWiggle{0.00175f, 4.f};
    za::SizeT                    tipCharIdx{0u};

    ////////////////////////////////////////////////////////////
    // Text buffers
    za::Text& textNameBuffer;
    za::Text& textStatusBuffer;
    za::Text& textMoneyBuffer;

    ////////////////////////////////////////////////////////////
    // Spent money count-down effect
    MoneyType spentMoney{0u};
    Progress  spentMoneyTimer{};

    ////////////////////////////////////////////////////////////
    // Thread pool
    za::ThreadPool threadPool;

    ////////////////////////////////////////////////////////////
    // Cached views
    MainOwnedPtr<za::View> gameViewStorage;
    za::View&              gameView; // TODO P1: compute on the fly, don't cache...

    MainOwnedPtr<za::View> nonScaledHUDViewStorage;
    za::View&              nonScaledHUDView; // TODO P1: compute on the fly, don't cache...

    MainOwnedPtr<za::View> scaledHUDViewStorage;
    za::View&              scaledHUDView; // TODO P1: compute on the fly, don't cache...

    ////////////////////////////////////////////////////////////
    // $ps sampler
    MoneyType      moneyGainedLastSecond{0u};
    Sampler<float> samplerMoneyPerSecond{/* capacity */ 60u};
    za::I64        moneyGainedUsAccumulator{0};

    ////////////////////////////////////////////////////////////
    // Bomb-cat tracker for money earned
    MainOwnedPtr<MainBombStorage> bombStorage;

    ////////////////////////////////////////////////////////////
    // Notification queue
    MainOwnedPtr<NotificationState> notificationStateStorage;
    NotificationState&              notificationState;

    ////////////////////////////////////////////////////////////
    // FPS counter
    float fps{0.f};

    ////////////////////////////////////////////////////////////
    // UI state
    MainOwnedPtr<UIState> uiStateStorage;
    UIState&              uiState;

    ////////////////////////////////////////////////////////////
    // Portal storm buff countdown
    Countdown portalStormTimer;

#ifdef BUBBLEBYTE_USE_STEAMWORKS
    ////////////////////////////////////////////////////////////
    // Steam manager
    hg::Steam::SteamManager& steamMgr;
#endif

    bool onSteamDeck{false};

    ////////////////////////////////////////////////////////////
    // Background hue changing based on shrine
    za::Angle currentBackgroundHue;
    za::Angle targetBackgroundHue;
    za::Color outlineHueColor{colorBlueOutline};

    ////////////////////////////////////////////////////////////
    // Victory state
    za::Optional<TimedCountdown> victoryTC;
    Countdown                    cdLetterAppear;
    Countdown                    cdLetterText;

    ////////////////////////////////////////////////////////////
    // Input management
    MainOwnedPtr<InputHelper> inputHelperStorage;
    InputHelper&              inputHelper;

    ////////////////////////////////////////////////////////////
    // Logging -- `logFile` is a non-owning view that may be null if
    // the log file failed to open (e.g. read-only filesystem).
    MainOwnedPtr<za::OutFile> logFileStorage;
    za::OutFile*              logFile;

    ////////////////////////////////////////////////////////////
    // Achievement progress tracking
    struct AchievementProgress
    {
        za::SizeT value;
        za::SizeT threshold;
    };

    za::Array<za::Optional<AchievementProgress>, za::getArraySize(achievementData)> achievementProgress{};

    ////////////////////////////////////////////////////////////
    // PP purchase undo
    za::Vector<za::FixedFunction<void(), 128>> undoPPPurchase;
    Countdown                                  undoPPPurchaseTimer;

    ////////////////////////////////////////////////////////////
    void saveMainPlaythroughToFile();

    ////////////////////////////////////////////////////////////
    void log(const char* format, ...); // NOLINT(modernize-avoid-variadic-functions)

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] bool inSpeedrunPlaythrough() const
    {
        return pt == &ptSpeedrun;
    }

    ////////////////////////////////////////////////////////////
    void addMoney(MoneyType reward);

    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Vector<za::Vector<za::StringView>> makeShuffledCatNames(za::FastNonCryptoRng& rng);

    ////////////////////////////////////////////////////////////
    [[nodiscard]] static unsigned int getTPWorkerCount();

    ////////////////////////////////////////////////////////////
    [[nodiscard]] SizeT getNextCatNameIdx(CatType catType);

    ////////////////////////////////////////////////////////////
    Particle& implEmplaceParticle(za::Vec2f    position,
                                  ParticleType particleType,
                                  float        scaleMult,
                                  float        speedMult,
                                  float        opacity = 1.f);

    ////////////////////////////////////////////////////////////
    bool spawnSpentCoinParticle(const ParticleData& particleData);

    ////////////////////////////////////////////////////////////
    void spawnHUDTopParticle(const ParticleData& particleData, float hue, ParticleType particleType);

    ////////////////////////////////////////////////////////////
    void spawnHUDBottomParticle(const ParticleData& particleData, float hue, ParticleType particleType);

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool spawnEarnedCoinParticle(za::Vec2f startPosition);

    ////////////////////////////////////////////////////////////
    void spawnParticle(const ParticleData& particleData, float hue, ParticleType particleType);

    ////////////////////////////////////////////////////////////
    // Definitions in `BubbleIdleMainInline.hpp`
    void spawnParticles(SizeT n, za::Vec2f position, auto... args);
    void spawnParticlesWithHue(float hue, SizeT n, za::Vec2f position, auto... args);
    void spawnParticlesNoGravity(SizeT n, za::Vec2f position, auto... args);
    void spawnParticlesWithHueNoGravity(float hue, SizeT n, za::Vec2f position, auto... args);

    ////////////////////////////////////////////////////////////
    // Definition in `BubbleIdleMainInline.hpp`
    void withAllStats(auto&& func);

    ////////////////////////////////////////////////////////////
    void statBubblePopped(BubbleType bubbleType, bool byHand, MoneyType reward);

    ////////////////////////////////////////////////////////////
    void statExplosionRevenue(MoneyType reward);

    ////////////////////////////////////////////////////////////
    void statFlightRevenue(MoneyType reward);

    ////////////////////////////////////////////////////////////
    void statHellPortalRevenue(MoneyType reward);

    ////////////////////////////////////////////////////////////
    void statSecondsPlayed();

    ////////////////////////////////////////////////////////////
    void statHighestStarBubblePopCombo(za::U64 comboValue);

    ////////////////////////////////////////////////////////////
    void statHighestNovaBubblePopCombo(za::U64 comboValue);

    ////////////////////////////////////////////////////////////
    void statAbsorbedStarBubble();

    ////////////////////////////////////////////////////////////
    void statSpellCast(SizeT spellIndex);

    ////////////////////////////////////////////////////////////
    void statMaintenance(SizeT nCatsHit);

    ////////////////////////////////////////////////////////////
    void statDisguise();

    ////////////////////////////////////////////////////////////
    void statDollCollected();

    ////////////////////////////////////////////////////////////
    void statRitual(CatType catType);

    ////////////////////////////////////////////////////////////
    void statHighestSimultaneousMaintenances(za::U64 value);

    ////////////////////////////////////////////////////////////
    void statHighestDPS(za::U64 value);


    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool keyDown(za::Keyboard::Key key) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool mBtnDown(za::Mouse::Button button, bool penetrateUI) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Rect2f addImgResourceToAtlas(const za::Path& path);

    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Rect2f addImgResourceToUIAtlas(const za::Path& path);

    ////////////////////////////////////////////////////////////
    void playSound(const LoadedSound& ls, za::SizeT maxOverlap = 255u);

    ////////////////////////////////////////////////////////////
    // Definitions in `BubbleIdleMainInline.hpp`
    void                  forEachBubbleInRadiusSquared(za::Vec2f center, float radiusSq, auto&& func);
    void                  forEachBubbleInRadius(za::Vec2f center, float radius, auto&& func);
    [[nodiscard]] Bubble* pickRandomBubbleInRadiusMatching(za::Vec2f center, float radius, auto&& predicate);

    ////////////////////////////////////////////////////////////
    [[nodiscard]] Bubble* pickRandomBubbleInRadius(za::Vec2f center, float radius);

    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Vec2f getResolution() const;

    ////////////////////////////////////////////////////////////
    // Definitions in `BubbleIdleMainInline.hpp`
    [[nodiscard]] float getComputedCooldownByCatTypeOrCopyCat(CatType catType) const;
    [[nodiscard]] float getComputedRangeByCatTypeOrCopyCat(CatType catType) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Vec2f getViewCenter() const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Vec2f getViewCenterWithoutScroll() const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] CullingBoundaries getViewCullingBoundaries(float offset) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Vec2f         getCatRangeCenter(const Cat& cat);
    [[nodiscard]] float                    getWindRepulsionMult() const;
    [[nodiscard]] float                    getWindAttractionMult() const;
    [[nodiscard]] static constexpr CatType shrineTypeToCatType(ShrineType shrineType);
    [[nodiscard]] za::Sprite               particleToSprite(const Particle& particle) const;
    [[nodiscard]] za::Vec2f                getEdgeSpawnPosition(const za::Rect2f& bounds, float thickness);
    [[nodiscard]] za::Vec2u                getNewResolution() const;

    ////////////////////////////////////////////////////////////
    Cat& spawnCat(za::Vec2f pos, CatType catType, float hue);

    ////////////////////////////////////////////////////////////
    Cat& spawnCatCentered(CatType catType, float hue, bool placeInHand = true);

    ////////////////////////////////////////////////////////////
    Cat& spawnSpecialCat(za::Vec2f pos, CatType catType);

    ////////////////////////////////////////////////////////////
    void resetTipState();

    ////////////////////////////////////////////////////////////
    void doTip(const za::String& str, SizeT maxPrestigeLevel = 0u);

    ////////////////////////////////////////////////////////////
    // Definitions in `BubbleIdleMainInline.hpp`
    [[nodiscard]] bool isUnicatTranscendenceActive() const;
    [[nodiscard]] bool isDevilcatHellsingedActive() const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getAspectRatioScalingFactor(za::Vec2f originalSize, za::Vec2f windowSize) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getCappedGameViewAspectRatio(za::Vec2f originalSize, za::Vec2f windowSize) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Vec2f getExpandedGameViewSize(za::Vec2f originalSize, za::Vec2f windowSize) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::View createScaledGameView(za::Vec2f originalSize, za::Vec2f windowSize) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::View createScaledTopGameView(za::Vec2f originalSize, za::Vec2f windowSize) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Vec2f getCurrentGameViewSize() const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] float clampGameViewCenterX(float desiredCenterX, float viewWidth) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::View makeScaledHUDView(za::Vec2f resolution, float scale) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Vec2f getHUDMousePos() const;

    ////////////////////////////////////////////////////////////
    template <za::SizeT BufferIdx = 0u, typename T>
    static const char* toStringWithSeparators(const T value)
    {
        // Thread-local buffer to store the result
        // Size should be 27 (max 20 digits for 64-bit integer + up to 6 separators + null terminator)
        // Using 32 for addinional safety just in case
        static thread_local char strBuffer[32];

        char* const end = strBuffer + sizeof(strBuffer) - 1;

        char* ptr = end;
        *ptr      = '\0';

        // Handle negative numbers
        const bool isNegative = value < 0;
        T          absValue   = isNegative ? -value : value;

        // Handle zero specially
        if (absValue == 0)
        {
            *--ptr = '0';
            return ptr;
        }

        // Convert digits and add separators
        int digitCount = 0;
        while (absValue > 0)
        {
            if (digitCount > 0 && digitCount % 3 == 0)
                *--ptr = '.';

            *--ptr = '0' + static_cast<char>(absValue % 10);
            absValue /= 10;

            ++digitCount;
        }

        if (isNegative)
            *--ptr = '-';

        return ptr;
    }

    ////////////////////////////////////////////////////////////
    static inline constexpr float uiNormalFontScale      = 0.95f;
    static inline constexpr float uiSubBulletFontScale   = 0.75f;
    static inline constexpr float uiToolTipFontScale     = 0.65f;
    static inline constexpr float uiWindowWidth          = 425.f;
    static inline constexpr float uiButtonWidth          = 150.f;
    static inline constexpr float uiTooltipWidth         = uiWindowWidth;
    static inline constexpr float maxGameViewAspectRatio = 21.f / 9.f;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] float uiGetMaxWindowHeight() const;
    void                uiSetFontScale(float scale);
    void                uiMakeButtonLabels(const char* label, const char* xLabelBuffer);
    void                uiPushButtonColors();
    void                uiPopButtonColors();
    void                uiBeginTooltip(float width);
    void                uiEndTooltip();
    void                uiMakeTooltip(bool small = false);
    void                uiMakeShrineOrCatTooltip(za::Vec2f mousePos);

    ////////////////////////////////////////////////////////////
    enum class [[nodiscard]] AnimatedButtonOutcome : za::U8
    {
        None,
        Clicked,
        ClickedWhileDisabled,
    };

    ////////////////////////////////////////////////////////////

    struct TabButtonPalette
    {
        za::Color idle;
        za::Color hovered;
        za::Color active;
    };

    [[nodiscard]] bool drawTabButton(float                   scaleMult,
                                     const char*             label,
                                     bool                    selected,
                                     const TabButtonPalette& palette,
                                     za::Vec2f               size   = {},
                                     bool                    square = false);

    [[nodiscard]] AnimatedButtonOutcome uiAnimatedButton(
        const za::Texture& tx,
        const char*        label,
        za::Vec2f          btnSize,
        float              fontScale,
        float              fontScaleMult,
        float              btnSizeMult  = 1.f,
        bool               forceHovered = false);

    [[nodiscard]] bool uiMakeButtonImpl(const char* label, const char* xBuffer);
    [[nodiscard]] bool makePSVButtonEx(const char* label, PurchasableScalingValue& psv, SizeT times, MoneyType cost);
    bool               makePSVButton(const char* label, PurchasableScalingValue& psv);
    [[nodiscard]] bool uiCheckPurchasability(const char* label, bool disabled);
    [[nodiscard]] bool makePurchasableButtonOneTime(const char* label, MoneyType cost, bool& done);
    [[nodiscard]] bool makePurchasablePPButtonOneTime(const char* label, PrestigePointsType prestigePointsCost, bool& done);
    bool makePrestigePurchasablePPButtonPSV(const char* label, PurchasableScalingValue& psv);

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool makePSVButtonExByCurrency(
        const char*              label,
        PurchasableScalingValue& psv,
        SizeT                    times,
        MoneyType                cost,
        MoneyType&               availability,
        const char*              currencyFmt);

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool makePurchasableButtonOneTimeByCurrency(
        const char* label,
        bool&       done,
        MoneyType   cost,
        MoneyType&  availability,
        const char* currencyFmt);

    ////////////////////////////////////////////////////////////
    void switchToBGM(za::SizeT index, bool force);

    ////////////////////////////////////////////////////////////
    static inline constexpr const char* bgmPaths[] = {
        bgmPathNormal, // Normal
        bgmPathWitch,  // Voodoo
        bgmPathWizard, // Magic
        bgmPathMouse,  // Clicking
        bgmPathEngi,   // Automation
        bgmPathNormal, // Repulsion
        bgmPathNormal, // Attraction
        bgmPathNormal, // Camouflage
        bgmPathNormal, // Victory
    };

    ////////////////////////////////////////////////////////////
    za::Optional<BGMBuffer>& getCurrentBGMBuffer();

    ////////////////////////////////////////////////////////////
    za::Optional<BGMBuffer>& getNextBGMBuffer();

    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getHueByCatType(CatType catType);

    ////////////////////////////////////////////////////////////
    void beginPrestigeTransition(PrestigePointsType ppReward);

    void uiBeginColumns() const;
    void uiCenteredText(const char* str, float offsetX = 0.f, float offsetY = 0.f);
    void uiCenteredTextColored(za::Color color, const char* str, float offsetX = 0.f, float offsetY = 0.f);
    [[nodiscard]] za::Vec2f uiGetWindowPos() const;
    void                    uiDrawExitPopup(float newScalingFactor);
    void                    uiClearLabel();
    void                    uiSetLabel(const char* fmt, ...) BUBBLE_IDLE_PRINTF_FORMAT(2, 3);
    void                    uiSetTooltip(const char* fmt, ...) BUBBLE_IDLE_PRINTF_FORMAT(2, 3);
    void                    uiSetTooltipOnly(const char* fmt, ...) BUBBLE_IDLE_PRINTF_FORMAT(2, 3);

    [[nodiscard]] bool uiMakePrestigeOneTimeButton(
        const char*        buttonLabel,
        PrestigePointsType cost,
        bool&              done,
        const char*        tooltipFmt,
        ...) BUBBLE_IDLE_PRINTF_FORMAT(5, 6);

    template <typename TLabel, typename TCurrent, typename TNext>
    bool uiMakePrestigePsvButtonValue(
        const char*              buttonLabel,
        PurchasableScalingValue& psv,
        const char*              labelFmt,
        TLabel                   labelValue,
        const char*              tooltipFmt,
        TCurrent                 currentValue,
        TNext                    nextValue,
        const char*              maxTooltipFmt)
    {
#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wformat-nonliteral"
    #pragma clang diagnostic ignored "-Wformat-security"
#elif defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wformat-nonliteral"
    #pragma GCC diagnostic ignored "-Wformat-security"
#endif
        if (!psv.isMaxedOut())
            uiSetTooltip(tooltipFmt, currentValue, nextValue);
        else
            uiSetTooltip(maxTooltipFmt);

        uiSetLabel(labelFmt, labelValue);
#if defined(__clang__)
    #pragma clang diagnostic pop
#elif defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif
        return makePrestigePurchasablePPButtonPSV(buttonLabel, psv);
    }

    void                     uiDrawCloudWindowBackground();
    [[nodiscard]] bool       uiDrawQuickbarIconButton(const char* label, bool selected, float scaleMult = 1.f);
    void                     uiDrawQuickbarCopyCat(za::Vec2f quickBarPos, Cat& copyCat);
    void                     uiDrawQuickbarBackgroundSelector(za::Vec2f quickBarPos);
    void                     uiDrawQuickbarBGMSelector(za::Vec2f quickBarPos);
    void                     uiDrawQuickbarQuickSettings(za::Vec2f quickBarPos);
    void                     uiDrawQuickbarVolumeControls(za::Vec2f quickBarPos);
    void                     uiDrawQuickbar();
    void                     uiDrawMinimapZoomButtons();
    void                     uiDrawDebugWindow();
    void                     uiDraw(za::Vec2f mousePos);
    void                     uiDpsMeter();
    void                     uiSpeedrunning();
    void                     uiTabBar();
    void                     uiSetUnlockLabelY(za::SizeT unlockId);
    [[nodiscard]] bool       checkUiUnlock(za::SizeT unlockId, bool unlockCondition);
    void                     uiImageFromAtlas(const za::Rect2f& txr, const za::DrawTextureSettings& drawParams);
    void                     uiImgsep(const za::Rect2f& txr, const char* sepLabel, bool first = false);
    void                     uiImgsep2(const za::Rect2f& txr, const char* sepLabel);
    void                     uiTabBarShop();
    void                     uiShopDrawCoreUpgrades();
    void                     uiShopDrawSpecialCats();
    void                     uiShopDrawUniqueCatBonuses();
    [[nodiscard]] za::String uiShopBuildNextGoalsText();
    void                     uiShopCooldownButton(const char* label, CatType catType, const char* additionalInfo = "");
    void                     uiShopRangeButton(const char* label, CatType catType, const char* additionalInfo = "");
    bool                     uiCheckbox(const char* label, bool* b);
    bool                     uiRadio(const char* label, int* i, int value);
    void                     uiTabBarPrestige();
    void                     uiPrestigeDrawOverview();
    void                     uiPrestigeDrawCoreUpgrades();
    void                     uiPrestigeUnsealButton(PrestigePointsType ppCost, const char* catName, CatType type);
    void                     uiPrestigeDrawShrineCatUpgrades();
    void                     uiTabBarMagic();
    void                     uiTabBarStats();
    void                     uiTabBarSettings();
    void                     uiSettingsDrawAudioTab();
    void                     uiSettingsDrawUiTab();
    void                     uiSettingsDrawGraphicsTab();
    void                     uiSettingsDrawDisplayTab();
    void                     uiSettingsDrawDataTab();
    void                     uiSettingsDrawDebugTab();

    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Optional<za::Rect2f> getAoEDragRect(za::Vec2f mousePos) const;

    ////////////////////////////////////////////////////////////
    void resetAllDraggedCats();

    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::SizeT pickDragPivotCatIndex() const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isCatBeingDragged(const Cat& cat) const;

    ////////////////////////////////////////////////////////////
    void stopDraggingCat(const Cat& cat);

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isWizardBusy() const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] Cat* findFirstCatByType(CatType catType) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] Cat* getWitchCat() const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] Cat* getWizardCat() const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] Cat* getMouseCat() const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] Cat* getEngiCat() const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] Cat* getRepulsoCat() const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] Cat* getAttractoCat() const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] Cat* getCopyCat() const;

    ////////////////////////////////////////////////////////////
    void addCombo(int& xCombo, Countdown& xComboCountdown) const;

    ////////////////////////////////////////////////////////////
    static bool checkComboEnd(float deltaTimeMs, int& xCombo, Countdown& xComboCountdown);

    ////////////////////////////////////////////////////////////
    void turnBubbleInto(Bubble& bubble, BubbleType newType);

    ////////////////////////////////////////////////////////////
    void doWizardSpellStarpawConversion(Cat& wizardCat);

    ////////////////////////////////////////////////////////////
    void doWizardSpellMewltiplierAura(Cat& wizardCat);

    ////////////////////////////////////////////////////////////
    void doWizardSpellDarkUnion(Cat& wizardCat);

    ////////////////////////////////////////////////////////////
    void doWizardSpellStasisField(Cat& wizardCat);

    ////////////////////////////////////////////////////////////
    static inline constexpr ManaType spellManaCostByIndex[4] = {5u, 20u, 30u, 40u};

    ////////////////////////////////////////////////////////////
    void castSpellByIndex(za::SizeT index, Cat* wizardCat, Cat* copyCat);

    ////////////////////////////////////////////////////////////
    struct FormatTimeResult
    {
        za::U64 h;
        za::U64 m;
        za::U64 s;
    };

    [[nodiscard]] static constexpr FormatTimeResult formatTime(const za::U64 seconds)
    {
        return {seconds / 3600u, (seconds / 60u) % 60u, seconds % 60u};
    }

    ////////////////////////////////////////////////////////////
    struct FormatSpeedrunTimeResult
    {
        za::U64 hours;
        za::U64 mins;
        za::U64 secs;
        za::U64 millis;
    };

    [[nodiscard]] static constexpr FormatSpeedrunTimeResult formatSpeedrunTime(const za::Time time)
    {
        const za::I64 elapsedTime       = time.asMicroseconds();
        const za::U64 totalMicroseconds = (elapsedTime >= 0) ? static_cast<za::U64>(elapsedTime) : 0ULL;

        constexpr za::U64 usPerMs   = 1000ULL;
        constexpr za::U64 usPerSec  = 1000ULL * usPerMs; // 1,000,000
        constexpr za::U64 usPerMin  = 60ULL * usPerSec;  // 60,000,000
        constexpr za::U64 usPerHour = 60ULL * usPerMin;  // 3,600,000,000

        return {totalMicroseconds / usPerHour,
                (totalMicroseconds % usPerHour) / usPerMin,
                (totalMicroseconds % usPerMin) / usPerSec,
                (totalMicroseconds % usPerSec) / usPerMs};
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool mustApplyMewltiplierAura(za::Vec2f bubblePosition) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] MoneyType computeFinalReward(const Bubble& bubble, float multiplier, float comboMult, const Cat* popperCat) const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Vec2u getReasonableWindowSize(float scalingFactorMult = 1.f);

    ////////////////////////////////////////////////////////////
    struct SelectorEntry
    {
        int         index;
        const char* name;
    };

    ////////////////////////////////////////////////////////////
    [[nodiscard]] int pickSelectedIndex(const za::Vector<SelectorEntry>& entries, int selectedIndex);

    ////////////////////////////////////////////////////////////
    void selectBackground(const za::Vector<SelectorEntry>& entries, int selectedIndex);

    ////////////////////////////////////////////////////////////
    void selectBGM(const za::Vector<SelectorEntry>& entries, int selectedIndex);

    ////////////////////////////////////////////////////////////
    void updateSelectedBackgroundSelectorIndex() const;

    ////////////////////////////////////////////////////////////
    void updateSelectedBGMSelectorIndex() const;

    ////////////////////////////////////////////////////////////
    struct SelectorData
    {
        za::Vector<SelectorEntry> entries;
        int                       selectedIndex = -1;
    };

    ////////////////////////////////////////////////////////////
    SelectorData& getBGMSelectorData() const;

    ////////////////////////////////////////////////////////////
    SelectorData& getBackgroundSelectorData() const;

    ////////////////////////////////////////////////////////////
    void reseedRNGs(RNGSeedType newSeed);

    ////////////////////////////////////////////////////////////
    void forceResetGame(bool goToShopTab = true);

    ////////////////////////////////////////////////////////////
    void forceResetProfile();

    ////////////////////////////////////////////////////////////
    TextParticle& makeRewardTextParticle(za::Vec2f position);

    ////////////////////////////////////////////////////////////
    void shrineCollectReward(Shrine& shrine, MoneyType reward, const Bubble& bubble);

    ////////////////////////////////////////////////////////////
    void doExplosion(Bubble& bubble);

    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Vec2f fromWorldToHud(za::Vec2f point) const;

    ////////////////////////////////////////////////////////////
    struct [[nodiscard]] BubblePopData
    {
        MoneyType reward;
        Bubble&   bubble;
        int       xCombo;
        bool      popSoundOverlap;
        Cat*      popperCat;
        bool      multiPop;
    };

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isBubbleInStasisField(const Bubble& bubble) const;

    ////////////////////////////////////////////////////////////
    void popWithRewardAndReplaceBubble(const BubblePopData& data);

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isDebugModeEnabled() const;

    ////////////////////////////////////////////////////////////
    void gameLoopCheats() const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Mouse::Button getLMB() const;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Mouse::Button getRMB() const;
    [[nodiscard]] bool              gameLoopHandleEvents(FrameInput& frameInput, bool shouldDrawUI);
    void                            gameLoopPrepareInput(FrameInput& frameInput, float deltaTimeMs);
    void gameLoopUpdateFrameWorld(float deltaTimeMs, FrameInput& frameInput, FrameUpdateState& frameUpdate);
    void gameLoopUpdateFrameUi(za::Time deltaTime, float deltaTimeMs, const FrameInput& frameInput);
    [[nodiscard]] FrameViewState gameLoopComputeViews();
    void                         gameLoopRenderFrame(float                   deltaTimeMs,
                                                     bool                    shouldDrawUI,
                                                     za::U8                  shouldDrawUIAlpha,
                                                     const FrameInput&       frameInput,
                                                     const FrameUpdateState& frameUpdate,
                                                     const FrameViewState&   frameViews);
    void                         gameLoopPresentFrame(const FrameViewState& frameViews);
    void                         gameLoopUpdateScrolling(float deltaTimeMs, const za::Vector<za::Vec2f>& downFingers);
    void                         gameLoopUpdateTransitions(float deltaTimeMs);
    void                         gameLoopUpdateBubbles(float deltaTimeMs);
    void                         gameLoopUpdateAttractoBuff(float deltaTimeMs) const;
    [[nodiscard]] bool           gameLoopUpdateBubbleClick(za::Optional<za::Vec2f>& clickPosition);
    void                         gameLoopUpdateCatActionNormal(float /* deltaTimeMs */, Cat& cat);
    void                         gameLoopUpdateCatActionUni(float /* deltaTimeMs */, Cat& cat);
    void                         gameLoopUpdateCatActionDevil(float /* deltaTimeMs */, Cat& cat);
    void                         gameLoopUpdateCatActionAstro(float /* deltaTimeMs */, Cat& cat);
    void                         gameLoopUpdateCatActionWarden(float /* deltaTimeMs */, Cat& cat);

    // Resolve a queued wardencat windup: deals the bonk to
    // `cat.pawBonkPendingTargetIdx` (if still valid + napping), kicks the
    // travel/hold animation, and clears the pending state. Called from the
    // per-frame paw update when the windup countdown reaches zero.
    void               resolveWardenBonkStrike(Cat& cat);
    [[nodiscard]] Cat* getSessionTargetCat(const HexSession& session) const;
    [[nodiscard]] bool anyCatHexedOrCopyHexed() const;
    [[nodiscard]] bool canHexMore() const;
    [[nodiscard]] bool canCopyHexMore() const;
    void               hexCat(Cat& cat, SizeT catIdx, bool copy);
    void gameLoopUpdateCatActionWitchImpl(float /* deltaTimeMs */, Cat& cat, za::Vector<HexSession>& sessionsToUse, SizeT nCatsToHex);
    void               gameLoopUpdateCatActionWitch(float deltaTimeMs, Cat& cat);
    void               gameLoopUpdateCatActionWizard(float deltaTimeMs, Cat& cat);
    void               gameLoopUpdateCatActionMouse(float /* deltaTimeMs */, Cat& cat);
    void               gameLoopUpdateCatActionEngi(float /* deltaTimeMs */, Cat& cat);
    void               gameLoopUpdateCatActionRepulso(float /* deltaTimeMs */, Cat& cat);
    void               gameLoopUpdateCatActionAttracto(float /* deltaTimeMs */, Cat& cat);
    void               gameLoopUpdateCatActionCopy(float deltaTimeMs, Cat& cat);
    void               gameLoopUpdateCatActionDuck(float deltaTimeMs, Cat& cat);
    [[nodiscard]] auto makeMagnetAction(
        za::Vec2f          position,
        CatType            catType,
        float              deltaTimeMs,
        auto               countdownPm,
        float              countdownTime,
        float              strengthMult,
        float              direction,
        BubbleIgnoreFlags& ignoreFlags);

    ////////////////////////////////////////////////////////////
    void gameLoopUpdateCatActions(float deltaTimeMs);

    [[nodiscard]] bool isCatDraggable(const Cat& cat) const;
    [[nodiscard]] bool isAOESelecting() const;

    void gameLoopUpdateCatDragging(float deltaTimeMs, SizeT countFingersDown, za::Vec2f mousePos);
    void gameLoopUpdateShrines(float deltaTimeMs);

    void collectDollImpl(Doll& d, HexSession& session, bool copy);
    void collectDoll(Doll& d, HexSession& session);
    void collectCopyDoll(Doll& d, HexSession& session);

    void addEventBubblefall(float regionCenterX);
    void addEventInvincibleBubble();

    void gameLoopUpdateDollsImpl(float deltaTimeMs, za::Vec2f mousePos, za::Vector<HexSession>& sessionsToUse, bool copy);
    void gameLoopUpdateDolls(float deltaTimeMs, za::Vec2f mousePos);
    void gameLoopUpdateCopyDolls(float deltaTimeMs, za::Vec2f mousePos);
    void gameLoopUpdateHellPortals(float deltaTimeMs);
    void gameLoopUpdateWitchBuffs(float deltaTimeMs);
    void gameLoopUpdateEvents(float deltaTimeMs);
    void gameLoopReapEphemeralBubbles();
    void gameLoopDrawEvents();

    [[nodiscard]] bool canCatNap(const Cat& cat) const;
    void               beginCatNap(Cat& cat, float sleepDurationMs);
    void               gameLoopUpdateNapScheduler(float deltaTimeMs);

    // Apply the Power Nap cooldown-reduction boost to a cat being forcibly
    // woken (shake or wardencat bonk). No-op when the upgrade isn't owned.
    void applyPowerNapBoost(Cat& cat);

    void popComboBubble(Bubble& bubble);
    void gameLoopUpdateComboBubblePayouts(float deltaTimeMs);
    void gameLoopDrawComboBubbleBurstingCoins();
    void gameLoopUpdateMana(float deltaTimeMs);
    void gameLoopUpdateAutocast();
    void pushNotification(const char* title, const char* format, ...) BUBBLE_IDLE_PRINTF_FORMAT(3, 4);

    ////////////////////////////////////////////////////////////
    void gameLoopUpdateMilestones();
    void gameLoopUpdateSplits();
    void gameLoopUpdateAchievements();
    void gameLoopDrawBubbles();
    void gameLoopDisplayBubblesWithoutShader();
    void gameLoopDrawCursorTrail(za::Vec2f mousePos);
    void gameLoopDrawMinimapIcons();
    void gameLoopDisplayBubblesWithShader();
    void gameLoopDrawCats(za::Vec2f mousePos, float deltaTimeMs);

    [[nodiscard]] bool isCatPerformingRitual(Cat& witch, Cat& cat) const;

    void gameLoopDrawCat(Cat&      cat,
                         float     deltaTimeMs,
                         za::Vec2f mousePos,
                         const za::Rect2f* const (&catTxrsByType)[nCatTypes],
                         const za::Rect2f* const (&catPawTxrsByType)[nCatTypes],
                         const za::Rect2f* const (&catTailTxrsByType)[nCatTypes]);

    void                gameLoopDrawShrines(za::Vec2f mousePos);
    void                gameLoopDrawDolls(za::Vec2f mousePos);
    void                gameLoopDrawHellPortals();
    void                gameLoopDrawParticles();
    void                gameLoopDrawHUDParticles();
    void                gameLoopDrawEarnedCoinParticles();
    void                gameLoopDrawHUDTopParticles();
    void                gameLoopDrawHUDBottomParticles();
    void                gameLoopDrawTextParticles();
    [[nodiscard]] float getLeftMostUsefulX() const;
    void                gameLoopDrawScrollArrowHint(float deltaTimeMs);
    void                gameLoopDrawImGui(za::U8 shouldDrawUIAlpha);
    void                gameLoopUpdatePurchaseUnlockedEffects(float deltaTimeMs);
    [[nodiscard]] bool  shouldDrawGrabbingCursor() const;
    void                gameLoopDrawCursor(float deltaTimeMs, float cursorGrow);
    void                gameLoopDrawCursorComboText(float deltaTimeMs, float cursorGrow);
    void                gameLoopDrawCursorComboBar();
    void drawMinimap(bool back, za::RenderTarget& rt, const za::View& hudView, za::Vec2f resolution, za::U8 shouldDrawUIAlpha);
    void drawSplashScreen(za::RenderTarget& rt, const za::View& view, za::Vec2f resolution, float hudScale) const;
    [[nodiscard]] za::Rect2f getViewportPixelBounds(const za::View& view, za::Vec2f targetSize) const;
    void                     gameLoopDrawDollParticleBorder(float hueMod);
    void                     gameLoopTips(float deltaTimeMs);
    void                     recreateImGuiRenderTexture(za::Vec2u newResolution);
    void                     recreateBackgroundRenderTexture(za::Vec2u newResolution);
    void                     recreateGameRenderTexture(za::Vec2u newResolution);
    void setPostProcessUniforms(float vibrance, float saturation, float lightness, float sharpness, float blur) const;
    void updateProcessedBackground();
    void drawActivatedShrineBackgroundEffects(za::RenderTarget& rt,
                                              const za::View&   backgroundView,
                                              za::Vec2f         activeGameViewCenter) const;
    [[nodiscard]] za::RenderTexture& getHexedCatRenderTexture(za::SizeT index);
    void enqueueHexedCatDrawCommand(const za::CPUDrawableBatch& batch, za::Vec2f position, bool top, float phaseSeed, float effectStrength);
    void                drawHexedCatDrawCommands(const za::View& view, bool top);
    void                gameLoopDisplayCloudBatch(const za::CPUDrawableBatch& batch, const za::View& view);
    void                recreateWindow();
    void                resizeWindow();
    [[nodiscard]] float gameLoopUpdateCursorGrowthEffect(float deltaTimeMs, bool anyBubblePoppedByClicking);
    void                gameLoopUpdateCombo(float                   deltaTimeMs,
                                            bool                    anyBubblePoppedByClicking,
                                            za::Vec2f               mousePos,
                                            za::Optional<za::Vec2f> clickPosition);
    void                gameLoopUpdateCollisionsBubbleBubble(float deltaTimeMs);
    void                gameLoopUpdateCollisionsCatCat(float deltaTimeMs);
    void                gameLoopUpdateCollisionsCatShrine(float deltaTimeMs) const;
    void                gameLoopUpdateCollisionsCatDoll();
    void                gameLoopUpdateCollisionsBubbleHellPortal();
    void                gameLoopUpdateScreenShake(float deltaTimeMs);
    void                gameLoopUpdateParticlesAndTextParticles(float deltaTimeMs);
    void                gameLoopUpdateSounds(float deltaTimeMs, za::Vec2f mousePos);
    void                gameLoopUpdateTimePlayed(za::I64 elapsedUs);
    void                gameLoopUpdateAutosave(za::I64 elapsedUs);
    void                gameLoopUpdateAndDrawFixedMenuBackground(float deltaTimeMs, za::I64 elapsedUs);
    void                gameLoopUpdateAndDrawBackground(float deltaTimeMs, const za::View& gameBackgroundView);
    void                gameLoopUpdateMoneyText(float deltaTimeMs, float yBelowMinimap);
    void                gameLoopUpdateSpentMoneyEffect(float deltaTimeMs);
    za::TextData        gameLoopUpdateComboText(float deltaTimeMs, float yBelowMinimap);
    za::TextData        gameLoopUpdateBuffText(const za::Rect2f& comboBounds);
    void                gameLoopPrestigeAvailableReminder();
    void                gameLoopReminderBuyCombo();
    void                gameLoopReminderSpendPPs();
    void                gameLoopUpdateDpsSampler(za::I64 elapsedUs);

    void drawCloudFrame(const CloudFrameDrawSettings& settings);

    ////////////////////////////////////////////////////////////
    void               gameLoopUpdateNotificationQueue(float deltaTimeMs);
    [[nodiscard]] bool gameLoop();
    void               loadPlaythroughFromFileAndReseed();
#ifdef BUBBLEBYTE_USE_STEAMWORKS
    Main(hg::Steam::SteamManager& xSteamMgr);
#else
    Main();
#endif
    ~Main();

    void run();
};

#undef BUBBLE_IDLE_PRINTF_FORMAT
