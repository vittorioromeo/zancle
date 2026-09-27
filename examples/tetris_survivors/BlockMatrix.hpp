#pragma once


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Block.hpp"
#include "IndexUtils.hpp"
#include "ShapeDimension.hpp"

#include "Zancle/Container/Array.hpp"
#include "Zancle/Container/InPlaceVector.hpp"

#include "Zancle/Vocabulary/Optional.hpp"


namespace tsurv
{
////////////////////////////////////////////////////////////
struct [[nodiscard]] BlockMatrix
{
    ////////////////////////////////////////////////////////////
    za::Array<za::Optional<Block>, shapeDimension * shapeDimension> data;


    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Optional<Block>& at(const za::SizeT x, const za::SizeT y)
    {
        ZA_ASSERT(x < shapeDimension);
        ZA_ASSERT(y < shapeDimension);

        const auto index = getIndex2Dto1D(za::Vec2uz{x, y}, shapeDimension);
        ZA_ASSERT(index < data.size());

        return data[index];
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] const za::Optional<Block>& at(const za::SizeT x, const za::SizeT y) const
    {
        return const_cast<BlockMatrix*>(this)->at(x, y);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Optional<Block>& at(const int x, const int y)
    {
        ZA_ASSERT(x >= 0);
        ZA_ASSERT(y >= 0);

        return at(static_cast<za::SizeT>(x), static_cast<za::SizeT>(y));
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] const za::Optional<Block>& at(const int x, const int y) const
    {
        return const_cast<BlockMatrix*>(this)->at(x, y);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isInBounds(const za::SizeT x, const za::SizeT y) const
    {
        return x < shapeDimension && y < shapeDimension;
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool isInBounds(const int x, const int y) const
    {
        return x >= 0 && x < static_cast<int>(shapeDimension) && y >= 0 && y < static_cast<int>(shapeDimension);
    }
};


////////////////////////////////////////////////////////////
[[nodiscard]] inline BlockMatrix shapeMatrixToBlockMatrix(const ShapeMatrix& shapeMatrix, const Block& block)
{
    BlockMatrix blockMatrix;

    for (za::SizeT i = 0u; i < shapeMatrix.size(); ++i)
        if (const auto shapeBlockSequence = shapeMatrix[i]; shapeBlockSequence != ShapeBlockSequence::_)
            blockMatrix.data[i].emplace(block).shapeBlockSequence = shapeBlockSequence;

    return blockMatrix;
}


////////////////////////////////////////////////////////////
using ShapeBlockPositionVector = za::InPlaceVector<za::Vec2uz, shapeDimension>;


////////////////////////////////////////////////////////////
inline ShapeBlockPositionVector findDownmostBlocks(const BlockMatrix& shape)
{
    ShapeBlockPositionVector result;

    bool foundLastRow = false;

    for (za::SizeT y = shapeDimension; y-- > 0;)
    {
        for (za::SizeT x = 0; x < shapeDimension; ++x)
        {
            if (shape.at(x, y).hasValue())
            {
                foundLastRow = true;
                result.unsafeEmplaceBack(x, y); // only one row is scanned: at most `shapeDimension` blocks
            }
        }

        if (foundLastRow)
            break;
    }

    return result;
}


////////////////////////////////////////////////////////////
[[nodiscard]] inline ShapeBlockPositionVector findTopmostBlocks(const BlockMatrix& shape)
{
    ShapeBlockPositionVector result;

    bool foundFirstRow = false;

    for (za::SizeT y = 0; y < shapeDimension; ++y)
    {
        for (za::SizeT x = 0; x < shapeDimension; ++x)
        {
            if (shape.at(x, y).hasValue())
            {
                foundFirstRow = true;
                result.unsafeEmplaceBack(x, y); // only one row is scanned: at most `shapeDimension` blocks
            }
        }

        if (foundFirstRow)
            break;
    }

    return result;
}


////////////////////////////////////////////////////////////
[[nodiscard]] inline ShapeBlockPositionVector findHorizontalBlocks(const BlockMatrix& shape, const za::SizeT maxDepth)
{
    ShapeBlockPositionVector result;

    za::SizeT foundY = 0u;

    for (za::SizeT y = shapeDimension; y-- > 0;)
    {
        za::SizeT xCount = 0u;

        for (za::SizeT x = 0; x < shapeDimension; ++x)
        {
            const bool leftEmpty  = (x == 0) || !shape.at(x - 1, y).hasValue();
            const bool rightEmpty = (x == shapeDimension - 1) || !shape.at(x + 1, y).hasValue();

            if (shape.at(x, y).hasValue() && leftEmpty && rightEmpty)
            {
                result.emplaceBack(x, y);
                ++foundY;
            }

            if (shape.at(x, y).hasValue())
                ++xCount;
        }

        if (foundY >= maxDepth || xCount > 1u)
            break;
    }

    return result;
}


////////////////////////////////////////////////////////////
[[nodiscard, gnu::pure]] inline bool blockMatricesIntersect(
    const BlockMatrix& shape1,
    const za::Vec2i    pos1,
    const BlockMatrix& shape2,
    const za::Vec2i    pos2)
{
    for (za::SizeT y1 = 0; y1 < shapeDimension; ++y1)
        for (za::SizeT x1 = 0; x1 < shapeDimension; ++x1)
        {
            if (!shape1.at(x1, y1).hasValue())
                continue;

            const za::Vec2i worldPos1 = pos1 + za::Vec2uz{x1, y1}.toVec2i();

            for (za::SizeT y2 = 0; y2 < shapeDimension; ++y2)
                for (za::SizeT x2 = 0; x2 < shapeDimension; ++x2)
                {
                    if (!shape2.at(x2, y2).hasValue())
                        continue;

                    const za::Vec2i worldPos2 = pos2 + za::Vec2uz{x2, y2}.toVec2i();

                    if (worldPos1 == worldPos2)
                        return true;
                }
        }

    return false;
}

} // namespace tsurv
