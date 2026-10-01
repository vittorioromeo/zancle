#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Fmt/FmtResult.hpp"
#include "Zancle/Fmt/FmtSink.hpp"
#include "Zancle/Fmt/FmtSpec.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/SizeT.hpp"


namespace za
{
////////////////////////////////////////////////////////////
// String-like: anything with byte `.data()` and `.size()`
//
// The constraint is in the template head: as a trailing requires-clause, it keeps MSVC from matching
// the explicit instantiations of the numeric `fmtArg` templates (see `FmtNumeric.hpp`)
template <typename T>
    requires requires(const T& arg) {
        static_cast<const char*>(arg.data());
        static_cast<SizeT>(arg.size());
    }
[[nodiscard, gnu::always_inline]] inline constexpr FmtResult fmtArg(FmtSink&                        sink,
                                                                    const T&                        arg,
                                                                    [[maybe_unused]] const FmtSpec& spec) noexcept
{
    if (spec.precision >= 0 || spec.type != '\0') [[unlikely]]
    {
        ZA_ASSERT(false && "invalid string format spec");
        return FmtResult::Failed;
    }

    return sink.append(static_cast<const char*>(arg.data()), static_cast<SizeT>(arg.size()));
}

} // namespace za


////////////////////////////////////////////////////////////
/// \file
/// Built-in `fmtArg` for string-like types: anything exposing byte
/// `.data()` and `.size()` (e.g. `za::String`, `za::StringView`).
///
////////////////////////////////////////////////////////////
