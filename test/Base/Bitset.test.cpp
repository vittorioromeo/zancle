#include "Tst/Tst.hpp"

#include "Zancle/Container/Bitset.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/Popcountll.hpp"
#include "Zancle/Base/SizeT.hpp"

#include "Zancle/Trait/IsTriviallyRelocatable.hpp"


namespace
{
using za::Bitset;
using za::SizeT;
using za::U64;
} // namespace


////////////////////////////////////////////////////////////
// Compile-time properties
////////////////////////////////////////////////////////////
static_assert(ZA_IS_TRIVIALLY_RELOCATABLE(Bitset<1>));
static_assert(ZA_IS_TRIVIALLY_RELOCATABLE(Bitset<64>));
static_assert(ZA_IS_TRIVIALLY_RELOCATABLE(Bitset<65>));
static_assert(ZA_IS_TRIVIALLY_RELOCATABLE(Bitset<256>));


////////////////////////////////////////////////////////////
// One word covers `<= 64`, two words cover `<= 128`, etc.
////////////////////////////////////////////////////////////
static_assert(sizeof(Bitset<1>) == sizeof(U64));
static_assert(sizeof(Bitset<32>) == sizeof(U64));
static_assert(sizeof(Bitset<63>) == sizeof(U64));
static_assert(sizeof(Bitset<64>) == sizeof(U64));
static_assert(sizeof(Bitset<65>) == 2 * sizeof(U64));
static_assert(sizeof(Bitset<128>) == 2 * sizeof(U64));
static_assert(sizeof(Bitset<129>) == 3 * sizeof(U64));
static_assert(sizeof(Bitset<256>) == 4 * sizeof(U64));


////////////////////////////////////////////////////////////
// `size()` is a static constexpr.
////////////////////////////////////////////////////////////
static_assert(Bitset<1>::size() == 1u);
static_assert(Bitset<100>::size() == 100u);


////////////////////////////////////////////////////////////
// Constexpr default ctor: all bits zero.
////////////////////////////////////////////////////////////
static_assert(Bitset<32>{}.none());
static_assert(Bitset<32>{}.count() == 0u);
static_assert(!Bitset<32>{}.any());
static_assert(!Bitset<32>{}.all());


////////////////////////////////////////////////////////////
// Constexpr value ctor.
////////////////////////////////////////////////////////////
static_assert(Bitset<8>{0xFFu}.count() == 8u);
static_assert(Bitset<8>{0xFFu}.all());
static_assert(Bitset<8>{0xFFu}.toU64() == 0xFFu);
static_assert(Bitset<8>{0xFF'FFu}.toU64() == 0xFFu); // high bits beyond N are masked
static_assert(Bitset<64>{0xDE'AD'BE'EF'CA'FE'BA'BEull}.toU64() == 0xDE'AD'BE'EF'CA'FE'BA'BEull);


////////////////////////////////////////////////////////////
// Trailing-zero invariant after `setAll` / `flipAll`.
//
// A naive bit-bashing impl that didn't mask the trailing word
// would make `setAll` produce a value where `count() != N` and
// where `~empty != setAll`. We assert via `count() == N` here,
// and indirectly via the `==` checks below.
////////////////////////////////////////////////////////////
static_assert([]
{
    Bitset<100> b;
    b.setAll();
    return b.count() == 100u && b.all();
}());

static_assert([]
{
    Bitset<100> b;
    b.flipAll();
    return b.count() == 100u && b.all();
}());


////////////////////////////////////////////////////////////
TEST_CASE("[Base] za::Bitset - default state")
{
    Bitset<128> b;
    CHECK(b.size() == 128u);
    CHECK(b.count() == 0u);
    CHECK(!b.any());
    CHECK(b.none());
    CHECK(!b.all());

    for (SizeT i = 0u; i < 128u; ++i)
        CHECK(!b.test(i));
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] za::Bitset - set / reset / flip / setBit on a single bit")
{
    Bitset<200> b;

    b.set(7);
    b.set(63);
    b.set(64);
    b.set(199);

    CHECK(b.count() == 4u);
    CHECK(b.test(7));
    CHECK(b.test(63));
    CHECK(b.test(64));
    CHECK(b.test(199));
    CHECK(!b.test(0));
    CHECK(!b.test(8));
    CHECK(!b.test(198));

    b.reset(63);
    CHECK(!b.test(63));
    CHECK(b.count() == 3u);

    b.flip(7);
    CHECK(!b.test(7));
    CHECK(b.count() == 2u);

    b.flip(7);
    CHECK(b.test(7));
    CHECK(b.count() == 3u);

    b.setBit(0, true);
    CHECK(b.test(0));
    b.setBit(0, false);
    CHECK(!b.test(0));
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] za::Bitset - operator[] reads")
{
    Bitset<10> b{0b0000010101u};
    CHECK(b[0]);
    CHECK(!b[1]);
    CHECK(b[2]);
    CHECK(!b[3]);
    CHECK(b[4]);
    CHECK(!b[5]);
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] za::Bitset - setAll / resetAll across word boundaries")
{
    // Pick sizes that exercise: exactly one word, partial trailing
    // word, and exactly two full words.
    {
        Bitset<64> b;
        b.setAll();
        CHECK(b.count() == 64u);
        CHECK(b.all());

        b.resetAll();
        CHECK(b.count() == 0u);
        CHECK(b.none());
    }

    {
        Bitset<100> b;
        b.setAll();
        CHECK(b.count() == 100u);
        CHECK(b.all());
        for (SizeT i = 0u; i < 100u; ++i)
            CHECK(b.test(i));
    }

    {
        Bitset<128> b;
        b.setAll();
        CHECK(b.count() == 128u);
        CHECK(b.all());
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] za::Bitset - flipAll preserves the trailing-zero invariant")
{
    // If `flipAll` left the unused bits of the trailing word at 1,
    // `count()` would observe more bits than `N` and `==` would
    // wrongly compare two equal-by-content bitsets unequal.
    Bitset<70> b;
    b.flipAll();
    CHECK(b.count() == 70u);
    CHECK(b.all());

    Bitset<70> c;
    c.setAll();
    CHECK(static_cast<bool>(b == c));
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] za::Bitset - operator~ preserves the trailing-zero invariant")
{
    Bitset<70> b;
    b.set(5);
    b.set(64);

    const Bitset<70> n = ~b;

    CHECK(n.count() == 70u - 2u);
    CHECK(!n.test(5));
    CHECK(!n.test(64));
    CHECK(n.test(0));
    CHECK(n.test(69));
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] za::Bitset - bitwise compound and free operators")
{
    Bitset<128> a;
    Bitset<128> b;

    a.set(0);
    a.set(64);
    a.set(127);

    b.set(0);
    b.set(63);
    b.set(127);

    SECTION("AND")
    {
        const auto c = a & b;
        CHECK(c.count() == 2u);
        CHECK(c.test(0));
        CHECK(c.test(127));
        CHECK(!c.test(63));
        CHECK(!c.test(64));
    }

    SECTION("OR")
    {
        const auto c = a | b;
        CHECK(c.count() == 4u);
        CHECK(c.test(0));
        CHECK(c.test(63));
        CHECK(c.test(64));
        CHECK(c.test(127));
    }

    SECTION("XOR")
    {
        const auto c = a ^ b;
        CHECK(c.count() == 2u);
        CHECK(!c.test(0));
        CHECK(c.test(63));
        CHECK(c.test(64));
        CHECK(!c.test(127));
    }

    SECTION("Compound assigns mutate in place")
    {
        Bitset<128> x = a;
        x &= b;
        CHECK(static_cast<bool>(x == (a & b)));

        Bitset<128> y = a;
        y |= b;
        CHECK(static_cast<bool>(y == (a | b)));

        Bitset<128> z = a;
        z ^= b;
        CHECK(static_cast<bool>(z == (a ^ b)));
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] za::Bitset - equality")
{
    Bitset<200> a;
    Bitset<200> b;

    CHECK(static_cast<bool>(a == b));

    a.set(42);
    CHECK(static_cast<bool>(a != b));

    b.set(42);
    CHECK(static_cast<bool>(a == b));

    a.flipAll();
    b.flipAll();
    CHECK(static_cast<bool>(a == b));
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] za::Bitset - findFirstSet / findNextSet")
{
    Bitset<200> b;

    SECTION("Empty bitset")
    {
        CHECK(b.findFirstSet() == 200u);
        CHECK(b.findNextSet(0u) == 200u);
        CHECK(b.findNextSet(199u) == 200u);
        CHECK(b.findNextSet(200u) == 200u); // past-end query
    }

    SECTION("Single bit at the very start")
    {
        b.set(0);
        CHECK(b.findFirstSet() == 0u);
        CHECK(b.findNextSet(0u) == 0u);
        CHECK(b.findNextSet(1u) == 200u);
    }

    SECTION("Bits scattered across word boundaries")
    {
        b.set(7);
        b.set(63);
        b.set(64);
        b.set(127);
        b.set(199);

        CHECK(b.findFirstSet() == 7u);
        CHECK(b.findNextSet(0u) == 7u);
        CHECK(b.findNextSet(7u) == 7u);
        CHECK(b.findNextSet(8u) == 63u);
        CHECK(b.findNextSet(63u) == 63u);
        CHECK(b.findNextSet(64u) == 64u);
        CHECK(b.findNextSet(65u) == 127u);
        CHECK(b.findNextSet(128u) == 199u);
        CHECK(b.findNextSet(200u) == 200u);
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] za::Bitset - forEachSet visits every set bit in ascending order")
{
    Bitset<200> b;

    constexpr SizeT bitsToSet[]{0u, 7u, 31u, 63u, 64u, 65u, 128u, 199u};

    for (const auto i : bitsToSet)
        b.set(i);

    za::Vector<SizeT> visited;
    b.forEachSet([&](const SizeT i) { visited.pushBack(i); });

    REQUIRE(visited.size() == sizeof(bitsToSet) / sizeof(bitsToSet[0]));

    for (SizeT i = 0u; i < visited.size(); ++i)
        CHECK(visited[i] == bitsToSet[i]);
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] za::Bitset - any / all / none across word boundaries")
{
    SECTION("N == 64 (single full word)")
    {
        Bitset<64> b;
        CHECK(b.none());
        CHECK(!b.any());
        CHECK(!b.all());
        b.setAll();
        CHECK(!b.none());
        CHECK(b.any());
        CHECK(b.all());
        b.reset(0);
        CHECK(!b.none());
        CHECK(b.any());
        CHECK(!b.all());
    }

    SECTION("N == 65 (one full word + one bit)")
    {
        Bitset<65> b;
        CHECK(b.none());
        CHECK(!b.any());
        CHECK(!b.all());

        b.setAll();
        CHECK(b.all());
        b.reset(64); // the lone bit in the trailing word
        CHECK(!b.all());
        CHECK(b.any());

        b.set(64);
        CHECK(b.all());
        b.reset(0);
        CHECK(!b.all());
        CHECK(b.any());
    }

    SECTION("N == 128 (two full words)")
    {
        Bitset<128> b;
        b.setAll();
        CHECK(b.all());
        b.reset(127);
        CHECK(!b.all());
        CHECK(b.any());
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] za::Bitset - toU64 round-trip for N <= 64")
{
    constexpr U64 pattern = 0xDE'AD'BE'EF'CA'FE'BA'BEull;

    Bitset<64> b{pattern};
    CHECK(b.toU64() == pattern);
    CHECK(b.count() == static_cast<SizeT>(ZA_POPCOUNTLL(pattern)));

    Bitset<32> c{pattern};
    CHECK(c.toU64() == (pattern & 0xFF'FF'FF'FFull));
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] za::Bitset - small bitsets work edge cases")
{
    // N == 1: minimal supported size.
    Bitset<1> b;
    CHECK(b.size() == 1u);
    CHECK(!b.test(0));
    b.set(0);
    CHECK(b.test(0));
    CHECK(b.all());
    CHECK(b.count() == 1u);
    b.flip(0);
    CHECK(b.none());
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] za::Bitset - left shift (operator<<= / operator<<)")
{
    SECTION("Shift by 0 is identity")
    {
        Bitset<200> b;
        b.set(7);
        b.set(63);
        b.set(64);
        b.set(199);

        Bitset<200> c = b;
        c <<= 0u;
        CHECK(static_cast<bool>(c == b));
    }

    SECTION("Shift by 1 within a word")
    {
        Bitset<128> b;
        b.set(0);
        b.set(5);
        b.set(31);

        b <<= 1u;
        CHECK(b.count() == 3u);
        CHECK(b.test(1));
        CHECK(b.test(6));
        CHECK(b.test(32));
        CHECK(!b.test(0));
        CHECK(!b.test(5));
        CHECK(!b.test(31));
    }

    SECTION("Shift across the word boundary (n == 1, source bit at 63)")
    {
        Bitset<128> b;
        b.set(63);

        b <<= 1u;
        CHECK(b.count() == 1u);
        CHECK(b.test(64));
        CHECK(!b.test(63));
    }

    SECTION("Shift by exactly bitsPerWord")
    {
        Bitset<200> b;
        b.set(0);
        b.set(63);
        b.set(64);
        b.set(127);

        b <<= 64u;
        CHECK(b.count() == 4u);
        CHECK(b.test(64));
        CHECK(b.test(127));
        CHECK(b.test(128));
        CHECK(b.test(191));
        CHECK(!b.test(0));
        CHECK(!b.test(63));
    }

    SECTION("Shift drops bits past N - 1")
    {
        Bitset<70> b;
        b.set(0);
        b.set(60);
        b.set(69);

        b <<= 5u;
        // Bits originally at 0, 60, 69 → 5, 65, (74 dropped).
        CHECK(b.count() == 2u);
        CHECK(b.test(5));
        CHECK(b.test(65));
    }

    SECTION("Shift by N is all-zero")
    {
        Bitset<128> b;
        b.setAll();

        b <<= 128u;
        CHECK(b.none());
    }

    SECTION("Shift by more than N is all-zero")
    {
        Bitset<128> b;
        b.setAll();

        b <<= 999u;
        CHECK(b.none());
    }

    SECTION("Shift preserves the trailing-zero invariant")
    {
        Bitset<70> b;
        b.setAll();

        b <<= 1u;
        // After shift: bits 1..69 set (bit 0 cleared, original bit 69
        // dropped). count must equal 69, NOT 70 -- if the trailing-word
        // mask was forgotten, we'd see leftover bits in the high word.
        CHECK(b.count() == 69u);
        CHECK(!b.test(0));
        CHECK(b.test(1));
        CHECK(b.test(69));
    }

    SECTION("Free `<<` operator matches the compound form")
    {
        Bitset<200> a;
        a.set(7);
        a.set(63);
        a.set(64);

        Bitset<200> aShifted = a;
        aShifted <<= 5u;

        CHECK(static_cast<bool>((a << 5u) == aShifted));
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] za::Bitset - right shift (operator>>= / operator>>)")
{
    SECTION("Shift by 0 is identity")
    {
        Bitset<200> b;
        b.set(7);
        b.set(63);
        b.set(64);
        b.set(199);

        Bitset<200> c = b;
        c >>= 0u;
        CHECK(static_cast<bool>(c == b));
    }

    SECTION("Shift by 1 within a word")
    {
        Bitset<128> b;
        b.set(1);
        b.set(5);
        b.set(31);

        b >>= 1u;
        CHECK(b.count() == 3u);
        CHECK(b.test(0));
        CHECK(b.test(4));
        CHECK(b.test(30));
        CHECK(!b.test(1));
        CHECK(!b.test(5));
        CHECK(!b.test(31));
    }

    SECTION("Shift across the word boundary (n == 1, source bit at 64)")
    {
        Bitset<128> b;
        b.set(64);

        b >>= 1u;
        CHECK(b.count() == 1u);
        CHECK(b.test(63));
        CHECK(!b.test(64));
    }

    SECTION("Shift by exactly bitsPerWord")
    {
        Bitset<200> b;
        b.set(64);
        b.set(127);
        b.set(128);
        b.set(191);

        b >>= 64u;
        CHECK(b.count() == 4u);
        CHECK(b.test(0));
        CHECK(b.test(63));
        CHECK(b.test(64));
        CHECK(b.test(127));
        CHECK(!b.test(128));
        CHECK(!b.test(191));
    }

    SECTION("Shift drops bits below 0")
    {
        Bitset<70> b;
        b.set(0);
        b.set(5);
        b.set(69);

        b >>= 6u;
        // Bits originally at 0, 5, 69 → (-6 dropped, -1 dropped, 63).
        CHECK(b.count() == 1u);
        CHECK(b.test(63));
    }

    SECTION("Shift by N is all-zero")
    {
        Bitset<128> b;
        b.setAll();

        b >>= 128u;
        CHECK(b.none());
    }

    SECTION("Shift by more than N is all-zero")
    {
        Bitset<128> b;
        b.setAll();

        b >>= 999u;
        CHECK(b.none());
    }

    SECTION("Shift trivially preserves the trailing-zero invariant")
    {
        // Right shift can only ever zero high bits, so the trailing
        // word's invariant holds without an explicit mask. Verify by
        // shifting a fully-set partial-trailing-word bitset and
        // checking count.
        Bitset<70> b;
        b.setAll();

        b >>= 1u;
        // After shift: bits 0..68 set (bit 69 dropped from below).
        // Original bit 0 dropped past index -1.
        CHECK(b.count() == 69u);
        CHECK(b.test(0));
        CHECK(b.test(68));
        CHECK(!b.test(69));
    }

    SECTION("Free `>>` operator matches the compound form")
    {
        Bitset<200> a;
        a.set(7);
        a.set(63);
        a.set(64);
        a.set(199);

        Bitset<200> aShifted = a;
        aShifted >>= 5u;

        CHECK(static_cast<bool>((a >> 5u) == aShifted));
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] za::Bitset - shift round-trip is monotonic for small shifts")
{
    // Shifting left then right by the same amount drops the lowest
    // `n` bits but otherwise preserves the bit pattern. A double-check
    // that lhs/rhs shifts are inverses on the bits that survive.
    Bitset<128> a;
    a.set(0);
    a.set(31);
    a.set(64);
    a.set(95);
    a.set(127);

    constexpr SizeT n = 16u;

    Bitset<128> b = a;
    b <<= n;
    b >>= n;

    // Bits whose left-shifted index `i + n < 128` survive both shifts
    // and end up back at their original position. Bit 127 is dropped
    // by the left shift (127 + 16 = 143 >= 128) and stays gone.
    CHECK(b.test(0));    // 0 -> 16 -> 0
    CHECK(b.test(31));   // 31 -> 47 -> 31
    CHECK(b.test(64));   // 64 -> 80 -> 64
    CHECK(b.test(95));   // 95 -> 111 -> 95
    CHECK(!b.test(127)); // dropped: 127 + 16 = 143 >= 128
    CHECK(b.count() == 4u);
}


////////////////////////////////////////////////////////////
// Shifts are usable in constant expressions (both paths).
////////////////////////////////////////////////////////////
static_assert((Bitset<8>{0b0000'0110u} << 2u).toU64() == 0b0001'1000u);
static_assert((Bitset<8>{0b1100'0000u} << 1u).toU64() == 0b1000'0000u); // top bit dropped
static_assert((Bitset<8>{0b0000'0110u} >> 1u).toU64() == 0b0000'0011u);

static_assert([]
{
    Bitset<130> b;
    b.set(1);
    b.set(129);

    b <<= 70u; // 1 -> 71, 129 dropped
    if (b.count() != 1u || !b.test(71))
        return false;

    b >>= 71u; // 71 -> 0
    return b.count() == 1u && b.test(0);
}());


namespace
{
namespace BitsetTest // for unity builds
{
////////////////////////////////////////////////////////////
/// Naive reference model: one `bool` per bit.
////////////////////////////////////////////////////////////
template <SizeT N>
struct NaiveBits
{
    bool bits[N]{};

    void shiftLeft(const SizeT n)
    {
        for (SizeT i = N; i-- > 0u;)
            bits[i] = (i >= n) ? bits[i - n] : false;
    }

    void shiftRight(const SizeT n)
    {
        for (SizeT i = 0u; i < N; ++i)
            bits[i] = (i + n < N) ? bits[i + n] : false;
    }
};


////////////////////////////////////////////////////////////
/// Deterministic pseudo-random pattern (LCG) written to both models.
////////////////////////////////////////////////////////////
template <SizeT N>
void fillPattern(Bitset<N>& b, NaiveBits<N>& model, U64 seed)
{
    for (SizeT i = 0u; i < N; ++i)
    {
        seed             = seed * 6'364'136'223'846'793'005ull + 1'442'695'040'888'963'407ull;
        const bool value = ((seed >> 33u) & 1u) != 0u;

        b.setBit(i, value);
        model.bits[i] = value;
    }
}


////////////////////////////////////////////////////////////
template <SizeT N>
[[nodiscard]] bool matches(const Bitset<N>& b, const NaiveBits<N>& model)
{
    SizeT expectedCount = 0u;

    for (SizeT i = 0u; i < N; ++i)
    {
        if (b.test(i) != model.bits[i])
            return false;

        expectedCount += model.bits[i] ? 1u : 0u;
    }

    // Also catches stray bits in the unused part of the trailing word.
    return b.count() == expectedCount;
}


////////////////////////////////////////////////////////////
template <SizeT N>
void checkShiftsAgainstModel(const SizeT n)
{
    for (U64 seed = 1u; seed <= 4u; ++seed)
    {
        {
            Bitset<N>    b;
            NaiveBits<N> model;
            fillPattern(b, model, seed);

            b <<= n;
            model.shiftLeft(n < N ? n : N);
            CHECK(matches(b, model));
        }

        {
            Bitset<N>    b;
            NaiveBits<N> model;
            fillPattern(b, model, seed);

            b >>= n;
            model.shiftRight(n < N ? n : N);
            CHECK(matches(b, model));
        }
    }
}

} // namespace BitsetTest
} // namespace


////////////////////////////////////////////////////////////
TEST_CASE("[Base] za::Bitset - single-word shifts (N <= 64)")
{
    using namespace BitsetTest;

    SECTION("N == 1")
    {
        const Bitset<1> b{1u};
        CHECK(static_cast<bool>((b << 0u) == b));
        CHECK(static_cast<bool>((b >> 0u) == b));
        CHECK((b << 1u).none());
        CHECK((b >> 1u).none());
        CHECK((b << 64u).none());
        CHECK((b >> 999u).none());

        checkShiftsAgainstModel<1>(0u);
        checkShiftsAgainstModel<1>(1u);
    }

    SECTION("N == 8")
    {
        const Bitset<8> b{0b1000'0001u};
        CHECK((b << 1u).toU64() == 0b0000'0010u); // bit 7 dropped
        CHECK((b >> 1u).toU64() == 0b0100'0000u); // bit 0 dropped
        CHECK((b << 7u).toU64() == 0b1000'0000u);
        CHECK((b >> 7u).toU64() == 0b0000'0001u);
        CHECK((b << 8u).none());
        CHECK((b >> 8u).none());

        for (SizeT n = 0u; n <= 9u; ++n)
            checkShiftsAgainstModel<8>(n);
    }

    SECTION("N == 63")
    {
        Bitset<63> b;
        b.setAll();

        const Bitset<63> l = b << 1u;
        CHECK(l.count() == 62u); // trailing-zero invariant: bit 63 must not be set
        CHECK(!l.test(0));
        CHECK(l.test(62));

        const Bitset<63> r = b >> 62u;
        CHECK(r.count() == 1u);
        CHECK(r.test(0));

        constexpr SizeT amounts[]{0u, 1u, 31u, 62u, 63u, 64u, 100u};
        for (const SizeT n : amounts)
            checkShiftsAgainstModel<63>(n);
    }

    SECTION("N == 64")
    {
        const Bitset<64> b{0x80'00'00'00'00'00'00'01ull};
        CHECK((b << 1u).toU64() == 0x2ull);
        CHECK((b >> 1u).toU64() == 0x40'00'00'00'00'00'00'00ull);
        CHECK((b << 63u).toU64() == 0x80'00'00'00'00'00'00'00ull);
        CHECK((b >> 63u).toU64() == 0x1ull);
        CHECK((b << 64u).none()); // must not perform a (UB) shift by the word width
        CHECK((b >> 64u).none());

        constexpr SizeT amounts[]{0u, 1u, 32u, 63u, 64u, 65u};
        for (const SizeT n : amounts)
            checkShiftsAgainstModel<64>(n);
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] za::Bitset - multi-word shifts match a naive model")
{
    using namespace BitsetTest;

    SECTION("Bitset<200>, whole-word + partial shift (n == 70)")
    {
        checkShiftsAgainstModel<200>(70u);
    }

    SECTION("Bitset<200>, assorted shift amounts")
    {
        constexpr SizeT amounts[]{0u, 1u, 63u, 64u, 65u, 127u, 128u, 130u, 199u, 200u, 500u};
        for (const SizeT n : amounts)
            checkShiftsAgainstModel<200>(n);
    }

    SECTION("Bitset<65> and Bitset<128>, every shift amount")
    {
        for (SizeT n = 0u; n <= 130u; ++n)
        {
            checkShiftsAgainstModel<65>(n);
            checkShiftsAgainstModel<128>(n);
        }
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] za::Bitset - U64 constructor for N > 64")
{
    constexpr U64 pattern = 0xDE'AD'BE'EF'CA'FE'BA'BEull;

    const Bitset<200> b{pattern};

    CHECK(b.count() == static_cast<SizeT>(ZA_POPCOUNTLL(pattern)));

    for (SizeT i = 0u; i < 64u; ++i)
        CHECK(b.test(i) == (((pattern >> i) & 1u) != 0u));

    for (SizeT i = 64u; i < 200u; ++i)
        CHECK(!b.test(i));

    CHECK(b.findNextSet(64u) == 200u);
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] za::Bitset - findNextSet in the partial trailing word")
{
    Bitset<70> b;

    CHECK(b.findNextSet(64u) == 70u);
    CHECK(b.findNextSet(69u) == 70u);

    b.set(69);
    CHECK(b.findFirstSet() == 69u);
    CHECK(b.findNextSet(0u) == 69u);
    CHECK(b.findNextSet(64u) == 69u);
    CHECK(b.findNextSet(69u) == 69u);
    CHECK(b.findNextSet(70u) == 70u);

    b.set(65);
    CHECK(b.findFirstSet() == 65u);
    CHECK(b.findNextSet(64u) == 65u);
    CHECK(b.findNextSet(66u) == 69u);

    // The unused bits of the trailing word must never be reported as set.
    Bitset<70> c;
    c.flipAll();
    c.reset(69);
    CHECK(c.findNextSet(69u) == 70u);
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] za::Bitset - operator~ on single-word sizes")
{
    {
        const Bitset<1> n = ~Bitset<1>{};
        CHECK(n.count() == 1u);
        CHECK(n.all());
        CHECK(n.toU64() == 1u);
    }

    {
        const Bitset<8> n = ~Bitset<8>{0b0000'0001u};
        CHECK(n.count() == 7u);
        CHECK(n.toU64() == 0xFEu);
    }

    {
        const Bitset<63> n = ~Bitset<63>{};
        CHECK(n.count() == 63u);
        CHECK(n.all());
        CHECK(n.toU64() == 0x7F'FF'FF'FF'FF'FF'FF'FFull);
    }

    {
        const Bitset<64> n = ~Bitset<64>{};
        CHECK(n.count() == 64u);
        CHECK(n.all());
        CHECK(n.toU64() == ~U64{0});
    }
}


////////////////////////////////////////////////////////////
TEST_CASE("[Base] za::Bitset - setBit does not disturb neighbouring bits")
{
    Bitset<130> b;
    b.setAll();

    b.setBit(64, false);
    CHECK(b.count() == 129u);
    CHECK(!b.test(64));
    CHECK(b.test(63));
    CHECK(b.test(65));

    b.setBit(64, true);
    CHECK(b.all());

    b.setBit(129, false);
    CHECK(!b.test(129));
    CHECK(b.count() == 129u);

    b.setBit(129, true);
    CHECK(b.test(129));
    CHECK(b.all());
}
