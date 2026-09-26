#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Base/NoUniqueAddress.hpp" // IWYU pragma: export (used by `ZA_REQUIRE_DESIGNATED_INITIALIZERS`)


namespace za::priv
{
////////////////////////////////////////////////////////////
/// \brief Sentinel type used by `ZA_REQUIRE_DESIGNATED_INITIALIZERS`
///
/// Its `explicit` default constructor blocks copy-list-initialization
/// from `{}`, which is what aggregate initialization performs for each
/// brace-element. This makes positional aggregate initialization
/// ill-formed while leaving designated initialization (and the
/// in-class default member initializer) unaffected.
///
////////////////////////////////////////////////////////////
struct ReqDInit
{
    explicit ReqDInit() = default;

    [[nodiscard, gnu::always_inline, gnu::pure]]
    constexpr bool operator==(const ReqDInit&) const noexcept = default;
};

} // namespace za::priv


////////////////////////////////////////////////////////////
/// \brief Force callers of an aggregate to use designated initializers
///
/// Place at the very top of an aggregate's member list. Adds a
/// zero-sized sentinel (via `ZA_NO_UNIQUE_ADDRESS`) that can only
/// be initialized by its in-class default -- the explicit default
/// constructor rejects the copy-list-initialization that aggregate
/// init performs for each positional brace element.
///
/// Result:
/// - `MyData{.foo = 1, .bar = 2}` compiles
/// - `MyData{}` compiles
/// - `MyData{1, 2}` does not compile
/// - `MyData{{}, 1, 2}` does not compile
///
/// \see `za::priv::ReqDInit`
///
////////////////////////////////////////////////////////////
#define ZA_REQUIRE_DESIGNATED_INITIALIZERS                \
    ZA_NO_UNIQUE_ADDRESS ::za::priv::ReqDInit _zaReqDInit \
    {                                                     \
    }
