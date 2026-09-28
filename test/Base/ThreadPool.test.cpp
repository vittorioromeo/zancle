#include "Tst/Tst.hpp"

#include "Zancle/Concurrency/ThreadPool.hpp"

#include "Zancle/Concurrency/Atomic.hpp"
#include "Zancle/Concurrency/Thread.hpp"

#include "Zancle/Chrono/Clock.hpp"
#include "Zancle/Chrono/Time.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Base/SizeT.hpp"


namespace
{
namespace ThreadPoolTest // for unity builds
{
////////////////////////////////////////////////////////////
// Occupy the single worker of `pool` until `release` is set
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
[[nodiscard]] bool visitsEachIndexOnce(za::ThreadPool& pool, const za::SizeT count, const za::SizeT chunkSize)
{
    za::Vector<za::Atomic<za::SizeT>> hits(count);
    za::Atomic<bool>                  badChunk{false};

    pool.parallelFor(count,
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

} // namespace ThreadPoolTest
} // namespace


TEST_CASE("[Base] Base/ThreadPool.hpp")
{
    SECTION("Construct with 1 worker")
    {
        za::ThreadPool pool(1u);
        REQUIRE(pool.getWorkerCount() == 1u);
    }

    SECTION("Construct with 4 workers")
    {
        za::ThreadPool pool(4u);
        REQUIRE(pool.getWorkerCount() == 4u);
    }

    SECTION("Construct with 8 workers")
    {
        za::ThreadPool pool(8u);
        REQUIRE(pool.getWorkerCount() == 8u);
    }

    SECTION("Construct with 16 workers")
    {
        za::ThreadPool pool(16u);
        REQUIRE(pool.getWorkerCount() == 16u);
    }

    SECTION("Construct with 32 workers")
    {
        za::ThreadPool pool(32u);
        REQUIRE(pool.getWorkerCount() == 32u);
    }

    const auto doJoinTest = [](za::Atomic<int>& result, const int nTasks)
    {
        za::ThreadPool pool(4u);

        for (int i = 0; i < nTasks; ++i)
            pool.post([&] { result.fetchAddRelaxed(1); });
    };

    SECTION("Join 1 task on destruction")
    {
        za::Atomic<int> result{0};
        doJoinTest(result, 1);
        REQUIRE(result.loadRelaxed() == 1);
    }

    SECTION("Join 2 tasks on destruction")
    {
        za::Atomic<int> result{0};
        doJoinTest(result, 2);
        REQUIRE(result.loadRelaxed() == 2);
    }

    SECTION("Join 4 tasks on destruction")
    {
        za::Atomic<int> result{0};
        doJoinTest(result, 4);
        REQUIRE(result.loadRelaxed() == 4);
    }

    SECTION("Join 8 tasks on destruction")
    {
        za::Atomic<int> result{0};
        doJoinTest(result, 8);
        REQUIRE(result.loadRelaxed() == 8);
    }

    SECTION("Join 256 tasks on destruction")
    {
        za::Atomic<int> result{0};
        doJoinTest(result, 256);
        REQUIRE(result.loadRelaxed() == 256);
    }
}


TEST_CASE("[Base] ThreadPool: postBulk and postCopies")
{
    za::Atomic<int> result{0};

    SECTION("postBulk runs every task")
    {
        {
            za::ThreadPool pool(4u);

            za::ThreadPool::Task tasks[100];
            for (int i = 0; i < 100; ++i)
                tasks[i] = [&result, i] { result.fetchAddRelaxed(i + 1); };

            pool.postBulk(tasks, 100u);
            pool.postBulk(tasks, 0u);   // no-op
            pool.postBulk(nullptr, 0u); // no-op
        }

        CHECK(result.loadRelaxed() == 100 * 101 / 2);
    }

    SECTION("postCopies runs every copy")
    {
        {
            za::ThreadPool pool(4u);
            pool.postCopies([&result] { result.fetchAddRelaxed(1); }, 1000u);
            pool.postCopies([&result] { result.fetchAddRelaxed(1); }, 0u); // no-op
        }

        CHECK(result.loadRelaxed() == 1000);
    }
}


TEST_CASE("[Base] ThreadPool: tryRunPendingTask")
{
    // Declared before the pool, which uses them until it is destroyed
    za::Atomic<bool> started{false};
    za::Atomic<bool> release{false};
    za::ThreadPool   pool(1u);

    ThreadPoolTest::blockWorker(pool, started, release);

    za::ThreadId ranOn;
    pool.post([&ranOn] { ranOn = za::ThisThread::getId(); });

    CHECK(pool.tryRunPendingTask()); // the worker is busy: the task runs here
    CHECK(ranOn == za::ThisThread::getId());
    CHECK(!pool.tryRunPendingTask()); // nothing left

    release.storeRelease(true);
}


TEST_CASE("[Base] ThreadPool: parallelFor")
{
    SECTION("Every index is visited exactly once")
    {
        for (const za::SizeT nWorkers : {1u, 3u, 8u})
        {
            za::ThreadPool pool(nWorkers);

            for (const za::SizeT count : {0u, 1u, 2u, 7u, 100u, 1000u, 100'000u})
                for (const za::SizeT chunkSize : {0u, 1u, 3u, 64u, 1'000'000u})
                    CHECK(ThreadPoolTest::visitsEachIndexOnce(pool, count, chunkSize));
        }
    }

    SECTION("Writes are visible to the caller after returning")
    {
        za::ThreadPool  pool(4u);
        za::Vector<int> values(10'000u, 0);

        pool.parallelFor(values.size(),
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
        za::Atomic<bool> started{false};
        za::Atomic<bool> release{false};
        za::ThreadPool   pool(1u);

        ThreadPoolTest::blockWorker(pool, started, release);

        // The helper task stays queued: the caller must run it itself rather than wait for the worker
        const za::ThreadId caller = za::ThisThread::getId();
        za::Atomic<bool>   otherThread{false};

        pool.parallelFor(1000u,
                         [&](za::SizeT, za::SizeT)
        {
            if (za::ThisThread::getId() != caller)
                otherThread.storeRelaxed(true);
        },
                         1u);

        CHECK(!otherThread.loadRelaxed());
        release.storeRelease(true);
    }

    SECTION("Nested calls from within a task")
    {
        za::ThreadPool        pool(2u);
        za::Atomic<za::SizeT> total{0u};

        pool.parallelFor(16u,
                         [&](za::SizeT begin, const za::SizeT end)
        {
            for (; begin != end; ++begin)
                pool.parallelFor(100u, [&](const za::SizeT b, const za::SizeT e) { total.fetchAddRelaxed(e - b); });
        },
                         1u);

        CHECK(total.loadRelaxed() == 1600u);
    }

    SECTION("Concurrent calls from multiple threads")
    {
        za::ThreadPool        pool(4u);
        za::Atomic<za::SizeT> total{0u};

        {
            za::Vector<za::Thread> callers;
            for (int t = 0; t < 4; ++t)
                callers.emplaceBack([&]
                {
                    for (int round = 0; round < 50; ++round)
                        pool.parallelFor(1000u,
                                         [&](const za::SizeT b, const za::SizeT e) { total.fetchAddRelaxed(e - b); });
                });
        } // joins

        CHECK(total.loadRelaxed() == 4u * 50u * 1000u);
    }
}


TEST_CASE("[Base] ThreadPool: posting")
{
    SECTION("Tasks can post tasks")
    {
        za::Atomic<int> result{0};
        za::ThreadPool  pool(4u);

        for (int i = 0; i < 100; ++i)
            pool.post([&]
            {
                result.fetchAddRelaxed(1);

                for (int j = 0; j < 10; ++j)
                    pool.post([&] { result.fetchAddRelaxed(1); });
            });

        // Wait here rather than relying on destruction, which cannot know about tasks posted later by running tasks
        while (result.loadRelaxed() != 1100)
            if (!pool.tryRunPendingTask())
                za::ThisThread::yield();

        CHECK(result.loadRelaxed() == 1100);
    }

    SECTION("Destruction waits for a running task and runs pending ones")
    {
        za::Atomic<bool> longTaskDone{false};
        za::Atomic<int>  result{0};

        {
            za::ThreadPool pool(2u);

            pool.post([&]
            {
                za::ThisThread::sleepFor(za::milliseconds(20));
                longTaskDone.storeRelease(true);
            });

            for (int i = 0; i < 100; ++i)
                pool.post([&] { result.fetchAddRelaxed(1); });
        }

        CHECK(longTaskDone.loadAcquire());
        CHECK(result.loadRelaxed() == 100);
    }

    SECTION("Concurrent posting from multiple threads")
    {
        za::Atomic<int> result{0};

        {
            za::ThreadPool pool(4u);

            {
                za::Vector<za::Thread> producers;
                for (int t = 0; t < 4; ++t)
                    producers.emplaceBack([&]
                    {
                        for (int i = 0; i < 10'000; ++i)
                            pool.post([&] { result.fetchAddRelaxed(1); });
                    });
            } // joins the producers
        } // runs the pending tasks

        CHECK(result.loadRelaxed() == 40'000);
    }
}


TEST_CASE("[Base] ThreadPool: destruction")
{
    SECTION("Construct and immediately destroy, many times")
    {
        // Workers may not even have started when destruction begins
        for (int i = 0; i < 50; ++i)
            for (const za::SizeT nWorkers : {1u, 2u, 7u, 32u})
            {
                za::ThreadPool pool(nWorkers);
                CHECK(pool.getWorkerCount() == nWorkers);
            }
    }

    SECTION("Does not spin while a long task runs")
    {
        // The previous implementation posted empty tasks in a tight loop until every worker noticed the
        // shutdown: gigabytes of queued tasks, and roughly double the time, when a task ran for a while
        constexpr auto taskDuration = za::milliseconds(300);

        const za::Clock clock;
        {
            za::ThreadPool pool(4u);
            pool.post([&] { za::ThisThread::sleepFor(taskDuration); });
        }

        const za::Time elapsed = clock.getElapsedTime();
        CHECK(elapsed >= taskDuration);
        CHECK(elapsed < taskDuration + za::milliseconds(250)); // generous, to tolerate loaded machines
    }

    SECTION("Tasks posted by running tasks during destruction still run")
    {
        za::Atomic<int> result{0};

        {
            za::ThreadPool pool(2u);

            pool.post([&]
            {
                za::ThisThread::sleepFor(za::milliseconds(30)); // destruction starts meanwhile

                for (int i = 0; i < 100; ++i)
                    pool.post([&]
                    {
                        result.fetchAddRelaxed(1);
                        pool.post([&] { result.fetchAddRelaxed(1); });
                    });
            });
        }

        CHECK(result.loadRelaxed() == 200);
    }

    SECTION("parallelFor from tasks running during destruction")
    {
        // The stop tasks are queued while `parallelFor` waits for its helpers: it must neither run
        // them as regular tasks nor block while its own helpers are still queued behind them
        for (int iteration = 0; iteration < 10; ++iteration)
            for (const za::SizeT nWorkers : {1u, 2u, 4u})
            {
                za::Atomic<za::SizeT> total{0u};

                {
                    za::ThreadPool pool(nWorkers);

                    for (za::SizeT w = 0u; w < nWorkers; ++w)
                        pool.post([&]
                        {
                            za::ThisThread::sleepFor(za::milliseconds(5)); // destruction starts meanwhile

                            pool.parallelFor(1000u, [&](const za::SizeT b, const za::SizeT e) {
                                total.fetchAddRelaxed(e - b);
                            }, 1u);
                        });
                }

                CHECK(total.loadRelaxed() == nWorkers * 1000u);
            }
    }
}


TEST_CASE("[Base] ThreadPool: a task's captured state is released right after it runs")
{
    // Sets `destroyed` when the last copy of the task holding it is destroyed
    struct DestructionFlag
    {
        za::Atomic<bool>* destroyed;
        bool              owner = true;

        explicit DestructionFlag(za::Atomic<bool>& flag) : destroyed{&flag}
        {
        }

        DestructionFlag(const DestructionFlag& rhs) : destroyed{rhs.destroyed}
        {
        }

        DestructionFlag(DestructionFlag&& rhs) noexcept : destroyed{rhs.destroyed}
        {
            rhs.owner = false;
        }

        DestructionFlag& operator=(const DestructionFlag&) = delete;
        DestructionFlag& operator=(DestructionFlag&&)      = delete;

        ~DestructionFlag()
        {
            if (owner)
                destroyed->storeRelease(true);
        }
    };

    za::Atomic<bool> destroyed{false};
    za::ThreadPool   pool(1u);

    {
        DestructionFlag flag{destroyed};
        pool.post([f = static_cast<DestructionFlag&&>(flag)] { (void)f; });
    }

    // No other task follows: the worker must not keep the finished task around until the next one
    const za::Clock clock;
    while (!destroyed.loadAcquire() && clock.getElapsedTime() < za::seconds(5.f))
        za::ThisThread::yield();

    CHECK(destroyed.loadAcquire());
}
