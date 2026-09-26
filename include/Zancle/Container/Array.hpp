#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/SizeT.hpp"

#include "Zancle/Trait/EnableTrivialRelocation.hpp"
#include "Zancle/Trait/IsTriviallyRelocatable.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Aggregate fixed-size array, lightweight `std::array` replacement
///
/// An aggregate type wrapping a C array of size `N`. Supports brace
/// initialization, deduction guides, and propagates trivial
/// relocatability from the element type. Zero-sized arrays are not
/// allowed.
///
/// All accessors assert in debug builds when given an out-of-bounds index.
///
////////////////////////////////////////////////////////////
template <typename T, SizeT N>
struct [[nodiscard]] Array
{
    ////////////////////////////////////////////////////////////
    ZA_ENABLE_TRIVIAL_RELOCATION_IF(ZA_IS_TRIVIALLY_RELOCATABLE(T));


    ////////////////////////////////////////////////////////////
    static_assert(N > 0, "Zero-sized arrays are not supported");


    ////////////////////////////////////////////////////////////
    /// \brief Underlying storage; exposed publicly to remain an aggregate
    ///
    ////////////////////////////////////////////////////////////
    T elements[N];


    ///////////////////////////////////////////////////////////
    /// \brief Number of elements in the array (always `N`)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::const]] constexpr SizeT size() const noexcept
    {
        return N;
    }


    ///////////////////////////////////////////////////////////
    /// \brief Pointer to the underlying contiguous storage
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr T* data() noexcept
    {
        return elements;
    }


    ///////////////////////////////////////////////////////////
    /// \brief Pointer to the underlying contiguous storage (const overload)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr const T* data() const noexcept
    {
        return elements;
    }


    ///////////////////////////////////////////////////////////
    /// \brief Indexed element access (asserts `i < N`)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr T& operator[](const SizeT i) noexcept
    {
        ZA_ASSERT(i < N);
        return elements[i];
    }


    ///////////////////////////////////////////////////////////
    /// \brief Indexed element access (const overload)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] constexpr const T& operator[](const SizeT i) const noexcept
    {
        ZA_ASSERT(i < N);
        return elements[i];
    }


    ///////////////////////////////////////////////////////////
    /// \brief Iterator to the first element
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr T* begin() noexcept
    {
        return elements;
    }


    ///////////////////////////////////////////////////////////
    /// \brief Iterator to the first element (const overload)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr const T* begin() const noexcept
    {
        return elements;
    }


    ///////////////////////////////////////////////////////////
    /// \brief Iterator one past the last element
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr T* end() noexcept
    {
        return elements + N;
    }


    ///////////////////////////////////////////////////////////
    /// \brief Iterator one past the last element (const overload)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr const T* end() const noexcept
    {
        return elements + N;
    }


    ///////////////////////////////////////////////////////////
    /// \brief Constant iterator to the first element
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr const T* cbegin() const noexcept
    {
        return elements;
    }


    ///////////////////////////////////////////////////////////
    /// \brief Constant iterator one past the last element
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] constexpr const T* cend() const noexcept
    {
        return elements + N;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Element-wise equality comparison
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] constexpr bool operator==(const Array& rhs) const = default;
};


////////////////////////////////////////////////////////////
/// \brief Deduction guide enabling `Array{a, b, c}` syntax
///
////////////////////////////////////////////////////////////
template <typename T, typename... Elements>
Array(T, Elements...) -> Array<T, 1 + sizeof...(Elements)>;

} // namespace za
