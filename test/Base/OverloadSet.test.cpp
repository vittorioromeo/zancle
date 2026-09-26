#include "Tst/Tst.hpp"

#include "Zancle/Vocabulary/OverloadSet.hpp"

#include "Zancle/Trait/IsAggregate.hpp"


TEST_CASE("[Base] Base/OverloadSet.hpp")
{
    SECTION("Dispatches to the matching overload")
    {
        const auto os = za::OverloadSet{[](int) { return 0; }, [](float) { return 1; }};

        STATIC_CHECK(ZA_IS_AGGREGATE(decltype(os)));

        CHECK(os(1) == 0);
        CHECK(os(1.f) == 1);
    }

    SECTION("Stateful callables are copied into the set")
    {
        int  value = 5;
        auto get   = [value] { return value; };

        auto os = za::OverloadSet{get, [](int x) { return x * 2; }};

        CHECK(os() == 5);
        CHECK(os(4) == 8);
    }

    SECTION("Copyable from a non-const lvalue")
    {
        auto os = za::OverloadSet{[](int) { return 0; }, [](float) { return 1; }};

        za::OverloadSet copy(os); // NOLINT(performance-unnecessary-copy-initialization)
        za::OverloadSet copy2{os};

        CHECK(copy(1.f) == 1);
        CHECK(copy2(1) == 0);
    }
}
