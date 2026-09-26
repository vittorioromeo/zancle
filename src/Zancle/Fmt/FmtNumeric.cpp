// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Fmt/FmtNumeric.hpp"

#include "Zancle/Fmt/Fmt.hpp"
#include "Zancle/Fmt/FmtResult.hpp"
#include "Zancle/Fmt/FmtSink.hpp"
#include "Zancle/Fmt/FmtSpec.hpp"

#include "Zancle/String/ToChars.hpp"
#include "Zancle/String/ToCharsRadix.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/SizeT.hpp"

#include "Zancle/Trait/IsSame.hpp"


namespace za
{
////////////////////////////////////////////////////////////
template <typename T>
    requires isIntegral<T>
FmtResult fmtArg(FmtSink& sink, const T& arg, const FmtSpec& spec)
{
    if (spec.precision >= 0 || spec.type == 'f') [[unlikely]]
    {
        ZA_ASSERT(false && "invalid integral format spec");
        return FmtResult::Failed;
    }

    // 64 covers the worst case: 64-bit binary (`{:b}` on `unsigned long long`).
    // Decimal and signed cases (max 20 chars + sign) and hex/oct also fit, so
    // the helper returns a non-null pointer on every code path below.
    char buf[64];

    const char* end = nullptr;

    // `bool` is formatted as a string ("true" / "false") by default, matching
    // `std::format` and `fmt::format`. The radix specs (`{:b}`, `{:x}`, etc.)
    // and `MakeUnsigned<bool>` would be ill-formed anyway, so the path is
    // bypassed entirely. Callers who want numeric output can cast to `int`.
    if constexpr (ZA_IS_SAME(T, bool))
    {
        if (spec.type != '\0') [[unlikely]]
        {
            ZA_ASSERT(false && "invalid bool format spec");
            return FmtResult::Failed;
        }

        return arg ? sink.append("true", 4u) : sink.append("false", 5u);
    }
    else
    {
        switch (spec.type)
        {
            case 'x':
                end = toCharsRadix(buf, buf + sizeof(buf), arg, Radix::Hex, /* upperHex */ false);
                break;
            case 'X':
                end = toCharsRadix(buf, buf + sizeof(buf), arg, Radix::Hex, /* upperHex */ true);
                break;
            case 'o':
                end = toCharsRadix(buf, buf + sizeof(buf), arg, Radix::Oct);
                break;
            case 'b':
                end = toCharsRadix(buf, buf + sizeof(buf), arg, Radix::Bin);
                break;
            case 'c': // emit lowest byte as a single glyph (matches `libfmt`)
                return sink.appendChar(static_cast<char>(arg));
            case '\0':
            case 'd':
                end = toChars(buf, buf + sizeof(buf), arg);
                break;
            default:
                ZA_ASSERT(false && "invalid integral format spec");
                return FmtResult::Failed;
        }
    }

    ZA_ASSERT(end != nullptr && "Numeric `fmtArg` ran out of buffer -- buf[64] should always fit");
    return end == nullptr ? FmtResult::Failed : sink.append(buf, static_cast<SizeT>(end - buf));
}


////////////////////////////////////////////////////////////
template <typename T>
    requires isFloatingPoint<T>
FmtResult fmtArg(FmtSink& sink, const T& arg, const FmtSpec& spec)
{
    if (spec.type != '\0' && spec.type != 'f') [[unlikely]]
    {
        ZA_ASSERT(false && "invalid floating-point format spec");
        return FmtResult::Failed;
    }

    const int prec = spec.precision >= 0 ? spec.precision : defaultFloatPrecision;

    // 40 covers sign + ~20-digit integral part + '.' + up to 10 fractional digits + slack.
    char buf[40];

    char* const end = toChars(buf, buf + sizeof(buf), arg, prec);

    if (end == nullptr)
        return FmtResult::Failed;

    return sink.append(buf, static_cast<SizeT>(end - buf));
}


////////////////////////////////////////////////////////////
// Explicit instantiations -- match the `extern template` decls in the header.
// Each row emits three symbols: `fmtArg<T>`, `priv::dispatchFmtArg<T>`,
// `priv::dispatchFmtArgErased<T>`. The last is the one `fmtAssembleImpl`'s
// dispatcher table takes the address of; pre-emitting them avoids per-TU
// weak symbols.
////////////////////////////////////////////////////////////
#define ZA_FMT_INSTANTIATE(T)                                                       \
    template FmtResult fmtArg<T>(FmtSink&, const T&, const FmtSpec&);               \
    template FmtResult priv::dispatchFmtArg<T>(FmtSink&, const T&, const FmtSpec&); \
    template FmtResult priv::dispatchFmtArgErased<T>(FmtSink&, const void*, const FmtSpec&)

ZA_FMT_INSTANTIATE(bool);

ZA_FMT_INSTANTIATE(signed char);
ZA_FMT_INSTANTIATE(short);
ZA_FMT_INSTANTIATE(int);
ZA_FMT_INSTANTIATE(long);
ZA_FMT_INSTANTIATE(long long);

ZA_FMT_INSTANTIATE(unsigned char);
ZA_FMT_INSTANTIATE(unsigned short);
ZA_FMT_INSTANTIATE(unsigned int);
ZA_FMT_INSTANTIATE(unsigned long);
ZA_FMT_INSTANTIATE(unsigned long long);

ZA_FMT_INSTANTIATE(float);
ZA_FMT_INSTANTIATE(double);
ZA_FMT_INSTANTIATE(long double);

#undef ZA_FMT_INSTANTIATE


////////////////////////////////////////////////////////////
FmtResult fmtArg(FmtSink& sink, const char& arg, const FmtSpec& spec)
{
    if (spec.precision >= 0) [[unlikely]]
    {
        ZA_ASSERT(false && "invalid char format spec");
        return FmtResult::Failed;
    }

    // Default ('\0') and explicit `:c` -> glyph. Matches `libfmt` /
    // `std::format`: passing a character literal to `{}` prints the
    // character, not its code-point. The numeric tags fall through to
    // the integer formatter (cast preserves signedness, like `libfmt`).
    if (spec.type == '\0' || spec.type == 'c')
        return sink.appendChar(arg);

    return fmtArg<int>(sink, static_cast<int>(arg), spec);
}


////////////////////////////////////////////////////////////
// Dispatcher instantiations for `char`. The `fmtArg` symbol is provided
// by the non-template definition above, so it is NOT instantiated here.
template FmtResult priv::dispatchFmtArg<char>(FmtSink&, const char&, const FmtSpec&);
template FmtResult priv::dispatchFmtArgErased<char>(FmtSink&, const void*, const FmtSpec&);

} // namespace za
