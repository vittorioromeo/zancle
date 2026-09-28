#include "ExampleUtils/RNGFast.hpp"
#include "ExampleUtils/Sampler.hpp"

#include "Zancle/Graphics/CircleShape.hpp"
#include "Zancle/Graphics/Color.hpp"
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

#include "Zancle/Concurrency/ThreadPool.hpp"

#include "Zancle/String/ToString.hpp"

#include "Zancle/Chrono/Clock.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Geometry/Angle.hpp"
#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Math/Clamp.hpp"
#include "Zancle/Math/Constants.hpp"
#include "Zancle/Math/MinMax.hpp"

#include "Zancle/Base/GetArraySize.hpp"
#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/InterferenceSize.hpp"
#include "Zancle/Base/PtrDiffT.hpp"
#include "Zancle/Base/SizeT.hpp"

#include <cstdio>


////////////////////////////////////////////////////////////
/// Main
///
////////////////////////////////////////////////////////////
int main()
{
    //
    //
    // Set up random generator
    RNGFast rng(/* seed */ 1234);

    const auto getRndFloat = [&](const float min, const float max) { return rng.getF(min, max); };

    const auto getRndUInt = [&](const unsigned int min, const unsigned int max)
    { return rng.getI<unsigned int>(min, max); };

    const auto getRndU8 = [&](const za::U8 min, const za::U8 max) { return rng.getI<za::U8>(min, max); };

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
                          .title     = "Zancle: batching example",
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
    za::TextureAtlas textureAtlas{za::Texture::create({1024u, 1024u}, {.smooth = true}).value()};

    const auto addImgResourceToAtlas = [&](const za::Path& path)
    { return textureAtlas.add(za::Image::loadFromFile("resources" / path).value()).value(); };

    //
    //
    // Add white dot to atlas
    const auto whiteDotAtlasRect = textureAtlas.add(za::GraphicsContext::getBuiltInWhiteDotTexture()).value();

    //
    //
    // Load fonts
    const auto fontTuffy        = za::Font::openFromFile("resources/tuffy.ttf", &textureAtlas).value();
    const auto fontMouldyCheese = za::Font::openFromFile("resources/mouldycheese.ttf", &textureAtlas).value();

    //
    //
    // Load images and add to texture atlas
    const za::Rect2f spriteTextureRects[]{
        addImgResourceToAtlas("elephant.png"),
        addImgResourceToAtlas("giraffe.png"),
        addImgResourceToAtlas("monkey.png"),
        addImgResourceToAtlas("pig.png"),
        addImgResourceToAtlas("rabbit.png"),
        addImgResourceToAtlas("snake.png"),
    };

    //
    //
    // Simulation stuff
    struct Entity
    {
        za::Text        text;
        za::CircleShape circleShape;
        za::Sprite      sprite;
        za::Vec2f       velocity;
        float           torque;
    };

    za::Vector<Entity> entities;

    const auto populateEntities = [&](const za::SizeT n)
    {
        if (n < entities.size())
        {
            entities.erase(entities.begin() + static_cast<za::PtrDiffT>(n), entities.end());
            return;
        }

        entities.reserve(n);

        char                  labelBuffer[64]{};
        constexpr const char* names[]{"Elephant", "Giraffe", "Monkey", "Pig", "Rabbit", "Snake"};

        for (za::SizeT i = entities.size(); i < n; ++i)
        {
            const za::SizeT   type        = i % 6u;
            const za::Rect2f& textureRect = spriteTextureRects[type];

            std::snprintf(labelBuffer, 64, "%s #%zu", names[type], (i / (type + 1u)) + 1u);

            auto& [text, circleShape, sprite, velocity, torque] = entities.emplaceBack(
                za::Text{i % 2u == 0u ? fontTuffy : fontMouldyCheese,
                         {.string           = labelBuffer,
                          .characterSize    = 30u,
                          .fillColor        = za::Color::Black,
                          .outlineColor     = za::Color::White,
                          .outlineThickness = 5.f}},
                za::CircleShape{
                    {.textureRect        = {.position = whiteDotAtlasRect.position, .size{0.f, 0.f}},
                     .outlineTextureRect = {.position = whiteDotAtlasRect.position, .size{0.f, 0.f}},
                     .fillColor    = {getRndU8(0u, 255u), getRndU8(0u, 255u), getRndU8(0u, 255u), getRndU8(125u, 255u)},
                     .outlineColor = {getRndU8(0u, 255u), getRndU8(0u, 255u), getRndU8(0u, 255u), getRndU8(125u, 255u)},
                     .outlineThickness = 3.f,
                     .radius           = getRndFloat(3.f, 8.f),
                     .pointCount       = getRndUInt(3u, 8u)}},
                za::Sprite{.textureRect = textureRect},
                za::Vec2f{getRndFloat(-2.5f, 2.5f), getRndFloat(-2.5f, 2.5f)},
                getRndFloat(-0.05f, 0.05f));

            sprite.origin   = textureRect.size / 2.f;
            sprite.rotation = za::radians(getRndFloat(0.f, za::tau));

            const float scaleFactor = getRndFloat(0.08f, 0.17f);
            sprite.scale            = {scaleFactor, scaleFactor};
            text.scale              = sprite.scale * 3.5f;

            sprite.position = {getRndFloat(0.f, windowSize.x), getRndFloat(0.f, windowSize.y)};

            text.origin        = text.getLocalBounds().getCenter();
            circleShape.origin = circleShape.getLocalBounds().getCenter();
        }
    };

    //
    //
    // Get hardware constants
    const auto     nMaxWorkers   = za::ThreadPool::getHardwareWorkerCount();
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

    auto         batchType                = BatchType::Disabled;
    auto         autobatchType            = defaultBatchType;
    za::U64      autoBatchVertexThreshold = 32'768u;
    bool         drawSprites              = true;
    bool         drawText                 = true;
    bool         drawShapes               = true;
    bool         multithreadedUpdate      = false;
    bool         multithreadedDraw        = false;
    bool         useWithRenderStatesAPI   = true;
    auto         nWorkers                 = static_cast<za::U64>(nMaxWorkers);
    int          numEntities              = 500;
    za::SizeT    drawnVertices            = 0u;
    unsigned int nDrawCalls               = 0u;

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

    za::Vector<AlignedCPUDrawableBatch> cpuDrawableBatches(nMaxWorkers);

#ifndef ZA_OPENGL_ES
    za::Vector<AlignedGPUDrawableBatch> gpuDrawableBatches(nMaxWorkers);
#endif

    //
    //
    // Set up thread pool
    // The calling thread takes part in `parallelFor`, so `nMaxWorkers` batches can run at once
    za::ThreadPool pool(za::ThreadPool::getHardwareWorkerCountExcludingCallingThread());

    const auto doInBatches = [&](auto&& f)
    {
        const auto      nBatches         = static_cast<za::SizeT>(nWorkers);
        const za::SizeT entitiesPerBatch = entities.size() / nBatches;

        pool.parallelFor(nBatches,
                         [&](za::SizeT i, const za::SizeT end)
        {
            for (; i < end; ++i)
            {
                const za::SizeT batchStartIdx = i * entitiesPerBatch;
                const za::SizeT batchEndIdx   = (i == nBatches - 1u) ? entities.size() : (i + 1u) * entitiesPerBatch;

                f(i, batchStartIdx, batchEndIdx);
            }
        },
                         /* chunkSize */ 1u);
    };

    // Set up clock and time sampling
    za::Clock clock;
    za::Clock fpsClock;

    Sampler<float> samplesEventMs(/* capacity */ 64u);
    Sampler<float> samplesUpdateMs(/* capacity */ 64u);
    Sampler<float> samplesImGuiMs(/* capacity */ 64u);
    Sampler<float> samplesDrawMs(/* capacity */ 64u);
    Sampler<float> samplesDisplayMs(/* capacity */ 64u);
    Sampler<float> samplesFPS(/* capacity */ 64u);

    //
    //
    // Set up initial simulation state
    populateEntities(static_cast<za::SizeT>(numEntities));

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
        samplesEventMs.record(clock.getElapsedTime().asSeconds() * 1000.f);
        // ---

        ////////////////////////////////////////////////////////////
        // Update step
        ////////////////////////////////////////////////////////////
        // ---
        clock.restart();
        {
            const auto updateEntity = [&](Entity& entity)
            {
                auto& [text, circleShape, sprite, velocity, torque] = entity;

                sprite.position += velocity;
                sprite.rotation += za::radians(torque);

                if ((sprite.position.x > windowSize.x && velocity.x > 0.f) || (sprite.position.x < 0.f && velocity.x < 0.f))
                    velocity.x = -velocity.x;

                if ((sprite.position.y > windowSize.y && velocity.y > 0.f) || (sprite.position.y < 0.f && velocity.y < 0.f))
                    velocity.y = -velocity.y;

                text.position = sprite.position - za::Vec2f{0.f, 250.f * sprite.scale.x};

                circleShape.position = sprite.position;
                circleShape.rotation = sprite.rotation;
            };

            if (!multithreadedUpdate)
            {
                for (Entity& entity : entities)
                    updateEntity(entity);
            }
            else
            {
                doInBatches([&](const za::SizeT /* iBatch */, const za::SizeT batchStartIdx, const za::SizeT batchEndIdx)
                {
                    for (za::SizeT i = batchStartIdx; i < batchEndIdx; ++i)
                        updateEntity(entities[i]);
                });
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

            ImGui::Begin("Zancle: batching example", nullptr, ImGuiWindowFlags_NoResize);
            ImGui::SetWindowSize({420.f, 490.f});

            const auto clearSamples = [&]
            {
                samplesEventMs.clear();
                samplesUpdateMs.clear();
                samplesImGuiMs.clear();
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

            const za::U64 step = 1u;
            ImGui::SetNextItemWidth(172.f);
            if (ImGui::InputScalar("Autobatch Vertex Threshold", ImGuiDataType_U64, &autoBatchVertexThreshold, &step))
                window.setAutoBatchVertexThreshold(
                    static_cast<za::SizeT>(za::max(autoBatchVertexThreshold, za::U64{1024u})));
            ImGui::EndDisabled();

            if (ImGui::Checkbox("Sprites", &drawSprites))
                clearSamples();

            ImGui::SameLine();
            if (ImGui::Checkbox("Texts", &drawText))
                clearSamples();

            ImGui::SameLine();
            if (ImGui::Checkbox("Shapes", &drawShapes))
                clearSamples();

            ImGui::Checkbox("Multithreaded Update", &multithreadedUpdate);

            ImGui::BeginDisabled(batchType == BatchType::Disabled);
            ImGui::Checkbox("Multithreaded Draw", &multithreadedDraw);
            ImGui::EndDisabled();

            ImGui::BeginDisabled(batchType != BatchType::Disabled);
            ImGui::Checkbox("Use withLockedRenderStates API", &useWithRenderStatesAPI);
            ImGui::EndDisabled();

            ImGui::SetNextItemWidth(172.f);
            ImGui::InputScalar("Workers", ImGuiDataType_U64, &nWorkers, &step);
            nWorkers = za::clamp(nWorkers, za::U64{2u}, static_cast<za::U64>(nMaxWorkers));

            ImGui::NewLine();

            ImGui::Text("Number of entities:");
            ImGui::SetNextItemWidth(172.f);
            ImGui::InputInt("##InputInt", &numEntities);

            if (ImGui::Button("Repopulate") && numEntities > 0)
            {
                populateEntities(static_cast<za::SizeT>(numEntities));
                clearSamples();
            }

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

            ImGui::Spacing();
            ImGui::Text("Drawn vertices: %zu", drawnVertices);
            ImGui::Text("Draw calls: %u", nDrawCalls);

            ImGui::End();
        }
        samplesImGuiMs.record(clock.getElapsedTime().asSeconds() * 1000.f);

        ////////////////////////////////////////////////////////////
        // Draw step
        ////////////////////////////////////////////////////////////
        clock.restart();
        {
            window.clear();

            const auto baseStates = za::RenderStates{.texture = &textureAtlas.getTexture()};

            const auto drawEntity = [&](const Entity& entity, za::SizeT& drawnVertexCounter, auto&& drawFn)
            {
                if (drawSprites)
                {
                    drawFn(entity.sprite);
                    drawnVertexCounter += 4u;
                }
                if (drawText)
                {
                    drawFn(entity.text);
                    drawnVertexCounter += entity.text.getVertices().size();
                }
                if (drawShapes)
                {
                    drawFn(entity.circleShape);
                    drawnVertexCounter += entity.circleShape.getFillVertices().size() +
                                          entity.circleShape.getOutlineVertices().size();
                }
            };

            if (batchType == BatchType::Disabled)
            {
                drawnVertices = 0u;

                if (useWithRenderStatesAPI)
                {
                    auto drawCtx = window.withLockedRenderStates(baseStates);

                    for (const Entity& entity : entities)
                        drawEntity(entity, drawnVertices, [&](const auto& drawable) { drawCtx.draw(drawable); });
                }
                else
                {
                    for (const Entity& entity : entities)
                        drawEntity(entity, drawnVertices, [&](const auto& drawable) {
                            window.draw(drawable, baseStates);
                        });
                }
            }
            else // CPUStorage or GPUStorage
            {
                const auto doWithBatch = [&](auto& batchesArray)
                {
                    for (auto& batch : batchesArray)
                        batch.clear();

                    // Initialize per-worker drawn vertex counts
                    za::Vector<za::SizeT> totalChunkDrawnVertices(nMaxWorkers);

                    const auto populateBatches =
                        [&](const za::SizeT iBatch, const za::SizeT batchStartIdx, const za::SizeT batchEndIdx)
                    {
                        za::SizeT chunkDrawnVertices = 0u; // avoid false sharing

                        for (za::SizeT i = batchStartIdx; i < batchEndIdx; ++i)
                            drawEntity(entities[i], chunkDrawnVertices, [&](const auto& drawable) {
                                batchesArray[iBatch].add(drawable);
                            });

                        totalChunkDrawnVertices[iBatch] = chunkDrawnVertices;
                    };

                    if (multithreadedDraw)
                        doInBatches(populateBatches);
                    else
                        populateBatches(0u, 0u, entities.size());

                    // Tally vertices and submit batches to GPU
                    drawnVertices = 0u;

                    for (za::SizeT i = 0u; i < (multithreadedDraw ? nWorkers : 1u); ++i)
                    {
                        drawnVertices += totalChunkDrawnVertices[i];
                        window.draw(batchesArray[i], baseStates);
                    }
                };

#ifndef ZA_OPENGL_ES
                // If GPU storage, preallocate memory to avoid race conditions
                if (batchType == BatchType::GPUStorage)
                {
                    const za::SizeT     maxEntitiesPerBatch       = (entities.size() + nWorkers - 1) / nWorkers;
                    constexpr za::SizeT maxQuadsPerEntityEstimate = 96u;
                    const za::SizeT     reservationSize           = maxEntitiesPerBatch * maxQuadsPerEntityEstimate;

                    for (za::SizeT i = 0u; i < (multithreadedDraw ? nWorkers : 1u); ++i)
                        gpuDrawableBatches[i].reserveQuads(reservationSize);
                }

                if (batchType == BatchType::CPUStorage)
                    doWithBatch(cpuDrawableBatches);
                else
                    doWithBatch(gpuDrawableBatches);
#else
                doWithBatch(cpuDrawableBatches);
#endif
            }
        }
        samplesDrawMs.record(clock.getElapsedTime().asSeconds() * 1000.f);

        ////////////////////////////////////////////////////////////
        // Display step
        ////////////////////////////////////////////////////////////
        clock.restart();
        {
            imGuiContext.render(window);
            const auto stats = window.display();
            nDrawCalls       = stats.drawCalls;
        }
        samplesDisplayMs.record(clock.getElapsedTime().asSeconds() * 1000.f);

        samplesFPS.record(1.f / fpsClock.getElapsedTime().asSeconds());
    }
}
