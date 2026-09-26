#include "GraphicsViewUtil.hpp"
#include "SystemUtil.hpp"
#include "Tst/Tst.hpp"

#include "Zancle/Graphics/BlendMode.hpp"
#include "Zancle/Graphics/Color.hpp"
#include "Zancle/Graphics/StencilMode.hpp"
#include "Zancle/Graphics/Transform.hpp"
#include "Zancle/Graphics/View.hpp"

#include "Zancle/String/String.hpp"
#include "Zancle/String/ToChars.hpp"

#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Math/Fabs.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/SizeT.hpp"

#include "Zancle/Trait/IsFloatingPoint.hpp"


namespace
{
////////////////////////////////////////////////////////////
za::String gfxFloatToString(const float value, const int precision = 6)
{
    char       buf[64];
    char*      end = za::toChars(buf, buf + sizeof(buf), value, precision);
    const auto len = static_cast<za::SizeT>(end - buf);
    return za::String{buf, len};
}


////////////////////////////////////////////////////////////
template <typename T>
za::String gfxIntToString(const T value)
{
    char       buf[32];
    char*      end = za::toChars(buf, buf + sizeof(buf), value);
    const auto len = static_cast<za::SizeT>(end - buf);
    return za::String{buf, len};
}


////////////////////////////////////////////////////////////
za::String hexToString(const za::U32 value)
{
    char buf[10];
    buf[0]    = '0';
    buf[1]    = 'x';
    int shift = 32;
    int pos   = 2;
    do
    {
        shift -= 4;
        const unsigned nibble = (value >> shift) & 0xFu;
        buf[pos++]            = static_cast<char>(nibble < 10 ? '0' + nibble : 'a' + (nibble - 10));
    } while (shift > 0);
    return za::String{buf, static_cast<za::SizeT>(pos)};
}


////////////////////////////////////////////////////////////
za::String stencilComparisonToString(const za::StencilComparison comparison)
{
    switch (comparison)
    {
        case za::StencilComparison::Never:
            return za::String{"Never"};
        case za::StencilComparison::Less:
            return za::String{"Less"};
        case za::StencilComparison::LessEqual:
            return za::String{"LessEqual"};
        case za::StencilComparison::Greater:
            return za::String{"Greater"};
        case za::StencilComparison::GreaterEqual:
            return za::String{"GreaterEqual"};
        case za::StencilComparison::Equal:
            return za::String{"Equal"};
        case za::StencilComparison::NotEqual:
            return za::String{"NotEqual"};
        case za::StencilComparison::Always:
            return za::String{"Always"};
    }
    return za::String{};
}


////////////////////////////////////////////////////////////
za::String stencilUpdateOperationToString(const za::StencilUpdateOperation updateOperation)
{
    switch (updateOperation)
    {
        case za::StencilUpdateOperation::Keep:
            return za::String{"Keep"};
        case za::StencilUpdateOperation::Zero:
            return za::String{"Zero"};
        case za::StencilUpdateOperation::Replace:
            return za::String{"Replace"};
        case za::StencilUpdateOperation::Increment:
            return za::String{"Increment"};
        case za::StencilUpdateOperation::Decrement:
            return za::String{"Decrement"};
        case za::StencilUpdateOperation::Invert:
            return za::String{"Invert"};
    }
    return za::String{};
}


////////////////////////////////////////////////////////////
za::String vec2ToString(const za::Vec2<float> v)
{
    return za::String{"("} + gfxFloatToString(v.x) + za::String{", "} + gfxFloatToString(v.y) + za::String{")"};
}


////////////////////////////////////////////////////////////
template <typename T>
za::String rectToString(const za::Rect2<T>& rect)
{
    if constexpr (za::isFloatingPoint<T>)
        return za::String{"(position=("} + gfxFloatToString(rect.position.x) + za::String{", "} +
               gfxFloatToString(rect.position.y) + za::String{"), size=("} + gfxFloatToString(rect.size.x) +
               za::String{", "} + gfxFloatToString(rect.size.y) + za::String{"))"};
    else
        return za::String{"(position=("} + gfxIntToString(rect.position.x) + za::String{", "} +
               gfxIntToString(rect.position.y) + za::String{"), size=("} + gfxIntToString(rect.size.x) +
               za::String{", "} + gfxIntToString(rect.size.y) + za::String{"))"};
}


////////////////////////////////////////////////////////////
za::String blendModeToString(const za::BlendMode& blendMode)
{
    return za::String{"( "} + gfxIntToString(static_cast<int>(blendMode.colorSrcFactor)) + za::String{", "} +
           gfxIntToString(static_cast<int>(blendMode.colorDstFactor)) + za::String{", "} +
           gfxIntToString(static_cast<int>(blendMode.colorEquation)) + za::String{", "} +
           gfxIntToString(static_cast<int>(blendMode.alphaSrcFactor)) + za::String{", "} +
           gfxIntToString(static_cast<int>(blendMode.alphaDstFactor)) + za::String{", "} +
           gfxIntToString(static_cast<int>(blendMode.alphaEquation)) + za::String{" )"};
}


////////////////////////////////////////////////////////////
za::String stencilModeToString(const za::StencilMode& stencilMode)
{
    return za::String{"( "} + stencilComparisonToString(stencilMode.stencilComparison) + za::String{", "} +
           stencilUpdateOperationToString(stencilMode.stencilUpdateOperation) + za::String{", "} +
           za::String{stencilMode.stencilOnly ? "true" : "false"} + za::String{", "} +
           gfxIntToString(static_cast<unsigned int>(stencilMode.stencilReference.value)) + za::String{", "} +
           gfxIntToString(static_cast<unsigned int>(stencilMode.stencilMask.value)) + za::String{" )"};
}


////////////////////////////////////////////////////////////
za::String colorToString(const za::Color color)
{
    return hexToString(color.toInteger()) + za::String{" (r="} + gfxIntToString(int{color.r}) + za::String{", g="} +
           gfxIntToString(int{color.g}) + za::String{", b="} + gfxIntToString(int{color.b}) + za::String{", a="} +
           gfxIntToString(int{color.a}) + za::String{")"};
}


////////////////////////////////////////////////////////////
za::String transformToString(const za::Transform& transform)
{
    // clang-format off
    float matrix[]{{},  {},  0.f, 0.f,
                   {},  {},  0.f, 0.f,
                   0.f, 0.f, 1.f, 0.f,
                   {},  {},  0.f, 1.f};
    // clang-format on

    transform.writeTo4x4Matrix(matrix);

    return gfxFloatToString(matrix[0]) + za::String{", "} + gfxFloatToString(matrix[4]) + za::String{", "} +
           gfxFloatToString(matrix[12]) + za::String{", "} + gfxFloatToString(matrix[1]) + za::String{", "} +
           gfxFloatToString(matrix[5]) + za::String{", "} + gfxFloatToString(matrix[13]) + za::String{", "} +
           gfxFloatToString(matrix[3]) + za::String{", "} + gfxFloatToString(matrix[7]) + za::String{", "} +
           gfxFloatToString(matrix[15]);
}


////////////////////////////////////////////////////////////
za::String viewToString(const za::View& view)
{
    return za::String{"( center="} + vec2ToString(view.center) + za::String{", size="} + vec2ToString(view.size) +
           za::String{", rotation="} + gfxFloatToString(view.rotation.asDegrees()) + za::String{" deg"} +
           za::String{", viewport="} + rectToString(view.viewport) + za::String{", scissor="} +
           rectToString(view.scissor) + za::String{" )"};
}

} // namespace


namespace za
{
////////////////////////////////////////////////////////////
za::SizeT stringifyValue(char* buf, za::SizeT cap, const za::BlendMode& blendMode) noexcept
{
    const za::String s = blendModeToString(blendMode);
    return ::tst::detail::copyInto(buf, cap, s.data(), s.size());
}


////////////////////////////////////////////////////////////
za::SizeT stringifyValue(char* buf, za::SizeT cap, const za::StencilComparison comparison) noexcept
{
    const za::String s = stencilComparisonToString(comparison);
    return ::tst::detail::copyInto(buf, cap, s.data(), s.size());
}


////////////////////////////////////////////////////////////
za::SizeT stringifyValue(char* buf, za::SizeT cap, const za::StencilUpdateOperation updateOperation) noexcept
{
    const za::String s = stencilUpdateOperationToString(updateOperation);
    return ::tst::detail::copyInto(buf, cap, s.data(), s.size());
}


////////////////////////////////////////////////////////////
za::SizeT stringifyValue(char* buf, za::SizeT cap, const za::StencilMode& stencilMode) noexcept
{
    const za::String s = stencilModeToString(stencilMode);
    return ::tst::detail::copyInto(buf, cap, s.data(), s.size());
}


////////////////////////////////////////////////////////////
za::SizeT stringifyValue(char* buf, za::SizeT cap, const za::Color& color) noexcept
{
    const za::String s = colorToString(color);
    return ::tst::detail::copyInto(buf, cap, s.data(), s.size());
}


////////////////////////////////////////////////////////////
za::SizeT stringifyValue(char* buf, za::SizeT cap, const za::Transform& transform) noexcept
{
    const za::String s = transformToString(transform);
    return ::tst::detail::copyInto(buf, cap, s.data(), s.size());
}


////////////////////////////////////////////////////////////
za::SizeT stringifyValue(char* buf, za::SizeT cap, const za::View& view) noexcept
{
    const za::String s = viewToString(view);
    return ::tst::detail::copyInto(buf, cap, s.data(), s.size());
}


////////////////////////////////////////////////////////////
za::SizeT stringifyValue(char* buf, za::SizeT cap, const za::View::ScissorRect& scissorRect) noexcept
{
    const za::String s = rectToString(static_cast<const za::Rect2<float>&>(scissorRect));
    return ::tst::detail::copyInto(buf, cap, s.data(), s.size());
}

} // namespace za


////////////////////////////////////////////////////////////
bool operator==(const za::Transform& lhs, const Approx<za::Transform>& rhs)
{
    // clang-format off
    float lhsMatrix[]{{},  {},  0.f, 0.f,
                      {},  {},  0.f, 0.f,
                      0.f, 0.f, 1.f, 0.f,
                      {},  {},  0.f, 1.f};

    float rhsMatrix[]{{},  {},  0.f, 0.f,
                      {},  {},  0.f, 0.f,
                      0.f, 0.f, 1.f, 0.f,
                      {},  {},  0.f, 1.f};
    // clang-format on

    lhs.writeTo4x4Matrix(lhsMatrix);
    rhs.value.writeTo4x4Matrix(rhsMatrix);

    return lhsMatrix[0] == Approx(rhsMatrix[0]) && lhsMatrix[4] == Approx(rhsMatrix[4]) &&
           lhsMatrix[12] == Approx(rhsMatrix[12]) && lhsMatrix[1] == Approx(rhsMatrix[1]) &&
           lhsMatrix[5] == Approx(rhsMatrix[5]) && lhsMatrix[13] == Approx(rhsMatrix[13]) &&
           lhsMatrix[3] == Approx(rhsMatrix[3]) && lhsMatrix[7] == Approx(rhsMatrix[7]) &&
           lhsMatrix[15] == Approx(rhsMatrix[15]);
}


////////////////////////////////////////////////////////////
bool operator==(const za::Color& lhs, const Approx<za::Color>& rhs)
{
    return za::fabs(static_cast<float>(lhs.r - rhs.value.r)) < 2.f &&
           za::fabs(static_cast<float>(lhs.g - rhs.value.g)) < 2.f &&
           za::fabs(static_cast<float>(lhs.b - rhs.value.b)) < 2.f &&
           za::fabs(static_cast<float>(lhs.a - rhs.value.a)) < 2.f;
}
