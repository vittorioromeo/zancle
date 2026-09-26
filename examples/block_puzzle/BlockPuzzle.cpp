#include "ExampleProfiler/Profiler.hpp"
#include "ExampleProfiler/ProfilerImGui.hpp"

#include "ExampleUtils/ControlFlow.hpp"
#include "ExampleUtils/Easing.hpp"
#include "ExampleUtils/HueColor.hpp"
#include "ExampleUtils/MathUtils.hpp"
#include "ExampleUtils/RNGFast.hpp"
#include "ExampleUtils/Scaling.hpp"

#include "Zancle/Audio/AudioContext.hpp"
#include "Zancle/Audio/Music.hpp"
#include "Zancle/Audio/MusicReader.hpp"
#include "Zancle/Audio/Sound.hpp"
#include "Zancle/Audio/SoundBuffer.hpp"

#include "Zancle/Graphics/BlendMode.hpp"
#include "Zancle/Graphics/Color.hpp"
#include "Zancle/Graphics/CurvedArrowShapeData.hpp"
#include "Zancle/Graphics/DrawableBatch.hpp"
#include "Zancle/Graphics/Font.hpp"
#include "Zancle/Graphics/Glsl.hpp"
#include "Zancle/Graphics/GraphicsContext.hpp"
#include "Zancle/Graphics/Image.hpp"
#include "Zancle/Graphics/RectangleShapeData.hpp"
#include "Zancle/Graphics/RenderStates.hpp"
#include "Zancle/Graphics/RenderTarget.hpp"
#include "Zancle/Graphics/RenderTexture.hpp"
#include "Zancle/Graphics/RenderWindow.hpp"
#include "Zancle/Graphics/Shader.hpp"
#include "Zancle/Graphics/Sprite.hpp"
#include "Zancle/Graphics/Text.hpp"
#include "Zancle/Graphics/Texture.hpp"
#include "Zancle/Graphics/TextureAtlas.hpp"
#include "Zancle/Graphics/View.hpp"

#include "Zancle/ImGui/ImGuiContext.hpp"
#include "Zancle/ImGui/IncludeImGui.hpp"

#include "Zancle/Window/Cursor.hpp"
#include "Zancle/Window/Event.hpp"
#include "Zancle/Window/EventUtils.hpp"
#include "Zancle/Window/Keyboard.hpp"
#include "Zancle/Window/Mouse.hpp"

#include "Zancle/IO/Path.hpp"

#include "Zancle/Algorithm/Erase.hpp"
#include "Zancle/Algorithm/Sort.hpp"
#include "Zancle/Algorithm/Unique.hpp"

#include "Zancle/Chrono/Clock.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Geometry/Angle.hpp"
#include "Zancle/Geometry/Rect2.hpp"
#include "Zancle/Geometry/Vec2.hpp"

#include "Zancle/Vocabulary/Optional.hpp"
#include "Zancle/Vocabulary/Variant.hpp"

#include "Zancle/Math/Ceil.hpp"
#include "Zancle/Math/Clamp.hpp"
#include "Zancle/Math/Constants.hpp"
#include "Zancle/Math/Fabs.hpp"
#include "Zancle/Math/Fmod.hpp"
#include "Zancle/Math/MinMax.hpp"

#include "Zancle/Base/Abort.hpp"
#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/GetArraySize.hpp"
#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/Macros.hpp"
#include "Zancle/Base/SizeT.hpp"

// TODO P2:
// - keys should have different colors compared to blocks
// - kill blocks in lava
// - portals
// - 4x1 surround kill

namespace
{
////////////////////////////////////////////////////////////
constexpr za::Vec2f baseResolution{1024.f, 768.f};


////////////////////////////////////////////////////////////
constexpr float     zoomFactor        = 2.f;
constexpr za::Vec2f logicalResolution = baseResolution * zoomFactor;


////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr float bounce(const float value) noexcept
{
    // return 4.f * value * (1.f - value);
    return 1.f - za::fabs(value - 0.5f) * 2.f;
}


////////////////////////////////////////////////////////////
enum class BlockKind : za::U8
{
    A,
    B,
    C,
};


////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline, gnu::const]] constexpr za::Color kindToColor(const BlockKind kind) noexcept
{
    if (kind == BlockKind::A)
        return za::Color::Red;

    if (kind == BlockKind::B)
        return za::Color::Green;

    ZA_ASSERT(kind == BlockKind::C);
    return za::Color::Blue;
}


////////////////////////////////////////////////////////////
struct BWall
{
};


////////////////////////////////////////////////////////////
struct BColored
{
    BlockKind kind;
};


////////////////////////////////////////////////////////////
struct BKey
{
    BlockKind kind;
};


////////////////////////////////////////////////////////////
struct BPadlock
{
};


////////////////////////////////////////////////////////////
using BlockType = za::Variant<BWall, BColored, BKey, BPadlock>;


////////////////////////////////////////////////////////////
enum class GravityType : za::U8
{
    None,
    Down,
    Up,
    Left,
    Right,
};


////////////////////////////////////////////////////////////
struct Block
{
    za::Vec2i               position;
    BlockType               type;
    za::Vec2i               gravityDir = {0, 0};
    bool                    fixed      = false;
    za::Optional<BlockKind> locked     = {};
};


////////////////////////////////////////////////////////////
using ObjectId = za::SizeT;


////////////////////////////////////////////////////////////
struct TGravityRotator
{
    bool clockwise{false};
};


////////////////////////////////////////////////////////////
struct TLava
{
};


////////////////////////////////////////////////////////////
using TileType = za::Variant<TGravityRotator, TLava>;


////////////////////////////////////////////////////////////
struct Tile
{
    za::Vec2i position;
    TileType  type;
};


////////////////////////////////////////////////////////////
using Object = za::Variant<Block, Tile>;


////////////////////////////////////////////////////////////
struct World
{
private:
    za::Vector<za::Optional<Object>> m_objects;

public:
    ////////////////////////////////////////////////////////////
    Object& addWall(const za::Vec2i position)
    {
        return *m_objects.emplaceBack(za::inPlace,
                                      Block{
                                          .position = position,
                                          .type     = BlockType{BWall{}},
                                      });
    }

    ////////////////////////////////////////////////////////////
    Object& addColored(const za::Vec2i position, const BlockKind kind, const za::Vec2i gravityDir = {0, 0})
    {
        return *m_objects.emplaceBack(za::inPlace,
                                      Block{
                                          .position   = position,
                                          .type       = BlockType{BColored{.kind = kind}},
                                          .gravityDir = gravityDir,
                                      });
    }

    ////////////////////////////////////////////////////////////
    Object& addKey(const za::Vec2i position, const BlockKind kind, const za::Vec2i gravityDir = {0, 0})
    {
        return *m_objects.emplaceBack(za::inPlace,
                                      Block{
                                          .position   = position,
                                          .type       = BlockType{BKey{.kind = kind}},
                                          .gravityDir = gravityDir,
                                      });
    }

    ////////////////////////////////////////////////////////////
    Object& addPadlock(const za::Vec2i position, const za::Vec2i gravityDir = {0, 0})
    {
        return *m_objects.emplaceBack(za::inPlace,
                                      Block{
                                          .position   = position,
                                          .type       = BlockType{BPadlock{}},
                                          .gravityDir = gravityDir,
                                      });
    }

    ////////////////////////////////////////////////////////////
    Object& addGravityRotator(const za::Vec2i position, const bool clockwise)
    {
        return *m_objects.emplaceBack(za::inPlace,
                                      Tile{
                                          .position = position,
                                          .type     = TileType{TGravityRotator{.clockwise = clockwise}},
                                      });
    }

    ////////////////////////////////////////////////////////////
    Object& addLava(const za::Vec2i position)
    {
        return *m_objects.emplaceBack(za::inPlace,
                                      Tile{
                                          .position = position,
                                          .type     = TileType{TLava{}},
                                      });
    }

    ////////////////////////////////////////////////////////////
    void forTiles(auto&& f)
    {
        for (ObjectId i = 0u; i < m_objects.size(); ++i)
            if (m_objects[i].hasValue() && m_objects[i]->is<Tile>())
                if (f(i, m_objects[i]->as<Tile>()) == ControlFlow::Break)
                    break;
    }

    ////////////////////////////////////////////////////////////
    void forBlocks(auto&& f)
    {
        for (ObjectId i = 0u; i < m_objects.size(); ++i)
            if (m_objects[i].hasValue() && m_objects[i]->is<Block>())
                if (f(i, m_objects[i]->as<Block>()) == ControlFlow::Break)
                    break;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] Block& getBlockById(const ObjectId objectId)
    {
        ZA_ASSERT(objectId < m_objects.size());
        ZA_ASSERT(m_objects[objectId].hasValue());
        ZA_ASSERT(m_objects[objectId]->is<Block>());

        return m_objects[objectId]->as<Block>();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] Tile& getTileById(const ObjectId objectId)
    {
        ZA_ASSERT(objectId < m_objects.size());
        ZA_ASSERT(m_objects[objectId].hasValue());
        ZA_ASSERT(m_objects[objectId]->is<Tile>());

        return m_objects[objectId]->as<Tile>();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isOOB(const za::Vec2i position) const
    {
        return position.x < 0 || position.x >= 12 || position.y < 0 || position.y >= 12;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isLava(const za::Vec2i position) const
    {
        for (const za::Optional<Object>& object : m_objects)
        {
            if (!object.hasValue() || !object->is<Tile>() || !object->as<Tile>().type.is<TLava>())
                continue;

            if (object->as<Tile>().position == position)
                return true;
        }

        return false;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isWall(const za::Vec2i position) const
    {
        for (const za::Optional<Object>& object : m_objects)
        {
            if (!object.hasValue() || !object->is<Block>() || !object->as<Block>().type.is<BWall>())
                continue;

            if (object->as<Block>().position == position)
                return true;
        }

        return false;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isBlocked(const za::Vec2i position) const
    {
        if (isOOB(position))
            return true;

        for (const za::Optional<Object>& object : m_objects)
        {
            if (!object.hasValue() || !object->is<Block>())
                continue;

            if (object->as<Block>().position == position)
                return true;
        }

        return false;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] TGravityRotator* tryGetGravityRotator(const za::Vec2i position)
    {
        for (za::Optional<Object>& object : m_objects)
        {
            if (!object.hasValue() || !object->is<Tile>())
                continue;

            if (object->as<Tile>().position == position)
                return object->as<Tile>().type.getIf<TGravityRotator>();
        }

        return nullptr;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Optional<ObjectId> getBlockByPosition(const za::Vec2i position) const
    {
        for (ObjectId i = 0u; i < m_objects.size(); ++i)
        {
            const auto& object = m_objects[i];

            if (object.hasValue() && object->is<Block>() && object->as<Block>().position == position)
                return za::makeOptional<ObjectId>(i);
        }

        return za::nullOpt;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Optional<ObjectId> getTileByPosition(const za::Vec2i position) const
    {
        for (ObjectId i = 0u; i < m_objects.size(); ++i)
        {
            const auto& object = m_objects[i];

            if (object.hasValue() && object->is<Tile>() && object->as<Tile>().position == position)
                return za::makeOptional<ObjectId>(i);
        }

        return za::nullOpt;
    }

    ////////////////////////////////////////////////////////////
    void forOrthogonalNeighbors(const ObjectId objectId, auto&& f) const
    {
        const za::Optional<Object>& object = m_objects[objectId];

        if (!object.hasValue() || !object->is<Block>())
            return;

        if (const auto e = getBlockByPosition(object->as<Block>().position.addY(1)); e.hasValue())
            if (f(*e) == ControlFlow::Break)
                return;

        if (const auto w = getBlockByPosition(object->as<Block>().position.addX(1)); w.hasValue())
            if (f(*w) == ControlFlow::Break)
                return;

        if (const auto n = getBlockByPosition(object->as<Block>().position.addY(-1)); n.hasValue())
            if (f(*n) == ControlFlow::Break)
                return;

        if (const auto s = getBlockByPosition(object->as<Block>().position.addX(-1)); s.hasValue())
            if (f(*s) == ControlFlow::Break)
                return;
    }

    ////////////////////////////////////////////////////////////
    void killObject(const ObjectId objectId)
    {
        ZA_ASSERT(objectId < m_objects.size());
        ZA_ASSERT(m_objects[objectId].hasValue());
        m_objects[objectId].reset();
    }
};


////////////////////////////////////////////////////////////
struct TEMoveBlock
{
    ObjectId  objectId;
    za::Vec2i newPosition;
    float     progress{0.f};
};


////////////////////////////////////////////////////////////
struct TEFallBlock
{
    ObjectId  objectId;
    za::Vec2i dir;
    za::Vec2i newPosition;
    bool      squish;
    float     progress{0.f};
};


////////////////////////////////////////////////////////////
struct TESquishBlock
{
    ObjectId  objectId;
    za::Vec2i dir;
    float     progress{0.f};
};


////////////////////////////////////////////////////////////
struct TERotateGravityDir
{
    ObjectId objectId;
    bool     clockwise;
    float    progress{0.f};
};


////////////////////////////////////////////////////////////
struct TEKill
{
    ObjectId objectId;
    float    progress{0.f};
};


////////////////////////////////////////////////////////////
struct TEUnlock
{
    ObjectId objectId;
    float    progress{0.f};
};


////////////////////////////////////////////////////////////
struct TEBurn
{
    ObjectId objectId;
    float    progress{0.f};
};


////////////////////////////////////////////////////////////
using TurnEvent = za::Variant<TEMoveBlock, TEFallBlock, TESquishBlock, TEKill, TERotateGravityDir, TEUnlock, TEBurn>;


////////////////////////////////////////////////////////////
struct [[nodiscard]] ParticleData
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
};


////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] inline za::Sprite particleToSprite(const ParticleData& particle,
                                                                     const za::Rect2f&   textureRect,
                                                                     const za::Color     color)
{
    const auto opacityAsAlpha = static_cast<za::U8>(particle.opacity * 255.f);

    return {
        .position    = particle.position,
        .scale       = {particle.scale, particle.scale},
        .origin      = textureRect.size / 2.f,
        .rotation    = za::radians(particle.rotation),
        .textureRect = textureRect,
        .color       = color.withAlpha(opacityAsAlpha),
    };
}


////////////////////////////////////////////////////////////
class Game
{
private:
    ////////////////////////////////////////////////////////////
    const unsigned int m_aaLevel = za::min(16u, za::RenderTexture::getMaximumSampleCount());

    ////////////////////////////////////////////////////////////
    za::RenderWindow m_window = makeDPIScaledRenderWindow(
                                    {
                                        .size           = baseResolution.toVec2u(),
                                        .title          = "Block Puzzle",
                                        .resizable      = true,
                                        .vsync          = true,
                                        .frametimeLimit = 144u,
                                    })
                                    .value();

    ////////////////////////////////////////////////////////////
    // For rendering logical game entities into the 2x render textures
    za::View m_worldView = za::View::fromScreenSize(logicalResolution);

    // For rendering the final texture to the window (this one gets aspect-ratio resized)
    za::View m_windowView = computeAspectRatioAwareView(m_window.getSize().toVec2f(), logicalResolution);

    ////////////////////////////////////////////////////////////
    za::Shader m_shader{[]
    {
        auto result = za::Shader::loadFromFile({.fragmentPath = "resources/shader.frag"}).value();
        result.setUniform(result.getUniformLocation("za_u_texture").value(), za::Shader::CurrentTexture);
        return result;
    }()};

    za::Shader::UniformLocation m_ulTime        = m_shader.getUniformLocation("u_time").value();
    za::Shader::UniformLocation m_ulWaveEnabled = m_shader.getUniformLocation("u_waveEnabled").value();

    ////////////////////////////////////////////////////////////
    za::Shader m_shaderSpriteAlpha{[]
    {
        auto result = za::Shader::loadFromFile({.fragmentPath = "resources/spritealpha.frag"}).value();
        result.setUniform(result.getUniformLocation("za_u_texture").value(), za::Shader::CurrentTexture);
        return result;
    }()};

    ////////////////////////////////////////////////////////////
    za::Shader m_shaderBlurQuad{[]
    {
        auto result = za::Shader::loadFromFile({.fragmentPath = "resources/blurquad.frag"}).value();
        result.setUniform(result.getUniformLocation("za_u_texture").value(), za::Shader::CurrentTexture);
        return result;
    }()};

    za::Shader::UniformLocation m_ulBlurQuadBlurDirection = m_shaderBlurQuad.getUniformLocation("u_blurDirection").value();
    za::Shader::UniformLocation m_ulBlurQuadRadiusPixels = m_shaderBlurQuad.getUniformLocation("u_blurRadiusPixels").value();

    ////////////////////////////////////////////////////////////
    za::Shader m_shaderShadow{[]
    {
        auto result = za::Shader::loadFromFile({.fragmentPath = "resources/shadow.frag"}).value();
        result.setUniform(result.getUniformLocation("za_u_texture").value(), za::Shader::CurrentTexture);
        return result;
    }()};

    za::Shader::UniformLocation m_ulShadowTexture = m_shaderShadow.getUniformLocation("za_u_texture").value();
    za::Shader::UniformLocation m_ulShadowColor   = m_shaderShadow.getUniformLocation("u_shadowColor").value();

    ////////////////////////////////////////////////////////////
    const za::Font m_font = za::Font::openFromFile("resources/Born2bSportyFS.ttf").value();

    ////////////////////////////////////////////////////////////
    za::ImGuiContext m_imGuiContext;

    ////////////////////////////////////////////////////////////
    za::Clock m_fpsClock;

    ////////////////////////////////////////////////////////////
    za::Vector<World>       m_prevWorlds;
    World                   m_world;
    za::Optional<za::SizeT> m_grabbedObjectId;
    za::Vector<TurnEvent>   m_turnEvents;

    ////////////////////////////////////////////////////////////
    za::Vector<ParticleData> m_lavaParticlesTop;
    za::Vector<ParticleData> m_lavaParticles;

    ////////////////////////////////////////////////////////////
    RNGFast m_rngFast; // very fast, low-quality, but good enough for VFXs

    ////////////////////////////////////////////////////////////
    float m_time = 0.f;

    ////////////////////////////////////////////////////////////
    const za::Cursor m_cursorArrow      = za::Cursor::loadFromSystem(za::Cursor::Type::Arrow).value();
    const za::Cursor m_cursorHand       = za::Cursor::loadFromSystem(za::Cursor::Type::Hand).value();
    const za::Cursor m_cursorNotAllowed = za::Cursor::loadFromSystem(za::Cursor::Type::NotAllowed).value();

    ////////////////////////////////////////////////////////////
    int       m_editorSelectedObjectIdx      = 0;
    int       m_editorSelectedColorIdx       = 0;
    int       m_editorSelectedLockedColorIdx = 0;
    za::Vec2i m_editorGravity;
    bool      m_editorObjectFixed     = false;
    bool      m_editorObjectLocked    = false;
    bool      m_editorObjectClockwise = false;

    ////////////////////////////////////////////////////////////
    za::TextureAtlas m_textureAtlas{za::Texture::create({4096u, 4096u}, {.smooth = false}).value()};

    ////////////////////////////////////////////////////////////
    const za::Rect2f m_txrWhiteDotTrue = m_textureAtlas.add(za::GraphicsContext::getBuiltInWhiteDotTexture()).value();
    const za::Rect2f m_txrWhiteDot     = {{0.f, 0.f}, {1.f, 1.f}};
    const za::Rect2f m_txrGrid         = addImgResourceToAtlas("grid.png");
    const za::Rect2f m_txrGrid0        = addImgResourceToAtlas("grid0.png");
    const za::Rect2f m_txrGrid1        = addImgResourceToAtlas("grid1.png");
    const za::Rect2f m_txrFixed        = addImgResourceToAtlas("fixed.png");
    const za::Rect2f m_txrLocked       = addImgResourceToAtlas("locked.png");
    const za::Rect2f m_txrGravArrow    = addImgResourceToAtlas("gravarrow.png");
    const za::Rect2f m_txrKey0         = addImgResourceToAtlas("key0.png");
    const za::Rect2f m_txrKey1         = addImgResourceToAtlas("key1.png");
    const za::Rect2f m_txrKey2         = addImgResourceToAtlas("key2.png");
    const za::Rect2f m_txrKeyBg        = addImgResourceToAtlas("keybg.png");
    const za::Rect2f m_txrBlock0       = addImgResourceToAtlas("block0.png");
    const za::Rect2f m_txrBlock1       = addImgResourceToAtlas("block1.png");
    const za::Rect2f m_txrBlock2       = addImgResourceToAtlas("block2.png");
    const za::Rect2f m_txrBlockBg      = addImgResourceToAtlas("blockbg.png");
    const za::Rect2f m_txrWall         = addImgResourceToAtlas("wall.png");
    const za::Rect2f m_txrWallCorner   = addImgResourceToAtlas("wallcorner.png");
    const za::Rect2f m_txrWallH        = addImgResourceToAtlas("wallh.png");
    const za::Rect2f m_txrWallV        = addImgResourceToAtlas("wallv.png");
    const za::Rect2f m_txrWallBg       = addImgResourceToAtlas("wallbg.png");
    const za::Rect2f m_txrWallSet      = addImgResourceToAtlas("wallset.png");
    const za::Rect2f m_txrWallBits     = addImgResourceToAtlas("wallBits.png");
    const za::Rect2f m_txrLavaParticle = addImgResourceToAtlas("lavaparticle.png");
    const za::Rect2f m_txrLock0        = addImgResourceToAtlas("lock0.png");
    const za::Rect2f m_txrPinned       = addImgResourceToAtlas("pinned.png");

    ////////////////////////////////////////////////////////////
    za::Texture m_txLava = za::Texture::loadFromFile("resources/lava.png", {.smooth = true}).value();

    ////////////////////////////////////////////////////////////
    za::CPUDrawableBatch m_dbBackground;
    za::CPUDrawableBatch m_dbLavaParticles;
    za::CPUDrawableBatch m_dbLavaParticlesTop;
    za::CPUDrawableBatch m_dbTile;
    za::CPUDrawableBatch m_dbWall;
    za::CPUDrawableBatch m_dbObjectBg;
    za::CPUDrawableBatch m_dbObject;
    za::CPUDrawableBatch m_dbObjectAttributes;

    ////////////////////////////////////////////////////////////
    static inline constexpr float shadowTextureResMult = 0.25f;

    ////////////////////////////////////////////////////////////
    za::RenderTexture m_rtSpriteBg{
        za::RenderTexture::create((logicalResolution.toVec2f() * shadowTextureResMult).toVec2u(),
                                  {.smooth = true, .sampleCount = m_aaLevel})
            .value()};

    za::RenderTexture m_rtSpriteBgTemp{
        za::RenderTexture::create((logicalResolution.toVec2f() * shadowTextureResMult).toVec2u(),
                                  {.smooth = true, .sampleCount = m_aaLevel})
            .value()};

    za::RenderTexture m_rtGame{
        za::RenderTexture::create(logicalResolution.toVec2u(), {.smooth = true, .sampleCount = m_aaLevel}).value()};

    ////////////////////////////////////////////////////////////
    za::Texture m_txUndo        = za::Texture::create(logicalResolution.toVec2u(), {.smooth = true}).value();
    float       m_undoCountdown = 0.f;

    ////////////////////////////////////////////////////////////
    int m_moves = 0;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Rect2f addImgResourceToAtlas(const za::Path& path)
    {
        return m_textureAtlas.add(za::Image::loadFromFile("resources" / path).value(), /* padding */ {2u, 2u}).value();
    }

    ////////////////////////////////////////////////////////////
    void checkForKill()
    {
        za::Vector<ObjectId> blocksToKill;

        m_world.forBlocks([&](const ObjectId objectId, const Block& block)
        {
            const auto* blockColored = block.type.getIf<BColored>();
            if (blockColored == nullptr || isLocked(block))
                return ControlFlow::Continue;

            m_world.forOrthogonalNeighbors(objectId,
                                           [&](const ObjectId neighborObjectId)
            {
                if (neighborObjectId >= objectId)
                    return ControlFlow::Continue;

                const Block& neighborBlock        = m_world.getBlockById(neighborObjectId);
                const auto*  neighborBlockColored = neighborBlock.type.getIf<BColored>();

                if (neighborBlockColored == nullptr || isLocked(neighborBlock))
                    return ControlFlow::Continue;

                if (blockColored->kind == neighborBlockColored->kind)
                {
                    blocksToKill.pushBack(objectId);
                    blocksToKill.pushBack(neighborObjectId);
                }

                return ControlFlow::Continue;
            });

            return ControlFlow::Continue;
        });

        za::quickSort(blocksToKill.begin(), blocksToKill.end());
        blocksToKill.erase(za::unique(blocksToKill.begin(), blocksToKill.end()), blocksToKill.end());

        for (const ObjectId objectId : blocksToKill)
            m_turnEvents.pushBack(TEKill{.objectId = objectId});
    }

    ////////////////////////////////////////////////////////////
    void checkForBurn()
    {
        za::Vector<ObjectId> blocksToBurn;

        m_world.forBlocks([&](const ObjectId objectId, const Block& block)
        {
            const auto* blockColored = block.type.getIf<BColored>();
            if (blockColored == nullptr)
                return ControlFlow::Continue;

            const auto tileId = m_world.getTileByPosition(block.position);
            if (!tileId.hasValue())
                return ControlFlow::Continue;

            const auto& tile = m_world.getTileById(*tileId);
            if (!tile.type.is<TLava>())
                return ControlFlow::Continue;

            blocksToBurn.pushBack(objectId);
            return ControlFlow::Continue;
        });

        za::quickSort(blocksToBurn.begin(), blocksToBurn.end());
        blocksToBurn.erase(za::unique(blocksToBurn.begin(), blocksToBurn.end()), blocksToBurn.end());

        for (const ObjectId objectId : blocksToBurn)
            m_turnEvents.pushBack(TEBurn{.objectId = objectId});
    }

    ////////////////////////////////////////////////////////////
    void checkForUnlock()
    {
        za::Vector<ObjectId> blocksToKill;

        m_world.forBlocks([&](const ObjectId objectId, const Block& block)
        {
            const auto* blockKey = block.type.getIf<BKey>();
            if (blockKey == nullptr)
                return ControlFlow::Continue;

            m_world.forOrthogonalNeighbors(objectId,
                                           [&](const ObjectId neighborObjectId)
            {
                Block& neighborBlock = m_world.getBlockById(neighborObjectId);
                if (!isLocked(neighborBlock))
                    return ControlFlow::Continue;

                if (neighborBlock.locked.value() != blockKey->kind)
                    return ControlFlow::Continue;

                const auto* neighborBlockColored = neighborBlock.type.getIf<BColored>();
                const auto* neighborBlockKey     = neighborBlock.type.getIf<BKey>();

                if (neighborBlockColored != nullptr || neighborBlockKey != nullptr)
                {
                    m_turnEvents.pushBack(TEUnlock{.objectId = neighborObjectId});
                    blocksToKill.pushBack(objectId);
                }

                return ControlFlow::Continue;
            });

            return ControlFlow::Continue;
        });

        za::quickSort(blocksToKill.begin(), blocksToKill.end());
        blocksToKill.erase(za::unique(blocksToKill.begin(), blocksToKill.end()), blocksToKill.end());

        for (const ObjectId objectId : blocksToKill)
            m_turnEvents.pushBack(TEKill{.objectId = objectId});
    }

    ////////////////////////////////////////////////////////////
    void checkForFall()
    {
        for (const auto& turnEvent : m_turnEvents)
        {
            if (turnEvent.is<TEFallBlock>())
                return; // already falling, no need to check again
        }

        m_world.forBlocks([&](const ObjectId objectId, Block& block)
        {
            if (block.gravityDir == za::Vec2i{0, 0} || isLocked(block))
                return ControlFlow::Continue;

            za::Vec2i targetPosition = block.position;
            bool      mustFall       = false;

            while (true)
            {
                if (targetPosition == block.position)
                {
                    targetPosition += block.gravityDir;
                    continue;
                }

                if (m_world.isBlocked(targetPosition))
                {
                    mustFall = targetPosition != block.position + block.gravityDir;
                    break;
                }

                if (m_world.isLava(targetPosition))
                {
                    targetPosition += block.gravityDir;
                    mustFall = true;
                    break;
                }

                if (auto* gr = m_world.tryGetGravityRotator(targetPosition))
                {
                    m_turnEvents.pushBack(TEFallBlock{
                        .objectId    = objectId,
                        .dir         = block.gravityDir,
                        .newPosition = targetPosition,
                        .squish      = false,
                    });

                    m_turnEvents.pushBack(TERotateGravityDir{
                        .objectId  = objectId,
                        .clockwise = gr->clockwise,
                    });

                    return ControlFlow::Break;
                }

                targetPosition += block.gravityDir;
            }

            if (mustFall)
            {
                m_turnEvents.pushBack(TEFallBlock{.objectId    = objectId,
                                                  .dir         = block.gravityDir,
                                                  .newPosition = targetPosition - block.gravityDir,
                                                  .squish      = true});

                return ControlFlow::Break;
            }

            return ControlFlow::Continue;
        });
    }

    ////////////////////////////////////////////////////////////
    void forTurnEventsToProcess(auto&& f)
    {
        if (m_turnEvents.empty())
            return;

        auto&      turnEvent  = m_turnEvents.front();
        const bool mustDelete = f(turnEvent);

        if (mustDelete)
            m_turnEvents.erase(m_turnEvents.begin());

        if (!turnEvent.is<TEKill>())
            return;

        for (ObjectId i = mustDelete ? 0u : 1u; i < m_turnEvents.size(); ++i)
        {
            if (!m_turnEvents[i].is<TEKill>())
                break;

            const bool mustDelete2 = f(m_turnEvents[i]);
            if (mustDelete2)
            {
                m_turnEvents.eraseAt(i);
                --i;
            }
        }
    }

    ////////////////////////////////////////////////////////////
    void resetLevel()
    {
        m_world = World{};
        m_turnEvents.clear();
        m_grabbedObjectId.reset();
        m_time = 0.f;

        for (int iX = 0; iX < 12; ++iX)
            for (int iY = 0; iY < 12; ++iY)
                if (iX == 0 || iX == 11 || iY == 0 || iY == 11)
                    m_world.addWall({iX, iY});

        m_world.addColored({4, 4}, BlockKind::A);

        m_world.addColored({7, 4}, BlockKind::A, {0, 1});
        m_world.addColored({6, 6}, BlockKind::A, {0, -1});
        m_world.addColored({7, 6}, BlockKind::A);
        m_world.addColored({3, 3}, BlockKind::A);
        m_world.addColored({8, 8}, BlockKind::B);
        m_world.addColored({9, 9}, BlockKind::B);

        m_world.addColored({3, 8}, BlockKind::B).as<Block>().fixed = true;

        m_world.addColored({4, 10}, BlockKind::B).as<Block>().locked.emplace(BlockKind::A);
        m_world.addKey({6, 10}, BlockKind::B);

        // m_world.addPadlock({8, 8});
        m_world.addPadlock({8, 10});

        // m_world.addGravityRotator({9, 10}, /* clockwise */ true);
        // m_world.addGravityRotator({2, 10}, /* clockwise */ true);
        // m_world.addGravityRotator({7, 1}, /* clockwise */ false);

        m_world.addLava({2, 9});
        m_world.addLava({2, 10});
        m_world.addLava({3, 9});

        m_world.addColored({10, 10}, BlockKind::C);
        m_world.addColored({10, 6}, BlockKind::C, {-1, 0});

        m_world.addWall({6, 4});
        m_world.addWall({6, 5});
        m_world.addWall({7, 5});
        m_world.addWall({8, 5});
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool makeProgress(auto& turnEvent, const float speed)
    {
        turnEvent->progress += speed;
        return turnEvent->progress >= 1.f;
    }

    ////////////////////////////////////////////////////////////
    void finishTurn()
    {
        m_prevWorlds.emplaceBack(m_world);
        ++m_moves;
    }

    ////////////////////////////////////////////////////////////
    void undoTurn()
    {
        if (m_prevWorlds.empty())
            return;

        if (!m_txUndo.update(m_rtGame.getTexture()))
            za::abort();

        m_undoCountdown = 1.f;

        m_world = ZA_MOVE(m_prevWorlds.back());
        m_prevWorlds.popBack();

        m_turnEvents.clear();
        m_grabbedObjectId.reset();

        --m_moves;
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool updateTurnEvent(TurnEvent& turnEvent, const float deltaTimeMs)
    {
        if (auto* moveBlock = turnEvent.getIf<TEMoveBlock>())
        {
            if (!makeProgress(moveBlock, deltaTimeMs * 0.0075f))
                return false;

            finishTurn();

            Block& grabbedBlock   = m_world.getBlockById(moveBlock->objectId);
            grabbedBlock.position = moveBlock->newPosition;

            return true;
        }

        if (auto* fallBlock = turnEvent.getIf<TEFallBlock>())
        {
            if (!makeProgress(fallBlock, deltaTimeMs * 0.0015f))
                return false;

            Block& grabbedBlock   = m_world.getBlockById(fallBlock->objectId);
            grabbedBlock.position = fallBlock->newPosition;

            if (fallBlock->squish)
                m_turnEvents.pushBack(TESquishBlock{.objectId = fallBlock->objectId, .dir = fallBlock->dir});

            return true;
        }

        if (auto* squishBlock = turnEvent.getIf<TESquishBlock>())
            return makeProgress(squishBlock, deltaTimeMs * 0.0020f);

        if (auto* kill = turnEvent.getIf<TEKill>())
        {
            if (!makeProgress(kill, deltaTimeMs * 0.0015f))
                return false;

            m_world.killObject(kill->objectId);
            m_grabbedObjectId.reset();

            return true;
        }

        if (auto* rotateGravityDir = turnEvent.getIf<TERotateGravityDir>())
        {
            if (!makeProgress(rotateGravityDir, deltaTimeMs * 0.0020f))
                return false;

            const int mult = rotateGravityDir->clockwise ? 1 : -1;

            m_world.getBlockById(rotateGravityDir->objectId)
                .gravityDir = za::Vec2i{m_world.getBlockById(rotateGravityDir->objectId).gravityDir.y * -mult,
                                        m_world.getBlockById(rotateGravityDir->objectId).gravityDir.x * mult};

            return true;
        }

        if (auto* unlock = turnEvent.getIf<TEUnlock>())
        {
            if (!makeProgress(unlock, deltaTimeMs * 0.0020f))
                return false;

            m_world.getBlockById(unlock->objectId).locked.reset();
            return true;
        }

        if (auto* burn = turnEvent.getIf<TEBurn>())
        {
            if (!makeProgress(burn, deltaTimeMs * 0.00025f))
            {
                const auto blockPos       = m_world.getBlockById(burn->objectId).position.toVec2f();
                const auto blockRenderPos = za::Vec2f{blockPos.x * 128.f, blockPos.y * 128.f} + za::Vec2f{64.f, 64.f};

                const float     offset = za::max(4.f, 64.f * (1.f - burn->progress));
                const za::Vec2f offsetVec{offset, offset};

                const auto nParticles = static_cast<int>(za::ceil((1.f - burn->progress) * 5.f));

                for (int i = 0; i < nParticles; ++i)
                {
                    makeLavaParticle(m_rngFast.getVec2f(blockRenderPos - offsetVec, blockRenderPos + offsetVec));
                    makeLavaParticleTop(m_rngFast.getVec2f(blockRenderPos - offsetVec, blockRenderPos + offsetVec));
                }

                return false;
            }

            m_world.killObject(burn->objectId);
            m_grabbedObjectId.reset();

            return true;
        }

        return false;
    }

    ////////////////////////////////////////////////////////////
    void updateTurn(const float deltaTimeMs)
    {
        if (!m_turnEvents.empty())
        {
            forTurnEventsToProcess([&](TurnEvent& turnEvent) { return updateTurnEvent(turnEvent, deltaTimeMs); });
            return;
        }

        checkForBurn();

        if (!m_turnEvents.empty())
            return;

        checkForKill();

        if (!m_turnEvents.empty())
            return;

        checkForUnlock();

        if (!m_turnEvents.empty())
            return;

        checkForFall();

        if (!m_turnEvents.empty())
            return;

        m_world.forBlocks([&](const ObjectId objectId, const Block& block)
        {
            const auto      blockPos     = block.position.toVec2f();
            const za::Vec2f drawPosition = za::Vec2f{blockPos.x * 128.f, blockPos.y * 128.f} + za::Vec2f{64.f, 64.f};

            const bool hoveredByMouse = getAdjustedMousePos().x > drawPosition.x &&
                                        getAdjustedMousePos().x < drawPosition.x + 128.f &&
                                        getAdjustedMousePos().y > drawPosition.y &&
                                        getAdjustedMousePos().y < drawPosition.y + 128.f;

            if (!block.fixed && !block.locked && !block.type.is<BWall>() && hoveredByMouse && isLMBPressed() &&
                !m_grabbedObjectId.hasValue())
            {
                m_grabbedObjectId.emplace(objectId);
                return ControlFlow::Break;
            }

            return ControlFlow::Continue;
        });

        if (m_grabbedObjectId.hasValue())
        {
            Block& grabbedBlock = m_world.getBlockById(*m_grabbedObjectId);

            auto            diff = getWorldMousePos() - grabbedBlock.position;
            const za::Vec2i dir  = {diff.x < 0 ? -1 : (diff.x > 0 ? 1 : 0), diff.y < 0 ? -1 : (diff.y > 0 ? 1 : 0)};

            const auto newPos = grabbedBlock.position + dir;

            const bool movingAgainstGravity = grabbedBlock.gravityDir != za::Vec2i{0, 0} && grabbedBlock.gravityDir == -dir;

            const bool movingDiagonally = dir.x != 0 && dir.y != 0;
            if (!movingDiagonally)
            {
                if (!m_world.isBlocked(newPos))
                {
                    if (!movingAgainstGravity)
                        m_turnEvents.pushBack(
                            TEMoveBlock{.objectId = *m_grabbedObjectId, .newPosition = grabbedBlock.position + dir});
                }
                else if (diff != za::Vec2i{0, 0})
                    m_turnEvents.pushBack(TESquishBlock{.objectId = *m_grabbedObjectId, .dir = dir});
            }
        }
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] bool isLMBPressed() const noexcept
    {
        return za::Mouse::isButtonPressed(za::Mouse::Button::Left);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] za::Vec2f getMousePos() const noexcept
    {
        return m_windowView.screenToWorld(za::Mouse::getPosition(m_window).toVec2f(), m_window.getSize().toVec2f());
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] za::Vec2f getAdjustedMousePos() const noexcept
    {
        return getMousePos() + za::Vec2f{64.f, 64.f};
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] za::Vec2i getWorldMousePos() const noexcept
    {
        return (getMousePos() / 128.f).toVec2i();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] bool isLocked(const Block& block) const noexcept
    {
        return block.locked.hasValue();
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] za::Color getHueColor(const float hue, const za::U8 alpha = 255u) const noexcept
    {
        return hueColor(za::fmod(m_time * 0.06f + hue, 360.f), alpha);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] za::Color getLavaColor() const noexcept
    {
        return hueColor(-5.f, 215u);
    }

    ////////////////////////////////////////////////////////////
    void makeLavaParticle(const za::Vec2f position)
    {
        m_lavaParticles.emplaceBack(
            ParticleData{.position      = position,
                         .velocity      = m_rngFast.getVec2f({-0.75f, -0.75f}, {0.75f, 0.75f}) * 0.05f,
                         .scale         = m_rngFast.getF(0.08f, 0.27f) * 1.25f,
                         .scaleDecay    = -0.0015f,
                         .accelerationY = 0.f,
                         .opacity       = 0.35f,
                         .opacityDecay  = m_rngFast.getF(0.001f, 0.002f) * 0.47f,
                         .rotation      = m_rngFast.getF(0.f, za::tau),
                         .torque        = m_rngFast.getF(-0.001f, 0.001f)});
    }

    ////////////////////////////////////////////////////////////
    void makeLavaParticleTop(const za::Vec2f position)
    {
        m_lavaParticlesTop.emplaceBack(
            ParticleData{.position      = position,
                         .velocity      = m_rngFast.getVec2f({-0.75f, -0.75f}, {0.75f, 0.75f}) * 0.05f,
                         .scale         = m_rngFast.getF(0.08f, 0.27f) * 1.25f,
                         .scaleDecay    = -0.0015f,
                         .accelerationY = 0.f,
                         .opacity       = 0.35f,
                         .opacityDecay  = m_rngFast.getF(0.001f, 0.002f) * 0.47f,
                         .rotation      = m_rngFast.getF(0.f, za::tau),
                         .torque        = m_rngFast.getF(-0.001f, 0.001f)});
    }

public:
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool run()
    {
        resetLevel();

        while (true)
        {
            ////////////////////////////////////////////////////////////
            // Event handling
            ////////////////////////////////////////////////////////////
            // ---
            while (za::Optional event = m_window.pollEvent())
            {
                m_imGuiContext.processEvent(m_window, *event);

                if (za::EventUtils::isClosedOrEscapeKeyPressed(*event))
                    return true;

                if (handleAspectRatioAwareResize(*event, logicalResolution, m_windowView))
                    continue;

                if (auto* eKeyPressed = event->getIf<za::Event::KeyPressed>())
                {
                    if (eKeyPressed->code == za::Keyboard::Key::Backspace && m_turnEvents.empty() && m_undoCountdown == 0.f)
                    {
                        undoTurn();
                        continue;
                    }

                    if (eKeyPressed->code == za::Keyboard::Key::R)
                    {
                        resetLevel();
                        continue;
                    }

                    if (eKeyPressed->code == za::Keyboard::Key::D)
                    {
                        {
                            auto optObjectId = m_world.getBlockByPosition(getWorldMousePos());
                            if (optObjectId.hasValue())
                                m_world.killObject(*optObjectId);
                        }

                        {
                            auto optObjectId = m_world.getTileByPosition(getWorldMousePos());
                            if (optObjectId.hasValue())
                                m_world.killObject(*optObjectId);
                        }

                        continue;
                    }

                    if (eKeyPressed->code == za::Keyboard::Key::E)
                    {
                        {
                            auto optObjectId = m_world.getBlockByPosition(getWorldMousePos());
                            if (optObjectId.hasValue())
                                m_world.killObject(*optObjectId);
                        }

                        {
                            auto optObjectId = m_world.getTileByPosition(getWorldMousePos());
                            if (optObjectId.hasValue())
                                m_world.killObject(*optObjectId);
                        }

                        const auto clampedGravity = m_editorGravity.componentWiseClamp({-1, -1}, {1, 1});
                        const auto kindToUse      = m_editorSelectedColorIdx == 0
                                                        ? BlockKind::A
                                                        : (m_editorSelectedColorIdx == 1 ? BlockKind::B : BlockKind::C);

                        const auto lockKindToUse = m_editorSelectedLockedColorIdx == 0
                                                       ? BlockKind::A
                                                       : (m_editorSelectedLockedColorIdx == 1 ? BlockKind::B : BlockKind::C);


                        if (m_editorSelectedObjectIdx == 0)
                        {
                            m_world.addWall(getWorldMousePos());
                        }
                        else if (m_editorSelectedObjectIdx == 1)
                        {
                            auto& coloredBlock = m_world.addColored(getWorldMousePos(), kindToUse, clampedGravity);
                            coloredBlock.as<Block>().fixed = m_editorObjectFixed;

                            if (m_editorObjectLocked)
                                coloredBlock.as<Block>().locked.emplace(lockKindToUse);
                        }
                        else if (m_editorSelectedObjectIdx == 2)
                        {
                            auto& key             = m_world.addKey(getWorldMousePos(), kindToUse, clampedGravity);
                            key.as<Block>().fixed = m_editorObjectFixed;

                            if (m_editorObjectLocked)
                                key.as<Block>().locked.emplace(lockKindToUse);
                        }
                        else if (m_editorSelectedObjectIdx == 3)
                        {
                            m_world.addGravityRotator(getWorldMousePos(), m_editorObjectClockwise);
                        }
                        else if (m_editorSelectedObjectIdx == 4)
                        {
                            m_world.addLava(getWorldMousePos());
                        }
                    }
                }
            }
            // ---
            ////////////////////////////////////////////////////////////

            ////////////////////////////////////////////////////////////
            // Update step
            ////////////////////////////////////////////////////////////
            // ---
            const auto  deltaTime   = m_fpsClock.restart();
            const float deltaTimeMs = deltaTime.asSeconds() * 1000.f;

            {
                SFEX_PROFILE_SCOPE("update");

                m_time += deltaTimeMs;

                m_shader.setUniform(m_ulTime, m_time);

                if (!isLMBPressed())
                    m_grabbedObjectId.reset();

                updateTurn(deltaTimeMs);

                m_world.forTiles([&](const ObjectId /* objectId */, const Tile& tile)
                {
                    float particleBudget = deltaTimeMs;

                    tile.type.linearMatch([&](const TGravityRotator&) {},
                                          [&](const TLava&)
                    {
                        const auto getParticlePos = [&](const za::Vec2f offset)
                        { return tile.position.toVec2f() * 128.f + za::Vec2f{64.f, 64.f} + offset; };

                        const auto makeLavaParticlePerDirection = [&](const za::Vec2i dir)
                        {
                            if (m_rngFast.getI(0, 100) > 30)
                                return;

                            if (m_world.isOOB(tile.position + dir) || m_world.isLava(tile.position + dir) ||
                                m_world.isWall(tile.position + dir))
                                return;

                            if (particleBudget <= 0.f)
                                return;

                            const auto rndOffset = m_rngFast.getF(-64.f, 64.f);
                            const auto dirOffset = m_rngFast.getF(-2.f, 6.f);
                            const auto dirF      = dir.toVec2f();

                            if (dir.x == 0)
                                makeLavaParticle(getParticlePos({rndOffset, dirF.y * 64.f + -dirF.y * dirOffset}));
                            else if (dir.y == 0)
                                makeLavaParticle(getParticlePos({dirF.x * 64.f + -dirF.x * dirOffset, rndOffset}));

                            particleBudget -= deltaTimeMs * 0.35f;
                        };

                        makeLavaParticlePerDirection({1, 0});
                        makeLavaParticlePerDirection({-1, 0});
                        makeLavaParticlePerDirection({0, 1});
                        makeLavaParticlePerDirection({0, -1});

                        while (particleBudget > 0.f)
                        {
                            if (m_rngFast.getI(0, 100) > 98)
                                makeLavaParticle(getParticlePos({m_rngFast.getF(-64.f, 64.f), m_rngFast.getF(-64.f, 64.f)}));

                            particleBudget -= deltaTimeMs * 0.35f;
                        }
                    });

                    return ControlFlow::Continue;
                });

                const auto updateParticleLike = [&](auto& particleLikeVec)
                {
                    for (auto& p : particleLikeVec)
                    {
                        p.velocity.y += p.accelerationY * deltaTimeMs;
                        p.position += p.velocity * deltaTimeMs;

                        p.rotation += p.torque * deltaTimeMs;

                        p.opacity = za::clamp(p.opacity - p.opacityDecay * deltaTimeMs, 0.f, 1.f);
                        p.scale   = za::max(p.scale - p.scaleDecay * deltaTimeMs, 0.f);
                    }

                    za::vectorEraseIf(particleLikeVec,
                                      [](const auto& particleLike) { return particleLike.opacity <= 0.f; });
                };

                updateParticleLike(m_lavaParticles);
                updateParticleLike(m_lavaParticlesTop);

                m_undoCountdown = za::max(m_undoCountdown - deltaTimeMs * 0.0065f, 0.f);
            }
            // ---
            ////////////////////////////////////////////////////////////

            ////////////////////////////////////////////////////////////
            // ImGui step
            ////////////////////////////////////////////////////////////
            // ---
            m_imGuiContext.update(m_window, deltaTime);

            ImGui::Begin("What", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

            constexpr const char* objectNames[]{
                "Wall",
                "Block",
                "Key",
                "Gravity Rotator",
                "Lava",
            };

            constexpr const char* colorNames[]{
                "Red",
                "Green",
                "Blue",
            };

            ImGui::SetNextItemWidth(160.f);
            ImGui::Combo("Object", &m_editorSelectedObjectIdx, objectNames, za::getArraySize(objectNames));

            ImGui::SetNextItemWidth(160.f);
            ImGui::Combo("Color", &m_editorSelectedColorIdx, colorNames, za::getArraySize(colorNames));

            ImGui::SetNextItemWidth(160.f);
            ImGui::Combo("Lock Color", &m_editorSelectedLockedColorIdx, colorNames, za::getArraySize(colorNames));

            ImGui::InputInt("Gravity X", &m_editorGravity.x, 1);
            ImGui::InputInt("Gravity Y", &m_editorGravity.y, 1);

            ImGui::Checkbox("Fixed", &m_editorObjectFixed);
            ImGui::Checkbox("Locked", &m_editorObjectLocked);
            ImGui::Checkbox("Clockwise", &m_editorObjectClockwise);

            ImGui::End();

            ImGui::Begin("SFEX Profiler");
            sfex::showImguiProfiler();
            ImGui::End();
            // ---
            ////////////////////////////////////////////////////////////

            ////////////////////////////////////////////////////////////
            // Draw step
            ////////////////////////////////////////////////////////////
            // ---
            {
                SFEX_PROFILE_SCOPE("draw");

                m_window.clear();

                m_dbBackground.clear();
                m_dbLavaParticles.clear();
                m_dbLavaParticlesTop.clear();
                m_dbTile.clear();
                m_dbWall.clear();
                m_dbObjectBg.clear();
                m_dbObject.clear();
                m_dbObjectAttributes.clear();

                {
                    SFEX_PROFILE_SCOPE("tiles");

                    m_world.forTiles([&](const ObjectId /* objectId */, const Tile& tile)
                    {
                        const auto      tilePos      = tile.position.toVec2f();
                        const za::Vec2f drawPosition = za::Vec2f{tilePos.x * 128.f, tilePos.y * 128.f} +
                                                       za::Vec2f{64.f, 64.f};

                        tile.type.linearMatch(
                            [&](const TGravityRotator& gravityRotator)
                        {
                            m_dbTile.add(za::CurvedArrowShapeData{
                                .position = drawPosition,
                                .scale    = {gravityRotator.clockwise ? 0.6f : -0.6f, 0.6f},
                                .origin   = {32.f, 32.f},
                                .rotation = za::radians(gravityRotator.clockwise ? m_time * 0.002f : -m_time * 0.002f)
                                                .wrapUnsigned(),
                                .textureRect      = m_txrWhiteDot,
                                .fillColor        = za::Color::LightYellow,
                                .outlineColor     = za::Color::White,
                                .outlineThickness = gravityRotator.clockwise ? 2.f : -2.f,
                                .outerRadius      = 32.f,
                                .innerRadius      = 24.f,
                                .startAngle       = za::degrees(0.f),
                                .sweepAngle       = za::degrees(270.f),
                                .headLength       = 16.f,
                                .headWidth        = 24.f,
                            });
                        },
                            [&](const TLava&) {});

                        return ControlFlow::Continue;
                    });
                }

                const za::Cursor* cursorToUse = &m_cursorArrow;

                {
                    SFEX_PROFILE_SCOPE("blocks");

                    m_world.forBlocks([&](const ObjectId objectId, const Block& block)
                    {
                        za::Vec2f drawPositionOffset   = {0.f, 0.f};
                        za::Vec2f scaleMultiplier      = {1.f, 1.f};
                        float     rotationRadians      = 0.f;
                        float     arrowRotationRadians = 0.f;
                        float     lockRotationRadians  = 0.f;

                        forTurnEventsToProcess([&](TurnEvent& turnEvent)
                        {
                            if (const auto* moveBlock = turnEvent.getIf<TEMoveBlock>())
                            {
                                if (moveBlock->objectId != objectId)
                                    return false;

                                const auto worldOffset = moveBlock->newPosition.toVec2f() - block.position.toVec2f();
                                drawPositionOffset     = easeInOutBack(moveBlock->progress) * worldOffset * 128.f;

                                const auto dir = (moveBlock->newPosition - block.position).toVec2f().abs();
                                scaleMultiplier += easeInOutSine(bounce(moveBlock->progress)) * dir * 0.25f;
                                scaleMultiplier -= easeInOutSine(bounce(moveBlock->progress)) *
                                                   dir.perpendicular().abs() * 0.25f;

                                return false;
                            }

                            if (const auto* fallBlock = turnEvent.getIf<TEFallBlock>())
                            {
                                if (fallBlock->objectId != objectId)
                                    return false;

                                const auto worldOffset = fallBlock->newPosition.toVec2f() - block.position.toVec2f();
                                drawPositionOffset     = easeInBack(fallBlock->progress) * worldOffset * 128.f;

                                const auto dir = (fallBlock->newPosition - block.position).toVec2f().normalized().abs();
                                scaleMultiplier += easeInOutSine(fallBlock->progress) * dir * 0.25f;
                                scaleMultiplier -= easeInOutSine(fallBlock->progress) * dir.perpendicular().abs() * 0.25f;

                                return false;
                            }

                            if (const auto* squishBlock = turnEvent.getIf<TESquishBlock>())
                            {
                                if (squishBlock->objectId != objectId)
                                    return false;

                                const auto absDir = za::Vec2f{za::fabs(static_cast<float>(squishBlock->dir.x)),
                                                              za::fabs(static_cast<float>(squishBlock->dir.y))};

                                const float progress = easeInOutBack(bounce(squishBlock->progress));

                                scaleMultiplier += (progress * absDir * -0.35f);
                                scaleMultiplier += (progress * za::Vec2f{absDir.y, absDir.x} * 0.35f);

                                drawPositionOffset = progress * squishBlock->dir.toVec2f() * 32.f;

                                return false;
                            }

                            if (const auto* kill = turnEvent.getIf<TEKill>())
                            {
                                if (kill->objectId != objectId)
                                    return false;

                                scaleMultiplier *= 1.f - easeInOutBack(kill->progress);
                                rotationRadians = easeInOutSine(kill->progress) * za::tau;

                                return false;
                            }

                            if (const auto* rotateGravityDir = turnEvent.getIf<TERotateGravityDir>())
                            {
                                if (rotateGravityDir->objectId != objectId)
                                    return false;

                                const float mult     = rotateGravityDir->clockwise ? 1.f : -1.f;
                                arrowRotationRadians = easeInOutSine(rotateGravityDir->progress) * za::halfPi * mult;

                                scaleMultiplier.x += 0.35f * easeInOutSine(bounce(rotateGravityDir->progress));
                                scaleMultiplier.y += 0.35f * easeInOutSine(bounce(rotateGravityDir->progress));

                                return false;
                            }

                            if (const auto* unlock = turnEvent.getIf<TEUnlock>())
                            {
                                if (unlock->objectId != objectId)
                                    return false;

                                lockRotationRadians = easeInOutSine(unlock->progress) * za::tau;

                                scaleMultiplier.x += 0.35f * easeInOutSine(bounce(unlock->progress));
                                scaleMultiplier.y += 0.35f * easeInOutSine(bounce(unlock->progress));

                                return false;
                            }

                            if (const auto& burn = turnEvent.getIf<TEBurn>())
                            {
                                if (burn->objectId != objectId)
                                    return false;

                                scaleMultiplier *= 1.f - easeInOutSine(burn->progress);
                                rotationRadians = easeInOutSine(burn->progress) * -za::tau * 2.f;
                            }

                            return false;
                        });

                        const auto      blockPos     = block.position.toVec2f();
                        const za::Vec2f drawPosition = za::Vec2f{blockPos.x * 128.f, blockPos.y * 128.f} +
                                                       drawPositionOffset + za::Vec2f{64.f, 64.f};

                        const bool hoveredByMouse = getAdjustedMousePos().x > drawPosition.x &&
                                                    getAdjustedMousePos().x < drawPosition.x + 128.f &&
                                                    getAdjustedMousePos().y > drawPosition.y &&
                                                    getAdjustedMousePos().y < drawPosition.y + 128.f;

                        const bool canInteractWithHoveredBlock = !block.fixed && !isLocked(block) && hoveredByMouse;

                        if (cursorToUse == &m_cursorArrow && canInteractWithHoveredBlock)
                            cursorToUse = &m_cursorHand;
                        else if (cursorToUse == &m_cursorArrow && hoveredByMouse && !canInteractWithHoveredBlock)
                            cursorToUse = &m_cursorNotAllowed;

                        const auto drawAttributes = [&](const auto& blockImpl)
                        {
                            if (block.fixed)
                                m_dbObjectAttributes.add(za::Sprite{
                                    .position    = drawPosition,
                                    .scale       = scaleMultiplier,
                                    .origin      = {64.f, 64.f},
                                    .rotation    = za::radians(rotationRadians).wrapUnsigned(),
                                    .textureRect = m_txrPinned,
                                    .color       = getHueColor(kindToColor(blockImpl.kind).toHSL().hue),
                                });

                            if (block.gravityDir != za::Vec2i{0, 0})
                                m_dbObjectAttributes.add(za::Sprite{
                                    .position = drawPosition,
                                    .scale = scaleMultiplier.rotatedBy(block.gravityDir.toVec2f().abs().angle()) * 0.6f,
                                    .origin      = {64.f, 64.f},
                                    .rotation    = za::radians(
                                                       block.gravityDir.toVec2f().componentWiseMul({-1.f, 1.f}).angle().asRadians() +
                                                       rotationRadians + arrowRotationRadians)
                                                       .wrapUnsigned(),
                                    .textureRect = m_txrGravArrow,
                                    .color       = getHueColor(kindToColor(blockImpl.kind).toHSL().hue),
                                });

                            if (isLocked(block))
                                m_dbObjectAttributes.add(za::Sprite{
                                    .position = drawPosition + za::Vec2f{40.f, 30.f},
                                    .scale    = scaleMultiplier * 0.5f,
                                    .origin   = {64.f, 64.f},
                                    .rotation = za::radians(rotationRadians + lockRotationRadians + 0.2f).wrapUnsigned(),
                                    .textureRect = m_txrLock0,
                                    .color       = getHueColor(kindToColor(block.locked.value()).toHSL().hue),
                                });
                        };

                        block.type.linearMatch(
                            [&](const BWall&)
                        {
                            unsigned int neighbors = 0u;

                            if (m_world.isWall(block.position.addY(-1)))
                                neighbors |= 1u << 0;

                            if (m_world.isWall(block.position.addX(-1)))
                                neighbors |= 1u << 1;

                            if (m_world.isWall(block.position.addX(1)))
                                neighbors |= 1u << 2;

                            if (m_world.isWall(block.position.addY(1)))
                                neighbors |= 1u << 3;

                            const auto tileX = static_cast<float>(neighbors % 4u);
                            const auto tileY = static_cast<float>(neighbors / 4u);

                            const za::Rect2f txr{
                                .position = m_txrWallSet.position + za::Vec2f{tileX * 128.f, tileY * 128.f},
                                .size     = {128.f, 128.f},
                            };

                            m_dbWall.add(za::Sprite{
                                .position    = drawPosition,
                                .scale       = scaleMultiplier,
                                .origin      = {64.f, 64.f},
                                .rotation    = za::radians(rotationRadians).wrapUnsigned(),
                                .textureRect = txr,
                                .color       = getHueColor(0.f),
                            });

                            const auto patchCorner = [&](const float index)
                            {
                                m_dbWall.add(za::Sprite{
                                    .position = drawPosition,
                                    .scale    = scaleMultiplier,
                                    .origin   = {64.f, 64.f},
                                    .rotation = za::radians(rotationRadians).wrapUnsigned(),
                                    .textureRect = {m_txrWallBits.position + za::Vec2f{128.f * index, 0.f}, {128.f, 128.f}},
                                    .color = getHueColor(0.f),
                                });
                            };

                            if (!m_world.isWall(block.position + za::Vec2i{-1, -1}) && (neighbors & 1u << 1) &&
                                (neighbors & 1u << 0))
                                patchCorner(0.f);

                            if (!m_world.isWall(block.position + za::Vec2i{1, -1}) && (neighbors & 1u << 2) &&
                                (neighbors & 1u << 0))
                                patchCorner(1.f);

                            if (!m_world.isWall(block.position + za::Vec2i{1, 1}) && (neighbors & 1u << 2) &&
                                (neighbors & 1u << 3))
                                patchCorner(2.f);

                            if (!m_world.isWall(block.position + za::Vec2i{-1, 1}) && (neighbors & 1u << 1) &&
                                (neighbors & 1u << 3))
                                patchCorner(3.f);
                        },
                            [&](const BColored& bColored)
                        {
                            const za::Rect2f* txr = bColored.kind == BlockKind::A
                                                        ? &m_txrBlock0
                                                        : (bColored.kind == BlockKind::B ? &m_txrBlock1 : &m_txrBlock2);

                            m_dbObject.add(za::Sprite{
                                .position    = drawPosition,
                                .scale       = scaleMultiplier,
                                .origin      = {64.f, 64.f},
                                .rotation    = za::radians(rotationRadians).wrapUnsigned(),
                                .textureRect = *txr,
                                .color       = getHueColor(kindToColor(bColored.kind).toHSL().hue),
                            });

                            drawAttributes(bColored);
                        },
                            [&](const BKey& bKey)
                        {
                            const za::Rect2f* txr = bKey.kind == BlockKind::A
                                                        ? &m_txrKey0
                                                        : (bKey.kind == BlockKind::B ? &m_txrKey1 : &m_txrKey2);

                            m_dbObject.add(za::Sprite{
                                .position    = drawPosition,
                                .scale       = scaleMultiplier,
                                .origin      = {64.f, 64.f},
                                .rotation    = za::radians(rotationRadians).wrapUnsigned(),
                                .textureRect = *txr,
                                .color       = getHueColor(kindToColor(bKey.kind).toHSL().hue),
                            });

                            drawAttributes(bKey);
                        },
                            [&](const BPadlock& /* bPadlock */)
                        {
                            const za::Rect2f* txr = &m_txrLock0;

                            m_dbObject.add(za::Sprite{
                                .position    = drawPosition,
                                .scale       = scaleMultiplier,
                                .origin      = {64.f, 64.f},
                                .rotation    = za::radians(rotationRadians).wrapUnsigned(),
                                .textureRect = *txr,
                                .color       = getHueColor(kindToColor(BlockKind::A).toHSL().hue),
                            });

                            // drawAttributes(bPadlock);
                        });

                        return ControlFlow::Continue;
                    });
                }

                m_window.setMouseCursor(*cursorToUse);

                for (int iX = 0; iX < 16; ++iX)
                    for (int iY = 0; iY < 16; ++iY)
                    {
                        if (m_world.isLava({iX, iY}))
                            continue;

                        const za::Rect2f* txr = (iX % 2 == 0) ^ (iY % 2 == 0) ? &m_txrGrid1 : &m_txrGrid0;

                        m_dbBackground.add(za::Sprite{
                            .position    = {static_cast<float>(iX) * 128.f, static_cast<float>(iY) * 128.f},
                            .textureRect = *txr,
                            .color       = getHueColor(0.f),
                        });
                    }

                {
                    SFEX_PROFILE_SCOPE("particles");

                    for (za::SizeT i = 0; i < m_lavaParticles.size(); ++i)
                        m_dbLavaParticles.add(particleToSprite(m_lavaParticles[m_lavaParticles.size() - i - 1],
                                                               m_txrLavaParticle,
                                                               getLavaColor()));

                    for (za::SizeT i = 0; i < m_lavaParticlesTop.size(); ++i)
                        m_dbLavaParticlesTop.add(particleToSprite(m_lavaParticlesTop[m_lavaParticlesTop.size() - i - 1],
                                                                  m_txrLavaParticle,
                                                                  getLavaColor()));
                }

                const auto updateShadowTexture = [&](const float blurRadius, const za::U8 alpha, auto&&... toDraw)
                {
                    const float invScale = 1.f / shadowTextureResMult;

                    m_rtSpriteBg.clear(za::Color::Transparent);

                    (...,
                     m_rtSpriteBg.draw(toDraw,
                                       {
                                           .view    = m_worldView,
                                           .texture = &m_textureAtlas.getTexture(),
                                           .shader  = &m_shaderSpriteAlpha,
                                       }));

                    m_rtSpriteBg.display();

                    m_shaderBlurQuad.setUniform(m_ulBlurQuadBlurDirection, za::Vec2f{1.f, 0.f});
                    m_shaderBlurQuad.setUniform(m_ulBlurQuadRadiusPixels, blurRadius * shadowTextureResMult);

                    m_rtSpriteBgTemp.clear(za::Color::Transparent);
                    m_rtSpriteBgTemp.draw(m_rtSpriteBg.getTexture(),
                                          {.scale = {invScale, invScale}},
                                          {.view = m_worldView, .shader = &m_shaderBlurQuad});
                    m_rtSpriteBgTemp.display();


                    m_shaderBlurQuad.setUniform(m_ulBlurQuadBlurDirection, za::Vec2f{0.f, 1.f});

                    m_rtSpriteBg.clear(za::Color::Transparent);
                    m_rtSpriteBg.draw(m_rtSpriteBgTemp.getTexture(),
                                      {.scale = {invScale, invScale}},
                                      {.view = m_worldView, .shader = &m_shaderBlurQuad});
                    m_rtSpriteBg.display();

                    m_shaderShadow.setUniform(m_ulShadowColor, za::Color::blackWithAlpha(alpha).toVec4<za::Glsl::Vec4>());
                };

                {
                    SFEX_PROFILE_SCOPE("rtGame");

                    const float invScale = 1.f / shadowTextureResMult;

                    const za::RenderStates states{
                        .view    = m_worldView,
                        .texture = &m_textureAtlas.getTexture(),
                        .shader  = &m_shader,
                    };

                    m_rtGame.flush();

                    m_shader.setUniform(m_ulWaveEnabled, true);

                    m_rtGame.clear();
                    m_rtGame.draw(m_txLava,
                                  {.position = {0.f, 0.f}, .scale = {2.f, 2.f}, .color = getLavaColor()},
                                  {.view = m_worldView, .shader = &m_shader});

                    m_rtGame.flush();

                    m_shader.setUniform(m_ulWaveEnabled, false);

                    m_rtGame.draw(m_dbBackground, states);
                    m_rtGame.draw(m_dbLavaParticles,
                                  {
                                      .blendMode = za::BlendAdd,
                                      .view      = m_worldView,
                                      .texture   = &m_textureAtlas.getTexture(),
                                      .shader    = &m_shader,
                                  });
                    m_rtGame.draw(m_dbTile, states);

                    {
                        SFEX_PROFILE_SCOPE("shadow pass 1");

                        updateShadowTexture(/* blurRadius */ 10.f, /* alpha */ 128u, m_dbWall, m_dbObject);
                        m_rtGame.draw(m_rtSpriteBg.getTexture(),
                                      {.position = {8.f, 8.f}, .scale = {invScale, invScale}},
                                      {.view = m_worldView, .shader = &m_shaderShadow});
                    }

                    m_rtGame.draw(m_dbWall, states);
                    m_rtGame.draw(m_dbObject, states);

                    m_rtGame.draw(m_dbLavaParticlesTop,
                                  {
                                      .blendMode = za::BlendAdd,
                                      .view      = m_worldView,
                                      .texture   = &m_textureAtlas.getTexture(),
                                      .shader    = &m_shader,
                                  });

                    {
                        SFEX_PROFILE_SCOPE("shadow pass 2");

                        updateShadowTexture(/* blurRadius */ 5.f, /* alpha */ 196u, m_dbObjectAttributes);
                        m_rtGame.draw(m_rtSpriteBg.getTexture(),
                                      {.position = {4.f, 4.f}, .scale = {invScale, invScale}},
                                      {
                                          .view   = m_worldView,
                                          .shader = &m_shaderShadow,
                                      });
                    }

                    m_rtGame.draw(m_dbObjectAttributes, states);
                }

                m_rtGame.draw(
                    za::RectangleShapeData{
                        .position  = getMousePos(),
                        .origin    = {10.f, 10.f},
                        .fillColor = za::Color::Black,
                        .size      = {20.f, 20.f},
                    },
                    {.view = m_worldView});

                m_rtGame.display();

                m_window.flush();
                m_window.clear();

                m_window.draw(m_rtGame.getTexture(), {.view = m_windowView, .shader = &m_shader});

                if (m_undoCountdown > 0.f)
                    m_window.draw(m_txUndo,
                                  {
                                      .color = za::Color::whiteWithAlpha(static_cast<za::U8>(
                                          remap(easeInOutSine(m_undoCountdown), 0.f, 1.f, 0.f, 255.f))),
                                  },
                                  {.view = m_windowView, .shader = &m_shader});

                m_imGuiContext.render(m_window);
            }

            m_window.display();
        }

        return true;
    }
};

} // namespace


////////////////////////////////////////////////////////////
int main()
{
    auto audioContext    = za::AudioContext::create().value();
    auto graphicsContext = za::GraphicsContext::create().value();

    Game game;

    if (!game.run())
        return 1;

    return 0;
}
