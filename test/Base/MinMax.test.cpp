#include "Tst/Tst.hpp"

#include "Zancle/Math/MinMax.hpp"

#include "Zancle/Base/Signbit.hpp"


namespace
{
////////////////////////////////////////////////////////////
struct Wrapper
{
    int value;

    [[nodiscard]] constexpr bool operator<(const Wrapper& rhs) const
    {
        return value < rhs.value;
    }
};

} // namespace


TEST_CASE("[Base] Base/MinMax.hpp")
{
    SECTION("Min/Max (non-class types, by value)")
    {
        STATIC_CHECK(za::min(10, -10) == -10);
        STATIC_CHECK(za::max(10, -10) == 10);
        STATIC_CHECK(za::min(1.5f, 2.5f) == 1.5f);
        STATIC_CHECK(za::max(1.5f, 2.5f) == 2.5f);

        // Equivalent values: both return the first argument (like `std::min`/`std::max`)
        STATIC_CHECK(ZA_SIGNBIT(za::min(-0.f, 0.f)));
        STATIC_CHECK(!ZA_SIGNBIT(za::min(0.f, -0.f)));
        STATIC_CHECK(ZA_SIGNBIT(za::max(-0.f, 0.f)));
        STATIC_CHECK(!ZA_SIGNBIT(za::max(0.f, -0.f)));

        const int  arr[2]{};
        const int* p0 = arr;
        const int* p1 = arr + 1;
        CHECK(za::min(p0, p1) == p0);
        CHECK(za::max(p0, p1) == p1);
    }

    SECTION("Min/Max (class types, by reference)")
    {
        const Wrapper a{10};
        const Wrapper b{-10};

        CHECK(&za::min(a, b) == &b);
        CHECK(&za::max(a, b) == &a);

        const Wrapper c{10};

        CHECK(&za::min(a, c) == &a);
        CHECK(&za::max(a, c) == &a);
    }
}
