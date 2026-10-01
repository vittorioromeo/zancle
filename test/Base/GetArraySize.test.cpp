#include "Tst/Tst.hpp"

#include "Zancle/Base/GetArraySize.hpp"

#include "Zancle/Base/SizeT.hpp"


namespace
{
namespace GetArraySizeTest // for unity builds
{
////////////////////////////////////////////////////////////
struct WithArrayMember
{
    float values[5];
};


////////////////////////////////////////////////////////////
// An array reached through a reference is not usable in a constant expression (unlike `values` below)
[[nodiscard]] za::SizeT sizeThroughReference(const int (&array)[3])
{
    return za::getArraySize(array);
}

} // namespace GetArraySizeTest
} // namespace


TEST_CASE("[Base] Base/GetArraySize.hpp")
{
    SECTION("Get Array Size")
    {
        const int values[]{0, 1, 2, 3, 4, 5, 6, 7};
        CHECK(za::getArraySize(values) == 8);
        STATIC_CHECK(za::getArraySize(values) == 8);
    }

    SECTION("Array reached through a reference")
    {
        const int values[]{1, 2, 3};
        CHECK(GetArraySizeTest::sizeThroughReference(values) == 3);
    }

    SECTION("Array member")
    {
        STATIC_CHECK(za::getArraySize(&GetArraySizeTest::WithArrayMember::values) == 5);
    }
}
