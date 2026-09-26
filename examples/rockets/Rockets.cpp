#include "ExampleProfiler/Profiler.hpp"
#include "ExampleProfiler/ProfilerImGui.hpp"
#include "SoAPFR.hpp"

#include "ExampleUtils/RNGFast.hpp"
#include "ExampleUtils/Sampler.hpp"
#include "ExampleUtils/Scaling.hpp"

#include "Zancle/Graphics/Color.hpp"
#include "Zancle/Graphics/DefaultShader.hpp"
#include "Zancle/Graphics/DrawInstancedIndexedVerticesSettings.hpp"
#include "Zancle/Graphics/DrawableBatch.hpp"
#include "Zancle/Graphics/Font.hpp"
#include "Zancle/Graphics/Glsl.hpp"
#include "Zancle/Graphics/GraphicsContext.hpp"
#include "Zancle/Graphics/Image.hpp"
#include "Zancle/Graphics/InstanceAttributeBinder.hpp"
#include "Zancle/Graphics/InstancedQuad.hpp"
#include "Zancle/Graphics/PrimitiveType.hpp"
#include "Zancle/Graphics/RenderStates.hpp"
#include "Zancle/Graphics/RenderTarget.hpp"
#include "Zancle/Graphics/RenderTexture.hpp"
#include "Zancle/Graphics/RenderWindow.hpp"
#include "Zancle/Graphics/Shader.hpp"
#include "Zancle/Graphics/Sprite.hpp"
#include "Zancle/Graphics/Text.hpp"
#include "Zancle/Graphics/Texture.hpp"
#include "Zancle/Graphics/TextureAtlas.hpp"
#include "Zancle/Graphics/VAOHandle.hpp"
#include "Zancle/Graphics/VBOHandle.hpp"
#include "Zancle/Graphics/View.hpp" // IWYU pragma: keep

#include "Zancle/ImGui/ImGuiContext.hpp"
#include "Zancle/ImGui/IncludeImGui.hpp"

#include "Zancle/Window/Event.hpp"
#include "Zancle/Window/EventUtils.hpp"
#include "Zancle/Window/Keyboard.hpp"
#include "Zancle/Window/VideoMode.hpp"
#include "Zancle/Window/VideoModeUtils.hpp"

#include "Zancle/IO/Path.hpp"

#include "Zancle/Algorithm/SwapAndPop.hpp"

#include "Zancle/Chrono/Clock.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Geometry/Angle.hpp"
#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Vocabulary/Optional.hpp"
#include "Zancle/Vocabulary/UniquePtr.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/Macros.hpp"
#include "Zancle/Base/SizeT.hpp"


namespace
{


////////////////////////////////////////////////////////////
struct ParticleInstanceData // NOLINT(cppcoreguidelines-pro-type-member-init)
{
    za::Vec2f position;
    float     scale;
    float     rotation;
    float     opacity;
};


////////////////////////////////////////////////////////////
constexpr const char* instancedVertexShader = R"glsl(

layout(location = 0) uniform vec3 za_u_mvpRow0;
layout(location = 1) uniform vec3 za_u_mvpRow1;
layout(location = 2) uniform sampler2D za_u_texture;
layout(location = 3) uniform vec2 za_u_invTextureSize;
layout(location = 4) uniform vec4 u_texRect;

layout(location = 0) in vec2 za_a_position;
layout(location = 1) in vec4 za_a_color; // Unused but part of `za::Vertex` struct
layout(location = 2) in vec2 za_a_texCoord;

// Per-instance attributes (unique for each sprite)
layout(location = 3) in vec2 instance_position;
layout(location = 4) in float instance_scale;
layout(location = 5) in float instance_rotation;
layout(location = 6) in float instance_opacity;

out vec4 za_v_color;
out vec2 za_v_texCoord;

void main()
{
    // scale in local (pixel) space
    vec2 local = za_a_position * u_texRect.zw;

    // inline rotate + scale
    float c = cos(instance_rotation);
    float s = sin(instance_rotation);
    float x = local.x * c - local.y * s;
    float y = local.x * s + local.y * c;
    vec2 worldPos = instance_position + instance_scale * vec2(x, y);

    gl_Position = vec4(dot(za_u_mvpRow0, vec3(worldPos, 1.0)), dot(za_u_mvpRow1, vec3(worldPos, 1.0)), 0.0, 1.0);

    za_v_color = vec4(1.0, 1.0, 1.0, instance_opacity);

    vec2 final_texCoord = u_texRect.xy + (za_a_texCoord * u_texRect.zw);
    za_v_texCoord = final_texCoord * za_u_invTextureSize;
}

)glsl";


////////////////////////////////////////////////////////////
za::Shader*                        instanceRenderingShader        = nullptr;
const za::Shader::UniformLocation* instanceRenderingULTextureRect = nullptr;
za::VAOHandle*                     instanceRenderingVAOGroup      = nullptr;
za::VBOHandle*                     instanceRenderingVBOs          = nullptr; // points to array of 4
za::Vector<ParticleInstanceData>   instanceRenderingDataBuffer[2];


////////////////////////////////////////////////////////////
RNGFast rng;


////////////////////////////////////////////////////////////
za::Texture* txAtlas = nullptr;
za::Rect2f   txrSmoke;
za::Rect2f   txrFire;
za::Rect2f   txrRocket;


////////////////////////////////////////////////////////////
[[nodiscard]] za::DrawInstancedIndexedVerticesSettings makeInstancedDrawSettings(const za::SizeT nInstances)
{
    return {
        .vaoHandle     = *instanceRenderingVAOGroup,
        .vertexSpan    = za::instancedQuadVertices,
        .indexSpan     = za::instancedQuadIndices,
        .instanceCount = nInstances,
        .primitiveType = za::PrimitiveType::Triangles,
    };
}


////////////////////////////////////////////////////////////
[[nodiscard]] za::RenderStates makeInstancedDrawRenderStates(const za::View& view)
{
    return {.view = view, .texture = txAtlas, .shader = instanceRenderingShader};
}


////////////////////////////////////////////////////////////
[[gnu::always_inline]] inline void drawParticleImpl(
    za::RenderTarget& rt,
    const za::View&   view,
    const za::Vec2f   position,
    const za::Vec2f   scale,
    const float       rotation,
    const za::Rect2f  txr,
    const float       opacity)
{
    rt.draw(
        za::Sprite{
            .position    = position,
            .scale       = scale,
            .origin      = txr.size / 2.f,
            .rotation    = za::radians(rotation),
            .textureRect = txr,
            .color       = za::Color::whiteWithAlpha(static_cast<za::U8>(opacity * 255.f)),
        },
        za::RenderStates{.view = view, .texture = txAtlas});
}


////////////////////////////////////////////////////////////
[[gnu::always_inline]] inline void drawRocketImpl(za::RenderTarget& rt, const za::View& view, const za::Vec2f position)
{
    rt.draw(
        za::Sprite{
            .position    = position,
            .scale       = {0.15f, 0.15f},
            .origin      = txrRocket.size / 2.f,
            .rotation    = za::radians(0.f),
            .textureRect = txrRocket,
            .color       = za::Color::White,
        },
        za::RenderStates{.view = view, .texture = txAtlas});
}


////////////////////////////////////////////////////////////
namespace OOP
{
////////////////////////////////////////////////////////////
struct World;

////////////////////////////////////////////////////////////
struct Entity
{
    World* world = nullptr;

    za::Vec2f position;
    za::Vec2f velocity;
    za::Vec2f acceleration;

    bool alive = true;

    virtual ~Entity() = default;

    virtual void update(float dt)
    {
        position += velocity * dt;
        velocity += acceleration * dt;
    }

    virtual void draw(za::RenderTarget&, const za::View&)
    {
    }
};

////////////////////////////////////////////////////////////
struct World
{
    za::Vector<za::UniquePtr<Entity>> entities;

    void cleanup()
    {
        za::vectorSwapAndPopIf(entities, [](const auto& entity) { return !entity->alive; });
    }

    void update(float dt)
    {
        // Cannot use range-based `for` as new elements are created while updating
        for (za::SizeT i = 0u; i < entities.size(); ++i) // NOLINT(modernize-loop-convert)
            entities[i]->update(dt);
    }

    void draw(za::RenderTarget& rt, const za::View& view)
    {
        for (const auto& entity : entities)
            entity->draw(rt, view);
    }

    template <typename T>
    T& addEntity()
    {
        auto  newEntity = za::makeUnique<T>();
        auto& result    = *newEntity;

        entities.emplaceBack(ZA_MOVE(newEntity));

        result.world = this;
        return result;
    }
};

////////////////////////////////////////////////////////////
struct Emitter : Entity // NOLINT(cppcoreguidelines-pro-type-member-init)
{
    float spawnTimer;
    float spawnRate;

    virtual void spawnParticle() = 0;

    void update(float dt) override
    {
        Entity::update(dt);

        spawnTimer += spawnRate * dt;

        for (; spawnTimer >= 1.f; spawnTimer -= 1.f)
            spawnParticle();
    }
};

////////////////////////////////////////////////////////////
struct Particle : Entity // NOLINT(cppcoreguidelines-pro-type-member-init)
{
    float scale;
    float opacity;
    float rotation;

    float scaleRate;
    float opacityChange;
    float angularVelocity;

    void update(float dt) override
    {
        Entity::update(dt);

        scale += scaleRate * dt;
        opacity += opacityChange * dt;
        rotation += angularVelocity * dt;

        alive = opacity > 0.f;
    }
};

////////////////////////////////////////////////////////////
struct SmokeParticle final : Particle
{
    void draw(za::RenderTarget& rt, const za::View& view) override
    {
        drawParticleImpl(rt, view, position, {scale, scale}, rotation, txrSmoke, opacity);
    }
};

////////////////////////////////////////////////////////////
struct FireParticle final : Particle
{
    void draw(za::RenderTarget& rt, const za::View& view) override
    {
        drawParticleImpl(rt, view, position, {scale, scale}, rotation, txrFire, opacity);
    }
};

////////////////////////////////////////////////////////////
struct SmokeEmitter final : Emitter
{
    void spawnParticle() override
    {
        auto& p = world->addEntity<SmokeParticle>();

        p.position     = position;
        p.velocity     = rng.getVec2f({-0.2f, -0.2f}, {0.2f, 0.2f}) * 0.5f;
        p.acceleration = {0.f, -0.011f};

        p.scale    = rng.getF(0.0025f, 0.0035f);
        p.opacity  = rng.getF(0.05f, 0.25f);
        p.rotation = rng.getF(0.f, 6.28f);

        p.scaleRate       = rng.getF(0.001f, 0.003f) * 2.75f;
        p.opacityChange   = -rng.getF(0.001f, 0.002f) * 3.25f;
        p.angularVelocity = rng.getF(-0.02f, 0.02f);
    }
};

////////////////////////////////////////////////////////////
struct FireEmitter final : Emitter
{
    void spawnParticle() override
    {
        auto& p = world->addEntity<FireParticle>();

        p.position     = position;
        p.velocity     = rng.getVec2f({-0.3f, -0.8f}, {0.3f, -0.2f});
        p.acceleration = {0.f, 0.07f};

        p.scale    = rng.getF(0.5f, 0.7f) * 0.085f;
        p.opacity  = rng.getF(0.2f, 0.4f) * 0.85f;
        p.rotation = rng.getF(0.f, 6.28f);

        p.scaleRate       = -rng.getF(0.001f, 0.003f) * 0.25f;
        p.opacityChange   = -0.001f;
        p.angularVelocity = rng.getF(-0.002f, 0.002f);
    }
};

////////////////////////////////////////////////////////////
struct Rocket final : Entity
{
    SmokeEmitter* smokeEmitter = nullptr;
    FireEmitter*  fireEmitter  = nullptr;

    void init()
    {
        smokeEmitter               = &world->addEntity<SmokeEmitter>();
        smokeEmitter->position     = position;
        smokeEmitter->velocity     = {};
        smokeEmitter->acceleration = {};
        smokeEmitter->spawnTimer   = 0.f;
        smokeEmitter->spawnRate    = 2.5f;

        fireEmitter               = &world->addEntity<FireEmitter>();
        fireEmitter->position     = position;
        fireEmitter->velocity     = {};
        fireEmitter->acceleration = {};
        fireEmitter->spawnTimer   = 0.f;
        fireEmitter->spawnRate    = 1.25f;
    }

    void update(float dt) override
    {
        Entity::update(dt);

        smokeEmitter->position = position - za::Vec2f{12.f, 0.f};
        fireEmitter->position  = position - za::Vec2f{12.f, 0.f};

        if (position.x > 1680.f + 64.f)
        {
            alive = false;

            smokeEmitter->alive = false;
            fireEmitter->alive  = false;
        }
    }

    void draw(za::RenderTarget& rt, const za::View& view) override
    {
        drawRocketImpl(rt, view, position);
    }
};

} // namespace OOP

#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wmissing-designated-field-initializers"
#endif

////////////////////////////////////////////////////////////
namespace Shared
{

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

////////////////////////////////////////////////////////////
template <typename TParticle>
[[nodiscard, gnu::always_inline]] inline TParticle makeAoSSmokeParticle(const za::Vec2f position)
{
    return {
        .position     = position,
        .velocity     = rng.getVec2f({-0.2f, -0.2f}, {0.2f, 0.2f}) * 0.5f,
        .acceleration = {0.f, -0.011f},

        .scale    = rng.getF(0.0025f, 0.0035f),
        .opacity  = rng.getF(0.05f, 0.25f),
        .rotation = rng.getF(0.f, 6.28f),

        .scaleRate       = rng.getF(0.001f, 0.003f) * 2.75f,
        .opacityChange   = -rng.getF(0.001f, 0.002f) * 3.25f,
        .angularVelocity = rng.getF(-0.02f, 0.02f),
    };
}


////////////////////////////////////////////////////////////
template <typename TParticle>
[[nodiscard, gnu::always_inline]] inline TParticle makeAoSFireParticle(const za::Vec2f position)
{
    return {
        .position     = position,
        .velocity     = rng.getVec2f({-0.3f, -0.8f}, {0.3f, -0.2f}),
        .acceleration = {0.f, 0.07f},

        .scale    = rng.getF(0.5f, 0.7f) * 0.085f,
        .opacity  = rng.getF(0.2f, 0.4f) * 0.85f,
        .rotation = rng.getF(0.f, 6.28f),

        .scaleRate       = -rng.getF(0.001f, 0.003f) * 0.25f,
        .opacityChange   = -0.001f,
        .angularVelocity = rng.getF(-0.002f, 0.002f),
    };
}

#pragma GCC diagnostic pop

}; // namespace Shared


////////////////////////////////////////////////////////////
namespace AOS
{
////////////////////////////////////////////////////////////
enum class ParticleType
{
    Smoke,
    Fire
};

////////////////////////////////////////////////////////////
struct Emitter
{
    za::Vec2f position;
    za::Vec2f velocity;
    za::Vec2f acceleration;

    float spawnTimer;
    float spawnRate;

    ParticleType type;
};

////////////////////////////////////////////////////////////
struct Particle // NOLINT(cppcoreguidelines-pro-type-member-init)
{
    za::Vec2f position;
    za::Vec2f velocity;
    za::Vec2f acceleration;

    float scale;
    float opacity;
    float rotation;

    float scaleRate;
    float opacityChange;
    float angularVelocity;

    ParticleType type;
};

////////////////////////////////////////////////////////////
struct Rocket // NOLINT(cppcoreguidelines-pro-type-member-init)
{
    za::Vec2f position;
    za::Vec2f velocity;
    za::Vec2f acceleration;

    za::SizeT smokeEmitterIdx;
    za::SizeT fireEmitterIdx;
};

////////////////////////////////////////////////////////////
struct World
{
    za::Vector<za::Optional<Emitter>> emitters;
    za::Vector<Particle>              particles;
    za::Vector<Rocket>                rockets;

    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::SizeT addEmitter(const Emitter& emitter)
    {
        for (za::SizeT i = 0u; i < emitters.size(); ++i)
            if (!emitters[i].hasValue())
            {
                emitters[i].emplace(emitter);
                return i;
            }

        emitters.emplaceBack(emitter);
        return emitters.size() - 1;
    }

    ////////////////////////////////////////////////////////////
    Rocket& addRocket(const Rocket& r)
    {
        Rocket& rocket = rockets.emplaceBack(r);

        rocket.smokeEmitterIdx = addEmitter({
            .position     = rocket.position,
            .velocity     = {},
            .acceleration = {},
            .spawnTimer   = 0.f,
            .spawnRate    = 2.5f,
            .type         = ParticleType::Smoke,
        });

        rocket.fireEmitterIdx = addEmitter({
            .position     = rocket.position,
            .velocity     = {},
            .acceleration = {},
            .spawnTimer   = 0.f,
            .spawnRate    = 1.25f,
            .type         = ParticleType::Fire,
        });

        return rocket;
    }

    ////////////////////////////////////////////////////////////
    void update(const float dt)
    {
        {
            SFEX_PROFILE_SCOPE("particles");

            for (Particle& p : particles)
            {
                p.position += p.velocity * dt;
                p.velocity += p.acceleration * dt;
                p.scale += p.scaleRate * dt;
                p.opacity += p.opacityChange * dt;
                p.rotation += p.angularVelocity * dt;
            }
        }

        {
            SFEX_PROFILE_SCOPE("emitters");
            for (za::Optional<Emitter>& e : emitters)
            {
                if (!e.hasValue())
                    continue;

                e->position += e->velocity * dt;
                e->velocity += e->acceleration * dt;
                e->spawnTimer += e->spawnRate * dt;

                for (; e->spawnTimer >= 1.f; e->spawnTimer -= 1.f)
                {
                    if (e->type == ParticleType::Smoke)
                        particles.pushBack(Shared::makeAoSSmokeParticle<Particle>(e->position));
                    else if (e->type == ParticleType::Fire)
                        particles.pushBack(Shared::makeAoSFireParticle<Particle>(e->position));

                    particles.back().type = e->type;
                }
            }
        }

        {
            SFEX_PROFILE_SCOPE("rockets");

            for (Rocket& r : rockets)
            {
                r.position += r.velocity * dt;
                r.velocity += r.acceleration * dt;

                if (za::Optional<Emitter>& se = emitters[r.smokeEmitterIdx])
                    se->position = r.position - za::Vec2f{12.f, 0.f};

                if (za::Optional<Emitter>& fe = emitters[r.fireEmitterIdx])
                    fe->position = r.position - za::Vec2f{12.f, 0.f};
            }
        }
    }

    ////////////////////////////////////////////////////////////
    void cleanup()
    {
        za::vectorSwapAndPopIf(particles, [](const Particle& p) { return p.opacity <= 0.f; });

        za::vectorSwapAndPopIf(rockets,
                               [&](const Rocket& r)
        {
            if (r.position.x <= 1680.f + 64.f)
                return false;

            emitters[r.smokeEmitterIdx].reset();
            emitters[r.fireEmitterIdx].reset();

            return true; // Out of bounds
        });
    }

    ////////////////////////////////////////////////////////////
    void draw(za::RenderTarget& rt, const za::View& view)
    {
        const auto drawParticlesInstanced = [&](const auto& instanceBuffer, const za::Rect2f& txr)
        {
            auto setupSpriteInstanceAttribs = [&](za::InstanceAttributeBinder& binder)
            {
                binder.uploadContiguousData(instanceRenderingVBOs[0], instanceBuffer);

                binder.setupField<&ParticleInstanceData::position>(3);
                binder.setupField<&ParticleInstanceData::scale>(4);
                binder.setupField<&ParticleInstanceData::rotation>(5);
                binder.setupField<&ParticleInstanceData::opacity>(6);
            };

            instanceRenderingShader->setUniform(*instanceRenderingULTextureRect,
                                                za::Glsl::Vec4{txr.position.x, txr.position.y, txr.size.x, txr.size.y});

            rt.drawInstancedIndexedVertices(makeInstancedDrawSettings(instanceBuffer.size()),
                                            setupSpriteInstanceAttribs,
                                            makeInstancedDrawRenderStates(view));
        };

        const auto nParticles = particles.size();

        instanceRenderingDataBuffer[0].clear();
        instanceRenderingDataBuffer[0].reserve(nParticles);

        instanceRenderingDataBuffer[1].clear();
        instanceRenderingDataBuffer[1].reserve(nParticles);

        for (const Particle& p : particles)
            (p.type == ParticleType::Smoke ? instanceRenderingDataBuffer[0] : instanceRenderingDataBuffer[1])
                .emplaceBack(p.position, p.scale, p.rotation, p.opacity);

        {
            SFEX_PROFILE_SCOPE("smoke particles");
            drawParticlesInstanced(instanceRenderingDataBuffer[0], txrSmoke);
        }

        {
            SFEX_PROFILE_SCOPE("fire particles");
            drawParticlesInstanced(instanceRenderingDataBuffer[1], txrFire);
        }

        {
            SFEX_PROFILE_SCOPE("rockets");

            for (const auto& r : rockets)
                drawRocketImpl(rt, view, r.position);
        }
    }
};

} // namespace AOS


////////////////////////////////////////////////////////////
namespace Shared
{
////////////////////////////////////////////////////////////
template <typename TEmitter>
struct AddU16EmitterMixin
{
    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::U16 addEmitter(za::Vector<za::Optional<TEmitter>>& emitters, const TEmitter& emitter)
    {
        for (za::SizeT i = 0u; i < emitters.size(); ++i)
            if (!emitters[i].hasValue())
            {
                emitters[i].emplace(emitter);
                return static_cast<za::U16>(i);
            }

        emitters.emplaceBack(emitter);
        return static_cast<za::U16>(emitters.size() - 1);
    }
};

////////////////////////////////////////////////////////////
template <typename TRocket>
struct AddRocketMixin
{
    ////////////////////////////////////////////////////////////
    TRocket& addRocket(this auto&& self, const TRocket& r)
    {
        TRocket& rocket = self.rockets.emplaceBack(r);

        rocket.smokeEmitterIdx = self.addEmitter(self.smokeEmitters,
                                                 {
                                                     .position     = rocket.position,
                                                     .velocity     = {},
                                                     .acceleration = {},
                                                     .spawnTimer   = 0.f,
                                                     .spawnRate    = 2.5f,
                                                 });

        rocket.fireEmitterIdx = self.addEmitter(self.fireEmitters,
                                                {
                                                    .position     = rocket.position,
                                                    .velocity     = {},
                                                    .acceleration = {},
                                                    .spawnTimer   = 0.f,
                                                    .spawnRate    = 1.25f,
                                                });

        return rocket;
    }
};

} // namespace Shared


////////////////////////////////////////////////////////////
namespace AOSImproved
{
////////////////////////////////////////////////////////////
struct Emitter
{
    za::Vec2f position;
    za::Vec2f velocity;
    za::Vec2f acceleration;

    float spawnTimer;
    float spawnRate;
};

////////////////////////////////////////////////////////////
struct Particle // NOLINT(cppcoreguidelines-pro-type-member-init)
{
    za::Vec2f position;
    za::Vec2f velocity;
    za::Vec2f acceleration;

    float scale;
    float opacity;
    float rotation;

    float scaleRate;
    float opacityChange;
    float angularVelocity;
};

////////////////////////////////////////////////////////////
struct Rocket // NOLINT(cppcoreguidelines-pro-type-member-init)
{
    za::Vec2f position;
    za::Vec2f velocity;
    za::Vec2f acceleration;

    za::U16 smokeEmitterIdx;
    za::U16 fireEmitterIdx;
};

////////////////////////////////////////////////////////////
struct World : Shared::AddU16EmitterMixin<Emitter>, Shared::AddRocketMixin<Rocket>
{
    za::Vector<za::Optional<Emitter>> smokeEmitters;
    za::Vector<za::Optional<Emitter>> fireEmitters;
    za::Vector<Particle>              smokeParticles;
    za::Vector<Particle>              fireParticles;
    za::Vector<Rocket>                rockets;

    ////////////////////////////////////////////////////////////
    void update(const float dt)
    {
        const auto updateParticle = [&](Particle& p)
        {
            p.position += p.velocity * dt;
            p.velocity += p.acceleration * dt;
            p.scale += p.scaleRate * dt;
            p.opacity += p.opacityChange * dt;
            p.rotation += p.angularVelocity * dt;
        };

        const auto updateEmitter = [&](za::Optional<Emitter>& e, auto&& fSpawn)
        {
            if (!e.hasValue())
                return;

            e->position += e->velocity * dt;
            e->velocity += e->acceleration * dt;
            e->spawnTimer += e->spawnRate * dt;

            for (; e->spawnTimer >= 1.f; e->spawnTimer -= 1.f)
                fSpawn();
        };

        {
            SFEX_PROFILE_SCOPE("smoke particles");

            for (Particle& p : smokeParticles)
                updateParticle(p);
        }

        {
            SFEX_PROFILE_SCOPE("fire particles");

            for (Particle& p : fireParticles)
                updateParticle(p);
        }

        {
            SFEX_PROFILE_SCOPE("smoke emitters");

            for (za::Optional<Emitter>& e : smokeEmitters)
                updateEmitter(e, [&] { smokeParticles.pushBack(Shared::makeAoSSmokeParticle<Particle>(e->position)); });
        }

        {
            SFEX_PROFILE_SCOPE("fire emitters");

            for (za::Optional<Emitter>& e : fireEmitters)
                updateEmitter(e, [&] { fireParticles.pushBack(Shared::makeAoSFireParticle<Particle>(e->position)); });
        }

        {
            SFEX_PROFILE_SCOPE("rockets");

            for (Rocket& r : rockets)
            {
                r.position += r.velocity * dt;
                r.velocity += r.acceleration * dt;

                if (za::Optional<Emitter>& se = smokeEmitters[r.smokeEmitterIdx])
                    se->position = r.position - za::Vec2f{12.f, 0.f};

                if (za::Optional<Emitter>& fe = fireEmitters[r.fireEmitterIdx])
                    fe->position = r.position - za::Vec2f{12.f, 0.f};
            }
        }
    }

    ////////////////////////////////////////////////////////////
    void cleanup()
    {
        za::vectorSwapAndPopIf(smokeParticles, [](const Particle& p) { return p.opacity <= 0.f; });
        za::vectorSwapAndPopIf(fireParticles, [](const Particle& p) { return p.opacity <= 0.f; });

        za::vectorSwapAndPopIf(rockets,
                               [&](const Rocket& r)
        {
            if (r.position.x <= 1680.f + 64.f)
                return false;

            smokeEmitters[r.smokeEmitterIdx].reset();
            fireEmitters[r.fireEmitterIdx].reset();

            return true; // Out of bounds
        });
    }

    ////////////////////////////////////////////////////////////
    void draw(za::RenderTarget& rt, const za::View& view)
    {
        const auto drawParticlesInstanced = [&](const za::Rect2f& txr, const auto& particles)
        {
            const auto nParticles = particles.size();

            instanceRenderingDataBuffer[0].clear();
            instanceRenderingDataBuffer[0].reserve(nParticles);

            for (za::SizeT i = 0u; i < nParticles; ++i)
                instanceRenderingDataBuffer[0]
                    .emplaceBack(particles[i].position, particles[i].scale, particles[i].rotation, particles[i].opacity);

            auto setupSpriteInstanceAttribs = [&](za::InstanceAttributeBinder& binder)
            {
                binder.uploadContiguousData(instanceRenderingVBOs[0], instanceRenderingDataBuffer[0]);

                binder.setupField<&ParticleInstanceData::position>(3);
                binder.setupField<&ParticleInstanceData::scale>(4);
                binder.setupField<&ParticleInstanceData::rotation>(5);
                binder.setupField<&ParticleInstanceData::opacity>(6);
            };

            instanceRenderingShader->setUniform(*instanceRenderingULTextureRect,
                                                za::Glsl::Vec4{txr.position.x, txr.position.y, txr.size.x, txr.size.y});

            rt.drawInstancedIndexedVertices(makeInstancedDrawSettings(nParticles),
                                            setupSpriteInstanceAttribs,
                                            makeInstancedDrawRenderStates(view));
        };

        {
            SFEX_PROFILE_SCOPE("smoke particles");
            drawParticlesInstanced(txrSmoke, smokeParticles);
        }

        {
            SFEX_PROFILE_SCOPE("fire particles");
            drawParticlesInstanced(txrFire, fireParticles);
        }

        {
            SFEX_PROFILE_SCOPE("rockets");

            for (const auto& r : rockets)
                drawRocketImpl(rt, view, r.position);
        }
    }
};

} // namespace AOSImproved


////////////////////////////////////////////////////////////
namespace SOAManual
{
////////////////////////////////////////////////////////////
struct ParticleSoA
{
    za::Vector<za::Vec2f> positions;
    za::Vector<za::Vec2f> velocities;
    za::Vector<za::Vec2f> accelerations;

    za::Vector<float> scales;
    za::Vector<float> opacities;
    za::Vector<float> rotations;

    za::Vector<float> scaleRates;
    za::Vector<float> opacityChanges;
    za::Vector<float> angularVelocities;

    ////////////////////////////////////////////////////////////
    void forEachVector(auto&& f)
    {
        f(positions);
        f(velocities);
        f(accelerations);

        f(scales);
        f(opacities);
        f(rotations);

        f(scaleRates);
        f(opacityChanges);
        f(angularVelocities);
    }

    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] za::SizeT getSize() const
    {
        return positions.size();
    }
};

////////////////////////////////////////////////////////////
struct Emitter
{
    za::Vec2f position;
    za::Vec2f velocity;
    za::Vec2f acceleration;

    float spawnTimer;
    float spawnRate;
};

////////////////////////////////////////////////////////////
struct Rocket // NOLINT(cppcoreguidelines-pro-type-member-init)
{
    za::Vec2f position;
    za::Vec2f velocity;
    za::Vec2f acceleration;

    za::U16 smokeEmitterIdx;
    za::U16 fireEmitterIdx;
};

////////////////////////////////////////////////////////////
struct World : Shared::AddU16EmitterMixin<Emitter>, Shared::AddRocketMixin<Rocket>
{
    za::Vector<za::Optional<Emitter>> smokeEmitters;
    za::Vector<za::Optional<Emitter>> fireEmitters;
    ParticleSoA                       smokeParticles;
    ParticleSoA                       fireParticles;
    za::Vector<Rocket>                rockets;

    ////////////////////////////////////////////////////////////
    void update(const float dt)
    {
        const auto updateParticles = [&](auto& soa)
        {
            const auto nParticles = soa.getSize();

            for (za::SizeT i = 0u; i < nParticles; ++i)
            {
                soa.velocities[i] += soa.accelerations[i] * dt;
                soa.positions[i] += soa.velocities[i] * dt;
                soa.scales[i] += soa.scaleRates[i] * dt;
                soa.opacities[i] += soa.opacityChanges[i] * dt;
                soa.rotations[i] += soa.angularVelocities[i] * dt;
            }
        };

        {
            SFEX_PROFILE_SCOPE("smoke particles");
            updateParticles(smokeParticles);
        }

        {
            SFEX_PROFILE_SCOPE("fire particles");
            updateParticles(fireParticles);
        }

        {
            SFEX_PROFILE_SCOPE("smoke emitters");

            for (za::Optional<Emitter>& e : smokeEmitters)
            {
                if (!e.hasValue())
                    continue;

                e->position += e->velocity * dt;
                e->velocity += e->acceleration * dt;
                e->spawnTimer += e->spawnRate * dt;

                for (; e->spawnTimer >= 1.f; e->spawnTimer -= 1.f)
                {
                    smokeParticles.positions.pushBack(e->position);
                    smokeParticles.velocities.pushBack(rng.getVec2f({-0.2f, -0.2f}, {0.2f, 0.2f}) * 0.5f);
                    smokeParticles.accelerations.pushBack({0.f, -0.011f});
                    smokeParticles.scales.pushBack(rng.getF(0.0025f, 0.0035f));
                    smokeParticles.opacities.pushBack(rng.getF(0.05f, 0.25f));
                    smokeParticles.rotations.pushBack(rng.getF(0.f, 6.28f));
                    smokeParticles.scaleRates.pushBack(rng.getF(0.001f, 0.003f) * 2.75f);
                    smokeParticles.opacityChanges.pushBack(-rng.getF(0.001f, 0.002f) * 3.25f);
                    smokeParticles.angularVelocities.pushBack(rng.getF(-0.02f, 0.02f));
                }
            }
        }

        {
            SFEX_PROFILE_SCOPE("fire emitters");

            for (za::Optional<Emitter>& e : fireEmitters)
            {
                if (!e.hasValue())
                    continue;

                e->position += e->velocity * dt;
                e->velocity += e->acceleration * dt;
                e->spawnTimer += e->spawnRate * dt;

                for (; e->spawnTimer >= 1.f; e->spawnTimer -= 1.f)
                {
                    fireParticles.positions.pushBack(e->position);
                    fireParticles.velocities.pushBack(rng.getVec2f({-0.3f, -0.8f}, {0.3f, -0.2f}));
                    fireParticles.accelerations.pushBack({0.f, 0.07f});
                    fireParticles.scales.pushBack(rng.getF(0.5f, 0.7f) * 0.085f);
                    fireParticles.opacities.pushBack(rng.getF(0.2f, 0.4f) * 0.85f);
                    fireParticles.rotations.pushBack(rng.getF(0.f, 6.28f));
                    fireParticles.scaleRates.pushBack(-rng.getF(0.001f, 0.003f) * 0.25f);
                    fireParticles.opacityChanges.pushBack(-0.001f);
                    fireParticles.angularVelocities.pushBack(rng.getF(-0.002f, 0.002f));
                }
            }
        }

        {
            SFEX_PROFILE_SCOPE("rockets");

            for (Rocket& r : rockets)
            {
                r.position += r.velocity * dt;
                r.velocity += r.acceleration * dt;

                if (za::Optional<Emitter>& se = smokeEmitters[r.smokeEmitterIdx])
                    se->position = r.position - za::Vec2f{12.f, 0.f};

                if (za::Optional<Emitter>& fe = fireEmitters[r.fireEmitterIdx])
                    fe->position = r.position - za::Vec2f{12.f, 0.f};
            }
        }
    }
    ////////////////////////////////////////////////////////////
    void cleanup()
    {
        const auto soaEraseIf = [&](ParticleSoA& soa, auto&& predicate)
        {
            za::SizeT currentSize = soa.positions.size();

            for (za::SizeT i = currentSize; i-- > 0u;)
            {
                if (!predicate(soa, i))
                    continue;

                --currentSize;
                soa.forEachVector([&](auto& vec) { vec[i] = ZA_MOVE(vec[currentSize]); });
            }

            soa.forEachVector([&](auto& vec) { vec.resize(currentSize); });
        };

        soaEraseIf(smokeParticles, [](const ParticleSoA& soa, const za::SizeT i) { return soa.opacities[i] <= 0.f; });
        soaEraseIf(fireParticles, [](const ParticleSoA& soa, const za::SizeT i) { return soa.opacities[i] <= 0.f; });

        za::vectorSwapAndPopIf(rockets,
                               [&](const Rocket& r)
        {
            if (r.position.x <= 1680.f + 64.f)
                return false;

            smokeEmitters[r.smokeEmitterIdx].reset();
            fireEmitters[r.fireEmitterIdx].reset();

            return true; // Out of bounds
        });
    }

    ////////////////////////////////////////////////////////////
    void draw(za::RenderTarget& rt, const za::View& view)
    {
        const auto drawParticlesInstanced = [&](const za::Rect2f& txr, const auto& particles)
        {
            const auto nParticles = particles.positions.size();

            auto setupSpriteInstanceAttribs = [&](za::InstanceAttributeBinder& binder)
            {
                binder.uploadContiguousData(instanceRenderingVBOs[0], particles.positions);
                binder.setupFlat<za::Vec2f>(3);

                binder.uploadContiguousData(instanceRenderingVBOs[1], particles.scales);
                binder.setupFlat<float>(4);

                binder.uploadContiguousData(instanceRenderingVBOs[2], particles.rotations);
                binder.setupFlat<float>(5);

                binder.uploadContiguousData(instanceRenderingVBOs[3], particles.opacities);
                binder.setupFlat<float>(6);
            };

            instanceRenderingShader->setUniform(*instanceRenderingULTextureRect,
                                                za::Glsl::Vec4{txr.position.x, txr.position.y, txr.size.x, txr.size.y});

            rt.drawInstancedIndexedVertices(makeInstancedDrawSettings(nParticles),
                                            setupSpriteInstanceAttribs,
                                            makeInstancedDrawRenderStates(view));
        };

        {
            SFEX_PROFILE_SCOPE("smoke particles");
            drawParticlesInstanced(txrSmoke, smokeParticles);
        }

        {
            SFEX_PROFILE_SCOPE("fire particles");
            drawParticlesInstanced(txrFire, fireParticles);
        }

        {
            SFEX_PROFILE_SCOPE("rockets");

            for (const auto& r : rockets)
                drawRocketImpl(rt, view, r.position);
        }
    }
};

} // namespace SOAManual

////////////////////////////////////////////////////////////
namespace SOAMeta
{
////////////////////////////////////////////////////////////
struct Particle // NOLINT(cppcoreguidelines-pro-type-member-init)
{
    za::Vec2f position;
    za::Vec2f velocity;
    za::Vec2f acceleration;

    float scale;
    float opacity;
    float rotation;

    float scaleRate;
    float opacityChange;
    float angularVelocity;
};

////////////////////////////////////////////////////////////
using ParticleSoA = SoAFor<Particle>;

////////////////////////////////////////////////////////////
struct Emitter
{
    za::Vec2f position;
    za::Vec2f velocity;
    za::Vec2f acceleration;

    float spawnTimer;
    float spawnRate;
};

////////////////////////////////////////////////////////////
struct Rocket // NOLINT(cppcoreguidelines-pro-type-member-init)
{
    za::Vec2f position;
    za::Vec2f velocity;
    za::Vec2f acceleration;

    za::U16 smokeEmitterIdx;
    za::U16 fireEmitterIdx;
};

////////////////////////////////////////////////////////////
struct World : Shared::AddU16EmitterMixin<Emitter>, Shared::AddRocketMixin<Rocket>
{
    za::Vector<za::Optional<Emitter>> smokeEmitters;
    za::Vector<za::Optional<Emitter>> fireEmitters;
    ParticleSoA                       smokeParticles;
    ParticleSoA                       fireParticles;
    za::Vector<Rocket>                rockets;

    ////////////////////////////////////////////////////////////
    void update(const float dt)
    {
        auto updateParticles = [&](auto& soa)
        {
            soa.withAll(
                [&](za::Vec2f&      position,
                    za::Vec2f&      velocity,
                    const za::Vec2f acceleration,
                    float&          scale,
                    float&          opacity,
                    float&          rotation,
                    const float&    scaleRate,
                    const float&    opacityChange,
                    const float&    angularVelocity)
            {
                velocity += acceleration * dt;
                position += velocity * dt;
                scale += scaleRate * dt;
                opacity += opacityChange * dt;
                rotation += angularVelocity * dt;
            });
        };

        {
            SFEX_PROFILE_SCOPE("smoke particles");
            updateParticles(smokeParticles);
        }

        {
            SFEX_PROFILE_SCOPE("fire particles");
            updateParticles(fireParticles);
        }

        {
            SFEX_PROFILE_SCOPE("smoke emitters");

            for (za::Optional<Emitter>& e : smokeEmitters)
            {
                if (!e.hasValue())
                    continue;

                e->position += e->velocity * dt;
                e->velocity += e->acceleration * dt;
                e->spawnTimer += e->spawnRate * dt;

                {
                    SFEX_PROFILE_SCOPE("spawn particles");

                    for (; e->spawnTimer >= 1.f; e->spawnTimer -= 1.f)
                        smokeParticles.pushBack(Shared::makeAoSSmokeParticle<Particle>(e->position));
                }
            }
        }

        {
            SFEX_PROFILE_SCOPE("fire emitters");

            for (za::Optional<Emitter>& e : fireEmitters)
            {
                if (!e.hasValue())
                    continue;

                e->position += e->velocity * dt;
                e->velocity += e->acceleration * dt;
                e->spawnTimer += e->spawnRate * dt;

                {
                    SFEX_PROFILE_SCOPE("spawn particles");

                    for (; e->spawnTimer >= 1.f; e->spawnTimer -= 1.f)
                        fireParticles.pushBack(Shared::makeAoSFireParticle<Particle>(e->position));
                }
            }
        }

        {
            SFEX_PROFILE_SCOPE("rockets");

            for (Rocket& r : rockets)
            {
                r.position += r.velocity * dt;
                r.velocity += r.acceleration * dt;

                if (za::Optional<Emitter>& se = smokeEmitters[r.smokeEmitterIdx])
                    se->position = r.position - za::Vec2f{12.f, 0.f};

                if (za::Optional<Emitter>& fe = fireEmitters[r.fireEmitterIdx])
                    fe->position = r.position - za::Vec2f{12.f, 0.f};
            }
        }
    }

    ////////////////////////////////////////////////////////////
    void cleanup()
    {
        smokeParticles.eraseIfBySwapping<&Particle::opacity>([](const float opacity) { return opacity <= 0.f; });
        fireParticles.eraseIfBySwapping<&Particle::opacity>([](const float opacity) { return opacity <= 0.f; });

        za::vectorSwapAndPopIf(rockets,
                               [&](const Rocket& r)
        {
            if (r.position.x <= 1680.f + 64.f)
                return false;

            smokeEmitters[r.smokeEmitterIdx].reset();
            fireEmitters[r.fireEmitterIdx].reset();

            return true; // Out of bounds
        });
    }

    ////////////////////////////////////////////////////////////
    void draw(za::RenderTarget& rt, const za::View& view)
    {
        const auto drawParticlesInstanced = [&](const za::Rect2f& txr, const auto& particles)
        {
            const auto nParticles = particles.getSize();

            auto setupSpriteInstanceAttribs = [&](za::InstanceAttributeBinder& binder)
            {
                binder.uploadContiguousData(instanceRenderingVBOs[0], particles.template get<&Particle::position>());
                binder.setupFlat<za::Vec2f>(3);

                binder.uploadContiguousData(instanceRenderingVBOs[1], particles.template get<&Particle::scale>());
                binder.setupFlat<float>(4);

                binder.uploadContiguousData(instanceRenderingVBOs[2], particles.template get<&Particle::rotation>());
                binder.setupFlat<float>(5);

                binder.uploadContiguousData(instanceRenderingVBOs[3], particles.template get<&Particle::opacity>());
                binder.setupFlat<float>(6);
            };

            instanceRenderingShader->setUniform(*instanceRenderingULTextureRect,
                                                za::Glsl::Vec4{txr.position.x, txr.position.y, txr.size.x, txr.size.y});

            rt.drawInstancedIndexedVertices(makeInstancedDrawSettings(nParticles),
                                            setupSpriteInstanceAttribs,
                                            makeInstancedDrawRenderStates(view));
        };

        {
            SFEX_PROFILE_SCOPE("smoke particles");
            drawParticlesInstanced(txrSmoke, smokeParticles);
        }

        {
            SFEX_PROFILE_SCOPE("fire particles");
            drawParticlesInstanced(txrFire, fireParticles);
        }

        {
            SFEX_PROFILE_SCOPE("rockets");

            for (const auto& r : rockets)
                drawRocketImpl(rt, view, r.position);
        }
    }
};

} // namespace SOAMeta

////////////////////////////////////////////////////////////
enum class Mode
{
    OOP,
    AOS,
    AOSImproved,
    SOAManual,
    SOAMeta,
};

} // namespace


////////////////////////////////////////////////////////////
/// Main
///
////////////////////////////////////////////////////////////
int main()
{
    //
    //
    // Set up graphics context
    auto graphicsContext = za::GraphicsContext::create().value();

    //
    //
    // Set up window and render texture
    constexpr za::Vec2f resolution{1680.f, 1050.f};

    auto window = makeDPIScaledRenderWindow(
                      {
                          .size           = resolution.toVec2u(),
                          .title          = "Rockets",
                          .fullscreen     = false,
                          .resizable      = false,
                          .closable       = false,
                          .hasTitlebar    = false,
                          .vsync          = false,
                          .frametimeLimit = 144u,
                      })
                      .value();

    auto windowView = computeAspectRatioAwareView(window.getSize().toVec2f(), resolution);
    auto worldView  = za::View::fromScreenSize(resolution);

    auto rtGame = makeAARenderTexture(resolution.toVec2u(), {.sampleCount = 8u}).value();

    //
    //
    // Set up imgui
    za::ImGuiContext imGuiContext;

    //
    //
    // Set up texture atlas
    za::TextureAtlas textureAtlas{za::Texture::create({512u, 256u}, {.smooth = true}).value()};
    txAtlas = &textureAtlas.getTexture();

    const auto addImgResourceToAtlas = [&](const za::Path& path)
    { return textureAtlas.add(za::Image::loadFromFile("resources" / path).value()).value(); };

    //
    //
    // Load fonts
    ImFont* const fontImGuiGeistMono{ImGui::GetIO().Fonts->AddFontFromFileTTF("resources/geistmono.ttf", 32.f)};

    //
    //
    // Load images and add to texture atlas
    txrSmoke  = addImgResourceToAtlas("pSmoke.png");
    txrFire   = addImgResourceToAtlas("pFire.png");
    txrRocket = addImgResourceToAtlas("rocket.png");

    //
    //
    // Instanced rendering setup
    // TODO P0: cleanup, improve user-facing API
    auto instancedRenderingShaderImpl = za::Shader::loadFromMemory({.vertexCode   = instancedVertexShader,
                                                                    .fragmentCode = za::DefaultShader::srcFragment})
                                            .value();

    instanceRenderingShader = &instancedRenderingShaderImpl;

    auto instancedRenderingVAOGroupImpl = za::VAOHandle{};
    instanceRenderingVAOGroup           = &instancedRenderingVAOGroupImpl;

    za::VBOHandle instanceRenderingVBOsImpl[4];
    instanceRenderingVBOs = instanceRenderingVBOsImpl;

    const auto instanceRenderingULTextureRectImpl = instancedRenderingShaderImpl.getUniformLocation("u_texRect").value();
    instanceRenderingULTextureRect = &instanceRenderingULTextureRectImpl;

    //
    //
    // Options
    Mode  mode            = Mode::OOP;
    bool  drawStep        = true;
    bool  drawUI          = true;
    float simulationSpeed = 0.1f;
    float rocketSpawnRate = 0.f;
    float zoom            = 3.f;
    float imguiMult       = 1.f;

    //
    //
    // Simulation state
    float              rocketSpawnTimer = 0.f;
    OOP::World         oopWorld;
    AOS::World         aosWorld;
    AOSImproved::World aosImprovedWorld;
    SOAManual::World   soaManualWorld;
    SOAMeta::World     soaMetaWorld;

    oopWorld.entities.reserve(1'250'000);

    aosWorld.rockets.reserve(2500);
    aosWorld.emitters.reserve(5000);
    aosWorld.particles.reserve(1'000'000);

    aosImprovedWorld.rockets.reserve(2500);
    aosImprovedWorld.smokeEmitters.reserve(2500);
    aosImprovedWorld.fireEmitters.reserve(2500);
    aosImprovedWorld.smokeParticles.reserve(750'000);
    aosImprovedWorld.fireParticles.reserve(750'000);

    soaManualWorld.rockets.reserve(2500);
    soaManualWorld.smokeEmitters.reserve(2500);
    soaManualWorld.fireEmitters.reserve(2500);
    soaManualWorld.smokeParticles.forEachVector([](auto& vec) { vec.reserve(750'000); });
    soaManualWorld.fireParticles.forEachVector([](auto& vec) { vec.reserve(750'000); });

    soaMetaWorld.rockets.reserve(2500);
    soaMetaWorld.smokeEmitters.reserve(2500);
    soaMetaWorld.fireEmitters.reserve(2500);
    soaMetaWorld.smokeParticles.reserve(750'000);
    soaMetaWorld.fireParticles.reserve(750'000);

    //
    //
    // Set up clock and time sampling
    za::Clock fpsClock;

    Sampler<float> samplesUpdateMs(/* capacity */ 32u);
    Sampler<float> samplesDrawMs(/* capacity */ 32u);
    Sampler<float> samplesDisplayMs(/* capacity */ 32u);
    Sampler<float> samplesFPS(/* capacity */ 32u);

    struct [[nodiscard]] SamplerScopeGuard
    {
        Sampler<float>& sampler;
        za::Time        startTime = za::Clock::now();

        ~SamplerScopeGuard()
        {
            sampler.record((za::Clock::now() - startTime).asSeconds() * 1000.f);
        }
    };

    //
    //
    // Helper functions
    const auto makeFullscreen = [&]
    {
        window.setResizable(false);
        window.setHasTitlebar(false);

        window.setSize(za::VideoModeUtils::getDesktopMode().size);
        window.setPosition({0, 0});
    };

    const auto makeWindowed = [&]
    {
        window.setResizable(true);
        window.setHasTitlebar(true);

        window.setSize(za::VideoModeUtils::getDesktopMode().size / 2u);
        window.setGlobalCenter(za::VideoModeUtils::getDesktopMode().size.toVec2f() / 2.f);
    };

    //
    //
    // Startup as windowed mode
    makeWindowed();

    //
    //
    // Simulation loop
    while (true)
    {
        fpsClock.restart();

        ////////////////////////////////////////////////////////////
        // Event handling
        ////////////////////////////////////////////////////////////
        {
            while (za::Optional event = window.pollEvent())
            {
                imGuiContext.processEvent(window, *event);

                if (za::EventUtils::isClosedOrEscapeKeyPressed(*event))
                    return 0;

                if (handleAspectRatioAwareResize(*event, resolution, windowView))
                    continue;

                if (auto* eKeyPressed = event->getIf<za::Event::KeyPressed>())
                {
                    if (eKeyPressed->code == za::Keyboard::Key::Num8)
                        imguiMult -= 0.25f;
                    else if (eKeyPressed->code == za::Keyboard::Key::Num9)
                        imguiMult += 0.25f;
                    else if (eKeyPressed->code == za::Keyboard::Key::F)
                        makeFullscreen();
                    else if (eKeyPressed->code == za::Keyboard::Key::W)
                        makeWindowed();
                }
            }
        }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"

        ////////////////////////////////////////////////////////////
        // Update step
        ////////////////////////////////////////////////////////////
        {
            SamplerScopeGuard guard{samplesUpdateMs};

            SFEX_PROFILE_SCOPE("update");

            rocketSpawnTimer += rocketSpawnRate * simulationSpeed;

            const auto updateNonOOPWorld = [&](auto& world)
            {
                for (; rocketSpawnTimer >= 1.f; rocketSpawnTimer -= 1.f)
                {
                    world.addRocket({
                        .position     = rng.getVec2f({-500.f, 0.f}, {-100.f, resolution.y}),
                        .velocity     = {},
                        .acceleration = {rng.getF(0.01f, 0.025f), 0.f},
                    });
                }

                world.update(simulationSpeed);

                {
                    SFEX_PROFILE_SCOPE("cleanup");
                    world.cleanup();
                }
            };

            if (mode == Mode::OOP)
            {
                for (; rocketSpawnTimer >= 1.f; rocketSpawnTimer -= 1.f)
                {
                    auto& rocket = oopWorld.addEntity<OOP::Rocket>();

                    rocket.position     = rng.getVec2f({-500.f, 0.f}, {-100.f, resolution.y});
                    rocket.velocity     = {};
                    rocket.acceleration = {rng.getF(0.01f, 0.025f), 0.f};

                    rocket.init();
                }

                oopWorld.update(simulationSpeed);

                {
                    SFEX_PROFILE_SCOPE("cleanup");
                    oopWorld.cleanup();
                }
            }
            else if (mode == Mode::AOS)
                updateNonOOPWorld(aosWorld);
            else if (mode == Mode::AOSImproved)
                updateNonOOPWorld(aosImprovedWorld);
            else if (mode == Mode::SOAManual)
                updateNonOOPWorld(soaManualWorld);
            else if (mode == Mode::SOAMeta)
                updateNonOOPWorld(soaMetaWorld);
        }
#pragma GCC diagnostic pop

        ////////////////////////////////////////////////////////////
        // ImGui step
        ////////////////////////////////////////////////////////////
        if (drawUI)
        {
            const auto clearSamples = [&]
            {
                samplesUpdateMs.clear();
                samplesDrawMs.clear();
                samplesDisplayMs.clear();
                samplesFPS.clear();

                sfex::resetNodes();
            };

            const auto setFontScale = [&](const float x) { ImGui::SetWindowFontScale(x * imguiMult); };

            imGuiContext.update(window, fpsClock.getElapsedTime());

            const auto plotGraphNoOverlay = [&](const char* label, const Sampler<float>& samples, float upperBound)
            {
                ImGui::PlotLines(label,
                                 samples.data(),
                                 static_cast<int>(samples.capacity()),
                                 static_cast<int>(samples.insertionIndex()),
                                 nullptr,
                                 0.f,
                                 upperBound,
                                 ImVec2{128.f * imguiMult, 32.f * imguiMult});
            };

            ImGui::SetNextWindowSize(ImVec2{440.f * imguiMult, 470.f * imguiMult});
            ImGui::SetNextWindowPos({window.getLocalRight() - 440.f * imguiMult - 24.f * imguiMult, 24.f * imguiMult});

            ImGui::PushFont(fontImGuiGeistMono);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.f * imguiMult); // Set corner radius

            ImGui::Begin("HUD", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
            setFontScale(1.25f);

            plotGraphNoOverlay("##infofps", samplesFPS, 144.f);
            ImGui::SameLine();
            ImGui::Text("    FPS: %.1f", samplesFPS.getAverageAs<double>());

            plotGraphNoOverlay("##infoupdate", samplesUpdateMs, 30.f);
            ImGui::SameLine();
            ImGui::Text(" Update: %.1f ms", samplesUpdateMs.getAverageAs<double>());

            plotGraphNoOverlay("##infodraw", samplesDrawMs, 30.f);
            ImGui::SameLine();
            ImGui::Text("   Draw: %.1f ms", samplesDrawMs.getAverageAs<double>());

            ImGui::Separator();
            setFontScale(1.f);

            const auto printDODEntityCount = [](const auto& world)
            {
                const auto getSize = [](const auto& container)
                {
                    if constexpr (requires { container.size(); })
                    {
                        return container.size();
                    }
                    else
                    {
                        return container.getSize();
                    }
                };

                ImGui::Text("Number of entities: %zu",
                            getSize(world.rockets) + getSize(world.smokeEmitters) + getSize(world.smokeParticles) +
                                getSize(world.fireEmitters) + getSize(world.fireParticles));
            };

            if (mode == Mode::OOP)
            {
                ImGui::Text("Number of entities: %zu", oopWorld.entities.size());
            }
            else if (mode == Mode::AOS)
            {
                ImGui::Text("Number of entities: %zu",
                            aosWorld.rockets.size() + aosWorld.emitters.size() + aosWorld.particles.size());
            }
            else if (mode == Mode::AOSImproved)
                printDODEntityCount(aosImprovedWorld);
            else if (mode == Mode::SOAManual)
                printDODEntityCount(soaManualWorld);
            else if (mode == Mode::SOAMeta)
                printDODEntityCount(soaMetaWorld);

            ImGui::Separator();
            setFontScale(0.75f);
            ImGui::Checkbox("Enable rendering", &drawStep);

            ImGui::Separator();
            setFontScale(0.75f);
            if (ImGui::Combo("##infoMode",
                             reinterpret_cast<int*>(&mode),
                             "OOP (Heap-allocated entities)\0AoS (Very branchy)\0AoS (Improved)\0SoA (Manual)\0SoA "
                             "(Meta)\0"))
            {
                clearSamples();
            }

            ImGui::Separator();
            setFontScale(0.75f);

            ImGui::Text("Rocket spawn rate: %.1fx", static_cast<double>(rocketSpawnRate));

            if (ImGui::Button("x1.0##r1"))
                rocketSpawnRate = 1.f;
            else if (ImGui::SameLine(), ImGui::Button("x2.0##r2"))
                rocketSpawnRate = 2.f;
            else if (ImGui::SameLine(), ImGui::Button("x3.0##r3"))
                rocketSpawnRate = 3.f;
            else if (ImGui::SameLine(), ImGui::Button("x4.0##r4"))
                rocketSpawnRate = 4.f;
            else if (ImGui::SameLine(), ImGui::Button("x5.0##r5"))
                rocketSpawnRate = 5.f;

            ImGui::Separator();
            setFontScale(0.75f);

            ImGui::Text("Simulation speed: %.1fx", static_cast<double>(simulationSpeed));

            if (ImGui::Button("x0.1##s1"))
                simulationSpeed = 0.1f;
            else if (ImGui::SameLine(), ImGui::Button("x0.5##s2"))
                simulationSpeed = 0.5f;
            else if (ImGui::SameLine(), ImGui::Button("x1.0##s3"))
                simulationSpeed = 1.f;
            else if (ImGui::SameLine(), ImGui::Button("x2.0##s4"))
                simulationSpeed = 2.f;
            else if (ImGui::SameLine(), ImGui::Button("x3.0##s5"))
                simulationSpeed = 3.f;
            else if (ImGui::SameLine(), ImGui::Button("x4.0##s6"))
                simulationSpeed = 4.f;
            else if (ImGui::SameLine(), ImGui::Button("x5.0##s7"))
                simulationSpeed = 5.f;

            ImGui::Separator();
            setFontScale(0.75f);

            ImGui::Text("Zoom level: %.1fx", static_cast<double>(zoom));
            ImGui::SliderFloat("##Zoom", &zoom, 1.f, 3.f);

            ImGui::Separator();

            ImGui::End();


            ImGui::Begin("SFEX Profiler");
            setFontScale(0.5f);
            sfex::showImguiProfiler();
            ImGui::End();

            ImGui::PopStyleVar();
            ImGui::PopFont();
        }
        ////////////////////////////////////////////////////////////
        // Draw step
        ////////////////////////////////////////////////////////////
        {
            SamplerScopeGuard guard{samplesDrawMs};

            SFEX_PROFILE_SCOPE("draw");

            worldView.size   = resolution / zoom;
            worldView.center = {(resolution.x / zoom) / 2.f, resolution.y / 2.f};

            rtGame.clear();

            if (drawStep)
            {
                if (mode == Mode::OOP)
                    oopWorld.draw(rtGame, worldView);
                else if (mode == Mode::AOS)
                    aosWorld.draw(rtGame, worldView);
                else if (mode == Mode::AOSImproved)
                    aosImprovedWorld.draw(rtGame, worldView);
                else if (mode == Mode::SOAManual)
                    soaManualWorld.draw(rtGame, worldView);
                else if (mode == Mode::SOAMeta)
                    soaMetaWorld.draw(rtGame, worldView);
            }

            rtGame.display();
        }
        {
            SamplerScopeGuard guard{samplesDisplayMs};

            window.clear();

            window.draw(rtGame.getTexture(), {.view = windowView});

            if (drawUI)
                imGuiContext.render(window);

            window.display();
        }
        samplesFPS.record(1.f / fpsClock.getElapsedTime().asSeconds());
    }
}

#ifdef __clang__
    #pragma clang diagnostic pop
#endif
