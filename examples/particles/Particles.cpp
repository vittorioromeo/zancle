#include "../bubble_idle/SoA.hpp" // TODO P1: avoid the relative path...?

#include "ExampleUtils/Sampler.hpp"

#include "Zancle/Graphics/DrawableBatch.hpp"
#include "Zancle/Graphics/Font.hpp"
#include "Zancle/Graphics/GraphicsContext.hpp"
#include "Zancle/Graphics/Image.hpp"
#include "Zancle/Graphics/RenderStates.hpp"
#include "Zancle/Graphics/RenderTarget.hpp"
#include "Zancle/Graphics/RenderTexture.hpp"
#include "Zancle/Graphics/RenderWindow.hpp"
#include "Zancle/Graphics/Sprite.hpp"
#include "Zancle/Graphics/Text.hpp"
#include "Zancle/Graphics/Texture.hpp"
#include "Zancle/Graphics/TextureAtlas.hpp"

#include "Zancle/ImGui/ImGuiContext.hpp"
#include "Zancle/ImGui/IncludeImGui.hpp"

#include "Zancle/Window/Event.hpp" // IWYU pragma: keep
#include "Zancle/Window/EventUtils.hpp"

#include "Zancle/IO/Path.hpp"

#include "Zancle/Concurrency/ParallelFor.hpp"
#include "Zancle/Concurrency/ThreadPool.hpp"

#include "Zancle/String/ToString.hpp"

#include "Zancle/Algorithm/Erase.hpp"
#include "Zancle/Algorithm/SwapAndPop.hpp"

#include "Zancle/Chrono/Clock.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Geometry/Angle.hpp"
#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Random/FastNonCryptoRng.hpp"

#include "Zancle/Vocabulary/Optional.hpp"
#include "Zancle/Vocabulary/UniquePtr.hpp"

#include "Zancle/Math/Clamp.hpp"
#include "Zancle/Math/Constants.hpp"
#include "Zancle/Math/MinMax.hpp"

#include "Zancle/Base/GetArraySize.hpp"
#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/InterferenceSize.hpp"
#include "Zancle/Base/SizeT.hpp"


namespace
{
////////////////////////////////////////////////////////////
[[gnu::always_inline]] inline void updateParticle(
    za::Vec2f&      position,
    za::Vec2f&      velocity,
    const za::Vec2f acceleration,

    float&      scale,
    const float scaleGrowth,

    float&      opacity,
    const float opacityGrowth,

    float&      rotation,
    const float torque)
{
    velocity += acceleration;
    position += velocity;
    scale += scaleGrowth;
    opacity += opacityGrowth;
    rotation += torque;
}

////////////////////////////////////////////////////////////
[[gnu::always_inline]] inline void drawParticleImpl(
    const za::Texture& texture,
    const za::Rect2f&  txr,
    const za::Vec2f    origin,
    const za::Vec2f    position,
    const float        scale,
    const float        rotation,
    auto&&             drawFn)
{
    drawFn(
        za::Sprite{
            .position    = position,
            .scale       = {scale, scale},
            .origin      = origin,
            .rotation    = za::radians(rotation),
            .textureRect = txr,
        },
        za::RenderStates{.texture = &texture});
}

} // namespace


////////////////////////////////////////////////////////////
/// Main
///
////////////////////////////////////////////////////////////
int main()
{
    //
    //
    // Set up random generator
    za::FastNonCryptoRng rng;

    //
    //
    // Set up graphics context
    auto graphicsContext = za::GraphicsContext::create().value();

    //
    //
    // Set up window
    constexpr za::Vec2f windowSize{1680.f, 1050.f};

    auto window = za::RenderWindow::create(
                      {
                          .size      = windowSize.toVec2u(),
                          .title     = "Zancle: particles example",
                          .resizable = false,
                          .vsync     = false,
                      })
                      .value();

    //
    //
    // Set up imgui
    za::ImGuiContext imGuiContext;

    //
    //
    // Set up texture atlas
    za::TextureAtlas textureAtlas{za::Texture::create({512u, 512u}, {.smooth = true}).value()};

    const auto addImgResourceToAtlas = [&](const za::Path& path)
    { return textureAtlas.add(za::Image::loadFromFile("resources" / path).value()).value(); };

    //
    //
    // Load fonts
    const auto fontTuffy        = za::Font::openFromFile("resources/tuffy.ttf", &textureAtlas).value();
    const auto fontMouldyCheese = za::Font::openFromFile("resources/mouldycheese.ttf", &textureAtlas).value();

    //
    //
    // Load images and add to texture atlas
    const za::Rect2f spriteTextureRects[]{
        addImgResourceToAtlas("pCircle.png"),
        addImgResourceToAtlas("pExplosion.png"),
        addImgResourceToAtlas("pFire.png"),
        addImgResourceToAtlas("pSmoke.png"),
        addImgResourceToAtlas("pStar.png"),
        addImgResourceToAtlas("pTrail.png"),
    };

    //
    //
    // OOP Particles
    struct Entity
    {
        bool alive = true;

        virtual ~Entity() = default;

        virtual void update()
        {
        }

        virtual void draw(const za::Texture&, za::RenderTarget&)
        {
        }
    };

    struct ParticleOOP : Entity
    {
        za::Vec2f position;
        za::Vec2f velocity;
        za::Vec2f acceleration;

        float scale;
        float scaleGrowth;

        float opacity;
        float opacityGrowth;

        float rotation;
        float torque;

        ParticleOOP(za::Vec2f thePosition,
                    za::Vec2f theVelocity,
                    za::Vec2f theAcceleration,
                    float     theScale,
                    float     theScaleGrowth,
                    float     theOpacity,
                    float     theOpacityGrowth,
                    float     theRotation,
                    float     theTorque) :
            position(thePosition),
            velocity(theVelocity),
            acceleration(theAcceleration),
            scale(theScale),
            scaleGrowth(theScaleGrowth),
            opacity(theOpacity),
            opacityGrowth(theOpacityGrowth),
            rotation(theRotation),
            torque(theTorque)
        {
        }

        void update() override
        {
            updateParticle(position, velocity, acceleration, scale, scaleGrowth, opacity, opacityGrowth, rotation, torque);
            alive = opacity > 0.f;
        }

        void draw(const za::Texture& texture, za::RenderTarget& rt) override
        {
            drawParticleImpl(texture,
                             {{0.f, 0.f}, {64.f, 64.f}},
                             {32.f, 32.f},
                             position,
                             scale,
                             rotation,
                             [&](const za::Sprite& sprite, const za::RenderStates& states) { rt.draw(sprite, states); });
        }
    };

    za::Vector<za::UniquePtr<Entity>> entities;

    //
    //
    // SoA Particles
    struct ParticleAoS // NOLINT(cppcoreguidelines-pro-type-member-init)
    {
        za::Vec2f position;
        za::Vec2f velocity;
        za::Vec2f acceleration;

        float scale;
        float scaleGrowth;

        float opacity;
        float opacityGrowth;

        float rotation;
        float torque;
    };

    za::Vector<ParticleAoS> particlesAoS;

    //
    //
    // AoS Particles
    using ParticleSoA = SoAFor<za::Vec2f, // position
                               za::Vec2f, // velocity
                               za::Vec2f, // acceleration

                               float, // scale
                               float, // scaleGrowth

                               float, // opacity
                               float, // opacityGrowth

                               float,  // rotation
                               float>; // torque

    ParticleSoA particlesSoA;

    //
    //
    // Get hardware constants
    const auto     nMaxWorkers   = static_cast<za::U64>(za::ThreadPool::getOptimalThreadCount());
    constexpr auto cacheLineSize = static_cast<za::SizeT>(za::hardwareDestructiveInterferenceSize);

    //
    //
    // Set up UI elements
    enum class BatchType : int
    {
        Disabled   = 0,
        CPUStorage = 1,
        GPUStorage = 2
    };

    const auto defaultBatchType =
#ifdef ZA_OPENGL_ES
        BatchType::CPUStorage
#else
        BatchType::GPUStorage
#endif
        ;

    auto    batchType                = BatchType::Disabled;
    auto    autobatchType            = defaultBatchType;
    za::U64 autoBatchVertexThreshold = 32'768u;
    bool    multithreadedUpdate      = false;
    bool    multithreadedDraw        = false;
    bool    useOOP                   = false;
    bool    useSoA                   = false;
    bool    unifiedSoAProcessing     = false;
    bool    destroyParticles         = true;
    bool    destroyBySwapping        = true;
    za::U64 nWorkers                 = nMaxWorkers;
    int     numEntities              = 50'000;
    bool    drawStep                 = true;

    //
    //
    // Set up drawable batches
    struct alignas(cacheLineSize) AlignedCPUDrawableBatch : za::CPUDrawableBatch
    {
        using za::CPUDrawableBatch::CPUDrawableBatch;
    };

    struct alignas(cacheLineSize) AlignedGPUDrawableBatch : za::PersistentGPUDrawableBatch
    {
        using za::PersistentGPUDrawableBatch::PersistentGPUDrawableBatch;
    };

    za::Vector<AlignedCPUDrawableBatch> cpuDrawableBatches(static_cast<za::SizeT>(nMaxWorkers));
    za::Vector<AlignedGPUDrawableBatch> gpuDrawableBatches(static_cast<za::SizeT>(nMaxWorkers));

    //
    //
    // Set up thread pool
    // The calling thread takes part in `parallelFor`, so `nMaxWorkers` batches can run at once
    za::ThreadPool       pool(za::ThreadPool::getOptimalWorkerCount());
    za::ParallelForSlots parallelForSlots;

    const auto doInBatches = [&](const za::SizeT nParticlesTotal, auto&& f)
    {
        const auto      nBatches          = static_cast<za::SizeT>(nWorkers);
        const za::SizeT particlesPerBatch = nParticlesTotal / nBatches;

        za::parallelFor(pool,
                        parallelForSlots,
                        nBatches,
                        [&](za::SizeT i, const za::SizeT end)
        {
            for (; i < end; ++i)
            {
                const za::SizeT batchStartIdx = i * particlesPerBatch;
                const za::SizeT batchEndIdx   = (i == nBatches - 1u) ? nParticlesTotal : (i + 1u) * particlesPerBatch;

                f(i, batchStartIdx, batchEndIdx);
            }
        },
                        /* chunkSize */ 1u);
    };

    //
    //
    // Set up clock and time sampling
    za::Clock clock;
    za::Clock fpsClock;

    Sampler<float> samplesUpdateMs(/* capacity */ 64u);
    Sampler<float> samplesDrawMs(/* capacity */ 64u);
    Sampler<float> samplesDisplayMs(/* capacity */ 64u);
    Sampler<float> samplesFPS(/* capacity */ 64u);

    //
    //
    // Population functions
    const auto pushParticle = [&] [[gnu::always_inline, gnu::flatten]] (auto&& pushFn)
    {
        pushFn(rng.getVec2f({0.f, 0.f}, windowSize),       // position
               rng.getVec2f({-0.5f, -0.5f}, {0.5f, 0.5f}), // velocity
               rng.getVec2f({-0.1f, -0.1f}, {0.1f, 0.1f}), // acceleration
               rng.getF(0.001f, 0.015f),                   // scale
               rng.getF(-0.002f, 0.002f),                  // scaleGrowth
               rng.getF(0.75f, 1.f),                       // opacity
               rng.getF(-0.0015f, -0.0005f),               // opacityGrowth
               rng.getF(0.f, za::tau),                     // rotation
               rng.getF(-0.005f, 0.005f)                   // torque
        );
    };

    const auto populateParticlesOOP = [&](const za::SizeT n)
    {
        if (n < entities.size())
        {
            entities.resize(n);
            return;
        }

        entities.reserve(n);

        for (za::SizeT i = entities.size(); i < n; ++i)
            pushParticle([&] [[gnu::always_inline, gnu::flatten]] (auto&&... args)
            { entities.emplaceBack(za::makeUnique<ParticleOOP>(args...)); });
    };

    const auto populateParticlesAoS = [&](const za::SizeT n)
    {
        if (n < particlesAoS.size())
        {
            particlesAoS.resize(n);
            return;
        }

        particlesAoS.reserve(n);

        for (za::SizeT i = particlesAoS.size(); i < n; ++i)
            pushParticle([&] [[gnu::always_inline, gnu::flatten]] (auto&&... args) { particlesAoS.emplaceBack(args...); });
    };

    const auto populateParticlesSoA = [&](const za::SizeT n)
    {
        if (n < particlesSoA.getSize())
        {
            particlesSoA.resize(n);
            return;
        }

        particlesSoA.reserve(n);

        for (za::SizeT i = particlesSoA.getSize(); i < n; ++i)
            pushParticle([&] [[gnu::always_inline, gnu::flatten]] (auto&&... args) { particlesSoA.pushBack(args...); });
    };

    const auto populateParticles = [&](const za::SizeT n)
    {
        if (useOOP)
            populateParticlesOOP(n);
        else if (useSoA)
            populateParticlesSoA(n);
        else
            populateParticlesAoS(n);
    };

    populateParticles(static_cast<za::SizeT>(numEntities));

    //
    //
    // Simulation loop
    while (true)
    {
        fpsClock.restart();

        ////////////////////////////////////////////////////////////
        // Event handling
        ////////////////////////////////////////////////////////////
        // ---
        clock.restart();
        {
            while (za::Optional event = window.pollEvent())
            {
                imGuiContext.processEvent(window, *event);

                if (za::EventUtils::isClosedOrEscapeKeyPressed(*event))
                    return 0;
            }
        }
        // ---

        ////////////////////////////////////////////////////////////
        // Update step
        ////////////////////////////////////////////////////////////
        // ---
        clock.restart();
        {
            // This is the bottleneck, consider reusing the particle instead of shifting/swapping
            if (destroyParticles)
            {
                const auto destroyPredicate = [] [[gnu::always_inline, gnu::flatten]] (const float opacity)
                { return opacity <= 0.f; };

                if (useOOP)
                {
                    za::vectorSwapAndPopIf(entities, [](const auto& e) { return !e->alive; });
                }
                else if (useSoA)
                {
                    if (destroyBySwapping)
                        particlesSoA.eraseIfBySwapping<5 /* opacity */>(destroyPredicate);
                    else
                        particlesSoA.eraseIfByShifting<5 /* opacity */>(destroyPredicate);
                }
                else
                {
                    if (destroyBySwapping)
                    {
                        za::SizeT n = particlesAoS.size();
                        za::SizeT i = 0;

                        while (i < n)
                        {
                            if (particlesAoS[i].opacity > 0.f)
                            {
                                ++i;
                                continue;
                            }

                            particlesAoS[i] = particlesAoS[--n];
                        }

                        particlesAoS.resize(n);
                    }
                    else
                    {
                        za::vectorEraseIf(particlesAoS, [] [[gnu::always_inline, gnu::flatten]] (const ParticleAoS& p) {
                            return p.opacity <= 0.f;
                        });
                    }
                }
            }

            populateParticles(static_cast<za::SizeT>(numEntities));

            if (!multithreadedUpdate)
            {
                if (useOOP)
                {
                    for (auto& e : entities)
                        e->update();
                }
                else if (useSoA)
                {
                    if (unifiedSoAProcessing)
                    {
                        particlesSoA.withAll(updateParticle);
                    }
                    else
                    {
                        particlesSoA.with<1, 2>(
                            [] [[gnu::always_inline, gnu::flatten]] (za::Vec2f & velocity, const za::Vec2f acc)
                        { velocity += acc; });

                        particlesSoA.with<0, 1>(
                            [] [[gnu::always_inline, gnu::flatten]] (za::Vec2f & position, za::Vec2f & velocity)
                        { position += velocity; });

                        particlesSoA.with<3, 4>(
                            [] [[gnu::always_inline, gnu::flatten]] (float& scale, const float scaleGrowth)
                        { scale += scaleGrowth; });

                        particlesSoA.with<5, 6>(
                            [] [[gnu::always_inline, gnu::flatten]] (float& opacity, const float opacityGrowth)
                        { opacity += opacityGrowth; });

                        particlesSoA.with<7, 8>([] [[gnu::always_inline, gnu::flatten]] (float& rotation, const float torque)
                        { rotation += torque; });
                    }
                }
                else
                {
                    for (ParticleAoS& p : particlesAoS)
                        updateParticle(p.position,
                                       p.velocity,
                                       p.acceleration,
                                       p.scale,
                                       p.scaleGrowth,
                                       p.opacity,
                                       p.opacityGrowth,
                                       p.rotation,
                                       p.torque);
                }
            }
            else
            {
                if (useOOP)
                {
                    // TODO
                }
                else if (useSoA)
                {
                    if (unifiedSoAProcessing)
                    {
                        doInBatches(static_cast<za::SizeT>(numEntities),
                                    [&] [[gnu::always_inline, gnu::flatten]] (const za::SizeT /* iBatch */,
                                                                              const za::SizeT batchStartIdx,
                                                                              const za::SizeT batchEndIdx)
                        { particlesSoA.withAllSubRange(batchStartIdx, batchEndIdx, updateParticle); });
                    }
                    else
                    {
                        doInBatches(static_cast<za::SizeT>(numEntities),
                                    [&] [[gnu::always_inline, gnu::flatten]] (const za::SizeT /* iBatch */,
                                                                              const za::SizeT batchStartIdx,
                                                                              const za::SizeT batchEndIdx)
                        {
                            particlesSoA.withSubRange<1, 2>(batchStartIdx,
                                                            batchEndIdx,
                                                            [] [[gnu::always_inline,
                                                                 gnu::flatten]] (za::Vec2f & velocity, const za::Vec2f acc)

                            { velocity += acc; });

                            particlesSoA.withSubRange<0, 1>(batchStartIdx,
                                                            batchEndIdx,
                                                            [] [[gnu::always_inline,
                                                                 gnu::flatten]] (za::Vec2f & position, za::Vec2f & velocity)

                            { position += velocity; });

                            particlesSoA.withSubRange<3, 4>(batchStartIdx,
                                                            batchEndIdx,
                                                            [] [[gnu::always_inline,
                                                                 gnu::flatten]] (float& scale, const float scaleGrowth)

                            { scale += scaleGrowth; });

                            particlesSoA.withSubRange<5, 6>(batchStartIdx,
                                                            batchEndIdx,
                                                            [] [[gnu::always_inline,
                                                                 gnu::flatten]] (float& opacity, const float opacityGrowth)

                            { opacity += opacityGrowth; });

                            particlesSoA.withSubRange<7, 8>(batchStartIdx,
                                                            batchEndIdx,
                                                            [] [[gnu::always_inline,
                                                                 gnu::flatten]] (float& rotation, const float torque)

                            { rotation += torque; });
                        });
                    }
                }
                else
                {
                    doInBatches(static_cast<za::SizeT>(numEntities),
                                [&] [[gnu::always_inline, gnu::flatten]] (const za::SizeT /* iBatch */,
                                                                          const za::SizeT batchStartIdx,
                                                                          const za::SizeT batchEndIdx)
                    {
                        for (za::SizeT i = batchStartIdx; i < batchEndIdx; ++i)
                        {
                            ParticleAoS& p = particlesAoS[i];

                            updateParticle(p.position,
                                           p.velocity,
                                           p.acceleration,
                                           p.scale,
                                           p.scaleGrowth,
                                           p.opacity,
                                           p.opacityGrowth,
                                           p.rotation,
                                           p.torque);
                        }
                    });
                }
            }
        }
        samplesUpdateMs.record(clock.getElapsedTime().asSeconds() * 1000.f);
        // ---

        ////////////////////////////////////////////////////////////
        // ImGui step
        ////////////////////////////////////////////////////////////
        // ---
        clock.restart();
        {
            imGuiContext.update(window, fpsClock.getElapsedTime());

            ImGui::Begin("Zancle: particles example", nullptr, ImGuiWindowFlags_NoResize);
            ImGui::SetWindowSize({380.f, 510.f});

            const auto clearSamples = [&]
            {
                samplesUpdateMs.clear();
                samplesDrawMs.clear();
                samplesDisplayMs.clear();
                samplesFPS.clear();
            };

            constexpr const char* batchTypeItems[]{"Disabled",
                                                   "CPU Storage",
#ifndef ZA_OPENGL_ES
                                                   "GPU Storage"
#endif
            };

            if (ImGui::Combo("Batch type", reinterpret_cast<int*>(&batchType), batchTypeItems, za::getArraySize(batchTypeItems)))
                clearSamples();

            ImGui::BeginDisabled(batchType != BatchType::Disabled);
            if (ImGui::Combo("Autobatch type",
                             reinterpret_cast<int*>(&autobatchType),
                             batchTypeItems,
                             za::getArraySize(batchTypeItems)))
            {
                clearSamples();
                window.setAutoBatchMode(static_cast<za::RenderTarget::AutoBatchMode>(autobatchType));
            }

            {
                const za::U64 step = 1u;
                ImGui::SetNextItemWidth(172.f);
                if (ImGui::InputScalar("Autobatch Vertex Threshold", ImGuiDataType_U64, &autoBatchVertexThreshold, &step))
                    window.setAutoBatchVertexThreshold(
                        static_cast<za::SizeT>(za::max(autoBatchVertexThreshold, za::U64{1024u})));
            }
            ImGui::EndDisabled();

            ImGui::Checkbox("Multithreaded Update", &multithreadedUpdate);

            ImGui::BeginDisabled(batchType == BatchType::Disabled);
            ImGui::Checkbox("Multithreaded Draw", &multithreadedDraw);
            ImGui::EndDisabled();

            if (ImGui::Checkbox("Use OOP", &useOOP))
                populateParticles(static_cast<za::SizeT>(numEntities));


            if (ImGui::Checkbox("Use SoA", &useSoA))
                populateParticles(static_cast<za::SizeT>(numEntities));

            ImGui::Checkbox("Unified SoA processing", &unifiedSoAProcessing);
            ImGui::Checkbox("Destroy/recreate particles", &destroyParticles);
            ImGui::Checkbox("Destroy via swapping", &destroyBySwapping);
            ImGui::Checkbox("Draw step", &drawStep);

            {
                const za::U64 step = 1u;
                ImGui::InputScalar("Workers", ImGuiDataType_U64, &nWorkers, &step);
            }
            nWorkers = za::clamp(nWorkers, za::U64{2u}, nMaxWorkers);

            ImGui::NewLine();

            ImGui::Text("Number of entities:");

            if (ImGui::InputInt("##InputInt", &numEntities))
                populateParticles(static_cast<za::SizeT>(numEntities));

            ImGui::NewLine();

            const auto plotGraph = [&](const char* label, const char* unit, const Sampler<float>& samples, float upperBound)
            {
                ImGui::PlotLines(label,
                                 samples.data(),
                                 static_cast<int>(samples.capacity()),
                                 static_cast<int>(samples.insertionIndex()),
                                 (za::toString(samples.getAverageAs<double>()) + unit).cStr(),
                                 0.f,
                                 upperBound,
                                 ImVec2{256.f, 32.f});
            };

            plotGraph("Update", " ms", samplesUpdateMs, 10.f);
            plotGraph("Draw", " ms", samplesDrawMs, 100.f);
            plotGraph("FPS", " FPS", samplesFPS, 300.f);
            plotGraph("Display", " ms", samplesDisplayMs, 300.f);

            ImGui::End();
        }
        // ---

        ////////////////////////////////////////////////////////////
        // Draw step
        ////////////////////////////////////////////////////////////
        // ---
        clock.restart();
        if (drawStep)
        {
            window.clear();

            const za::Rect2f& textureRect = spriteTextureRects[0];
            const auto        origin      = textureRect.size / 2.f;

            const auto nParticles = static_cast<za::SizeT>(numEntities);

            const auto makeParticleSprite =
                [&] [[gnu::always_inline, gnu::flatten]] (const za::Vec2f position, const float scale, const float rotation)

            {
                return za::Sprite{
                    .position    = position,
                    .scale       = {scale, scale},
                    .origin      = origin,
                    .rotation    = za::radians(rotation),
                    .textureRect = textureRect,
                };
            };

            // Iterates over non-OOP particles, hoisting the SoA/AoS branch outside the inner loop
            const auto forEachNonOOPParticle =
                [&] [[gnu::always_inline, gnu::flatten]] (const za::SizeT startIdx, const za::SizeT endIdx, auto&& fn)

            {
                if (useSoA)
                {
                    for (za::SizeT i = startIdx; i < endIdx; ++i)
                        particlesSoA.withNth<0, 3, 7>(i,
                                                      [&] [[gnu::always_inline, gnu::flatten]] (const auto& position,
                                                                                                const auto& scale,
                                                                                                const auto& rotation)

                        { fn(makeParticleSprite(position, scale, rotation)); });
                }
                else
                {
                    for (za::SizeT i = startIdx; i < endIdx; ++i)
                    {
                        const ParticleAoS& p = particlesAoS[i];
                        fn(makeParticleSprite(p.position, p.scale, p.rotation));
                    }
                }
            };

            if (useOOP)
            {
                // OOP particles always draw directly to window (no batching/locked states)
                for (za::SizeT i = 0u; i < nParticles; ++i)
                    entities[i]->draw(textureAtlas.getTexture(), window);
            }
            else if (batchType == BatchType::Disabled)
            {
                // Use withLockedRenderStates to skip per-sprite RenderStates comparison
                auto drawCtx = window.withLockedRenderStates({.texture = &textureAtlas.getTexture()});

                forEachNonOOPParticle(0u, nParticles, [&] [[gnu::always_inline, gnu::flatten]] (const za::Sprite& sprite) {
                    drawCtx.draw(sprite);
                });
            }
            else if (!multithreadedDraw)
            {
                const auto doBatchedDraw = [&](auto& batch)
                {
                    batch.clear();

                    forEachNonOOPParticle(0u, nParticles, [&] [[gnu::always_inline, gnu::flatten]] (const za::Sprite& sprite) {
                        batch.add(sprite);
                    });

                    window.draw(batch, {.texture = &textureAtlas.getTexture()});
                };

                if (batchType == BatchType::CPUStorage)
                    doBatchedDraw(cpuDrawableBatches[0]);
                else if (batchType == BatchType::GPUStorage)
                    doBatchedDraw(gpuDrawableBatches[0]);
            }
            else
            {
                const auto doMultithreadedDraw = [&](auto& batchesArray)
                {
                    for (auto& batch : batchesArray)
                        batch.clear();

                    doInBatches(nParticles,
                                [&](const za::SizeT iBatch, const za::SizeT batchStartIdx, const za::SizeT batchEndIdx)
                    {
                        forEachNonOOPParticle(batchStartIdx,
                                              batchEndIdx,
                                              [&] [[gnu::always_inline, gnu::flatten]] (const za::Sprite& sprite)
                        { batchesArray[iBatch].add(sprite); });
                    });

                    for (auto& batch : batchesArray)
                        window.draw(batch, {.texture = &textureAtlas.getTexture()});
                };

                if (batchType == BatchType::CPUStorage)
                {
                    doMultithreadedDraw(cpuDrawableBatches);
                }
                else if (batchType == BatchType::GPUStorage)
                {
                    // Must reserve in advance as reserving is not thread-safe
                    for (za::SizeT iBatch = 0u; iBatch < nMaxWorkers; ++iBatch)
                        gpuDrawableBatches[iBatch].reserveQuads(nParticles / nWorkers * 2u);

                    doMultithreadedDraw(gpuDrawableBatches);
                }
            }
        }
        else
        {
            window.clear();
        }
        samplesDrawMs.record(clock.getElapsedTime().asSeconds() * 1000.f);
        // ---

        // ---
        clock.restart();
        {
            imGuiContext.render(window);
            window.display();
        }
        samplesDisplayMs.record(clock.getElapsedTime().asSeconds() * 1000.f);
        // ---

        samplesFPS.record(1.f / fpsClock.getElapsedTime().asSeconds());
    }
}
