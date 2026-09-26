#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Export.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Blending modes for drawing
///
////////////////////////////////////////////////////////////
struct [[nodiscard]] ZA_GRAPHICS_API BlendMode
{
    ////////////////////////////////////////////////////////
    /// \brief Enumeration of the blending factors
    ///
    /// The factors are mapped directly to their OpenGL equivalents,
    /// specified by `glBlendFunc` or `glBlendFuncSeparate`.
    ///
    ////////////////////////////////////////////////////////
    enum class [[nodiscard]] Factor : unsigned char
    {
        Zero             = 0u, //!< (0, 0, 0, 0)
        One              = 1u, //!< (1, 1, 1, 1)
        SrcColor         = 2u, //!< (src.r, src.g, src.b, src.a)
        OneMinusSrcColor = 3u, //!< (1, 1, 1, 1) - (src.r, src.g, src.b, src.a)
        DstColor         = 4u, //!< (dst.r, dst.g, dst.b, dst.a)
        OneMinusDstColor = 5u, //!< (1, 1, 1, 1) - (dst.r, dst.g, dst.b, dst.a)
        SrcAlpha         = 6u, //!< (src.a, src.a, src.a, src.a)
        OneMinusSrcAlpha = 7u, //!< (1, 1, 1, 1) - (src.a, src.a, src.a, src.a)
        DstAlpha         = 8u, //!< (dst.a, dst.a, dst.a, dst.a)
        OneMinusDstAlpha = 9u  //!< (1, 1, 1, 1) - (dst.a, dst.a, dst.a, dst.a)
    };

    ////////////////////////////////////////////////////////
    /// \brief Enumeration of the blending equations
    ///
    /// The equations are mapped directly to their OpenGL equivalents,
    /// specified by `glBlendEquation` or `glBlendEquationSeparate`.
    ///
    ////////////////////////////////////////////////////////
    enum class [[nodiscard]] Equation : unsigned char
    {
        Add             = 0u, //!< Pixel = Src * SrcFactor + Dst * DstFactor
        Subtract        = 1u, //!< Pixel = Src * SrcFactor - Dst * DstFactor
        ReverseSubtract = 2u, //!< Pixel = Dst * DstFactor - Src * SrcFactor
        Min             = 3u, //!< Pixel = min(Dst, Src)
        Max             = 4u  //!< Pixel = max(Dst, Src)
    };

    ////////////////////////////////////////////////////////////
    /// \brief Construct the blend mode given the factors and equation.
    ///
    /// This constructor uses the same factors and equation for both
    /// color and alpha components. It also defaults to the Add equation.
    ///
    /// \param sourceFactor      Specifies how to compute the source factor for the color and alpha channels.
    /// \param destinationFactor Specifies how to compute the destination factor for the color and alpha channels.
    /// \param blendEquation     Specifies how to combine the source and destination colors and alpha.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten]] static constexpr BlendMode from(
        const Factor   sourceFactor,
        const Factor   destinationFactor,
        const Equation blendEquation = Equation::Add)
    {
        return BlendMode{
            .colorSrcFactor = sourceFactor,
            .colorDstFactor = destinationFactor,
            .colorEquation  = blendEquation,
            .alphaSrcFactor = sourceFactor,
            .alphaDstFactor = destinationFactor,
            .alphaEquation  = blendEquation,
        };
    }

    ////////////////////////////////////////////////////////////
    /// \brief Overload of the `operator==`
    ///
    /// \param rhs Right operand
    ///
    /// \return `true` if blending modes are equal, `false` if they are different
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] constexpr bool operator==(const BlendMode& rhs) const = default;

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    Factor   colorSrcFactor : 4 {Factor::SrcAlpha};         //!< Source blending factor for the color channels
    Factor   colorDstFactor : 4 {Factor::OneMinusSrcAlpha}; //!< Destination blending factor for the color channels
    Equation colorEquation  : 3 {Equation::Add};            //!< Blending equation for the color channels
    Factor   alphaSrcFactor : 4 {Factor::One};              //!< Source blending factor for the alpha channel
    Factor   alphaDstFactor : 4 {Factor::OneMinusSrcAlpha}; //!< Destination blending factor for the alpha channel
    Equation alphaEquation  : 3 {Equation::Add};            //!< Blending equation for the alpha channel
};

////////////////////////////////////////////////////////////
// Commonly used blending modes
////////////////////////////////////////////////////////////
// NOLINTBEGIN(readability-identifier-naming)

/// Blend source and dest according to dest alpha
inline constexpr BlendMode BlendAlpha{
    .colorSrcFactor = BlendMode::Factor::SrcAlpha,
    .colorDstFactor = BlendMode::Factor::OneMinusSrcAlpha,
    .colorEquation  = BlendMode::Equation::Add,
    .alphaSrcFactor = BlendMode::Factor::One,
    .alphaDstFactor = BlendMode::Factor::OneMinusSrcAlpha,
    .alphaEquation  = BlendMode::Equation::Add,
};

/// Add source to dest
inline constexpr BlendMode BlendAdd{
    .colorSrcFactor = BlendMode::Factor::SrcAlpha,
    .colorDstFactor = BlendMode::Factor::One,
    .colorEquation  = BlendMode::Equation::Add,
    .alphaSrcFactor = BlendMode::Factor::One,
    .alphaDstFactor = BlendMode::Factor::One,
    .alphaEquation  = BlendMode::Equation::Add,
};

/// Multiply source and dest
inline constexpr auto BlendMultiply = BlendMode::from(BlendMode::Factor::DstColor,
                                                      BlendMode::Factor::Zero,
                                                      BlendMode::Equation::Add);

/// Take minimum between source and dest
inline constexpr auto BlendMin = BlendMode::from(BlendMode::Factor::One, BlendMode::Factor::One, BlendMode::Equation::Min);

/// Take maximum between source and dest
inline constexpr auto BlendMax = BlendMode::from(BlendMode::Factor::One, BlendMode::Factor::One, BlendMode::Equation::Max);

/// Overwrite dest with source
inline constexpr auto BlendNone = BlendMode::from(BlendMode::Factor::One, BlendMode::Factor::Zero, BlendMode::Equation::Add);

// NOLINTEND(readability-identifier-naming)

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::BlendMode
/// \ingroup graphics
///
/// `za::BlendMode` is a class that represents a blend mode. A blend
/// mode determines how the colors of an object you draw are
/// mixed with the colors that are already in the buffer.
///
/// The class is composed of 6 components, each of which has its
/// own public member variable:
/// \li %Color Source Factor (@ref colorSrcFactor)
/// \li %Color Destination Factor (@ref colorDstFactor)
/// \li %Color Blend Equation (@ref colorEquation)
/// \li Alpha Source Factor (@ref alphaSrcFactor)
/// \li Alpha Destination Factor (@ref alphaDstFactor)
/// \li Alpha Blend Equation (@ref alphaEquation)
///
/// The source factor specifies how the pixel you are drawing contributes
/// to the final color. The destination factor specifies how the pixel
/// already drawn in the buffer contributes to the final color.
///
/// The color channels RGB (red, green, blue; simply referred to as
/// color) and A (alpha; the transparency) can be treated separately. This
/// separation can be useful for specific blend modes, but most often you
/// won't need it and will simply treat the color as a single unit.
///
/// The blend factors and equations correspond to their OpenGL equivalents.
/// In general, the color of the resulting pixel is calculated according
/// to the following formula (`src` is the color of the source pixel, `dst`
/// the color of the destination pixel, the other variables correspond to the
/// public members, with the equations being + or - operators):
/// \code
/// dst.rgb = colorSrcFactor * src.rgb (colorEquation) colorDstFactor * dst.rgb
/// dst.a   = alphaSrcFactor * src.a   (alphaEquation) alphaDstFactor * dst.a
/// \endcode
/// All factors and colors are represented as floating point numbers between
/// 0 and 1. Where necessary, the result is clamped to fit in that range.
///
/// The most common blending modes are predefined as inline
/// `constexpr` constants in the `sf` namespace:
///
/// \code
/// za::BlendMode alphaBlending          = za::BlendAlpha;
/// za::BlendMode additiveBlending       = za::BlendAdd;
/// za::BlendMode multiplicativeBlending = za::BlendMultiply;
/// za::BlendMode minBlending            = za::BlendMin;
/// za::BlendMode maxBlending            = za::BlendMax;
/// za::BlendMode noBlending             = za::BlendNone;
/// \endcode
///
/// You can also build a custom blend mode using designated
/// initializers, or use the `za::BlendMode::from` helper when the
/// same factors and equation are used for both color and alpha:
///
/// \code
/// constexpr auto premultiplied = za::BlendMode{
///     .colorSrcFactor = za::BlendMode::Factor::One,
///     .colorDstFactor = za::BlendMode::Factor::OneMinusSrcAlpha,
/// };
/// \endcode
///
/// A blend mode can be specified every time you draw something to a
/// render target. It is part of the `za::RenderStates` compound that
/// is passed to `za::RenderTarget::draw`.
///
/// \see `za::RenderStates`, `za::RenderTarget`
///
////////////////////////////////////////////////////////////
