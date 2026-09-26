#pragma once


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "ExampleUtils/Xoroshiro128PlusPlusBitGenerator.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Math/Constants.hpp"
#include "Zancle/Math/Cos.hpp"
#include "Zancle/Math/Sin.hpp"
#include "Zancle/Math/Sqrt.hpp"

#include "Zancle/Base/AssertAndAssume.hpp"
#include "Zancle/Base/IntTypes.hpp"

#include "Zancle/Trait/IsIntegral.hpp"
#include "Zancle/Trait/MakeUnsigned.hpp"


////////////////////////////////////////////////////////////
class [[nodiscard]] RNGFast
{
private:
    Xoroshiro128PlusPlusBitGenerator m_engine;

public:
    using SeedType = za::U64; //!< Type used for seeding

    ////////////////////////////////////////////////////////////
    /// \brief Default constructor. Initializes with a fixed internal seed.
    ///
    ////////////////////////////////////////////////////////////
    explicit RNGFast() noexcept = default;

    ////////////////////////////////////////////////////////////
    /// \brief Constructor that initializes the generator with a specific seed.
    ///
    /// \param seed The seed value.
    ///
    ////////////////////////////////////////////////////////////
    explicit RNGFast(const SeedType seed) noexcept : m_engine{seed}
    {
    }

    ////////////////////////////////////////////////////////////
    /// \brief Generates a pseudo-random integer within a specified range `[min, max]`.
    ///
    /// \tparam T An integral type.
    ///
    /// \param min Minimum inclusive value.
    /// \param max Maximum inclusive value.
    ///
    /// \return A pseudo-random integer in the range `[min, max]`.
    ///
    /// \warning Uses modulo biasing, which might be unsuitable for applications
    ///          requiring perfect uniformity, especially with large ranges.
    ///
    ////////////////////////////////////////////////////////////
    template <typename T>
    [[nodiscard, gnu::always_inline, gnu::flatten]] inline T getI(const T min, const T max)
    {
        static_assert(ZA_IS_INTEGRAL(T));

        ZA_ASSERT_AND_ASSUME(min <= max);

        using UnsignedT = ZA_MAKE_UNSIGNED(T);

        const auto unsignedMin = static_cast<UnsignedT>(min);
        const auto unsignedMax = static_cast<UnsignedT>(max);

        const auto range = static_cast<za::U64>(unsignedMax - unsignedMin) + za::U64{1};

        return min + static_cast<T>(m_engine.next() % range);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Generates a pseudo-random float within a specified range `[min, max]`.
    ///
    /// \param min Minimum inclusive value.
    /// \param max Maximum inclusive value.
    ///
    /// \return A pseudo-random float in the range `[min, max]`.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten]] inline float getF(const float min, const float max)
    {
        ZA_ASSERT_AND_ASSUME(min <= max);

        // Returns a float in the inclusive range [min, max].

        // We extract 24 random bits, which is enough to fill the 23-bit mantissa of a float,
        // and normalize by dividing by (2^24 - 1).

        const auto  randomBits = static_cast<za::U32>(m_engine.next() >> (64u - 24u));  // Extract 24 bits.
        const float normalized = static_cast<float>(randomBits) / float{(1 << 24) - 1}; // Normalize to [0, 1].

        return min + normalized * (max - min);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Generates a random 2D vector with components in specified ranges.
    ///
    /// \param mins Vec2 containing minimum inclusive values `(x, y)`.
    /// \param maxs Vec2 containing maximum inclusive values `(x, y)`.
    ///
    /// \return A random za::Vec2f within the specified bounds.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten]] inline za::Vec2f getVec2f(const za::Vec2f mins, const za::Vec2f maxs)
    {
        return {getF(mins.x, maxs.x), getF(mins.y, maxs.y)};
    }

    ////////////////////////////////////////////////////////////
    /// \brief Generates a random 2D vector with components between 0 and specified maximums.
    ///
    /// \param maxs Vec2 containing maximum inclusive values `(x, y)`.
    ///
    /// \return A random za::Vec2f within the range `[0, maxs.x]` and `[0, maxs.y]`.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten]] inline za::Vec2f getVec2f(const za::Vec2f maxs)
    {
        return {getF(0.f, maxs.x), getF(0.f, maxs.y)};
    }

    ////////////////////////////////////////////////////////////
    /// \brief Generates a random point uniformly distributed within a circle.
    ///
    /// \param center Center of the circle.
    /// \param radius Radius of the circle.
    ///
    /// \return A random `za::Vec2f` inside the specified circle.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten]] inline za::Vec2f getPointInCircle(const za::Vec2f center, const float radius)
    {
        const float angle    = getF(0.f, za::tau);
        const float distance = radius * ZA_MATH_SQRTF(getF(0.f, 1.f));

        // Compute the point's coordinates using polar-to-Cartesian conversion.
        return {center.x + distance * ZA_MATH_COSF(angle), center.y + distance * ZA_MATH_SINF(angle)};
    }

    ////////////////////////////////////////////////////////////
    /// \brief Generates either `-1.f` or `1.f` with equal probability.
    ///
    /// \return Either `-1.f` or `1.f`.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten]] inline float getSignF()
    {
        return static_cast<float>((m_engine() >> 63u) << 1u) - 1.f;
    }

    ////////////////////////////////////////////////////////////
    /// \brief Generates a random 2D unit vector (direction).
    ///
    /// \return A random `za::Vec2f` with magnitude `1`.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten]] inline za::Vec2f getDirVec2f()
    {
        const float angle = getF(0.f, za::tau);
        return {ZA_MATH_COSF(angle), ZA_MATH_SINF(angle)};
    }
};
