#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Config.hpp" // IWYU pragma: keep

#include "Zancle/GLUtils/FenceUtils.hpp"
#include "Zancle/GLUtils/GLFenceSync.hpp"
#include "Zancle/GLUtils/GLPersistentBuffer.hpp"

#include "Zancle/Container/SmallVector.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/SizeT.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Persistently-mapped buffer with per-commit fence synchronization
///
/// Wraps `za::GLPersistentBuffer` with a bump-allocator that tracks
/// *submission epochs* as GL fence syncs, so the CPU can safely reuse
/// regions of a persistently-mapped buffer without racing with in-flight
/// GPU reads.
///
/// # Problem
///
/// A persistent mapping (`GL_MAP_PERSISTENT_BIT` + `GL_MAP_UNSYNCHRONIZED_BIT`)
/// exposes a raw CPU pointer into GPU-visible memory. Writes propagate via
/// explicit `glFlushMappedNamedBufferRange`, but the driver does **not**
/// stall on in-flight GPU work -- the caller is responsible for never
/// overwriting a region a pending draw is still reading.
///
/// # Model
///
/// The class tracks a single *linear write cursor* that bump-allocates
/// forward, plus a FIFO of *commit markers*. Each marker records the
/// cursor position at a `commit()` call, paired with a `glFenceSync` that
/// signals once the GPU has finished executing every command issued
/// before the commit:
///
/// ```
///   buffer:  [ ....in flight....|..uncommitted..|...free...]
///             0                  ^              ^           cap
///                                lastCommit    writeCursor
///
///   markers: [ {offset=120, F1}, {offset=180, F2} ]
/// ```
///
/// States:
///   - `[0, lastCommitCursor)`               : in flight (fenced by markers, GPU may still be reading)
///   - `[lastCommitCursor, writeCursor)`     : staged writes, not yet committed
///   - `[writeCursor, capacity)`             : free tail
///
/// When `reclaim()` finds signaled markers at the front of the FIFO, it
/// removes them. The cursors are never reset spontaneously: only the
/// explicit `drain()` / `drainIfWouldOverflow()` entry points rewind
/// them to `0`, so owners that track offsets into the buffer never
/// observe the cursor moving behind their backs.
///
/// # Usage modes
///
///   - **Ring-buffer streaming** (`za::VBOHandle`): call
///     `drainIfWouldOverflow` + `beginWrite` for each upload, `commit()`
///     once after issuing the draw. A single marker covers all uploads
///     in that cycle. `rollback()` discards any uncommitted writes if
///     the draw path unwinds before submission.
///
///   - **Batch filling with offset-from-0 guarantee**
///     (`PersistentGPUDrawableBatch`): call `drain()` at batch reset to
///     stall until the previous batch's fence signals, resetting both
///     cursors to `0`. Subsequent `beginWrite` calls land at the monotonic
///     offsets the batch expects (`0`, `sizeof(Vertex)*nVertices`, ...).
///
/// # Lifecycle
///
/// ```
///   beginWrite ──►  [lastCommit .. writeCursor)  (uncommitted)
///                          │
///                          ├── commit ──► marker inserted
///                          │                 │
///                          │                 │  fence signals
///                          │                 ▼
///                          │              marker erased (reclaim)
///                          │
///                          └── rollback ──► writeCursor rewinds to lastCommit
/// ```
///
/// # Notes
///
/// - Persistent mapping requires desktop OpenGL 4.4+; methods that touch
///   the underlying `GLPersistentBuffer` abort on OpenGL ES.
/// - The allocator is *linear*: the cursor only returns to `0` via the
///   explicit `drain()` / `drainIfWouldOverflow()` entry points. If a
///   commit cycle exceeds capacity, the underlying buffer is grown. This
///   keeps caller-visible offsets monotonic, which matters for batches
///   that reserve storage incrementally.
/// - `commit()` must be called *after* the draw it fences has been
///   submitted to the GL driver, so the fence covers the draw.
///
////////////////////////////////////////////////////////////
template <typename TBufferObject>
class [[nodiscard]] GLPersistentRingBuffer
{
private:
    ////////////////////////////////////////////////////////////
    struct Marker // NOLINT(cppcoreguidelines-pro-type-member-init)
    {
        za::SizeT         offset; //!< writeCursor position captured at commit time
        priv::GLFenceSync fence;  //!< fence that signals when the GPU has processed up to `offset`
    };


    ////////////////////////////////////////////////////////////
    GLPersistentBuffer<TBufferObject> m_persistentBuffer;
    za::SizeT                         m_writeCursor{0u};      //!< next bump-allocation position
    za::SizeT                         m_lastCommitCursor{0u}; //!< writeCursor at the last `commit()`


    ////////////////////////////////////////////////////////////
    za::SmallVector<Marker, 8> m_markers;


    ////////////////////////////////////////////////////////////
    /// \brief Block on the oldest marker's fence and erase it
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void waitAndReclaimOldestMarker()
    {
        ZA_ASSERT(!m_markers.empty());

        priv::waitOnFence(m_markers.front().fence);
        m_markers.erase(m_markers.begin());
    }


    ////////////////////////////////////////////////////////////
    /// \brief Grow the underlying persistent buffer to fit the pending write
    ///
    /// Invoked from `handleOverflow` once all in-flight markers
    /// have been reclaimed and the pending write still does not
    /// fit. Computes a target capacity of `max(byteCount, writeCursor
    /// + byteCount)` and defers to `GLPersistentBuffer::reserve`,
    /// which applies its own geometric growth policy on top.
    ///
    /// Any live bytes of the current commit cycle (`[0, writeCursor)`)
    /// are preserved across the remap by a server-side copy inside
    /// `reserve` (which flushes the old mapping first), so they stay
    /// visible to subsequent draws without re-flushing the new mapping.
    ///
    /// Marked `[[gnu::cold, gnu::noinline]]` so it stays out of
    /// `beginWrite`'s hot path.
    ///
    /// \param obj       Buffer object; will be move-assigned a fresh
    ///                  instance by `reserve()`.
    /// \param byteCount Size of the pending allocation that triggered
    ///                  the grow.
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::cold, gnu::noinline]] void growInternalStorage(TBufferObject& obj, const za::SizeT byteCount)
    {
        const auto currentCapacity = m_persistentBuffer.capacity();
        const auto targetCapacity  = currentCapacity == 0u ? byteCount : m_writeCursor + byteCount;

        m_persistentBuffer.reserve(obj, targetCapacity, /* preserveByteCount */ m_writeCursor);
    }


public:
    ////////////////////////////////////////////////////////////
    GLPersistentRingBuffer() = default;

    ////////////////////////////////////////////////////////////
    GLPersistentRingBuffer(const GLPersistentRingBuffer&)            = delete;
    GLPersistentRingBuffer& operator=(const GLPersistentRingBuffer&) = delete;


    ////////////////////////////////////////////////////////////
    /// \brief Move constructor
    ///
    /// Transfers the persistent-buffer metadata, fence markers, and
    /// cursor state to the new ring buffer, leaving `rhs` empty.
    ///
    /// \warning The associated `TBufferObject` is not owned by this type.
    /// The caller must move that buffer object in lockstep with the ring
    /// buffer so the transferred mapping metadata and fences continue to
    /// refer to the correct GL object.
    ///
    /// \warning The move itself does not perform any cleanup of the
    /// underlying GL mapping. Lifetime management remains the
    /// responsibility of the owning type.
    ///
    ////////////////////////////////////////////////////////////
    GLPersistentRingBuffer(GLPersistentRingBuffer&&) noexcept = default;


    ////////////////////////////////////////////////////////////
    /// \brief Move-assign the persistent-buffer metadata, fence markers,
    /// and cursor state
    ///
    /// Leaves `rhs` empty after the transfer.
    ///
    /// \warning The destination must already have been cleaned up by the
    /// owning type via `destroy(obj)` or an equivalent path before this
    /// assignment. `GLPersistentRingBuffer` cannot safely release the old
    /// mapping on its own because it does not own the associated
    /// `TBufferObject`.
    ///
    /// \warning As with the move constructor, the caller must move the
    /// matching `TBufferObject` in lockstep so the transferred metadata
    /// continues to describe the correct GL buffer object.
    ///
    ////////////////////////////////////////////////////////////
    GLPersistentRingBuffer& operator=(GLPersistentRingBuffer&&) noexcept = default;


    ////////////////////////////////////////////////////////////
    /// \brief Release the fence handles and unmap the persistent buffer
    ///
    /// Must be called before the owning object is destroyed. Safe to
    /// call on a default-constructed instance (no-op) or twice in a row.
    ///
    ////////////////////////////////////////////////////////////
    void destroy(TBufferObject& obj)
    {
        m_markers.clear();

        m_writeCursor      = 0u;
        m_lastCommitCursor = 0u;

        m_persistentBuffer.unmapIfNeeded(obj);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] void* data()
    {
        return m_persistentBuffer.data();
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] const void* data() const
    {
        return m_persistentBuffer.data();
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] za::SizeT capacity() const
    {
        return m_persistentBuffer.capacity();
    }


    ////////////////////////////////////////////////////////////
    /// \brief Flush a mapped byte range so the GPU can see the writes
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void flushBytesToGPU(const TBufferObject& obj, const za::SizeT byteOffset, const za::SizeT byteCount) const
    {
        m_persistentBuffer.flushBytesToGPU(obj, byteOffset, byteCount);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Ensure the underlying storage can hold `byteCount` bytes without advancing the write cursor
    ///
    /// Used by callers that want to preallocate capacity ahead of time
    /// without staging any new writes yet. If a grow remaps the buffer,
    /// any currently live bytes (`[0, writeCursor)`) are preserved via a
    /// server-side copy inside `reserve` and stay visible to the GPU.
    ///
    ////////////////////////////////////////////////////////////
    void reserveCapacity(TBufferObject& obj, const za::SizeT byteCount)
    {
        reclaim();

        if (m_persistentBuffer.capacity() >= byteCount)
            return;

        m_persistentBuffer.reserve(obj, byteCount, /* preserveByteCount */ m_writeCursor);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Bump-allocate `byteCount` bytes and return the write offset
    ///
    /// If the write does not fit at the current cursor, waits for the
    /// oldest marker's fence and retries. If no markers remain and the
    /// write still does not fit, the underlying buffer is grown.
    ///
    /// The returned offset is valid as a direct `data() + offset` write
    /// target. The caller is responsible for calling `flushBytesToGPU`
    /// and `commit()` after the writes are done.
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] za::SizeT beginWrite(TBufferObject& obj, const za::SizeT byteCount)
    {
        if (m_writeCursor + byteCount > m_persistentBuffer.capacity()) [[unlikely]]
            handleOverflow(obj, byteCount);

        const auto result = m_writeCursor;
        m_writeCursor += byteCount;
        return result;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Slow path for `beginWrite` when the request exceeds capacity
    ///
    /// Invoked from `beginWrite` when `m_writeCursor + byteCount`
    /// exceeds `capacity()`. Applies three strategies in sequence,
    /// stopping as soon as the pending write fits:
    ///
    /// 1. `reclaim()` -- non-blocking poll that frees any signaled
    ///    markers, keeping the marker FIFO small.
    /// 2. Block on the oldest pending marker's fence and remove it,
    ///    repeating until no markers remain.
    /// 3. Grow the underlying persistent buffer, preserving the live
    ///    `[0, writeCursor)` bytes.
    ///
    /// Cursors are never reset here: owners that track offsets into the
    /// buffer (`PersistentGPUDrawableBatch`) rely on the cursor only
    /// rewinding via an explicit `drain()`. Streaming owners
    /// (`za::VBOHandle`) get front-of-buffer reuse by calling
    /// `drainIfWouldOverflow()` before `beginWrite`.
    ///
    /// Split out from `beginWrite` and marked `[[gnu::cold, gnu::noinline]]`
    /// so the common-case hot path (write fits without any marker
    /// traversal) stays a tight `compare + bump`. Keeping the cold
    /// code out-of-line shrinks the hot path's instruction footprint
    /// and improves inlining at the call sites.
    ///
    /// \param obj       Buffer object used by the grow path; unused
    ///                  if the first two strategies succeed.
    /// \param byteCount Size of the pending allocation the caller is
    ///                  about to bump-allocate.
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::cold, gnu::noinline]] void handleOverflow(TBufferObject& obj, const za::SizeT byteCount)
    {
        reclaim(); // free signaled markers

        while (m_writeCursor + byteCount > m_persistentBuffer.capacity() && !m_markers.empty())
            waitAndReclaimOldestMarker();

        if (m_writeCursor + byteCount > m_persistentBuffer.capacity())
            growInternalStorage(obj, byteCount);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Insert a fence covering every write since the last commit
    ///
    /// Must be called *after* the draw call that consumes the writes has
    /// been submitted, so the fence guards the draw. A no-op if nothing
    /// has been written since the previous commit.
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void commit()
    {
        if (m_writeCursor == m_lastCommitCursor)
            return;

        m_markers.emplaceBack(m_writeCursor, priv::makeFence());
        m_lastCommitCursor = m_writeCursor;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Rewind the write cursor to the last committed position
    ///
    /// Discards any uncommitted writes. Used by `InstanceAttributeBinder`
    /// when the draw path unwinds before `commit()`.
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void rollback() noexcept
    {
        m_writeCursor = m_lastCommitCursor;
    }


    ////////////////////////////////////////////////////////////
    /// \brief Opportunistically reclaim markers whose fences have signaled
    ///
    /// Polls the front of the FIFO with `priv::tryWaitOnFence` (non-blocking).
    /// Never resets the cursors: owners tracking offsets derived from the
    /// cursor (`PersistentGPUDrawableBatch`) must not observe spontaneous
    /// resets; only `drain()` / `drainIfWouldOverflow()` rewind to `0`.
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void reclaim()
    {
        while (!m_markers.empty() && priv::tryWaitOnFence(m_markers.front().fence))
            m_markers.erase(m_markers.begin());
    }


    ////////////////////////////////////////////////////////////
    /// \brief Restart from offset `0` if the pending write would overflow (streaming mode)
    ///
    /// If `byteCount` more bytes do not fit at the current write cursor
    /// and nothing has been staged since the last commit, blocks until
    /// every in-flight marker signals and resets the cursors to `0`
    /// (via `drain()`), so the following `beginWrite` reuses the buffer
    /// from the front instead of growing it.
    ///
    /// A no-op when the write already fits or when uncommitted writes
    /// are staged (restarting would discard them). Owners that require
    /// monotonic offsets (`PersistentGPUDrawableBatch`) must NOT call
    /// this; it exists for the streaming mode (`za::VBOHandle`), where
    /// no offsets outlive a commit cycle.
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void drainIfWouldOverflow(const za::SizeT byteCount)
    {
        if (m_writeCursor + byteCount <= m_persistentBuffer.capacity()) [[likely]]
            return;

        if (m_writeCursor != m_lastCommitCursor)
            return; // Uncommitted writes staged; restarting would discard them

        drain();
    }


    ////////////////////////////////////////////////////////////
    /// \brief Block until every in-flight marker has signaled, then reset
    ///
    /// On return, the marker FIFO is empty and both cursors are at `0`,
    /// regardless of whether uncommitted writes existed beforehand. Any
    /// uncommitted writes are intentionally discarded -- callers that
    /// want to keep them must `commit()` first.
    ///
    /// Use this when the caller requires the next `beginWrite` to start
    /// at offset `0` (e.g. `PersistentGPUDrawableBatch` after `clear()`,
    /// or the auto-batch path at frame start, where the enclosing
    /// framework already synchronized the GPU and the ring's per-commit
    /// fences were never created).
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline]] void drain()
    {
        while (!m_markers.empty())
            waitAndReclaimOldestMarker();

        // Uncommitted writes: no marker covers them, but the caller is
        // explicitly asking for a fresh start. Force the reset.
        m_writeCursor      = 0u;
        m_lastCommitCursor = 0u;
    }
};

} // namespace za
