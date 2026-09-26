

#include "BGMStorage.hpp"
#include "BubbleIdleApp.hpp"
#include "BubbleIdleMain.hpp"
#include "ComboState.hpp"
#include "Constants.hpp"
#include "GameConstants.hpp"
#include "IconsFontAwesome6.h"
#include "InputHelper.hpp"
#include "MainAtlasRects.hpp"
#include "MainBombStorage.hpp"
#include "MainGameStorage.hpp"
#include "MainShaders.hpp"
#include "NotificationState.hpp"
#include "Playthrough.hpp"
#include "Profile.hpp"
#include "RNGSeedType.hpp"
#include "Serialization.hpp"
#include "Steam.hpp"
#include "SweepAndPrune.hpp"
#include "UIState.hpp"

#include "ExampleUtils/SoundManager.hpp"

#include "Zancle/Audio/AudioContext.hpp"
#include "Zancle/Audio/Listener.hpp"
#include "Zancle/Audio/PlaybackDevice.hpp"

#include "Zancle/Graphics/Color.hpp"
#include "Zancle/Graphics/DrawableBatch.hpp"
#include "Zancle/Graphics/Font.hpp"
#include "Zancle/Graphics/GraphicsContext.hpp"
#include "Zancle/Graphics/Priv/ShaderBase.hpp"
#include "Zancle/Graphics/RenderTexture.hpp"
#include "Zancle/Graphics/RenderWindow.hpp"
#include "Zancle/Graphics/Text.hpp"
#include "Zancle/Graphics/TextData.hpp"
#include "Zancle/Graphics/TextUtils.hpp"
#include "Zancle/Graphics/Texture.hpp"
#include "Zancle/Graphics/TextureAtlas.hpp"
#include "Zancle/Graphics/TextureWrapMode.hpp"
#include "Zancle/Graphics/View.hpp"

#include "Zancle/ImGui/ImGuiContext.hpp"
#include "Zancle/ImGui/IncludeImGui.hpp"

#include "Zancle/Window/VideoMode.hpp"
#include "Zancle/Window/VideoModeUtils.hpp"

#include "Zancle/Fmt/Fmt.hpp"
#include "Zancle/Fmt/FmtNumeric.hpp"

#include "Zancle/IO/IO.hpp"
#include "Zancle/IO/Path.hpp"

#include "Zancle/Chrono/Clock.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Vocabulary/UniquePtr.hpp"

#include "Zancle/Math/MinMax.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/Macros.hpp"
#include "Zancle/Base/SizeT.hpp"


namespace
{
constexpr za::TextureLoadSettings bgSettings{.smooth = true, .wrapMode = za::TextureWrapMode::Repeat};
} // namespace


////////////////////////////////////////////////////////////
struct MainDrawableBatches
{
    za::CPUDrawableBatch bubbleDrawableBatch;
    za::CPUDrawableBatch starBubbleDrawableBatch;
    za::CPUDrawableBatch bombBubbleDrawableBatch;
    za::CPUDrawableBatch cpuCloudDrawableBatch;
    za::CPUDrawableBatch cpuTopCloudDrawableBatch;
    za::CPUDrawableBatch cpuCloudHudDrawableBatch;
    za::CPUDrawableBatch cpuCloudUiDrawableBatch;
    za::CPUDrawableBatch cpuDrawableBatchBeforeCats;
    za::CPUDrawableBatch cpuDrawableBatch;
    za::CPUDrawableBatch cpuDrawableBatchAfterCats;
    za::CPUDrawableBatch cpuDrawableBatchAdditive;
    za::CPUDrawableBatch minimapDrawableBatch;
    za::CPUDrawableBatch catTextDrawableBatch;
    za::CPUDrawableBatch hudDrawableBatch;
    za::CPUDrawableBatch hudTopDrawableBatch;
    za::CPUDrawableBatch hudBottomDrawableBatch;
    za::CPUDrawableBatch cpuTopDrawableBatch;
    za::CPUDrawableBatch catTextTopDrawableBatch;
    za::CPUDrawableBatch tempDrawableBatch;
};


////////////////////////////////////////////////////////////
struct MainTextureStorage
{
    za::Texture txLogo;
    za::Texture txFixedBg;
    za::Texture txBackgroundChunk;
    za::Texture txBackgroundChunkDesaturated;
    za::Texture txClouds;
    za::Texture txTintedClouds;
    za::Texture txBgSwamp;
    za::Texture txBgObservatory;
    za::Texture txBgAimTraining;
    za::Texture txBgFactory;
    za::Texture txBgWindTunnel;
    za::Texture txBgMagnetosphere;
    za::Texture txBgAuditorium;
    za::Texture txDrawings;
    za::Texture txTipBg;
    za::Texture txTipByte;
    za::Texture txCursor;
    za::Texture txCursorMultipop;
    za::Texture txCursorLaser;
    za::Texture txCursorGrab;
    za::Texture txArrow;
    za::Texture txUnlock;
    za::Texture txPurchasable;
    za::Texture txLetter;
    za::Texture txLetterText;
    za::Texture txFrame;
    za::Texture txFrameTiny;
    za::Texture txCloudBtn;
    za::Texture txCloudBtnSmall;
    za::Texture txCloudBtnSquare;
    za::Texture txCloudBtnSquare2;

    MainTextureStorage() :
        txLogo(za::Texture::loadFromFile("resources/logo.png", {.smooth = true}).value()),
        txFixedBg(za::Texture::loadFromFile("resources/fixedbg.png",
                                            {.smooth = true, .wrapMode = za::TextureWrapMode::MirroredRepeat})
                      .value()),
        txBackgroundChunk(za::Texture::loadFromFile("resources/bgtest.png", bgSettings).value()),
        txBackgroundChunkDesaturated(za::Texture::loadFromFile("resources/bgtestdesaturated.png", bgSettings).value()),
        txClouds(za::Texture::loadFromFile("resources/clouds.png", bgSettings).value()),
        txTintedClouds(za::Texture::loadFromFile("resources/tintedclouds.png", bgSettings).value()),
        txBgSwamp(za::Texture::loadFromFile("resources/bgswamp.png", bgSettings).value()),
        txBgObservatory(za::Texture::loadFromFile("resources/bgobservatory.png", bgSettings).value()),
        txBgAimTraining(za::Texture::loadFromFile("resources/bgaimtraining.png", bgSettings).value()),
        txBgFactory(za::Texture::loadFromFile("resources/bgfactory.png", bgSettings).value()),
        txBgWindTunnel(za::Texture::loadFromFile("resources/bgwindtunnel.png", bgSettings).value()),
        txBgMagnetosphere(za::Texture::loadFromFile("resources/bgmagnetosphere.png", bgSettings).value()),
        txBgAuditorium(za::Texture::loadFromFile("resources/bgauditorium.png", bgSettings).value()),
        txDrawings(za::Texture::loadFromFile("resources/drawings.png", {.smooth = true}).value()),
        txTipBg(za::Texture::loadFromFile("resources/tipbg.png", {.smooth = true}).value()),
        txTipByte(za::Texture::loadFromFile("resources/tipbyte.png", {.smooth = true}).value()),
        txCursor(za::Texture::loadFromFile("resources/cursor.png", {.smooth = true}).value()),
        txCursorMultipop(za::Texture::loadFromFile("resources/cursormultipop.png", {.smooth = true}).value()),
        txCursorLaser(za::Texture::loadFromFile("resources/cursorlaser.png", {.smooth = true}).value()),
        txCursorGrab(za::Texture::loadFromFile("resources/cursorgrab.png", {.smooth = true}).value()),
        txArrow(za::Texture::loadFromFile("resources/arrow.png", {.smooth = true}).value()),
        txUnlock(za::Texture::loadFromFile("resources/unlock.png", {.smooth = true}).value()),
        txPurchasable(za::Texture::loadFromFile("resources/purchasable.png", {.smooth = true}).value()),
        txLetter(za::Texture::loadFromFile("resources/letter.png", {.smooth = true}).value()),
        txLetterText(za::Texture::loadFromFile("resources/lettertext.png", {.smooth = true}).value()),
        txFrame(za::Texture::loadFromFile("resources/frame.png", {.smooth = true}).value()),
        txFrameTiny(za::Texture::loadFromFile("resources/frametiny.png", {.smooth = true}).value()),
        txCloudBtn(za::Texture::loadFromFile("resources/cloudbtn.png", {.smooth = true}).value()),
        txCloudBtnSmall(za::Texture::loadFromFile("resources/cloudbtnsmall.png", {.smooth = true}).value()),
        txCloudBtnSquare(za::Texture::loadFromFile("resources/cloudbtnsquare.png", {.smooth = true}).value()),
        txCloudBtnSquare2(za::Texture::loadFromFile("resources/cloudbtnsquare2.png", {.smooth = true}).value())
    {
    }
};


////////////////////////////////////////////////////////////
struct MainTextStorage
{
    za::Text moneyText;
    za::Text demoText;
    za::Text textNameBuffer;
    za::Text textStatusBuffer;
    za::Text textMoneyBuffer;

    explicit MainTextStorage(const za::Font& fontSuperBakery) :
        moneyText(fontSuperBakery,
                  {.position         = Main::moneyTextInitialPosition,
                   .string           = "$0",
                   .characterSize    = 64u,
                   .fillColor        = za::Color::White,
                   .outlineColor     = colorBlueOutline,
                   .outlineThickness = 4.f}),
        demoText(fontSuperBakery,
                 {.position         = {},
                  .string           = "DEMO VERSION",
                  .characterSize    = 48u,
                  .fillColor        = za::Color::White,
                  .outlineColor     = colorBlueOutline,
                  .outlineThickness = 3.f}),
        textNameBuffer(fontSuperBakery,
                       {
                           .string           = "",
                           .characterSize    = 48u,
                           .fillColor        = za::Color::White,
                           .outlineColor     = colorBlueOutline,
                           .outlineThickness = 3.f,
                       }),
        textStatusBuffer(fontSuperBakery,
                         {
                             .string           = "",
                             .characterSize    = 32u,
                             .fillColor        = za::Color::White,
                             .outlineColor     = colorBlueOutline,
                             .outlineThickness = 2.f,
                         }),
        textMoneyBuffer(fontSuperBakery,
                        {
                            .string           = "",
                            .characterSize    = 24u,
                            .fillColor        = za::Color::White,
                            .outlineColor     = colorBlueOutline,
                            .outlineThickness = 1.5f,
                        })
    {
    }
};


////////////////////////////////////////////////////////////
template <typename T>
void MainOwnedDeleter<T>::operator()(T* ptr) noexcept
{
    delete ptr;
}


////////////////////////////////////////////////////////////
template struct MainOwnedDeleter<za::RenderTexture>;
template struct MainOwnedDeleter<za::AudioContext>;
template struct MainOwnedDeleter<za::TextureAtlas>;
template struct MainOwnedDeleter<za::RenderWindow>;
template struct MainOwnedDeleter<za::Font>;
template struct MainOwnedDeleter<za::GraphicsContext>;
template struct MainOwnedDeleter<za::ImGuiContext>;
template struct MainOwnedDeleter<za::Listener>;
template struct MainOwnedDeleter<za::OutFile>;
template struct MainOwnedDeleter<za::PlaybackDevice>;
template struct MainOwnedDeleter<za::View>;
template struct MainOwnedDeleter<ComboState>;
template struct MainOwnedDeleter<InputHelper>;
template struct MainOwnedDeleter<MainBombStorage>;
template struct MainOwnedDeleter<MainDrawableBatches>;
template struct MainOwnedDeleter<MainAtlasRects>;
template struct MainOwnedDeleter<MainBGMStorage>;
template struct MainOwnedDeleter<MainGameStorage>;
template struct MainOwnedDeleter<MainShaders>;
template struct MainOwnedDeleter<MainTextStorage>;
template struct MainOwnedDeleter<MainTextureStorage>;
template struct MainOwnedDeleter<MainRenderTextureVector>;
template struct MainOwnedDeleter<NotificationState>;
template struct MainOwnedDeleter<UIState>;


////////////////////////////////////////////////////////////
bool debugMode = false;

////////////////////////////////////////////////////////////
void runBubbleIdleApp()
{
#ifdef BUBBLEBYTE_USE_STEAMWORKS
    hg::Steam::SteamManager steamMgr;
    steamMgr.requestStatsAndAchievements();
    steamMgr.runCallbacks();

    // Using a heap-allocation here because `Main` exceeds the stack size
    za::makeUnique<Main>(steamMgr)->run();
#else
    za::makeUnique<Main>()->run();
#endif
}


////////////////////////////////////////////////////////////
#ifdef BUBBLEBYTE_USE_STEAMWORKS
Main::Main(hg::Steam::SteamManager& xSteamMgr) :
#else
Main::Main() :
#endif
#ifndef BUBBLEBYTE_NO_AUDIO
    audioContextStorage(new za::AudioContext{za::AudioContext::create().value()}),
    audioContext(*audioContextStorage),
    playbackDeviceStorage(new za::PlaybackDevice{za::AudioContext::getDefaultPlaybackDeviceHandle().value()}),
    playbackDevice(*playbackDeviceStorage),
#endif
    graphicsContextStorage(new za::GraphicsContext{za::GraphicsContext::create().value()}),
    graphicsContext(*graphicsContextStorage),
    shadersStorage(
        []
{
    const auto loadShader = [](const za::Shader::LoadFromFileSettings& settings)
    {
        auto s = za::Shader::loadFromFile(settings).value();
        s.setUniform(s.getUniformLocation("za_u_texture").value(), za::Shader::CurrentTexture);
        return s;
    };

    const auto bind = [](const za::Shader& s, const char* name) { return s.getUniformLocation(name).value(); };

    auto shader = loadShader({.vertexPath = "resources/shader.vert", .fragmentPath = "resources/shader.frag"});

    const auto suBackgroundTexture   = bind(shader, "u_backgroundTexture");
    const auto suTime                = bind(shader, "u_time");
    const auto suResolution          = bind(shader, "u_resolution");
    const auto suBackgroundOrigin    = bind(shader, "u_backgroundOrigin");
    const auto suBubbleEffect        = bind(shader, "u_bubbleEffect");
    const auto suIridescenceStrength = bind(shader, "u_iridescenceStrength");
    const auto suEdgeFactorMin       = bind(shader, "u_edgeFactorMin");
    const auto suEdgeFactorMax       = bind(shader, "u_edgeFactorMax");
    const auto suEdgeFactorStrength  = bind(shader, "u_edgeFactorStrength");
    const auto suDistorsionStrength  = bind(shader, "u_distorsionStrength");
    const auto suSubTexOrigin        = bind(shader, "u_subTexOrigin");
    const auto suSubTexSize          = bind(shader, "u_subTexSize");
    const auto suBubbleLightness     = bind(shader, "u_bubbleLightness");
    const auto suLensDistortion      = bind(shader, "u_lensDistortion");
    const auto suRimShineStrength    = bind(shader, "u_rimShineStrength");
    const auto suRimShineFallRate    = bind(shader, "u_rimShineFallRate");
    const auto suRimShineTimeRate    = bind(shader, "u_rimShineTimeRate");
    const auto suRimShineArc         = bind(shader, "u_rimShineArc");

    // TODO P2: (lib) add support for `#include` in shaders
    auto shaderPostProcess = loadShader({.fragmentPath = "resources/postprocess.frag"});

    const auto suPPVibrance   = bind(shaderPostProcess, "u_vibrance");
    const auto suPPSaturation = bind(shaderPostProcess, "u_saturation");
    const auto suPPLightness  = bind(shaderPostProcess, "u_lightness");
    const auto suPPSharpness  = bind(shaderPostProcess, "u_sharpness");
    const auto suPPBlur       = bind(shaderPostProcess, "u_blur");

    auto shaderClouds = loadShader({.fragmentPath = "resources/clouds.frag"});

    const auto suCloudTime       = bind(shaderClouds, "u_time");
    const auto suCloudResolution = bind(shaderClouds, "u_resolution");

    auto shaderHexed = loadShader({.fragmentPath = "resources/hexed_cat.frag"});

    const auto suHexedTime               = bind(shaderHexed, "u_time");
    const auto suHexedSeed               = bind(shaderHexed, "u_seed");
    const auto suHexedDistortionStrength = bind(shaderHexed, "u_distortionStrength");
    const auto suHexedShimmerStrength    = bind(shaderHexed, "u_shimmerStrength");

    auto shaderShrineBackground = loadShader(
        {.vertexPath = "resources/shrine_background.vert", .fragmentPath = "resources/shrine_background.frag"});

    const auto suShrineBgTime               = bind(shaderShrineBackground, "u_time");
    const auto suShrineBgViewOrigin         = bind(shaderShrineBackground, "u_viewOrigin");
    const auto suShrineBgCenter             = bind(shaderShrineBackground, "u_shrineCenter");
    const auto suShrineBgRange              = bind(shaderShrineBackground, "u_shrineRange");
    const auto suShrineBgTintR              = bind(shaderShrineBackground, "u_shrineTintR");
    const auto suShrineBgTintG              = bind(shaderShrineBackground, "u_shrineTintG");
    const auto suShrineBgTintB              = bind(shaderShrineBackground, "u_shrineTintB");
    const auto suShrineBgTintA              = bind(shaderShrineBackground, "u_shrineTintA");
    const auto suShrineBgDistortionStrength = bind(shaderShrineBackground, "u_distortionStrength");
    const auto suShrineBgTintStrength       = bind(shaderShrineBackground, "u_tintStrength");
    const auto suShrineBgEffectStrength     = bind(shaderShrineBackground, "u_effectStrength");

    return new MainShaders{
        .shader                       = ZA_MOVE(shader),
        .suBackgroundTexture          = suBackgroundTexture,
        .suTime                       = suTime,
        .suResolution                 = suResolution,
        .suBackgroundOrigin           = suBackgroundOrigin,
        .suBubbleEffect               = suBubbleEffect,
        .suIridescenceStrength        = suIridescenceStrength,
        .suEdgeFactorMin              = suEdgeFactorMin,
        .suEdgeFactorMax              = suEdgeFactorMax,
        .suEdgeFactorStrength         = suEdgeFactorStrength,
        .suDistorsionStrength         = suDistorsionStrength,
        .suSubTexOrigin               = suSubTexOrigin,
        .suSubTexSize                 = suSubTexSize,
        .suBubbleLightness            = suBubbleLightness,
        .suLensDistortion             = suLensDistortion,
        .suRimShineStrength           = suRimShineStrength,
        .suRimShineFallRate           = suRimShineFallRate,
        .suRimShineTimeRate           = suRimShineTimeRate,
        .suRimShineArc                = suRimShineArc,
        .shaderPostProcess            = ZA_MOVE(shaderPostProcess),
        .suPPVibrance                 = suPPVibrance,
        .suPPSaturation               = suPPSaturation,
        .suPPLightness                = suPPLightness,
        .suPPSharpness                = suPPSharpness,
        .suPPBlur                     = suPPBlur,
        .shaderClouds                 = ZA_MOVE(shaderClouds),
        .suCloudTime                  = suCloudTime,
        .suCloudResolution            = suCloudResolution,
        .shaderHexed                  = ZA_MOVE(shaderHexed),
        .suHexedTime                  = suHexedTime,
        .suHexedSeed                  = suHexedSeed,
        .suHexedDistortionStrength    = suHexedDistortionStrength,
        .suHexedShimmerStrength       = suHexedShimmerStrength,
        .shaderShrineBackground       = ZA_MOVE(shaderShrineBackground),
        .suShrineBgTime               = suShrineBgTime,
        .suShrineBgViewOrigin         = suShrineBgViewOrigin,
        .suShrineBgCenter             = suShrineBgCenter,
        .suShrineBgRange              = suShrineBgRange,
        .suShrineBgTintR              = suShrineBgTintR,
        .suShrineBgTintG              = suShrineBgTintG,
        .suShrineBgTintB              = suShrineBgTintB,
        .suShrineBgTintA              = suShrineBgTintA,
        .suShrineBgDistortionStrength = suShrineBgDistortionStrength,
        .suShrineBgTintStrength       = suShrineBgTintStrength,
        .suShrineBgEffectStrength     = suShrineBgEffectStrength,
    };
}()),
    shaders(*shadersStorage),
    aaLevel(za::min(16u, za::RenderTexture::getMaximumSampleCount())),
    gameStorage(new MainGameStorage{
        .profile =
            [&]
{
    Profile out;

    if (za::Path{"userdata/profile.json"}.exists())
    {
        loadProfileFromFile(out);
        za::printLn("Loaded profile from file on startup");
    }

    return out;
}(),
        .gameConstants =
            [&]
{
    GameConstants out;

    if (za::Path{"resources/game_constants.json"}.exists())
    {
        loadGameConstantsFromFile(out);
        za::printLn("Loaded game constants from file on startup");
    }

    return out;
}(),
        .ptMain     = {},
        .ptSpeedrun = {},
    }),
    profile(gameStorage->profile),
    gameConstants(gameStorage->gameConstants),
    fontMouldyCheeseStorage(new za::Font{za::Font::openFromFile("resources/fredoka.ttf").value()}),
    fontMouldyCheese(*fontMouldyCheeseStorage),
    windowStorage(new za::RenderWindow{makeWindow()}),
    window(*windowStorage),
    loadingGuard(
        [&]
{
    refreshWindowAutoBatchModeFromProfile();
    window.clear(za::Color::Black);

    window.draw(fontMouldyCheese,
                za::TextUtils::anchored(fontMouldyCheese,
                                        za::TextData{.position         = window.getSize().toVec2f() / 2.f,
                                                     .string           = "Loading...",
                                                     .characterSize    = 48u,
                                                     .fillColor        = za::Color::White,
                                                     .outlineColor     = colorBlueOutline,
                                                     .outlineThickness = 2.f},
                                        {0.5f, 0.5f}));

    window.display();
    return true;
}()),
    imGuiContextStorage(new za::ImGuiContext{/* loadDefaultFont */ false}),
    imGuiContext(*imGuiContextStorage),
    textureAtlasStorage(new za::TextureAtlas{za::Texture::create({6000u, 4096u}, {.smooth = true}).value()}),
    textureAtlas(*textureAtlasStorage),
    fontSuperBakeryStorage(new za::Font{za::Font::openFromFile("resources/fredoka.ttf", &textureAtlas).value()}),
    fontSuperBakery(*fontSuperBakeryStorage),
    fontImGuiMouldyCheese(ImGui::GetIO().Fonts->AddFontFromFileTTF("resources/fredoka.ttf", 28.f)),
    fontImGuiSuperBakery(ImGui::GetIO().Fonts->AddFontFromFileTTF("resources/fredoka.ttf", 28.f)),
    fontImGuiFA(
        []
{
    static const ImWchar iconRanges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};

    ImFontConfig iconConfig;
    iconConfig.MergeMode        = true;
    iconConfig.PixelSnapH       = true;
    iconConfig.GlyphMinAdvanceX = 18.0f; // Helps keep icons square
    iconConfig.GlyphOffset.y    = -1.0f; // Moves icons down 2 pixels

    // 3. Load FontAwesome into the SAME font object
    auto* res = ImGui::GetIO().Fonts->AddFontFromFileTTF("resources/fa-solid-900.ttf", 16.0f, &iconConfig, iconRanges);
    ZA_ASSERT(res != nullptr);

    ImGui::GetIO().Fonts->Build();
    return res;
}()),
    bgmStorage(new MainBGMStorage{}),
    bgm(*bgmStorage),
    soundManagerStorage(new SoundManager{}),
    soundManager(*soundManagerStorage),
    listenerStorage(new za::Listener{}),
    listener(*listenerStorage),
    rtBackgroundStorage(new za::RenderTexture{
        za::RenderTexture::create(gameScreenSize.toVec2u(),
                                  {.smooth = true, .wrapMode = za::TextureWrapMode::Repeat, .sampleCount = aaLevel})
            .value()}),
    rtBackground(*rtBackgroundStorage),
    rtBackgroundProcessedStorage(new za::RenderTexture{
        za::RenderTexture::create(gameScreenSize.toVec2u(), {.smooth = true, .sampleCount = aaLevel}).value()}),
    rtBackgroundProcessed(*rtBackgroundProcessedStorage),
    rtImGuiStorage(new za::RenderTexture{
        za::RenderTexture::create(window.getSize(), {.smooth = true, .sampleCount = aaLevel}).value()}),
    rtImGui(*rtImGuiStorage),
    rtCloudMaskStorage(new za::RenderTexture{
        za::RenderTexture::create(window.getSize(), {.smooth = true, .sampleCount = aaLevel}).value()}),
    rtCloudMask(*rtCloudMaskStorage),
    rtCloudProcessedStorage(new za::RenderTexture{
        za::RenderTexture::create(window.getSize(), {.smooth = true, .sampleCount = aaLevel}).value()}),
    rtCloudProcessed(*rtCloudProcessedStorage),
    rtGameStorage(new za::RenderTexture{
        za::RenderTexture::create(window.getSize(), {.smooth = true, .sampleCount = aaLevel}).value()}),
    rtGame(*rtGameStorage),
    hexedCatRenderTexturesStorage(new MainRenderTextureVector{[this]
{
    MainRenderTextureVector result;
    result.reserve(maxHexedCatRenderTextures);

    for (za::SizeT i = 0u; i < maxHexedCatRenderTextures; ++i)
        result.emplaceBack(
            za::RenderTexture::create(hexedCatRenderTextureSize, {.smooth = true, .sampleCount = aaLevel}).value());

    return result;
}()}),
    hexedCatRenderTextures(*hexedCatRenderTexturesStorage),
    textureStorage(new MainTextureStorage{}),
    txLogo(textureStorage->txLogo),
    txFixedBg(textureStorage->txFixedBg),
    txBackgroundChunk(textureStorage->txBackgroundChunk),
    txBackgroundChunkDesaturated(textureStorage->txBackgroundChunkDesaturated),
    txClouds(textureStorage->txClouds),
    txTintedClouds(textureStorage->txTintedClouds),
    txBgSwamp(textureStorage->txBgSwamp),
    txBgObservatory(textureStorage->txBgObservatory),
    txBgAimTraining(textureStorage->txBgAimTraining),
    txBgFactory(textureStorage->txBgFactory),
    txBgWindTunnel(textureStorage->txBgWindTunnel),
    txBgMagnetosphere(textureStorage->txBgMagnetosphere),
    txBgAuditorium(textureStorage->txBgAuditorium),
    txDrawings(textureStorage->txDrawings),
    txTipBg(textureStorage->txTipBg),
    txTipByte(textureStorage->txTipByte),
    txCursor(textureStorage->txCursor),
    txCursorMultipop(textureStorage->txCursorMultipop),
    txCursorLaser(textureStorage->txCursorLaser),
    txCursorGrab(textureStorage->txCursorGrab),
    txArrow(textureStorage->txArrow),
    txUnlock(textureStorage->txUnlock),
    txPurchasable(textureStorage->txPurchasable),
    txLetter(textureStorage->txLetter),
    txLetterText(textureStorage->txLetterText),
    txFrame(textureStorage->txFrame),
    txFrameTiny(textureStorage->txFrameTiny),
    txCloudBtn(textureStorage->txCloudBtn),
    txCloudBtnSmall(textureStorage->txCloudBtnSmall),
    txCloudBtnSquare(textureStorage->txCloudBtnSquare),
    txCloudBtnSquare2(textureStorage->txCloudBtnSquare2),
    uiTextureAtlasStorage(new za::TextureAtlas{za::Texture::create({2048u, 1024u}, {.smooth = true}).value()}),
    uiTextureAtlas(*uiTextureAtlasStorage),
    atlasRectsStorage(new MainAtlasRects{
        .txrIconVolume          = addImgResourceToUIAtlas("iconvolumeon.png"),
        .txrIconBGM             = addImgResourceToUIAtlas("iconmusicon.png"),
        .txrIconBg              = addImgResourceToUIAtlas("iconbg.png"),
        .txrIconCfg             = addImgResourceToUIAtlas("iconcfg.png"),
        .txrIconCopyCat         = addImgResourceToUIAtlas("iconcopycat.png"),
        .txrMenuSeparator0      = addImgResourceToUIAtlas("menuseparator0.png"),
        .txrMenuSeparator1      = addImgResourceToUIAtlas("menuseparator1.png"),
        .txrMenuSeparator2      = addImgResourceToUIAtlas("menuseparator2.png"),
        .txrMenuSeparator3      = addImgResourceToUIAtlas("menuseparator3.png"),
        .txrMenuSeparator4      = addImgResourceToUIAtlas("menuseparator4.png"),
        .txrMenuSeparator5      = addImgResourceToUIAtlas("menuseparator5.png"),
        .txrMenuSeparator6      = addImgResourceToUIAtlas("menuseparator6.png"),
        .txrMenuSeparator7      = addImgResourceToUIAtlas("menuseparator7.png"),
        .txrMenuSeparator8      = addImgResourceToUIAtlas("menuseparator8.png"),
        .txrPrestigeSeparator0  = addImgResourceToUIAtlas("prestigeseparator0.png"),
        .txrPrestigeSeparator1  = addImgResourceToUIAtlas("prestigeseparator1.png"),
        .txrPrestigeSeparator2  = addImgResourceToUIAtlas("prestigeseparator2.png"),
        .txrPrestigeSeparator3  = addImgResourceToUIAtlas("prestigeseparator3.png"),
        .txrPrestigeSeparator4  = addImgResourceToUIAtlas("prestigeseparator4.png"),
        .txrPrestigeSeparator5  = addImgResourceToUIAtlas("prestigeseparator5.png"),
        .txrPrestigeSeparator6  = addImgResourceToUIAtlas("prestigeseparator6.png"),
        .txrPrestigeSeparator7  = addImgResourceToUIAtlas("prestigeseparator7.png"),
        .txrPrestigeSeparator8  = addImgResourceToUIAtlas("prestigeseparator8.png"),
        .txrPrestigeSeparator9  = addImgResourceToUIAtlas("prestigeseparator9.png"),
        .txrPrestigeSeparator10 = addImgResourceToUIAtlas("prestigeseparator10.png"),
        .txrPrestigeSeparator11 = addImgResourceToUIAtlas("prestigeseparator11.png"),
        .txrPrestigeSeparator12 = addImgResourceToUIAtlas("prestigeseparator12.png"),
        .txrPrestigeSeparator13 = addImgResourceToUIAtlas("prestigeseparator13.png"),
        .txrPrestigeSeparator14 = addImgResourceToUIAtlas("prestigeseparator14.png"),
        .txrPrestigeSeparator15 = addImgResourceToUIAtlas("prestigeseparator15.png"),
        .txrMagicSeparator0     = addImgResourceToUIAtlas("magicseparator0.png"),
        .txrMagicSeparator1     = addImgResourceToUIAtlas("magicseparator1.png"),
        .txrMagicSeparator2     = addImgResourceToUIAtlas("magicseparator2.png"),
        .txrMagicSeparator3     = addImgResourceToUIAtlas("magicseparator3.png"),
        .txrWhiteDot            = textureAtlas.add(za::GraphicsContext::getBuiltInWhiteDotTexture()).value(),
        .txrBubble              = addImgResourceToAtlas("bubble2.png"),
        .txrBubbleStar          = addImgResourceToAtlas("bubble3.png"),
        .txrBubbleNova          = addImgResourceToAtlas("bubble4.png"),
        .txrBubbleGlass         = addImgResourceToAtlas("bubbleglass.png"),
        .txrCat                 = addImgResourceToAtlas("cat.png"),
        .txrGuardhouseBack      = addImgResourceToAtlas("guardhouse_back.png"),
        .txrWardenCat           = addImgResourceToAtlas("wardencat.png"),
        .txrGuardhouseFront     = addImgResourceToAtlas("guardhouse_front.png"),
        .txrWardencatPaw        = addImgResourceToAtlas("wardencatpaw.png"),
        .txrUniCat              = addImgResourceToAtlas("unicat3.png"),
        .txrUniCat2             = addImgResourceToAtlas("unicat2.png"),
        .txrUniCatWings         = addImgResourceToAtlas("unicatwings.png"),
        .txrDevilCat2           = addImgResourceToAtlas("devilcat2.png"),
        .txrDevilCat3           = addImgResourceToAtlas("devilcat3.png"),
        .txrDevilCat3Arm        = addImgResourceToAtlas("devilcat3arm.png"),
        .txrDevilCat3Book       = addImgResourceToAtlas("devilcat3book.png"),
        .txrDevilCat3Tail       = addImgResourceToAtlas("devilcat3tail.png"),
        .txrDevilCat2Book       = addImgResourceToAtlas("devilcat2book.png"),
        .txrCatPaw              = addImgResourceToAtlas("catpaw.png"),
        .txrCatTail             = addImgResourceToAtlas("cattail.png"),
        .txrSmartCatHat         = addImgResourceToAtlas("smartcathat.png"),
        .txrSmartCatDiploma     = addImgResourceToAtlas("smartcatdiploma.png"),
        .txrBrainBack           = addImgResourceToAtlas("brainback.png"),
        .txrBrainFront          = addImgResourceToAtlas("brainfront.png"),
        .txrUniCatTail          = addImgResourceToAtlas("unicattail.png"),
        .txrUniCat2Tail         = addImgResourceToAtlas("unicat2tail.png"),
        .txrDevilCatTail2       = addImgResourceToAtlas("devilcattail2.png"),
        .txrAstroCatTail        = addImgResourceToAtlas("astrocattail.png"),
        .txrAstroCatFlag        = addImgResourceToAtlas("astrocatflag.png"),
        .txrWitchCatTail        = addImgResourceToAtlas("witchcattail.png"),
        .txrWizardCatTail       = addImgResourceToAtlas("wizardcattail.png"),
        .txrMouseCatTail        = addImgResourceToAtlas("mousecattail.png"),
        .txrMouseCatMouse       = addImgResourceToAtlas("mousecatmouse.png"),
        .txrEngiCatTail         = addImgResourceToAtlas("engicattail.png"),
        .txrEngiCatWrench       = addImgResourceToAtlas("engicatwrench.png"),
        .txrRepulsoCatTail      = addImgResourceToAtlas("repulsocattail.png"),
        .txrAttractoCatTail     = addImgResourceToAtlas("attractocattail.png"),
        .txrCopyCatTail         = addImgResourceToAtlas("copycattail.png"),
        .txrAttractoCatMagnet   = addImgResourceToAtlas("attractocatmagnet.png"),
        .txrUniCatPaw           = addImgResourceToAtlas("unicatpaw.png"),
        .txrDevilCatPaw         = addImgResourceToAtlas("devilcatpaw.png"),
        .txrDevilCatPaw2        = addImgResourceToAtlas("devilcatpaw2.png"),
        .txrParticle            = addImgResourceToAtlas("particle.png"),
        .txrStarParticle        = addImgResourceToAtlas("starparticle.png"),
        .txrFireParticle        = addImgResourceToAtlas("fireparticle.png"),
        .txrFireParticle2       = addImgResourceToAtlas("fireparticle2.png"),
        .txrSmokeParticle       = addImgResourceToAtlas("smokeparticle.png"),
        .txrExplosionParticle   = addImgResourceToAtlas("explosionparticle.png"),
        .txrTrailParticle       = addImgResourceToAtlas("trailparticle.png"),
        .txrHexParticle         = addImgResourceToAtlas("hexparticle.png"),
        .txrShrineParticle      = addImgResourceToAtlas("shrineparticle.png"),
        .txrCogParticle         = addImgResourceToAtlas("cogparticle.png"),
        .txrGlassParticle       = addImgResourceToAtlas("glassparticle.png"),
        .txrWitchCat            = addImgResourceToAtlas("witchcat.png"),
        .txrWitchCatPaw         = addImgResourceToAtlas("witchcatpaw.png"),
        .txrAstroCat            = addImgResourceToAtlas("astromeow.png"),
        .txrBomb                = addImgResourceToAtlas("bomb.png"),
        .txrShrine              = addImgResourceToAtlas("shrine.png"),
        .txrWizardCat           = addImgResourceToAtlas("wizardcat.png"),
        .txrWizardCatPaw        = addImgResourceToAtlas("wizardcatpaw.png"),
        .txrMouseCat            = addImgResourceToAtlas("mousecat.png"),
        .txrMouseCatPaw         = addImgResourceToAtlas("mousecatpaw.png"),
        .txrEngiCat             = addImgResourceToAtlas("engicat.png"),
        .txrEngiCatPaw          = addImgResourceToAtlas("engicatpaw.png"),
        .txrRepulsoCat          = addImgResourceToAtlas("repulsocat.png"),
        .txrRepulsoCatPaw       = addImgResourceToAtlas("repulsocatpaw.png"),
        .txrAttractoCat         = addImgResourceToAtlas("attractocat.png"),
        .txrCopyCat             = addImgResourceToAtlas("copycat.png"),
        .txrDuckCat             = addImgResourceToAtlas("duck.png"),
        .txrDuckFlag            = addImgResourceToAtlas("duckflag.png"),
        .txrAttractoCatPaw      = addImgResourceToAtlas("attractocatpaw.png"),
        .txrCopyCatPaw          = addImgResourceToAtlas("copycatpaw.png"),
        .txrDollNormal          = addImgResourceToAtlas("dollnormal.png"),
        .txrDollUni             = addImgResourceToAtlas("dolluni.png"),
        .txrDollDevil           = addImgResourceToAtlas("dolldevil.png"),
        .txrDollAstro           = addImgResourceToAtlas("dollastro.png"),
        .txrDollWizard          = addImgResourceToAtlas("dollwizard.png"),
        .txrDollMouse           = addImgResourceToAtlas("dollmouse.png"),
        .txrDollEngi            = addImgResourceToAtlas("dollengi.png"),
        .txrDollRepulso         = addImgResourceToAtlas("dollrepulso.png"),
        .txrDollAttracto        = addImgResourceToAtlas("dollattracto.png"),
        .txrCoin                = addImgResourceToAtlas("bytecoin.png"),
        .txrCatSoul             = addImgResourceToAtlas("catsoul.png"),
        .txrHellPortal          = addImgResourceToAtlas("hellportal.png"),
        .txrCatEyeLid0          = addImgResourceToAtlas("cateyelid0.png"),
        .txrCatEyeLid1          = addImgResourceToAtlas("cateyelid1.png"),
        .txrCatEyeLid2          = addImgResourceToAtlas("cateyelid2.png"),
        .txrCatWhiteEyeLid0     = addImgResourceToAtlas("catwhiteeyelid0.png"),
        .txrCatWhiteEyeLid1     = addImgResourceToAtlas("catwhiteeyelid1.png"),
        .txrCatWhiteEyeLid2     = addImgResourceToAtlas("catwhiteeyelid2.png"),
        .txrCatDarkEyeLid0      = addImgResourceToAtlas("catdarkeyelid0.png"),
        .txrCatDarkEyeLid1      = addImgResourceToAtlas("catdarkeyelid1.png"),
        .txrCatDarkEyeLid2      = addImgResourceToAtlas("catdarkeyelid2.png"),
        .txrCatGrayEyeLid0      = addImgResourceToAtlas("catgrayeyelid0.png"),
        .txrCatGrayEyeLid1      = addImgResourceToAtlas("catgrayeyelid1.png"),
        .txrCatGrayEyeLid2      = addImgResourceToAtlas("catgrayeyelid2.png"),
        .txrCatEars0            = addImgResourceToAtlas("catears0.png"),
        .txrCatEars1            = addImgResourceToAtlas("catears1.png"),
        .txrCatEars2            = addImgResourceToAtlas("catears2.png"),
        .txrCatYawn0            = addImgResourceToAtlas("catyawn0.png"),
        .txrCatYawn1            = addImgResourceToAtlas("catyawn1.png"),
        .txrCatYawn2            = addImgResourceToAtlas("catyawn2.png"),
        .txrCatYawn3            = addImgResourceToAtlas("catyawn3.png"),
        .txrCatYawn4            = addImgResourceToAtlas("catyawn4.png"),
        .txrCCMaskWitch         = addImgResourceToAtlas("ccmaskwitch.png"),
        .txrCCMaskWizard        = addImgResourceToAtlas("ccmaskwizard.png"),
        .txrCCMaskMouse         = addImgResourceToAtlas("ccmaskmouse.png"),
        .txrCCMaskEngi          = addImgResourceToAtlas("ccmaskengi.png"),
        .txrCCMaskRepulso       = addImgResourceToAtlas("ccmaskrepulso.png"),
        .txrCCMaskAttracto      = addImgResourceToAtlas("ccmaskattracto.png"),
        .txrMMNormal            = addImgResourceToAtlas("mmcatnormal.png"),
        .txrMMUni               = addImgResourceToAtlas("mmcatuni.png"),
        .txrMMDevil             = addImgResourceToAtlas("mmcatdevil.png"),
        .txrMMAstro             = addImgResourceToAtlas("mmcatastro.png"),
        .txrMMWitch             = addImgResourceToAtlas("mmcatwitch.png"),
        .txrMMWizard            = addImgResourceToAtlas("mmcatwizard.png"),
        .txrMMMouse             = addImgResourceToAtlas("mmcatmouse.png"),
        .txrMMEngi              = addImgResourceToAtlas("mmcatengi.png"),
        .txrMMRepulso           = addImgResourceToAtlas("mmcatrepulso.png"),
        .txrMMAttracto          = addImgResourceToAtlas("mmcatattracto.png"),
        .txrMMCopy              = addImgResourceToAtlas("mmcatcopy.png"),
        .txrMMDuck              = addImgResourceToAtlas("mmduck.png"),
        .txrMMShrine            = addImgResourceToAtlas("mmshrine.png"),
        .txrCloud               = addImgResourceToAtlas("cloud.png"),
    }),
    atlasRects(*atlasRectsStorage),
    ptMain(gameStorage->ptMain),
    ptSpeedrun(gameStorage->ptSpeedrun),
    pt(&ptMain),
    textStorage(new MainTextStorage{fontSuperBakery}),
    moneyText(textStorage->moneyText),
    comboStateStorage(new ComboState{moneyTextInitialPosition}),
    comboState(*comboStateStorage),
    demoText(textStorage->demoText),
    sweepAndPrune(za::makeUnique<SweepAndPrune>()),
    seed(static_cast<RNGSeedType>(za::Clock::now().asMicroseconds())),
    shuffledCatNamesPerType(makeShuffledCatNames(rng)),
    drawableBatchesStorage(new MainDrawableBatches{}),
    bubbleDrawableBatch(drawableBatchesStorage->bubbleDrawableBatch),
    starBubbleDrawableBatch(drawableBatchesStorage->starBubbleDrawableBatch),
    bombBubbleDrawableBatch(drawableBatchesStorage->bombBubbleDrawableBatch),
    cpuCloudDrawableBatch(drawableBatchesStorage->cpuCloudDrawableBatch),
    cpuTopCloudDrawableBatch(drawableBatchesStorage->cpuTopCloudDrawableBatch),
    cpuCloudHudDrawableBatch(drawableBatchesStorage->cpuCloudHudDrawableBatch),
    cpuCloudUiDrawableBatch(drawableBatchesStorage->cpuCloudUiDrawableBatch),
    cpuDrawableBatchBeforeCats(drawableBatchesStorage->cpuDrawableBatchBeforeCats),
    cpuDrawableBatch(drawableBatchesStorage->cpuDrawableBatch),
    cpuDrawableBatchAfterCats(drawableBatchesStorage->cpuDrawableBatchAfterCats),
    cpuDrawableBatchAdditive(drawableBatchesStorage->cpuDrawableBatchAdditive),
    minimapDrawableBatch(drawableBatchesStorage->minimapDrawableBatch),
    catTextDrawableBatch(drawableBatchesStorage->catTextDrawableBatch),
    hudDrawableBatch(drawableBatchesStorage->hudDrawableBatch),
    hudTopDrawableBatch(drawableBatchesStorage->hudTopDrawableBatch),
    hudBottomDrawableBatch(drawableBatchesStorage->hudBottomDrawableBatch),
    cpuTopDrawableBatch(drawableBatchesStorage->cpuTopDrawableBatch),
    catTextTopDrawableBatch(drawableBatchesStorage->catTextTopDrawableBatch),
    tempDrawableBatch(drawableBatchesStorage->tempDrawableBatch),
    textNameBuffer(textStorage->textNameBuffer),
    textStatusBuffer(textStorage->textStatusBuffer),
    textMoneyBuffer(textStorage->textMoneyBuffer),
    threadPool(getTPWorkerCount()),
    gameViewStorage(new za::View{.center = {1.f, 1.f}, .size = {1.f, 1.f}}),
    gameView(*gameViewStorage),
    nonScaledHUDViewStorage(new za::View{.center = {1.f, 1.f}, .size = {1.f, 1.f}}),
    nonScaledHUDView(*nonScaledHUDViewStorage),
    scaledHUDViewStorage(new za::View{.center = {1.f, 1.f}, .size = {1.f, 1.f}}),
    scaledHUDView(*scaledHUDViewStorage),
    bombStorage(new MainBombStorage{}),
    notificationStateStorage(new NotificationState{}),
    notificationState(*notificationStateStorage),
    uiStateStorage(new UIState{}),
    uiState(*uiStateStorage),
#ifdef BUBBLEBYTE_USE_STEAMWORKS
    steamMgr(xSteamMgr),
    onSteamDeck(steamMgr.isOnSteamDeck()),
#else
    onSteamDeck(false),
#endif
    inputHelperStorage(new InputHelper{}),
    inputHelper(*inputHelperStorage),
    logFileStorage(
        []
{
    auto opt = za::OutFile::open("bubblebyte.log", za::FileOpenMode::out | za::FileOpenMode::app);
    return opt.hasValue() ? MainOwnedPtr<za::OutFile>(new za::OutFile(ZA_MOVE(*opt))) : MainOwnedPtr<za::OutFile>{};
}()),
    logFile(logFileStorage.get())
{
    sounds.setupSounds(/* volumeOnly */ true, profile.sfxVolume / 100.f);

    if (onSteamDeck)
    {
        // borderless windowed
        profile.resWidth = za::VideoModeUtils::getDesktopMode().size;
        profile.windowed = true;

        profile.uiScale = 1.25f;
    }

    //
    // Playthrough
    if (za::Path{"userdata/playthrough.json"}.exists())
    {
        loadPlaythroughFromFileAndReseed();
        za::printLn("Loaded playthrough from file on startup");
    }
    else
    {
        pt->seed = seed;
    }

    //
    // Reserve memory
    particles.reserve(512);
    spentCoinParticles.reserve(512);
    textParticles.reserve(256);
    pt->bubbles.reserve(32'768);
    pt->cats.reserve(512);

    //
    // Touch state
    playerInputState.fingerPositions.resize(10);
}

////////////////////////////////////////////////////////////
Main::~Main()
{
    za::printLn("Saving playthrough to file on exit");
    saveMainPlaythroughToFile();

    za::printLn("Saving profile to file on exit");
    saveProfileToFile(profile);
}

////////////////////////////////////////////////////////////
void Main::run()
{
    //
    // Startup (splash screen and meow)
    splashCountdown.restart();
    playSound(sounds.byteMeow);

    //
    //
    // Background music
    auto& [entries, selectedIndex] = getBGMSelectorData();
    selectBGM(entries, selectedIndex);
    switchToBGM(static_cast<za::SizeT>(profile.selectedBGM), /* force */ true);

    //
    // Game loop
    playedClock.start();

    while (true)
        if (!gameLoop())
            return;
}
