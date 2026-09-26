#include "Tst/Tst.hpp"

#include "Zancle/Math/Clamp.hpp"


namespace
{
////////////////////////////////////////////////////////////
// Only provides `operator<`, which is all that `za::clamp` requires (like `std::clamp`)
struct Wrapper
{
    int value;

    [[nodiscard]] constexpr bool operator<(const Wrapper& rhs) const
    {
        return value < rhs.value;
    }
};

} // namespace


TEST_CASE("[Base] Base/Clamp.hpp")
{
    SECTION("Clamp")
    {
        CHECK(za::clamp(5, 0, 10) == 5);
        CHECK(za::clamp(15, 0, 10) == 10);
        CHECK(za::clamp(-15, 0, 10) == 0);

        STATIC_CHECK(za::clamp(0.5f, 0.f, 1.f) == 0.5f);
        STATIC_CHECK(za::clamp(1.5f, 0.f, 1.f) == 1.f);
        STATIC_CHECK(za::clamp(-1.5f, 0.f, 1.f) == 0.f);
    }

    SECTION("Clamp (class types, by reference)")
    {
        const Wrapper lo{0};
        const Wrapper hi{10};

        const Wrapper inRange{5};
        const Wrapper below{-5};
        const Wrapper above{15};

        CHECK(&za::clamp(inRange, lo, hi) == &inRange);
        CHECK(&za::clamp(below, lo, hi) == &lo);
        CHECK(&za::clamp(above, lo, hi) == &hi);
    }
}
