// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Window/Event.hpp"


namespace
{
////////////////////////////////////////////////////////////
template <typename>
constexpr bool isEventSubtype = false;

#define ZA_PRIV_EVENT_X_IS_EVENT_SUBTYPE(x) \
    template <>                             \
    constexpr bool isEventSubtype<x> = true

ZA_PRIV_EVENTS_X_MACRO(ZA_PRIV_EVENT_X_IS_EVENT_SUBTYPE, ZA_PRIV_EVENT_X_SEMICOLON);

} // namespace

// Repeating isEventSubtype<T> allows for cleaner compiler errors.
// It is not strictly necessary but it's useful nonetheless.
// It works by ensuring that the code within the conditional is
// only compiled when the condition is met. Otherwise you get
// a static_assert failure in addition to the compiler failing
// to compile the code within the compiletime conditional when
// an incorrect template parameter is provided.

namespace za
{
////////////////////////////////////////////////////////////
template <typename TEventSubtype>
Event::Event(const TEventSubtype& eventSubtype)
{
    static_assert(isEventSubtype<TEventSubtype>, "`TEventSubtype` must be a subtype of `za::Event`");
    if constexpr (isEventSubtype<TEventSubtype>)
        m_data = eventSubtype;
}


////////////////////////////////////////////////////////////
template <typename TEventSubtype>
bool Event::is() const
{
    static_assert(isEventSubtype<TEventSubtype>, "`TEventSubtype` must be a subtype of `za::Event`");
    if constexpr (isEventSubtype<TEventSubtype>)
        return m_data.is<TEventSubtype>();
}


////////////////////////////////////////////////////////////
template <typename TEventSubtype>
TEventSubtype* Event::getIf()
{
    static_assert(isEventSubtype<TEventSubtype>, "`TEventSubtype` must be a subtype of `za::Event`");
    if constexpr (isEventSubtype<TEventSubtype>)
        return m_data.getIf<TEventSubtype>();
}


////////////////////////////////////////////////////////////
template <typename TEventSubtype>
const TEventSubtype* Event::getIf() const
{
    static_assert(isEventSubtype<TEventSubtype>, "`TEventSubtype` must be a subtype of `za::Event`");
    if constexpr (isEventSubtype<TEventSubtype>)
        return m_data.getIf<TEventSubtype>();
}

} // namespace za


////////////////////////////////////////////////////////////
// Explicit instantiation definitions
////////////////////////////////////////////////////////////

template class ZA_PRIV_EVENT_VARIANT_TYPE;

#define ZA_PRIV_EVENT_X_TEMPLATE_CTOR(x)        template ZA_WINDOW_API za::Event::Event(const x&);
#define ZA_PRIV_EVENT_X_TEMPLATE_IS(x)          template ZA_WINDOW_API bool za::Event::is<x>() const;
#define ZA_PRIV_EVENT_X_TEMPLATE_GETIF(x)       template ZA_WINDOW_API x* za::Event::getIf<x>();
#define ZA_PRIV_EVENT_X_TEMPLATE_GETIF_CONST(x) template ZA_WINDOW_API const x* za::Event::getIf<x>() const;

ZA_PRIV_EVENTS_X_MACRO(ZA_PRIV_EVENT_X_TEMPLATE_CTOR, ZA_PRIV_EVENT_X_SEMICOLON);
ZA_PRIV_EVENTS_X_MACRO(ZA_PRIV_EVENT_X_TEMPLATE_IS, ZA_PRIV_EVENT_X_SEMICOLON);
ZA_PRIV_EVENTS_X_MACRO(ZA_PRIV_EVENT_X_TEMPLATE_GETIF, ZA_PRIV_EVENT_X_SEMICOLON);
ZA_PRIV_EVENTS_X_MACRO(ZA_PRIV_EVENT_X_TEMPLATE_GETIF_CONST, ZA_PRIV_EVENT_X_SEMICOLON);
