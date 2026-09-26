#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Export.hpp"

#include "Zancle/Math/ClampMacro.hpp"
#include "Zancle/Math/Fabs.hpp"
#include "Zancle/Math/MinMaxMacros.hpp"
#include "Zancle/Math/Remainder.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/AssertAndAssume.hpp"
#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/Unreachable.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Utility class for manipulating RGBA colors
///
////////////////////////////////////////////////////////////
struct [[nodiscard]] ZA_GRAPHICS_API Color
{
    ////////////////////////////////////////////////////////////
    /// \brief Construct a fully white color with a given alpha
    ///
    /// \param alpha Alpha channel of the white mask
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::const]] static constexpr Color whiteWithAlpha(const za::U8 alpha)
    {
        return {255u, 255u, 255u, alpha};
    }


    ////////////////////////////////////////////////////////////
    /// \brief Construct a fully black color with a given alpha
    ///
    /// \param alpha Alpha channel of the black mask
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::const]] static constexpr Color blackWithAlpha(const za::U8 alpha)
    {
        return {0u, 0u, 0u, alpha};
    }


    ////////////////////////////////////////////////////////////
    /// \brief Construct the color from four floating-point components
    ///
    /// The RGBA floating point must be in the range `[0, 1]`, the behavior
    /// is undefined otherwise.
    ///
    /// \param r Red component of the color
    /// \param g Green component of the color
    /// \param b Blue component of the color
    /// \param a Alpha component of the color (default is 1, for fully opaque)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::const]] static constexpr Color fromFloats(
        const float r,
        const float g,
        const float b,
        const float a = 1.f)
    {
        ZA_ASSERT_AND_ASSUME(r >= 0.f && r <= 1.f);
        ZA_ASSERT_AND_ASSUME(g >= 0.f && g <= 1.f);
        ZA_ASSERT_AND_ASSUME(b >= 0.f && b <= 1.f);
        ZA_ASSERT_AND_ASSUME(a >= 0.f && a <= 1.f);

        return {static_cast<za::U8>(r * 255.f + 0.5f),  // NOLINT(bugprone-incorrect-roundings)
                static_cast<za::U8>(g * 255.f + 0.5f),  // NOLINT(bugprone-incorrect-roundings)
                static_cast<za::U8>(b * 255.f + 0.5f),  // NOLINT(bugprone-incorrect-roundings)
                static_cast<za::U8>(a * 255.f + 0.5f)}; // NOLINT(bugprone-incorrect-roundings)
    }


    ////////////////////////////////////////////////////////////
    /// \brief Construct the color from 32-bit unsigned integer
    ///
    /// The RGBA components are packed in the integer, with R in the
    /// most significant byte and A in the least significant byte.
    /// For example, `0xFF0000FF` represents an opaque red color.
    ///
    /// \param color Number containing the RGBA components (in that order)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::const]] static constexpr Color fromRGBA(const za::U32 color)
    {
        return {static_cast<za::U8>((color & 0xff'00'00'00) >> 24),
                static_cast<za::U8>((color & 0x00'ff'00'00) >> 16),
                static_cast<za::U8>((color & 0x00'00'ff'00) >> 8),
                static_cast<za::U8>((color & 0x00'00'00'ff) >> 0)};
    }


    ////////////////////////////////////////////////////////////
    /// \brief Color in HSL color model (hue, saturation, lightness)
    ///
    ////////////////////////////////////////////////////////////
    struct HSL
    {
        float hue;        //!< Hue in degrees (between `[0, 360)`)
        float saturation; //!< Saturation (between `[0, 1]`)
        float lightness;  //!< Lightness (between `[0, 1]`)
    };


    ////////////////////////////////////////////////////////////
    /// \brief Construct the color from HSLA components
    ///
    /// \param hsl HSL components of the color.
    ///            - `hsl.hue` is an angle in degrees; it will be normalized to the `[0, 360)` range.
    ///            - `hsl.saturation` is a ratio, typically `[0, 1]`; it will be clamped to this range.
    ///            - `hsl.lightness` is a ratio, typically `[0, 1]`; it will be clamped to this range.
    ///
    /// \param alpha Alpha component (0 to 255)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::const]] static constexpr Color fromHSLA(HSL hsl, const za::U8 alpha = 255u)
    {
        auto& [hue, saturation, lightness] = hsl;

        hue = za::positiveRemainder(hue, 360.f);

        // Float rounding at the wrap boundary: `positiveRemainder` can return exactly `360.f`
        // for tiny negative hues; fold it back so `hueSegment` below stays in `[0, 5]`.
        if (hue >= 360.f)
            hue = 0.f;

        saturation = ZA_CLAMP(saturation, 0.f, 1.f);
        lightness  = ZA_CLAMP(lightness, 0.f, 1.f);

        if (saturation == 0.f)
        {
            const auto gray = static_cast<za::U8>(lightness * 255.f + 0.5f); // NOLINT(bugprone-incorrect-roundings)
            return {gray, gray, gray, alpha};
        }

        // Uses standard HSL to RGB conversion intermediate values C, X, m
        const float c = (1.f - ZA_MATH_FABSF(2.f * lightness - 1.f)) * saturation;
        const float x = c * (1.f - ZA_MATH_FABSF(za::positiveRemainder(hue / 60.f, 2.f) - 1.f));
        const float m = lightness - c / 2.f;

        float rPrime = 0.f;
        float gPrime = 0.f;
        float bPrime = 0.f;

        // Uses hue segments (0-5) and a switch statement
        const int hueSegment = static_cast<int>(hue / 60.f); // Result is [0, 5] due to hue normalization
        ZA_ASSERT_AND_ASSUME(hueSegment >= 0 && hueSegment <= 5);

        // clang-format off
        switch (hueSegment)
        {
            case 0: rPrime = c;   gPrime = x;   bPrime = 0.f; break;
            case 1: rPrime = x;   gPrime = c;   bPrime = 0.f; break;
            case 2: rPrime = 0.f; gPrime = c;   bPrime = x;   break;
            case 3: rPrime = 0.f; gPrime = x;   bPrime = c;   break;
            case 4: rPrime = x;   gPrime = 0.f; bPrime = c;   break;
            case 5: rPrime = c;   gPrime = 0.f; bPrime = x;   break;

            default:
                ZA_UNREACHABLE();
        }
        // clang-format on

        // NOLINTBEGIN(bugprone-incorrect-roundings)
        return {static_cast<za::U8>((rPrime + m) * 255.f + 0.5f),
                static_cast<za::U8>((gPrime + m) * 255.f + 0.5f),
                static_cast<za::U8>((bPrime + m) * 255.f + 0.5f),
                alpha};
        // NOLINTEND(bugprone-incorrect-roundings)
    }


    ////////////////////////////////////////////////////////////
    /// \brief Converts the current RGB color to its HSL (Hue, Saturation, Lightness) representation.
    ///
    /// The alpha component of the `za::Color` is not included in the HSL representation
    /// returned by this function.
    /// - `hue` will be in the range `[0, 360)`. For achromatic colors (grays), hue is 0.
    /// - `saturation` will be in the range `[0, 1]`.
    /// - `lightness` will be in the range `[0, 1]`.
    ///
    /// \return HSL representation of the color.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr HSL toHSL() const
    {
        const float rNorm = static_cast<float>(r) / 255.f;
        const float gNorm = static_cast<float>(g) / 255.f;
        const float bNorm = static_cast<float>(b) / 255.f;

        const float maxVal = ZA_MAX(ZA_MAX(rNorm, gNorm), bNorm);
        const float minVal = ZA_MIN(ZA_MIN(rNorm, gNorm), bNorm);
        const float chroma = maxVal - minVal;

        if (chroma == 0.f)
            return {0.f, 0.f, maxVal};

        const float lightness = (maxVal + minVal) / 2.f;
        ZA_ASSERT(lightness != 0.f && lightness != 1.f);

        float hue = (maxVal == rNorm   ? (gNorm - bNorm) / chroma
                     : maxVal == gNorm ? (bNorm - rNorm) / chroma + 2.f
                                       : (rNorm - gNorm) / chroma + 4.f);

        hue *= 60.f;

        if (hue < 0.f)
            hue += 360.f;

        return {hue, chroma / (1.f - ZA_MATH_FABSF(2.f * lightness - 1.f)), lightness};
    }


    ////////////////////////////////////////////////////////////
    /// \brief Retrieve the color as a 32-bit unsigned integer
    ///
    /// The RGBA components are packed into the integer, with R in the
    /// most significant byte and A in the least significant byte.
    /// For example, an opaque red color `(255, 0, 0, 255)` would be
    /// represented as `0xFF0000FF`.
    ///
    /// \return Color represented as a 32-bit unsigned integer
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr za::U32 toInteger() const
    {
        return static_cast<za::U32>((r << 24) | (g << 16) | (b << 8) | a);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Create a new color from the current one with the red component changed
    ///
    /// \param red New red component for the color (0 to 255).
    ///
    /// \return A new `za::Color` instance with the modified red component,
    ///         while G, B, and A components remain the same as the original color.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr Color withRed(const za::U8 red) const
    {
        return {red, g, b, a};
    }


    ////////////////////////////////////////////////////////////
    /// \brief Create a new color from the current one with the green component changed
    ///
    /// \param green New green component for the color (0 to 255).
    ///
    /// \return A new `za::Color` instance with the modified green component,
    ///         while R, B, and A components remain the same as the original color.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr Color withGreen(const za::U8 green) const
    {
        return {r, green, b, a};
    }


    ////////////////////////////////////////////////////////////
    /// \brief Create a new color from the current one with the blue component changed
    ///
    /// \param blue New blue component for the color (0 to 255).
    ///
    /// \return A new `za::Color` instance with the modified blue component,
    ///         while R, G, and A components remain the same as the original color.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr Color withBlue(const za::U8 blue) const
    {
        return {r, g, blue, a};
    }


    ////////////////////////////////////////////////////////////
    /// \brief Create a new color from the current one with the alpha component changed
    ///
    /// \param alpha New alpha component for the color (0 to 255).
    ///
    /// \return A new `za::Color` instance with the modified alpha component,
    ///         while R, G, and B components remain the same as the original color.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr Color withAlpha(const za::U8 alpha) const
    {
        return {r, g, b, alpha};
    }


    ////////////////////////////////////////////////////////////
    /// \brief Creates a new color by rotating the hue of the current color by a given angle.
    ///
    /// This function converts the color to HSL, adds the `degrees` offset to the hue
    /// (normalized to `[0, 360)`), and then converts back to RGB.
    /// Saturation, lightness, and alpha components remain unchanged.
    ///
    /// \param degrees The angle in degrees to add to the hue. The hue progression is
    ///                red (0°) → yellow (60°) → green (120°) → cyan (180°) → blue (240°) → magenta (300°) → red.
    ///
    /// \return A new `za::Color` instance with the rotated hue.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr Color withRotatedHue(const float degrees) const
    {
        auto hsl = toHSL();
        hsl.hue  = za::positiveRemainder(hsl.hue + degrees, 360.f);
        return fromHSLA(hsl, a);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Creates a new color by setting the saturation of the current color to a new value.
    ///
    /// This function converts the color to HSL, sets the saturation to the provided value
    /// (clamped to `[0, 1]`), and then converts back to RGB.
    /// Hue, lightness, and alpha components remain unchanged.
    ///
    /// \param saturation The new saturation value, typically in the range `[0, 1]`.
    ///                   Values outside this range will be clamped.
    ///
    /// \return A new `za::Color` instance with the modified saturation.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr Color withSaturation(const float saturation) const
    {
        auto hsl       = toHSL();
        hsl.saturation = ZA_CLAMP(saturation, 0.f, 1.f);
        return fromHSLA(hsl, a);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Creates a new color by setting the lightness of the current color to a new value.
    ///
    /// This function converts the color to HSL, sets the lightness to the provided value
    /// (clamped to `[0, 1]`), and then converts back to RGB.
    /// Hue, saturation, and alpha components remain unchanged.
    ///
    /// \param lightness The new lightness value, typically in the range `[0, 1]`.
    ///                  Values outside this range will be clamped.
    ///
    /// \return A new `za::Color` instance with the modified lightness.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr Color withLightness(const float lightness) const
    {
        auto hsl      = toHSL();
        hsl.lightness = ZA_CLAMP(lightness, 0.f, 1.f);
        return fromHSLA(hsl, a);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Constructs a `za::Color` object from a 4D vector.
    ///
    /// The vector's components (x, y, z, w) are assumed to be floating-point values
    /// in the range `[0, 1]` and correspond to R, G, B, A respectively.
    /// Values outside this range will be clamped. Each component is then scaled
    /// by 255 and rounded to the nearest integer to fit into `za::U8`.
    ///
    /// \tparam TVec4 The type of the 4D vector (e.g., `za::Vector4f`).
    ///               It must have public `x, y, z, w` members.
    /// \param vec The 4D vector to convert from. Its components `vec.x`, `vec.y`,
    ///            `vec.z`, `vec.w` map to R, G, B, A.
    ///
    /// \return A `za::Color` instance corresponding to the vector.
    ///
    ////////////////////////////////////////////////////////////
    template <typename TVec4>
    [[nodiscard, gnu::always_inline, gnu::pure]] static constexpr Color fromVec4(const TVec4& vec)
    {
        const auto convert = []<typename T>(const T value)
        { return static_cast<za::U8>(ZA_CLAMP(value * T{255}, T{0}, T{255})); };

        return {convert(vec.x), convert(vec.y), convert(vec.z), convert(vec.w)};
    }


    ////////////////////////////////////////////////////////////
    /// \brief Converts the color to a 3D vector of a specified type `TVec3`.
    ///
    /// The components of the vector will be floating-point values
    /// in the range `[0, 1]`, corresponding to the R, G, B components
    /// of the color (divided by 255 to normalize them).
    ///
    /// \tparam TVec3 The type of the 3D vector. Must be constructible
    ///               from three values of `decltype(TVec3{}.x)`.
    ///
    /// \return A `TVec3` instance with normalized R, G, B components.
    ///
    ////////////////////////////////////////////////////////////
    template <typename TVec3>
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr TVec3 toVec3() const
    {
        using DimType = decltype(TVec3{}.x);

        return {
            static_cast<DimType>(r) / DimType{255},
            static_cast<DimType>(g) / DimType{255},
            static_cast<DimType>(b) / DimType{255},
        };
    }


    ////////////////////////////////////////////////////////////
    /// \brief Converts the color to a 4D vector of a specified type `TVec4`.
    ///
    /// The components of the vector will be floating-point values
    /// in the range `[0, 1]`, corresponding to the R, G, B, A components
    /// of the color (divided by 255 to normalize them).
    ///
    /// \tparam TVec4 The type of the 4D vector. Must be constructible
    ///               from four values of `decltype(TVec4{}.x)`.
    ///
    /// \return A `TVec4` instance with normalized R, G, B, A components.
    ///
    ////////////////////////////////////////////////////////////
    template <typename TVec4>
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr TVec4 toVec4() const
    {
        using DimType = decltype(TVec4{}.x);

        return {
            static_cast<DimType>(r) / DimType{255},
            static_cast<DimType>(g) / DimType{255},
            static_cast<DimType>(b) / DimType{255},
            static_cast<DimType>(a) / DimType{255},
        };
    }


    ////////////////////////////////////////////////////////////
    /// \brief Overload of the `operator==`
    ///
    /// This operator compares two colors and check if they are equal.
    ///
    /// \param rhs Right operand
    ///
    /// \return `true` if colors are equal, `false` if they are different
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::const]] constexpr bool operator==(const Color& rhs) const = default;


    ////////////////////////////////////////////////////////////
    // Static member data
    ////////////////////////////////////////////////////////////
    // NOLINTBEGIN(readability-identifier-naming)
    static const Color Black;       //!< Black predefined color
    static const Color White;       //!< White predefined color
    static const Color Red;         //!< Red predefined color
    static const Color Green;       //!< Green predefined color
    static const Color Blue;        //!< Blue predefined color
    static const Color Yellow;      //!< Yellow predefined color
    static const Color Magenta;     //!< Magenta predefined color
    static const Color Cyan;        //!< Cyan predefined color
    static const Color Transparent; //!< Transparent (black) predefined color

    static const Color Brown;    //!< Brown predefined color
    static const Color Orange;   //!< Orange predefined color
    static const Color Pink;     //!< Pink predefined color
    static const Color BabyPink; //!< Baby pink predefined color
    static const Color HotPink;  //!< Hot pink predefined color
    static const Color Salmon;   //!< Salmon predefined color
    static const Color Violet;   //!< Violet predefined color
    static const Color Purple;   //!< Purple predefined color
    static const Color Peach;    //!< Peach predefined color
    static const Color Lime;     //!< Lime predefined color
    static const Color Mint;     //!< Mint predefined color
    static const Color Gray;     //!< Gray predefined color

    static const Color LightBlack;    //!< Light black predefined color
    static const Color LightBlue;     //!< Light blue predefined color
    static const Color LightRed;      //!< Light red predefined color
    static const Color LightMagenta;  //!< Light magenta predefined color
    static const Color LightGreen;    //!< Light green predefined color
    static const Color LightCyan;     //!< Light cyan predefined color
    static const Color LightYellow;   //!< Light yellow predefined color
    static const Color LightBrown;    //!< Light brown predefined color
    static const Color LightOrange;   //!< Light orange predefined color
    static const Color LightPink;     //!< Light pink predefined color
    static const Color LightBabyPink; //!< Light baby pink predefined color
    static const Color LightHotPink;  //!< Light hot pink predefined color
    static const Color LightSalmon;   //!< Light salmon predefined color
    static const Color LightViolet;   //!< Light violet predefined color
    static const Color LightPurple;   //!< Light purple predefined color
    static const Color LightPeach;    //!< Light peach predefined color
    static const Color LightLime;     //!< Light lime predefined color
    static const Color LightMint;     //!< Light mint predefined color
    static const Color LightGray;     //!< Light gray predefined color

    static const Color DarkBlue;     //!< Dark blue predefined color
    static const Color DarkRed;      //!< Dark red predefined color
    static const Color DarkMagenta;  //!< Dark magenta predefined color
    static const Color DarkGreen;    //!< Dark green predefined color
    static const Color DarkCyan;     //!< Dark cyan predefined color
    static const Color DarkYellow;   //!< Dark yellow predefined color
    static const Color DarkWhite;    //!< Dark white predefined color
    static const Color DarkBrown;    //!< Dark brown predefined color
    static const Color DarkOrange;   //!< Dark orange predefined color
    static const Color DarkPink;     //!< Dark pink predefined color
    static const Color DarkBabyPink; //!< Dark baby pink predefined color
    static const Color DarkHotPink;  //!< Dark hot pink predefined color
    static const Color DarkSalmon;   //!< Dark salmon predefined color
    static const Color DarkViolet;   //!< Dark violet predefined color
    static const Color DarkPurple;   //!< Dark purple predefined color
    static const Color DarkPeach;    //!< Dark peach predefined color
    static const Color DarkLime;     //!< Dark lime predefined color
    static const Color DarkMint;     //!< Dark mint predefined color
    static const Color DarkGray;     //!< Dark gray predefined color

    static const Color VeryDarkBlue;     //!< Very dark blue predefined color
    static const Color VeryDarkRed;      //!< Very dark red predefined color
    static const Color VeryDarkMagenta;  //!< Very dark magenta predefined color
    static const Color VeryDarkGreen;    //!< Very dark green predefined color
    static const Color VeryDarkCyan;     //!< Very dark cyan predefined color
    static const Color VeryDarkYellow;   //!< Very dark yellow predefined color
    static const Color VeryDarkWhite;    //!< Very dark white predefined color
    static const Color VeryDarkBrown;    //!< Very dark brown predefined color
    static const Color VeryDarkOrange;   //!< Very dark orange predefined color
    static const Color VeryDarkPink;     //!< Very dark pink predefined color
    static const Color VeryDarkBabyPink; //!< Very dark babyPink predefined color
    static const Color VeryDarkHotPink;  //!< Very dark hotPink predefined color
    static const Color VeryDarkSalmon;   //!< Very dark salmon predefined color
    static const Color VeryDarkViolet;   //!< Very dark violet predefined color
    static const Color VeryDarkPurple;   //!< Very dark purple predefined color
    static const Color VeryDarkPeach;    //!< Very dark peach predefined color
    static const Color VeryDarkLime;     //!< Very dark lime predefined color
    static const Color VeryDarkMint;     //!< Very dark mint predefined color
    static const Color VeryDarkGray;     //!< Very dark gray predefined color
    // NOLINTEND(readability-identifier-naming)

    ////////////////////////////////////////////////////////////
    /// \relates Color
    /// \brief Overload of the binary `operator+`
    ///
    /// This operator returns the component-wise sum of two colors.
    /// Components that exceed 255 are clamped to 255.
    ///
    /// \param lhs  Left operand
    /// \param rhs Right operand
    ///
    /// \return Result of \a lhs + \a rhs
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::const]] friend constexpr Color operator+(const Color lhs, const Color rhs)
    {
        const auto clampedAdd = [] [[gnu::always_inline, gnu::flatten]] (const za::U8 lc, const za::U8 rc) -> za::U8
        {
            const int intResult = int{lc} + int{rc};
            return static_cast<za::U8>(intResult < 255 ? intResult : 255);
        };

        return {clampedAdd(lhs.r, rhs.r), clampedAdd(lhs.g, rhs.g), clampedAdd(lhs.b, rhs.b), clampedAdd(lhs.a, rhs.a)};
    }


    ////////////////////////////////////////////////////////////
    /// \relates Color
    /// \brief Overload of the binary `operator-`
    ///
    /// This operator returns the component-wise subtraction of two colors.
    /// Components below 0 are clamped to 0.
    ///
    /// \param lhs  Left operand
    /// \param rhs Right operand
    ///
    /// \return Result of \a lhs - \a rhs
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::const]] friend constexpr Color operator-(const Color lhs, const Color rhs)
    {
        const auto clampedSub = [] [[gnu::always_inline, gnu::flatten]] (const za::U8 lc, const za::U8 rc) -> za::U8
        {
            const int intResult = int{lc} - int{rc};
            return static_cast<za::U8>(intResult > 0 ? intResult : 0);
        };

        return {clampedSub(lhs.r, rhs.r), clampedSub(lhs.g, rhs.g), clampedSub(lhs.b, rhs.b), clampedSub(lhs.a, rhs.a)};
    }


    ////////////////////////////////////////////////////////////
    /// \relates Color
    /// \brief Overload of the binary `operator*`
    ///
    /// This operator returns the component-wise multiplication
    /// (also called "modulation") of two colors.
    /// Components are then divided by 255 so that the result is
    /// still in the range [0, 255].
    ///
    /// \param lhs  Left operand
    /// \param rhs Right operand
    ///
    /// \return Result of \a lhs * \a rhs
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::const]] friend constexpr Color operator*(const Color lhs, const Color rhs)
    {
        const auto scaledMul = [] [[gnu::always_inline, gnu::flatten]] (const za::U8 lc, const za::U8 rc) -> za::U8
        {
            const auto uint16Result = static_cast<za::U16>(za::U16{lc} * za::U16{rc});
            return static_cast<za::U8>(uint16Result / 255u);
        };

        return {scaledMul(lhs.r, rhs.r), scaledMul(lhs.g, rhs.g), scaledMul(lhs.b, rhs.b), scaledMul(lhs.a, rhs.a)};
    }


    ////////////////////////////////////////////////////////////
    /// \relates Color
    /// \brief Overload of the binary `operator+=`
    ///
    /// This operator computes the component-wise sum of two colors,
    /// and assigns the result to the lhs operand.
    /// Components that exceed 255 are clamped to 255.
    ///
    /// \param lhs  Left operand
    /// \param rhs Right operand
    ///
    /// \return Reference to \a lhs
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] friend constexpr Color& operator+=(Color& lhs, const Color rhs)
    {
        return lhs = lhs + rhs;
    }


    ////////////////////////////////////////////////////////////
    /// \relates Color
    /// \brief Overload of the binary `operator-=`
    ///
    /// This operator computes the component-wise subtraction of two colors,
    /// and assigns the result to the lhs operand.
    /// Components below 0 are clamped to 0.
    ///
    /// \param lhs  Left operand
    /// \param rhs Right operand
    ///
    /// \return Reference to \a lhs
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] friend constexpr Color& operator-=(Color& lhs, const Color rhs)
    {
        return lhs = lhs - rhs;
    }


    ////////////////////////////////////////////////////////////
    /// \relates Color
    /// \brief Overload of the binary `operator*=`
    ///
    /// This operator returns the component-wise multiplication
    /// (also called "modulation") of two colors, and assigns
    /// the result to the lhs operand.
    /// Components are then divided by 255 so that the result is
    /// still in the range [0, 255].
    ///
    /// \param lhs  Left operand
    /// \param rhs Right operand
    ///
    /// \return Reference to \a lhs
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] friend constexpr Color& operator*=(Color& lhs, const Color rhs)
    {
        return lhs = lhs * rhs;
    }


    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    za::U8 r{};     //!< Red component
    za::U8 g{};     //!< Green component
    za::U8 b{};     //!< Blue component
    za::U8 a{255u}; //!< Alpha (opacity) component
};


////////////////////////////////////////////////////////////
// Static member data
////////////////////////////////////////////////////////////

// Note: the 'inline' keyword here is technically not required, but VS2019 fails
// to compile with a bogus "multiple definition" error if not explicitly used.
inline constexpr Color Color::Black(0, 0, 0);
inline constexpr Color Color::White(255, 255, 255);
inline constexpr Color Color::Red(255, 0, 0);
inline constexpr Color Color::Green(0, 255, 0);
inline constexpr Color Color::Blue(0, 0, 255);
inline constexpr Color Color::Yellow(255, 255, 0);
inline constexpr Color Color::Magenta(255, 0, 255);
inline constexpr Color Color::Cyan(0, 255, 255);
inline constexpr Color Color::Transparent(0, 0, 0, 0);

inline constexpr Color Color::Brown{128u, 80u, 32u};
inline constexpr Color Color::Orange{255u, 128u, 0u};
inline constexpr Color Color::Pink{255u, 128u, 192u};
inline constexpr Color Color::BabyPink{255u, 192u, 224u};
inline constexpr Color Color::HotPink{255u, 0u, 192u};
inline constexpr Color Color::Salmon{255u, 128u, 128u};
inline constexpr Color Color::Violet{128u, 0u, 255u};
inline constexpr Color Color::Purple{64u, 0u, 128u};
inline constexpr Color Color::Peach{255u, 128u, 96u};
inline constexpr Color Color::Lime{128u, 255u, 0u};
inline constexpr Color Color::Mint{64u, 255u, 192u};
inline constexpr Color Color::Gray{128u, 128u, 128u};

inline constexpr Color Color::LightBlack{64u, 64u, 64u};
inline constexpr Color Color::LightBlue{128u, 128u, 255u};
inline constexpr Color Color::LightRed{255u, 128u, 128u};
inline constexpr Color Color::LightMagenta{255u, 128u, 255u};
inline constexpr Color Color::LightGreen{128u, 255u, 128u};
inline constexpr Color Color::LightCyan{128u, 255u, 255u};
inline constexpr Color Color::LightYellow{255u, 255u, 128u};
inline constexpr Color Color::LightBrown{192u, 128u, 64u};
inline constexpr Color Color::LightOrange{255u, 160u, 64u};
inline constexpr Color Color::LightPink{255u, 160u, 224u};
inline constexpr Color Color::LightBabyPink{255u, 208u, 232u};
inline constexpr Color Color::LightHotPink{255u, 96u, 224u};
inline constexpr Color Color::LightSalmon{255u, 160u, 160u};
inline constexpr Color Color::LightViolet{160u, 96u, 255u};
inline constexpr Color Color::LightPurple{128u, 64u, 192u};
inline constexpr Color Color::LightPeach{255u, 160u, 128u};
inline constexpr Color Color::LightLime{192u, 255u, 128u};
inline constexpr Color Color::LightMint{128u, 255u, 224u};
inline constexpr Color Color::LightGray{192u, 192u, 192u};

inline constexpr Color Color::DarkBlue{0u, 0u, 128u};
inline constexpr Color Color::DarkRed{128u, 0u, 0u};
inline constexpr Color Color::DarkMagenta{128u, 0u, 128u};
inline constexpr Color Color::DarkGreen{0u, 128u, 0u};
inline constexpr Color Color::DarkCyan{0u, 128u, 128u};
inline constexpr Color Color::DarkYellow{128u, 128u, 0u};
inline constexpr Color Color::DarkWhite{128u, 128u, 128u};
inline constexpr Color Color::DarkBrown{64u, 32u, 0u};
inline constexpr Color Color::DarkOrange{128u, 64u, 0u};
inline constexpr Color Color::DarkPink{128u, 64u, 96u};
inline constexpr Color Color::DarkBabyPink{160u, 96u, 128u};
inline constexpr Color Color::DarkHotPink{128u, 0u, 96u};
inline constexpr Color Color::DarkSalmon{128u, 64u, 64u};
inline constexpr Color Color::DarkViolet{64u, 0u, 128u};
inline constexpr Color Color::DarkPurple{32u, 0u, 64u};
inline constexpr Color Color::DarkPeach{128u, 64u, 48u};
inline constexpr Color Color::DarkLime{64u, 128u, 0u};
inline constexpr Color Color::DarkMint{32u, 128u, 96u};
inline constexpr Color Color::DarkGray{64u, 64u, 64u};

inline constexpr Color Color::VeryDarkBlue{0u, 0u, 64u};
inline constexpr Color Color::VeryDarkRed{64u, 0u, 0u};
inline constexpr Color Color::VeryDarkMagenta{64u, 0u, 64u};
inline constexpr Color Color::VeryDarkGreen{0u, 64u, 0u};
inline constexpr Color Color::VeryDarkCyan{0u, 64u, 64u};
inline constexpr Color Color::VeryDarkYellow{64u, 64u, 0u};
inline constexpr Color Color::VeryDarkWhite{64u, 64u, 64u};
inline constexpr Color Color::VeryDarkBrown{32u, 16u, 0u};
inline constexpr Color Color::VeryDarkOrange{64u, 32u, 0u};
inline constexpr Color Color::VeryDarkPink{64u, 32u, 48u};
inline constexpr Color Color::VeryDarkBabyPink{80u, 48u, 64u};
inline constexpr Color Color::VeryDarkHotPink{64u, 0u, 48u};
inline constexpr Color Color::VeryDarkSalmon{64u, 32u, 32u};
inline constexpr Color Color::VeryDarkViolet{32u, 0u, 64u};
inline constexpr Color Color::VeryDarkPurple{16u, 0u, 32u};
inline constexpr Color Color::VeryDarkPeach{64u, 32u, 24u};
inline constexpr Color Color::VeryDarkLime{32u, 64u, 0u};
inline constexpr Color Color::VeryDarkMint{16u, 64u, 48u};
inline constexpr Color Color::VeryDarkGray{32u, 32u, 32u};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::Color
/// \ingroup graphics
///
/// `za::Color` is a simple aggregate composed of 4 components:
/// \li Red
/// \li Green
/// \li Blue
/// \li Alpha (opacity)
///
/// Each component is a public `za::U8` member in the range
/// `[0, 255]`. Colors can be constructed and manipulated very easily:
///
/// \code
/// za::Color color(255, 0, 0); // red
/// color.r = 0;                // make it black
/// color.b = 128;              // make it dark blue
/// \endcode
///
/// The fourth component, "alpha", represents opacity. A color with
/// an alpha value of 255 is fully opaque, while an alpha value of 0
/// is fully transparent, regardless of the other components.
///
/// An extensive palette of common colors is provided as static
/// constants, including grayscale, primary, secondary, and tertiary
/// colors with `Light`, `Dark`, and `VeryDark` variants:
/// \code
/// auto black       = za::Color::Black;
/// auto white       = za::Color::White;
/// auto red         = za::Color::Red;
/// auto green       = za::Color::Green;
/// auto blue        = za::Color::Blue;
/// auto yellow      = za::Color::Yellow;
/// auto magenta     = za::Color::Magenta;
/// auto cyan        = za::Color::Cyan;
/// auto transparent = za::Color::Transparent;
/// auto darkOrange  = za::Color::DarkOrange;
/// auto lightMint   = za::Color::LightMint;
/// \endcode
///
/// Colors can be added, subtracted, and modulated (multiplied)
/// component-wise using the overloaded operators `+`, `-`, and `*`,
/// with results clamped to `[0, 255]`.
///
/// `za::Color` also provides a number of convenience factory and
/// transformation helpers:
/// \li `fromRGBA` / `toInteger` -- convert from/to a packed 32-bit RGBA value.
/// \li `fromHSLA` / `toHSL` -- convert from/to the HSL (hue, saturation, lightness) model.
/// \li `withRed`, `withGreen`, `withBlue`, `withAlpha` -- produce a copy
///     with a single component replaced.
/// \li `withRotatedHue`, `withSaturation`, `withLightness` -- produce a
///     copy with HSL adjustments applied.
/// \li `whiteWithAlpha` / `blackWithAlpha` -- produce a fully white/black color
///     with the requested alpha.
/// \li `fromVec4` / `toVec3` / `toVec4` -- convert from/to floating-point
///     vector types whose components are in `[0, 1]`.
///
////////////////////////////////////////////////////////////
