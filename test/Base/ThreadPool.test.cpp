#include "AlignedAllocationUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/Concurrency/ThreadPool.hpp"

#include "Zancle/Concurrency/Atomic.hpp"
#include "Zancle/Concurrency/Thread.hpp"

#include "Zancle/Chrono/Clock.hpp"
#include "Zancle/Chrono/Time.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Base/SizeT.hpp"

#ifdef ALIGNED_ALLOCATION_UTIL_AVAILABLE
    #include <new>
#endif


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


TEST_CASE("[Base] ThreadPool: optimal thread and worker counts")
{
    const za::SizeT threads = za::ThreadPool::getOptimalThreadCount();
    const za::SizeT workers = za::ThreadPool::getOptimalWorkerCount();

    CHECK(threads >= 1u);
    CHECK(threads == za::Thread::usableHardwareConcurrency());
    CHECK(workers == (threads > 1u ? threads - 1u : 1u)); // a pool needs a worker
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
}


#ifdef ALIGNED_ALLOCATION_UTIL_AVAILABLE

TEST_CASE("[Base] ThreadPool: a failed construction stops the workers it started")
{
    // Fail each aligned allocation of the constructor in turn (the workers' thread entries): the constructor
    // must stop and join the workers it already started, which would otherwise wait forever on the empty queue
    za::SizeT failures = 0u;

    for (za::SizeT successes = 0u; successes < 64u; ++successes)
    {
        bool threw    = false;
        bool injected = false;

        failAlignedAllocationAfter(successes);

        try
        {
            const za::ThreadPool pool(4u);
            injected = stopFailingAlignedAllocations();
        } catch (const std::bad_alloc&)
        {
            threw    = true;
            injected = stopFailingAlignedAllocations();
        }

        CHECK(threw == injected);

        if (!injected) // every allocation of the constructor succeeded
            break;

        ++failures;
    }

    #if defined(ZA_STATIC) || !defined(_WIN32)
    CHECK(failures >= 1u); // at least one thread entry (a Windows DLL keeps its own `operator new`)
    #endif
}

#endif


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
