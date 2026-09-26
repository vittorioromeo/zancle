#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/GLUtils/Export.hpp"

#include "Zancle/GLUtils/GLFenceSync.hpp"


namespace za::priv
{
////////////////////////////////////////////////////////////
/// \brief Create a new GPU fence sync object in the current GL context
///
/// Inserts a `GL_SYNC_GPU_COMMANDS_COMPLETE` fence into the GL command
/// stream. The returned handle becomes signaled once every command issued
/// before the fence has finished executing on the GPU.
///
/// Requires a current GL context with `GL_ARB_sync` (core in GL 3.2+ / ES 3.0+).
///
/// The caller owns the returned handle and must eventually release it via
/// `deleteFenceIfNeeded`, `tryWaitOnFence` (on success), or `waitOnFence`
/// (on success). Leaking a fence leaks a driver-side sync object.
///
/// \return A non-null owning fence object. Aborts on failure.
///
////////////////////////////////////////////////////////////
[[nodiscard]] ZA_GLUTILS_API GLFenceSync makeFence(); // NOLINT(readability-redundant-declaration)

////////////////////////////////////////////////////////////
/// \brief Destroy a fence and null out the caller's handle
///
/// If `fence` is null, this is a no-op. Otherwise the underlying sync object
/// is released via `glDeleteSync` and `fence` is set to `nullptr`, so the
/// same handle can be safely passed again.
///
/// Safe to call regardless of whether the fence has been signaled.
///
/// \param fence Handle to destroy; nulled on return.
///
////////////////////////////////////////////////////////////
ZA_GLUTILS_API void deleteFenceIfNeeded(GLFenceSync& fence) noexcept;

////////////////////////////////////////////////////////////
/// \brief Non-blocking poll of a GPU fence
///
/// Performs a zero-timeout `glClientWaitSync`. The first wait on a given
/// fence uses `GL_SYNC_FLUSH_COMMANDS_BIT` so queued commands can reach the
/// server; later polls skip the flush to avoid repeatedly draining the
/// command queue from hot paths.
///
/// Behavior:
/// - `fenceToWaitOn == nullptr`: returns `true` (nothing to wait on is
///   semantically equivalent to "already done").
/// - Fence already signaled or signaled during the call: deletes the fence,
///   nulls `fenceToWaitOn`, returns `true`.
/// - Fence not yet signaled: leaves `fenceToWaitOn` untouched, returns `false`.
///
/// Aborts on `GL_WAIT_FAILED` (driver-level error).
///
/// Use this to reclaim resources as soon as the GPU is done with them,
/// without ever stalling the CPU.
///
/// \param fenceToWaitOn Fence handle; consumed (deleted and nulled) on success.
///
/// \return `true` if the fence is signaled (or was null), `false` if still pending.
///
////////////////////////////////////////////////////////////
[[nodiscard]] ZA_GLUTILS_API bool tryWaitOnFence(GLFenceSync& fenceToWaitOn);

////////////////////////////////////////////////////////////
/// \brief Blocking wait on a GPU fence
///
/// Performs a `glClientWaitSync` with an effectively-unbounded timeout.
/// The first wait on a given fence uses `GL_SYNC_FLUSH_COMMANDS_BIT`;
/// later waits skip it because the client-side flush has already happened.
/// Intended for frame pacing or forced serialization when a range of GPU
/// memory must be reclaimed before the CPU can overwrite it.
///
/// If `fenceToWaitOn` is null, this is a no-op. Otherwise, once the fence
/// is signaled, it is deleted and `fenceToWaitOn` is nulled.
///
/// Aborts on `GL_WAIT_FAILED` or on `GL_TIMEOUT_EXPIRED`: a driver returning
/// `GL_TIMEOUT_EXPIRED` here indicates a multi-year stall and is treated as
/// a fatal error.
///
/// \param fenceToWaitOn Fence handle; consumed (deleted and nulled) on return
///                      if it was non-null on entry.
///
////////////////////////////////////////////////////////////
ZA_GLUTILS_API void waitOnFence(GLFenceSync& fenceToWaitOn);

} // namespace za::priv
