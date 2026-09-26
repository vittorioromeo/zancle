#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Vertex.hpp"
#include "Zancle/Graphics/VertexUtils.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Vocabulary/Span.hpp"

#include "Zancle/Base/LifetimeAttributes.hpp"


namespace za::priv
{
////////////////////////////////////////////////////////////
/// \brief Implementation of a vertex span, can be mutable or immutable
///
////////////////////////////////////////////////////////////
template <typename T>
struct ZA_GSL_POINTER(T) VertexSpanImpl : za::Span<T>
{
    using za::Span<T>::Span;

    ////////////////////////////////////////////////////////////
    /// \brief Converts a mutable vertex span to an immutable vertex span
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten]] inline constexpr operator VertexSpanImpl<const T>() const noexcept
    {
        return VertexSpanImpl<const T>{this->theData, this->theSize};
    }

    ////////////////////////////////////////////////////////////
    /// \brief Computes the bounding rectangle of this vertex span.
    ///
    /// This function iterates through a sequence of vertices and determines the
    /// smallest axis-aligned rectangle that encloses all the vertices.
    ///
    /// \return A rect representing the bounding rectangle.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] inline constexpr Rect2f getGlobalBounds() const noexcept
    {
        return VertexUtils::getVertexRangeBounds(this->theData, this->theSize);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Computes the minimum x-coordinate of this vertex span.
    ///
    /// \return The minimum x-coordinate.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr float getLeft() const noexcept
    {
        return VertexUtils::getLeft(this->theData, this->theSize);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Computes the maximum x-coordinate of this vertex span.
    ///
    /// \return The maximum x-coordinate.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr float getRight() const noexcept
    {
        return VertexUtils::getRight(this->theData, this->theSize);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Computes the minimum y-coordinate of this vertex span.
    ///
    /// \return The minimum y-coordinate.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr float getTop() const noexcept
    {
        return VertexUtils::getTop(this->theData, this->theSize);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Computes the maximum y-coordinate of this vertex span.
    ///
    /// \return The maximum y-coordinate.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr float getBottom() const noexcept
    {
        return VertexUtils::getBottom(this->theData, this->theSize);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Computes the (min x, min y) of this vertex span.
    ///
    /// \return The (min x, min y) of this vertex span.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr Vec2f getTopLeft() const noexcept
    {
        return VertexUtils::getTopLeft(this->theData, this->theSize);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Computes the (max x, min y) of this vertex span.
    ///
    /// \return The (max x, min y) of this vertex span.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr Vec2f getTopRight() const noexcept
    {
        return VertexUtils::getTopRight(this->theData, this->theSize);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Computes the (min y, max y) of this vertex span.
    ///
    /// \return The (min y, max y) of this vertex span.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr Vec2f getBottomLeft() const noexcept
    {
        return VertexUtils::getBottomLeft(this->theData, this->theSize);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Computes the (max x, max y) of this vertex span.
    ///
    /// \return The (max x, max y) of this vertex span.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten, gnu::pure]] inline constexpr Vec2f getBottomRight() const noexcept
    {
        return VertexUtils::getBottomRight(this->theData, this->theSize);
    }
};

} // namespace za::priv


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Mutable span of vertices
///
////////////////////////////////////////////////////////////
using VertexSpan = priv::VertexSpanImpl<Vertex>;

////////////////////////////////////////////////////////////
/// \brief Immutable span of vertices
///
////////////////////////////////////////////////////////////
using ConstVertexSpan = priv::VertexSpanImpl<const Vertex>;

} // namespace za

////////////////////////////////////////////////////////////
/// \struct za::VertexSpan
/// \ingroup graphics
///
/// `za::VertexSpan` is a thin refinement of `za::Span<Vertex>`
/// that adds vertex-specific helpers such as bounding box and
/// extrema queries (`getGlobalBounds`, `getTopLeft`,
/// `getBottomRight`, ...).
///
/// `za::ConstVertexSpan` is the immutable equivalent
/// (`za::Span<const Vertex>`) and exposes the same query API.
/// A mutable `VertexSpan` implicitly converts to a
/// `ConstVertexSpan`.
///
/// `VertexSpan` is the return type of the stateless draw paths
/// in `za::RenderTarget` and `za::DrawableBatch`. It is meant
/// for short-term use only -- the underlying storage may be
/// invalidated by the next batch flush or vertex append.
///
/// \see `za::Vertex`, `za::Span`, `za::VertexUtils`
///
////////////////////////////////////////////////////////////
