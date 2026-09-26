#include "Zancle/Trait/AddPointer.hpp"
#include "Zancle/Trait/CommonType.hpp"
#include "Zancle/Trait/Conditional.hpp"
#include "Zancle/Trait/CopyCV.hpp"
#include "Zancle/Trait/Decay.hpp"
#include "Zancle/Trait/DeclVal.hpp"
#include "Zancle/Trait/EnableTrivialRelocation.hpp"
#include "Zancle/Trait/IsArray.hpp"
#include "Zancle/Trait/IsBaseOf.hpp"
#include "Zancle/Trait/IsClass.hpp"
#include "Zancle/Trait/IsConst.hpp"
#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsEnum.hpp"
#include "Zancle/Trait/IsFloatingPoint.hpp"
#include "Zancle/Trait/IsFunction.hpp"
#include "Zancle/Trait/IsIntegral.hpp"
#include "Zancle/Trait/IsInvocableR.hpp"
#include "Zancle/Trait/IsMemberPointer.hpp"
#include "Zancle/Trait/IsMoveAssignable.hpp"
#include "Zancle/Trait/IsMoveConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"
#include "Zancle/Trait/IsNothrowSwappable.hpp"
#include "Zancle/Trait/IsPointer.hpp"
#include "Zancle/Trait/IsReference.hpp"
#include "Zancle/Trait/IsRvalueReference.hpp"
#include "Zancle/Trait/IsSame.hpp"
#include "Zancle/Trait/IsTriviallyCopyAssignable.hpp"
#include "Zancle/Trait/IsTriviallyCopyConstructible.hpp"
#include "Zancle/Trait/IsTriviallyCopyable.hpp"
#include "Zancle/Trait/IsTriviallyDefaultConstructible.hpp"
#include "Zancle/Trait/IsTriviallyDestructible.hpp"
#include "Zancle/Trait/IsTriviallyMoveAssignable.hpp"
#include "Zancle/Trait/IsTriviallyMoveConstructible.hpp"
#include "Zancle/Trait/IsTriviallyRelocatable.hpp"
#include "Zancle/Trait/IsUnion.hpp"
#include "Zancle/Trait/IsUnsigned.hpp"
#include "Zancle/Trait/IsVoid.hpp"
#include "Zancle/Trait/MakeUnsigned.hpp"
#include "Zancle/Trait/ReferenceConvertsFromTemporary.hpp"
#include "Zancle/Trait/RegularizeVoid.hpp"
#include "Zancle/Trait/RemoveCV.hpp"
#include "Zancle/Trait/RemoveCVRef.hpp"
#include "Zancle/Trait/RemoveReference.hpp"
#include "Zancle/Trait/UnderlyingType.hpp"


namespace
{
namespace TraitsTest // for unity builds
{
////////////////////////////////////////////////////////////
static_assert(!za::isUnsigned<float>);
static_assert(!za::isUnsigned<char>);
static_assert(!za::isUnsigned<int>);
static_assert(za::isUnsigned<bool>);
static_assert(za::isUnsigned<unsigned char>);
static_assert(za::isUnsigned<unsigned int>);


////////////////////////////////////////////////////////////
struct B
{
};

struct D : B
{
};

static_assert(ZA_IS_BASE_OF(B, D));
static_assert(!ZA_IS_BASE_OF(D, B));
static_assert(!ZA_IS_BASE_OF(int, D));


////////////////////////////////////////////////////////////
enum E : int
{
};

enum class EC : int
{
};

static_assert(ZA_IS_ENUM(E));
static_assert(ZA_IS_ENUM(EC));
static_assert(!ZA_IS_ENUM(int));


////////////////////////////////////////////////////////////
static_assert(za::isFloatingPoint<float>);
static_assert(za::isFloatingPoint<double>);
static_assert(za::isFloatingPoint<long double>);
static_assert(!za::isFloatingPoint<int>);
static_assert(!za::isFloatingPoint<float&>);


////////////////////////////////////////////////////////////
static_assert(za::isRvalueReference<int&&>);
static_assert(za::isRvalueReference<const int&&>);
static_assert(!za::isRvalueReference<const int&>);
static_assert(!za::isRvalueReference<int&>);
static_assert(!za::isRvalueReference<int>);


////////////////////////////////////////////////////////////
static_assert(ZA_IS_SAME(int, int));
static_assert(!ZA_IS_SAME(int, float));


////////////////////////////////////////////////////////////
struct NonTrivial
{
    static inline int si{};

    NonTrivial(const NonTrivial&) : i(si)
    {
    }

    // NOLINTNEXTLINE(modernize-use-equals-default)
    ~NonTrivial()
    {
    }

    int& i; // NOLINT(cppcoreguidelines-use-default-member-init, modernize-use-default-member-init)
};

static_assert(ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(int));
static_assert(!ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(NonTrivial));

static_assert(ZA_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(int));
static_assert(!ZA_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(NonTrivial));

static_assert(ZA_IS_TRIVIALLY_COPYABLE(int));
static_assert(!ZA_IS_TRIVIALLY_COPYABLE(NonTrivial));

static_assert(ZA_IS_TRIVIALLY_DESTRUCTIBLE(int));
static_assert(!ZA_IS_TRIVIALLY_DESTRUCTIBLE(NonTrivial));

static_assert(ZA_IS_TRIVIALLY_MOVE_ASSIGNABLE(int));
static_assert(!ZA_IS_TRIVIALLY_MOVE_ASSIGNABLE(NonTrivial));

static_assert(ZA_IS_TRIVIALLY_MOVE_CONSTRUCTIBLE(int));
static_assert(!ZA_IS_TRIVIALLY_MOVE_CONSTRUCTIBLE(NonTrivial));


////////////////////////////////////////////////////////////
static_assert(ZA_IS_SAME(ZA_REMOVE_CVREF(int), int));
static_assert(ZA_IS_SAME(ZA_REMOVE_CVREF(int&), int));
static_assert(ZA_IS_SAME(ZA_REMOVE_CVREF(const int&), int));
static_assert(ZA_IS_SAME(ZA_REMOVE_CVREF(volatile int&), int));
static_assert(ZA_IS_SAME(ZA_REMOVE_CVREF(const volatile int&&), int));


////////////////////////////////////////////////////////////
static_assert(ZA_IS_SAME(ZA_REMOVE_REFERENCE(int), int));
static_assert(ZA_IS_SAME(ZA_REMOVE_REFERENCE(int&), int));
static_assert(ZA_IS_SAME(ZA_REMOVE_REFERENCE(const int&), const int));
static_assert(ZA_IS_SAME(ZA_REMOVE_REFERENCE(volatile int&), volatile int));
static_assert(ZA_IS_SAME(ZA_REMOVE_REFERENCE(const volatile int&&), const volatile int));


////////////////////////////////////////////////////////////
static_assert(ZA_IS_SAME(ZA_UNDERLYING_TYPE(E), int));
static_assert(ZA_IS_SAME(ZA_UNDERLYING_TYPE(EC), int));


////////////////////////////////////////////////////////////
static_assert(ZA_IS_SAME(ZA_COMMON_TYPE(int, int), int));
static_assert(ZA_IS_SAME(ZA_COMMON_TYPE(int, float), float));
static_assert(ZA_IS_SAME(ZA_COMMON_TYPE(double, float), double));

////////////////////////////////////////////////////////////
static_assert(ZA_IS_SAME(ZA_DECAY(int), int));
static_assert(ZA_IS_SAME(ZA_DECAY(int*), int*));
static_assert(ZA_IS_SAME(ZA_DECAY(int (&)[1]), int*));
static_assert(ZA_IS_SAME(ZA_DECAY(int (&)[2]), int*));
static_assert(ZA_IS_SAME(ZA_DECAY(const int), int));
static_assert(ZA_IS_SAME(ZA_DECAY(const int&), int));
static_assert(ZA_IS_SAME(ZA_DECAY(int&), int));

////////////////////////////////////////////////////////////
static_assert(ZA_IS_TRIVIALLY_RELOCATABLE(int));
static_assert(ZA_IS_TRIVIALLY_RELOCATABLE(char));
static_assert(ZA_IS_TRIVIALLY_RELOCATABLE(float));
static_assert(ZA_IS_TRIVIALLY_RELOCATABLE(int*));

////////////////////////////////////////////////////////////
struct Custom0
{
};

struct Custom1
{
    ~Custom1() // NOLINT(modernize-use-equals-default)
    {
    }
};


static_assert(ZA_IS_TRIVIALLY_RELOCATABLE(Custom0));
static_assert(!ZA_IS_TRIVIALLY_RELOCATABLE(Custom1));


static_assert(ZA_IS_TRIVIALLY_RELOCATABLE(B));
static_assert(ZA_IS_TRIVIALLY_RELOCATABLE(D));
static_assert(ZA_IS_TRIVIALLY_RELOCATABLE(E));
static_assert(ZA_IS_TRIVIALLY_RELOCATABLE(EC));
static_assert(!ZA_IS_TRIVIALLY_RELOCATABLE(NonTrivial));

struct Custom2
{
    ~Custom2() // NOLINT(modernize-use-equals-default)
    {
    }
};

} // namespace TraitsTest
} // namespace


namespace za
{

template <>
inline constexpr bool enableTrivialRelocation<TraitsTest::Custom2> = true;

} // namespace za


namespace
{

static_assert(ZA_IS_TRIVIALLY_RELOCATABLE(TraitsTest::Custom2));
static_assert(ZA_IS_TRIVIALLY_RELOCATABLE(const TraitsTest::Custom2)); // specialization applies to cv-qualified `T`
static_assert(ZA_IS_TRIVIALLY_RELOCATABLE(const volatile TraitsTest::Custom2));


struct Custom3
{
    ZA_ENABLE_TRIVIAL_RELOCATION;

    ~Custom3() // NOLINT(modernize-use-equals-default)
    {
    }
};

static_assert(ZA_IS_TRIVIALLY_RELOCATABLE(Custom3));
static_assert(ZA_IS_TRIVIALLY_RELOCATABLE(const Custom3));


////////////////////////////////////////////////////////////
// Opt-in is not inherited: derived classes may add members that do not survive a `memcpy`
struct DerivedFromCustom3 : Custom3
{
    int  x{};
    int* self{&x};
};

static_assert(!ZA_IS_TRIVIALLY_RELOCATABLE(DerivedFromCustom3));


////////////////////////////////////////////////////////////
template <bool Enable>
struct ConditionallyRelocatable
{
    ZA_ENABLE_TRIVIAL_RELOCATION_IF(Enable);

    ~ConditionallyRelocatable() // NOLINT(modernize-use-equals-default)
    {
    }
};

static_assert(ZA_IS_TRIVIALLY_RELOCATABLE(ConditionallyRelocatable<true>));
static_assert(!ZA_IS_TRIVIALLY_RELOCATABLE(ConditionallyRelocatable<false>));
static_assert(!ZA_IS_TRIVIALLY_RELOCATABLE(int&) || ZA_IS_TRIVIALLY_RELOCATABLE(int&)); // must compile


////////////////////////////////////////////////////////////
template <typename TA, typename TB>
struct Pair
{
    TA a;
    TB b;
};

using FnPtr = void (*)();

// Pointers must not turn into `const int*&` via textual `const __VA_ARGS__&`
static_assert(ZA_IS_COPY_ASSIGNABLE(int*));
static_assert(ZA_IS_COPY_CONSTRUCTIBLE(int*));
static_assert(ZA_IS_TRIVIALLY_COPY_ASSIGNABLE(int*));
static_assert(ZA_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(int*));
static_assert(ZA_IS_COPY_ASSIGNABLE(const int*));
static_assert(!ZA_IS_COPY_ASSIGNABLE(int* const));

// Arrays, function pointers, `void`, references, and types with commas
static_assert(!ZA_IS_COPY_ASSIGNABLE(int[3]));
static_assert(!ZA_IS_COPY_CONSTRUCTIBLE(int[3]));
static_assert(!ZA_IS_MOVE_ASSIGNABLE(int[3]));
static_assert(!ZA_IS_MOVE_CONSTRUCTIBLE(int[3]));
static_assert(ZA_IS_COPY_ASSIGNABLE(void (*)()));
static_assert(ZA_IS_COPY_CONSTRUCTIBLE(FnPtr));
static_assert(!za::isCopyAssignable<void>);
static_assert(!za::isCopyConstructible<void>);
static_assert(!za::isMoveAssignable<void>);
static_assert(!za::isMoveConstructible<void>);
static_assert(!za::isNoThrowMoveAssignable<void>);
static_assert(!za::isNoThrowMoveConstructible<void>);
static_assert(!za::isTriviallyCopyAssignable<const void>);
static_assert(!za::isTriviallyMoveConstructible<const void>);
static_assert(ZA_IS_COPY_CONSTRUCTIBLE(int&));
static_assert(ZA_IS_COPY_ASSIGNABLE(int&));
static_assert(!ZA_IS_COPY_ASSIGNABLE(const int));
static_assert(ZA_IS_TRIVIALLY_MOVE_CONSTRUCTIBLE(Pair<int, float>));
static_assert(ZA_IS_NOTHROW_MOVE_ASSIGNABLE(Pair<int, float>));


////////////////////////////////////////////////////////////
struct ThrowingMemberSwap
{
    void swap(ThrowingMemberSwap&)
    {
    }
};

struct NothrowMemberSwap
{
    void swap(NothrowMemberSwap&) noexcept
    {
    }
};

struct ThrowingMove
{
    ThrowingMove(ThrowingMove&&)
    {
    }

    ThrowingMove& operator=(ThrowingMove&&)
    {
        return *this;
    }
};

static_assert(za::isNoThrowSwappable<int>);
static_assert(za::isNoThrowSwappable<int*>);
static_assert(za::isNoThrowSwappable<int[3]>);
static_assert(za::isNoThrowSwappable<Pair<int, float>>);
static_assert(za::isNoThrowSwappable<NothrowMemberSwap>);
static_assert(!za::isNoThrowSwappable<ThrowingMemberSwap>);
static_assert(!za::isNoThrowSwappable<ThrowingMove>);
static_assert(!za::isNoThrowSwappable<ThrowingMove[2]>);
static_assert(!za::isNoThrowSwappable<const int>);


////////////////////////////////////////////////////////////
// cv-qualified and extended types must behave the same on every compiler
static_assert(za::isIntegral<int>);
static_assert(za::isIntegral<const int>);
static_assert(za::isIntegral<volatile unsigned char>);
static_assert(za::isIntegral<const volatile bool>);
static_assert(za::isIntegral<char8_t>);
static_assert(!za::isIntegral<float>);
static_assert(!za::isIntegral<int&>);
static_assert(!za::isIntegral<TraitsTest::E>);

#ifdef __SIZEOF_INT128__
static_assert(za::isIntegral<__int128_t>);
static_assert(za::isIntegral<__uint128_t>);
static_assert(za::isUnsigned<__uint128_t>);
static_assert(!za::isUnsigned<__int128_t>);
static_assert(za::isSame<za::MakeUnsigned<__int128_t>, __uint128_t>);
#endif

static_assert(za::isUnsigned<const unsigned int>);
static_assert(za::isUnsigned<volatile unsigned char>);
static_assert(!za::isUnsigned<const int>);

static_assert(za::isFloatingPoint<const float>);
static_assert(za::isFloatingPoint<volatile double>);
static_assert(za::isFloatingPoint<const volatile long double>);
static_assert(!za::isFloatingPoint<const int>);
static_assert(!ZA_IS_FLOATING_POINT(Pair<float, float>));

static_assert(za::isVoid<void>);
static_assert(za::isVoid<const void>);
static_assert(za::isVoid<volatile void>);
static_assert(za::isVoid<const volatile void>);
static_assert(!za::isVoid<void*>);
static_assert(!za::isVoid<int>);

static_assert(za::isPointer<int*>);
static_assert(za::isPointer<int* const>);
static_assert(za::isPointer<int* const volatile>);
static_assert(!za::isPointer<int>);
static_assert(!za::isPointer<int TraitsTest::Custom0::*>);

static_assert(za::isFunction<void()>);
static_assert(za::isFunction<int(int, float)>);
static_assert(za::isFunction<void() noexcept>);
static_assert(za::isFunction<void(...)>);
static_assert(za::isFunction<void() const&>); // abominable function type
static_assert(!za::isFunction<void (*)()>);
static_assert(!za::isFunction<void (&)()>);
static_assert(!za::isFunction<void (TraitsTest::Custom0::*)()>);
static_assert(!za::isFunction<int>);
static_assert(!za::isFunction<const int>);
static_assert(!za::isFunction<int&>);
static_assert(!za::isFunction<void>);
static_assert(!za::isFunction<int[3]>);
static_assert(!za::isFunction<TraitsTest::Custom0>);
static_assert(!za::isFunction<decltype([] {})>);

static_assert(za::referenceConvertsFromTemporary<const int&, int>);   // prvalue: materialized as a temporary
static_assert(za::referenceConvertsFromTemporary<int&&, int>);        // prvalue: materialized as a temporary
static_assert(za::referenceConvertsFromTemporary<const int&, long&>); // conversion creates a temporary
static_assert(za::referenceConvertsFromTemporary<const TraitsTest::B&, TraitsTest::D>);
static_assert(!za::referenceConvertsFromTemporary<const int&, int&>); // binds directly
static_assert(!za::referenceConvertsFromTemporary<int&&, int&&>);     // binds directly to the xvalue
static_assert(!za::referenceConvertsFromTemporary<const TraitsTest::B&, TraitsTest::D&>); // derived-to-base
static_assert(!za::referenceConvertsFromTemporary<int, int>);                             // not a reference
static_assert(!za::referenceConvertsFromTemporary<int&, long&>);                          // not convertible at all

static_assert(za::isInvocableR<int (*)(), int>);
static_assert(za::isInvocableR<int (*)(), long>);
static_assert(za::isInvocableR<int (*)(), void>);
static_assert(za::isInvocableR<int (*)(), const void>);
static_assert(za::isInvocableR<int& (*)(), const int&>);
static_assert(!za::isInvocableR<int (*)(), const int&>);   // would dangle
static_assert(!za::isInvocableR<long& (*)(), const int&>); // would dangle (converted temporary)
static_assert(!za::isInvocableR<int (*)(int), int>);       // wrong arguments
static_assert(!za::isInvocableR<int, int>);                // not callable

static_assert(za::isMemberPointer<int TraitsTest::Custom0::*>);
static_assert(za::isMemberPointer<int TraitsTest::Custom0::* const>);
static_assert(!za::isMemberPointer<int*>);

static_assert(za::isArray<int[3]>);
static_assert(za::isArray<int[]>);
static_assert(!za::isArray<int*>);


////////////////////////////////////////////////////////////
static_assert(za::isSame<za::MakeUnsigned<char>, unsigned char>);
static_assert(za::isSame<za::MakeUnsigned<signed char>, unsigned char>);
static_assert(za::isSame<za::MakeUnsigned<int>, unsigned int>);
static_assert(za::isSame<za::MakeUnsigned<unsigned long long>, unsigned long long>);
static_assert(za::isSame<za::MakeUnsigned<const int>, const unsigned int>);
static_assert(za::isSame<za::MakeUnsigned<volatile short>, volatile unsigned short>);
static_assert(za::isSame<za::MakeUnsigned<const volatile long>, const volatile unsigned long>);
static_assert(za::isSame<za::MakeUnsigned<char8_t>, unsigned char>);
static_assert(za::isSame<za::MakeUnsigned<char16_t>, unsigned short>);
static_assert(za::isSame<za::MakeUnsigned<char32_t>, unsigned int>);
static_assert(za::isSame<za::MakeUnsigned<TraitsTest::E>, unsigned int>);
static_assert(za::isSame<za::MakeUnsigned<TraitsTest::EC>, unsigned int>);
static_assert(sizeof(za::MakeUnsigned<wchar_t>) == sizeof(wchar_t));
static_assert(za::isUnsigned<za::MakeUnsigned<wchar_t>>);


////////////////////////////////////////////////////////////
static_assert(za::isSame<za::CommonType<void, void>, void>);
static_assert(za::isSame<za::CommonType<const void, void>, void>);
static_assert(za::isSame<za::CommonType<int, long>, long>);
static_assert(za::isSame<za::CommonType<int&, const int&>, int>);
static_assert(za::isSame<za::CommonType<TraitsTest::D*, TraitsTest::B*>, TraitsTest::B*>);
static_assert(za::isSame<za::CommonType<char, short, int, float>, float>);


////////////////////////////////////////////////////////////
static_assert(za::isSame<za::Conditional<true, int, float>, int>);
static_assert(za::isSame<za::Conditional<false, int, float>, float>);


////////////////////////////////////////////////////////////
static_assert(za::isTriviallyDefaultConstructible<int>);
static_assert(za::isTriviallyDefaultConstructible<TraitsTest::Custom0>);
static_assert(!za::isTriviallyDefaultConstructible<int&>);
static_assert(!za::isTriviallyDefaultConstructible<TraitsTest::Custom1>);
static_assert(!za::isTriviallyDefaultConstructible<NothrowMemberSwap[2]> ||
              za::isTriviallyDefaultConstructible<NothrowMemberSwap[2]>); // must compile


////////////////////////////////////////////////////////////
static_assert(ZA_IS_SAME(ZA_REMOVE_REFERENCE(int (&)[3]), int[3]));
static_assert(ZA_IS_SAME(ZA_REMOVE_REFERENCE(void (&&)()), void()));


////////////////////////////////////////////////////////////
namespace TraitsTest
{
inline constexpr int regularizeVoidGlobal = 0;

struct RefQualifiedCall
{
    constexpr void operator()() &
    {
    }

    constexpr int operator()() &&
    {
        return 0;
    }
};

struct NonCopyable
{
    NonCopyable()                   = default;
    NonCopyable(const NonCopyable&) = delete;
};

inline NonCopyable nonCopyableGlobal;

} // namespace TraitsTest

static_assert(ZA_IS_SAME(decltype(za::regularizeVoid([] {})), za::RegularizeVoidDummy));
static_assert(ZA_IS_SAME(decltype(za::regularizeVoid([] { return 1; })), int));
static_assert(za::regularizeVoid([] { return 42; }) == 42);

// References are forwarded rather than copied
static_assert(ZA_IS_SAME(decltype(za::regularizeVoid([]() -> const int& { return TraitsTest::regularizeVoidGlobal; })),
                         const int&));
static_assert(&za::regularizeVoid([]() -> const int& { return TraitsTest::regularizeVoidGlobal; }) ==
              &TraitsTest::regularizeVoidGlobal);
static_assert(ZA_IS_SAME(decltype(za::regularizeVoid([]() -> TraitsTest::NonCopyable&
{ return TraitsTest::nonCopyableGlobal; })),
                         TraitsTest::NonCopyable&));

// The `void` check uses the same (forwarded) invocation as the call itself
static_assert(ZA_IS_SAME(decltype(za::regularizeVoid(TraitsTest::RefQualifiedCall{})), int));
static_assert(ZA_IS_SAME(decltype(za::regularizeVoid(za::declVal<TraitsTest::RefQualifiedCall&>())),
                         za::RegularizeVoidDummy));


////////////////////////////////////////////////////////////
namespace TraitsTest
{
union U
{
    int   i;
    float f;
};

struct DeletedDtor
{
    ~DeletedDtor() = delete;
};

class PrivateDtor
{
    ~PrivateDtor() = default;
};

struct TrivialDtor
{
    int i;
};

using Abominable = void() const;

#ifdef __SIZEOF_INT128__
enum class E128 : __int128_t
{
};
#endif

} // namespace TraitsTest


////////////////////////////////////////////////////////////
static_assert(ZA_IS_SAME(za::RemoveCVRefIndirect<const volatile int&>, int));
static_assert(ZA_IS_SAME(za::RemoveCVRefIndirect<const int&&>, int));
static_assert(ZA_IS_SAME(za::RemoveCVRefIndirect<const int*>, const int*));
static_assert(ZA_IS_SAME(za::RemoveCVRefIndirect<const int (&)[3]>, int[3]));
static_assert(ZA_IS_SAME(za::RemoveCVRefIndirect<void (&)()>, void()));


////////////////////////////////////////////////////////////
static_assert(ZA_IS_SAME(za::RemoveCV<const volatile int>, int));
static_assert(ZA_IS_SAME(za::RemoveCV<const int&>, const int&)); // not through references
static_assert(ZA_IS_SAME(za::RemoveCV<const int*>, const int*)); // not through pointers
static_assert(ZA_IS_SAME(za::RemoveCV<int* const>, int*));
static_assert(ZA_IS_SAME(za::RemoveCV<const int[3]>, int[3]));


////////////////////////////////////////////////////////////
static_assert(za::isSame<za::CopyCV<int, float>, float>);
static_assert(za::isSame<za::CopyCV<const int, float>, const float>);
static_assert(za::isSame<za::CopyCV<volatile int, float>, volatile float>);
static_assert(za::isSame<za::CopyCV<const volatile int, float>, const volatile float>);
static_assert(za::isSame<za::CopyCV<int, const float>, const float>); // adds, never removes
static_assert(za::isSame<za::CopyCV<const int&, float>, float>);      // `const int&` itself is not `const`


////////////////////////////////////////////////////////////
static_assert(za::isConst<const int>);
static_assert(za::isConst<const volatile int>);
static_assert(za::isConst<int* const>);
static_assert(za::isConst<const int[3]>);
static_assert(!za::isConst<int>);
static_assert(!za::isConst<const int*>);
static_assert(!za::isConst<const int&>);
static_assert(!za::isConst<void()>);


////////////////////////////////////////////////////////////
static_assert(za::isReference<int&>);
static_assert(za::isReference<const int&&>);
static_assert(za::isReference<void (&)()>);
static_assert(!za::isReference<int>);
static_assert(!za::isReference<int*>);
static_assert(!za::isReference<void>);


////////////////////////////////////////////////////////////
static_assert(za::isClass<TraitsTest::B>);
static_assert(za::isClass<const TraitsTest::B>);
static_assert(!za::isClass<TraitsTest::U>); // unions are not classes
static_assert(!za::isClass<TraitsTest::B&>);
static_assert(!za::isClass<TraitsTest::E>);
static_assert(!za::isClass<int>);


////////////////////////////////////////////////////////////
static_assert(za::isUnion<TraitsTest::U>);
static_assert(za::isUnion<const TraitsTest::U>);
static_assert(!za::isUnion<TraitsTest::B>);
static_assert(!za::isUnion<TraitsTest::U&>);
static_assert(!za::isUnion<int>);


////////////////////////////////////////////////////////////
static_assert(ZA_IS_SAME(za::AddPointer<int>, int*));
static_assert(ZA_IS_SAME(za::AddPointer<const int>, const int*));
static_assert(ZA_IS_SAME(za::AddPointer<int&>, int*));
static_assert(ZA_IS_SAME(za::AddPointer<const int&&>, const int*));
static_assert(ZA_IS_SAME(za::AddPointer<void>, void*));
static_assert(ZA_IS_SAME(za::AddPointer<void()>, void (*)()));
static_assert(ZA_IS_SAME(za::AddPointer<void (&)()>, void (*)()));
static_assert(ZA_IS_SAME(za::AddPointer<TraitsTest::Abominable>, TraitsTest::Abominable)); // cannot form a pointer


////////////////////////////////////////////////////////////
static_assert(ZA_IS_TRIVIALLY_DESTRUCTIBLE(TraitsTest::TrivialDtor));
static_assert(ZA_IS_TRIVIALLY_DESTRUCTIBLE(TraitsTest::TrivialDtor[4]));
static_assert(ZA_IS_TRIVIALLY_DESTRUCTIBLE(int&));
static_assert(ZA_IS_TRIVIALLY_DESTRUCTIBLE(TraitsTest::NonTrivial&&));
static_assert(ZA_IS_TRIVIALLY_DESTRUCTIBLE(int*));
static_assert(!ZA_IS_TRIVIALLY_DESTRUCTIBLE(TraitsTest::NonTrivial[4]));
static_assert(!ZA_IS_TRIVIALLY_DESTRUCTIBLE(int[])); // unbounded arrays cannot be destroyed
static_assert(!ZA_IS_TRIVIALLY_DESTRUCTIBLE(TraitsTest::DeletedDtor));
static_assert(!ZA_IS_TRIVIALLY_DESTRUCTIBLE(TraitsTest::PrivateDtor));
static_assert(!ZA_IS_TRIVIALLY_DESTRUCTIBLE(void));
static_assert(!ZA_IS_TRIVIALLY_DESTRUCTIBLE(void()));


////////////////////////////////////////////////////////////
// Extended types follow Clang/libc++ (implementation-defined, see `IsFloatingPoint.hpp` and `MakeUnsigned.hpp`)
#ifdef __SIZEOF_FLOAT128__
static_assert(za::isFloatingPoint<__float128>);
static_assert(za::isFloatingPoint<const __float128>);
#endif

#ifdef __SIZEOF_INT128__
static_assert(za::isSame<za::MakeUnsigned<TraitsTest::E128>, __uint128_t>);
static_assert(za::isSame<za::MakeUnsigned<const TraitsTest::E128>, const __uint128_t>);
#endif

} // namespace
