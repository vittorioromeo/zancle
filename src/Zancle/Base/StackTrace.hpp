#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md

namespace za::priv
{
////////////////////////////////////////////////////////////
/// \brief Print a stack trace to stderr
///
/// Only has effect if `ZA_ENABLE_STACK_TRACES` is defined
///
////////////////////////////////////////////////////////////
[[gnu::cold, gnu::noinline]] void printStackTrace();

} // namespace za::priv
