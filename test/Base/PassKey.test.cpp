#include "Tst/Tst.hpp"

#include "Zancle/Vocabulary/PassKey.hpp"

#include "Zancle/Trait/IsAggregate.hpp"
#include "Zancle/Trait/IsConstructible.hpp"


namespace
{
namespace PassKeyTest // for unity builds
{
////////////////////////////////////////////////////////////
struct Owner
{
    [[nodiscard]] static int callRestricted();
};


////////////////////////////////////////////////////////////
[[nodiscard]] int restricted(const za::PassKey<Owner>&)
{
    return 42;
}


////////////////////////////////////////////////////////////
int Owner::callRestricted()
{
    return restricted(za::PassKey<Owner>{});
}

} // namespace PassKeyTest
} // namespace


TEST_CASE("[Base] Base/PassKey.hpp")
{
    using namespace PassKeyTest;

    SECTION("Only the befriended type can construct a key")
    {
        STATIC_CHECK(!ZA_IS_AGGREGATE(za::PassKey<Owner>));
        STATIC_CHECK(!ZA_IS_CONSTRUCTIBLE(za::PassKey<Owner>));
        STATIC_CHECK(!ZA_IS_CONSTRUCTIBLE(za::PassKey<Owner>, const za::PassKey<Owner>&));
        STATIC_CHECK(!ZA_IS_CONSTRUCTIBLE(za::PassKey<Owner>, za::PassKey<Owner>&&));

        CHECK(Owner::callRestricted() == 42);
    }
}
