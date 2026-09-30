#include "StringifyStringViewUtil.hpp" // IWYU pragma: keep
#include "Tst/Tst.hpp"

#include "Zancle/Reflection/Rfl.hpp"

#include "Zancle/Reflection/RflNames.hpp"

#include "Zancle/Base/NoUniqueAddress.hpp"

#include "Zancle/Trait/IsSame.hpp"


namespace
{
////////////////////////////////////////////////////////////
struct S0
{
};

////////////////////////////////////////////////////////////
struct S1
{
    int i;
};

////////////////////////////////////////////////////////////
struct S2
{
    int   i;
    float f;
};

////////////////////////////////////////////////////////////
struct S3
{
    int   i;
    float f;
    char  c;
};

////////////////////////////////////////////////////////////
struct SNested
{
    S2  inner;
    int outer;
};

////////////////////////////////////////////////////////////
struct SWithPtrRef
{
    int*        p;
    const char* s;
};

////////////////////////////////////////////////////////////
struct SEmpty1
{
};

////////////////////////////////////////////////////////////
struct SWithEmpty
{
    ZA_NO_UNIQUE_ADDRESS SEmpty1 e;
    int                          x;
};

////////////////////////////////////////////////////////////
// NOLINTBEGIN(readability-identifier-naming)
struct S32
{
    int a, b, c, d, e, f, g, h;
    int i, j, k, l, m, n, o, p;
    int q, r, s, t, u, v, w, x;
    int y, z, A, B, C, D, E, F;
};

////////////////////////////////////////////////////////////
struct SUnderscoreNames
{
    int  _leading;
    int  trailing_;
    char mix_ed;
};

// NOLINTEND(readability-identifier-naming)

////////////////////////////////////////////////////////////
struct S64
{
    int m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23,
        m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45,
        m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58, m59, m60, m61, m62, m63;
};

////////////////////////////////////////////////////////////
// Reference member without a default: not initializable with zero values
struct SWithRef
{
    int& r;
    int  x;
};

////////////////////////////////////////////////////////////
struct SWithConst
{
    const int c;
    float     f;
};

////////////////////////////////////////////////////////////
union UInts
{
    int   i;
    float f;
};

////////////////////////////////////////////////////////////
// GCC prints template arguments inline: quoted brackets must not confuse the name parser
template <char C>
struct SCharTmpl
{
    int  before;
    char after;
};

////////////////////////////////////////////////////////////
// Constraint probes (invalid calls yield `false` instead of a hard error)
template <typename T>
concept CanCountFields = requires { za::rfl::numFields<T>; };

template <typename T>
concept CanTieAsTuple = requires(T&& t) { za::rfl::tieAsTuple(static_cast<T&&>(t)); };

template <typename T>
concept CanGetField = requires(T&& t) { za::rfl::getField<0>(static_cast<T&&>(t)); };

template <typename T>
concept CanForEachField = requires(T&& t) { za::rfl::forEachField(static_cast<T&&>(t), [](auto&) {}); };

}; // namespace


TEST_CASE("[Reflection] Reflection/Rfl.hpp")
{
    SECTION("NumFields")
    {
        STATIC_CHECK(za::rfl::numFields<S0> == 0);
        STATIC_CHECK(za::rfl::numFields<S1> == 1);
        STATIC_CHECK(za::rfl::numFields<S2> == 2);
        STATIC_CHECK(za::rfl::numFields<S3> == 3);
    }

    SECTION("TieAsTuple")
    {
        {
            S0   obj{};
            auto tpl = za::rfl::tieAsTuple(obj);

            STATIC_CHECK(decltype(tpl)::memberCount == 0);
        }

        {
            S1   obj{42};
            auto tpl = za::rfl::tieAsTuple(obj);

            STATIC_CHECK(decltype(tpl)::memberCount == 1);

            CHECK(tpl.get<0>() == 42);

            tpl.get<0>() = 4;
            CHECK(obj.i == 4);
        }

        {
            S2   obj{42, 10.f};
            auto tpl = za::rfl::tieAsTuple(obj);

            STATIC_CHECK(decltype(tpl)::memberCount == 2);

            CHECK(tpl.get<0>() == 42);
            CHECK(tpl.get<1>() == 10.f);

            tpl.get<0>() = 4;
            CHECK(obj.i == 4);
        }

        {
            S3   obj{42, 10.f, 'x'};
            auto tpl = za::rfl::tieAsTuple(obj);

            STATIC_CHECK(decltype(tpl)::memberCount == 3);

            CHECK(tpl.get<0>() == 42);
            CHECK(tpl.get<1>() == 10.f);
            CHECK(tpl.get<2>() == 'x');

            tpl.get<0>() = 10;
            CHECK(obj.i == 10);
        }
    }

    SECTION("GetField")
    {
        {
            S1 obj{42};

            CHECK(za::rfl::getField<0>(obj) == 42);

            za::rfl::getField<0>(obj) = 4;
            CHECK(obj.i == 4);
        }

        {
            S2 obj{42, 10.f};

            CHECK(za::rfl::getField<0>(obj) == 42);
            CHECK(za::rfl::getField<1>(obj) == 10.f);

            za::rfl::getField<0>(obj) = 4;
            CHECK(obj.i == 4);
        }

        {
            S3 obj{42, 10.f, 'x'};

            CHECK(za::rfl::getField<0>(obj) == 42);
            CHECK(za::rfl::getField<1>(obj) == 10.f);
            CHECK(za::rfl::getField<2>(obj) == 'x');

            za::rfl::getField<0>(obj) = 10;
            CHECK(obj.i == 10);
        }
    }

    SECTION("GetFieldName")
    {
        STATIC_CHECK(za::rfl::getFieldName<S1, 0>() == "i");

        STATIC_CHECK(za::rfl::getFieldName<S2, 0>() == "i");
        STATIC_CHECK(za::rfl::getFieldName<S2, 1>() == "f");

        STATIC_CHECK(za::rfl::getFieldName<S3, 0>() == "i");
        STATIC_CHECK(za::rfl::getFieldName<S3, 1>() == "f");
        STATIC_CHECK(za::rfl::getFieldName<S3, 2>() == "c");

        struct TestNames
        {
            int  hello;
            char world;
        };

        STATIC_CHECK(za::rfl::getFieldName<TestNames, 0>() == "hello");
        STATIC_CHECK(za::rfl::getFieldName<TestNames, 1>() == "world");
    }

    SECTION("ForEachField")
    {
        {
            S3  obj{42, 10.f, 'x'};
            int visitCount = 0;
            za::rfl::forEachField(obj, [&](auto&) { ++visitCount; });
            CHECK(visitCount == 3);
        }

        {
            S3 obj{1, 2.f, 'a'};
            za::rfl::forEachField(obj,
                                  [](auto& v)
            {
                if constexpr (za::isSame<decltype(v), int&>)
                    v = 100;
                else if constexpr (za::isSame<decltype(v), float&>)
                    v = 200.f;
                else if constexpr (za::isSame<decltype(v), char&>)
                    v = 'z';
            });
            CHECK(obj.i == 100);
            CHECK(obj.f == 200.f);
            CHECK(obj.c == 'z');
        }

        {
            const S2 obj{7, 1.5f};
            int      sum = 0;
            za::rfl::forEachField(obj,
                                  [&](const auto& v)
            {
                if constexpr (za::isSame<decltype(v), const int&>)
                    sum += v;
                else if constexpr (za::isSame<decltype(v), const float&>)
                    sum += static_cast<int>(v);
            });
            CHECK(sum == 8);
        }
    }

    SECTION("FieldType")
    {
        STATIC_CHECK(za::isSame<za::rfl::FieldType<0, S1>, int>);
        STATIC_CHECK(za::isSame<za::rfl::FieldType<0, S3>, int>);
        STATIC_CHECK(za::isSame<za::rfl::FieldType<1, S3>, float>);
        STATIC_CHECK(za::isSame<za::rfl::FieldType<2, S3>, char>);
        STATIC_CHECK(za::isSame<za::rfl::FieldType<0, SWithPtrRef>, int*>);
        STATIC_CHECK(za::isSame<za::rfl::FieldType<1, SWithPtrRef>, const char*>);
    }

    SECTION("TieAsFieldNamesTuple")
    {
        constexpr auto names1 = za::rfl::tieAsFieldNamesTuple<S1>();
        STATIC_CHECK(decltype(names1)::memberCount == 1);
        STATIC_CHECK(names1.template get<0>() == "i");

        constexpr auto names3 = za::rfl::tieAsFieldNamesTuple<S3>();
        STATIC_CHECK(decltype(names3)::memberCount == 3);
        STATIC_CHECK(names3.template get<0>() == "i");
        STATIC_CHECK(names3.template get<1>() == "f");
        STATIC_CHECK(names3.template get<2>() == "c");

        constexpr auto namesNested = za::rfl::tieAsFieldNamesTuple<SNested>();
        STATIC_CHECK(decltype(namesNested)::memberCount == 2);
        STATIC_CHECK(namesNested.template get<0>() == "inner");
        STATIC_CHECK(namesNested.template get<1>() == "outer");
    }

    SECTION("ConstPropagation")
    {
        const S3 obj{42, 10.f, 'x'};

        auto tpl = za::rfl::tieAsTuple(obj);
        STATIC_CHECK(za::isSame<decltype(tpl.template get<0>()), const int&>);
        STATIC_CHECK(za::isSame<decltype(tpl.template get<1>()), const float&>);
        STATIC_CHECK(za::isSame<decltype(tpl.template get<2>()), const char&>);

        CHECK(tpl.template get<0>() == 42);
        CHECK(tpl.template get<1>() == 10.f);
        CHECK(tpl.template get<2>() == 'x');

        STATIC_CHECK(za::isSame<decltype(za::rfl::getField<0>(obj)), const int&>);
    }
}


TEST_CASE("[Reflection] Reflection/RflNames.hpp")
{
    SECTION("NestedAggregate")
    {
        STATIC_CHECK(za::rfl::numFields<SNested> == 2);

        SNested obj{{1, 2.f}, 99};
        auto    tpl = za::rfl::tieAsTuple(obj);

        CHECK(tpl.get<0>().i == 1);
        CHECK(tpl.get<0>().f == 2.f);
        CHECK(tpl.get<1>() == 99);

        tpl.get<0>().i = 123;
        CHECK(obj.inner.i == 123);

        STATIC_CHECK(za::rfl::getFieldName<SNested, 0>() == "inner");
        STATIC_CHECK(za::rfl::getFieldName<SNested, 1>() == "outer");
    }

    SECTION("PointerAndReferenceMembers")
    {
        int         n = 5;
        const char* s = "hi";
        SWithPtrRef obj{&n, s};

        STATIC_CHECK(za::rfl::numFields<SWithPtrRef> == 2);
        CHECK(za::rfl::getField<0>(obj) == &n);
        CHECK(*za::rfl::getField<0>(obj) == 5);
        CHECK(za::rfl::getField<1>(obj) == s);

        STATIC_CHECK(za::rfl::getFieldName<SWithPtrRef, 0>() == "p");
        STATIC_CHECK(za::rfl::getFieldName<SWithPtrRef, 1>() == "s");
    }

    SECTION("NoUniqueAddressEmptyMember")
    {
        STATIC_CHECK(za::rfl::numFields<SWithEmpty> == 2);

        SWithEmpty obj{};
        obj.x = 7;

        auto tpl = za::rfl::tieAsTuple(obj);
        CHECK(tpl.get<1>() == 7);

        STATIC_CHECK(za::rfl::getFieldName<SWithEmpty, 0>() == "e");
        STATIC_CHECK(za::rfl::getFieldName<SWithEmpty, 1>() == "x");
    }

    SECTION("MaxFieldCount")
    {
        STATIC_CHECK(za::rfl::numFields<S32> == 32);

        S32 obj{};
        obj.a = 1;
        obj.F = 32;

        auto tpl = za::rfl::tieAsTuple(obj);
        STATIC_CHECK(decltype(tpl)::memberCount == 32);

        CHECK(tpl.get<0>() == 1);
        CHECK(tpl.get<31>() == 32);

        tpl.get<15>() = 1616;
        CHECK(obj.p == 1616);

        int visitCount = 0;
        za::rfl::forEachField(obj, [&](auto&) { ++visitCount; });
        CHECK(visitCount == 32);

        STATIC_CHECK(za::rfl::getFieldName<S32, 0>() == "a");
        STATIC_CHECK(za::rfl::getFieldName<S32, 25>() == "z");
        STATIC_CHECK(za::rfl::getFieldName<S32, 26>() == "A");
        STATIC_CHECK(za::rfl::getFieldName<S32, 31>() == "F");
    }

    SECTION("NameParsingEdgeCases")
    {
        STATIC_CHECK(za::rfl::getFieldName<SUnderscoreNames, 0>() == "_leading");
        STATIC_CHECK(za::rfl::getFieldName<SUnderscoreNames, 1>() == "trailing_");
        STATIC_CHECK(za::rfl::getFieldName<SUnderscoreNames, 2>() == "mix_ed");
    }
}


TEST_CASE("[Reflection] Reflection/Rfl.hpp: limits and constraints")
{
    SECTION("64 fields")
    {
        STATIC_CHECK(za::rfl::numFields<S64> == 64);

        S64 obj{};
        obj.m63 = 63;

        CHECK(za::rfl::getField<63>(obj) == 63);

        int visitCount = 0;
        za::rfl::forEachField(obj, [&](int&) { ++visitCount; });
        CHECK(visitCount == 64);

        STATIC_CHECK(za::rfl::getFieldName<S64, 0>() == "m0");
        STATIC_CHECK(za::rfl::getFieldName<S64, 63>() == "m63");
    }

    SECTION("Reference members")
    {
        STATIC_CHECK(za::rfl::numFields<SWithRef> == 2);
        STATIC_CHECK(za::isSame<za::rfl::FieldType<0, SWithRef>, int>); // the referred-to type
        STATIC_CHECK(za::isSame<za::rfl::FieldType<1, SWithRef>, int>);

        int      target = 1;
        SWithRef obj{target, 2};

        za::rfl::getField<0>(obj) = 5; // writes through the reference
        CHECK(target == 5);
        CHECK(za::rfl::getField<1>(obj) == 2);
    }

    SECTION("Const members")
    {
        STATIC_CHECK(za::rfl::numFields<SWithConst> == 2);
        STATIC_CHECK(za::isSame<za::rfl::FieldType<0, SWithConst>, const int>);

        const SWithConst obj{7, 1.5f};
        CHECK(za::rfl::getField<0>(obj) == 7);
    }

    SECTION("C-style array as the reflected type")
    {
        STATIC_CHECK(za::rfl::numFields<int[3]> == 3);

        int arr[3]{1, 2, 3};
        za::rfl::getField<2>(arr) = 30;
        CHECK(arr[2] == 30);

        int sum = 0;
        za::rfl::forEachField(arr, [&](int x) { sum += x; });
        CHECK(sum == 33);
    }

    SECTION("Unions are rejected")
    {
        STATIC_CHECK(CanCountFields<S3>);
        STATIC_CHECK(!CanCountFields<UInts>);
        STATIC_CHECK(!CanTieAsTuple<UInts&>);
        STATIC_CHECK(!CanGetField<UInts&>);
        STATIC_CHECK(!CanForEachField<UInts&>);
    }

    SECTION("References to temporaries are rejected")
    {
        STATIC_CHECK(CanTieAsTuple<S3&>);
        STATIC_CHECK(CanTieAsTuple<const S3&>);
        STATIC_CHECK(!CanTieAsTuple<S3>);
        STATIC_CHECK(!CanTieAsTuple<S3&&>);

        STATIC_CHECK(CanGetField<S3&>);
        STATIC_CHECK(CanGetField<const S3&>);
        STATIC_CHECK(!CanGetField<S3>);
        STATIC_CHECK(!CanGetField<S3&&>);

        // Fine: the temporary outlives the call
        STATIC_CHECK(CanForEachField<S3>);

        int sum = 0;
        za::rfl::forEachField(S3{1, 2.f, 'a'}, [&](const auto& x) { sum += static_cast<int>(x); });
        CHECK(sum == 1 + 2 + 'a');
    }

    SECTION("Names of fields of types with quoted template arguments")
    {
        STATIC_CHECK(za::rfl::getFieldName<SCharTmpl<'{'>, 0>() == "before");
        STATIC_CHECK(za::rfl::getFieldName<SCharTmpl<'{'>, 1>() == "after");
        STATIC_CHECK(za::rfl::getFieldName<SCharTmpl<'<'>, 1>() == "after");
        STATIC_CHECK(za::rfl::getFieldName<SCharTmpl<'}'>, 1>() == "after");
        STATIC_CHECK(za::rfl::getFieldName<SCharTmpl<'\''>, 1>() == "after");
        STATIC_CHECK(za::rfl::getFieldName<SCharTmpl<','>, 1>() == "after");
    }
}
