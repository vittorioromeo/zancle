// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Export.hpp"

#include "Zancle/Graphics/Color.hpp"
#include "Zancle/Graphics/Glsl.hpp" // NOLINT(misc-header-include-cycle)

#include "Zancle/Base/Memcpy.hpp"
#include "Zancle/Base/SizeT.hpp"

#include "Zancle/Trait/IsSame.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
struct Transform;
} // namespace za


namespace za::priv
{
////////////////////////////////////////////////////////////
/// \brief Helper functions to copy `za::Transform` to `za::Glsl::Mat3/4`
///
////////////////////////////////////////////////////////////
ZA_GRAPHICS_API void copyMatrix(const Transform& source, Matrix<3, 3>& dest);
ZA_GRAPHICS_API void copyMatrix(const Transform& source, Matrix<4, 4>& dest);

////////////////////////////////////////////////////////////
/// \brief Copy array-based matrix with given number of elements
///
////////////////////////////////////////////////////////////
[[gnu::always_inline]] ZA_GRAPHICS_API inline void copyMatrix(const float* source, za::SizeT elements, float* dest)
{
    ZA_MEMCPY(dest, source, elements * sizeof(float));
}


////////////////////////////////////////////////////////////
/// \brief Matrix type, used to set uniforms in GLSL
///
////////////////////////////////////////////////////////////
template <za::SizeT Columns, za::SizeT Rows>
struct [[nodiscard]] Matrix
{
    ////////////////////////////////////////////////////////////
    /// \brief Construct from raw data
    ///
    /// \param pointer Points to the beginning of an array that
    ///                has the size of the matrix. The elements
    ///                are copied to the instance.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] explicit Matrix(const float* pointer)
    {
        copyMatrix(pointer, Columns * Rows, array);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Construct implicitly from Zancle transform
    ///
    /// This constructor is only supported for 3x3 and 4x4
    /// matrices.
    ///
    /// \param transform Object containing a transform.
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] Matrix(const Transform& transform)
    {
        copyMatrix(transform, *this);
    }

    float array[Columns * Rows]{}; //!< Array holding matrix data
};

////////////////////////////////////////////////////////////
/// \brief 4D vector type, used to set uniforms in GLSL
///
////////////////////////////////////////////////////////////
template <typename T>
struct [[nodiscard]] Vec4
{
    ////////////////////////////////////////////////////////////
    /// \brief Default constructor, creates a zero vec4
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] constexpr Vec4() = default;

    ////////////////////////////////////////////////////////////
    /// \brief Construct from four components
    ///
    /// \param x Component of the vec4
    /// \param y Component of the vec4
    /// \param z Component of the vec4
    /// \param w Component of the vec4
    ///
    ////////////////////////////////////////////////////////////
#if defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wshadow"
#endif
    [[nodiscard, gnu::always_inline]] constexpr Vec4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w)
    {
    }
#if defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif

    ////////////////////////////////////////////////////////////
    /// \brief Converts the vec4 to another type of vec4
    ///
    ////////////////////////////////////////////////////////////
    template <typename U>
    [[nodiscard, gnu::always_inline]] constexpr explicit Vec4(const Vec4<U>& other) :
        x(static_cast<T>(other.x)),
        y(static_cast<T>(other.y)),
        z(static_cast<T>(other.z)),
        w(static_cast<T>(other.w))
    {
    }

    ////////////////////////////////////////////////////////////
    /// \brief Construct vec4 implicitly from color
    ///
    /// The vec4 is normalized to [0, 1] for floats, and left as-is
    /// for ints. Only available for float and int vec4s
    /// (`za::Glsl::Vec4` / `za::Glsl::Ivec4`).
    ///
    /// \param color Color instance
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] constexpr Vec4(Color color)
        requires(za::isSame<T, float> || za::isSame<T, int>);

    T x{}; //!< 1st component (X) of the vec4
    T y{}; //!< 2nd component (Y) of the vec4
    T z{}; //!< 3rd component (Z) of the vec4
    T w{}; //!< 4th component (W) of the vec4
};

////////////////////////////////////////////////////////////
template <>
constexpr Vec4<float>::Vec4(Color color) :
    x(color.r / 255.f),
    y(color.g / 255.f),
    z(color.b / 255.f),
    w(color.a / 255.f)
{
}


////////////////////////////////////////////////////////////
template <>
constexpr Vec4<int>::Vec4(Color color) : x(color.r), y(color.g), z(color.b), w(color.a)
{
}

} // namespace za::priv
