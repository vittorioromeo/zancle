#include "SystemUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/Geometry/Vec3.hpp"

#include "Zancle/Trait/IsAggregate.hpp"
#include "Zancle/Trait/IsSame.hpp"
#include "Zancle/Trait/IsStandardLayout.hpp"
#include "Zancle/Trait/IsTrivial.hpp"
#include "Zancle/Trait/IsTriviallyAssignable.hpp"
#include "Zancle/Trait/IsTriviallyCopyAssignable.hpp"
#include "Zancle/Trait/IsTriviallyCopyConstructible.hpp"
#include "Zancle/Trait/IsTriviallyCopyable.hpp"
#include "Zancle/Trait/IsTriviallyDestructible.hpp"
#include "Zancle/Trait/IsTriviallyMoveAssignable.hpp"
#include "Zancle/Trait/IsTriviallyMoveConstructible.hpp"


////////////////////////////////////////////////////////////
// Explicit instantiations: compile-check every member function
////////////////////////////////////////////////////////////
template struct za::Vec3<float>;
template struct za::Vec3<double>;


TEMPLATE_TEST_CASE("[System] za::Vec3", "", int, float)
{
    SECTION("Type traits")
    {
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(za::Vec3<TestType>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(za::Vec3<TestType>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_MOVE_CONSTRUCTIBLE(za::Vec3<TestType>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_MOVE_ASSIGNABLE(za::Vec3<TestType>));

        STATIC_CHECK(!ZA_IS_TRIVIAL(za::Vec3<TestType>)); // because of member initializers
        STATIC_CHECK(ZA_IS_STANDARD_LAYOUT(za::Vec3<TestType>));
        STATIC_CHECK(ZA_IS_AGGREGATE(za::Vec3<TestType>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPYABLE(za::Vec3<TestType>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_DESTRUCTIBLE(za::Vec3<TestType>));
        STATIC_CHECK(ZA_IS_TRIVIALLY_ASSIGNABLE(za::Vec3<TestType>, za::Vec3<TestType>));
    }

    SECTION("Construction")
    {
        SECTION("Default constructor")
        {
            constexpr za::Vec3<TestType> vec;
            STATIC_CHECK(vec.x == 0);
            STATIC_CHECK(vec.y == 0);
            STATIC_CHECK(vec.z == 0);
        }

        SECTION("(x, y, z) coordinate constructor")
        {
            constexpr za::Vec3<TestType> vec(1, 2, 3);
            STATIC_CHECK(vec.x == 1);
            STATIC_CHECK(vec.y == 2);
            STATIC_CHECK(vec.z == 3);
        }
    }

    SECTION("Unary operations")
    {
        SECTION("-vec")
        {
            constexpr za::Vec3<TestType> vec(1, 2, 3);
            constexpr za::Vec3<TestType> negatedVec = -vec;

            STATIC_CHECK(negatedVec.x == -1);
            STATIC_CHECK(negatedVec.y == -2);
            STATIC_CHECK(negatedVec.z == -3);
        }
    }

    SECTION("Arithmetic operations between two vecs")
    {
        za::Vec3<TestType>           firstVec(2, 5, 6);
        constexpr za::Vec3<TestType> secondVec(8, 3, 7);

        SECTION("vec += vec")
        {
            firstVec += secondVec;

            CHECK(firstVec.x == 10);
            CHECK(firstVec.y == 8);
            CHECK(firstVec.z == 13);
        }

        SECTION("vec -= vec")
        {
            firstVec -= secondVec;

            CHECK(firstVec.x == -6);
            CHECK(firstVec.y == 2);
            CHECK(firstVec.z == -1);
        }

        SECTION("vec + vec")
        {
            const za::Vec3<TestType> result = firstVec + secondVec;

            CHECK(result.x == 10);
            CHECK(result.y == 8);
            CHECK(result.z == 13);
        }

        SECTION("vec - vec")
        {
            const za::Vec3<TestType> result = firstVec - secondVec;

            CHECK(result.x == -6);
            CHECK(result.y == 2);
            CHECK(result.z == -1);
        }
    }

    SECTION("Arithmetic operations between vec and scalar value")
    {
        za::Vec3<TestType> vec(26, 12, 6);
        constexpr TestType scalar = 2;

        SECTION("vec * scalar")
        {
            const za::Vec3<TestType> result = vec * scalar;

            CHECK(result.x == 52);
            CHECK(result.y == 24);
            CHECK(result.z == 12);
        }

        SECTION("scalar * vec")
        {
            const za::Vec3<TestType> result = scalar * vec;

            CHECK(result.x == 52);
            CHECK(result.y == 24);
            CHECK(result.z == 12);
        }

        SECTION("vec *= scalar")
        {
            vec *= scalar;

            CHECK(vec.x == 52);
            CHECK(vec.y == 24);
            CHECK(vec.z == 12);
        }

        SECTION("vec / scalar")
        {
            const za::Vec3<TestType> result = vec / scalar;

            CHECK(result.x == 13);
            CHECK(result.y == 6);
            CHECK(result.z == 3);
        }

        SECTION("vec /= scalar")
        {
            vec /= scalar;

            CHECK(vec.x == 13);
            CHECK(vec.y == 6);
            CHECK(vec.z == 3);
        }
    }

    SECTION("Comparison operations (two equal and one different vec)")
    {
        constexpr za::Vec3<TestType> firstEqualVec(1, 5, 6);
        constexpr za::Vec3<TestType> secondEqualVec(1, 5, 6);
        constexpr za::Vec3<TestType> differentVec(6, 9, 7);

        SECTION("vec == vec")
        {
            STATIC_CHECK(firstEqualVec == secondEqualVec);
            STATIC_CHECK_FALSE(firstEqualVec == differentVec);
        }

        SECTION("vec != vec")
        {
            STATIC_CHECK(firstEqualVec != differentVec);
            STATIC_CHECK_FALSE(firstEqualVec != secondEqualVec);
        }
    }

    SECTION("Structured bindings")
    {
        za::Vec3<TestType> vec(1, 2, 3); // NOLINT(misc-const-correctness)

        SECTION("destructure by value")
        {
            auto [x, y, z] = vec;

            CHECK(x == 1);
            CHECK(y == 2);
            CHECK(z == 3);

            STATIC_CHECK(ZA_IS_SAME(decltype(x), decltype(vec.x)));

            x = 3;

            CHECK(x == 3);
            CHECK(vec.x == 1);
        }

        SECTION("destructure by ref")
        {
            auto& [x, y, z] = vec;

            CHECK(x == 1);
            CHECK(y == 2);
            CHECK(z == 3);

            STATIC_CHECK(ZA_IS_SAME(decltype(x), decltype(vec.x)));

            x = 3;

            CHECK(x == 3);
            CHECK(vec.x == 3);
        }
    }

    SECTION("Length and normalization")
    {
        constexpr za::Vec3f v(2.4f, 3.f, 5.2f);

        CHECK(v.length() == Approx(6.46529f));
        CHECK(v.lengthSquared() == Approx(41.79997f));
        CHECK(v.normalized() == Approx(za::Vec3f(0.37121f, 0.46401f, 0.80429f)));
    }

    SECTION("Products and quotients")
    {
        constexpr za::Vec3f v(2.4f, 3.f, 5.2f);
        constexpr za::Vec3f w(-0.7f, -2.2f, -4.8f);

        CHECK(v.dot(w) == Approx(-33.24f));
        CHECK(w.dot(v) == Approx(-33.24f));

        CHECK(v.cross(w) == Approx(za::Vec3f(-2.96f, 7.88f, -3.18f)));
        CHECK(w.cross(v) == Approx(za::Vec3f(2.96f, -7.88f, 3.18f)));

        CHECK(v.componentWiseMul(w) == Approx(za::Vec3f(-1.68f, -6.6f, -24.96f)));
        CHECK(w.componentWiseMul(v) == Approx(za::Vec3f(-1.68f, -6.6f, -24.96f)));
        CHECK(v.componentWiseDiv(w) == Approx(za::Vec3f(-3.428571f, -1.363636f, -1.0833333f)));
        CHECK(w.componentWiseDiv(v) == Approx(za::Vec3f(-0.291666f, -0.733333f, -0.9230769f)));
    }
}
