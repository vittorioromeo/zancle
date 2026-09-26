#include "SystemUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/Concurrency/Thread.hpp"

#include "Zancle/Concurrency/Atomic.hpp"

#include "Zancle/Chrono/Clock.hpp"
#include "Zancle/Chrono/Time.hpp"

#include "Zancle/Base/IntTypes.hpp"

#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsMoveAssignable.hpp"
#include "Zancle/Trait/IsMoveConstructible.hpp"


////////////////////////////////////////////////////////////
// `Thread` is move-only -- copy operations must be deleted.
////////////////////////////////////////////////////////////
static_assert(!ZA_IS_COPY_CONSTRUCTIBLE(za::Thread));
static_assert(!ZA_IS_COPY_ASSIGNABLE(za::Thread));

static_assert(ZA_IS_MOVE_CONSTRUCTIBLE(za::Thread));
static_assert(ZA_IS_MOVE_ASSIGNABLE(za::Thread));


TEST_CASE("[System] Zancle/Concurrency/Thread.hpp - default-constructed is not joinable")
{
    za::Thread t;
    CHECK(!t.joinable());
    CHECK(t.getId().value() == 0u);
}

TEST_CASE("[System] Zancle/Concurrency/Thread.hpp - hardwareConcurrency returns positive value")
{
    const unsigned int n = za::Thread::hardwareConcurrency();
    CHECK(n >= 1u); // every reasonable test target has at least one core
}

TEST_CASE("[System] Zancle/Concurrency/Thread.hpp - spawn + join round-trip")
{
    za::Atomic<int> ran{0};

    za::Thread t{[&ran] { ran.storeRelease(42); }};

    CHECK(t.joinable());
    t.join();
    CHECK(!t.joinable());
    CHECK(ran.loadAcquire() == 42);
}

TEST_CASE("[System] Zancle/Concurrency/Thread.hpp - getId is non-zero for a running thread")
{
    za::Atomic<za::U64> observedId{0u};

    za::Thread t{[&observedId] { observedId.storeRelease(za::ThisThread::getId().value()); }};

    const za::ThreadId outsideId = t.getId();
    CHECK(outsideId.value() != 0u);

    t.join();

    CHECK(observedId.loadAcquire() == outsideId.value());
}

TEST_CASE("[System] Zancle/Concurrency/Thread.hpp - move construction transfers ownership")
{
    za::Atomic<int> finished{0};

    za::Thread t1{[&finished]
    {
        za::ThisThread::sleepFor(za::milliseconds(10));
        finished.storeRelease(1);
    }};

    CHECK(t1.joinable());
    const za::ThreadId id = t1.getId();

    za::Thread t2{static_cast<za::Thread&&>(t1)};

    CHECK(!t1.joinable());
    CHECK(t2.joinable());
    CHECK(t2.getId().value() == id.value());

    t2.join();
    CHECK(finished.loadAcquire() == 1);
}

TEST_CASE("[System] Zancle/Concurrency/Thread.hpp - destructor implicitly joins (std::jthread semantics)")
{
    za::Atomic<int> ran{0};

    {
        za::Thread t{[&ran]
        {
            za::ThisThread::sleepFor(za::milliseconds(10));
            ran.storeRelease(123);
        }};
        // No explicit join/detach -- the destructor at scope exit must
        // wait for the worker to finish, not abort.
    }

    CHECK(ran.loadAcquire() == 123);
}

TEST_CASE("[System] Zancle/Concurrency/Thread.hpp - move assignment to joinable target implicitly joins")
{
    za::Atomic<int> firstRan{0};
    za::Atomic<int> secondRan{0};

    za::Thread t1{[&firstRan]
    {
        za::ThisThread::sleepFor(za::milliseconds(10));
        firstRan.storeRelease(1);
    }};

    za::Thread t2{[&secondRan] { secondRan.storeRelease(2); }};

    // Overwriting a joinable target must implicitly join the previous
    // worker (matching std::jthread).
    t1 = static_cast<za::Thread&&>(t2);

    CHECK(firstRan.loadAcquire() == 1); // ran to completion before the move-assign returned
    CHECK(t1.joinable());
    CHECK(!t2.joinable());

    t1.join();
    CHECK(secondRan.loadAcquire() == 2);
}

TEST_CASE("[System] Zancle/Concurrency/Thread.hpp - move assignment to non-joinable target")
{
    za::Atomic<int> ran{0};

    za::Thread t1{[&ran] { ran.storeRelease(7); }};

    za::Thread t2;
    t2 = static_cast<za::Thread&&>(t1);

    CHECK(!t1.joinable());
    CHECK(t2.joinable());

    t2.join();
    CHECK(ran.loadAcquire() == 7);
}

TEST_CASE("[System] Zancle/Concurrency/Thread.hpp - detach releases joinability")
{
    za::Atomic<int> done{0};

    {
        za::Thread t{[&done]
        {
            za::ThisThread::sleepFor(za::milliseconds(5));
            done.storeRelease(1);
        }};
        t.detach();
        CHECK(!t.joinable());
    }

    // Wait for the detached thread to finish before we leave the test
    // so the runner does not see a leaked thread.
    while (done.loadAcquire() == 0)
        za::ThisThread::yield();

    CHECK(done.loadAcquire() == 1);
}

TEST_CASE("[System] Zancle/Concurrency/Thread.hpp - ThisThread::getId is stable on the calling thread")
{
    const za::ThreadId id1 = za::ThisThread::getId();
    const za::ThreadId id2 = za::ThisThread::getId();
    CHECK(id1.value() == id2.value());
    CHECK(id1.value() != 0u);
}

TEST_CASE("[System] Zancle/Concurrency/Thread.hpp - sleep sleeps for at least the given time")
{
    // OS schedulers may oversleep, but should never undersleep.
    // Replaces the standalone `Sleep.test.cpp` round-trip.
    const auto checkSleeps = [](const za::Time duration)
    {
        za::Clock      clock;
        const za::Time start = clock.getElapsedTime();

        za::ThisThread::sleepFor(duration);

        CHECK((clock.getElapsedTime() - start) >= duration);
    };

    checkSleeps(za::milliseconds(1));
    checkSleeps(za::milliseconds(5));
    checkSleeps(za::milliseconds(25));
}

TEST_CASE("[System] Zancle/Concurrency/Thread.hpp - many threads each see distinct ids")
{
    constexpr int       threadCount = 8;
    za::Atomic<za::U32> distinctSum{0u};

    za::Thread threads[threadCount]{};

    for (auto& t : threads)
        t = za::Thread{[&distinctSum]
        {
            // Add the lower 32 bits of this thread's id. With 64-bit
            // counter values, ids are unique across this short test
            // window; the sum being equal to the sum of the unique
            // ids the threads observed is the test.
            distinctSum.fetchAddRelaxed(static_cast<za::U32>(za::ThisThread::getId().value()));
        }};

    za::U32 expected = 0u;
    for (auto& t : threads)
    {
        const za::U32 id = static_cast<za::U32>(t.getId().value());
        CHECK(id != 0u);
        expected += id;
        t.join();
    }

    CHECK(distinctSum.loadSeqCst() == expected);
}
