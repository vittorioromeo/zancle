#include "Zancle/Trait/CommonType.hpp"
#include "Zancle/Trait/Conditional.hpp"
#include "Zancle/Trait/Decay.hpp"
#include "Zancle/Trait/IsArray.hpp"
#include "Zancle/Trait/IsBaseOf.hpp"
#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsEnum.hpp"
#include "Zancle/Trait/IsFloatingPoint.hpp"
#include "Zancle/Trait/IsIntegral.hpp"
#include "Zancle/Trait/IsMemberPointer.hpp"
#include "Zancle/Trait/IsMoveAssignable.hpp"
#include "Zancle/Trait/IsMoveConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"
#include "Zancle/Trait/IsNothrowSwappable.hpp"
#include "Zancle/Trait/IsPointer.hpp"
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
#include "Zancle/Trait/IsUnsigned.hpp"
#include "Zancle/Trait/IsVoid.hpp"
#include "Zancle/Trait/MakeUnsigned.hpp"
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


struct Custom3
{
    using TriviallyRelocatableTag = Custom3;

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
    using TriviallyRelocatableTag = za::Conditional<Enable, ConditionallyRelocatable, void>;

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
static_assert(!za::isNothrowMoveAssignable<void>);
static_assert(!za::isNothrowMoveConstructible<void>);
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

static_assert(za::isNothrowSwappable<int>);
static_assert(za::isNothrowSwappable<int*>);
static_assert(za::isNothrowSwappable<int[3]>);
static_assert(za::isNothrowSwappable<Pair<int, float>>);
static_assert(za::isNothrowSwappable<NothrowMemberSwap>);
static_assert(!za::isNothrowSwappable<ThrowingMemberSwap>);
static_assert(!za::isNothrowSwappable<ThrowingMove>);
static_assert(!za::isNothrowSwappable<ThrowingMove[2]>);
static_assert(!za::isNothrowSwappable<const int>);


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

} // namespace
