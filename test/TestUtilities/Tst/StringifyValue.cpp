// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Tst/Approx.hpp"
#include "Tst/Detail/StringifyValue.hpp"

#include "Zancle/Fmt/Fmt.hpp"
#include "Zancle/Fmt/FmtCString.hpp" // IWYU pragma: keep
#include "Zancle/Fmt/FmtNumeric.hpp" // IWYU pragma: keep
#include "Zancle/Fmt/FmtSink.hpp"

#include "Zancle/Geometry/Rect2.hpp"
#include "Zancle/Geometry/Vec2.hpp"
#include "Zancle/Geometry/Vec3.hpp"

#include "Zancle/Base/Memcpy.hpp"
#include "Zancle/Base/SizeT.hpp"


////////////////////////////////////////////////////////////
// The single TU that implements `stringifyValue`. Everything heavy --
// `<Fmt.hpp>`, the numeric/cstring formatters, and the full value-type
// headers -- is confined here; the public header stays at `SizeT` + a few
// forward declarations. Each overload wraps the caller-provided
// `char* + cap` sink into a `FmtSink` internally.
////////////////////////////////////////////////////////////


namespace tst::detail
{
////////////////////////////////////////////////////////////
za::SizeT copyInto(char* buf, za::SizeT cap, const char* data, za::SizeT n) noexcept
{
    const za::SizeT toCopy = n < cap ? n : cap;
    if (toCopy != 0u)
        ZA_MEMCPY(buf, data, toCopy);
    return toCopy;
}


////////////////////////////////////////////////////////////
#define ZA_TST_DEFINE_STRINGIFY_NUMERIC(T)                           \
    za::SizeT stringifyValue(char* buf, za::SizeT cap, T v) noexcept \
    {                                                                \
        za::FmtSink sink{buf, cap};                                  \
        (void)sink.fmt("{}", v);                                     \
        return sink.size();                                          \
    }

ZA_TST_DEFINE_STRINGIFY_NUMERIC(short)
ZA_TST_DEFINE_STRINGIFY_NUMERIC(int)
ZA_TST_DEFINE_STRINGIFY_NUMERIC(long)
ZA_TST_DEFINE_STRINGIFY_NUMERIC(long long)

ZA_TST_DEFINE_STRINGIFY_NUMERIC(unsigned short)
ZA_TST_DEFINE_STRINGIFY_NUMERIC(unsigned int)
ZA_TST_DEFINE_STRINGIFY_NUMERIC(unsigned long)
ZA_TST_DEFINE_STRINGIFY_NUMERIC(unsigned long long)

ZA_TST_DEFINE_STRINGIFY_NUMERIC(float)
ZA_TST_DEFINE_STRINGIFY_NUMERIC(double)
ZA_TST_DEFINE_STRINGIFY_NUMERIC(long double)

#undef ZA_TST_DEFINE_STRINGIFY_NUMERIC


////////////////////////////////////////////////////////////
za::SizeT stringifyValue(char* buf, za::SizeT cap, bool v) noexcept
{
    za::FmtSink sink{buf, cap};
    (void)sink.append(v ? "true" : "false", v ? 4u : 5u);
    return sink.size();
}


////////////////////////////////////////////////////////////
za::SizeT stringifyValue(char* buf, za::SizeT cap, char v) noexcept
{
    za::FmtSink sink{buf, cap};
    (void)sink.fmt("'{}'", v);
    return sink.size();
}


////////////////////////////////////////////////////////////
// Wide character types: render as `'X'` when the codepoint lands in printable
// ASCII (lets `*next == u8'Z'` failures show "'a' == 'Z'"), otherwise as
// `U+XXXX` so non-printables and emoji still round-trip readably.
#define ZA_TST_DEFINE_STRINGIFY_WIDECHAR(T)                          \
    za::SizeT stringifyValue(char* buf, za::SizeT cap, T v) noexcept \
    {                                                                \
        za::FmtSink sink{buf, cap};                                  \
        const auto  cp = static_cast<unsigned long>(v);              \
                                                                     \
        if (cp >= 0x20u && cp <= 0x7Eu)                              \
            (void)sink.fmt("'{}'", static_cast<char>(cp));           \
        else                                                         \
            (void)sink.fmt("U+{:04X}", cp);                          \
                                                                     \
        return sink.size();                                          \
    }

ZA_TST_DEFINE_STRINGIFY_WIDECHAR(char8_t)
ZA_TST_DEFINE_STRINGIFY_WIDECHAR(char16_t)
ZA_TST_DEFINE_STRINGIFY_WIDECHAR(char32_t)
ZA_TST_DEFINE_STRINGIFY_WIDECHAR(wchar_t)

#undef ZA_TST_DEFINE_STRINGIFY_WIDECHAR


////////////////////////////////////////////////////////////
za::SizeT stringifyValue(char* buf, za::SizeT cap, const char* v) noexcept
{
    za::FmtSink sink{buf, cap};

    if (v == nullptr)
        (void)sink.append("<null>", 6u);
    else
        (void)sink.fmt("\"{}\"", v);

    return sink.size();
}


////////////////////////////////////////////////////////////
template <typename T>
za::SizeT stringifyValue(char* buf, za::SizeT cap, const za::Vec2<T>& v) noexcept
{
    za::FmtSink sink{buf, cap};
    (void)sink.fmt("({}, {})", v.x, v.y);
    return sink.size();
}


////////////////////////////////////////////////////////////
template <typename T>
za::SizeT stringifyValue(char* buf, za::SizeT cap, const za::Vec3<T>& v) noexcept
{
    za::FmtSink sink{buf, cap};
    (void)sink.fmt("({}, {}, {})", v.x, v.y, v.z);
    return sink.size();
}


////////////////////////////////////////////////////////////
template <typename T>
za::SizeT stringifyValue(char* buf, za::SizeT cap, const za::Rect2<T>& v) noexcept
{
    za::FmtSink sink{buf, cap};
    (void)sink.fmt("[pos=({}, {}) size=({}, {})]", v.position.x, v.position.y, v.size.x, v.size.y);
    return sink.size();
}


////////////////////////////////////////////////////////////
za::SizeT stringifyValue(char* buf, za::SizeT cap, const tst::Approx& v) noexcept
{
    za::FmtSink sink{buf, cap};
    (void)sink.fmt("Approx({})", v.value());
    return sink.size();
}


////////////////////////////////////////////////////////////
template za::SizeT stringifyValue(char*, za::SizeT, const za::Vec2<int>&) noexcept;
template za::SizeT stringifyValue(char*, za::SizeT, const za::Vec2<unsigned int>&) noexcept;
template za::SizeT stringifyValue(char*, za::SizeT, const za::Vec2<float>&) noexcept;

template za::SizeT stringifyValue(char*, za::SizeT, const za::Vec3<int>&) noexcept;
template za::SizeT stringifyValue(char*, za::SizeT, const za::Vec3<unsigned int>&) noexcept;
template za::SizeT stringifyValue(char*, za::SizeT, const za::Vec3<float>&) noexcept;

template za::SizeT stringifyValue(char*, za::SizeT, const za::Rect2<int>&) noexcept;
template za::SizeT stringifyValue(char*, za::SizeT, const za::Rect2<unsigned int>&) noexcept;
template za::SizeT stringifyValue(char*, za::SizeT, const za::Rect2<float>&) noexcept;

} // namespace tst::detail
