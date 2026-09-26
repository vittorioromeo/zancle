#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////

#include "Zancle/Config.hpp"

#include "Zancle/Fmt/FmtArgDefaultAlign.hpp" // IWYU pragma: export
#include "Zancle/Fmt/FmtCString.hpp"         // IWYU pragma: export
#include "Zancle/Fmt/FmtResult.hpp"
#include "Zancle/Fmt/FmtSink.hpp"    // IWYU pragma: export
#include "Zancle/Fmt/FmtSinkRef.hpp" // IWYU pragma: export
#include "Zancle/Fmt/FmtSpan.hpp"    // IWYU pragma: export
#include "Zancle/Fmt/FmtSpec.hpp"    // IWYU pragma: export
#include "Zancle/Fmt/FmtString.hpp"  // IWYU pragma: export

#include "Zancle/Base/AssertAndAssume.hpp"
#include "Zancle/Base/Memcpy.hpp"
#include "Zancle/Base/Memmove.hpp"
#include "Zancle/Base/Memset.hpp"
#include "Zancle/Base/NonDeduced.hpp"
#include "Zancle/Base/SizeT.hpp"
#include "Zancle/Base/Strlen.hpp"

#include "Zancle/Trait/IsSame.hpp"


////////////////////////////////////////////////////////////
// This header intentionally OMITS the numeric `fmtArg` overloads
// (int, float) -- they live in `<Zancle/Fmt/FmtNumeric.hpp>` and pull
// in `ToChars` + integer/floating-point traits. Callers that want to
// format numeric arguments should include `FmtNumeric.hpp` (or one
// of the umbrella headers that re-exports it, such as `FmtToString.hpp`
// or `<Zancle/Err/Err.hpp>`).
//
////////////////////////////////////////////////////////////


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Default float precision when a spec omits `.N` (matches `std::format` / `printf`).
///
////////////////////////////////////////////////////////////
inline constexpr int defaultFloatPrecision = 6;


////////////////////////////////////////////////////////////
/// \brief Initial stack-staging size for `fmtTo` / `print`. NOT a
/// hard limit: an overflow falls back to a doubling heap buffer, so
/// any output size is supported.
///
////////////////////////////////////////////////////////////
inline constexpr SizeT fmtScratchSize = 512u;

} // namespace za


namespace za::priv
{
////////////////////////////////////////////////////////////
// Dual-mode failure for `parseFmtSpec`. At consteval the `throw` makes
// the format-string-literal constructor reject the bad input with the
// diagnostic string surfacing in the compile error. At runtime the
// assert fires in debug (stringifying `false && (msg)` keeps the literal
// text in the log) and `[[assume(false)]]` makes the path unreachable
// in release so the optimizer drops the dead checks.
////////////////////////////////////////////////////////////
#define ZA_PRIV_FMT_FAIL(msg)                 \
    do                                        \
    {                                         \
        if consteval                          \
        {                                     \
            throw(msg);                       \
        }                                     \
                                              \
        ZA_ASSERT_AND_ASSUME(false && (msg)); \
                                              \
    } while (false)


////////////////////////////////////////////////////////////
/// \brief Parse a format spec body (everything between '{:' and '}').
///
/// `p` is advanced to the position of the closing '}'. Caller verifies '}' presence.
/// At consteval, throws on width/precision overflow for a clear compile-time
/// diagnostic; at runtime, the corresponding asserts fire (debug only).
///
/// Grammar:
///     spec ::= [[fill]align]? [width]? [.precision]? [type]?
///
////////////////////////////////////////////////////////////
constexpr void parseFmtSpec(const char*& p, const char* const end, FmtSpec& spec)
{
    constexpr auto isAlignChar = [](const char c) { return c == '<' || c == '>' || c == '^'; };
    constexpr auto isDigit     = [](const char c) { return c >= '0' && c <= '9'; };

    if (p + 1 < end && isAlignChar(p[1]))
    {
        spec.fill  = *p++;
        spec.align = *p++;
    }
    else if (p < end && isAlignChar(*p))
    {
        spec.align = *p++;
    }

    while (p < end && isDigit(*p))
    {
        spec.width = spec.width * 10 + (*p - '0');

        if (spec.width >= 65'536)
            ZA_PRIV_FMT_FAIL("Format spec width out of range (>= 65536)");

        ++p;
    }

    if (p < end && *p == '.')
    {
        ++p;

        if (p >= end || !isDigit(*p))
            ZA_PRIV_FMT_FAIL("Format spec precision requires at least one digit");

        spec.precision = 0;

        while (p < end && isDigit(*p))
        {
            spec.precision = spec.precision * 10 + (*p - '0');

            // Upper bound matches the float backend (`toChars` asserts precision <= 10).
            if (spec.precision > 10)
                ZA_PRIV_FMT_FAIL("Format spec precision out of range (> 10)");

            ++p;
        }
    }

    // Type tag. Accepted set: 'd' (decimal int), 'x' / 'X' (hex), 'o' (octal),
    // 'b' (binary), 'c' (char glyph), 'f' (float fixed). Anything else is malformed.
    if (p < end && *p != '}')
    {
        const char t = *p;

        if (t != 'd' && t != 'x' && t != 'X' && t != 'o' && t != 'b' && t != 'c' && t != 'f')
            ZA_PRIV_FMT_FAIL("Format spec type tag must be one of d/x/X/o/b/c/f or omitted");

        spec.type = t;
        ++p;
    }
}


////////////////////////////////////////////////////////////
#undef ZA_PRIV_FMT_FAIL


////////////////////////////////////////////////////////////
/// \brief Per-format-string properties extracted at consteval.
///
/// `placeholderCount` is matched against `sizeof...(Args)` for the count
/// invariant. `hasEscapes` is set when the parser steps over a `{{` or
/// `}}` -- the runtime needs to know about these because they require
/// `{` / `}` translation and disqualify the literal-only fast path.
////////////////////////////////////////////////////////////
struct FmtStringInfo
{
    SizeT placeholderCount;
    bool  hasEscapes;
};


////////////////////////////////////////////////////////////
/// \brief Walk the format string once, validating each placeholder and
/// reporting placeholder count + escape presence.
///
/// Throws (at consteval) on:
///   - unclosed `{`
///   - unescaped `}`
///   - malformed spec body (anything that leaves the parser short of `}`)
///   - width >= 65536 or precision > 10
///   - unknown type tag (anything outside `d` / `x` / `X` / `o` / `b` / `c` / `f`)
////////////////////////////////////////////////////////////
[[nodiscard]] consteval FmtStringInfo analyzeFmtString(const FmtSpan fmtStr)
{
    const auto*       p   = fmtStr.data;
    const auto* const end = p + fmtStr.size;

    FmtStringInfo info{0u, false};

    while (p < end)
    {
        if (*p == '{')
        {
            // Escaped '{{' -> '{'
            if (p + 1 < end && p[1] == '{')
            {
                info.hasEscapes = true;
                p += 2;
                continue;
            }

            ++p; // skip '{'

            // Optional spec body
            if (p < end && *p == ':')
            {
                ++p; // skip ':'
                FmtSpec spec;
                parseFmtSpec(p, end, spec);
            }

            // Bare throws here (no `ZA_PRIV_FMT_FAIL`): this
            // function is `consteval`, so the macro's `if consteval`
            // branch would be trivially always-true and trigger
            // `-Wredundant-consteval-if`. The macro stays for
            // `parseFmtSpec`, which is `constexpr` and reachable at
            // runtime.
            if (p >= end)
                throw "Invalid format string: Unclosed '{'";

            if (*p != '}')
                throw "Invalid format string: malformed spec (expected '}')";

            ++p; // skip '}'
            ++info.placeholderCount;
        }
        else if (*p == '}')
        {
            // Escaped '}}' -> '}'
            if (p + 1 < end && p[1] == '}')
            {
                info.hasEscapes = true;
                p += 2;
                continue;
            }
            throw "Invalid format string: unescaped '}'";
        }
        else
        {
            ++p;
        }
    }

    return info;
}

} // namespace za::priv


////////////////////////////////////////////////////////////
namespace za
{
////////////////////////////////////////////////////////////
/// \brief Compile-time-validated format string: ensures placeholder
/// count matches `sizeof...(Args)` and exposes the "pure-literal"
/// property used by the runtime fast path.
///
/// `isPureLiteral` is true when the string carries no placeholders AND
/// no `{{` / `}}` escapes -- in that case the runtime can skip the whole
/// assemble loop and write the bytes directly to the sink.
////////////////////////////////////////////////////////////
template <typename... Args>
struct [[nodiscard]] FmtString
{
    priv::FmtSpan str;
    bool          isPureLiteral = false;

    consteval FmtString(const char* const s) : str{s, ZA_STRLEN(s)}
    {
        const priv::FmtStringInfo info = priv::analyzeFmtString(str);

        if (info.placeholderCount != sizeof...(Args))
            throw "Mismatch between number of '{}' and number of arguments.";

        isPureLiteral = (info.placeholderCount == 0u) && !info.hasEscapes;
    }
};

} // namespace za


////////////////////////////////////////////////////////////
namespace za::priv
{
////////////////////////////////////////////////////////////
/// \brief Apply per-placeholder padding around a `fmtArg` call.
///
/// Saves the sink position, runs the ADL `fmtArg`, measures content
/// length, and if `spec.width` exceeds it, applies fill+align in-place
/// via a single `memmove` + two `memset`s. The destination is the sink
/// itself -- no auxiliary scratch.
///
/// Body is in the header so user types still instantiate correctly via
/// ADL. Common numeric instantiations are pre-emitted in
/// `FmtNumeric.cpp` and declared `extern template` in
/// `FmtNumeric.hpp` so consumer TUs skip the per-TU codegen.
////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard]] constexpr FmtResult dispatchFmtArg(FmtSink& sink, const T& arg, const FmtSpec& spec) noexcept
{
    const SizeT startOffset = sink.size();

    ZA_FMT_TRY(fmtArg(sink, arg, spec)); // ADL

    if (spec.width <= 0)
        return FmtResult::Ok;

    const SizeT contentSize = sink.size() - startOffset;
    if (static_cast<SizeT>(spec.width) <= contentSize)
        return FmtResult::Ok;

    const SizeT padTotal = static_cast<SizeT>(spec.width) - contentSize;
    ZA_FMT_TRY(sink.ensureRoom(padTotal));

    // `:c` formats the value as a single glyph regardless of the source
    // argument type, so the visual semantics are character-like -- left-
    // align by default to match `libfmt` / `std::format`, overriding the
    // numeric `'>'` that `fmtArgDefaultAlign<T>` reports for integer T.
    constexpr char defAlign    = fmtArgDefaultAlign<T>;
    const char     effDefAlign = spec.type == 'c' ? '<' : defAlign;
    const char     align       = spec.align == '\0' ? effDefAlign : spec.align;

    const SizeT padLeft  = (align == '<') ? 0u : (align == '^') ? padTotal / 2u : padTotal;
    const SizeT padRight = padTotal - padLeft;

    char* const start = sink.atOffset(startOffset);

    if (padLeft != 0u)
    {
        ZA_MEMMOVE(start + padLeft, start, contentSize);
        ZA_MEMSET(start, spec.fill, padLeft);
    }

    if (padRight != 0u)
        ZA_MEMSET(start + padLeft + contentSize, spec.fill, padRight);

    sink.advance(padTotal);
    return FmtResult::Ok;
}


////////////////////////////////////////////////////////////
using ErasedDispatchFn = FmtResult (*)(FmtSink&, const void*, const FmtSpec&);


////////////////////////////////////////////////////////////
template <typename T>
[[nodiscard]] constexpr FmtResult dispatchFmtArgErased(FmtSink& sink, const void* const erasedArg, const FmtSpec& spec) noexcept
{
    return dispatchFmtArg(sink, *static_cast<const T*>(erasedArg), spec);
}


////////////////////////////////////////////////////////////
/// \brief Type-erased core. Walks `fmtStr` once, copying literals and
/// dispatching `{...}` placeholders via `dispatchers`.
////////////////////////////////////////////////////////////
[[nodiscard]] ZA_SYSTEM_API FmtResult
    fmtAssembleImpl(FmtSink& sink, FmtSpan fmtStr, const void* const* args, const ErasedDispatchFn* dispatchers, SizeT argCount);


////////////////////////////////////////////////////////////
/// \brief Heap-fallback for `fmtTo` / `print`. Allocates a doubling
/// `za::String` until the format succeeds, then flushes once
/// through `userSink`.
////////////////////////////////////////////////////////////
[[nodiscard]] ZA_SYSTEM_API FmtResult fmtToHeapFallback(
    FmtSinkRef              userSink,
    FmtSpan                 fmtStr,
    const void* const*      args,
    const ErasedDispatchFn* dispatchers,
    SizeT                   argCount);


////////////////////////////////////////////////////////////
/// \brief Heap-fallback for `fmtArgTo`. Same growing-buffer strategy
/// as `fmtToHeapFallback`, but calls a single `dispatcher` instead of
/// walking a format string.
////////////////////////////////////////////////////////////
[[nodiscard]] ZA_SYSTEM_API FmtResult
    fmtArgToHeapFallback(FmtSinkRef userSink, const void* erasedArg, ErasedDispatchFn dispatcher);


////////////////////////////////////////////////////////////
ZA_SYSTEM_API void fmtWriteStdout(const char* data, SizeT size);
ZA_SYSTEM_API void fmtWriteStdoutNewline();
ZA_SYSTEM_API void fmtFlushStdout();

ZA_SYSTEM_API void fmtWriteStderr(const char* data, SizeT size);
ZA_SYSTEM_API void fmtWriteStderrNewline();
ZA_SYSTEM_API void fmtFlushStderr();


////////////////////////////////////////////////////////////
struct StdoutSink
{
    void append(const char* data, SizeT n)
    {
        fmtWriteStdout(data, n);
    }
};


////////////////////////////////////////////////////////////
struct StderrSink
{
    void append(const char* data, SizeT n)
    {
        fmtWriteStderr(data, n);
    }
};


////////////////////////////////////////////////////////////
template <typename... Args>
[[nodiscard]] constexpr FmtResult fmtAssemble(FmtSink& sink, const FmtSpan fmtStr, const Args&... args)
{
    // Size both arrays at `sizeof...(Args) + 1` so the empty-pack case still
    // produces a 1-element array (zero-initialized, never indexed because
    // `argCount == 0`). The trailing slot is wasted but unconditional --
    // collapses the dispatch into a single code path with no branch.
    const void* const          erasedArgs[sizeof...(Args) + 1]  = {&args...};
    constexpr ErasedDispatchFn dispatchers[sizeof...(Args) + 1] = {&dispatchFmtArgErased<Args>...};

    return fmtAssembleImpl(sink, fmtStr, erasedArgs, dispatchers, sizeof...(Args));
}

} // namespace za::priv


////////////////////////////////////////////////////////////
namespace za
{
////////////////////////////////////////////////////////////
template <typename... Args>
constexpr FmtResult FmtSink::fmt(typename NonDeduced<const FmtString<Args...>>::type fmtStr, const Args&... args)
{
    return priv::fmtAssemble(*this, fmtStr.str, args...);
}


////////////////////////////////////////////////////////////
/// \brief Format into a fixed buffer.
///
/// Returns a pointer one past the last written character on success, or
/// `nullptr` on overflow / formatter failure. The buffer is **not**
/// null-terminated -- callers that need a C string must reserve an extra
/// byte and write `'\0'` at the returned pointer themselves. On failure the
/// buffer contents are indeterminate (may hold a partial write).
///
/// Matches `fmt::format_to` / `std::format_to` semantics; differs from
/// `snprintf` in that overflow is hard-fail rather than truncate.
////////////////////////////////////////////////////////////
template <typename... Args>
[[nodiscard]] constexpr char* fmtIntoBuffer(char* const                                         buffer,
                                            const SizeT                                         bufferSize,
                                            typename NonDeduced<const FmtString<Args...>>::type fmtStr,
                                            const Args&... args)
{
    // Fast path: literal-only string skips the whole assemble loop.
    if constexpr (sizeof...(Args) == 0)
    {
        if (fmtStr.isPureLiteral) [[likely]]
        {
            if (bufferSize < fmtStr.str.size)
                return nullptr;
            ZA_MEMCPY(buffer, fmtStr.str.data, fmtStr.str.size);
            return buffer + fmtStr.str.size;
        }
    }

    FmtSink sink{buffer, buffer + bufferSize};
    if (priv::fmtAssemble(sink, fmtStr.str, args...) != FmtResult::Ok)
        return nullptr;

    return sink.position();
}


////////////////////////////////////////////////////////////
template <SizeT N, typename... Args>
[[nodiscard]] constexpr char* fmtIntoBuffer(char (&buffer)[N],
                                            typename NonDeduced<const FmtString<Args...>>::type fmtStr,
                                            const Args&... args)
{
    return fmtIntoBuffer(buffer, N, fmtStr, args...);
}


////////////////////////////////////////////////////////////
/// \brief Sink that accepts `append(const char*, SizeT)`.
///
/// An `append` returning `FmtResult` has that result propagated. Other
/// return types are treated as infallible append operations.
////////////////////////////////////////////////////////////
template <typename T>
concept AppendSink = requires(T& sink, const char* p, SizeT n) { sink.append(p, n); };

} // namespace za


////////////////////////////////////////////////////////////
namespace za::priv
{
////////////////////////////////////////////////////////////
template <AppendSink Sink>
[[nodiscard, gnu::always_inline]] inline FmtResult appendToSink(Sink& sink, const char* const data, const SizeT size)
{
    if constexpr (ZA_IS_SAME(decltype(sink.append(data, size)), FmtResult))
        return sink.append(data, size);
    else
    {
        sink.append(data, size);
        return FmtResult::Ok;
    }
}


////////////////////////////////////////////////////////////
/// \brief Shared scratch-buffer + heap-fallback protocol for `fmtTo`-family
/// functions.
///
/// Templated on the sink type so the success-path flush is a direct,
/// inlined `userSink.append(...)` call (static dispatch). `FmtSinkRef`
/// satisfies `AppendSink`, so opting into dynamic dispatch just means
/// passing one explicitly.
///
/// On `overflow`, hands off to `heapFn(FmtSinkRef{userSink})` -- the
/// heap fallback lives out-of-line in `.cpp` and can't be templated on
/// `Sink` without dragging `String` into this header, so it always
/// accepts a type-erased ref. That indirect call only fires when the
/// scratch buffer overflows, which is rare.
///
/// `scratchFn` and `heapFn` are passed as templates so the lambdas are
/// fully inlined into the caller.
////////////////////////////////////////////////////////////
template <AppendSink Sink, typename ScratchFn, typename HeapFn>
[[nodiscard, gnu::always_inline]] inline FmtResult fmtViaScratchOrHeap(Sink& userSink, ScratchFn&& scratchFn, HeapFn&& heapFn)
{
    char    scratch[fmtScratchSize];
    FmtSink sink{scratch, scratch + sizeof(scratch)};

    const FmtResult result = scratchFn(sink);

    if (result == FmtResult::Ok) [[likely]]
        return appendToSink(userSink, scratch, sink.size());

    if (result == FmtResult::Failed)
        return FmtResult::Failed;

    return heapFn(FmtSinkRef{userSink});
}

} // namespace za::priv


////////////////////////////////////////////////////////////
namespace za::priv
{
////////////////////////////////////////////////////////////
/// \brief Shared implementation for `fmtTo`. Templated on `Sink` so
/// `userSink.append(...)` is a *dependent* member access -- only checked
/// at instantiation time.
////////////////////////////////////////////////////////////
template <AppendSink Sink, typename... Args>
FmtResult fmtToImpl(Sink& userSink, typename NonDeduced<const FmtString<Args...>>::type fmtStr, const Args&... args)
{
    // Fast path: literal-only string bypasses the scratch + assemble loop
    // and writes the bytes straight to the sink in one call.
    if constexpr (sizeof...(Args) == 0)
    {
        if (fmtStr.isPureLiteral) [[likely]]
            return priv::appendToSink(userSink, fmtStr.str.data, fmtStr.str.size);
    }

    // See `priv::fmtAssemble`: trailing-sentinel sizing avoids the empty-pack branch.
    const void* const                erasedArgs[sizeof...(Args) + 1]  = {&args...};
    constexpr priv::ErasedDispatchFn dispatchers[sizeof...(Args) + 1] = {&priv::dispatchFmtArgErased<Args>...};

    return priv::fmtViaScratchOrHeap(userSink, [&](FmtSink& sink) {
        return priv::fmtAssembleImpl(sink, fmtStr.str, erasedArgs, dispatchers, sizeof...(Args));
    }, [&](FmtSinkRef us) { return priv::fmtToHeapFallback(us, fmtStr.str, erasedArgs, dispatchers, sizeof...(Args)); });
}

} // namespace za::priv


////////////////////////////////////////////////////////////
namespace za
{
////////////////////////////////////////////////////////////
/// \brief Format into any sink with `append(const char*, SizeT)`.
///
/// Templated on `Sink` so the success path uses static dispatch
/// (`userSink.append(...)` is a direct, inlined call). Users who want
/// dynamic dispatch instead pass an `FmtSinkRef` (which itself satisfies
/// `AppendSink`); the same template handles both, but the indirect call
/// only happens for the type-erased case.
///
/// Tries to fit into a `fmtScratchSize`-byte stack buffer first; on
/// overflow falls back to a doubling heap buffer (no hard size limit).
/// Formatter failures are returned immediately. On success the sink
/// receives the formatted output in a single `append` call.
///
/// Sinks that genuinely cannot fail (e.g. `String` / `Utf8String`,
/// which grow on demand) can ignore the result via `(void)`. The
/// `FmtAppendMixin` does this internally for `.appendFmt(...)`.
////////////////////////////////////////////////////////////
template <AppendSink Sink, typename... Args>
[[nodiscard]] FmtResult fmtTo(Sink& userSink, typename NonDeduced<const FmtString<Args...>>::type fmtStr, const Args&... args)
{
    return priv::fmtToImpl<Sink, Args...>(userSink, fmtStr, args...);
}


////////////////////////////////////////////////////////////
/// \brief Stringify a single value directly into a sink, bypassing the
/// format-string machinery.
///
/// Calls the ADL `fmtArg(sink, value, {})` once into a `fmtScratchSize`
/// scratch buffer; on overflow falls back to the same doubling heap
/// strategy as `fmtTo`. Skips placeholder parsing, the erased-dispatch
/// table, and `fmtAssembleImpl` entirely -- intended for hot per-element
/// stringification (e.g. `concat(a, b, c)`).
////////////////////////////////////////////////////////////
template <AppendSink Sink, typename T>
[[nodiscard]] FmtResult fmtArgTo(Sink& userSink, const T& value)
{
    return priv::fmtViaScratchOrHeap(userSink,
                                     [&](FmtSink& sink) { return fmtArg(sink, value, FmtSpec{}); }, // ADL
                                     [&](FmtSinkRef us)
    { return priv::fmtArgToHeapFallback(us, &value, &priv::dispatchFmtArgErased<T>); });
}


////////////////////////////////////////////////////////////
/// \brief Format and write to stdout (no trailing newline; flushes
/// stdout). Use this for prompts and partial-line output.
////////////////////////////////////////////////////////////
template <typename... Args>
void print(typename NonDeduced<const FmtString<Args...>>::type fmtStr, const Args&... args)
{
    if constexpr (sizeof...(Args) == 0)
    {
        // Fast path: literal-only string goes straight to stdio, skipping the
        // 512-byte scratch buffer + assemble loop + final memcpy.
        if (fmtStr.isPureLiteral) [[likely]]
        {
            priv::fmtWriteStdout(fmtStr.str.data, fmtStr.str.size);
            priv::fmtFlushStdout();
            return;
        }
    }

    priv::StdoutSink sink;
    (void)fmtTo(sink, fmtStr, args...);
    priv::fmtFlushStdout();
}


////////////////////////////////////////////////////////////
/// \brief Format and write to stdout, followed by a newline.
/// No explicit flush -- on a TTY, '\n' triggers line-buffer flush;
/// on a piped stream, block buffering is preserved for throughput.
////////////////////////////////////////////////////////////
template <typename... Args>
void printLn(typename NonDeduced<const FmtString<Args...>>::type fmtStr, const Args&... args)
{
    if constexpr (sizeof...(Args) == 0)
    {
        if (fmtStr.isPureLiteral) [[likely]]
        {
            priv::fmtWriteStdout(fmtStr.str.data, fmtStr.str.size);
            priv::fmtWriteStdoutNewline();
            return;
        }
    }

    priv::StdoutSink sink;
    if (fmtTo(sink, fmtStr, args...) == FmtResult::Ok)
        priv::fmtWriteStdoutNewline();
}


////////////////////////////////////////////////////////////
/// \brief Format and write to stderr (no trailing newline; flushes
/// stderr). Use this for partial-line error output.
////////////////////////////////////////////////////////////
template <typename... Args>
void printErr(typename NonDeduced<const FmtString<Args...>>::type fmtStr, const Args&... args)
{
    if constexpr (sizeof...(Args) == 0)
    {
        if (fmtStr.isPureLiteral) [[likely]]
        {
            priv::fmtWriteStderr(fmtStr.str.data, fmtStr.str.size);
            priv::fmtFlushStderr();
            return;
        }
    }

    priv::StderrSink sink;
    (void)fmtTo(sink, fmtStr, args...);
    priv::fmtFlushStderr();
}


////////////////////////////////////////////////////////////
/// \brief Format and write to stderr, followed by a newline + flush.
////////////////////////////////////////////////////////////
template <typename... Args>
void printErrLn(typename NonDeduced<const FmtString<Args...>>::type fmtStr, const Args&... args)
{
    if constexpr (sizeof...(Args) == 0)
    {
        if (fmtStr.isPureLiteral) [[likely]]
        {
            priv::fmtWriteStderr(fmtStr.str.data, fmtStr.str.size);
            priv::fmtWriteStderrNewline();
            priv::fmtFlushStderr();
            return;
        }
    }

    priv::StderrSink sink;
    if (fmtTo(sink, fmtStr, args...) == FmtResult::Ok)
        priv::fmtWriteStderrNewline();
    priv::fmtFlushStderr();
}


////////////////////////////////////////////////////////////
/// \brief Format and append to an arbitrary sink (alias for `fmtTo`).
///
/// Exists for symmetry with the stdout/stderr `print` overloads and
/// with `std::print(FILE*, ...)`. No flush -- the sink decides its own
/// buffering policy.
////////////////////////////////////////////////////////////
template <AppendSink Sink, typename... Args>
void print(Sink& sink, typename NonDeduced<const FmtString<Args...>>::type fmtStr, const Args&... args)
{
    (void)fmtTo(sink, fmtStr, args...);
}


////////////////////////////////////////////////////////////
/// \brief Format, append to a sink, then append a trailing '\n'.
///
/// No flush; matches `std::println(FILE*, ...)` semantics. The newline
/// is emitted via a single byte `append` -- block-buffered sinks like
/// `FILE*` keep their throughput characteristics.
////////////////////////////////////////////////////////////
template <AppendSink Sink, typename... Args>
void printLn(Sink& sink, typename NonDeduced<const FmtString<Args...>>::type fmtStr, const Args&... args)
{
    if (fmtTo(sink, fmtStr, args...) == FmtResult::Ok)
        sink.append("\n", 1u);
}

} // namespace za
