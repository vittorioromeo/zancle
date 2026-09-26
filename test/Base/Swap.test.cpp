#include "Tst/Tst.hpp"

#include "Zancle/Base/Swap.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Trait/DeclVal.hpp"


namespace
{

struct MoveOnlySwappable
{
    int value{0};

    MoveOnlySwappable() = default;

    explicit MoveOnlySwappable(int v) : value(v)
    {
    }

    MoveOnlySwappable(const MoveOnlySwappable&)            = delete;
    MoveOnlySwappable& operator=(const MoveOnlySwappable&) = delete;

    MoveOnlySwappable(MoveOnlySwappable&& other) noexcept : value(other.value)
    {
        other.value = -1;
    }

    MoveOnlySwappable& operator=(MoveOnlySwappable&& other) noexcept
    {
        value       = other.value;
        other.value = -1;

        return *this;
    }
};

struct TestMemberSwap
{
    int swapped = 0;

    void swap(TestMemberSwap& other) noexcept
    {
        swapped       = 1;
        other.swapped = 2;
    }
};

struct TestHiddenFriend
{
    int swapped = 0;

    friend void swap(TestHiddenFriend& lhs, TestHiddenFriend& rhs) noexcept
    {
        lhs.swapped = 1;
        rhs.swapped = 2;
    }
};

namespace TestADL
{

struct TestADLStruct
{
    int swapped = 0;
};

void swap(TestADLStruct& lhs, TestADLStruct& rhs) noexcept
{
    lhs.swapped = 1;
    rhs.swapped = 2;
}

} // namespace TestADL

namespace TestAmbiguity
{

struct AmbiguousStruct
{
    int value{0};
};

// This mimics `std::swap` by being an unconstrained generic template.
// During ADL, it will collide with `priv::swap_adl::swap(T&, T&) = delete;`.
template <typename T>
void swap(T& lhs, T& rhs) noexcept
{
    lhs.value = 999;
    rhs.value = 999;
}

} // namespace TestAmbiguity

// Only the `throw`s are gated: the types stay potentially-throwing for the `noexcept` checks below
struct ThrowingMemberSwap
{
    void swap(ThrowingMemberSwap&)
    {
#ifdef __cpp_exceptions
        throw 42;
#endif
    }
};

struct ThrowingMove
{
    ThrowingMove() = default;

    ThrowingMove(ThrowingMove&&)
    {
#ifdef __cpp_exceptions
        throw 42;
#endif
    }

    ThrowingMove& operator=(ThrowingMove&&)
    {
        return *this;
    }
};

template <typename T>
inline constexpr bool isGenericSwapNoexcept = noexcept(za::genericSwap(za::declVal<T&>(), za::declVal<T&>()));

// `genericSwap` is `noexcept` exactly when the selected operation is
static_assert(isGenericSwapNoexcept<int>);
static_assert(isGenericSwapNoexcept<int[2][2]>);
static_assert(isGenericSwapNoexcept<za::Vector<int>>);
static_assert(isGenericSwapNoexcept<TestMemberSwap>);
static_assert(isGenericSwapNoexcept<TestHiddenFriend>);
static_assert(isGenericSwapNoexcept<TestADL::TestADLStruct>);
static_assert(isGenericSwapNoexcept<TestAmbiguity::AmbiguousStruct>);
static_assert(isGenericSwapNoexcept<MoveOnlySwappable>);
static_assert(!isGenericSwapNoexcept<ThrowingMemberSwap>);
static_assert(!isGenericSwapNoexcept<ThrowingMove>);
static_assert(!isGenericSwapNoexcept<ThrowingMove[2][2]>);


TEST_CASE("[Base] Base/Swap.hpp")
{
    SECTION("Swap")
    {
        int a = 10;
        int b = 20;
        za::genericSwap(a, b);
        CHECK(a == 20);
        CHECK(b == 10);

        za::Vector<int> v1{1, 2};
        za::Vector<int> v2{3, 4, 5};
        za::genericSwap(v1, v2);
        CHECK((v1 == za::Vector<int>{3, 4, 5}));
        CHECK((v2 == za::Vector<int>{1, 2}));
    }

    SECTION("IterSwap")
    {
        int values[] = {10, 20, 30, 40};
        za::iterSwap(values, values + 2); // Swap values[0] and values[2]
        CHECK(values[0] == 30);
        CHECK(values[1] == 20);
        CHECK(values[2] == 10);
        CHECK(values[3] == 40);

        za::Vector<int> v{5, 15, 25};
        za::iterSwap(v.begin(), v.begin() + 1);
        CHECK(v[0] == 15);
        CHECK(v[1] == 5);
        CHECK(v[2] == 25);
    }

    SECTION("SwapRanges")
    {
        int arr1[] = {1, 2, 3, 4, 5};
        int arr2[] = {10, 20, 30, 40, 50};

        // Swap the first 3 elements
        auto* resultIter = za::swapRanges(arr1, arr1 + 3, arr2);

        CHECK(resultIter == arr2 + 3);

        CHECK(arr1[0] == 10);
        CHECK(arr1[1] == 20);
        CHECK(arr1[2] == 30);
        CHECK(arr1[3] == 4); // Unchanged
        CHECK(arr1[4] == 5); // Unchanged

        CHECK(arr2[0] == 1);
        CHECK(arr2[1] == 2);
        CHECK(arr2[2] == 3);
        CHECK(arr2[3] == 40); // Unchanged
        CHECK(arr2[4] == 50); // Unchanged

        za::Vector<int> v1{100, 200, 300, 400};
        za::Vector<int> v2{500, 600, 700, 800};

        // Swap the middle 2 elements
        resultIter = za::swapRanges(v1.begin() + 1, v1.begin() + 3, v2.begin() + 1);

        CHECK((resultIter == (v2.begin() + 3)));

        CHECK((v1 == za::Vector<int>{100, 600, 700, 400}));
        CHECK((v2 == za::Vector<int>{500, 200, 300, 800}));
    }

    SECTION("Member swap")
    {
        TestMemberSwap a;
        TestMemberSwap b;

        za::genericSwap(a, b);

        CHECK(a.swapped == 1);
        CHECK(b.swapped == 2);
    }

    SECTION("Hidden friend, explicit namespace")
    {
        TestHiddenFriend a;
        TestHiddenFriend b;

        za::genericSwap(a, b);

        CHECK(a.swapped == 1);
        CHECK(b.swapped == 2);
    }

    SECTION("Hidden friend, ADL")
    {
        TestHiddenFriend a;
        TestHiddenFriend b;

        swap(a, b);

        CHECK(a.swapped == 1);
        CHECK(b.swapped == 2);
    }

    SECTION("ADL")
    {
        TestADL::TestADLStruct a;
        TestADL::TestADLStruct b;

        swap(a, b);

        CHECK(a.swapped == 1);
        CHECK(b.swapped == 2);
    }

    SECTION("Poison Pill / Generic Template Ambiguity Fallback")
    {
        TestAmbiguity::AmbiguousStruct a{10};
        TestAmbiguity::AmbiguousStruct b{20};

        // If the Poison Pill succeeds, `HasCustomSwap` gracefully evaluates to false,
        // bypasses the 999-assigning template entirely, and manually swaps them!
        za::genericSwap(a, b);

        CHECK(a.value == 20);
        CHECK(b.value == 10);

        // Ensure the dummy `std::swap`-like function was NOT called
        CHECK(a.value != 999);
        CHECK(b.value != 999);
    }

    SECTION("Move-Only types fallback")
    {
        MoveOnlySwappable a{100};
        MoveOnlySwappable b{200};

        // Ensures `static_cast<T&&>` is correctly applied in the fallback
        za::genericSwap(a, b);

        CHECK(a.value == 200);
        CHECK(b.value == 100);
    }

    SECTION("Raw 1D Arrays")
    {
        int arr1[3] = {1, 2, 3};
        int arr2[3] = {4, 5, 6};

        // Proves the `T (&a)[N]` overload correctly triggers
        za::genericSwap(arr1, arr2);

        CHECK(arr1[0] == 4);
        CHECK(arr1[1] == 5);
        CHECK(arr1[2] == 6);

        CHECK(arr2[0] == 1);
        CHECK(arr2[1] == 2);
        CHECK(arr2[2] == 3);
    }

    SECTION("Multidimensional Arrays")
    {
        int arr1[2][2] = {{1, 2}, {3, 4}};
        int arr2[2][2] = {{5, 6}, {7, 8}};

        // Proves the array overload safely recurses into itself!
        za::genericSwap(arr1, arr2);

        CHECK(arr1[0][0] == 5);
        CHECK(arr1[0][1] == 6);
        CHECK(arr1[1][0] == 7);
        CHECK(arr1[1][1] == 8);

        CHECK(arr2[0][0] == 1);
        CHECK(arr2[0][1] == 2);
        CHECK(arr2[1][0] == 3);
        CHECK(arr2[1][1] == 4);
    }

#ifdef __cpp_exceptions
    SECTION("Exceptions propagate instead of terminating")
    {
        const auto throwsInt = [](auto& a, auto& b)
        {
            try
            {
                za::genericSwap(a, b);
            } catch (const int e)
            {
                return e == 42;
            }

            return false;
        };

        ThrowingMemberSwap tms0;
        ThrowingMemberSwap tms1;
        CHECK(throwsInt(tms0, tms1));

        ThrowingMove tm0;
        ThrowingMove tm1;
        CHECK(throwsInt(tm0, tm1));
    }
#endif
}

} // namespace
