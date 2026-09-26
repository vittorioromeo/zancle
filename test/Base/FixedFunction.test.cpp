#include "Tst/Tst.hpp"

#include "Zancle/Vocabulary/FixedFunction.hpp"

#include "Zancle/Vocabulary/UniquePtr.hpp"

#include "Zancle/Base/Macros.hpp"


namespace
{
namespace FixedFunctionTest // to support unity builds
{

////////////////////////////////////////////////////////////
struct Counters
{
    int constructions{};
    int copies{};
    int moves{};
    int destructions{};

    [[nodiscard]] int alive() const
    {
        return constructions + copies + moves - destructions;
    }
};


////////////////////////////////////////////////////////////
struct LifecycleTracker
{
    Counters* counters;
    int       value;

    explicit LifecycleTracker(Counters& c, int v = 0) : counters(&c), value(v)
    {
        ++counters->constructions;
    }

    LifecycleTracker(const LifecycleTracker& rhs) : counters(rhs.counters), value(rhs.value)
    {
        ++counters->copies;
    }

    LifecycleTracker(LifecycleTracker&& rhs) noexcept : counters(rhs.counters), value(rhs.value)
    {
        rhs.value = -1;
        ++counters->moves;
    }

    LifecycleTracker& operator=(const LifecycleTracker& rhs)
    {
        counters = rhs.counters;
        value    = rhs.value;
        ++counters->copies;
        return *this;
    }

    LifecycleTracker& operator=(LifecycleTracker&& rhs) noexcept
    {
        counters  = rhs.counters;
        value     = rhs.value;
        rhs.value = -1;
        ++counters->moves;
        return *this;
    }

    ~LifecycleTracker()
    {
        ++counters->destructions;
    }

    int operator()() const
    {
        return value;
    }
};


////////////////////////////////////////////////////////////
using FF  = za::FixedFunction<int(), 64>;
using FFv = za::FixedFunction<void(), 64>;


////////////////////////////////////////////////////////////
int freeFunction()
{
    return 42;
}


////////////////////////////////////////////////////////////
int freeFunctionWithArg(int x)
{
    return x * 2;
}


TEST_CASE("[Base] Base/FixedFunction.hpp")
{
    SECTION("Default construction")
    {
        FF ff;
        CHECK(!ff);
    }

    SECTION("Nullptr construction")
    {
        FF ff(nullptr);
        CHECK(!ff);
    }

    SECTION("Null function pointer construction")
    {
        int (*const nullFn)() = nullptr;

        FF ff(nullFn);
        CHECK(!ff);

        FF copy(ff); // NOLINT(performance-unnecessary-copy-initialization)
        CHECK(!copy);
    }

    SECTION("Function pointer construction and invocation")
    {
        FF ff(freeFunction);
        CHECK(!!ff);
        CHECK(ff() == 42);
    }

    SECTION("Function pointer with argument")
    {
        za::FixedFunction<int(int), 64> ff(freeFunctionWithArg);
        CHECK(!!ff);
        CHECK(ff(5) == 10);
    }

    SECTION("Stateless lambda")
    {
        FF ff([] { return 99; });
        CHECK(!!ff);
        CHECK(ff() == 99);
    }

    SECTION("Stateful lambda (capture by value)")
    {
        int x = 7;
        FF  ff([x] { return x * 3; });
        CHECK(!!ff);
        CHECK(ff() == 21);
    }

    SECTION("Construction from a const lvalue callable copies it")
    {
        Counters c;

        {
            const LifecycleTracker tracker(c, 5);
            FF                     ff(tracker);

            CHECK(c.copies == 1);
            CHECK(c.moves == 0);
            CHECK(ff() == 5);
        }

        CHECK(c.alive() == 0);

        // The stored copy is not const, so a mutable lambda remains callable
        const auto counter = [n = 0]() mutable { return ++n; };
        FF         ff(counter);

        CHECK(ff() == 1);
        CHECK(ff() == 2);
    }

    SECTION("Move-only callable")
    {
        FF ff([p = za::makeUnique<int>(21)] { return *p * 2; });
        CHECK(ff() == 42);

        FF moved(ZA_MOVE(ff));
        CHECK(!ff);
        CHECK(moved() == 42);

        FF assigned;
        assigned = ZA_MOVE(moved);
        CHECK(!moved);
        CHECK(assigned() == 42);
    }

    SECTION("Stateful callable - construction and destruction")
    {
        Counters c;

        {
            LifecycleTracker tracker(c, 10);
            CHECK(c.constructions == 1);
            CHECK(c.alive() == 1);

            {
                FF ff(ZA_MOVE(tracker));
                // 1 construction + 1 move into FixedFunction storage
                CHECK(c.constructions == 1);
                CHECK(c.moves == 1);
                CHECK(ff() == 10);
            }

            // FixedFunction destroyed: the stored callable should be destroyed
            // +1 destruction for the stored callable
        }

        // +1 destruction for the original tracker (moved-from)
        CHECK(c.destructions == c.constructions + c.copies + c.moves);
        CHECK(c.alive() == 0);
    }

    SECTION("Copy construction - lifecycle")
    {
        Counters c;

        {
            LifecycleTracker tracker(c, 20);

            FF ff1(ZA_MOVE(tracker));
            CHECK(ff1() == 20);

            {
                FF ff2(ff1); // NOLINT(performance-unnecessary-copy-initialization)
                CHECK(ff2() == 20);
                CHECK(ff1() == 20); // original still works
            }

            // ff2 destroyed
        }

        // ff1 and tracker destroyed
        CHECK(c.alive() == 0);
        CHECK(c.destructions == c.constructions + c.copies + c.moves);
    }

    SECTION("Copy assignment - lifecycle")
    {
        Counters c;

        {
            LifecycleTracker tracker1(c, 30);
            LifecycleTracker tracker2(c, 40);

            FF ff1(ZA_MOVE(tracker1));
            FF ff2(ZA_MOVE(tracker2));

            CHECK(ff1() == 30);
            CHECK(ff2() == 40);

            ff2 = ff1;
            CHECK(ff2() == 30);
            CHECK(ff1() == 30);
        }

        CHECK(c.alive() == 0);
        CHECK(c.destructions == c.constructions + c.copies + c.moves);
    }

    SECTION("Copy assignment from empty")
    {
        Counters c;

        {
            FF ff1;
            FF ff2(LifecycleTracker(c, 50));

            CHECK(!ff1);
            CHECK(ff2() == 50);

            ff2 = ff1;
            CHECK(!ff2);
        }

        CHECK(c.alive() == 0);
        CHECK(c.destructions == c.constructions + c.copies + c.moves);
    }

    SECTION("Copy assignment to empty")
    {
        Counters c;

        {
            FF ff1(LifecycleTracker(c, 60));
            FF ff2;

            CHECK(ff1() == 60);
            CHECK(!ff2);

            ff2 = ff1;
            CHECK(ff2() == 60);
            CHECK(ff1() == 60);
        }

        CHECK(c.alive() == 0);
        CHECK(c.destructions == c.constructions + c.copies + c.moves);
    }

    SECTION("Self copy assignment")
    {
        Counters c;

        {
            FF ff(LifecycleTracker(c, 70));
            CHECK(ff() == 70);

            const auto& ffRef = ff;
            ff                = ffRef;

            CHECK(ff() == 70);
        }

        CHECK(c.alive() == 0);
        CHECK(c.destructions == c.constructions + c.copies + c.moves);
    }

    SECTION("Move construction - lifecycle")
    {
        Counters c;

        {
            LifecycleTracker tracker(c, 80);

            FF ff1(ZA_MOVE(tracker));
            CHECK(ff1() == 80);

            const int prevMoves = c.moves;
            FF        ff2(ZA_MOVE(ff1));
            CHECK(c.moves == prevMoves + 1);

            CHECK(ff2() == 80);
            CHECK(!ff1); // moved-from should be empty
        }

        CHECK(c.alive() == 0);
        CHECK(c.destructions == c.constructions + c.copies + c.moves);
    }

    SECTION("Move assignment - lifecycle")
    {
        Counters c;

        {
            LifecycleTracker tracker1(c, 90);
            LifecycleTracker tracker2(c, 100);

            FF ff1(ZA_MOVE(tracker1));
            FF ff2(ZA_MOVE(tracker2));

            CHECK(ff1() == 90);
            CHECK(ff2() == 100);

            ff2 = ZA_MOVE(ff1);
            CHECK(ff2() == 90);
            CHECK(!ff1);
        }

        CHECK(c.alive() == 0);
        CHECK(c.destructions == c.constructions + c.copies + c.moves);
    }

    SECTION("Move assignment from empty")
    {
        Counters c;

        {
            FF ff1;
            FF ff2(LifecycleTracker(c, 110));

            CHECK(!ff1);
            CHECK(ff2() == 110);

            ff2 = ZA_MOVE(ff1);
            CHECK(!ff2);
        }

        CHECK(c.alive() == 0);
        CHECK(c.destructions == c.constructions + c.copies + c.moves);
    }

    SECTION("Move assignment to empty")
    {
        Counters c;

        {
            FF ff1(LifecycleTracker(c, 120));
            FF ff2;

            CHECK(ff1() == 120);
            CHECK(!ff2);

            ff2 = ZA_MOVE(ff1);
            CHECK(ff2() == 120);
            CHECK(!ff1);
        }

        CHECK(c.alive() == 0);
        CHECK(c.destructions == c.constructions + c.copies + c.moves);
    }

    SECTION("Self move assignment")
    {
        Counters c;

        {
            FF ff(LifecycleTracker(c, 130));
            CHECK(ff() == 130);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-move"
            ff = ZA_MOVE(ff);
#pragma GCC diagnostic pop

            CHECK(ff() == 130);
        }

        CHECK(c.alive() == 0);
        CHECK(c.destructions == c.constructions + c.copies + c.moves);
    }

    SECTION("Nullptr assignment destroys stored callable")
    {
        Counters c;

        {
            FF ff(LifecycleTracker(c, 140));
            CHECK(!!ff);

            const int prevDestructions = c.destructions;
            ff                         = nullptr;
            CHECK(!ff);
            CHECK(c.destructions == prevDestructions + 1);
        }

        CHECK(c.alive() == 0);
        CHECK(c.destructions == c.constructions + c.copies + c.moves);
    }

    SECTION("Overwriting with new callable")
    {
        Counters c1;
        Counters c2;

        {
            FF ff(LifecycleTracker(c1, 150));
            CHECK(ff() == 150);

            ff = FF(LifecycleTracker(c2, 160));
            CHECK(ff() == 160);
        }

        CHECK(c1.alive() == 0);
        CHECK(c1.destructions == c1.constructions + c1.copies + c1.moves);

        CHECK(c2.alive() == 0);
        CHECK(c2.destructions == c2.constructions + c2.copies + c2.moves);
    }

    SECTION("Multiple return types - void")
    {
        int x = 0;
        FFv ff([&x] { x = 42; });
        ff();
        CHECK(x == 42);
    }

    SECTION("Void return type discards the callable's result")
    {
        int x = 0;
        FFv ff([&x] { return x = 42; });
        ff();
        CHECK(x == 42);
    }

    SECTION("Multiple arguments")
    {
        za::FixedFunction<int(int, int, int), 64> ff([](int a, int b, int c) { return a + b + c; });
        CHECK(ff(1, 2, 3) == 6);
    }

    SECTION("Exact count tracking - move construct FixedFunction from rvalue callable")
    {
        Counters c;

        {
            // Construct tracker: +1 construction
            // Move into FixedFunction: +1 move
            // Tracker destroyed: +1 destruction
            FF ff(LifecycleTracker(c, 200));

            CHECK(c.constructions == 1);
            CHECK(c.moves == 1);
            CHECK(c.copies == 0);
            CHECK(c.destructions == 1); // the temporary is destroyed
            CHECK(c.alive() == 1);      // one alive inside ff
            CHECK(ff() == 200);
        }

        CHECK(c.destructions == 2); // +1 for the one inside ff
        CHECK(c.alive() == 0);
    }

    SECTION("Exact count tracking - copy construct FixedFunction")
    {
        Counters c;

        {
            FF ff1(LifecycleTracker(c, 210));
            // 1 construction, 1 move (from temporary), 1 destruction (temporary)

            const int prevCopies = c.copies;
            FF        ff2(ff1); // NOLINT(performance-unnecessary-copy-initialization)
            CHECK(c.copies == prevCopies + 1);

            CHECK(ff1() == 210);
            CHECK(ff2() == 210);
        }

        CHECK(c.alive() == 0);
    }

    SECTION("Exact count tracking - move construct FixedFunction")
    {
        Counters c;

        {
            FF ff1(LifecycleTracker(c, 220));
            // 1 construction, 1 move (from temporary), 1 destruction (temporary)

            const int prevMoves = c.moves;

            FF ff2(ZA_MOVE(ff1));
            CHECK(c.moves == prevMoves + 1);

            CHECK(ff2() == 220);
            CHECK(!ff1);
        }

        CHECK(c.alive() == 0);
    }

    SECTION("Function pointer - copy and move")
    {
        FF ff1(freeFunction);
        CHECK(ff1() == 42);

        FF ff2(ff1); // NOLINT(performance-unnecessary-copy-initialization)
        CHECK(ff2() == 42);

        FF ff3(ZA_MOVE(ff1));
        CHECK(ff3() == 42);
    }

    SECTION("Chained moves")
    {
        Counters c;

        {
            FF ff1(LifecycleTracker(c, 230));
            FF ff2(ZA_MOVE(ff1));
            FF ff3(ZA_MOVE(ff2));
            FF ff4(ZA_MOVE(ff3));

            CHECK(!ff1);
            CHECK(!ff2);
            CHECK(!ff3);
            CHECK(ff4() == 230);
        }

        CHECK(c.alive() == 0);
        CHECK(c.destructions == c.constructions + c.copies + c.moves);
    }

    SECTION("Chained move assignments")
    {
        Counters c;

        {
            FF ff1(LifecycleTracker(c, 240));
            FF ff2;
            FF ff3;
            FF ff4;

            ff2 = ZA_MOVE(ff1);
            ff3 = ZA_MOVE(ff2);
            ff4 = ZA_MOVE(ff3);

            CHECK(!ff1);
            CHECK(!ff2);
            CHECK(!ff3);
            CHECK(ff4() == 240);
        }

        CHECK(c.alive() == 0);
        CHECK(c.destructions == c.constructions + c.copies + c.moves);
    }

    SECTION("Assign over non-empty with move")
    {
        Counters c1;
        Counters c2;

        {
            FF ff1(LifecycleTracker(c1, 250));
            FF ff2(LifecycleTracker(c2, 260));

            CHECK(ff1() == 250);
            CHECK(ff2() == 260);

            ff2 = ZA_MOVE(ff1);
            CHECK(ff2() == 250);
            CHECK(!ff1);
        }

        CHECK(c1.alive() == 0);
        CHECK(c1.destructions == c1.constructions + c1.copies + c1.moves);

        CHECK(c2.alive() == 0);
        CHECK(c2.destructions == c2.constructions + c2.copies + c2.moves);
    }

    SECTION("Assign over non-empty with copy")
    {
        Counters c1;
        Counters c2;

        {
            FF ff1(LifecycleTracker(c1, 270));
            FF ff2(LifecycleTracker(c2, 280));

            CHECK(ff1() == 270);
            CHECK(ff2() == 280);

            ff2 = ff1;
            CHECK(ff2() == 270);
            CHECK(ff1() == 270);
        }

        CHECK(c1.alive() == 0);
        CHECK(c1.destructions == c1.constructions + c1.copies + c1.moves);

        CHECK(c2.alive() == 0);
        CHECK(c2.destructions == c2.constructions + c2.copies + c2.moves);
    }
}


////////////////////////////////////////////////////////////
// Free function used by the const-call test below.
////////////////////////////////////////////////////////////
[[nodiscard]] int squareInt(const int x)
{
    return x * x;
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] za::FixedFunction - const call operator")
{
    using FFi = za::FixedFunction<int(int), 64>;

    SECTION("Const call dispatches to free function pointer")
    {
        const FFi f{&squareInt};
        CHECK(f(7) == 49);

        // Non-const call should also work via the delegating overload.
        FFi g{&squareInt};
        CHECK(g(8) == 64);
    }

    SECTION("Const call dispatches to a const-callable closure")
    {
        int       captured = 10;
        const FFi f{[captured](const int x) { return x + captured; }};

        // Const lambdas have a `const operator()` -- safe to call
        // through `const FFi&` with no logical-const violation.
        CHECK(f(5) == 15);

        // Repeated calls return the same value (no hidden mutation).
        CHECK(f(5) == 15);
        CHECK(f(0) == 10);
    }

    SECTION("Both overloads forward arguments and return value identically")
    {
        FFi       g{[](const int x) { return x + 1; }};
        const FFi f{[](const int x) { return x + 1; }};

        CHECK(f(3) == 4);
        CHECK(g(3) == 4);
        CHECK(f(-1) == 0);
        CHECK(g(-1) == 0);
    }

    SECTION("Const call on a wrapped mutable lambda still runs (matches std::function)")
    {
        // A `mutable` lambda has a non-`const operator()`. Calling it
        // through a const FixedFunction is the documented "logical
        // const violation" case (matches std::function::operator()
        // const). The call still works; the captured state mutates.
        const FFi f{[count = 0](const int x) mutable { return ++count + x; }};
        CHECK(f(0) == 1);
        CHECK(f(0) == 2);
        CHECK(f(0) == 3);
    }

    SECTION("Const call from a copied FixedFunction")
    {
        // After copy-construction, the destination is independently
        // callable through const.
        const FFi original{&squareInt};
        const FFi copy(original); // NOLINT(performance-unnecessary-copy-initialization)

        CHECK(original(4) == 16);
        CHECK(copy(4) == 16);
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] za::FixedFunction - moved-from is empty regardless of payload kind")
{
    // Ensures the move ops uniformly null `m_methodPtr` -- previously
    // a moved-from free-function FixedFunction was still callable,
    // inconsistent with the stored-callable case.
    using FFi = za::FixedFunction<int(int), 64>;

    SECTION("Move-construct from free-function FixedFunction")
    {
        FFi source{&squareInt};
        FFi dest{static_cast<FFi&&>(source)};

        CHECK(static_cast<bool>(dest));
        CHECK(!static_cast<bool>(source));
        CHECK(dest(6) == 36);
    }

    SECTION("Move-assign from free-function FixedFunction")
    {
        FFi source{&squareInt};
        FFi dest{[](int) { return 0; }};
        dest = static_cast<FFi&&>(source);

        CHECK(static_cast<bool>(dest));
        CHECK(!static_cast<bool>(source));
        CHECK(dest(7) == 49);
    }
}

} // namespace FixedFunctionTest
} // namespace
