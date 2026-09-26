#include "AnimationCommands.hpp"
#include "AnimationTimeline.hpp"
#include "BitmapFont.hpp"
#include "BitmapTextAlignment.hpp"
#include "BitmapTextUtils.hpp"
#include "Block.hpp"
#include "BlockGrid.hpp"
#include "BlockMatrix.hpp"
#include "Constants.hpp"
#include "DefaultPerks.hpp"
#include "DrillDirection.hpp"
#include "ExampleProfiler/Profiler.hpp"
#include "ExampleProfiler/ProfilerImGui.hpp"
#include "LaserBeam.hpp"
#include "LaserDirection.hpp"
#include "LaserableBlocksInfo.hpp"
#include "LightningBolt.hpp"
#include "Perk.hpp"
#include "RandomBag.hpp"
#include "ShapeBlockSequence.hpp"
#include "ShapeDimension.hpp"
#include "StringUtils.hpp"
#include "Tetramino.hpp"
#include "TetraminoShapes.hpp"
#include "Utils.hpp"
#include "World.hpp"

#include "ExampleUtils/ControlFlow.hpp"
#include "ExampleUtils/Easing.hpp"
#include "ExampleUtils/HueColor.hpp"
#include "ExampleUtils/LoadedSound.hpp"
#include "ExampleUtils/MathUtils.hpp"
#include "ExampleUtils/Progress.hpp"
#include "ExampleUtils/RNGFast.hpp"
#include "ExampleUtils/Scaling.hpp"
#include "ExampleUtils/SoundManager.hpp"

#include "Zancle/Audio/AudioContext.hpp"
#include "Zancle/Audio/Music.hpp"
#include "Zancle/Audio/MusicReader.hpp"
#include "Zancle/Audio/PlaybackDevice.hpp"
#include "Zancle/Audio/Sound.hpp"
#include "Zancle/Audio/SoundBuffer.hpp"

#include "Zancle/Graphics/BlendMode.hpp"
#include "Zancle/Graphics/CircleShapeData.hpp"
#include "Zancle/Graphics/Color.hpp"
#include "Zancle/Graphics/DrawIndexedVerticesSettings.hpp"
#include "Zancle/Graphics/DrawTextureSettings.hpp"
#include "Zancle/Graphics/DrawableBatch.hpp"
#include "Zancle/Graphics/Font.hpp"
#include "Zancle/Graphics/GraphicsContext.hpp"
#include "Zancle/Graphics/Image.hpp"
#include "Zancle/Graphics/IndexType.hpp"
#include "Zancle/Graphics/PrimitiveType.hpp"
#include "Zancle/Graphics/RectangleShape.hpp"
#include "Zancle/Graphics/RectangleShapeData.hpp"
#include "Zancle/Graphics/RenderTarget.hpp"
#include "Zancle/Graphics/RenderTexture.hpp"
#include "Zancle/Graphics/RenderWindow.hpp"
#include "Zancle/Graphics/Shader.hpp"
#include "Zancle/Graphics/Sprite.hpp"
#include "Zancle/Graphics/Text.hpp"
#include "Zancle/Graphics/Texture.hpp"
#include "Zancle/Graphics/TextureAtlas.hpp"
#include "Zancle/Graphics/Transform.hpp"
#include "Zancle/Graphics/Vertex.hpp"
#include "Zancle/Graphics/View.hpp"

#include "Zancle/ImGui/ImGuiContext.hpp"
#include "Zancle/ImGui/IncludeImGui.hpp"

#include "Zancle/Window/Event.hpp"
#include "Zancle/Window/EventUtils.hpp"
#include "Zancle/Window/Keyboard.hpp"
#include "Zancle/Window/VideoMode.hpp"
#include "Zancle/Window/VideoModeUtils.hpp"

#include "Zancle/Fmt/FmtToString.hpp"

#include "Zancle/IO/Path.hpp"

#include "Zancle/String/StringView.hpp"
#include "Zancle/String/ToString.hpp"

#include "Zancle/Algorithm/Erase.hpp"
#include "Zancle/Algorithm/Find.hpp"
#include "Zancle/Algorithm/Sort.hpp"

#include "Zancle/Chrono/Clock.hpp"
#include "Zancle/Chrono/Time.hpp"

#include "Zancle/Container/AnkerlUnorderedDense.hpp"
#include "Zancle/Container/Array.hpp"
#include "Zancle/Container/InPlaceVector.hpp"
#include "Zancle/Container/Vector.hpp"

#include "Zancle/Geometry/Angle.hpp"
#include "Zancle/Geometry/Rect2.hpp"
#include "Zancle/Geometry/Vec2.hpp"

#include "Zancle/Vocabulary/Optional.hpp"
#include "Zancle/Vocabulary/OverloadSet.hpp"
#include "Zancle/Vocabulary/UniquePtr.hpp"
#include "Zancle/Vocabulary/Variant.hpp"

#include "Zancle/Math/Clamp.hpp"
#include "Zancle/Math/Constants.hpp"
#include "Zancle/Math/Floor.hpp"
#include "Zancle/Math/Fmod.hpp"
#include "Zancle/Math/MinMax.hpp"
#include "Zancle/Math/Remainder.hpp"
#include "Zancle/Math/Sin.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/SizeT.hpp"
#include "Zancle/Base/Unreachable.hpp"

#include "Zancle/Trait/IsConst.hpp"


namespace
{
////////////////////////////////////////////////////////////
constexpr za::Array<za::StringView, 7> glyphRows{"ABCDEFGHIJKLM",
                                                 "NOPQRSTUVWXYZ",
                                                 "abcdefghijklm",
                                                 "nopqrstuvwxyz",
                                                 "0123456789+-=",
                                                 "()[]{}<>/*:#%",
                                                 "!?.,'\"@&$"};


////////////////////////////////////////////////////////////
[[nodiscard]] tsurv::BitmapFont loadMinogramData()
{
    tsurv::BitmapFont result;

    for (za::SizeT iY = 0; iY < glyphRows.size(); ++iY)
        for (za::SizeT iX = 0; iX < glyphRows[iY].size(); ++iX)
        {
            const char c = glyphRows[iY][iX];
            result.addGlyph(c, {iX * 6u, iY * 10u}, {6u, 10u});
        }

    return result;
}


////////////////////////////////////////////////////////////
[[nodiscard]] tsurv::BitmapFont loadTiny5Data()
{
    tsurv::BitmapFont result;

    za::SizeT stepX = 15;
    za::SizeT stepY = 17;

    for (za::SizeT iY = 0; iY < glyphRows.size(); ++iY)
        for (za::SizeT iX = 0; iX < glyphRows[iY].size(); ++iX)
        {
            const char c = glyphRows[iY][iX];
            result.addGlyph(c, {4 + (stepX * iX), 7 + (stepY * iY)}, {6u, 8u});
        }

    result.adjustSize('I', {-3, 0});
    result.adjustSize('i', {-3, 0});
    result.adjustSize('l', {-3, 0});
    result.adjustSize(':', {-3, 0});
    result.adjustSize('!', {-3, 0});

    result.adjustSize('a', {-1, 0});
    result.adjustSize('b', {-1, 0});
    result.adjustSize('c', {-1, 0});
    result.adjustSize('d', {-1, 0});
    result.adjustSize('e', {-1, 0});
    result.adjustSize('f', {-2, 0});
    result.adjustSize('g', {-1, 0});
    result.adjustSize('h', {-1, 0});
    result.adjustSize('k', {-1, 0});
    result.adjustSize('n', {-1, 0});
    result.adjustSize('o', {-1, 0});
    result.adjustSize('p', {-1, 0});
    result.adjustSize('q', {-1, 0});
    result.adjustSize('r', {-2, 0});
    result.adjustSize('t', {-2, 0});
    result.adjustSize('u', {-1, 0});
    result.adjustSize('v', {-1, 0});
    result.adjustSize('x', {-1, 0});
    result.adjustSize('y', {-1, 0});
    result.adjustSize('z', {-1, 0});
    result.adjustSize('j', {-2, 0});

    result.adjustSize('J', {-1, 0});
    result.adjustSize('L', {-1, 0});
    result.adjustSize('T', {-1, 0});
    result.adjustSize('V', {-1, 0});
    result.adjustSize('X', {-1, 0});
    result.adjustSize('Y', {-1, 0});
    result.adjustSize('Z', {-1, 0});

    result.adjustSize('0', {-1, 0});
    result.adjustSize('1', {-2, 0});
    result.adjustSize('2', {-1, 0});
    result.adjustSize('3', {-1, 0});
    result.adjustSize('4', {-1, 0});
    result.adjustSize('5', {-1, 0});
    result.adjustSize('6', {-1, 0});
    result.adjustSize('7', {-1, 0});
    result.adjustSize('8', {-1, 0});
    result.adjustSize('9', {-1, 0});

    result.adjustSize('(', {-2, 0});
    result.adjustSize(')', {-2, 0});
    result.adjustSize('[', {-2, 0});
    result.adjustSize(']', {-2, 0});

    result.adjustSize('{', {-1, 0});
    result.adjustSize('}', {-1, 0});
    result.adjustSize('<', {-1, 0});
    result.adjustSize('>', {-1, 0});
    result.adjustSize('+', {-1, 0});
    result.adjustSize('-', {-1, 0});
    result.adjustSize('=', {-1, 0});
    result.adjustSize('*', {-1, 0});
    result.adjustSize('?', {-1, 0});

    result.adjustSize('.', {-3, 0});
    result.adjustSize(',', {-3, 0});
    result.adjustSize('\'', {-3, 0});
    result.adjustSize('"', {-1, 0});
    result.adjustSize('@', {-1, 0});
    result.adjustSize('$', {-1, 0});

    result.adjustSize('/', {-2, 0});

    return result;
}

} // namespace

namespace tsurv
{
////////////////////////////////////////////////////////////
struct [[nodiscard]] CircleParticleData
{
    za::Vec2f position;
    za::Vec2f velocity;

    float scale;
    float scaleDecay;

    float accelerationY;

    float opacity;
    float opacityDecay;

    float rotation;
    float torque;

    za::Color color;

    float        radius;
    unsigned int pointCount;
};


////////////////////////////////////////////////////////////
struct [[nodiscard]] SpriteParticleData // NOLINT(cppcoreguidelines-pro-type-member-init)
{
    za::Vec2f position;
    za::Vec2f origin;
    za::Vec2f velocity;

    float scale;
    float scaleDecay;

    float accelerationY;

    float opacity;
    float opacityDecay;

    float rotation;
    float torque;

    za::Rect2f textureRect;
};


////////////////////////////////////////////////////////////
struct [[nodiscard]] EarnedXPParticle // NOLINT(cppcoreguidelines-pro-type-member-init)
{
    za::Vec2f  startPosition;
    za::Vec2f  targetPosition;
    PaletteIdx paletteIdx;

    float delay;
    float startRotation = 0.f;

    Progress progress{};
};


////////////////////////////////////////////////////////////
struct [[nodiscard]] DrawBlockOptions
{
    float opacity          = 1.f;
    float squishMult       = 0.f;
    float rotation         = 0.f;
    float scale            = 1.f;
    bool  drawText         = true;
    bool  applyYOffset     = true;
    bool  applyQuakeOffset = true;
    bool  drawTimer        = false;
};


////////////////////////////////////////////////////////////
struct [[nodiscard]] QuakeSinEffect
{
    float timeRemaining = 0.f;
    float magnitude     = 0.f;
    float speed         = 0.f;

    ////////////////////////////////////////////////////////////
    void update(const za::Time deltaTime)
    {
        if (timeRemaining <= 0.f)
            return;

        timeRemaining -= deltaTime.asSeconds() * speed;

        if (timeRemaining <= 0.f)
        {
            timeRemaining = 0.f;
            magnitude     = 0.f;
        }
    }

    ////////////////////////////////////////////////////////////
    void start(const float newMagnitude, const float newSpeed)
    {
        magnitude = za::max(magnitude, newMagnitude);
        speed     = newSpeed;

        timeRemaining = 1.f;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] float getValue() const
    {
        return za::sin(timeRemaining * za::pi) * magnitude;
    }
};


////////////////////////////////////////////////////////////
struct [[nodiscard]] Sounds
{
    LoadedSound landed{"Landed.wav"};
    LoadedSound newLevel{"NewLevel.wav"};
    LoadedSound rotate{"Rotate.wav"};
    LoadedSound single{"Single.wav"};
    LoadedSound exp{"Exp.wav"};
    LoadedSound place{"Place.wav"};
    LoadedSound hold{"Hold.wav"};
    LoadedSound hit{"Hit.wav"};
    LoadedSound bonus{"Bonus.wav"};
    LoadedSound strike{"Strike.wav"};
    LoadedSound drill{"Drill.wav"};
    LoadedSound error{"Error.wav"};
    LoadedSound laser{"Laser.wav"};
    LoadedSound bounce{"Bounce.wav"};
    LoadedSound menuSelect{"MenuSelect.ogg"};
    LoadedSound menuConfirm{"MenuConfirm.ogg"};
    LoadedSound menuReroll{"MenuReroll.ogg"};
    LoadedSound armorHit{"ArmorHit.ogg"};
    LoadedSound armorBreak{"ArmorBreak.ogg"};
    LoadedSound armorGuard{"ArmorGuard.ogg"};

    ////////////////////////////////////////////////////////////
    explicit Sounds()
    {
        single.settings.volume = 0.5f;
        exp.settings.volume    = 0.5f;
        place.settings.volume  = 0.4f;
    }
};


////////////////////////////////////////////////////////////
struct EveryNCounter
{
    za::SizeT requiredCount;
    za::SizeT currentCount = 0u;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool incrementAndCheck(const za::SizeT n)
    {
        currentCount += n;

        if (currentCount < requiredCount)
            return false;

        currentCount = 0u;
        return true;
    }
};


////////////////////////////////////////////////////////////
struct TriggerTetraminoPlaced
{
    za::Optional<TetraminoType> requiredType;

    EveryNCounter counter;
};


////////////////////////////////////////////////////////////
struct TriggerLinesCleared
{
    EveryNCounter counter;

    // TODO: also detect doubles/triples? or different trigger
};


////////////////////////////////////////////////////////////
struct TriggerDrillHit
{
    EveryNCounter counter;
};


////////////////////////////////////////////////////////////
struct TriggerLaserHit
{
    EveryNCounter counter;
};


////////////////////////////////////////////////////////////
struct TriggerLightningHit
{
    EveryNCounter counter;
};


////////////////////////////////////////////////////////////
struct TriggerBlockDamaged
{
    EveryNCounter counter;
};


////////////////////////////////////////////////////////////
struct TriggerPowerUpCollected
{
    EveryNCounter counter;
};


////////////////////////////////////////////////////////////
using DynamicPerkTrigger = za::Variant< //
    TriggerTetraminoPlaced,
    TriggerLinesCleared,
    TriggerDrillHit,
    TriggerLaserHit,
    TriggerLightningHit,
    TriggerBlockDamaged,
    TriggerPowerUpCollected>;


////////////////////////////////////////////////////////////
struct EffectGainXP
{
    za::U64 amount;
    // TODO: effect spawn position
};


////////////////////////////////////////////////////////////
using DynamicPerkEffect = za::Variant< //
    EffectGainXP>;


////////////////////////////////////////////////////////////
struct DynamicPerk
{
    DynamicPerkTrigger            trigger;
    za::Vector<DynamicPerkEffect> effects;

    za::Vector<DynamicPerkEffect> pendingEffects{};

    ////////////////////////////////////////////////////////////
    void onTetraminoPlaced(const Tetramino& tetramino)
    {
        auto* t = trigger.getIf<TriggerTetraminoPlaced>();

        if (t == nullptr)
            return;

        if (t->requiredType.hasValue() && tetramino.tetraminoType != t->requiredType.value())
            return;

        if (!t->counter.incrementAndCheck(1u))
            return;

        apply();
    }

    ////////////////////////////////////////////////////////////
    void onLinesCleared(const za::SizeT nLinesCleared)
    {
        auto* t = trigger.getIf<TriggerLinesCleared>();

        if (t == nullptr)
            return;

        if (!t->counter.incrementAndCheck(nLinesCleared))
            return;

        apply();
    }

    ////////////////////////////////////////////////////////////
    void onDrillHit()
    {
        auto* t = trigger.getIf<TriggerDrillHit>();

        if (t == nullptr)
            return;

        if (!t->counter.incrementAndCheck(1u))
            return;

        apply();
    }

    ////////////////////////////////////////////////////////////
    void onLaserHit()
    {
        auto* t = trigger.getIf<TriggerLaserHit>();

        if (t == nullptr)
            return;

        if (!t->counter.incrementAndCheck(1u))
            return;

        apply();
    }

    ////////////////////////////////////////////////////////////
    void onLightningHit()
    {
        auto* t = trigger.getIf<TriggerLightningHit>();

        if (t == nullptr)
            return;

        if (!t->counter.incrementAndCheck(1u))
            return;

        apply();
    }

    ////////////////////////////////////////////////////////////
    void onBlockDamaged([[maybe_unused]] const za::Vec2uz position, [[maybe_unused]] Block& block)
    {
        auto* t = trigger.getIf<TriggerBlockDamaged>();

        if (t == nullptr)
            return;

        if (!t->counter.incrementAndCheck(1u))
            return;

        apply();
    }

    ////////////////////////////////////////////////////////////
    void onPowerupCollected([[maybe_unused]] Block& block)
    {
        auto* t = trigger.getIf<TriggerPowerUpCollected>();

        if (t == nullptr)
            return;

        if (!t->counter.incrementAndCheck(1u))
            return;

        apply();
    }

    ////////////////////////////////////////////////////////////
    void apply()
    {
        pendingEffects.emplaceRange(effects.begin(), effects.size());
    }
};


////////////////////////////////////////////////////////////
enum class TSpinType : za::U8
{
    None,
    Mini,
    Full
};


////////////////////////////////////////////////////////////
// Detect T-spin after a T-piece locks.
// Standard 3-corner rule: the T-piece center is always at local (1,1).
// Check the 4 diagonal corners around the center on the grid.
// A corner counts as "occupied" if it's out of bounds or contains a block.
// Full T-spin: 3+ corners occupied.
// Mini T-spin: exactly 2 corners occupied (and last move was rotation).
[[nodiscard]] inline TSpinType detectTSpin(const BlockGrid& grid, const Tetramino& tetramino, const bool lastMoveWasRotation)
{
    if (tetramino.tetraminoType != TetraminoType::T)
        return TSpinType::None;

    if (!lastMoveWasRotation)
        return TSpinType::None;

    // T-piece center is always at local position (1,1) in the 4x4 shape matrix
    const za::Vec2i center = tetramino.position + za::Vec2i{1, 1};

    const auto isOccupied = [&](const za::Vec2i pos) -> bool { return !grid.isInBounds(pos) || grid.at(pos).hasValue(); };

    int occupiedCorners = 0;
    occupiedCorners += isOccupied(center + za::Vec2i{-1, -1}) ? 1 : 0;
    occupiedCorners += isOccupied(center + za::Vec2i{+1, -1}) ? 1 : 0;
    occupiedCorners += isOccupied(center + za::Vec2i{-1, +1}) ? 1 : 0;
    occupiedCorners += isOccupied(center + za::Vec2i{+1, +1}) ? 1 : 0;

    if (occupiedCorners >= 3)
        return TSpinType::Full;

    if (occupiedCorners == 2)
        return TSpinType::Mini;

    return TSpinType::None;
}


////////////////////////////////////////////////////////////
class Game
{
private:
    ////////////////////////////////////////////////////////////
    za::RenderWindow m_window = makeDPIScaledRenderWindow(
                                    {
                                        .size           = resolution.toVec2u(),
                                        .title          = "Tetris Survivors",
                                        .resizable      = true,
                                        .vsync          = false,
                                        .frametimeLimit = 144u,
                                    })
                                    .value();

    ////////////////////////////////////////////////////////////
    za::View m_worldView  = za::View::fromScreenSize(resolution);
    za::View m_windowView = m_window.computeView();

    ////////////////////////////////////////////////////////////
    za::Shader m_shader{[]
    {
        auto result = za::Shader::loadFromFile({
                                                   .vertexPath   = "resources/shader.vert",
                                                   .fragmentPath = "resources/shader.frag",
                                               })
                          .value();
        result.setUniform(result.getUniformLocation("za_u_texture").value(), za::Shader::CurrentTexture);
        return result;
    }()};

    ////////////////////////////////////////////////////////////
    za::Shader m_shaderCRT{[]
    {
        auto result = za::Shader::loadFromFile({.fragmentPath = "resources/crt.frag"}).value();
        result.setUniform(result.getUniformLocation("za_u_texture").value(), za::Shader::CurrentTexture);
        return result;
    }()};

    bool m_useCRTShader = true;

    za::Shader::UniformLocation m_ulInputSize        = m_shaderCRT.getUniformLocation("u_inputSize").value();
    za::Shader::UniformLocation m_ulTime             = m_shaderCRT.getUniformLocation("u_time").value();
    za::Shader::UniformLocation m_ulCurvature        = m_shaderCRT.getUniformLocation("u_curvature").value();
    za::Shader::UniformLocation m_ulVignetteStrength = m_shaderCRT.getUniformLocation("u_vignetteStrength").value();
    za::Shader::UniformLocation m_ulVignetteInnerRadius = m_shaderCRT.getUniformLocation("u_vignetteInnerRadius").value();
    za::Shader::UniformLocation m_ulVignetteOuterRadius = m_shaderCRT.getUniformLocation("u_vignetteOuterRadius").value();
    za::Shader::UniformLocation m_ulScanlineStrength = m_shaderCRT.getUniformLocation("u_scanlineStrength").value();
    za::Shader::UniformLocation
        m_ulScanlineBrightnessModulation = m_shaderCRT.getUniformLocation("u_scanlineBrightnessModulation").value();
    za::Shader::UniformLocation m_ulScanlineScrollSpeed = m_shaderCRT.getUniformLocation("u_scanlineScrollSpeed").value();
    za::Shader::UniformLocation m_ulScanlineThickness = m_shaderCRT.getUniformLocation("u_scanlineThickness").value();
    za::Shader::UniformLocation m_ulScanlineHeight    = m_shaderCRT.getUniformLocation("u_scanlineHeight").value();
    za::Shader::UniformLocation m_ulNoiseStrength     = m_shaderCRT.getUniformLocation("u_noiseStrength").value();
    za::Shader::UniformLocation m_ulMaskStrength      = m_shaderCRT.getUniformLocation("u_maskStrength").value();
    za::Shader::UniformLocation m_ulMaskScale         = m_shaderCRT.getUniformLocation("u_maskScale").value();
    za::Shader::UniformLocation m_ulBloomStrength     = m_shaderCRT.getUniformLocation("u_bloomStrength").value();
    za::Shader::UniformLocation m_ulInputGamma        = m_shaderCRT.getUniformLocation("u_inputGamma").value();
    za::Shader::UniformLocation m_ulOutputGamma       = m_shaderCRT.getUniformLocation("u_outputGamma").value();
    za::Shader::UniformLocation m_ulSaturation        = m_shaderCRT.getUniformLocation("u_saturation").value();

    float m_fUlCurvature                    = -0.015f;
    float m_fUlVignetteStrength             = 0.2f;
    float m_fUlVignetteInnerRadius          = 0.5f;
    float m_fUlVignetteOuterRadius          = 1.f;
    float m_fUlScanlineStrength             = 0.25f;
    float m_fUlScanlineBrightnessModulation = 1.2f;
    float m_fUlScanlineScrollSpeed          = 1.f;
    float m_fUlScanlineThickness            = 4.f;
    float m_fUlScanlineHeight               = 2.f;
    float m_fUlNoiseStrength                = 2.5f;
    float m_fUlMaskStrength                 = 0.15f;
    float m_fUlMaskScale                    = 0.25f;
    float m_fUlBloomStrength                = 0.1f;
    float m_fUlInputGamma                   = 2.3f;
    float m_fUlOutputGamma                  = 2.6f;
    float m_fUlSaturation                   = 1.15f;

    bool m_mustSyncShaderUniforms = true;

    ////////////////////////////////////////////////////////////
    // Shader with post-processing effects
    za::Shader m_shaderPostProcess{[]
    {
        auto result = za::Shader::loadFromFile({.fragmentPath = "resources/postprocess.frag"}).value();
        result.setUniform(result.getUniformLocation("za_u_texture").value(), za::Shader::CurrentTexture);
        return result;
    }()};

    za::Shader::UniformLocation m_ulPPVibrance   = m_shaderPostProcess.getUniformLocation("u_vibrance").value();
    za::Shader::UniformLocation m_ulPPSaturation = m_shaderPostProcess.getUniformLocation("u_saturation").value();
    za::Shader::UniformLocation m_ulPPLightness  = m_shaderPostProcess.getUniformLocation("u_lightness").value();
    za::Shader::UniformLocation m_ulPPSharpness  = m_shaderPostProcess.getUniformLocation("u_sharpness").value();

    float m_fUlPPVibrance   = 0.1f;
    float m_fUlPPSaturation = 0.85f;
    float m_fUlPPLightness  = 1.f;
    float m_fUlPPSharpness  = 0.1f;

    ////////////////////////////////////////////////////////////
    za::Font m_font      = za::Font::openFromFile("resources/monogram.ttf").value();
    za::Font m_fontMago2 = za::Font::openFromFile("resources/petty5.bdf").value();
    za::Font m_font3     = za::Font::openFromFile("resources/ChiKareGo.ttf").value();
    za::Font m_font4     = za::Font::openFromFile("resources/TinyUnicode.ttf").value();

    ////////////////////////////////////////////////////////////
    za::PlaybackDevice m_playbackDevice{za::AudioContext::getDefaultPlaybackDeviceHandle().value()};

    ////////////////////////////////////////////////////////////
    Sounds       m_sounds;
    SoundManager m_soundManager;

    ////////////////////////////////////////////////////////////
    void playSound(const LoadedSound& ls, const za::SizeT maxOverlap = 255u)
    {
        (void)m_playbackDevice.applyListener({.volume = 0.5f});
        m_soundManager.playPooled(m_playbackDevice, ls, maxOverlap);
    }

    ////////////////////////////////////////////////////////////
    za::ImGuiContext m_imGuiContext;
    ImFont* const    m_imguiFont{ImGui::GetIO().Fonts->AddFontFromFileTTF("resources/monogram.ttf", 16.f)};
    ImFont* const    m_imguiFontBig{ImGui::GetIO().Fonts->AddFontFromFileTTF("resources/BoldPixels.ttf", 16.f)};

    ////////////////////////////////////////////////////////////
    za::Clock m_tickClock;
    float     m_timeAccumulator = 0.f;
    float     m_totalTime       = 0.f;

    ////////////////////////////////////////////////////////////
    World m_world;

    ////////////////////////////////////////////////////////////
    za::Vec2f                             m_currentTetraminoVisualCenter;
    AnimationTimeline<AnimationCommandP0> m_animationTimelineP0;
    AnimationTimeline<AnimationCommandP1> m_animationTimelineP1;
    AnimationTimeline<AnimationCommandP2> m_animationTimelineP2;
    za::Vector<float>                     m_rowYOffsets;

    ////////////////////////////////////////////////////////////
    bool      m_inLevelUpScreen      = false;
    za::SizeT m_selectedPerk         = 0u;
    float     m_menuDelayProgress    = 0.f; // delay before accepting input in menus
    za::SizeT m_rerollsLeftThisLevel = 0u;

    ////////////////////////////////////////////////////////////
    PerkChainLightning                  m_perkChainLightning;
    PerkPeekNextTetraminos              m_perkPeekNextTetraminos;
    PerkOnClearLightningStrike          m_perkOnClearLightningStrike;
    PerkVerticalDrillUnlock             m_perkVerticalDrillUnlock;
    PerkVerticalDrillPenetration        m_perkVerticalDrillPenetration;
    PerkVerticalDrillBluntForce         m_perkVerticalDrillBluntForce;
    PerkHoldSkipTetramino               m_perkHoldSkipTetramino;
    PerkXpPerTetraminoPlaced            m_perkXpPerTetraminoPlaced;
    PerkXpPerBlockDamaged               m_perkXpPerBlockDamaged;
    PerkDeleteFloorPerNTetraminos       m_perkDeleteFloorPerNTetraminos;
    PerkRndHitPerNTetraminos            m_perkRndHitPerNTetraminos;
    PerkExtraLinePieces                 m_perkExtraLinePieces;
    PerkHorizontalDrillLeftUnlock       m_perkHorizontalDrillLeftUnlock;
    PerkHorizontalDrillRightUnlock      m_perkHorizontalDrillRightUnlock;
    PerkHorizontalDrillLeftPenetration  m_perkHorizontalDrillLeftPenetration;
    PerkHorizontalDrillLeftCoverage     m_perkHorizontalDrillLeftCoverage;
    PerkHorizontalDrillRightPenetration m_perkHorizontalDrillRightPenetration;
    PerkHorizontalDrillRightCoverage    m_perkHorizontalDrillRightCoverage;
    PerkDiagonalLaserLeftUnlock         m_perkDiagonalLaserLeftUnlock;
    PerkDiagonalLaserRightUnlock        m_perkDiagonalLaserRightUnlock;
    PerkDiagonalLaserLeftPenetration    m_perkDiagonalLaserLeftPenetration;
    PerkDiagonalLaserRightPenetration   m_perkDiagonalLaserRightPenetration;
    PerkDiagonalLaserLeftBounce         m_perkDiagonalLaserLeftBounce;
    PerkDiagonalLaserRightBounce        m_perkDiagonalLaserRightBounce;

    ////////////////////////////////////////////////////////////
    za::Vector<const Perk*> m_perks{
        &m_perkChainLightning,
        &m_perkPeekNextTetraminos,
        &m_perkOnClearLightningStrike,
        &m_perkVerticalDrillUnlock,
        &m_perkVerticalDrillPenetration,
        &m_perkVerticalDrillBluntForce,
        &m_perkHoldSkipTetramino,
        &m_perkXpPerTetraminoPlaced,
        &m_perkXpPerBlockDamaged,
        &m_perkDeleteFloorPerNTetraminos,
        &m_perkRndHitPerNTetraminos,
        &m_perkExtraLinePieces,
        &m_perkHorizontalDrillLeftUnlock,
        &m_perkHorizontalDrillRightUnlock,
        &m_perkHorizontalDrillLeftPenetration,
        &m_perkHorizontalDrillLeftCoverage,
        &m_perkHorizontalDrillRightPenetration,
        &m_perkHorizontalDrillRightCoverage,
        &m_perkDiagonalLaserLeftUnlock,
        &m_perkDiagonalLaserRightUnlock,
        &m_perkDiagonalLaserLeftPenetration,
        &m_perkDiagonalLaserRightPenetration,
        &m_perkDiagonalLaserLeftBounce,
        &m_perkDiagonalLaserRightBounce,
    };

    za::Vector<za::SizeT> m_perkIndicesSelectedThisLevel;

    ////////////////////////////////////////////////////////////
    za::Vector<DynamicPerk> m_dynamicPerks; // TODO: to world

    ////////////////////////////////////////////////////////////
    za::Vector<LightningBolt> m_lightningBolts;

    ////////////////////////////////////////////////////////////
    QuakeSinEffect m_quakeSinEffectLineClear;
    QuakeSinEffect m_quakeSinEffectHardDrop;

    ////////////////////////////////////////////////////////////
    za::Optional<LaserBeam> m_optLaserBeam;

    ////////////////////////////////////////////////////////////
    // Screen shake effect state
    float m_screenShakeAmount{0.f};
    float m_screenShakeTimer{0.f};

    ////////////////////////////////////////////////////////////
    struct [[nodiscard]] BlockEffect
    {
        static constexpr float squishDuration = 0.2f;
        float                  squishTime     = 0.f;
    };

    ankerl::unordered_dense::map<BlockId, BlockEffect> m_blockEffects;

    ////////////////////////////////////////////////////////////
    za::Vector<EarnedXPParticle>   m_earnedXPParticles;
    za::Vector<CircleParticleData> m_hueColorCircleShapeParticles;
    za::Vector<CircleParticleData> m_fixedColorCircleShapeParticles;
    za::Vector<SpriteParticleData> m_fixedColorSpriteParticles;

    ////////////////////////////////////////////////////////////
    RNGFast m_rngFast{static_cast<RNGFast::SeedType>(
        za::Clock::now().asMicroseconds())}; // very fast, low-quality, but good enough for VFXs

    ////////////////////////////////////////////////////////////
    za::TextureAtlas m_textureAtlas{za::Texture::create({512u, 512u}, {.smooth = false}).value()};

    ////////////////////////////////////////////////////////////
    const za::Rect2f m_txrWhiteDotTrue   = m_textureAtlas.add(za::GraphicsContext::getBuiltInWhiteDotTexture()).value();
    const za::Rect2f m_txrWhiteDot       = {{0.f, 0.f}, {1.f, 1.f}};
    const za::Rect2f m_txrBlock0         = addImgResourceToAtlas("block0.png");
    const za::Rect2f m_txrBlock1         = addImgResourceToAtlas("block1.png");
    const za::Rect2f m_txrBlock2         = addImgResourceToAtlas("block2.png");
    const za::Rect2f m_txrBlock3         = addImgResourceToAtlas("block3.png");
    const za::Rect2f m_txrBlock4         = addImgResourceToAtlas("block4.png");
    const za::Rect2f m_txrBlock5         = addImgResourceToAtlas("block5.png");
    const za::Rect2f m_txrBlock6         = addImgResourceToAtlas("block6.png");
    const za::Rect2f m_txrDivider        = addImgResourceToAtlas("divider.png");
    const za::Rect2f m_txrDrill          = addImgResourceToAtlas("drill.png");
    const za::Rect2f m_txrRedDot         = addImgResourceToAtlas("reddot.png");
    const za::Rect2f m_txrEmitter        = addImgResourceToAtlas("emitter.png");
    const za::Rect2f m_txrPowerupXP      = addImgResourceToAtlas("powerupxp.png");
    const za::Rect2f m_txrPowerupBomb    = addImgResourceToAtlas("powerupbomb.png");
    const za::Rect2f m_txrBFMinogram6x10 = addImgResourceToAtlas("minogram_6x10.png");
    const za::Rect2f m_txrBFTiny5        = addImgResourceToAtlas("tiny5.png");
    const za::Rect2f m_txrBlockHalf0     = addImgResourceToAtlas("blockhalf0.png");
    const za::Rect2f m_txrBlockHalf1     = addImgResourceToAtlas("blockhalf1.png");

    ////////////////////////////////////////////////////////////
    BitmapFont m_bitmapFontMinogram{loadMinogramData()};
    BitmapFont m_bitmapFontTiny5{loadTiny5Data()};


    ////////////////////////////////////////////////////////////
    za::RenderTexture m_rtGame{za::RenderTexture::create(resolution.toVec2u(), {.smooth = false}).value()};
    za::RenderTexture m_rtPostProcess{za::RenderTexture::create(resolution.toVec2u(), {.smooth = true}).value()};


    ////////////////////////////////////////////////////////////
    za::Vector<za::Vertex>    m_textVerticesBuffer;
    za::Vector<za::IndexType> m_textIndicesBuffer;


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline constexpr za::CircleShapeData particleToCircleData(const CircleParticleData& particle)
    {
        ZA_ASSERT(particle.opacity >= 0.f && particle.opacity <= 1.f);
        const auto opacityAsAlpha = static_cast<za::U8>(particle.opacity * 255.f);

        return {
            .position    = particle.position.componentWiseFloor(),
            .scale       = {particle.scale, particle.scale},
            .rotation    = za::radians(particle.rotation),
            .textureRect = m_txrRedDot, // No texture
            .fillColor   = particle.color.withAlpha(opacityAsAlpha),
            .radius      = particle.radius,
            .pointCount  = particle.pointCount,
        };
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] inline constexpr za::Sprite particleToSpriteData(const SpriteParticleData& particle)
    {
        ZA_ASSERT(particle.opacity >= 0.f && particle.opacity <= 1.f);
        const auto opacityAsAlpha = static_cast<za::U8>(particle.opacity * 255.f);

        return {
            .position    = particle.position.componentWiseFloor(),
            .scale       = {particle.scale, particle.scale},
            .origin      = particle.origin.componentWiseFloor(),
            .rotation    = za::radians(particle.rotation),
            .textureRect = particle.textureRect,
            .color       = za::Color::whiteWithAlpha(opacityAsAlpha),
        };
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Rect2f addImgResourceToAtlas(const za::Path& path)
    {
        return m_textureAtlas.add(za::Image::loadFromFile("resources" / path).value(), /* padding */ {2u, 2u}).value();
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] int calculateGhostY(const Tetramino& tetramino) const
    {
        za::Optional<Tetramino> finalHardDropState;

        if (const auto* squish = m_animationTimelineP1.getIfPlaying<AnimSquish>())
            finalHardDropState.emplace(squish->tetramino);

        int ghostY = tetramino.position.y;

        while (true)
        {
            const za::Vec2i nextGhostPos = {tetramino.position.x, ghostY + 1};

            if (!m_world.blockGrid.isValidMove(tetramino.shape, nextGhostPos))
                break;

            if (finalHardDropState.hasValue() &&
                blockMatricesIntersect(tetramino.shape, nextGhostPos, finalHardDropState->shape, finalHardDropState->position))
                break;

            ++ghostY;
        }

        return ghostY;
    }


    ////////////////////////////////////////////////////////////
    static inline constexpr za::Vec2f drawBlockSize{11.f, 11.f};
    static inline constexpr za::Vec2f drawOffset{drawBlockSize.x + 3.f, drawBlockSize.y - drawBlockSize.y * gridGraceY + 3.f};


    ////////////////////////////////////////////////////////////
    template <typename T>
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] za::Vec2f toDrawCoordinates(const za::Vec2<T> position) const noexcept
    {
        return (drawOffset + position.toVec2f().componentWiseMul(drawBlockSize)).componentWiseFloor();
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] za::Vec2i toGridCoordinates(const za::Vec2f drawPosition) const noexcept
    {
        return (drawPosition - drawOffset).componentWiseDiv(drawBlockSize).toVec2i();
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] za::Color hueColorFromPaletteIdxRotated(
        const PaletteIdx paletteIdx,
        const za::U8     alpha,
        const float      degrees)
    {
        const auto hue = za::positiveRemainder(blockPalette[paletteIdx].toHSL().hue + degrees, 360.f);
        return hueColor(hue, 255u).withAlpha(alpha);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::const]] za::Color hueColorFromPaletteIdx(const PaletteIdx paletteIdx,
                                                                                                 const za::U8 alpha)
    {
        return hueColorFromPaletteIdxRotated(paletteIdx, alpha, 0.f);
    }


    ////////////////////////////////////////////////////////////
    za::Rect2f drawBlock(const Block& block, const za::Vec2f position, const DrawBlockOptions& options = {})
    {
        const float timerProgress = block.getTimerProgress();

        za::Vec2f mins{9999.f, 9999.f};
        za::Vec2f maxs{-9999.f, -9999.f};

        float yOffset = 0.f;

        if (options.applyYOffset && !m_rowYOffsets.empty())
        {
            const float progress = m_animationTimelineP1.isPlaying<AnimCollapseGrid>()
                                       ? m_animationTimelineP1.getProgress()
                                       : 0.f;

            const auto gridPos = toGridCoordinates(position);

            if (gridPos.y >= 0 && static_cast<za::SizeT>(gridPos.y) < m_rowYOffsets.size())
            {
                const float initialOffset = m_rowYOffsets[static_cast<za::SizeT>(gridPos.y)];
                yOffset += initialOffset * (1.f - easeInBack(progress));
            }
        }

        if (options.applyQuakeOffset)
        {
            yOffset += -(m_quakeSinEffectHardDrop.getValue() + m_quakeSinEffectLineClear.getValue());
        }

        const auto alpha = static_cast<za::U8>(options.opacity * 255.f);

        float finalSquishMult = 1.f + options.squishMult;
        if (const auto* it = m_blockEffects.find(block.blockId); it != m_blockEffects.end())
        {
            const BlockEffect& effect = it->second;

            const float progress = effect.squishTime / BlockEffect::squishDuration;
            finalSquishMult += easeInOutSine(bounce(progress)) * 0.5f;
        }

        const za::DrawTextureSettings commonDrawParams{
            .position = position.addY(yOffset).componentWiseFloor().addX(1.f).addY(1.f),
            .scale    = za::Vec2f{finalSquishMult, finalSquishMult} * options.scale,
            .origin   = (drawBlockSize / 2.f).componentWiseFloor(),
            .color    = hueColorFromPaletteIdx(block.paletteIdx, alpha),
        };

        {
            const auto transform = za::Transform::fromPositionScaleOriginRotation(commonDrawParams.position,
                                                                                  commonDrawParams.scale,
                                                                                  commonDrawParams.origin,
                                                                                  commonDrawParams.rotation);

            mins = transform.transformPoint({0.f, 0.f});
            maxs = transform.transformPoint(drawBlockSize);
        }

        const float shakeAmount = easeInOutSine(timerProgress) * 0.75f;
        const auto  shake       = m_rngFast.getVec2f({-shakeAmount, -shakeAmount}, {shakeAmount, shakeAmount});

        const bool hasTimer = block.tickTimerTarget != nullTickTimerTarget;

        // const bool doesNotHaveTimerOrIsPowerup = !hasTimer || block.powerup != BlockPowerup::None;
        // const bool useNormalTexture  = block.health == 1u && !doesNotHaveTimerOrIsPowerup;

        const bool useDamagedTexture = block.health == 1u && hasTimer && block.powerup == BlockPowerup::None;
        const bool useArmoredTexture = block.health > 1u;

        const auto txr = useArmoredTexture ? m_txrBlock4 : useDamagedTexture ? m_txrBlock6 : m_txrBlock1;

        m_rtGame.draw(m_textureAtlas.getTexture(),
                      {
                          .position    = commonDrawParams.position + shake,
                          .scale       = commonDrawParams.scale,
                          .origin      = commonDrawParams.origin,
                          .rotation    = za::degrees(options.rotation),
                          .textureRect = txr,
                          .color       = commonDrawParams.color,
                      },
                      {.view = m_worldView, .shader = &m_shader});

        if (block.powerup != BlockPowerup::None && options.drawText)
        {
            if (block.powerup == BlockPowerup::XPBonus)
            {
                m_rtGame.draw(m_textureAtlas.getTexture(),
                              {
                                  .position    = commonDrawParams.position,
                                  .scale       = commonDrawParams.scale,
                                  .origin      = commonDrawParams.origin,
                                  .textureRect = m_txrPowerupXP,
                                  .color       = commonDrawParams.color,
                              },
                              {.view = m_worldView, .shader = &m_shader});
            }
            else if (block.powerup == BlockPowerup::ColumnDrill)
            {
                // TODO
            }
            else if (block.powerup == BlockPowerup::ThreeRowDrill)
            {
                m_rtGame.draw(m_textureAtlas.getTexture(),
                              {
                                  .position    = commonDrawParams.position,
                                  .scale       = commonDrawParams.scale,
                                  .origin      = commonDrawParams.origin,
                                  .textureRect = m_txrPowerupBomb,
                                  .color       = commonDrawParams.color,
                              },
                              {.view = m_worldView, .shader = &m_shader});
            }
        }

        if (block.health > 1u && options.drawText)
        {
            za::Text text{m_fontMago2,
                          {
                              .origin        = (drawBlockSize / 2.f).componentWiseFloor(),
                              .string        = za::toString(static_cast<unsigned int>(block.health - 1u)),
                              .characterSize = 5u,
                              .fillColor     = za::Color::blackWithAlpha(alpha),
                          }};

            text.setGlobalCenter((position.addY(yOffset) + za::Vec2f{2.f, 3.f}).componentWiseFloor());
            m_rtGame.draw(text, {.view = m_worldView});

            text.setFillColor(za::Color::whiteWithAlpha(alpha));

            text.setGlobalCenter(position.addY(yOffset).componentWiseFloor() + za::Vec2f{2.f, 2.f});
            m_rtGame.draw(text, {.view = m_worldView});
        }

        if (block.tickTimerTarget != nullTickTimerTarget && options.drawTimer)
        {
            const bool timerDirection = block.powerup == BlockPowerup::None;

            const float progress0 = za::clamp(timerProgress * 4.f, 0.f, 1.f);
            const float progress1 = za::clamp((timerProgress - 0.25f) * 4.f, 0.f, 1.f);
            const float progress2 = za::clamp((timerProgress - 0.5f) * 4.f, 0.f, 1.f);
            const float progress3 = za::clamp((timerProgress - 0.75f) * 4.f, 0.f, 1.f);

            const float p0 = timerDirection ? progress0 : (1.f - progress3);
            const float p1 = timerDirection ? progress1 : (1.f - progress2);
            const float p2 = timerDirection ? progress2 : (1.f - progress1);
            const float p3 = timerDirection ? progress3 : (1.f - progress0);

            za::RectangleShapeData timerLine{
                .scale       = commonDrawParams.scale,
                .origin      = commonDrawParams.origin,
                .textureRect = m_txrWhiteDot,
                .fillColor   = za::Color::White,
            };

            const auto drawTimerLines = [&](const float embed)
            {
                const auto drawCtx = m_rtGame.withLockedRenderStates(
                    {.view = m_worldView, .texture = &m_textureAtlas.getTexture(), .shader = &m_shader});

                // top line
                timerLine.position = commonDrawParams.position + za::Vec2f{embed, embed};
                timerLine.rotation = za::degrees(0.f);
                timerLine.size     = {(drawBlockSize.x - embed * 2.f) * p0, 1.f};
                drawCtx.draw(timerLine);

                // right line
                timerLine.position = commonDrawParams.position + za::Vec2f{drawBlockSize.x - embed - 1.f, embed};
                timerLine.rotation = za::degrees(0.f);
                timerLine.size     = {1.f, (drawBlockSize.y - embed * 2.f) * p1};
                drawCtx.draw(timerLine);

                // bottom line
                timerLine.position = commonDrawParams.position + za::Vec2f{1.f - embed, 1.f - embed};
                timerLine.rotation = za::degrees(180.f);
                timerLine.size     = {(drawBlockSize.x - embed * 2.f) * p2, 1.f};
                drawCtx.draw(timerLine);

                // left line
                timerLine.position = commonDrawParams.position + za::Vec2f{-drawBlockSize.x + embed + 2.f, 1.f - embed};
                timerLine.rotation = za::degrees(180.f);
                timerLine.size     = {1.f, (drawBlockSize.y - embed * 2.f) * p3};
                drawCtx.draw(timerLine);
            };

            if (useDamagedTexture)
            {
                timerLine.textureRect = m_txrWhiteDot;

                timerLine.fillColor = za::Color::White;
                drawTimerLines(0.f);

                timerLine.fillColor = za::Color::DarkGray;
                drawTimerLines(1.f);
            }
            else
            {
                timerLine.textureRect = m_txrRedDot;
                timerLine.fillColor   = useDamagedTexture ? za::Color::White
                                                          : hueColorFromPaletteIdxRotated(block.paletteIdx, 255u, 90.f);

                drawTimerLines(0.f);
            }
        }

        return {mins, maxs - mins};
    }


    ////////////////////////////////////////////////////////////
    za::Rect2f drawTetramino(const BlockMatrix& shape, const za::Vec2f centerPosition, const DrawBlockOptions& options = {})
    {
        za::Vec2f mins{9999.f, 9999.f};
        za::Vec2f maxs{-9999.f, -9999.f};

        const float     scale    = options.scale;
        constexpr float rotation = 0.f;

        // Define the pivot point in the tetramino's local, unscaled coordinate space.
        // This is the center of the 4x4 grid.
        const za::Vec2f localPivot = ((drawBlockSize * static_cast<float>(shapeDimension)) / 2.f).componentWiseFloor();

        for (za::SizeT y = 0u; y < shapeDimension; ++y)
            for (za::SizeT x = 0u; x < shapeDimension; ++x)
            {
                const za::Optional<Block>& optBlock = shape.at(x, y);

                if (!optBlock.hasValue())
                    continue;

                // Calculate this block's local center position, relative to the top-left corner
                const za::Vec2f localBlockCenter = za::Vec2uz{x, y}.toVec2f().componentWiseMul(drawBlockSize) +
                                                   (drawBlockSize / 2.f).componentWiseFloor();

                // Get the block's position vector relative to the central pivot
                za::Vec2f positionRelativeToPivot = localBlockCenter - localPivot;

                // Scale and rotate this relative vector
                positionRelativeToPivot = positionRelativeToPivot.componentWiseMul({scale, scale});
                positionRelativeToPivot = positionRelativeToPivot.rotatedBy(za::degrees(rotation));

                // The final screen position is the tetramino's center plus the transformed relative vector
                const za::Vec2f finalDrawPosition = (centerPosition + positionRelativeToPivot).componentWiseFloor();

                const auto [pos,
                            size] = drawBlock(*optBlock,
                                              finalDrawPosition,
                                              {
                                                  .opacity    = options.opacity,
                                                  .squishMult = options.squishMult,
                                                  .rotation = rotation, // Pass rotation for the block's own orientation
                                                  .scale    = scale,
                                                  .drawText = options.drawText,
                                                  .applyYOffset     = false, // Usually false for UI elements
                                                  .applyQuakeOffset = options.applyQuakeOffset,
                                                  .drawTimer        = options.drawTimer,
                                              });

                mins.x = za::min(mins.x, pos.x);
                mins.y = za::min(mins.y, pos.y);
                maxs.x = za::max(maxs.x, pos.x + size.x);
                maxs.y = za::max(maxs.y, pos.y + size.y);
            }

        return {mins, maxs - mins};
    }


    ////////////////////////////////////////////////////////////
    void moveTetramino(Tetramino& tetramino, const za::Vec2i delta)
    {
        const auto newPosition = tetramino.position + delta;

        if (m_world.blockGrid.isValidMove(tetramino.shape, newPosition))
        {
            tetramino.position          = newPosition;
            m_world.lastMoveWasRotation = false;
        }
    }


    ////////////////////////////////////////////////////////////
    void rotateTetramino(Tetramino& tetramino, const bool clockwise)
    {
        const auto nextRotationState = static_cast<RotationState>((tetramino.rotationState + (clockwise ? 1u : 3u)) % 4u);

        const auto& targetShapeTemplate = srsTetraminoShapes[static_cast<za::SizeT>(tetramino.tetraminoType)][nextRotationState];

        // Create the new stateful shape by re-mapping the blocks
        const BlockMatrix rotatedShape = mapBlocksToNewShape(tetramino, targetShapeTemplate);

        const auto& kickTable = (tetramino.tetraminoType == TetraminoType::I) ? kickDataI : kickDataJLSTZ;

        int kickTableIndex = clockwise ? tetramino.rotationState : nextRotationState;
        kickTableIndex     = kickTableIndex * 2 + (clockwise ? 0 : 1);

        for (const za::Vec2i offset : kickTable[static_cast<za::SizeT>(kickTableIndex)])
        {
            const za::Vec2i testPosition = tetramino.position + offset;

            if (!m_world.blockGrid.isValidMove(rotatedShape, testPosition))
                continue;

            tetramino.shape             = rotatedShape;
            tetramino.position          = testPosition;
            tetramino.rotationState     = nextRotationState;
            m_world.lastMoveWasRotation = true;

            return;
        }
    }


    ////////////////////////////////////////////////////////////
    void moveCurrentTetraminoBy(const za::Vec2i delta)
    {
        if (m_world.currentTetramino.hasValue())
            moveTetramino(*m_world.currentTetramino, delta);
    }


    ////////////////////////////////////////////////////////////
    void rotateCurrentTetramino(const bool clockwise)
    {
        if (m_world.currentTetramino.hasValue())
        {
            rotateTetramino(*m_world.currentTetramino, clockwise);
            playSound(m_sounds.rotate);
        }
    }


    ////////////////////////////////////////////////////////////
    void resetAndRedrawCurrentTetramino(const bool usedHold)
    {
        ZA_ASSERT(m_world.currentTetramino.hasValue());

        m_world.currentTetramino.reset();
        m_world.holdUsedThisTurn = usedHold;

        updateStepRefillBlockMatrixIfNeeded();
        initializeCurrentTetraminoFromBag();
    }


    ////////////////////////////////////////////////////////////
    void skipCurrentTetramino()
    {
        if (m_world.holdUsedThisTurn)
            return;

        ZA_ASSERT(m_world.currentTetramino.hasValue());

        playSound(m_sounds.hold);
        resetAndRedrawCurrentTetramino(/* usedHold */ true);
    }


    ////////////////////////////////////////////////////////////
    void holdCurrentTetramino()
    {
        if (m_world.holdUsedThisTurn)
            return;

        ZA_ASSERT(m_world.currentTetramino.hasValue());

        playSound(m_sounds.hold);

        const auto temp       = m_world.heldTetramino;
        m_world.heldTetramino = m_world.currentTetramino;

        if (temp.hasValue())
        {
            m_world.currentTetramino = temp;
            m_world.currentTetramino->position = za::Vec2uz{(m_world.blockGrid.getWidth() - shapeDimension) / 2u, 0u}.toVec2i();

            m_world.holdUsedThisTurn = true;
        }
        else
        {
            resetAndRedrawCurrentTetramino(/* usedHold */ true);
        }
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] AnimClearLines::RowVector findFullRows()
    {
        AnimClearLines::RowVector fullRows;

        for (za::SizeT y = gridGraceY; y < m_world.blockGrid.getHeight(); ++y)
        {
            bool isFull = true;

            for (za::SizeT x = 0u; x < m_world.blockGrid.getWidth(); ++x)
            {
                if (!m_world.blockGrid.at(za::Vec2uz{x, y}).hasValue())
                {
                    isFull = false;
                    break;
                }
            }

            if (isFull)
                fullRows.pushBack(y);
        }

        return fullRows;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] ShapeBlockPositionVector findDrillBlocks(const Tetramino& tetramino, const DrillDirection::Enum direction) const
    {
        if (direction == DrillDirection::Down)
        {
            auto downmostBlocksXY = findDownmostBlocks(tetramino.shape);

            if (downmostBlocksXY.size() > 1u && m_world.perkDrill[DrillDirection::Down]->coverage == 1u)
                downmostBlocksXY.clear(); // Instead of `return {}`, to enable NRVO

            return downmostBlocksXY;
        }

        return findHorizontalBlocks(tetramino.shape, static_cast<za::SizeT>(m_world.perkDrill[direction]->coverage));
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] ShapeBlockPositionVector findLaserBlocks(const Tetramino& tetramino, const LaserDirection::Enum direction) const
    {
        const auto outOfBoundsOrEmpty = [&](const za::SizeT x, const za::SizeT y) -> bool
        { return !tetramino.shape.isInBounds(x, y) || !tetramino.shape.at(x, y).hasValue(); };

        ShapeBlockPositionVector result;

        for (const auto bPos : findTopmostBlocks(tetramino.shape))
        {
            if (direction == LaserDirection::Left)
            {
                if (bPos.x == 0u) // to avoid unsigned wraparound shenanigans
                {
                    result.pushBack(bPos);
                }
                else
                {
                    const bool leftValid       = outOfBoundsOrEmpty(bPos.x - 1u, bPos.y);
                    const bool bottomLeftValid = outOfBoundsOrEmpty(bPos.x - 1u, bPos.y + 1u);

                    if (leftValid && bottomLeftValid)
                        result.pushBack(bPos);
                }
            }

            if (direction == LaserDirection::Right)
            {
                if (bPos.x == shapeDimension - 1u) // to avoid unsigned wraparound shenanigans
                {
                    result.pushBack(bPos);
                }
                else
                {
                    const bool rightValid       = outOfBoundsOrEmpty(bPos.x + 1u, bPos.y);
                    const bool bottomRightValid = outOfBoundsOrEmpty(bPos.x + 1u, bPos.y + 1u);

                    if (rightValid && bottomRightValid)
                        result.pushBack(bPos);
                }
            }
        }

        return result;
    }


    ////////////////////////////////////////////////////////////
    struct [[nodiscard]] BlockAndPosition // NOLINT(cppcoreguidelines-pro-type-member-init)
    {
        Block*     block;
        za::Vec2uz position;
    };


    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Vector<BlockAndPosition> findDrillTargetBlocks(const Tetramino&           tetramino,
                                                                     const DrillDirection::Enum direction)
    {
        za::Vector<BlockAndPosition> result;

        if (!m_world.perkDrill[direction].hasValue())
            return result;

        auto nToHit = static_cast<za::SizeT>(m_world.perkDrill[direction]->maxPenetration);
        if (nToHit == 0)
            return result;

        const za::Vec2i step = drillDirectionToVec2i(direction);

        for (const auto bPos : findDrillBlocks(tetramino, direction))
        {
            za::Vec2i probePos = tetramino.position + bPos.toVec2i() + step;

            while (m_world.blockGrid.isInBounds(probePos))
            {
                const auto blockGridPos = probePos.toVec2uz();
                auto&      optBlock     = m_world.blockGrid.at(blockGridPos);

                // If we hit empty space, the drill path is blocked.
                if (!optBlock.hasValue())
                    break;

                // If the block is damageable (health > 1), add it to our list.
                if (optBlock->isDamageable())
                {
                    result.pushBack({optBlock.asPtr(), blockGridPos});
                    if (--nToHit == 0u)
                        break; // Stop probing if we've reached the penetration limit.
                }

                // Move to the next block in the line.
                probePos += step;
            }

            // If we've found all the blocks we can hit, we can stop checking other source blocks.
            if (nToHit == 0u)
                break;
        }

        return result;
    }


    ////////////////////////////////////////////////////////////
    void handleEffects()
    {
        const auto visitor = za::OverloadSet{
            [&](const EffectGainXP& e) { addXP(e.amount); },
        };

        for (auto& dp : m_dynamicPerks)
        {
            for (auto& e : dp.pendingEffects)
                e.linearVisit(visitor);

            dp.pendingEffects.clear();
        }
    }


    ////////////////////////////////////////////////////////////
    void handleTriggerTetraminoPlaced(const Tetramino& tetramino)
    {
        for (auto& dp : m_dynamicPerks)
            dp.onTetraminoPlaced(tetramino);

        handleEffects();

        if (m_world.perkXPPerTetraminoPlaced > 0)
        {
            addXP(static_cast<za::U64>(m_world.perkXPPerTetraminoPlaced));

            const auto downmostBlocksXY = findDownmostBlocks(tetramino.shape);

            for (const auto& bPos : downmostBlocksXY)
            {
                const za::Optional<Block>& optBlock = tetramino.shape.at(bPos.x, bPos.y);
                ZA_ASSERT(optBlock.hasValue());

                spawnXPEarnedParticle(toDrawCoordinates(tetramino.position + bPos.toVec2i()), optBlock->paletteIdx);
            }
        }

        if (auto* rndHitPerNTetraminos = m_world.perkRndHitPerNTetraminos.asPtr())
        {
            ++(rndHitPerNTetraminos->tetraminosPlacedCount);

            if (rndHitPerNTetraminos->tetraminosPlacedCount >= rndHitPerNTetraminos->nTetraminos)
            {
                m_animationTimelineP0.addInstantaneous(AnimLightningStrike{.numStrikes = 1u});
                rndHitPerNTetraminos->tetraminosPlacedCount = 0;
            }
        }

        if (auto* deleteFloorNTetraminos = m_world.perkDeleteFloorPerNTetraminos.asPtr())
        {
            ++(deleteFloorNTetraminos->tetraminosPlacedCount);

            if (deleteFloorNTetraminos->tetraminosPlacedCount >= deleteFloorNTetraminos->nTetraminos)
            {
                // Delete bottom line (must happen after regular clear)
                m_animationTimelineP2.addInstantaneous(AnimClearLines{
                    .rows       = {m_world.blockGrid.getHeight() - 1u},
                    .awardXP    = false,
                    .forceClear = true,
                });

                deleteFloorNTetraminos->tetraminosPlacedCount = 0;
            }
        }
    }


    ////////////////////////////////////////////////////////////
    void handleTriggerLinesCleared(const za::SizeT nLinesCleared)
    {
        for (auto& dp : m_dynamicPerks)
            dp.onLinesCleared(nLinesCleared);

        handleEffects();

        if (m_world.perkRndHitOnClear > 0)
        {
            m_animationTimelineP0.addInstantaneous(
                AnimLightningStrike{.numStrikes = static_cast<za::SizeT>(m_world.perkRndHitOnClear)});
        }
    }


    ////////////////////////////////////////////////////////////
    void handleTriggerDrillHit()
    {
        for (auto& dp : m_dynamicPerks)
            dp.onDrillHit();

        handleEffects();
    }


    ////////////////////////////////////////////////////////////
    void handleTriggerLaserHit()
    {
        for (auto& dp : m_dynamicPerks)
            dp.onLaserHit();

        handleEffects();

        if (roll100(m_world.perkChainLightning))
            m_animationTimelineP0.addInstantaneous(AnimLightningStrike{.numStrikes = 1u});
    }


    ////////////////////////////////////////////////////////////
    void handleTriggerLightningHit()
    {
        for (auto& dp : m_dynamicPerks)
            dp.onLightningHit();

        handleEffects();

        if (roll100(m_world.perkChainLightning))
            m_animationTimelineP0.addInstantaneous(AnimLightningStrike{.numStrikes = 1u});
    }


    ////////////////////////////////////////////////////////////
    void handleTriggerBlockDamaged(const za::Vec2uz position, Block& block)
    {
        for (auto& dp : m_dynamicPerks)
            dp.onBlockDamaged(position, block);

        handleEffects();

        if (m_world.perkXPPerBlockDamaged > 0)
        {
            addXP(static_cast<za::U64>(m_world.perkXPPerBlockDamaged));
            playSound(m_sounds.exp);

            spawnXPEarnedParticle(toDrawCoordinates(position) + drawBlockSize / 2.f, block.paletteIdx);
        }
    }


    ////////////////////////////////////////////////////////////
    void handleTriggerPowerupCollected(Block& block)
    {
        for (auto& dp : m_dynamicPerks)
            dp.onPowerupCollected(block);

        handleEffects();
    }


    ////////////////////////////////////////////////////////////
    void handleTSpin(const TSpinType tSpinType, const Tetramino& tetramino)
    {
        (void)tetramino;

        if (tSpinType == TSpinType::None)
            return;

        // TODO P1: add T-spin effects here, test detection
        //
        // `tSpinType` is `TSpinType::Full` or `TSpinType::Mini`.
        // `tetramino.position` is the grid position of the locked T-piece.
    }


    ////////////////////////////////////////////////////////////
    void embedTetraminoAndClearLines(const Tetramino& tetramino)
    {
        handleTriggerTetraminoPlaced(tetramino);

        // Detect T-spin before embedding (corners must be checked against the grid
        // before the tetramino's blocks are placed into it)
        const TSpinType tSpinType = detectTSpin(m_world.blockGrid, tetramino, m_world.lastMoveWasRotation);

        m_world.blockGrid.embedTetramino(tetramino);
        m_world.graceDropMoves = 0u;

        ++m_world.tetaminosPlaced;

        handleTSpin(tSpinType, tetramino);

        if (const auto fullRows = findFullRows(); !fullRows.empty())
            m_animationTimelineP1.addInstantaneous(AnimClearLines{
                .rows       = fullRows,
                .awardXP    = true,
                .forceClear = false,
            });
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isInPlayableState() const
    {
        return !m_animationTimelineP0.anyAnimationPlaying() && !m_animationTimelineP1.anyAnimationPlaying() &&
               !m_animationTimelineP2.anyAnimationPlaying() && !m_inLevelUpScreen;
    }


    ////////////////////////////////////////////////////////////
    void addXP(const za::U64 amount)
    {
        m_world.currentXP += amount;

        // Check for level up (can happen multiple times from one large XP gain)
        while (m_world.currentXP >= getXPNeededForLevelUp(m_world.playerLevel))
        {
            m_world.currentXP -= getXPNeededForLevelUp(m_world.playerLevel);
            ++m_world.playerLevel;
        }
    }


    ////////////////////////////////////////////////////////////
    void restartGame()
    {
        m_world = World{};

        updateStepRefillBlockMatrixIfNeeded();
        initializeCurrentTetraminoFromBag();

        m_dynamicPerks.clear();

        m_dynamicPerks.pushBack(DynamicPerk{
            .trigger = DynamicPerkTrigger{TriggerLightningHit{2u}},
            .effects =
                {
                    DynamicPerkEffect{EffectGainXP{10u}},
                },
        });
    }


    ////////////////////////////////////////////////////////////
    void handleMenuKeyPressedEvent(const za::Event::KeyPressed& eKeyPressed)
    {
        ZA_ASSERT(m_inLevelUpScreen);

        if (m_menuDelayProgress < 1.f)
            return;

        const auto nPerks = m_perkIndicesSelectedThisLevel.size();

        if (eKeyPressed.code == za::Keyboard::Key::Down)
        {
            ++m_selectedPerk;

            if (m_selectedPerk >= nPerks)
                m_selectedPerk = 0u;

            playSound(m_sounds.menuSelect);
        }
        else if (eKeyPressed.code == za::Keyboard::Key::Up)
        {
            --m_selectedPerk;

            if (m_selectedPerk >= nPerks)
                m_selectedPerk = nPerks - 1u;

            playSound(m_sounds.menuSelect);
        }
        else if (eKeyPressed.code == za::Keyboard::Key::Enter || eKeyPressed.code == za::Keyboard::Key::Space)
        {
            m_inLevelUpScreen = false;

            ++m_world.committedPlayerLevel;

            m_perks[m_perkIndicesSelectedThisLevel[m_selectedPerk]]->apply(m_world);
            m_perkIndicesSelectedThisLevel.clear();

            playSound(m_sounds.menuConfirm);
        }
        else if (m_rerollsLeftThisLevel > 0 && eKeyPressed.code == za::Keyboard::Key::LShift)
        {
            --m_rerollsLeftThisLevel;

            m_menuDelayProgress = 0.5f;
            rerollPerks();

            playSound(m_sounds.menuReroll);
        }
    }


    ////////////////////////////////////////////////////////////
    void handleKeyPressedEvent(const za::Event::KeyPressed& eKeyPressed)
    {
        ZA_ASSERT(!m_inLevelUpScreen);

        const bool isP0TimelineBusy = m_animationTimelineP0.anyAnimationPlaying() &&
                                      !m_animationTimelineP0.isPlaying<AnimFadeAttachments>(); // skippable

        const bool isP1TimelineBusy = (m_animationTimelineP1.anyAnimationPlaying() &&
                                       !m_animationTimelineP1.isPlaying<AnimCollapseGrid>() && // skippable
                                       !m_animationTimelineP1.isPlaying<AnimFadeBlocks>()) ||  // skippable
                                      m_animationTimelineP1.isEnqueued<AnimHardDrop>() ||      // important
                                      m_animationTimelineP1.isEnqueued<AnimSquish>();          // important

        const bool isP2TimelineBusy = m_animationTimelineP2.anyAnimationPlaying();

        const bool inAnimation = isP0TimelineBusy || isP1TimelineBusy || isP2TimelineBusy;

        if (eKeyPressed.code == za::Keyboard::Key::Right)
        {
            moveCurrentTetraminoBy({1, 0});
            return;
        }

        if (eKeyPressed.code == za::Keyboard::Key::Down)
        {
            if (inAnimation)
            {
                playSound(m_sounds.error);
                return;
            }

            moveCurrentTetraminoBy({0, 1});
            return;
        }

        if (eKeyPressed.code == za::Keyboard::Key::Left)
        {
            moveCurrentTetraminoBy({-1, 0});
            return;
        }

        if (eKeyPressed.code == za::Keyboard::Key::Up)
        {
            rotateCurrentTetramino(/* clockwise */ true);
            return;
        }

        if (eKeyPressed.code == za::Keyboard::Key::Z)
        {
            rotateCurrentTetramino(/* clockwise */ false);
            return;
        }

        if (eKeyPressed.code == za::Keyboard::Key::X)
        {
            rotateCurrentTetramino(/* clockwise */ true);
            return;
        }

        if (eKeyPressed.code == za::Keyboard::Key::Space)
        {
            if (inAnimation)
            {
                playSound(m_sounds.error);
                return;
            }

            const int endY = calculateGhostY(*m_world.currentTetramino);

            m_animationTimelineP1.add(0.125f,
                                      AnimHardDrop{
                                          .tetramino = *m_world.currentTetramino,
                                          .endY      = endY,
                                      });

            resetAndRedrawCurrentTetramino(/* usedHold */ false);
            return;
        }

        if (eKeyPressed.code == za::Keyboard::Key::LShift)
        {
            if (inAnimation)
            {
                playSound(m_sounds.error);
                return;
            }

            if (m_world.perkCanHoldTetramino == 1)
                holdCurrentTetramino();
            else if (m_world.perkCanHoldTetramino == 2)
                skipCurrentTetramino();

            return;
        }

        if (eKeyPressed.code == za::Keyboard::Key::R)
        {
            if (inAnimation)
            {
                playSound(m_sounds.error);
                return;
            }

            restartGame();
            return;
        }

        if (eKeyPressed.code == za::Keyboard::Key::Q)
        {
            ++m_world.playerLevel;
            return;
        }

        if (eKeyPressed.code == za::Keyboard::Key::W)
        {
            m_animationTimelineP0.addInstantaneous(AnimLightningStrike{.numStrikes = 1u});
            return;
        }

        if (eKeyPressed.code == za::Keyboard::Key::E)
        {
            m_world.tick += 2000;
            m_world.blockMatrixBag.clear();
            updateStepRefillBlockMatrixIfNeeded();

            return;
        }

        if (eKeyPressed.code == za::Keyboard::Key::Num1)
        {
            if (auto* b = pickRandomBlock())
            {
                b->health          = 1u;
                b->tickTimer       = 0u;
                b->tickTimerTarget = nullTickTimerTarget;
                b->powerup         = BlockPowerup::None;
            }

            return;
        }

        if (eKeyPressed.code == za::Keyboard::Key::Num2)
        {
            if (auto* b = pickRandomBlock())
            {
                b->health          = 1u;
                b->tickTimer       = 0u;
                b->tickTimerTarget = static_cast<za::U32>(secondsToTicks(10.f));
                b->powerup         = BlockPowerup::None;
            }

            return;
        }

        if (eKeyPressed.code == za::Keyboard::Key::Num3)
        {
            if (auto* b = pickRandomBlock())
            {
                b->health          = 2u;
                b->tickTimer       = 0u;
                b->tickTimerTarget = nullTickTimerTarget;
                b->powerup         = BlockPowerup::None;
            }

            return;
        }

        if (eKeyPressed.code == za::Keyboard::Key::Num4)
        {
            if (auto* b = pickRandomBlock())
            {
                b->health          = 3u;
                b->tickTimer       = 0u;
                b->tickTimerTarget = nullTickTimerTarget;
                b->powerup         = BlockPowerup::None;
            }

            return;
        }
    }


    ////////////////////////////////////////////////////////////
    void initializeCurrentTetraminoFromBag()
    {
        ZA_ASSERT(!m_world.currentTetramino.hasValue());
        ZA_ASSERT(!m_world.blockMatrixBag.empty());

        const TaggedBlockMatrix taggedBlockMatrix = m_world.blockMatrixBag.front();
        m_world.blockMatrixBag.eraseAt(0u);

        m_world.currentTetramino.emplace(Tetramino{
            .shape         = taggedBlockMatrix.blockMatrix,
            .position      = za::Vec2uz{(m_world.blockGrid.getWidth() - shapeDimension) / 2u, 0u}.toVec2i(),
            .tetraminoType = taggedBlockMatrix.tetraminoType,
            .rotationState = RotationState{0u},
        });

        m_world.lastMoveWasRotation    = false;
        m_currentTetraminoVisualCenter = toDrawCoordinates(m_world.currentTetramino->position);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] ControlFlow eventStep()
    {
        while (za::Optional event = m_window.pollEvent())
        {
            m_imGuiContext.processEvent(m_window, *event);

            if (za::EventUtils::isClosedOrEscapeKeyPressed(*event))
                return ControlFlow::Break;

            if (handleNonScalingResize(*event, resolution, m_windowView))
                continue;

            if (auto* eKeyPressed = event->getIf<za::Event::KeyPressed>())
            {
                if (m_inLevelUpScreen)
                    handleMenuKeyPressedEvent(*eKeyPressed);
                else
                    handleKeyPressedEvent(*eKeyPressed);
            }
        }

        return ControlFlow::Continue;
    }


    ////////////////////////////////////////////////////////////
    void damageBlock(const za::Vec2uz position, Block& block)
    {
        if (block.health == 2u && block.isArmored())
        {
            m_fixedColorSpriteParticles.emplaceBack(SpriteParticleData{
                .position      = toDrawCoordinates(position) + za::Vec2f{-2.f, 1.f},
                .origin        = drawBlockSize / 2.f,
                .velocity      = m_rngFast.getVec2f({-0.95f, -2.15f}, {-0.15f, -1.25f}) * 0.065f,
                .scale         = 1.f,
                .scaleDecay    = 0.f,
                .accelerationY = 0.0004f,
                .opacity       = 1.f,
                .opacityDecay  = 0.f,
                .rotation      = 0.f,
                .torque        = m_rngFast.getF(-0.003f, -0.001f),
                .textureRect   = m_txrBlockHalf0,
            });

            m_fixedColorSpriteParticles.emplaceBack(SpriteParticleData{
                .position      = toDrawCoordinates(position).addX(drawBlockSize.x / 2.f) + za::Vec2f{-1.f, 1.f},
                .origin        = drawBlockSize / 2.f,
                .velocity      = m_rngFast.getVec2f({0.15f, -2.15f}, {0.95f, -1.25f}) * 0.065f,
                .scale         = 1.f,
                .scaleDecay    = 0.f,
                .accelerationY = 0.0004f,
                .opacity       = 1.f,
                .opacityDecay  = 0.f,
                .rotation      = 0.f,
                .torque        = m_rngFast.getF(0.001f, 0.003f),
                .textureRect   = m_txrBlockHalf1,
            });

            playSound(m_sounds.armorBreak);
        }

        block.applyDamage();


        playSound(m_sounds.armorHit);
        m_blockEffects[block.blockId] = BlockEffect{};
    }


    ////////////////////////////////////////////////////////////
    void rerollPerks()
    {
        m_perkIndicesSelectedThisLevel.clear();

        for (za::SizeT i = 0u; i < m_perks.size(); ++i)
            if (m_perks[i]->meetsPrerequisites(m_world))
                m_perkIndicesSelectedThisLevel.pushBack(i);

        while (m_perkIndicesSelectedThisLevel.size() > m_world.nPerkChoicesPerLevel)
        {
            const auto removeIdx = m_rngFast.getI<za::SizeT>(0u, m_perkIndicesSelectedThisLevel.size() - 1u);
            m_perkIndicesSelectedThisLevel.eraseAt(removeIdx);
        }

        shuffleBag(m_perkIndicesSelectedThisLevel, m_rngFast);
    }


    ////////////////////////////////////////////////////////////
    void spawnXPEarnedParticle(const za::Vec2f startPosition, const PaletteIdx paletteIdx)
    {
        m_earnedXPParticles.pushBack(EarnedXPParticle{
            .startPosition  = startPosition,
            .targetPosition = za::Vec2f{430.f, 64.f} + m_rngFast.getVec2f({-16.f, -16.f}, {16.f, 16.f}),
            .paletteIdx     = paletteIdx,
            .delay          = m_rngFast.getF(0.f, 0.2f),
            .startRotation  = m_rngFast.getF(0.f, za::tau),
        });
    }

    ////////////////////////////////////////////////////////////
    struct [[nodiscard]] EligibleBlock // NOLINT(cppcoreguidelines-pro-type-member-init)
    {
        Block*     block;
        za::Vec2uz position;
    };


    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool roll100(const int chanceOutOf100)
    {
        const auto roll = m_rngFast.getI(0, 100);
        return roll < chanceOutOf100;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] Block* pickRandomBlockMatching(auto&& predicate)
    {
        za::SizeT count    = 0u;
        Block*    selected = nullptr;

        for (auto& optBlock : m_world.blockGrid.getBlocks())
        {
            if (!optBlock.hasValue())
                continue;

            if (!predicate(*optBlock))
                continue;

            ++count;

            // Select the current bubble with probability `1/count` (reservoir sampling)
            if (m_rngFast.getI<za::SizeT>(0, count - 1) == 0)
                selected = optBlock.asPtr();
        }

        return (count == 0u) ? nullptr : selected;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] Block* pickRandomBlock()
    {
        return pickRandomBlockMatching([](const Block&) { return true; });
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] Block* pickDamageableBlock()
    {
        return pickRandomBlockMatching([](const Block& block) { return block.health > 1u; });
    }


    ////////////////////////////////////////////////////////////
    void updateStepQuakeEffect(const za::Time deltaTime)
    {
        m_quakeSinEffectLineClear.update(deltaTime);
        m_quakeSinEffectHardDrop.update(deltaTime);
    }


    ////////////////////////////////////////////////////////////
    void updateStepScreenShake(const za::Time deltaTime)
    {
        if (m_screenShakeTimer <= 0.f)
            return;

        m_screenShakeTimer -= deltaTime.asSeconds();

        if (m_screenShakeTimer <= 0.f)
        {
            m_screenShakeTimer  = 0.f;
            m_screenShakeAmount = 0.f;
        }
    }


    ////////////////////////////////////////////////////////////
    void updateStepShowLevelUpScreenIfNeeded()
    {
        if (m_animationTimelineP0.anyAnimationPlaying() || m_animationTimelineP1.anyAnimationPlaying() ||
            !m_earnedXPParticles.empty())
            return;

        if (m_world.committedPlayerLevel < m_world.playerLevel && !m_inLevelUpScreen)
        {
            m_inLevelUpScreen      = true;
            m_selectedPerk         = 0u;
            m_menuDelayProgress    = 0.f;
            m_rerollsLeftThisLevel = m_world.rerollsPerLevel;

            rerollPerks();

            playSound(m_sounds.newLevel);
        }
    }


    ////////////////////////////////////////////////////////////
    void updateStepInterpolateVisualTetraminoPosition(const za::Optional<Tetramino>& optTetramino,
                                                      za::Vec2f&                     visualCenter,
                                                      const za::Time                 deltaTime)
    {
        if (!optTetramino.hasValue())
            return;

        const za::Vec2f targetPosition = getTetraminoCenterDrawPosition(optTetramino->position);

        const float interpolationTime = 25.f;
        const auto  deltaTimeMs       = static_cast<float>(deltaTime.asMicroseconds()) / 1000.f;

        visualCenter = exponentialApproach(visualCenter, targetPosition, deltaTimeMs, interpolationTime);
    }


    ////////////////////////////////////////////////////////////
    void updateStepAnimations(const za::Time deltaTime)
    {
        if (m_inLevelUpScreen)
            return;

        const auto processTimeline = [&](auto& timeline)
        {
            auto& anim = timeline.commands.front();

            auto visitor = [&]<typename T>(T& innerAnim)
                requires(!za::isConst<T>) { return updateAnimation(timeline, innerAnim); };

            if (anim.data.linearVisit(visitor))
            {
                timeline.popFrontCommand();
                return;
            }

            timeline.timeOnCurrentCommand += deltaTime.asSeconds() * 1.f;

            if (!timeline.commands.empty() && timeline.commands.front().duration == 0.f)
            {
                updateStepAnimations(deltaTime);
            }
        };

        if (m_animationTimelineP0.anyAnimationPlaying())
        {
            processTimeline(m_animationTimelineP0);
            return;
        }

        if (m_animationTimelineP1.anyAnimationPlaying())
        {
            processTimeline(m_animationTimelineP1);
            return;
        }

        if (m_animationTimelineP2.anyAnimationPlaying())
        {
            processTimeline(m_animationTimelineP2);
            return;
        }
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool updateAnimation(auto& timeline, AnimWait&)
    {
        return timeline.getProgress() >= 1.f;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool updateAnimation(auto& timeline, AnimHardDrop& hardDrop)
    {
        if (timeline.getProgress() < 1.f)
            return false;

        // hardDrop.tetramino.position.y = hardDrop.endY;
        hardDrop.tetramino.position.y = calculateGhostY(hardDrop.tetramino);

        m_animationTimelineP1.add(0.125f, AnimSquish{.tetramino = hardDrop.tetramino});

        m_quakeSinEffectHardDrop.start(4.f, 4.f);

        playSound(m_sounds.landed);

        return true;
    }


    ////////////////////////////////////////////////////////////
    void processWeaponsBeforeEmbed(Tetramino& tetramino)
    {
        const auto processDrill = [&](const DrillDirection::Enum direction)
        {
            if (!m_world.perkDrill[direction].hasValue() || findDrillTargetBlocks(tetramino, direction).empty())
                return;

            int maxDrilledBlocks = 0;

            for (const auto bPos : findDrillBlocks(tetramino, direction))
            {
                const auto startPos                   = tetramino.position + bPos.toVec2i();
                const auto [nDrillableBlocks, endPos] = countDrillableBlocks(startPos, direction);

                maxDrilledBlocks = za::max(maxDrilledBlocks, nDrillableBlocks);
            }

            m_animationTimelineP0.add(0.3f + (0.1f * static_cast<float>(maxDrilledBlocks)),
                                      AnimDrill{
                                          .tetramino = tetramino,
                                          .direction = direction,
                                      });
        };

        const auto processLaser = [&](const LaserDirection::Enum direction)
        {
            if (!m_world.perkLaser[direction].hasValue())
                return;

            const auto maxPenetration = static_cast<za::SizeT>(m_world.perkLaser[direction]->maxPenetration);

            for (const auto bPos : findLaserBlocks(tetramino, direction))
            {
                const auto startPos            = tetramino.position + bPos.toVec2i();
                auto       laserableBlocksInfo = findLaserableBlocks(startPos, direction);

                if (laserableBlocksInfo.positions.empty() && laserableBlocksInfo.bouncePositions.empty())
                    continue;

                if (laserableBlocksInfo.positions.size() > maxPenetration)
                    laserableBlocksInfo.positions.resize(maxPenetration);

                for (const auto targetPos : laserableBlocksInfo.positions)
                    m_animationTimelineP0.add(0.175f,
                                              AnimLaser{
                                                  .tetramino     = tetramino,
                                                  .direction     = direction,
                                                  .gridStartPos  = startPos,
                                                  .gridTargetPos = targetPos,
                                                  .onlyVisual    = false,
                                              });

                for (const auto targetPos : laserableBlocksInfo.bouncePositions)
                {
                    m_animationTimelineP0.add(0.175f,
                                              AnimLaser{
                                                  .tetramino     = tetramino,
                                                  .direction     = direction,
                                                  .gridStartPos  = startPos,
                                                  .gridTargetPos = laserableBlocksInfo.bouncePos,
                                                  .onlyVisual    = true,
                                              });

                    m_animationTimelineP0.add(0.175f,
                                              AnimLaser{
                                                  .tetramino     = tetramino,
                                                  .direction     = direction,
                                                  .gridStartPos  = laserableBlocksInfo.bouncePos,
                                                  .gridTargetPos = targetPos,
                                                  .onlyVisual    = false,
                                              });
                }
            }
        };

        processDrill(DrillDirection::Down);
        processDrill(DrillDirection::Left);
        processDrill(DrillDirection::Right);

        processLaser(LaserDirection::Left);
        processLaser(LaserDirection::Right);

        m_animationTimelineP0.add(0.125f, AnimFadeAttachments{.tetramino = tetramino});
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool updateAnimation(auto& timeline, AnimSquish& squish)
    {
        if (timeline.getProgress() < 1.f)
            return false;

        processWeaponsBeforeEmbed(squish.tetramino);
        embedTetraminoAndClearLines(squish.tetramino);

        return true;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool updateAnimation([[maybe_unused]] auto& timeline, AnimClearLines& clearLines)
    {
        AnimClearLines::RowVector         trulyClearedRows;
        AnimFadeBlocks::FadingBlockVector fadingBlocks;
        za::Vector<za::Vec2uz>            columnClearPositions;

        const auto addRowIfNotExistent = [&](const za::SizeT row)
        {
            if (za::find(trulyClearedRows.begin(), trulyClearedRows.end(), row) == trulyClearedRows.end())
                trulyClearedRows.pushBack(row);
        };

        for (za::SizeT y : clearLines.rows)
        {
            bool rowIsFullyCleared = true;

            for (za::SizeT x = 0u; x < m_world.blockGrid.getWidth(); ++x)
            {
                if (auto& optBlock = m_world.blockGrid.at(za::Vec2uz{x, y}); optBlock.hasValue())
                {
                    // TODO: powerup
                    if (!clearLines.forceClear && optBlock->isArmored())
                    {
                        damageBlock(za::Vec2uz{x, y}, *optBlock);
                        rowIsFullyCleared = false;
                    }
                    else
                    {
                        fadingBlocks.pushBack(AnimFadeBlocks::FadingBlock{
                            .block    = *optBlock,
                            .position = {x, y},
                        });

                        if (optBlock->powerup != BlockPowerup::None)
                            handleTriggerPowerupCollected(*optBlock);

                        if (optBlock->powerup == BlockPowerup::XPBonus)
                        {
                            addXP(5u * m_world.playerLevel);
                            playSound(m_sounds.bonus);

                            spawnXPEarnedParticle(toDrawCoordinates(za::Vec2uz{x, y}), optBlock->paletteIdx);
                        }
                        else if (optBlock->powerup == BlockPowerup::ColumnDrill)
                        {
                            columnClearPositions.emplaceBack(x, y);
                            playSound(m_sounds.bonus);
                        }
                        else if (optBlock->powerup == BlockPowerup::ThreeRowDrill)
                        {
                            addRowIfNotExistent(y);

                            if (y + 1u < m_world.blockGrid.getHeight())
                                addRowIfNotExistent(y + 1);

                            if (y > 0u)
                                addRowIfNotExistent(y - 1);

                            playSound(m_sounds.bonus);
                        }

                        optBlock.reset();
                    }
                }
            }

            if (rowIsFullyCleared)
                addRowIfNotExistent(y);
        }

        if (!fadingBlocks.empty())
            m_animationTimelineP1.add(0.125f, AnimFadeBlocks{.fadingBlocks = fadingBlocks});

        if (!trulyClearedRows.empty())
        {
            playSound(m_sounds.single);

            const za::SizeT numCleared = trulyClearedRows.size();

            if (clearLines.awardXP)
            {
                m_world.linesCleared += numCleared;

                const za::U64 amount = [&]
                {
                    if (numCleared == 1)
                        return 10u;

                    if (numCleared == 2)
                        return 25u;

                    if (numCleared == 3)
                        return 40u;

                    return 60u;
                }();

                const float quakeMagnitude = 8.f + static_cast<float>(numCleared) * 1.5f;
                const float quakeSpeed     = 4.f - static_cast<float>(numCleared) * 0.5f;

                m_quakeSinEffectLineClear.start(quakeMagnitude, quakeSpeed);

                addXP(amount);
                playSound(m_sounds.exp);

                for (za::U64 i = 0u; i < fadingBlocks.size() * 4u; ++i)
                {
                    const auto& block = fadingBlocks[i % fadingBlocks.size()];

                    const auto startPosition = toDrawCoordinates(block.position) +
                                               m_rngFast.getVec2f(-drawBlockSize, drawBlockSize) / 2.f;

                    spawnXPEarnedParticle(startPosition, block.block.paletteIdx);
                }
            }

            za::quickSort(trulyClearedRows.begin(), trulyClearedRows.end(), [](const za::SizeT a, const za::SizeT b) {
                return a < b;
            });

            const auto height = m_world.blockGrid.getHeight();
            m_rowYOffsets.resize(height);

            for (za::SizeT y = 0u; y < height; ++y)
                m_rowYOffsets[y] = 0.f;

            za::SizeT dropAmount = 0;

            // This calculates the correct offset for each row's FINAL position.
            for (int y = static_cast<int>(height) - 1; y >= 0; --y)
            {
                // Check if the current row 'y' (from the original grid) is being cleared.
                const bool isCleared = za::find(trulyClearedRows.begin(), trulyClearedRows.end(), static_cast<za::SizeT>(y)) !=
                                       trulyClearedRows.end();

                if (isCleared)
                {
                    // If it is, increment the number of lines that rows above this one must drop.
                    ++dropAmount;
                }
                else if (dropAmount > 0)
                {
                    // If this row is being kept, the row that lands at its *new* position
                    // (which is y + dropAmount) needs an initial offset.
                    const za::SizeT finalY = static_cast<za::SizeT>(y) + dropAmount;
                    if (finalY < height)
                        m_rowYOffsets[finalY] = -static_cast<float>(dropAmount) * drawBlockSize.y;
                }
            }

            for (const auto rowIndex : trulyClearedRows)
                m_world.blockGrid.shiftRowDown(rowIndex);

            m_animationTimelineP1.add(0.15f, AnimCollapseGrid{.clearedRows = trulyClearedRows});

            if (clearLines.awardXP)
                handleTriggerLinesCleared(numCleared);
        }
        else if (!fadingBlocks.empty())
        {
            const auto numPartiallyCleared = clearLines.rows.size();

            const za::U64 amount = [&]
            {
                if (numPartiallyCleared == 1)
                    return 4u;

                if (numPartiallyCleared == 2)
                    return 10u;

                if (numPartiallyCleared == 3)
                    return 16u;

                return 20u;
            }();

            addXP(amount);
            playSound(m_sounds.exp);

            for (za::U64 i = 0u; i < fadingBlocks.size() * 4u; ++i)
            {
                const auto& block = fadingBlocks[i % fadingBlocks.size()];

                const auto startPosition = toDrawCoordinates(block.position) +
                                           m_rngFast.getVec2f(-drawBlockSize, drawBlockSize) / 2.f;

                spawnXPEarnedParticle(startPosition, block.block.paletteIdx);
            }
        }

        for (const auto columnClearPos : columnClearPositions)
            m_animationTimelineP1.add(0.3f, AnimColumnClear{.position = columnClearPos.addY(1)});

        return true;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool updateAnimation(auto& timeline, AnimFadeBlocks&)
    {
        return timeline.getProgress() >= 1.f;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool updateAnimation(auto& timeline, AnimCollapseGrid&)
    {
        if (timeline.getProgress() < 1.f)
            return false;

        for (auto& rowOffset : m_rowYOffsets)
            rowOffset = 0.f;

        return true;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool updateAnimation(auto& timeline, AnimDrill& drill)
    {
        if (timeline.justStarted())
            playSound(m_sounds.drill);

        m_screenShakeAmount = 0.85f;
        m_screenShakeTimer  = 0.05f;

        if (timeline.getProgress() < 1.f)
            return false;

        for (auto [blockPtr, position] : findDrillTargetBlocks(drill.tetramino, drill.direction))
        {
            damageBlock(position, *blockPtr);
            handleTriggerDrillHit();
        }

        return true;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool updateAnimation(auto& timeline, AnimLaser& laser)
    {
        auto& optBlock = m_world.blockGrid.at(laser.gridTargetPos);

        if (!laser.onlyVisual && (!optBlock.hasValue() || !optBlock->isDamageable()))
        {
            // cancel animation if block is already at 1 health or destroyed
            // (e.g. drilled earlier in the same animation sequence)
            return true;
        }

        if (timeline.justStarted())
        {
            playSound(m_sounds.laser);

            const auto dir      = laserDirectionToVec2i(laser.direction).toVec2f();
            const auto startPos = toDrawCoordinates(laser.gridStartPos) + dir * 8.f + za::Vec2f{0, 2.f};

            ZA_ASSERT(!m_optLaserBeam.hasValue());
            m_optLaserBeam.emplace(startPos, startPos, blockPalette[getTetraminoPaletteIdx(laser.tetramino)]);
        }

        m_screenShakeAmount = 0.65f;
        m_screenShakeTimer  = 0.05f;

        if (timeline.getProgress() < 1.f)
            return false;

        if (!laser.onlyVisual)
        {
            ZA_ASSERT(optBlock.hasValue());
            damageBlock(laser.gridTargetPos.toVec2uz(), *optBlock);

            handleTriggerLaserHit();
        }
        else
        {
            playSound(m_sounds.bounce);
        }

        ZA_ASSERT(m_optLaserBeam.hasValue());
        m_optLaserBeam.reset();

        return true;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool updateAnimation(auto& timeline, AnimFadeAttachments&)
    {
        return timeline.getProgress() >= 1.f;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool updateAnimation(auto& timeline, AnimColumnClear& columnClear)
    {
        if (timeline.getProgress() < 1.f)
            return false;

        AnimFadeBlocks::FadingBlockVector fadingBlocks;

        for (za::SizeT y = columnClear.position.y; y < m_world.blockGrid.getHeight(); ++y)
            if (auto& optBlock = m_world.blockGrid.at(za::Vec2uz{columnClear.position.x, y}); optBlock.hasValue())
            {
                fadingBlocks.pushBack(AnimFadeBlocks::FadingBlock{
                    .block    = *optBlock,
                    .position = za::Vec2uz{columnClear.position.x, y},
                });

                optBlock.reset();
            }

        if (!fadingBlocks.empty())
            m_animationTimelineP1.add(0.125f, AnimFadeBlocks{.fadingBlocks = fadingBlocks});

        return true;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool updateAnimation(auto& timeline, AnimLightningStrike& lightningStrike)
    {
        if (timeline.getProgress() < 1.f)
            return false;

        za::Vector<EligibleBlock> eligibleBlocks;

        m_world.blockGrid.forBlocks([&](Block& block, const za::Vec2uz position)
        {
            if (block.isDamageable())
                eligibleBlocks.pushBack({&block, position});

            return ControlFlow::Continue;
        });

        if (eligibleBlocks.empty())
            return true;

        const auto blockInfo = eligibleBlocks[0];

        m_lightningBolts.emplaceBack(m_rngFast,
                                     za::Vec2f{9.f + m_rngFast.getF(0.f,
                                                                    drawBlockSize.x *
                                                                        static_cast<float>(m_world.blockGrid.getWidth())),
                                               0.f},
                                     toDrawCoordinates(blockInfo.position));

        for (int i = 0; i < 16; ++i)
        {
            m_fixedColorCircleShapeParticles.emplaceBack(CircleParticleData{
                .position      = toDrawCoordinates(blockInfo.position),
                .velocity      = m_rngFast.getVec2f({-0.75f, -2.15f}, {0.75f, -0.25f}) * 0.25f,
                .scale         = m_rngFast.getF(0.08f, 0.27f) * 0.75f,
                .scaleDecay    = 0.f,
                .accelerationY = 0.0004f,
                .opacity       = 0.75f,
                .opacityDecay  = m_rngFast.getF(0.001f, 0.002f) * 0.7f,
                .rotation      = m_rngFast.getF(0.f, za::tau),
                .torque        = m_rngFast.getF(-0.001f, 0.001f),
                .color         = za::Color::White,
                .radius        = m_rngFast.getF(9.f, 16.f),
                .pointCount    = 5u,
            });

            m_fixedColorCircleShapeParticles.emplaceBack(CircleParticleData{
                .position      = toDrawCoordinates(blockInfo.position),
                .velocity      = m_rngFast.getVec2f({-0.75f, -2.15f}, {0.75f, -0.25f}) * 0.075f,
                .scale         = m_rngFast.getF(0.08f, 0.27f) * 0.25f,
                .scaleDecay    = 0.f,
                .accelerationY = 0.0004f,
                .opacity       = 0.75f,
                .opacityDecay  = m_rngFast.getF(0.001f, 0.002f) * 0.7f,
                .rotation      = m_rngFast.getF(0.f, za::tau),
                .torque        = m_rngFast.getF(-0.001f, 0.001f),
                .color         = za::Color::LightYellow,
                .radius        = m_rngFast.getF(8.f, 14.f),
                .pointCount    = 5u,
            });
        }

        playSound(m_sounds.strike);
        damageBlock(blockInfo.position, *blockInfo.block);

        m_screenShakeAmount = 2.5f;
        m_screenShakeTimer  = 0.2f;

        m_animationTimelineP0.add(0.15f, AnimWait{});

        handleTriggerLightningHit();

        if (lightningStrike.numStrikes > 1u)
            m_animationTimelineP0.addInstantaneous(AnimLightningStrike{.numStrikes = lightningStrike.numStrikes - 1u});

        return true;
    }


    ////////////////////////////////////////////////////////////
    void updateStepBlockEffects(const za::Time deltaTime)
    {
        for (auto* it = m_blockEffects.begin(); it != m_blockEffects.end();)
        {
            it->second.squishTime += deltaTime.asSeconds();

            if (it->second.squishTime >= BlockEffect::squishDuration)
            {
                it = m_blockEffects.erase(it);
                continue;
            }

            ++it;
        }
    }


    ////////////////////////////////////////////////////////////
    void updateStepEarnedXPParticles(const za::Time deltaTime)
    {
        const auto deltaTimeMs = static_cast<float>(deltaTime.asMicroseconds()) / 1000.f;

        for (auto& particle : m_earnedXPParticles)
        {
            if (particle.delay > 0.f)
            {
                particle.delay -= deltaTimeMs * 0.0015f;
                continue;
            }

            (void)particle.progress.advance(deltaTimeMs * 0.0015f);
        }

        za::vectorEraseIf(m_earnedXPParticles, [&](const auto& p) { return p.progress.isAtEnd(); });
    }


    ////////////////////////////////////////////////////////////
    void updateStepParticleData(auto& vec, const za::Time deltaTime)
    {
        const auto deltaTimeMs = static_cast<float>(deltaTime.asMicroseconds()) / 1000.f;

        for (auto& p : vec)
        {
            p.velocity.y += p.accelerationY * deltaTimeMs;
            p.position += p.velocity * deltaTimeMs;

            p.rotation += p.torque * deltaTimeMs;
            p.rotation = za::positiveRemainder(p.rotation, za::tau);

            p.opacity = za::clamp(p.opacity - p.opacityDecay * deltaTimeMs, 0.f, 1.f);
            p.scale   = za::max(p.scale - p.scaleDecay * deltaTimeMs, 0.f);
        }

        za::vectorEraseIf(vec, [](const auto& particleLike) { return particleLike.opacity <= 0.f; });
    }


    ////////////////////////////////////////////////////////////
    void updateStepRefillBlockMatrixIfNeeded()
    {
        if (m_world.blockMatrixBag.size() >= 3u) // TODO: adjust for peek?
            return;

        const auto oldBagSize = m_world.blockMatrixBag.size();

        constexpr za::SizeT bagMult = 2u;

        const auto addToBag = [&](const TetraminoType j)
        {
            const Block block{
                .tetraminoId        = m_world.nextTetraminoId++,
                .blockId            = BlockId{0u}, // updated below
                .health             = Health{1u},
                .paletteIdx         = PaletteIdx::from(j),
                .shapeBlockSequence = ShapeBlockSequence::_, // set by `shapeMatrixToBlockMatrix`
                .powerup            = BlockPowerup::None,
                .tickTimer          = 0u,
                .tickTimerTarget    = nullTickTimerTarget,
            };

            auto& [blockMatrix, tetraminoType] = m_world.blockMatrixBag.pushBack({
                .blockMatrix = shapeMatrixToBlockMatrix(srsTetraminoShapes[static_cast<za::SizeT>(j) /* pieceType */][0], block),
                .tetraminoType = j,
            });

            const auto healthDist = generateTetraminoHealthDistribution(getDifficultyFactor(m_world.tick), m_rngFast);
            za::SizeT  nextHealthDistIdx = 0u;

            for (za::Optional<Block>& b : blockMatrix.data)
            {
                if (!b.hasValue())
                    continue;

                b->blockId = m_world.nextBlockId++;

                const auto blockType = static_cast<za::U8>(healthDist[nextHealthDistIdx++]);

                if (blockType > 2)
                {
                    b->health = static_cast<Health>(blockType - 1);
                }
                else if (blockType == 2)
                {
                    b->tickTimerTarget = static_cast<za::U32>(secondsToTicks(20.f));
                }
                else if (roll100(1))
                {
                    b->health = 1u;

                    if (roll100(25))
                    {
                        b->powerup         = BlockPowerup::ThreeRowDrill;
                        b->tickTimerTarget = static_cast<za::U32>(secondsToTicks(10.f));
                    }
                    else
                    {
                        b->powerup         = BlockPowerup::XPBonus;
                        b->tickTimerTarget = static_cast<za::U32>(secondsToTicks(20.f));
                    }
                }
            }
        };

        for (za::SizeT i = 0u; i < bagMult; ++i)
            for (za::U8 j = 0u; j < static_cast<za::U8>(tetraminoShapeCount); ++j)
                addToBag(static_cast<TetraminoType>(j));

        for (int i = 0; i < m_world.perkExtraLinePiecesInPool; ++i)
            addToBag(TetraminoType::I);

        // only shuffle the newly added items
        shuffleBag(m_world.blockMatrixBag.begin() + oldBagSize, m_world.blockMatrixBag.end(), m_rngFast);
    }


    ////////////////////////////////////////////////////////////
    void applyGravityToCurrentTetramino()
    {
        ZA_ASSERT(m_world.currentTetramino.hasValue());

        const auto newPosition = m_world.currentTetramino->position.addY(1);

        if (m_world.blockGrid.isValidMove(m_world.currentTetramino->shape, newPosition))
        {
            m_world.currentTetramino->position = newPosition;
            return;
        }

        if (m_world.graceDropMoves < m_world.maxGraceDropMoves)
        {
            ++m_world.graceDropMoves;
            return;
        }

        processWeaponsBeforeEmbed(*m_world.currentTetramino);
        embedTetraminoAndClearLines(*m_world.currentTetramino);
        resetAndRedrawCurrentTetramino(/* usedHold */ false);

        playSound(m_sounds.place);
    }


    ////////////////////////////////////////////////////////////
    void updateStepProcessSimulation(const float xTicksPerSecond)
    {
        for (unsigned int i = 0; i < static_cast<unsigned int>(m_timeAccumulator * xTicksPerSecond); ++i)
        {
            m_timeAccumulator -= 1.f / xTicksPerSecond;

            ++m_world.tick;

            ZA_ASSERT(m_world.dropTickAccumulator < m_world.dropTickTarget);
            ZA_ASSERT(m_world.dropTickTarget > 0u);

            ++m_world.dropTickAccumulator;

            if (m_world.dropTickAccumulator == m_world.dropTickTarget)
            {
                m_world.dropTickAccumulator = 0u;
                applyGravityToCurrentTetramino();
            }

            for (auto& optBlock : m_world.blockGrid.getBlocks())
            {
                if (!optBlock.hasValue())
                    continue;

                if (optBlock->tickTimerTarget == nullTickTimerTarget)
                    continue;

                if (optBlock->tickTimer == optBlock->tickTimerTarget)
                    continue;

                ++optBlock->tickTimer;

                if (optBlock->tickTimer == optBlock->tickTimerTarget)
                {
                    optBlock->tickTimerTarget = nullTickTimerTarget;

                    if (optBlock->powerup != BlockPowerup::None)
                    {
                        // powerup expires
                        optBlock->powerup = BlockPowerup::None;
                    }
                    else
                    {
                        // health increases
                        ++optBlock->health;
                        playSound(m_sounds.armorGuard);
                        m_blockEffects[optBlock->blockId] = BlockEffect{};
                    }
                }
            }
        }
    }


    ////////////////////////////////////////////////////////////
    void updateStepLightningBolts(const za::Time deltaTime)
    {
        for (auto& lb : m_lightningBolts)
            lb.update(deltaTime);

        za::vectorEraseIf(m_lightningBolts, [](const LightningBolt& lb) { return lb.isFinished(); });
    }


    ////////////////////////////////////////////////////////////
    void updateStepLaserBeam(const za::Time deltaTime)
    {
        if (m_optLaserBeam.hasValue())
            m_optLaserBeam->update(deltaTime);
    }


    ////////////////////////////////////////////////////////////
    void updateStep(const za::Time deltaTime, const float xTicksPerSecond)
    {
        SFEX_PROFILE_SCOPE("update");

        updateStepQuakeEffect(deltaTime);
        updateStepScreenShake(deltaTime);
        updateStepShowLevelUpScreenIfNeeded();
        updateStepInterpolateVisualTetraminoPosition(m_world.currentTetramino, m_currentTetraminoVisualCenter, deltaTime);
        updateStepAnimations(deltaTime);
        updateStepBlockEffects(deltaTime);
        updateStepEarnedXPParticles(deltaTime);
        updateStepParticleData(m_hueColorCircleShapeParticles, deltaTime);
        updateStepParticleData(m_fixedColorCircleShapeParticles, deltaTime);
        updateStepParticleData(m_fixedColorSpriteParticles, deltaTime);

        if (isInPlayableState())
            updateStepProcessSimulation(xTicksPerSecond);

        updateStepLightningBolts(deltaTime);
        updateStepLaserBeam(deltaTime);

        // TODO:
        if (m_menuDelayProgress < 1.f)
        {
            m_menuDelayProgress += deltaTime.asSeconds();
            m_menuDelayProgress = za::min(m_menuDelayProgress, 1.f);
        }
    }


    ////////////////////////////////////////////////////////////
    void syncShaderUniforms()
    {
        if (!m_mustSyncShaderUniforms)
            return;

        m_mustSyncShaderUniforms = false;

        m_shaderCRT.setUniform(m_ulCurvature, m_fUlCurvature);
        m_shaderCRT.setUniform(m_ulVignetteStrength, m_fUlVignetteStrength);
        m_shaderCRT.setUniform(m_ulVignetteInnerRadius, m_fUlVignetteInnerRadius);
        m_shaderCRT.setUniform(m_ulVignetteOuterRadius, m_fUlVignetteOuterRadius);
        m_shaderCRT.setUniform(m_ulScanlineStrength, m_fUlScanlineStrength);
        m_shaderCRT.setUniform(m_ulScanlineBrightnessModulation, m_fUlScanlineBrightnessModulation);
        m_shaderCRT.setUniform(m_ulScanlineScrollSpeed, m_fUlScanlineScrollSpeed);
        m_shaderCRT.setUniform(m_ulScanlineThickness, m_fUlScanlineThickness);
        m_shaderCRT.setUniform(m_ulScanlineHeight, m_fUlScanlineHeight);
        m_shaderCRT.setUniform(m_ulNoiseStrength, m_fUlNoiseStrength);
        m_shaderCRT.setUniform(m_ulMaskStrength, m_fUlMaskStrength);
        m_shaderCRT.setUniform(m_ulMaskScale, m_fUlMaskScale);
        m_shaderCRT.setUniform(m_ulBloomStrength, m_fUlBloomStrength);
        m_shaderCRT.setUniform(m_ulInputGamma, m_fUlInputGamma);
        m_shaderCRT.setUniform(m_ulOutputGamma, m_fUlOutputGamma);
        m_shaderCRT.setUniform(m_ulSaturation, m_fUlSaturation);

        m_shaderPostProcess.setUniform(m_ulPPVibrance, m_fUlPPVibrance);
        m_shaderPostProcess.setUniform(m_ulPPSaturation, m_fUlPPSaturation);
        m_shaderPostProcess.setUniform(m_ulPPLightness, m_fUlPPLightness);
        m_shaderPostProcess.setUniform(m_ulPPSharpness, m_fUlPPSharpness);
    }


    ////////////////////////////////////////////////////////////
    void drawShop()
    {
        SFEX_PROFILE_SCOPE_AUTOLABEL();

        if (!m_inLevelUpScreen)
            return;

        ZA_ASSERT(m_world.committedPlayerLevel < m_world.playerLevel);

        const auto darkenAlpha = static_cast<za::U8>((0.65f * m_menuDelayProgress) * 255.f);

        m_rtGame.draw(
            za::RectangleShapeData{
                .position  = {0.f, 0.f},
                .fillColor = za::Color::blackWithAlpha(darkenAlpha),
                .size      = resolution,
            },
            {.view = m_worldView});

        za::Vec2f shopSize{280.f, 161.f};

        if (m_perkIndicesSelectedThisLevel.size() == 4u)
            shopSize.y += 53.f;

        const auto centeredShopPos = resolution * 0.5f;

        const float menuScale = easeInOutBack(m_menuDelayProgress);

        // The offset seems to fix rendering corruption on Surface
        const auto menuTransform = za::Transform::fromPositionScaleOrigin(centeredShopPos + za::Vec2f{0.1f, 0.1f},
                                                                          {menuScale, menuScale},
                                                                          shopSize * 0.5f);

        m_rtGame.draw(
            za::RectangleShapeData{
                .position         = {0.f, 0.f},
                .origin           = {0.f, 0.f},
                .fillColor        = {0, 0, 0},
                .outlineColor     = {135, 135, 135},
                .outlineThickness = 1.f,
                .size             = shopSize,
            },
            {.transform = menuTransform, .view = m_worldView});

        m_textVerticesBuffer.clear();
        m_textIndicesBuffer.clear();

        za::String levelUpString = "^bold[](^wobble[5,1.2,0.5](LEVEL UP)^)^";

        if (m_rerollsLeftThisLevel > 0u)
            levelUpString += za::fmtToString("^color[190,190,190]( - Press SHIFT to reroll ({} left))^",
                                             m_rerollsLeftThisLevel);

        const BitmapTextToVerticesOptions titleOpts = {
            .outVertices     = m_textVerticesBuffer,
            .outIndices      = m_textIndicesBuffer,
            .bitmapFont      = m_bitmapFontMinogram,
            .fontTextureRect = m_txrBFMinogram6x10,
            .alignment       = BitmapTextAlignment::Center,
            .baseColor       = za::Color::White,
            .time            = m_totalTime,
            .string          = levelUpString,
        };

        auto            bounds       = bitmapTextToVertices<true>(titleOpts);
        const za::Vec2f textPosition = {(shopSize.x - bounds.x) * 0.5f, -8.f};

        const auto titleGlobalBounds = bitmapTextToVerticesPretransformed(titleOpts,
                                                                          za::Transform::fromPosition(textPosition));

        m_rtGame.draw(za::RectangleShapeData{.position         = titleGlobalBounds.position - za::Vec2f{8.f, 5.f},
                                             .origin           = {0.f, 0.f},
                                             .fillColor        = {0, 0, 0},
                                             .outlineColor     = {135, 135, 135},
                                             .outlineThickness = 1.f,
                                             .size             = titleGlobalBounds.size + za::Vec2f{16.f, 8.f}},
                      {.transform = menuTransform, .view = m_worldView});

        /*

        m_rtGame.draw(
            za::RectangleShapeData{
                .position  = {4.f, titleGlobalBounds.getBottom() + 4.f},
                .origin    = {0.f, 0.f},
                .fillColor = {135, 135, 135},
                .size      = {shopSize.x - 8.f, 1.f},
            },
            {.transform = menuTransform, .view = m_worldView});

        */

        auto perkDrawPos = za::Vec2f{48.f, titleGlobalBounds.getBottom() + 8.f};

        za::SizeT iPerk = 0u;

        for (const za::SizeT psIndex : m_perkIndicesSelectedThisLevel)
        {
            const Perk& perk = *(m_perks[psIndex]);

            za::String perkName        = perk.getName();
            za::String perkDescription = wrapText(perk.getDescription(m_world), 38u);
            za::String perkProgression = wrapText(perk.getProgressionStr(m_world), 38u);

            const auto perkStr = za::fmtToString("^bold[]({})^\n^hspace[0](^color[190,190,190]({})^)^", perkName, perkDescription);

            const auto transform = za::Transform::fromPosition(perkDrawPos);

            const auto globalBounds = bitmapTextToVerticesPretransformed(
                {
                    .outVertices     = m_textVerticesBuffer,
                    .outIndices      = m_textIndicesBuffer,
                    .bitmapFont      = m_bitmapFontMinogram,
                    .fontTextureRect = m_txrBFMinogram6x10,
                    .alignment       = BitmapTextAlignment::Left,
                    .baseColor       = za::Color::White,
                    .time            = m_totalTime,
                    .string          = perkStr,
                },
                transform);

            m_rtGame.draw(za::RectangleShapeData{.position  = globalBounds.position.withX(8),
                                                 .origin    = {0.f, 0.f},
                                                 .fillColor = za::Color::VeryDarkBrown.withAlpha(100),
                                                 .size      = {32.f, 32.f}},
                          {.transform = menuTransform, .view = m_worldView});

            m_rtGame.draw(za::RectangleShapeData{.position  = globalBounds.position - za::Vec2f{1.f, 1.f},
                                                 .origin    = {0.f, 0.f},
                                                 .fillColor = za::Color::VeryDarkBrown.withAlpha(100),
                                                 .size = globalBounds.size.withY(za::max(40.f, globalBounds.size.y)) +
                                                         za::Vec2f{2.f, 2.f}},
                          {.transform = menuTransform, .view = m_worldView});

            if (m_selectedPerk == iPerk)
            {
                m_rtGame.draw(za::RectangleShapeData{.position  = globalBounds.position - za::Vec2f{1.f, 1.f},
                                                     .origin    = {0.f, 0.f},
                                                     .fillColor = za::Color::VeryDarkGreen,
                                                     .size      = globalBounds.size + za::Vec2f{2.f, 2.f}},
                              {.transform = menuTransform, .view = m_worldView});
            }

            perkDrawPos.y += za::max(40.f, globalBounds.size.y) + 12.f;

            ++iPerk;
        }

        m_rtGame.drawIndexedVertices(
            za::DrawIndexedVerticesSettings{
                .vertexSpan    = m_textVerticesBuffer,
                .indexSpan     = m_textIndicesBuffer,
                .primitiveType = za::PrimitiveType::Triangles,
            },
            {
                .transform = menuTransform,
                .view      = m_worldView,
                .texture   = &m_textureAtlas.getTexture(),
            });
    }


    ////////////////////////////////////////////////////////////
    void imguiStep(const za::Time deltaTime)
    {
        SFEX_PROFILE_SCOPE("imgui");

        m_imGuiContext.update(m_window, deltaTime);

        ImGui::Begin("SFEX Profiler");
        sfex::showImguiProfiler();
        ImGui::End();

        return; // TODO P1: remove?

        {
            ImGui::Begin("Graphics settings", nullptr);

            {
                ImGui::Checkbox("CRT Shader", &m_useCRTShader);

                bool textureFiltering = m_rtGame.isSmooth();
                if (ImGui::Checkbox("Texture Filtering", &textureFiltering))
                    m_rtGame.setSmooth(textureFiltering);

                bool textureFilteringPost = m_rtPostProcess.isSmooth();
                if (ImGui::Checkbox("Texture Filtering (post)", &textureFilteringPost))
                    m_rtPostProcess.setSmooth(textureFilteringPost);

                const auto makeSlider = [&](float& value, const char* label, const float min, const float max)
                {
                    if (ImGui::SliderFloat(label, &value, min, max))
                    {
                        m_mustSyncShaderUniforms = true;
                        return true;
                    }

                    return false;
                };

                makeSlider(m_fUlCurvature, "Curvature", -0.5, 0.5);
                makeSlider(m_fUlVignetteStrength, "Vignette Strength", 0.f, 1.f);
                makeSlider(m_fUlVignetteInnerRadius, "Vignette Inner Radius", -1.f, 2.f);
                makeSlider(m_fUlVignetteOuterRadius, "Vignette Outer Radius", -1.f, 2.f);
                makeSlider(m_fUlScanlineStrength, "Scanline Strength", -1.f, 10.f);
                makeSlider(m_fUlScanlineBrightnessModulation, "Scanline Brightness Modulation", -5.f, 5.f);
                makeSlider(m_fUlScanlineScrollSpeed, "Scanline Scroll Speed", -100.f, 100.f);
                makeSlider(m_fUlScanlineThickness, "Scanline Thickness", -40.f, 40.f);
                makeSlider(m_fUlScanlineHeight, "Scanline Height", -10.f, 10.f);
                makeSlider(m_fUlNoiseStrength, "Noise Strength", -100.f, 100.f);
                makeSlider(m_fUlMaskStrength, "Mask Strength", -5.f, 5.f);
                makeSlider(m_fUlMaskScale, "Mask Scale", -5.f, 5.f);
                makeSlider(m_fUlBloomStrength, "Bloom Strength", -5.f, 5.f);
                makeSlider(m_fUlInputGamma, "Input Gamma", -5.f, 5.f);
                makeSlider(m_fUlOutputGamma, "Output Gamma", -5.f, 5.f);
                makeSlider(m_fUlSaturation, "Saturation", -5.f, 5.f);

                makeSlider(m_fUlPPVibrance, "Vibrance (post)", -3.f, 3.f);
                makeSlider(m_fUlPPSaturation, "Saturation (post)", -3.f, 3.f);
                makeSlider(m_fUlPPLightness, "Lightness (post)", -3.f, 3.f);
                makeSlider(m_fUlPPSharpness, "Sharpness (post)", -3.f, 3.f);
            }

            ImGui::End();
        }

        const auto  windowSize = m_window.getSize().toVec2f();
        const float scale      = getPixelPerfectScale(windowSize, resolution);

        const auto setFontScale = [&](const float x) { ImGui::SetWindowFontScale(x * scale / 2.f); };

        const auto textCentered = [&](const za::String& text)
        {
            const auto windowWidth = ImGui::GetWindowSize().x;
            const auto textWidth   = ImGui::CalcTextSize(text.cStr()).x;

            ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
            ImGui::Text("%s", text.cStr());
        };

        if (m_inLevelUpScreen)
        {
            ZA_ASSERT(m_world.committedPlayerLevel < m_world.playerLevel);

            ImGui::SetNextWindowBgAlpha(0.95f);
            ImGui::PushFont(m_imguiFont);

            ImGui::Begin("Level Up!", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

            const ImVec2 menuSize{100.f * scale, 200.f * scale};
            const ImVec2 menuPos{windowSize.x / 2.f - menuSize.x / 2.f, windowSize.y / 2.f - menuSize.y / 2.f};

            ImGui::SetWindowPos(menuPos);
            ImGui::SetWindowSize(menuSize);

            setFontScale(1.f);
            ImGui::PushFont(m_imguiFontBig);
            textCentered("*** LEVEL UP ***");
            ImGui::PopFont();
            setFontScale(1.f);
            textCentered("CHOOSE A PERK");
            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Spacing();

            static int  selectedPerk = -1;
            static bool sep          = false;

            for (const za::SizeT psIndex : m_perkIndicesSelectedThisLevel)
            {
                const Perk& perk = *(m_perks[psIndex]);

                za::String perkName        = perk.getName();
                za::String perkDescription = perk.getDescription(m_world);
                za::String perkProgression = perk.getProgressionStr(m_world);

                if (sep)
                    ImGui::Separator();

                // setFontScale(2.f);
                ImGui::PushFont(m_imguiFontBig);

                if (ImGui::Selectable(perkName.cStr(), selectedPerk == static_cast<int>(psIndex)))
                    selectedPerk = static_cast<int>(psIndex);

                ImGui::PopFont();

                setFontScale(0.5f);
                if (!perkProgression.empty())
                    ImGui::Text("(%s)\n", perkProgression.cStr());
                else
                    ImGui::Text("\n");
                setFontScale(1.f);
                ImGui::TextWrapped("%s", perkDescription.cStr());

                sep = true;
            }

            auto buttonCenteredOnLine = [&](const char* label, float alignment = 0.5f)
            {
                ImGuiStyle& style = ImGui::GetStyle();

                float size  = ImGui::CalcTextSize(label).x + style.FramePadding.x * 2.f;
                float avail = ImGui::GetContentRegionAvail().x;

                float off = (avail - size) * alignment;
                if (off > 0.f)
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

                return ImGui::Button(label);
            };

            {
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::PushFont(m_imguiFontBig);
                setFontScale(1.f);

                if (selectedPerk == -1)
                    ImGui::BeginDisabled();

                if (buttonCenteredOnLine("Confirm"))
                {
                    m_inLevelUpScreen = false;
                    m_perkIndicesSelectedThisLevel.clear();

                    ++m_world.committedPlayerLevel;

                    m_perks[static_cast<za::SizeT>(selectedPerk)]->apply(m_world);
                }

                if (selectedPerk == -1)
                    ImGui::EndDisabled();

                ImGui::PopFont();
            }

            ImGui::PopFont();
            ImGui::End();
        }
    }


    ////////////////////////////////////////////////////////////
    void drawStepBackground()
    {
        SFEX_PROFILE_SCOPE_AUTOLABEL();

        const za::Vec2uz gridSize{m_world.blockGrid.getWidth(), m_world.blockGrid.getHeight() - gridGraceY};

        m_rtGame.draw(
            za::RectangleShapeData{
                .position         = toDrawCoordinates(za::Vec2uz{0, gridGraceY}),
                .origin           = (drawBlockSize / 2.f).componentWiseFloor(),
                .fillColor        = {30, 30, 30},
                .outlineColor     = {35, 35, 35},
                .outlineThickness = 1.f,
                .size             = gridSize.toVec2f().componentWiseMul(drawBlockSize).addX(2.f).addY(2.f),
            },
            {.view = m_worldView});

        const auto dividerStartPos = toDrawCoordinates(za::Vec2uz{0, gridGraceY});

        for (za::SizeT x = 0u; x < m_world.blockGrid.getWidth() + 1u; ++x)
            for (za::SizeT y = 0u; y < m_world.blockGrid.getHeight() - gridGraceY + 1u; ++y)
            {
                m_rtGame.draw(m_textureAtlas.getTexture(),
                              {
                                  .position    = dividerStartPos - drawBlockSize + za::Vec2f{3.f, 3.f} +
                                                 za::Vec2uz{x, y}.toVec2f().componentWiseMul(drawBlockSize),
                                  .textureRect = m_txrDivider,
                              },
                              {
                                  .view   = m_worldView,
                                  .shader = &m_shader,
                              });
            }

        m_rtGame.draw(
            za::RectangleShapeData{
                .position         = toDrawCoordinates(za::Vec2uz{0, gridGraceY}) - za::Vec2f{1.f, 1.f},
                .origin           = (drawBlockSize / 2.f).componentWiseFloor(),
                .fillColor        = za::Color::Transparent,
                .outlineColor     = {35, 35, 35},
                .outlineThickness = 1.f,
                .size             = gridSize.toVec2f().componentWiseMul(drawBlockSize).addX(4.f).addY(4.f),
            },
            {.view = m_worldView});

        m_rtGame.draw(
            za::RectangleShapeData{
                .position         = toDrawCoordinates(za::Vec2uz{0, gridGraceY}) - za::Vec2f{3.f, 3.f},
                .origin           = (drawBlockSize / 2.f).componentWiseFloor(),
                .fillColor        = za::Color::Transparent,
                .outlineColor     = {135, 135, 135},
                .outlineThickness = 1.f,
                .size             = gridSize.toVec2f().componentWiseMul(drawBlockSize).addX(8.f).addY(8.f),
            },
            {.view = m_worldView});
    }


    ////////////////////////////////////////////////////////////
    void drawStepEmbeddedBlocks()
    {
        SFEX_PROFILE_SCOPE_AUTOLABEL();

        for (za::SizeT y = gridGraceY; y < m_world.blockGrid.getHeight(); ++y)
            for (za::SizeT x = 0u; x < m_world.blockGrid.getWidth(); ++x)
            {
                const za::Vec2uz gridPosition{x, y};
                const auto&      optBlock = m_world.blockGrid.at(gridPosition);

                if (!optBlock.hasValue())
                    continue;

                (void)drawBlock(*optBlock,
                                toDrawCoordinates(gridPosition),
                                {
                                    .opacity          = 1.f,
                                    .applyQuakeOffset = false,
                                    .drawTimer        = true,
                                });
            }
    }


    ////////////////////////////////////////////////////////////
    void drawStepFadingBlocks()
    {
        SFEX_PROFILE_SCOPE_AUTOLABEL();

        auto* fadeBlocks = m_animationTimelineP1.getIfPlaying<AnimFadeBlocks>();
        if (fadeBlocks == nullptr)
            return;

        const float progress = m_animationTimelineP1.getProgress();

        for (const auto& fadingBlock : fadeBlocks->fadingBlocks)
        {
            (void)drawBlock(fadingBlock.block,
                            toDrawCoordinates(fadingBlock.position),
                            {
                                .opacity          = 1.f,
                                .squishMult       = -easeInBackWithCustomOvershoot(progress, 6.f),
                                .applyYOffset     = false,
                                .applyQuakeOffset = false,
                                .drawTimer        = false,
                            });
        }
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] PaletteIdx getTetraminoPaletteIdx(const Tetramino& tetramino) const
    {
        for (const auto& b : tetramino.shape.data)
            if (b.hasValue())
                return b->paletteIdx;

        ZA_UNREACHABLE();
    }


    ////////////////////////////////////////////////////////////
    struct [[nodiscard]] DrillableBlocksInfo
    {
        int       count;
        za::Vec2i endPos;
    };


    ////////////////////////////////////////////////////////////
    [[nodiscard]] DrillableBlocksInfo countDrillableBlocks(const za::Vec2i startPos, const DrillDirection::Enum direction)
    {
        DrillableBlocksInfo info{
            .count  = 0,
            .endPos = startPos + drillDirectionToVec2i(direction),
        };

        const auto horizontalIteration = [&](const int iX) -> ControlFlow
        {
            auto& optBlock = m_world.blockGrid.at(za::Vec2i{iX, info.endPos.y});

            if (!optBlock.hasValue())
                return ControlFlow::Break;

            if (!optBlock->isDamageable())
                return ControlFlow::Continue;

            info.endPos.x = iX;

            if (++info.count >= m_world.perkDrill[direction]->maxPenetration)
                return ControlFlow::Break;

            return ControlFlow::Continue;
        };

        if (direction == DrillDirection::Left)
        {
            info.endPos.x = za::max(info.endPos.x, 0);

            for (int iX = info.endPos.x; iX >= 0; --iX)
                if (horizontalIteration(iX) == ControlFlow::Break)
                    break;
        }
        else if (direction == DrillDirection::Right)
        {
            const auto gridWidth = static_cast<int>(m_world.blockGrid.getWidth());
            info.endPos.x        = za::min(info.endPos.x, gridWidth - 1);

            for (int iX = info.endPos.x; iX < gridWidth; ++iX)
                if (horizontalIteration(iX) == ControlFlow::Break)
                    break;
        }
        else if (direction == DrillDirection::Down)
        {
            const auto gridHeight = static_cast<int>(m_world.blockGrid.getHeight());
            info.endPos.y         = za::min(info.endPos.y, gridHeight - 1);

            for (int iY = info.endPos.y; iY < gridHeight; ++iY)
            {
                auto& optBlock = m_world.blockGrid.at(za::Vec2i{info.endPos.x, iY});

                if (!optBlock.hasValue())
                    break;

                if (!optBlock->isDamageable())
                    continue;

                info.endPos.y = iY;

                if (++info.count >= m_world.perkDrill[DrillDirection::Down]->maxPenetration)
                    break;
            }
        }

        return info;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] LaserableBlocksInfo findLaserableBlocks(const za::Vec2i startPos, const LaserDirection::Enum direction)
    {
        LaserableBlocksInfo result;

        const auto dir = laserDirectionToVec2i(direction);
        za::Vec2i  pos = startPos + dir;

        while (m_world.blockGrid.isInBounds(pos))
        {
            auto& optBlock = m_world.blockGrid.at(pos);

            if (optBlock.hasValue() && optBlock->isDamageable())
                result.positions.emplaceBack(pos);

            pos += dir;
        }

        if (m_world.perkLaser[direction]->bounce)
        {
            const auto bounceDir = za::Vec2i{-dir.x, dir.y};

            pos += za::Vec2i{-dir.x, 0};
            result.bouncePos = pos;
            pos += bounceDir;

            while (m_world.blockGrid.isInBounds(pos))
            {
                auto& optBlock = m_world.blockGrid.at(pos);

                if (optBlock.hasValue() && optBlock->isDamageable())
                    result.bouncePositions.emplaceBack(pos);

                pos += bounceDir;
            }
        }

        return result;
    }


    ////////////////////////////////////////////////////////////
    void drawStepAnimDrill()
    {
        SFEX_PROFILE_SCOPE_AUTOLABEL();

        auto* drillAnim = m_animationTimelineP0.getIfPlaying<AnimDrill>();

        if (drillAnim == nullptr)
            return;

        drawStepActiveTetramino({
            .tetramino     = drillAnim->tetramino,
            .visualCenter  = toDrawCoordinates(drillAnim->tetramino.position) + drawBlockSize.toVec2f() * 2.f,
            .squishMult    = 0.f,
            .drawTetramino = false,
            .drawGhost     = false,
        });

        const za::Vec2f offsetByDirection[] = {
            {-drawBlockSize.x / 2.f - 3.f + 2.f, 1.f}, // Left
            {drawBlockSize.x - 1.f, 2.f},              // Right
            {1.f, drawBlockSize.y - 1.f},              // Down
        };

        const za::Angle arrayByDirection[] = {
            za::degrees(90.f),  // Left
            za::degrees(270.f), // Right
            za::degrees(0.f),   // Down
        };

        const auto drillDrawOffset = offsetByDirection[static_cast<za::SizeT>(drillAnim->direction)];
        const auto rotation        = arrayByDirection[static_cast<za::SizeT>(drillAnim->direction)];

        const auto& tetramino = drillAnim->tetramino;

        const float progress   = m_animationTimelineP0.getProgress();
        const auto  paletteIdx = getTetraminoPaletteIdx(tetramino);

        for (const auto bPos : findDrillBlocks(tetramino, drillAnim->direction))
        {
            const auto startPos = tetramino.position + bPos.toVec2i();

            const auto [nDrillableBlocks, endPos] = countDrillableBlocks(startPos, drillAnim->direction);

            if (nDrillableBlocks == 0)
                continue;

            const auto startDrawPos = toDrawCoordinates(startPos.toVec2uz());
            const auto endDrawPos   = toDrawCoordinates(endPos.toVec2uz());
            const auto diff         = endDrawPos - startDrawPos;

            const int  nDrills = 1 + static_cast<int>(diff.length() / 2.5f);
            const auto radius  = drawBlockSize.x;

            const auto getDrawPosition = [&](int i)
            {
                return drillDrawOffset +
                       blend(startDrawPos,
                             startDrawPos + ((diff / static_cast<float>(nDrills)) * static_cast<float>(i + 1)),
                             easeInOutSine(bounce(progress)));
            };

            const auto gridWidth  = m_world.blockGrid.getWidth();
            const auto gridHeight = m_world.blockGrid.getHeight();

            const auto lastDrawPos = getDrawPosition(nDrills - 1);
            const auto lastGridPos = toGridCoordinates(
                                         (lastDrawPos + za::Vec2f{radius / 2.f, radius / 2.f}).componentWiseFloor())
                                         .componentWiseClamp({0, 0}, za::Vec2uz{gridWidth - 1, gridHeight - 1}.toVec2i());

            const auto& optBlock = m_world.blockGrid.at(lastGridPos);


            if (optBlock.hasValue())
            {
                for (int i = 0; i < 2; ++i)
                    m_hueColorCircleShapeParticles.emplaceBack(CircleParticleData{
                        .position      = lastDrawPos - drillDrawOffset +
                                         drillDirectionToVec2i(drillAnim->direction).toVec2f() * (radius / 2.f) +
                                         m_rngFast.getVec2f({-3.f, -3.f}, {3.f, 3.f}),
                        .velocity      = m_rngFast.getVec2f({-0.75f, -2.15f}, {0.75f, -0.25f}) * 0.05f,
                        .scale         = m_rngFast.getF(0.08f, 0.27f) * 0.95f,
                        .scaleDecay    = 0.f,
                        .accelerationY = 0.0004f,
                        .opacity       = 0.95f,
                        .opacityDecay  = m_rngFast.getF(0.001f, 0.002f) * 0.5f,
                        .rotation      = m_rngFast.getF(0.f, za::tau),
                        .torque        = m_rngFast.getF(-0.001f, 0.001f),
                        .color         = hueColorFromPaletteIdx(optBlock->paletteIdx, 255u),
                        .radius        = m_rngFast.getF(6.f, 12.f),
                        .pointCount    = 3u,
                    });
            }

            for (int i = 0; i < nDrills; ++i)
            {
                m_rtGame.draw(m_textureAtlas.getTexture(),
                              {
                                  .position    = getDrawPosition(i).componentWiseFloor(),
                                  .origin      = za::Vec2f{radius / 2.f, radius / 2.f}.componentWiseFloor(),
                                  .rotation    = rotation,
                                  .textureRect = m_txrDrill,
                                  .color       = hueColorFromPaletteIdx(paletteIdx, 255u),
                              },
                              {.view = m_worldView, .shader = &m_shader});
            }
        }
    }


    ////////////////////////////////////////////////////////////
    void drawStepAnimLaser()
    {
        SFEX_PROFILE_SCOPE_AUTOLABEL();

        auto* laserAnim = m_animationTimelineP0.getIfPlaying<AnimLaser>();

        if (laserAnim == nullptr)
            return;

        if (!m_optLaserBeam.hasValue())
            return;

        drawStepActiveTetramino({
            .tetramino     = laserAnim->tetramino,
            .visualCenter  = toDrawCoordinates(laserAnim->tetramino.position) + drawBlockSize.toVec2f() * 2.f,
            .squishMult    = 0.f,
            .drawTetramino = false,
            .drawGhost     = false,
        });

        const auto targetPos = toDrawCoordinates(laserAnim->gridTargetPos) +
                               m_rngFast.getVec2f(-drawBlockSize, drawBlockSize) / 4.f;

        const auto progress = m_animationTimelineP0.getProgress();
        m_optLaserBeam->end = m_optLaserBeam->start + (targetPos - m_optLaserBeam->start) * easeInOutBack(progress);

        for (int i = 0; i < 3; ++i)
            m_fixedColorCircleShapeParticles.emplaceBack(CircleParticleData{
                .position      = m_optLaserBeam->end,
                .velocity      = m_rngFast.getVec2f({-0.75f, -2.15f}, {0.75f, -0.25f}) * 0.25f,
                .scale         = m_rngFast.getF(0.08f, 0.27f) * 0.75f,
                .scaleDecay    = 0.f,
                .accelerationY = 0.0004f,
                .opacity       = 0.75f,
                .opacityDecay  = m_rngFast.getF(0.001f, 0.002f) * 0.7f,
                .rotation      = m_rngFast.getF(0.f, za::tau),
                .torque        = m_rngFast.getF(-0.001f, 0.001f),
                .color         = blockPalette[getTetraminoPaletteIdx(laserAnim->tetramino)],
                .radius        = m_rngFast.getF(4.f, 7.f),
                .pointCount    = 5u,
            });
    }


    ////////////////////////////////////////////////////////////
    void drawStepAnimFadeAttachments()
    {
        SFEX_PROFILE_SCOPE_AUTOLABEL();

        auto* fadeAttachments = m_animationTimelineP0.getIfPlaying<AnimFadeAttachments>();

        if (fadeAttachments == nullptr)
            return;

        drawStepActiveTetramino({
            .tetramino     = fadeAttachments->tetramino,
            .visualCenter  = toDrawCoordinates(fadeAttachments->tetramino.position) + drawBlockSize.toVec2f() * 2.f,
            .squishMult    = 0.f - m_animationTimelineP0.getProgress(),
            .drawTetramino = false,
            .drawGhost     = false,
        });
    }


    ////////////////////////////////////////////////////////////
    static inline constexpr za::Vec2f tetraminoVisualCenterOffset = (drawBlockSize * static_cast<float>(shapeDimension)) / 2.f;


    ////////////////////////////////////////////////////////////
    template <typename T>
    [[nodiscard]] za::Vec2f getTetraminoCenterDrawPosition(const za::Vec2<T> tetraminoGridPosition) const
    {
        const za::Vec2f topLeftDrawPosition = toDrawCoordinates(tetraminoGridPosition);

        return topLeftDrawPosition + tetraminoVisualCenterOffset;
    }


    ////////////////////////////////////////////////////////////
    template <typename T>
    [[nodiscard]] za::Vec2f getDrawPositionOfLocalBlock(const za::Vec2<T>& localBlockGridPos,
                                                        const za::Vec2f    tetraminoCenter) const
    {
        const za::Vec2f localPivot = tetraminoVisualCenterOffset;

        const za::Vec2f localBlockCenter = localBlockGridPos.toVec2f().componentWiseMul(drawBlockSize) +
                                           (drawBlockSize / 2.f);

        const za::Vec2f positionRelativeToPivot = localBlockCenter - localPivot;

        return (tetraminoCenter + positionRelativeToPivot).componentWiseFloor();
    }


    ////////////////////////////////////////////////////////////
    void drawDrillSpikesForPerk(
        const ShapeBlockPositionVector& localBlockPositions,
        const za::Vec2f                 offset,
        const za::Color                 color,
        const za::Angle                 rotation,
        const za::Vec2f                 mainTetraminoCenter,
        const za::Vec2f                 ghostTetraminoCenter,
        const bool                      drawGhost,
        const float                     squishMult)
    {
        const auto mainColor  = color;
        const auto ghostColor = mainColor.withAlpha(64);

        float finalSquishMult = 1.f + squishMult;

        za::Sprite spike{
            .scale       = za::Vec2f{finalSquishMult, finalSquishMult},
            .origin      = za::Vec2f{drawBlockSize.x / 2.f, drawBlockSize.y / 2.f}.componentWiseFloor(),
            .rotation    = rotation,
            .textureRect = m_txrDrill,
        };

        for (const auto& bPos : localBlockPositions)
        {
            const za::Vec2f mainBlockDrawPos  = getDrawPositionOfLocalBlock(bPos, mainTetraminoCenter);
            const za::Vec2f ghostBlockDrawPos = getDrawPositionOfLocalBlock(bPos, ghostTetraminoCenter);

            // Draw main spike
            spike.position = (offset + mainBlockDrawPos.addX(za::floor(-drawBlockSize.x / 2.f))).componentWiseFloor();
            spike.color    = mainColor;
            m_rtGame.draw(spike, {.view = m_worldView, .texture = &m_textureAtlas.getTexture(), .shader = &m_shader});

            // Draw ghost spike
            if (drawGhost)
            {
                spike.position = (offset + ghostBlockDrawPos.addY(za::floor(drawBlockSize.y / 2.f))).componentWiseFloor() -
                                 za::Vec2f{1.f, 1.f};
                spike.color = ghostColor;
                m_rtGame.draw(spike, {.view = m_worldView, .texture = &m_textureAtlas.getTexture(), .shader = &m_shader});
            }
        }
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Vec2f calculateLaserGridIntersection(const za::Vec2f startPos, const LaserDirection::Enum direction) const
    {
        // Define grid boundaries in draw coordinates. The playable grid area starts below gridGraceY.
        const float left  = toDrawCoordinates(za::Vec2f{0.f, 0.f}).x;
        const float right = toDrawCoordinates(za::Vec2f{static_cast<float>(m_world.blockGrid.getWidth() - 1), 0.f}).x;
        const float top   = toDrawCoordinates(za::Vec2f{0.f, static_cast<float>(gridGraceY)}).y;
        const float bottom = toDrawCoordinates(za::Vec2f{0.f, static_cast<float>(m_world.blockGrid.getHeight() - 1)}).y + 1.f;

        // Define the ray using its starting position and direction vector in draw space.
        const za::Vec2f rayDirGrid = laserDirectionToVec2i(direction).toVec2f();
        const za::Vec2f rayDirDraw = rayDirGrid.componentWiseMul(drawBlockSize);

        // Avoid division by zero if direction is a zero vector.
        if (rayDirDraw.x == 0.f && rayDirDraw.y == 0.f)
            return startPos;

        float minT = 10000.f;

        // Calculate the parametric 't' value for intersection with each of the four boundary lines.
        // A valid intersection occurs if t > 0 (in front of the ray) and the intersection
        // point lies on the boundary segment. The smallest valid 't' is the first point of impact.

        // Check intersection with vertical walls (left/right).
        if (rayDirDraw.x != 0.f)
        {
            if (direction == LaserDirection::Left)
            {
                const float tLeft = (left - startPos.x) / rayDirDraw.x;
                if (tLeft > 0.f)
                {
                    const float y = startPos.y + tLeft * rayDirDraw.y;
                    if (y >= top && y <= bottom)
                        minT = za::min(minT, tLeft);
                }
            }
            else
            {
                const float tRight = (right - startPos.x) / rayDirDraw.x;
                if (tRight > 0.f)
                {
                    const float y = startPos.y + tRight * rayDirDraw.y;
                    if (y >= top && y <= bottom)
                        minT = za::min(minT, tRight);
                }
            }
        }

        // Check intersection with horizontal walls (top/bottom).
        if (rayDirDraw.y != 0.f)
        {
            const float tBottom = (bottom - startPos.y) / rayDirDraw.y;
            if (tBottom > 0.f)
            {
                const float x = startPos.x + tBottom * rayDirDraw.x;
                if (x >= left && x <= right)
                    minT = za::min(minT, tBottom);
            }
        }

        // If a valid intersection was found (minT is not infinity), calculate the precise point.
        if (minT < 10000.f)
            return startPos + minT * rayDirDraw;

        // Fallback: If no intersection is found (e.g., ray points away from grid),
        // return a point far along the ray's direction.
        return startPos + 5.f * rayDirDraw;
    }


    ////////////////////////////////////////////////////////////
    void drawLaserEmittersForPerk(
        const ShapeBlockPositionVector& localBlockPositions,
        const za::Vec2f                 offset,
        const za::Color                 color,
        const za::Angle                 rotation,
        const za::Vec2f                 mainTetraminoCenter,
        const za::Vec2f                 ghostTetraminoCenter,
        const bool                      drawGhost,
        const float                     squishMult)
    {
        const auto mainColor  = color;
        const auto ghostColor = mainColor.withAlpha(64);

        const auto laserDirection = rotation == za::degrees(45.f) ? LaserDirection::Left : LaserDirection::Right;
        const auto laserDir       = laserDirectionToVec2i(laserDirection);

        float finalSquishMult = 1.f + squishMult;

        za::Sprite spike{
            .scale = za::Vec2f{finalSquishMult, finalSquishMult},
            .origin = (za::Vec2f{drawBlockSize.x / 2.f, drawBlockSize.y / 2.f} - za::Vec2f{2.f, 2.f}).componentWiseFloor(),
            .rotation    = rotation + za::degrees(180.f + 45.f),
            .textureRect = m_txrEmitter,
        };

        const auto drawGuide = [this, &mainColor](const za::Vec2f            startPos,
                                                  const LaserDirection::Enum guideLaserDirection,
                                                  const za::Angle            guideRotation)
        {
            const auto guideLaserDir = laserDirectionToVec2i(guideLaserDirection);
            const auto guideOffset   = guideLaserDir.toVec2f() * 2.f;

            auto endPos = calculateLaserGridIntersection(startPos - guideLaserDir.toVec2f() * 6.f, guideLaserDirection) +
                          guideLaserDir.toVec2f() * 5.f - guideOffset;

            if (guideLaserDirection == LaserDirection::Right)
                endPos += guideLaserDir.toVec2f() * 3.f;

            za::RectangleShape guide{{
                .rotation    = guideRotation.wrapUnsigned(),
                .textureRect = m_txrRedDot,
                .size        = za::Vec2f{1.f, (endPos - startPos).length()},
            }};

            const auto guideAnchorPoint = guideLaserDirection == LaserDirection::Left ? za::Vec2f{1.f, 0.f} // top-right
                                                                                      : za::Vec2f{0.f, 0.f}; // top-left

            guide.setFillColor(mainColor.withAlpha(32));
            guide.setSize({1.f, (endPos - startPos).length()});
            guide.setGlobalAnchorPoint(guideAnchorPoint, startPos + guideOffset);
            m_rtGame.draw(guide, {.view = m_worldView, .texture = &m_textureAtlas.getTexture(), .shader = &m_shader});

            guide.setFillColor(mainColor.withAlpha(16));
            guide.setSize({2.f, (endPos - startPos).length()});
            guide.setGlobalAnchorPoint(guideAnchorPoint, startPos + guideOffset);
            m_rtGame.draw(guide, {.view = m_worldView, .texture = &m_textureAtlas.getTexture(), .shader = &m_shader});

            return endPos;
        };

        for (const auto& bPos : localBlockPositions)
        {
            const za::Vec2f mainBlockDrawPos  = getDrawPositionOfLocalBlock(bPos, mainTetraminoCenter);
            const za::Vec2f ghostBlockDrawPos = getDrawPositionOfLocalBlock(bPos, ghostTetraminoCenter);

            const auto mainSpikePos = (offset + mainBlockDrawPos.addX(za::floor(-drawBlockSize.x / 2.f))).componentWiseFloor();

            // Draw main spike
            spike.position = mainSpikePos + (laserDir * 4).toVec2f();
            spike.color    = mainColor;
            m_rtGame.draw(spike, {.view = m_worldView, .texture = &m_textureAtlas.getTexture(), .shader = &m_shader});

            if (!drawGhost)
                continue;

            const auto ghostSpikePos = (offset + ghostBlockDrawPos.addY(za::floor(drawBlockSize.y / 2.f))).componentWiseFloor() -
                                       za::Vec2f{1.f, 1.f};

            // Draw ghost spike
            spike.position = ghostSpikePos + (laserDir * 4).toVec2f();
            spike.color    = ghostColor;
            m_rtGame.draw(spike, {.view = m_worldView, .texture = &m_textureAtlas.getTexture(), .shader = &m_shader});

            const auto endPos = drawGuide(ghostSpikePos, laserDirection, rotation);

            if (m_world.perkLaser[laserDirection]->bounce)
            {
                const auto bounceOffset = laserDirection == LaserDirection::Left ? za::Vec2f{-4.f, -2.f}
                                                                                 : za::Vec2f{4.f, -2.f};

                (void)drawGuide(endPos + bounceOffset,
                                laserDirection == LaserDirection::Left ? LaserDirection::Right : LaserDirection::Left,
                                za::degrees(180.f) - rotation);
            }
        }
    }


    ////////////////////////////////////////////////////////////
    void drawStepAnimHardDrop()
    {
        SFEX_PROFILE_SCOPE_AUTOLABEL();

        auto* hardDrop = m_animationTimelineP1.getIfPlaying<AnimHardDrop>();

        if (hardDrop == nullptr)
            return;

        const float progress = m_animationTimelineP1.getProgress();

        const float targetVisualY = toDrawCoordinates(hardDrop->tetramino.position.withY(hardDrop->endY)).y +
                                    drawBlockSize.y / 2.f;

        const float startVisualY = toDrawCoordinates(hardDrop->tetramino.position).y + drawBlockSize.y * 2.f;

        const auto visualCenterY = startVisualY + (targetVisualY - startVisualY) * easeInBack(progress);
        const auto visualCenterX = toDrawCoordinates(hardDrop->tetramino.position).x + drawBlockSize.x * 2.f;

        drawStepActiveTetramino({
            .tetramino     = hardDrop->tetramino,
            .visualCenter  = {visualCenterX, visualCenterY},
            .squishMult    = 0.f,
            .drawTetramino = true,
            .drawGhost     = false,
        });
    }


    ////////////////////////////////////////////////////////////
    void drawStepAnimSquish()
    {
        SFEX_PROFILE_SCOPE_AUTOLABEL();

        auto* squish = m_animationTimelineP1.getIfPlaying<AnimSquish>();

        if (squish == nullptr)
            return;

        drawStepActiveTetramino({
            .tetramino     = squish->tetramino,
            .visualCenter  = toDrawCoordinates(squish->tetramino.position) + drawBlockSize.toVec2f() * 2.f,
            .squishMult    = easeInOutSine(bounce(m_animationTimelineP1.getProgress())) * 0.5f,
            .drawTetramino = true,
            .drawGhost     = false,
        });
    }


    ////////////////////////////////////////////////////////////
    struct [[nodiscard]] DrawActiveTetraminoOptions // NOLINT(cppcoreguidelines-pro-type-member-init)
    {
        Tetramino& tetramino;
        za::Vec2f  visualCenter;
        float      squishMult;
        bool       drawTetramino;
        bool       drawGhost;
    };


    ////////////////////////////////////////////////////////////
    void drawStepActiveTetramino(const DrawActiveTetraminoOptions& options)
    {
        const auto& [tetramino, visualCenter, squishMult, mustDrawTetramino, mustDrawGhost] = options;

        const auto color = hueColorFromPaletteIdx(getTetraminoPaletteIdx(tetramino), 255u);

        auto tetraminoDrawPosition = (visualCenter - drawBlockSize / 2.f).componentWiseFloor() + za::Vec2f{1, 1};

        const za::Vec2f ghostGridPosition = tetramino.position.toVec2f().withY(
            static_cast<float>(calculateGhostY(tetramino)));

        const za::Vec2f ghostCenterDrawPosition = getTetraminoCenterDrawPosition(ghostGridPosition)
                                                      .withX(tetraminoDrawPosition.x)
                                                      .addY(-drawBlockSize.y / 2.f)
                                                      .addY(1.f);

        if (m_world.perkDrill[DrillDirection::Down].hasValue())
            drawDrillSpikesForPerk(findDrillBlocks(tetramino, DrillDirection::Down),
                                   za::Vec2f{0.f + 2.f, drawBlockSize.y / 2.f + 2.f}.componentWiseFloor(),
                                   color,
                                   za::degrees(0.f),
                                   visualCenter,
                                   ghostCenterDrawPosition,
                                   mustDrawGhost,
                                   squishMult);

        if (m_world.perkDrill[DrillDirection::Left].hasValue())
            drawDrillSpikesForPerk(findDrillBlocks(tetramino, DrillDirection::Left),
                                   za::Vec2f{-drawBlockSize.x / 2.f - 2.f, -drawBlockSize.y / 2.f + 2.f}.componentWiseFloor(),
                                   color,
                                   za::degrees(90.f),
                                   visualCenter,
                                   ghostCenterDrawPosition,
                                   mustDrawGhost,
                                   squishMult);

        if (m_world.perkDrill[DrillDirection::Right].hasValue())
            drawDrillSpikesForPerk(findDrillBlocks(tetramino, DrillDirection::Right),
                                   za::Vec2f{drawBlockSize.x + 2.f, -1.f - 2.f}.componentWiseFloor(),
                                   color,
                                   za::degrees(270.f),
                                   visualCenter,
                                   ghostCenterDrawPosition,
                                   mustDrawGhost,
                                   squishMult);

        if (mustDrawTetramino)
            (void)drawTetramino(tetramino.shape,
                                tetraminoDrawPosition,
                                {
                                    .squishMult = squishMult,
                                    .drawTimer  = true,
                                });

        if (mustDrawGhost)
            (void)drawTetramino(tetramino.shape,
                                ghostCenterDrawPosition,
                                {
                                    .opacity    = 0.25f,
                                    .squishMult = squishMult,
                                    .drawTimer  = false,
                                });


        if (m_world.perkLaser[LaserDirection::Left].hasValue())
            drawLaserEmittersForPerk(findLaserBlocks(tetramino, LaserDirection::Left),
                                     {0.f, -1.f},
                                     color,
                                     za::degrees(45.f),
                                     visualCenter,
                                     ghostCenterDrawPosition,
                                     mustDrawGhost,
                                     squishMult);

        if (m_world.perkLaser[LaserDirection::Right].hasValue())
            drawLaserEmittersForPerk(findLaserBlocks(tetramino, LaserDirection::Right),
                                     za::Vec2f{drawBlockSize.x / 2.f, -1.f}.componentWiseFloor(),
                                     color,
                                     za::degrees(315.f),
                                     visualCenter,
                                     ghostCenterDrawPosition,
                                     mustDrawGhost,
                                     squishMult);
    }


    ////////////////////////////////////////////////////////////
    void drawStepUINextTetraminos()
    {
        SFEX_PROFILE_SCOPE_AUTOLABEL();

        const za::SizeT nPeek = za::min(static_cast<za::SizeT>(m_world.perkNPeek), m_world.blockMatrixBag.size());

        constexpr float uiTetraminoScale = 9.f / drawBlockSize.x;

        const auto hudPos = getHudPos();

        za::Vec2f uiBoxCenter = za::Vec2f{hudPos.x + 12.f, hudPos.y + 68.f + 24.f} + za::Vec2f{16.f, 16.f};

        for (za::SizeT iPeek = 0u; iPeek < nPeek; ++iPeek)
        {
            const auto& shape = m_world.blockMatrixBag[iPeek].blockMatrix;

            const auto globalBounds = drawTetramino(shape,
                                                    uiBoxCenter,
                                                    {
                                                        .opacity = 1.f - static_cast<float>(iPeek) * 0.1f,
                                                        .scale   = uiTetraminoScale - static_cast<float>(iPeek) * 0.05f,
                                                        .drawText         = true,
                                                        .applyYOffset     = false,
                                                        .applyQuakeOffset = false,
                                                        .drawTimer        = false,
                                                    });

            uiBoxCenter.x = globalBounds.getCenterRight().x + 24.f;
            uiBoxCenter.y -= static_cast<float>(iPeek) * 0.1f;
        }
    }


    ////////////////////////////////////////////////////////////
    void drawStepUIHeldTetramino()
    {
        SFEX_PROFILE_SCOPE_AUTOLABEL();

        if (!m_world.heldTetramino.hasValue())
            return;

        const auto&     shape       = m_world.heldTetramino->shape;
        const auto      hudPos      = getHudPos();
        const za::Vec2f uiBoxCenter = {hudPos.x + 128.f + 10.f, hudPos.y};

        (void)drawTetramino(shape, uiBoxCenter + za::Vec2f{20.f, 48.f});
    }


    ////////////////////////////////////////////////////////////
    void drawStepLightningBolts()
    {
        SFEX_PROFILE_SCOPE_AUTOLABEL();

        for (auto& lb : m_lightningBolts)
            lb.draw(m_rtGame, {.blendMode = za::BlendAdd, .view = m_worldView});

        if (m_optLaserBeam.hasValue())
        {
            m_optLaserBeam->draw(m_rtGame, {.blendMode = za::BlendAdd, .view = m_worldView});
            m_optLaserBeam->draw(m_rtGame, {.blendMode = za::BlendAdd, .view = m_worldView});
        }
    }


    ////////////////////////////////////////////////////////////
    void drawStepEarnedXPParticles()
    {
        SFEX_PROFILE_SCOPE_AUTOLABEL();

        const auto bezier = [](const za::Vec2f start, const za::Vec2f end, const float t)
        {
            const za::Vec2f control(start.x, end.y);
            const float     u = 1.f - t;

            return u * u * start + 2.f * u * t * control + t * t * end;
        };

        for (const auto& particle : m_earnedXPParticles)
        {
            const auto newPos = bezier(particle.startPosition, particle.targetPosition, easeInOutSine(particle.progress.value));

            const auto newPos2 = bezier(particle.startPosition,
                                        particle.targetPosition,
                                        easeInOutBack(particle.progress.value));

            const auto alpha = static_cast<za::U8>((particle.progress.remapBouncedEased(easeInOutQuint, 64.f, 255.f)));

            m_rtGame.draw(
                za::CircleShapeData{
                    .position = {blend(newPos2.x, newPos.x, 0.5f), newPos.y},
                    .scale    = za::Vec2f{0.25f, 0.25f} * particle.progress.remapBounced(0.6f, 2.f),
                    .origin   = {12.f, 12.f},
                    .rotation = za::radians(
                        za::fmod(particle.startRotation + particle.progress.remap(0.f, za::tau * 2.f), za::tau)),
                    .textureRect = m_txrRedDot,
                    .fillColor   = hueColorFromPaletteIdx(particle.paletteIdx, alpha),
                    .radius      = 12.f,
                    .pointCount  = 3u,
                },
                {
                    .view    = m_worldView,
                    .texture = &m_textureAtlas.getTexture(),
                    .shader  = &m_shader,
                });
        }
    }


    ////////////////////////////////////////////////////////////
    void drawStepParticleData()
    {
        SFEX_PROFILE_SCOPE_AUTOLABEL();

        for (const auto& particle : m_hueColorCircleShapeParticles)
            m_rtGame.draw(particleToCircleData(particle),
                          {.view = m_worldView, .texture = &m_textureAtlas.getTexture(), .shader = &m_shader});

        for (const auto& particle : m_fixedColorCircleShapeParticles)
            m_rtGame.draw(particleToCircleData(particle), {.view = m_worldView});

        for (const auto& particle : m_fixedColorSpriteParticles)
            m_rtGame.draw(particleToSpriteData(particle), {.view = m_worldView, .texture = &m_textureAtlas.getTexture()});
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Vec2f getHudPos()
    {
        const za::Vec2uz gridSize{m_world.blockGrid.getWidth(), m_world.blockGrid.getHeight() - gridGraceY};
        const auto       bgPosition = toDrawCoordinates(za::Vec2uz{0, gridGraceY}) - za::Vec2f{1.f, 1.f};
        const auto       bgSize     = gridSize.toVec2f().componentWiseMul(drawBlockSize).addX(4.f).addY(4.f);

        const auto hudStart = bgPosition + za::Vec2f{bgSize.x + 4.f, -11.f};

        return hudStart;
    }


    ////////////////////////////////////////////////////////////
    void drawStepStatsText()
    {
        SFEX_PROFILE_SCOPE_AUTOLABEL();

        za::RectangleShape statsBorder{{
            .position         = getHudPos().addY(4.f).addX(-1.f),
            .fillColor        = za::Color::Transparent,
            .outlineColor     = {135, 135, 135},
            .outlineThickness = 1.f,
            .size             = {128.f, 64.f},
        }};

        m_rtGame.draw(statsBorder, {.view = m_worldView});

        m_textVerticesBuffer.clear();
        m_textIndicesBuffer.clear();

        auto statsStr = za::fmtToString(
            "^bold[](Level)^: {}\n"
            "^bold[](XP)^: {} / {}\n"
            "^bold[](Clock)^: {}s\n"
            "^bold[](Lines Cleared)^: {}\n"
            "^bold[](Pieces Placed)^: {}\n"
            "^bold[](Difficulty)^: {}",

            m_world.playerLevel,
            m_world.currentXP,
            getXPNeededForLevelUp(m_world.playerLevel),
            getElapsedSeconds(m_world.tick),
            m_world.linesCleared,
            m_world.tetaminosPlaced,
            getDifficultyFactor(m_world.tick));


        (void)bitmapTextToVerticesPretransformed(
            {
                .outVertices     = m_textVerticesBuffer,
                .outIndices      = m_textIndicesBuffer,
                .bitmapFont      = m_bitmapFontMinogram,
                .fontTextureRect = m_txrBFMinogram6x10,
                .alignment       = BitmapTextAlignment::Left,
                .baseColor       = za::Color::White,
                .time            = m_totalTime,
                .string          = statsStr,
            },
            za::Transform::fromPosition(statsBorder.getGlobalTopLeft() + za::Vec2f{4.f, 2.f}));

        m_rtGame.drawIndexedVertices(
            za::DrawIndexedVerticesSettings{
                .vertexSpan    = m_textVerticesBuffer,
                .indexSpan     = m_textIndicesBuffer,
                .primitiveType = za::PrimitiveType::Triangles,
            },
            {.view = m_worldView, .texture = &m_textureAtlas.getTexture()});
    }


    ////////////////////////////////////////////////////////////
    void drawStepPerksText()
    {
        SFEX_PROFILE_SCOPE_AUTOLABEL();

        za::RectangleShape statsBorder{{
            .position         = getHudPos().addY(168.f - 48.f).addX(-1.f),
            .fillColor        = za::Color::Transparent,
            .outlineColor     = {135, 135, 135},
            .outlineThickness = 1.f,
            .size             = {184.f, 64.f + 48.f},
        }};

        m_rtGame.draw(statsBorder, {.view = m_worldView});

        za::String perksStr;

        for (const auto& perk : m_perks)
            if (perk->isActive(m_world))
                perksStr += za::fmtToString("- {} {}\n", perk->getName(), perk->getInventoryStr(m_world));

        m_textVerticesBuffer.clear();
        m_textIndicesBuffer.clear();

        (void)bitmapTextToVerticesPretransformed(
            {
                .outVertices     = m_textVerticesBuffer,
                .outIndices      = m_textIndicesBuffer,
                .bitmapFont      = m_bitmapFontTiny5,
                .fontTextureRect = m_txrBFTiny5,
                .alignment       = BitmapTextAlignment::Left,
                .baseColor       = za::Color::White,
                .time            = m_totalTime,
                .string          = perksStr,
            },
            za::Transform::fromPosition(statsBorder.getGlobalTopLeft() + za::Vec2f{4.f, 2.f}));

        m_rtGame.drawIndexedVertices(
            za::DrawIndexedVerticesSettings{
                .vertexSpan    = m_textVerticesBuffer,
                .indexSpan     = m_textIndicesBuffer,
                .primitiveType = za::PrimitiveType::Triangles,
            },
            {.view = m_worldView, .texture = &m_textureAtlas.getTexture()});
    }


    ////////////////////////////////////////////////////////////
    void drawStep()
    {
        SFEX_PROFILE_SCOPE("draw");

        {
            SFEX_PROFILE_SCOPE("syncShaderUniforms");
            syncShaderUniforms();
        }

        m_rtGame.clear({9u, 9u, 9u});

        {
            SFEX_PROFILE_SCOPE("rtGame");

            drawStepBackground();
            drawStepEmbeddedBlocks();
            drawStepFadingBlocks();

            drawStepAnimHardDrop();
            drawStepAnimSquish();

            if (m_world.currentTetramino.hasValue())
                drawStepActiveTetramino({
                    .tetramino     = *m_world.currentTetramino,
                    .visualCenter  = m_currentTetraminoVisualCenter,
                    .squishMult    = 0.f,
                    .drawTetramino = true,
                    .drawGhost     = true,
                });

            drawStepEarnedXPParticles();
            drawStepParticleData();

            drawStepAnimDrill();
            drawStepAnimLaser();
            drawStepAnimFadeAttachments();

            drawStepUINextTetraminos();
            drawStepUIHeldTetramino();
            drawStepLightningBolts();

            drawStepStatsText();
            drawStepPerksText();

            if (m_world.perkNPeek > 0)
                m_rtGame.draw(m_font,
                              za::TextData{
                                  .position      = getHudPos().addY(68.f).addX(4.f),
                                  .string        = "Next:",
                                  .characterSize = 16u,
                                  .outlineColor  = za::Color::White,
                              },
                              {.view = m_worldView});

            if (m_world.perkCanHoldTetramino == 1)
                m_rtGame.draw(m_font,
                              za::TextData{
                                  .position      = getHudPos().addX(128.f + 10.f),
                                  .string        = "Held:",
                                  .characterSize = 16u,
                                  .outlineColor  = za::Color::White,
                              },
                              {.view = m_worldView});

            drawShop();
        }

        {
            SFEX_PROFILE_SCOPE("display rtgame");
            m_rtGame.display();
        }

        const auto screenShake = m_rngFast.getVec2f({-m_screenShakeAmount, -m_screenShakeAmount},
                                                    {m_screenShakeAmount, m_screenShakeAmount});


        float scale = 1.f;

        // figure out scale that fits in the window (pixel-perfect)
        {
            const za::Vec2f windowSize = m_window.getSize().toVec2f();
            const za::Vec2f rtGameSize = m_rtGame.getSize().toVec2f();

            scale = za::floor(za::min(windowSize.x / rtGameSize.x, windowSize.y / rtGameSize.y));
        }

        // figure out position to center the rtGame texture in the window
        const za::Vec2f windowSize = m_window.getSize().toVec2f();
        const za::Vec2f rtGameSize = m_rtGame.getSize().toVec2f() * scale;

        if (m_rtPostProcess.getSize() != rtGameSize.toVec2u())
            m_rtPostProcess = za::RenderTexture::create(rtGameSize.toVec2u()).value();

        {
            SFEX_PROFILE_SCOPE("postprocess");

            const auto postProcessInternalView = za::View::fromScreenSize(m_rtGame.getSize().toVec2f());

            m_rtPostProcess.flush();

            m_shaderCRT.setUniform(m_ulInputSize, m_rtGame.getSize().toVec2f());
            m_shaderCRT.setUniform(m_ulTime, m_totalTime);

            m_rtPostProcess.clear();
            m_rtPostProcess.draw(m_rtGame.getTexture(),
                                 {.view = postProcessInternalView, .shader = m_useCRTShader ? &m_shaderCRT : nullptr});
            m_rtPostProcess.display();
        }

        const za::Vec2f centeredPosition = (windowSize - rtGameSize) / 2.f;
        const float     quakeYOffset     = m_quakeSinEffectHardDrop.getValue() + m_quakeSinEffectLineClear.getValue();
        const za::Vec2f finalPosition    = (centeredPosition + screenShake.addY(quakeYOffset)).componentWiseFloor();

        {
            SFEX_PROFILE_SCOPE("final draw");

            m_window.clear();
            m_window.draw(m_rtPostProcess.getTexture(),
                          {.position = finalPosition},
                          {.view = m_windowView, .shader = &m_shaderPostProcess});
        }

        {
            SFEX_PROFILE_SCOPE("imgui");

            m_imGuiContext.render(m_window); // TODO P0: causes gl error in emscripten
        }

        {
            SFEX_PROFILE_SCOPE("window display");
            m_window.display();
        }
    }


public:
    ////////////////////////////////////////////////////////////
    Game()
    {
        m_font.getTexture().setSmooth(false);      // TODO P1: add Font::openFromXXX overloads with TextureSettings
        m_fontMago2.getTexture().setSmooth(false); // TODO P1: add Font::openFromXXX overloads with TextureSettings

        float scale = 4.f;

        m_window.setSize((resolution * scale).toVec2u());
        m_window.setPosition(
            (za::VideoModeUtils::getDesktopMode().size / 2u - (resolution * (scale * 0.5f)).toVec2u()).toVec2i());

        restartGame();
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool run()
    {
        while (true)
        {
            if (eventStep() == ControlFlow::Break)
                return false;

            const auto deltaTime = m_tickClock.restart();

            m_totalTime += deltaTime.asSeconds();

            if (isInPlayableState())
                m_timeAccumulator += deltaTime.asSeconds();

            updateStep(deltaTime, ticksPerSecond);
            imguiStep(deltaTime);
            drawStep();
        }

        return true;
    }
};

} // namespace tsurv


////////////////////////////////////////////////////////////
int main()
{
    auto audioContext    = za::AudioContext::create().value();
    auto graphicsContext = za::GraphicsContext::create().value();

    auto game = za::makeUnique<tsurv::Game>();

    if (!game->run())
        return 1;

    return 0;
}


// TODO:
// - combo system for line clears
// - random powerups
// - bombs?
// - every X damage deal, remove line from bottom
// - rerolls
// - perk to choose one more perk
// - lightning = random block
// - fire = random row
// - water = random column
// - earth = janitor?
// - make everything more chance based? roll func with luck parameter?
// - every time drill, lightning% chance
// - max perks
// - remove CC
// - damaging a block damages a random adjacent block
// - diagonal left/right lasers + bounce perk
// - separate weapon perks from utility perks
// - change of drill block, change of lightning block, chance of laser block
// - count combo clears and related perks
// - bag manipulation perks (e.g. never get same piece twice in a row)
// - multi hold?
// - dynamic perk system?
//      - triggers:
//            - every N ...
//                - any/specific tetromino place
//                - 1/2/3/4 lines clear
//                - drill hit
//                - laser hit
//                - block destroy
//                - block damage
//                - combo
//                - lightning
//                - power up collected
//                - gap filled
//      - effects
//           - X% chance of...
//               - lightning bolt
//               - fill a gap
//               - gain xp
//               - remove a block from last line
//               - remove last line
//               - turn random block into power up
//               - shift line l/r
//               - compact random column
// - concrete filler weapon instead of drill?
// - separate weapons and items, items must be bought with gold and are RNG based
// - finalize main mechanics, then work on items/perks/synergies
// - timed block +1hp
// - timed powerups
// - perks that affect the timers
// - tutorial modals
// - active perks/items with cooldowns or restrictions (e.g. one-time undo)

// - try linking libstdc++ statically and using LTO
