#include "Tst/Tst.hpp"

#include "Zancle/Vocabulary/EnumClassBitwiseOps.hpp"


namespace
{
namespace EnumClassBitwiseOpsTest // for unity builds
{
////////////////////////////////////////////////////////////
enum class Flags : unsigned char
{
    None = 0u,
    A    = 1u << 0u,
    B    = 1u << 1u,
    C    = 1u << 2u,
};

ZA_DEFINE_ENUM_CLASS_BITWISE_OPS(Flags);

} // namespace EnumClassBitwiseOpsTest
} // namespace


TEST_CASE("[Base] Base/EnumClassBitwiseOps.hpp")
{
    using namespace EnumClassBitwiseOpsTest;

    SECTION("Binary and unary operators")
    {
        STATIC_CHECK((Flags::A | Flags::B) == static_cast<Flags>(3u));
        STATIC_CHECK(((Flags::A | Flags::B) & Flags::B) == Flags::B);
        STATIC_CHECK(((Flags::A | Flags::B) ^ Flags::A) == Flags::B);
        STATIC_CHECK((~Flags::None & Flags::C) == Flags::C);
        STATIC_CHECK(!Flags::None);
        STATIC_CHECK(!!Flags::A);
    }

    SECTION("Compound assignment operators")
    {
        Flags f = Flags::None;

        f |= Flags::A | Flags::C;
        CHECK(f == (Flags::A | Flags::C));

        f &= Flags::C;
        CHECK(f == Flags::C);

        f ^= Flags::B | Flags::C;
        CHECK(f == Flags::B);

        f ^= Flags::B;
        CHECK(!f);
    }
}
