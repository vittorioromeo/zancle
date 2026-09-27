#include "Tst/Tst.hpp"

#include "Zancle/Container/AnkerlUnorderedDense.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/SizeT.hpp"


TEST_CASE("[Base] za::ankerl::map::replace_key")
{
    using Map = ankerl::unordered_dense::map<za::U64, za::U64>;

    SECTION("Successful replacement returns {it, true}")
    {
        Map m;
        m.try_emplace(1u, 100u);
        m.try_emplace(2u, 200u);
        m.try_emplace(3u, 300u);

        auto* const it = m.find(2u);
        REQUIRE(it != m.end());

        const auto [newIt, ok] = m.replace_key(it, 42u);
        CHECK(ok);
        CHECK(newIt == it); // same iterator position
        CHECK(newIt->first == 42u);
        CHECK(newIt->second == 200u); // value preserved

        // Old key no longer present.
        CHECK(m.find(2u) == m.end());
        // New key is found.
        auto* const found = m.find(42u);
        REQUIRE(found != m.end());
        CHECK(found->second == 200u);

        // Other entries untouched.
        CHECK(m.find(1u)->second == 100u);
        CHECK(m.find(3u)->second == 300u);

        // Size unchanged.
        CHECK(m.size() == 3u);
    }

    SECTION("Replacement to an existing key fails and returns {iter-to-existing, false}")
    {
        Map m;
        m.try_emplace(1u, 100u);
        m.try_emplace(2u, 200u);

        auto* const it1 = m.find(1u);
        REQUIRE(it1 != m.end());

        const auto [retIt, ok] = m.replace_key(it1, 2u);
        CHECK(!ok);
        CHECK(retIt != m.end());
        CHECK(retIt->first == 2u);
        CHECK(retIt->second == 200u);

        // Both original keys still present, untouched.
        CHECK(m.find(1u) != m.end());
        CHECK(m.find(1u)->second == 100u);
        CHECK(m.find(2u)->second == 200u);
        CHECK(m.size() == 2u);
    }

    SECTION("Replace many keys preserves all values")
    {
        Map m;
        for (za::U64 i = 0; i < 100u; ++i)
            m.try_emplace(i, i * 10u);

        // Shift every key by +1000.
        for (za::U64 i = 0; i < 100u; ++i)
        {
            auto* const it = m.find(i);
            REQUIRE(it != m.end());
            const auto [newIt, ok] = m.replace_key(it, i + 1000u);
            CHECK(ok);
            CHECK(newIt->second == i * 10u);
        }

        CHECK(m.size() == 100u);

        // Verify all new keys present with correct values.
        for (za::U64 i = 0; i < 100u; ++i)
        {
            auto* const it = m.find(i + 1000u);
            REQUIRE(it != m.end());
            CHECK(it->second == i * 10u);
        }

        // None of the old keys remain.
        for (za::U64 i = 0; i < 100u; ++i)
            CHECK(m.find(i) == m.end());
    }

    SECTION("Replace with the same key reports failure and changes nothing")
    {
        Map m;
        m.try_emplace(5u, 500u);

        auto* const it = m.find(5u);
        REQUIRE(it != m.end());

        // replace_key(it, same_key) hits the "already exists" branch and
        // returns {it_to_existing, false}.
        const auto [retIt, ok] = m.replace_key(it, 5u);
        CHECK(!ok);
        CHECK(retIt == it);
        CHECK(retIt->first == 5u);
        CHECK(retIt->second == 500u);
        CHECK(m.size() == 1u);
    }
}


namespace
{
namespace AnkerlUnorderedDenseReplaceKeyTest // unity build
{
////////////////////////////////////////////////////////////
/// Maps every key to one of `NBuckets` hashes: forces long probe chains
///
/// Not avalanching: the table runs the result through wyhash, so the
/// colliding keys all land in the same home bucket.
////////////////////////////////////////////////////////////
template <za::U64 NBuckets>
struct FewHashes
{
    [[nodiscard]] za::U64 operator()(const za::U64 x) const noexcept
    {
        return x % NBuckets;
    }
};


////////////////////////////////////////////////////////////
/// Avalanching hash placing every key at the very last bucket, so that probe
/// chains wrap around to bucket 0
////////////////////////////////////////////////////////////
struct LastBucketHash
{
    using is_avalanching = void;

    [[nodiscard]] za::U64 operator()(const za::U64) const noexcept
    {
        return ~za::U64{0}; // `hash >> shifts` is the last bucket for any bucket count
    }
};


////////////////////////////////////////////////////////////
template <typename Map>
void checkContents(const Map& m, const za::U64 keyOffset, const za::U64 n)
{
    REQUIRE(m.size() == n);

    for (za::U64 i = 0u; i < n; ++i)
    {
        const auto* const it = m.find(i + keyOffset);
        REQUIRE(it != m.end());
        CHECK(it->second == i * 10u);
    }
}


////////////////////////////////////////////////////////////
template <typename Map>
void replaceAllKeys(const za::U64 n)
{
    Map m;
    for (za::U64 i = 0u; i < n; ++i)
        m.try_emplace(i, i * 10u);

    checkContents(m, 0u, n);

    // Replace keys in an order that shuffles the chains.
    for (za::U64 i = 0u; i < n; ++i)
    {
        const za::U64 k  = (i * 7u) % n;
        auto* const   it = m.find(k);
        REQUIRE(it != m.end());

        const auto [newIt, ok] = m.replace_key(it, k + 1000u);
        CHECK(ok);
        CHECK(newIt == it);
        CHECK(newIt->second == k * 10u);

        // Every key (replaced or not) must still be reachable after each step.
        for (za::U64 j = 0u; j < n; ++j)
        {
            const bool replaced = [&]
            {
                for (za::U64 r = 0u; r <= i; ++r)
                    if ((r * 7u) % n == j)
                        return true;

                return false;
            }();

            const auto* const found = m.find(replaced ? j + 1000u : j);
            REQUIRE(found != m.end());
            CHECK(found->second == j * 10u);
            CHECK(m.find(replaced ? j : j + 1000u) == m.end());
        }
    }

    checkContents(m, 1000u, n);

    // Collisions with an existing key are rejected.
    const auto [retIt, ok] = m.replace_key(m.find(1000u), 1001u);
    CHECK(!ok);
    CHECK(retIt->first == 1001u);
    checkContents(m, 1000u, n);

    // Replace back after erasing a few entries.
    CHECK(m.erase(1000u) == 1u);
    CHECK(m.erase(1003u) == 1u);
    CHECK(m.replace_key(m.find(1001u), 0u).second);
    CHECK(m.find(0u)->second == 10u);
    CHECK(m.find(1001u) == m.end());
    CHECK(m.size() == n - 2u);
}

} // namespace AnkerlUnorderedDenseReplaceKeyTest
} // namespace


TEST_CASE("[Base] za::ankerl::map::replace_key with collisions")
{
    using namespace AnkerlUnorderedDenseReplaceKeyTest;

    SECTION("All keys share one hash")
    {
        replaceAllKeys<ankerl::unordered_dense::map<za::U64, za::U64, FewHashes<1u>>>(20u);
    }

    SECTION("Keys share a few hashes")
    {
        replaceAllKeys<ankerl::unordered_dense::map<za::U64, za::U64, FewHashes<3u>>>(40u);
    }

    SECTION("Probe chains wrap around the end of the bucket array")
    {
        using Map = ankerl::unordered_dense::map<za::U64, za::U64, LastBucketHash>;

        // Bucket count stays small (8 buckets for 5 elements), so the chain
        // starting at the last bucket wraps around to buckets 0, 1, ...
        replaceAllKeys<Map>(5u);

        Map m;
        for (za::U64 i = 0u; i < 5u; ++i)
            m.try_emplace(i, i * 10u);

        CHECK(m.bucket_count() == 8u);
        checkContents(m, 0u, 5u);
    }
}


// `segmented_vector` is currently disabled (see `AnkerlUnorderedDense.hpp`),
// its tests are kept for when it gets re-enabled.
#if 0
TEST_CASE("[Base] za::ankerl::segmented_vector::resize")
{
    using Vec = ankerl::unordered_dense::v4_8_1::segmented_vector<za::U64>;

    SECTION("Grow with default value-init")
    {
        Vec v;
        v.resize(5u);
        CHECK(v.size() == 5u);
        for (za::SizeT i = 0; i < 5u; ++i)
            CHECK(v[i] == 0u);
    }

    SECTION("Grow with explicit value")
    {
        Vec v;
        v.resize(4u, 42u);
        CHECK(v.size() == 4u);
        for (za::SizeT i = 0; i < 4u; ++i)
            CHECK(v[i] == 42u);
    }

    SECTION("Shrink drops trailing elements")
    {
        Vec v;
        v.emplace_back(1u);
        v.emplace_back(2u);
        v.emplace_back(3u);
        v.resize(1u);
        CHECK(v.size() == 1u);
        CHECK(v[0] == 1u);
    }

    SECTION("Resize to current size is a no-op")
    {
        Vec v;
        v.emplace_back(7u);
        v.emplace_back(8u);
        v.resize(2u);
        CHECK(v.size() == 2u);
        CHECK(v[0] == 7u);
        CHECK(v[1] == 8u);
    }

    SECTION("Resize across segment boundary")
    {
        Vec v;
        // segment size is at least 1 element; force growth past one segment.
        v.resize(1000u, 99u);
        CHECK(v.size() == 1000u);
        CHECK(v[0] == 99u);
        CHECK(v[500] == 99u);
        CHECK(v[999] == 99u);

        v.resize(10u);
        CHECK(v.size() == 10u);
        CHECK(v[0] == 99u);
        CHECK(v[9] == 99u);
    }
}


TEST_CASE("[Base] za::ankerl::segmented_vector iterator is random-access")
{
    using Vec = ankerl::unordered_dense::v4_8_1::segmented_vector<za::U64>;

    Vec v;
    for (za::U64 i = 0; i < 10u; ++i)
        v.emplace_back(i);

    auto it = v.begin();

    SECTION("operator++ / operator--")
    {
        ++it;
        CHECK(*it == 1u);
        it++;
        CHECK(*it == 2u);
        --it;
        CHECK(*it == 1u);
        it--;
        CHECK(*it == 0u);
    }

    SECTION("operator+= / operator-=")
    {
        it += 5;
        CHECK(*it == 5u);
        it -= 3;
        CHECK(*it == 2u);
    }

    SECTION("operator+ scalar / operator- scalar")
    {
        const auto it2 = it + 7;
        CHECK(*it2 == 7u);
        const auto it3 = it2 - 4;
        CHECK(*it3 == 3u);
    }

    SECTION("operator- (iterator difference)")
    {
        const auto a = v.begin();
        const auto b = a + 6;
        CHECK(b - a == 6);
    }

    SECTION("Ordering comparisons")
    {
        const auto a = v.begin();
        const auto b = a + 3;
        // Wrap each comparison in `static_cast<bool>` so the framework's expression
        // decomposer doesn't need to stringify iterator operands.
        CHECK(static_cast<bool>(a < b));
        CHECK(static_cast<bool>(b > a));
        CHECK(static_cast<bool>(a <= b));
        CHECK(static_cast<bool>(b >= a));
        CHECK(static_cast<bool>(a <= a));
        CHECK(static_cast<bool>(a >= a));
        CHECK(!static_cast<bool>(b < a));
        CHECK(!static_cast<bool>(a > b));
    }
}
#endif
