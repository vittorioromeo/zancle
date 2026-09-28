#include "GraphicsUtil.hpp"
#include "SystemUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/Graphics/TransformableMixin.hpp"

#include "Zancle/Graphics/Priv/TransformableMacros.hpp"

#include "Zancle/Geometry/Angle.hpp"
#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Trait/IsAggregate.hpp"
#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"
#include "Zancle/Trait/IsStandardLayout.hpp"
#include "Zancle/Trait/IsTrivial.hpp"
#include "Zancle/Trait/IsTriviallyAssignable.hpp"
#include "Zancle/Trait/IsTriviallyCopyable.hpp"
#include "Zancle/Trait/IsTriviallyDestructible.hpp"


struct TestTransformable : za::TransformableMixin
{
    ZA_DEFINE_TRANSFORMABLE_DATA_MEMBERS;
};


TEST_CASE("[Graphics] za::Transformable")
{
    SECTION("Type traits")
    {
        STATIC_CHECK(ZA_IS_COPY_CONSTRUCTIBLE(TestTransformable));
        STATIC_CHECK(ZA_IS_COPY_ASSIGNABLE(TestTransformable));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_CONSTRUCTIBLE(TestTransformable));
        STATIC_CHECK(ZA_IS_NOTHROW_MOVE_ASSIGNABLE(TestTransformable));

        STATIC_CHECK(!ZA_IS_TRIVIAL(TestTransformable)); // because of member initializers
        STATIC_CHECK(ZA_IS_STANDARD_LAYOUT(TestTransformable));
        STATIC_CHECK(ZA_IS_AGGREGATE(TestTransformable));
        STATIC_CHECK(ZA_IS_TRIVIALLY_COPYABLE(TestTransformable));
        STATIC_CHECK(ZA_IS_TRIVIALLY_DESTRUCTIBLE(TestTransformable));
        STATIC_CHECK(ZA_IS_TRIVIALLY_ASSIGNABLE(TestTransformable, TestTransformable));
    }

    SECTION("Construction")
    {
        const TestTransformable transformable;
        CHECK(transformable.position == za::Vec2f{0, 0});
        CHECK(transformable.rotation == za::Angle::Zero);
        CHECK(transformable.scale == za::Vec2f{1, 1});
        CHECK(transformable.origin == za::Vec2f{0, 0});
        CHECK(transformable.getTransform() == za::Transform());
        CHECK(transformable.getInverseTransform() == za::Transform());
    }

    SECTION("Setters and getters")
    {
        TestTransformable transformable;

        transformable.position = {3, 4};
        CHECK(transformable.position == za::Vec2f{3, 4});

        transformable.rotation = za::degrees(3.14f);
        CHECK(transformable.rotation == Approx(za::degrees(3.14f)));
        transformable.rotation = za::degrees(540);
        CHECK(transformable.rotation == Approx(za::degrees(180)));
        transformable.rotation = za::degrees(-72);
        CHECK(transformable.rotation == Approx(za::degrees(288)));

        transformable.scale = {5, 6};
        CHECK(transformable.scale == za::Vec2f{5, 6});

        transformable.origin = {7, 8};
        CHECK(transformable.origin == za::Vec2f{7, 8});

        za::Transform transform;
        transform.translate(transformable.position - transformable.origin);
        transform.rotate(transformable.rotation, transformable.origin);
        transform.scaleBy(transformable.scale, transformable.origin);

        // clang-format off
        float lhsMatrix[]{{},  {},  0.f, 0.f,
                          {},  {},  0.f, 0.f,
                          0.f, 0.f, 1.f, 0.f,
                          {},  {},  0.f, 1.f};

        float rhsMatrix[]{{},  {},  0.f, 0.f,
                          {},  {},  0.f, 0.f,
                          0.f, 0.f, 1.f, 0.f,
                          {},  {},  0.f, 1.f};
        // clang-format on

        transformable.getTransform().writeTo4x4Matrix(lhsMatrix);
        transform.writeTo4x4Matrix(rhsMatrix);

        CHECK(lhsMatrix[0] == Approx(rhsMatrix[0]));
        CHECK(lhsMatrix[1] == Approx(rhsMatrix[1]));
        CHECK(lhsMatrix[2] == Approx(rhsMatrix[2]));
        CHECK(lhsMatrix[3] == Approx(rhsMatrix[3]));
        CHECK(lhsMatrix[4] == Approx(rhsMatrix[4]));
        CHECK(lhsMatrix[5] == Approx(rhsMatrix[5]));
        CHECK(lhsMatrix[6] == Approx(rhsMatrix[6]));
        CHECK(lhsMatrix[7] == Approx(rhsMatrix[7]));
        CHECK(lhsMatrix[8] == Approx(rhsMatrix[8]));
        CHECK(lhsMatrix[9] == Approx(rhsMatrix[9]));
        CHECK(lhsMatrix[10] == Approx(rhsMatrix[10]));
        CHECK(lhsMatrix[11] == Approx(rhsMatrix[11]));
        CHECK(lhsMatrix[12] == Approx(rhsMatrix[12]));
        CHECK(lhsMatrix[13] == Approx(rhsMatrix[13]));
        CHECK(lhsMatrix[14] == Approx(rhsMatrix[14]));
        CHECK(lhsMatrix[15] == Approx(rhsMatrix[15]));

        const za::Transform inverseTransform = transform.getInverse();
        transformable.getInverseTransform().writeTo4x4Matrix(lhsMatrix);
        inverseTransform.writeTo4x4Matrix(rhsMatrix);

        CHECK(lhsMatrix[0] == Approx(rhsMatrix[0]));
        CHECK(lhsMatrix[1] == Approx(rhsMatrix[1]));
        CHECK(lhsMatrix[2] == Approx(rhsMatrix[2]));
        CHECK(lhsMatrix[3] == Approx(rhsMatrix[3]));
        CHECK(lhsMatrix[4] == Approx(rhsMatrix[4]));
        CHECK(lhsMatrix[5] == Approx(rhsMatrix[5]));
        CHECK(lhsMatrix[6] == Approx(rhsMatrix[6]));
        CHECK(lhsMatrix[7] == Approx(rhsMatrix[7]));
        CHECK(lhsMatrix[8] == Approx(rhsMatrix[8]));
        CHECK(lhsMatrix[9] == Approx(rhsMatrix[9]));
        CHECK(lhsMatrix[10] == Approx(rhsMatrix[10]));
        CHECK(lhsMatrix[11] == Approx(rhsMatrix[11]));
        CHECK(lhsMatrix[12] == Approx(rhsMatrix[12]));
        CHECK(lhsMatrix[13] == Approx(rhsMatrix[13]));
        CHECK(lhsMatrix[14] == Approx(rhsMatrix[14]));
        CHECK(lhsMatrix[15] == Approx(rhsMatrix[15]));
    }

    SECTION("Movement")
    {
        TestTransformable transformable;
        CHECK(transformable.position == za::Vec2f{0, 0});
        transformable.position += {9, 10};
        CHECK(transformable.position == za::Vec2f{9, 10});
        transformable.position += {-15, 2};
        CHECK(transformable.position == za::Vec2f(-6, 12));
    }

    SECTION("Rotation")
    {
        TestTransformable transformable;
        CHECK(transformable.rotation == za::Angle::Zero);
        transformable.rotation += za::degrees(15);
        CHECK(transformable.rotation == Approx(za::degrees(15)));
        transformable.rotation += za::degrees(360);
        CHECK(transformable.rotation == Approx(za::degrees(15)));
        transformable.rotation += za::degrees(-25);
        CHECK(transformable.rotation == Approx(za::degrees(350)));
        transformable.rotation += za::degrees(-720);
        CHECK(transformable.rotation == Approx(za::degrees(350)));
        transformable.rotation += za::degrees(-370);
        CHECK(transformable.rotation == Approx(za::degrees(340)));
    }

    SECTION("Rotation near a quarter turn (lookup overshoot)")
    {
        // `sinCosLookup(1.57007849f)` returns a sine of `1.00000012f`: `getTransform` used to fail an
        // assertion requiring `[-1, 1]` (e.g. crashing the batching example at startup)
        TestTransformable transformable;
        transformable.rotation = za::radians(1.57007849f);

        const za::Transform transform = transformable.getTransform();
        CHECK(transform.transformPoint({1.f, 0.f}) == Approx(za::Vec2f{0.f, 1.f}));
    }

    SECTION("scaleBy()")
    {
        TestTransformable transformable;
        CHECK(transformable.scale == za::Vec2f{1, 1});
        transformable.scaleBy({2, 3});
        CHECK(transformable.scale == za::Vec2f{2, 3});
        transformable.scaleBy({2, 1});
        CHECK(transformable.scale == za::Vec2f{4, 3});
        transformable.scaleBy({-1, -1});
        CHECK(transformable.scale == za::Vec2f(-4, -3));
    }
}
