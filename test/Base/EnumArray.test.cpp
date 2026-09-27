#include "Tst/Tst.hpp"

#include "Zancle/Container/EnumArray.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/SizeT.hpp"

#include "Zancle/Trait/IsAggregate.hpp"
#include "Zancle/Trait/IsSame.hpp"
#include "Zancle/Trait/IsTriviallyCopyable.hpp"
#include "Zancle/Trait/IsTriviallyRelocatable.hpp"


namespace
{
namespace EnumArrayTest // for unity builds
{
////////////////////////////////////////////////////////////
enum class Color : za::U8
{
    Red,
    Green,
    Blue,

    Count
};


////////////////////////////////////////////////////////////
enum class Wide : za::U64
{
    A,
    B
};


////////////////////////////////////////////////////////////
enum Unscoped : int
{
    UnscopedA,
    UnscopedB
};


////////////////////////////////////////////////////////////
struct NonRelocatable
{
    NonRelocatable() = default;

    NonRelocatable(const NonRelocatable& rhs) : value(rhs.value)
    {
    }

    NonRelocatable& operator=(const NonRelocatable&) = default;

    // NOLINTNEXTLINE(modernize-use-equals-default)
    ~NonRelocatable()
    {
    }

    int value{};
};


////////////////////////////////////////////////////////////
using ColorArray = za::EnumArray<Color, int, static_cast<za::SizeT>(Color::Count)>;


////////////////////////////////////////////////////////////
// Aggregate and trivial relocation propagation.
////////////////////////////////////////////////////////////
static_assert(ZA_IS_AGGREGATE(ColorArray));
static_assert(ZA_IS_TRIVIALLY_COPYABLE(ColorArray));
static_assert(ZA_IS_TRIVIALLY_RELOCATABLE(ColorArray));

static_assert(!ZA_IS_TRIVIALLY_RELOCATABLE(NonRelocatable));
static_assert(ZA_IS_AGGREGATE(za::EnumArray<Color, NonRelocatable, 3>));
static_assert(!ZA_IS_TRIVIALLY_COPYABLE(za::EnumArray<Color, NonRelocatable, 3>));
static_assert(!ZA_IS_TRIVIALLY_RELOCATABLE(za::EnumArray<Color, NonRelocatable, 3>));


////////////////////////////////////////////////////////////
// Layout: exactly `Count` values.
////////////////////////////////////////////////////////////
static_assert(sizeof(ColorArray) == 3 * sizeof(int));
static_assert(sizeof(ColorArray{}.elements) / sizeof(int) == 3u);


////////////////////////////////////////////////////////////
// Usable in constant expressions.
////////////////////////////////////////////////////////////
static_assert([]
{
    ColorArray a{1, 2, 3};
    a[Color::Green] = 20;

    return a[Color::Red] == 1 && a[Color::Green] == 20 && a[Color::Blue] == 3;
}());

static_assert([]
{
    za::EnumArray<Wide, int, 2> a{};
    a.fill(7);
    a[Wide::B] = 8;

    return a[Wide::A] == 7 && a[Wide::B] == 8;
}());

} // namespace EnumArrayTest
} // namespace


TEST_CASE("[Base] Container/EnumArray.hpp")
{
    using namespace EnumArrayTest;

    SECTION("Aggregate initialization")
    {
        const ColorArray a{10, 20, 30};

        CHECK(a.elements[0] == 10);
        CHECK(a.elements[1] == 20);
        CHECK(a.elements[2] == 30);

        const ColorArray b{}; // value-initialized
        CHECK(b.elements[0] == 0);
        CHECK(b.elements[1] == 0);
        CHECK(b.elements[2] == 0);

        const ColorArray c{5}; // remaining elements value-initialized
        CHECK(c.elements[0] == 5);
        CHECK(c.elements[1] == 0);
        CHECK(c.elements[2] == 0);
    }

    SECTION("operator[] (non-const)")
    {
        ColorArray a{};

        a[Color::Red]   = 1;
        a[Color::Green] = 2;
        a[Color::Blue]  = 3;

        CHECK(a.elements[0] == 1);
        CHECK(a.elements[1] == 2);
        CHECK(a.elements[2] == 3);

        STATIC_CHECK(za::isSame<decltype(a[Color::Red]), int&>);
        CHECK(&a[Color::Blue] == &a.elements[2]);
    }

    SECTION("operator[] (const)")
    {
        const ColorArray a{4, 5, 6};

        CHECK(a[Color::Red] == 4);
        CHECK(a[Color::Green] == 5);
        CHECK(a[Color::Blue] == 6);

        STATIC_CHECK(za::isSame<decltype(a[Color::Red]), const int&>);
        CHECK(&a[Color::Green] == &a.elements[1]);
    }

    SECTION("Wide and unscoped underlying types")
    {
        za::EnumArray<Wide, char, 2> w{'a', 'b'};
        CHECK(w[Wide::A] == 'a');
        CHECK(w[Wide::B] == 'b');

        za::EnumArray<Unscoped, float, 2> u{1.5f, 2.5f};
        CHECK(u[UnscopedA] == 1.5f);
        CHECK(u[UnscopedB] == 2.5f);
    }

    SECTION("fill")
    {
        ColorArray a{1, 2, 3};
        a.fill(9);

        CHECK(a[Color::Red] == 9);
        CHECK(a[Color::Green] == 9);
        CHECK(a[Color::Blue] == 9);
    }

    SECTION("Range-for over elements")
    {
        ColorArray a{1, 2, 3};

        int sum = 0;
        for (const int x : a.elements)
            sum += x;

        CHECK(sum == 6);
    }

    SECTION("Non-trivially-relocatable values")
    {
        za::EnumArray<Color, NonRelocatable, 3> a{};
        a[Color::Blue].value = 42;

        const auto copy = a;
        CHECK(copy[Color::Red].value == 0);
        CHECK(copy[Color::Blue].value == 42);
    }
}
