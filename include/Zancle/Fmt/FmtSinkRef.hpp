#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Fmt/FmtResult.hpp"

#include "Zancle/Base/SizeT.hpp"

#include "Zancle/Trait/IsSame.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Type-erased, non-owning reference to an append-style sink.
///
/// Holds a `void*` plus a function pointer that knows how to call
/// `Sink::append(const char*, SizeT)` for the original type. Implicitly
/// constructible from any object exposing that method, so callers can
/// pass `String&`, `StdoutSink`, an ofstream-wrapping adapter, etc.
/// without templating the receiving function on the sink type.
///
/// If `Sink::append` returns `FmtResult`, that result is propagated.
/// Other return types are treated as infallible append operations.
///
/// Trivially copyable (two pointers). The reference is non-owning --
/// callers must keep the underlying sink alive while a `FmtSinkRef`
/// holding it is in use.
///
/// Designed for third-party serializers (e.g. JSON writers) that want
/// to stream output into any append-style sink without paying the
/// virtual-call cost of a polymorphic base class or the
/// header-bloat cost of a templated implementation.
////////////////////////////////////////////////////////////
class FmtSinkRef
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Default-constructs a no-op sink. Useful for default-initializing
    /// `FmtSinkRef` members that get overwritten before use; appending bytes
    /// to a default-constructed `FmtSinkRef` silently discards them.
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] constexpr FmtSinkRef() noexcept :
        m_sink{nullptr},
        m_appendFn{+[](void*, const char*, SizeT) { return FmtResult::Ok; }}
    {
    }


    ////////////////////////////////////////////////////////////
    /// \note The `Sink::append(const char*, SizeT)` requirement is checked
    /// lazily at lambda instantiation rather than via a `requires` clause:
    /// the constructor is often called from templates where `Sink` is a
    /// concrete type whose definition is not yet visible (e.g. `String`
    /// forward-declared in `Fmt.hpp`), and a `requires` clause would be
    /// evaluated eagerly and fail.
    ////////////////////////////////////////////////////////////
    template <typename Sink>
    [[gnu::always_inline]] FmtSinkRef(Sink& sink) noexcept :
        m_sink{&sink},
        m_appendFn{+[](void* s, const char* data, SizeT n)
    {
        if constexpr (ZA_IS_SAME(decltype(static_cast<Sink*>(s)->append(data, n)), FmtResult))
            return static_cast<Sink*>(s)->append(data, n);
        else
        {
            static_cast<Sink*>(s)->append(data, n);
            return FmtResult::Ok;
        }
    }}
    {
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] FmtResult append(const char* const data, const SizeT n) const
    {
        return m_appendFn(m_sink, data, n);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] void* sink() const noexcept
    {
        return m_sink;
    }


    ////////////////////////////////////////////////////////////
    using AppendFn = FmtResult (*)(void*, const char*, SizeT);


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] AppendFn appendFn() const noexcept
    {
        return m_appendFn;
    }

private:
    void*    m_sink;
    AppendFn m_appendFn;
};

} // namespace za


////////////////////////////////////////////////////////////
/// \file
/// `FmtSinkRef` is the public face of the existing internal
/// "void* + appendFn" pattern used inside `fmtToHeapFallback`. Promoting
/// it to a first-class value type lets external code (jsoncpp's
/// `StyledWriter`, custom serializers, etc.) accept any append-style
/// sink without depending on `<Zancle/Fmt/Fmt.hpp>`.
////////////////////////////////////////////////////////////
