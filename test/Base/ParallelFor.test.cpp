#include "Tst/Tst.hpp"

#include "Zancle/Concurrency/ParallelFor.hpp"

#include "Zancle/Concurrency/Atomic.hpp"
#include "Zancle/Concurrency/Thread.hpp"
#include "Zancle/Concurrency/ThreadPool.hpp"

#include "Zancle/Chrono/Clock.hpp"
#include "Zancle/Chrono/Time.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Base/SizeT.hpp"

#include <initializer_list>


namespace
{
namespace ParallelForTest // for unity builds
{
////////////////////////////////////////////////////////////
// Occupy one worker of `pool` until `release` is set
void blockWorker(za::ThreadPool& pool, za::Atomic<bool>& started, za::Atomic<bool>& release)
{
    pool.post([&]
    {
        started.storeRelease(true);

        while (!release.loadAcquire())
            za::ThisThread::yield();
    });

    while (!started.loadAcquire())
        za::ThisThread::yield();
}


////////////////////////////////////////////////////////////
// Run `parallelFor` over `count` elements, checking that every index is visited exactly once
[[nodiscard]] bool visitsEachIndexOnce(za::ThreadPool& pool, za::ParallelForSlots& slots, const za::SizeT count, const za::SizeT chunkSize)
{
    za::Vector<za::Atomic<za::SizeT>> hits(count);
    za::Atomic<bool>                  badChunk{false};

    za::parallelFor(pool,
                    slots,
                    count,
                    [&](const za::SizeT begin, const za::SizeT end)
    {
        if (begin >= end || end > count || (chunkSize != 0u && end - begin > chunkSize))
            badChunk.storeRelaxed(true);

        for (za::SizeT i = begin; i < end; ++i)
            hits[i].fetchAddRelaxed(1u);
    },
                    chunkSize);

    if (badChunk.loadRelaxed())
        return false;

    for (const auto& h : hits)
        if (h.loadRelaxed() != 1u)
            return false;

    return true;
}


////////////////////////////////////////////////////////////
// `depth` nested calls, each counting its own elements into `total`
void nest(za::ThreadPool& pool, za::ParallelForSlots& slots, const int depth, za::Atomic<za::SizeT>& total)
{
    za::parallelFor(pool,
                    slots,
                    2u,
                    [&](za::SizeT begin, const za::SizeT end)
    {
        for (; begin != end; ++begin)
        {
            total.fetchAddRelaxed(1u);

            if (begin == 0u && depth > 1)
                nest(pool, slots, depth - 1, total);
        }
    },
                    1u);
}


////////////////////////////////////////////////////////////
// Deterministic pseudo-random sequence (no shared state between threads)
[[nodiscard]] za::SizeT nextRandom(za::SizeT& state)
{
    state = state * 6'364'136'223'846'793'005ull + 1'442'695'040'888'963'407ull;
    return static_cast<za::SizeT>(state >> 33u);
}

} // namespace ParallelForTest
} // namespace


TEST_CASE("[Base] ParallelFor.hpp")
{
    SECTION("Every index is visited exactly once")
    {
        for (const za::SizeT nWorkers : {1u, 3u, 8u})
        {
            za::ThreadPool       pool(nWorkers);
            za::ParallelForSlots slots;

            for (const za::SizeT count : {0u, 1u, 2u, 7u, 100u, 1000u, 100'000u})
                for (const za::SizeT chunkSize : {0u, 1u, 3u, 64u, 1'000'000u})
                    CHECK(ParallelForTest::visitsEachIndexOnce(pool, slots, count, chunkSize));
        }
    }

    SECTION("Writes are visible to the caller after returning")
    {
        za::ThreadPool       pool(4u);
        za::ParallelForSlots slots;
        za::Vector<int>      values(10'000u, 0);

        za::parallelFor(pool,
                        slots,
                        values.size(),
                        [&](za::SizeT begin, const za::SizeT end)
        {
            for (; begin != end; ++begin)
                values[begin] = static_cast<int>(begin);
        });

        bool allWritten = true;
        for (za::SizeT i = 0u; i < values.size(); ++i)
            allWritten &= values[i] == static_cast<int>(i);

        CHECK(allWritten);
    }

    SECTION("The calling thread does all the work when every worker is busy")
    {
        // Declared before the pool, which uses them until it is destroyed
        za::Atomic<bool>     started{false};
        za::Atomic<bool>     release{false};
        za::ThreadPool       pool(1u);
        za::ParallelForSlots slots;

        ParallelForTest::blockWorker(pool, started, release);

        const za::ThreadId caller = za::ThisThread::getId();
        za::Atomic<bool>   otherThread{false};

        // The helper task stays queued: the caller must neither wait for it nor run it
        za::parallelFor(pool,
                        slots,
                        1000u,
                        [&](za::SizeT, za::SizeT)
        {
            if (za::ThisThread::getId() != caller)
                otherThread.storeRelaxed(true);
        },
                        1u);

        CHECK(!otherThread.loadRelaxed());
        release.storeRelease(true);
    }

    SECTION("The calling thread never runs unrelated tasks, nor waits for them")
    {
        // Regression: while waiting for its queued helpers, the caller used to run whatever task was next
        // in the queue, e.g. a long unrelated one, turning a quick call into a long one
        za::Atomic<bool>     started{false};
        za::Atomic<bool>     release{false};
        za::Atomic<bool>     unrelatedRan{false};
        za::ThreadPool       pool(1u);
        za::ParallelForSlots slots;

        ParallelForTest::blockWorker(pool, started, release);

        pool.post([&]
        {
            za::ThisThread::sleepFor(za::milliseconds(200));
            unrelatedRan.storeRelease(true);
        });

        za::Atomic<za::SizeT> total{0u};

        const za::Clock clock;
        za::parallelFor(pool, slots, 1000u, [&](const za::SizeT b, const za::SizeT e) { total.fetchAddRelaxed(e - b); }, 1u);
        const za::Time elapsed = clock.getElapsedTime();

        CHECK(total.loadRelaxed() == 1000u);
        CHECK(!unrelatedRan.loadAcquire()); // still queued behind the blocked worker
        CHECK(elapsed < za::milliseconds(100));

        release.storeRelease(true);
    }

    SECTION("Nested calls from within a task")
    {
        za::ThreadPool        pool(2u);
        za::ParallelForSlots  slots;
        za::Atomic<za::SizeT> total{0u};

        za::parallelFor(pool,
                        slots,
                        16u,
                        [&](za::SizeT begin, const za::SizeT end)
        {
            for (; begin != end; ++begin)
                za::parallelFor(pool, slots, 100u, [&](const za::SizeT b, const za::SizeT e) {
                    total.fetchAddRelaxed(e - b);
                });
        },
                        1u);

        CHECK(total.loadRelaxed() == 1600u);
    }

    SECTION("More nested calls than gates: the excess run on the calling thread")
    {
        za::ThreadPool        pool(4u);
        za::ParallelForSlots  slots;
        za::Atomic<za::SizeT> total{0u};

        constexpr int depth = static_cast<int>(za::ParallelForSlots::slotCount) + 16;
        ParallelForTest::nest(pool, slots, depth, total);

        CHECK(total.loadRelaxed() == 2u * static_cast<za::SizeT>(depth));
    }

    SECTION("Concurrent calls from multiple threads")
    {
        za::ThreadPool        pool(4u);
        za::ParallelForSlots  slots;
        za::Atomic<za::SizeT> total{0u};

        {
            za::Vector<za::Thread> callers;
            for (int t = 0; t < 4; ++t)
                callers.emplaceBack([&]
                {
                    for (int round = 0; round < 50; ++round)
                        za::parallelFor(pool, slots, 1000u, [&](const za::SizeT b, const za::SizeT e) {
                            total.fetchAddRelaxed(e - b);
                        });
                });
        } // joins

        CHECK(total.loadRelaxed() == 4u * 50u * 1000u);
    }

    SECTION("Two pools sharing the same slots")
    {
        za::ThreadPool        poolA(2u);
        za::ThreadPool        poolB(3u);
        za::ParallelForSlots  slots;
        za::Atomic<za::SizeT> total{0u};

        za::parallelFor(poolA,
                        slots,
                        8u,
                        [&](za::SizeT begin, const za::SizeT end)
        {
            for (; begin != end; ++begin)
                za::parallelFor(poolB, slots, 500u, [&](const za::SizeT b, const za::SizeT e) {
                    total.fetchAddRelaxed(e - b);
                });
        },
                        1u);

        CHECK(total.loadRelaxed() == 8u * 500u);
    }
}


TEST_CASE("[Base] ParallelFor.hpp - helpers that start late")
{
    SECTION("Late helpers never touch a call that already returned, even when its gate is reused")
    {
        // One worker is blocked: its helpers pile up in the queue, and only run (long) after their calls
        // returned, when their gates have been reused many times. AddressSanitizer would catch a helper
        // touching a dead stack frame, and the per-call checks a helper entering a later call's gate.
        za::Atomic<bool>     started{false};
        za::Atomic<bool>     release{false};
        za::ThreadPool       pool(2u);
        za::ParallelForSlots slots;

        ParallelForTest::blockWorker(pool, started, release);

        bool allCorrect = true;
        for (int call = 0; call < 20'000; ++call)
        {
            za::Atomic<za::SizeT> visited{0u};
            za::parallelFor(pool, slots, 8u, [&](const za::SizeT b, const za::SizeT e) {
                visited.fetchAddRelaxed(e - b);
            }, 1u);
            allCorrect &= visited.loadRelaxed() == 8u;
        }

        CHECK(allCorrect);
        release.storeRelease(true);
    }

    SECTION("Destroying the slots before the pool waits for queued helpers")
    {
        za::Atomic<bool> started{false};
        za::Atomic<bool> release{false};
        za::ThreadPool   pool(1u);

        ParallelForTest::blockWorker(pool, started, release);

        // Releases the worker later, from another thread, while the slots' destructor waits
        za::Thread releaser{[&]
        {
            za::ThisThread::sleepFor(za::milliseconds(50));
            release.storeRelease(true);
        }};

        const za::Clock clock;
        {
            za::ParallelForSlots  slots;
            za::Atomic<za::SizeT> total{0u};

            za::parallelFor(pool, slots, 100u, [&](const za::SizeT b, const za::SizeT e) {
                total.fetchAddRelaxed(e - b);
            }, 1u);
            CHECK(total.loadRelaxed() == 100u);
        } // the helper is still queued behind the blocked worker: wait for it

        CHECK(clock.getElapsedTime() >= za::milliseconds(40));
        releaser.join();
    }

    SECTION("Destroying the pool before the slots")
    {
        za::ParallelForSlots  slots;
        za::Atomic<za::SizeT> total{0u};

        for (int i = 0; i < 100; ++i)
        {
            za::ThreadPool pool(3u);
            za::parallelFor(pool, slots, 1000u, [&](const za::SizeT b, const za::SizeT e) {
                total.fetchAddRelaxed(e - b);
            }, 1u);
        }

        CHECK(total.loadRelaxed() == 100u * 1000u);
    }

    SECTION("Calls from tasks running during the pool's destruction")
    {
        // The stop tasks are queued while a call is in progress: its helpers may never run before the
        // destructor drains the queue, and the call must not depend on them
        for (int iteration = 0; iteration < 10; ++iteration)
            for (const za::SizeT nWorkers : {1u, 2u, 4u})
            {
                za::ParallelForSlots  slots;
                za::Atomic<za::SizeT> total{0u};

                {
                    za::ThreadPool pool(nWorkers);

                    for (za::SizeT w = 0u; w < nWorkers; ++w)
                        pool.post([&]
                        {
                            za::ThisThread::sleepFor(za::milliseconds(5)); // destruction starts meanwhile

                            za::parallelFor(pool, slots, 1000u, [&](const za::SizeT b, const za::SizeT e) {
                                total.fetchAddRelaxed(e - b);
                            }, 1u);
                        });
                }

                CHECK(total.loadRelaxed() == nWorkers * 1000u);
            }
    }
}


TEST_CASE("[Base] ParallelFor.hpp - stress")
{
    // Several threads, each making many calls of random sizes and chunk sizes, some nested
    za::ThreadPool       pool(6u);
    za::ParallelForSlots slots;
    za::Atomic<int>      failures{0};

    {
        za::Vector<za::Thread> callers;
        for (za::SizeT t = 0u; t < 4u; ++t)
            callers.emplaceBack([&, t]
            {
                za::SizeT rng = t + 1u;

                for (int round = 0; round < 300; ++round)
                {
                    const za::SizeT count     = ParallelForTest::nextRandom(rng) % 2000u;
                    const za::SizeT chunkSize = ParallelForTest::nextRandom(rng) % 50u;
                    const bool      nested    = ParallelForTest::nextRandom(rng) % 4u == 0u;

                    za::Atomic<za::SizeT> visited{0u};
                    za::Atomic<za::SizeT> calls{0u};
                    za::Atomic<za::SizeT> innerVisited{0u};

                    za::parallelFor(pool,
                                    slots,
                                    count,
                                    [&](const za::SizeT b, const za::SizeT e)
                    {
                        visited.fetchAddRelaxed(e - b);
                        calls.fetchAddRelaxed(1u);

                        if (nested)
                            za::parallelFor(pool, slots, 64u, [&](const za::SizeT ib, const za::SizeT ie) {
                                innerVisited.fetchAddRelaxed(ie - ib);
                            });
                    },
                                    chunkSize);

                    // With an explicit chunk size, the number of calls is known
                    const bool callsOk = chunkSize == 0u || count == 0u ||
                                         calls.loadRelaxed() == (count - 1u) / chunkSize + 1u;

                    if (visited.loadRelaxed() != count || !callsOk ||
                        innerVisited.loadRelaxed() != (nested ? calls.loadRelaxed() * 64u : 0u))
                        failures.fetchAddRelaxed(1);
                }
            });
    } // joins

    CHECK(failures.loadRelaxed() == 0);
}
