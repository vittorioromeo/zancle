#include "Tst/Tst.hpp"

#include "Zancle/Vocabulary/FunctionRef.hpp"

#include "Zancle/Base/Macros.hpp"

#include "Zancle/Trait/IsConvertible.hpp"


namespace
{
namespace FunctionRefTest // to support unity builds
{

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


////////////////////////////////////////////////////////////
int freeFunctionSum(int a, int b, int c)
{
    return a + b + c;
}


////////////////////////////////////////////////////////////
void freeFunctionVoid(int& out)
{
    out = 777;
}


////////////////////////////////////////////////////////////
long lastLong = 0;

void freeFunctionTakingLong(const long x)
{
    lastLong = x;
}


////////////////////////////////////////////////////////////
int globalValue = 7;

int& freeFunctionReturningRef()
{
    return globalValue;
}


////////////////////////////////////////////////////////////
[[gnu::noinline]] void clobberStack()
{
    volatile char buffer[256];

    for (volatile char& c : buffer)
        c = static_cast<char>(0xAB);
}


////////////////////////////////////////////////////////////
struct CallableConst
{
    int value;

    int operator()() const
    {
        return value;
    }
};


////////////////////////////////////////////////////////////
struct CallableMutable
{
    int counter = 0;

    int operator()()
    {
        return ++counter;
    }
};


////////////////////////////////////////////////////////////
struct CallableWithArgs
{
    int multiplier;

    int operator()(int a, int b) const
    {
        return (a + b) * multiplier;
    }
};


////////////////////////////////////////////////////////////
int takesFunctionRef(za::FunctionRef<int()> f)
{
    return f();
}


////////////////////////////////////////////////////////////
int takesFunctionRefWithArg(za::FunctionRef<int(int)> f, int x)
{
    return f(x);
}


////////////////////////////////////////////////////////////
int sumViaRef(za::FunctionRef<int(int, int)> f)
{
    return f(3, 4) + f(5, 6);
}

TEST_CASE("[Base] Base/FunctionRef.hpp")
{
    SECTION("Construction from free function pointer")
    {
        za::FunctionRef<int()> fr(freeFunction);
        CHECK(fr() == 42);
    }

    SECTION("Construction from free function with argument")
    {
        za::FunctionRef<int(int)> fr(freeFunctionWithArg);
        CHECK(fr(5) == 10);
        CHECK(fr(-3) == -6);
    }

    SECTION("Construction from free function with multiple arguments")
    {
        za::FunctionRef<int(int, int, int)> fr(freeFunctionSum);
        CHECK(fr(1, 2, 3) == 6);
        CHECK(fr(10, 20, 30) == 60);
    }

    SECTION("Construction from stateless lambda")
    {
        auto                   lam = [] { return 99; };
        za::FunctionRef<int()> fr(lam);
        CHECK(fr() == 99);
    }

    SECTION("Construction from stateful lambda (capture by value)")
    {
        int                    x   = 7;
        auto                   lam = [x] { return x * 3; };
        za::FunctionRef<int()> fr(lam);
        CHECK(fr() == 21);
    }

    SECTION("Construction from stateful lambda (capture by reference)")
    {
        int  x   = 10;
        auto lam = [&x]
        {
            x += 5;
            return x;
        };
        za::FunctionRef<int()> fr(lam);
        CHECK(fr() == 15);
        CHECK(fr() == 20);
        CHECK(x == 20);
    }

    SECTION("Construction from const functor")
    {
        const CallableConst    cc{123};
        za::FunctionRef<int()> fr(cc);
        CHECK(fr() == 123);
    }

    SECTION("Construction from mutable functor")
    {
        CallableMutable        cm;
        za::FunctionRef<int()> fr(cm);
        CHECK(fr() == 1);
        CHECK(fr() == 2);
        CHECK(fr() == 3);
        CHECK(cm.counter == 3);
    }

    SECTION("Construction from functor with arguments")
    {
        const CallableWithArgs         cwa{3};
        za::FunctionRef<int(int, int)> fr(cwa);
        CHECK(fr(2, 3) == 15);
        CHECK(fr(10, 0) == 30);
    }

    SECTION("Use as function parameter - lambda rvalue")
    {
        CHECK(takesFunctionRef([] { return 1000; }) == 1000);
    }

    SECTION("Use as function parameter - free function")
    {
        CHECK(takesFunctionRef(freeFunction) == 42);
    }

    SECTION("Use as function parameter - stateful lambda rvalue")
    {
        int y = 4;
        CHECK(takesFunctionRefWithArg([y](int x) { return x * y; }, 9) == 36);
    }

    SECTION("Use as function parameter - functor rvalue")
    {
        CHECK(takesFunctionRef(CallableConst{77}) == 77);
    }

    SECTION("Use as function parameter - multiple invocations")
    {
        // (3+4)*2 + (5+6)*2 = 14 + 22 = 36
        CHECK(sumViaRef(CallableWithArgs{2}) == 36);
    }

    SECTION("Use as function parameter - lambda with reference capture")
    {
        int sum = 0;
        takesFunctionRefWithArg(
            [&sum](int x)
        {
            sum += x;
            return sum;
        },
            10);
        takesFunctionRefWithArg(
            [&sum](int x)
        {
            sum += x;
            return sum;
        },
            20);
        CHECK(sum == 30);
    }

    SECTION("Void return type")
    {
        int                     out = 0;
        auto                    lam = [&out] { out = 42; };
        za::FunctionRef<void()> fr(lam);
        fr();
        CHECK(out == 42);
    }

    SECTION("Void return type discards the callable's result")
    {
        int                     out = 0;
        auto                    lam = [&out] { return out = 42; };
        za::FunctionRef<void()> fr(lam);
        fr();
        CHECK(out == 42);
    }

    SECTION("Void return type - free function")
    {
        int                         out = 0;
        za::FunctionRef<void(int&)> fr(freeFunctionVoid);
        fr(out);
        CHECK(out == 777);
    }

    SECTION("Forward by lvalue reference argument")
    {
        int  value = 5;
        auto lam   = [](int& v) { v *= 3; };

        za::FunctionRef<void(int&)> fr(lam);
        fr(value);
        CHECK(value == 15);
    }

    SECTION("Forward by rvalue reference argument")
    {
        struct Move
        {
            int v;
            Move(int x) : v(x)
            {
            }
            Move(const Move&)            = delete;
            Move& operator=(const Move&) = delete;
            Move(Move&& other) noexcept : v(other.v)
            {
                other.v = -1;
            }
            Move& operator=(Move&& other) noexcept
            {
                v       = other.v;
                other.v = -1;
                return *this;
            }
        };

        auto                         lam = [](Move&& m) { return m.v; };
        za::FunctionRef<int(Move&&)> fr(lam);

        Move m(88);
        CHECK(fr(ZA_MOVE(m)) == 88);
    }

    SECTION("Copyable and trivially small")
    {
        za::FunctionRef<int()> fr1(freeFunction);
        za::FunctionRef<int()> fr2 = fr1; // copy
        CHECK(fr2() == 42);

        za::FunctionRef<int()> fr3(fr1); // copy-construct
        CHECK(fr3() == 42);

        // FunctionRef should fit in two pointers (obj + thunk)
        static_assert(sizeof(za::FunctionRef<int()>) <= 2 * sizeof(void*));
    }

    SECTION("Can rebind via assignment")
    {
        auto lam1 = [] { return 1; };
        auto lam2 = [] { return 2; };

        za::FunctionRef<int()> fr(lam1);
        CHECK(fr() == 1);

        fr = lam2;
        CHECK(fr() == 2);

        fr = freeFunction;
        CHECK(fr() == 42);
    }

    SECTION("Different lambdas with same signature")
    {
        auto a = [] { return 11; };
        auto b = [] { return 22; };

        za::FunctionRef<int()> fa(a);
        za::FunctionRef<int()> fb(b);

        CHECK(fa() == 11);
        CHECK(fb() == 22);
    }

    SECTION("Referenced object is not copied")
    {
        struct Tracker
        {
            int* copies;
            int  value;

            Tracker(int* c, int v) : copies(c), value(v)
            {
            }

            Tracker(const Tracker& other) : copies(other.copies), value(other.value)
            {
                ++*copies;
            }

            Tracker(Tracker&&)                 = delete;
            Tracker& operator=(const Tracker&) = delete;
            Tracker& operator=(Tracker&&)      = delete;

            int operator()() const
            {
                return value;
            }
        };

        int     copies = 0;
        Tracker t(&copies, 321);

        za::FunctionRef<int()> fr(t);
        CHECK(fr() == 321);
        CHECK(copies == 0);

        // Multiple invocations still don't copy
        (void)fr();
        (void)fr();
        CHECK(copies == 0);
    }

    SECTION("Pass non-copyable callable as parameter")
    {
        struct NonCopyable
        {
            int value;

            NonCopyable(int v) : value(v)
            {
            }

            NonCopyable(const NonCopyable&)            = delete;
            NonCopyable& operator=(const NonCopyable&) = delete;

            int operator()() const
            {
                return value;
            }
        };

        NonCopyable nc(555);
        CHECK(takesFunctionRef(nc) == 555);
    }

    SECTION("Return by reference")
    {
        int  x   = 0;
        auto lam = [&x]() -> int& { return x; };

        za::FunctionRef<int&()> fr(lam);
        fr() = 99;
        CHECK(x == 99);
    }

    SECTION("Same lambda called many times with changing capture")
    {
        int  accumulator = 0;
        auto lam         = [&accumulator](int v)
        {
            accumulator += v;
            return accumulator;
        };

        za::FunctionRef<int(int)> fr(lam);
        CHECK(fr(1) == 1);
        CHECK(fr(2) == 3);
        CHECK(fr(7) == 10);
        CHECK(accumulator == 10);
    }
}


////////////////////////////////////////////////////////////
[[nodiscard]] int overloadedOnSignature(za::FunctionRef<int(int)> f)
{
    return f(1);
}


////////////////////////////////////////////////////////////
[[nodiscard]] int overloadedOnSignature(za::FunctionRef<int(const char*)> f)
{
    return f("abc") + 100;
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] za::FunctionRef - construction is constrained to matching callables")
{
    SECTION("Non-callables and mismatched signatures are rejected")
    {
        auto takesInt = [](int x) { return x; };

        STATIC_CHECK(ZA_IS_CONVERTIBLE(decltype(takesInt), za::FunctionRef<int(int)>));
        STATIC_CHECK(ZA_IS_CONVERTIBLE(decltype(takesInt), za::FunctionRef<void(int)>));
        STATIC_CHECK(!ZA_IS_CONVERTIBLE(int, za::FunctionRef<int(int)>));
        STATIC_CHECK(!ZA_IS_CONVERTIBLE(decltype(takesInt), za::FunctionRef<int(const char*)>));
        STATIC_CHECK(!ZA_IS_CONVERTIBLE(decltype([] {}), za::FunctionRef<int()>)); // `void` result
    }

    SECTION("Overloads on different signatures are not ambiguous")
    {
        CHECK(overloadedOnSignature([](int x) { return x + 1; }) == 2);
        CHECK(overloadedOnSignature([](const char* s) { return static_cast<int>(s[0]); }) == 'a' + 100);
    }

    SECTION("Function pointer with a compatible signature is stored by value")
    {
        // Regression: used to reference the (destroyed) temporary function pointer
        const za::FunctionRef<void(int)> f{&freeFunctionTakingLong};
        clobberStack();

        f(5);
        CHECK(lastLong == 5);
    }

    SECTION("Function lvalues with a compatible signature")
    {
        const za::FunctionRef<void(int)> f{freeFunctionTakingLong};
        f(6);
        CHECK(lastLong == 6);

        const za::FunctionRef<void(int)> g{freeFunctionWithArg}; // result discarded
        g(1);

        const za::FunctionRef<long(int)> h{freeFunctionWithArg};
        CHECK(h(1) == freeFunctionWithArg(1));
    }

    SECTION("Reference results must not bind to temporaries")
    {
        // A by-value result would dangle once returned as a reference
        STATIC_CHECK(!ZA_IS_CONVERTIBLE(decltype([] { return 0; }), za::FunctionRef<const int&()>));
        STATIC_CHECK(!ZA_IS_CONVERTIBLE(decltype(&freeFunction), za::FunctionRef<const int&()>));

        const za::FunctionRef<const int&()> f{&freeFunctionReturningRef};
        CHECK(&f() == &globalValue);
    }
}

} // namespace FunctionRefTest
} // namespace
