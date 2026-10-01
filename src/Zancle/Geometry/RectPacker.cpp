// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Geometry/RectPacker.hpp"

#include "Zancle/Err/Err.hpp"

#include "Zancle/Algorithm/Sort.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"

#include "Zancle/Vocabulary/Optional.hpp"
#include "Zancle/Vocabulary/Span.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/Limits.hpp"
#include "Zancle/Base/Memcpy.hpp"
#include "Zancle/Base/SizeT.hpp"

// `NULL`, used by `stb_rect_pack.h` (freestanding, and much lighter than `<stdlib.h>`)
#include <stddef.h> // NOLINT(modernize-deprecated-headers)

// Use Zancle's sort and assertions instead of `qsort` and `assert`,
// so that `<stdlib.h>` and `<assert.h>` are not pulled in
#define STBRP_SORT(base, count, size, compare)                                               \
    ::za::quickSort((base), (base) + (count), [](const stbrp_rect& a, const stbrp_rect& b) { \
        return compare(&a, &b) < 0;                                                          \
    })

#define STBRP_ASSERT(...) ZA_ASSERT(__VA_ARGS__)

#define STBRP_STATIC
#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>


namespace za
{
////////////////////////////////////////////////////////////
struct RectPacker::Impl
{
    za::Vector<stbrp_node> nodes;
    za::Vector<stbrp_node> nodesBackup; //!< Lazily sized, used to roll back failed `packMultiple` calls
    stbrp_context          context{};

    explicit Impl(const Vec2u size) : nodes(size.x)
    {
        // `stbrp_init_target` requires at least one node and `int`-representable dimensions
        ZA_ASSERT(size.x > 0u && size.y > 0u && "Bin size must be non-zero");
        ZA_ASSERT(size.x <= unsigned{ZA_INT_MAX} && size.y <= unsigned{ZA_INT_MAX} && "Bin size must fit in an `int`");

        // TODO P1: add limits header to Zancle/Base and fix all hardcoded values/macros

        stbrp_init_target(&context,
                          static_cast<int>(size.x),
                          static_cast<int>(size.y),
                          nodes.data(),
                          static_cast<int>(nodes.size()));
    }
};


////////////////////////////////////////////////////////////
RectPacker::RectPacker(const Vec2u size) : m_impl(size)
{
}


////////////////////////////////////////////////////////////
RectPacker::~RectPacker() = default;


////////////////////////////////////////////////////////////
za::Optional<Vec2u> RectPacker::pack(const Vec2u rectSize)
{
    const auto fail = [&](const char* what)
    {
        priv::errMsg("Failure packing rectangle with size {{{}, {}}}: {}", rectSize.x, rectSize.y, what);
        return za::nullOpt;
    };

    if (rectSize.x == 0u || rectSize.y == 0u)
        return fail("zero-sized coordinate");

    // Also guarantees that the size fits in an `int` for `stb_rect_pack`
    if (rectSize.x > getSize().x || rectSize.y > getSize().y)
        return fail("no room to pack");

    stbrp_rect toPack{/* id */ 0,
                      /* input width */ static_cast<int>(rectSize.x),
                      /* input height */ static_cast<int>(rectSize.y),
                      /* output x */ {},
                      /* output y */ {},
                      /* was_packed */ {}};

    const int rc = stbrp_pack_rects(&m_impl->context, &toPack, /* num_rects */ 1);

    if (rc == /* failure */ 0)
        return fail("no room to pack");

    ZA_ASSERT(rc == /* success */ 1);
    ZA_ASSERT(toPack.was_packed != 0);

    return za::makeOptional<Vec2u>(static_cast<unsigned int>(toPack.x), static_cast<unsigned int>(toPack.y));
}


////////////////////////////////////////////////////////////
bool RectPacker::packMultiple(const za::Span<Vec2u> outPositions, const za::Span<const Vec2u> rectSizes)
{
    const auto fail = [&](const char* what)
    {
        priv::errMsg("Failure packing multiple rectangles: {}", what);
        return false;
    };

    if (outPositions.size() != rectSizes.size())
        return fail("mismatched output and input sizes");

    // Avoid a heap allocation for the common case of a small batch
    constexpr za::SizeT    stackCapacity = 512u;
    stbrp_rect             stackBuffer[stackCapacity];
    za::Vector<stbrp_rect> heapBuffer;
    stbrp_rect*            toPack = stackBuffer;

    if (rectSizes.size() > stackCapacity)
    {
        heapBuffer.resize(rectSizes.size());
        toPack = heapBuffer.data();
    }

    const Vec2u binSize = getSize();

    for (za::SizeT i = 0u; i < rectSizes.size(); ++i)
    {
        const auto& size = rectSizes[i];

        if (size.x == 0u || size.y == 0u)
            return fail("zero-sized input rect size");

        // Also guarantees that the size fits in an `int` for `stb_rect_pack`
        if (size.x > binSize.x || size.y > binSize.y)
            return fail("no room to pack");

        toPack[i] = {/* id */ static_cast<int>(i),
                     /* input width */ static_cast<int>(size.x),
                     /* input height */ static_cast<int>(size.y),
                     /* output x */ {},
                     /* output y */ {},
                     /* was_packed */ {}};
    }

    // `stbrp_pack_rects` packs every rectangle that fits even if some don't, permanently
    // consuming their space. Snapshot the packer state so that a failure can be rolled back.
    // All node links point into `nodes` or into `context.extra`, whose addresses never change,
    // so restoring their bytes in place restores a consistent state.
    Impl& impl = *m_impl;

    if (impl.nodesBackup.size() != impl.nodes.size())
        impl.nodesBackup.resize(impl.nodes.size());

    const za::SizeT nodesBytes = impl.nodes.size() * sizeof(stbrp_node);
    ZA_MEMCPY(impl.nodesBackup.data(), impl.nodes.data(), nodesBytes);
    const stbrp_context contextBackup = impl.context;

    const int rc = stbrp_pack_rects(&impl.context, toPack, /* num_rects */ static_cast<int>(rectSizes.size()));

    if (rc == /* failure */ 0)
    {
        ZA_MEMCPY(impl.nodes.data(), impl.nodesBackup.data(), nodesBytes);
        impl.context = contextBackup;

        return fail("no room to pack");
    }

    ZA_ASSERT(rc == /* success */ 1);

    for (za::SizeT i = 0u; i < rectSizes.size(); ++i)
    {
        const auto& packed = toPack[i];
        ZA_ASSERT(packed.was_packed != 0);

        outPositions[i] = {static_cast<unsigned int>(packed.x), static_cast<unsigned int>(packed.y)};
    }

    return true;
}


////////////////////////////////////////////////////////////
Vec2u RectPacker::getSize() const
{
    return {static_cast<unsigned int>(m_impl->context.width), static_cast<unsigned int>(m_impl->context.height)};
}

} // namespace za
