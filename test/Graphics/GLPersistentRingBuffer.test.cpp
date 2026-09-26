#include "Tst/Tst.hpp"
#include "WindowUtil.hpp"

#include "Zancle/GLUtils/GLPersistentRingBuffer.hpp"

#include "Zancle/GLUtils/GLBufferObject.hpp"
#include "Zancle/GLUtils/GLCheck.hpp"
#include "Zancle/GLUtils/GLUniqueResource.hpp"
#include "Zancle/GLUtils/Glad.hpp"

#include "Zancle/Graphics/GraphicsContext.hpp"

#include "Zancle/Window/WindowContext.hpp"

#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"

#include "Zancle/Base/Macros.hpp"
#include "Zancle/Base/Memcmp.hpp"
#include "Zancle/Base/Memcpy.hpp"
#include "Zancle/Base/SizeT.hpp"


#ifndef ZA_OPENGL_ES

namespace
{
using VBO = za::GLVertexBufferObject;
using EBO = za::GLElementBufferObject;

using VRingBuffer = za::GLPersistentRingBuffer<VBO>;
using ERingBuffer = za::GLPersistentRingBuffer<EBO>;


////////////////////////////////////////////////////////////
template <typename TBufferObject>
void ringReadbackBufferBytes(const TBufferObject& obj, const za::SizeT offset, const za::SizeT size, unsigned char* const out)
{
    glCheck(glGetNamedBufferSubData(obj.getId(), static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), out));
}


////////////////////////////////////////////////////////////
void ringDrainGLCommandQueue()
{
    glCheck(glFinish());
}


////////////////////////////////////////////////////////////
template <typename TBufferObject>
struct ScopedRingBuffer
{
    TBufferObject                             obj;
    za::GLPersistentRingBuffer<TBufferObject> buffer;

    ScopedRingBuffer() : obj{za::tryCreateGLUniqueResource<TBufferObject>().value()}
    {
    }

    ~ScopedRingBuffer()
    {
        buffer.destroy(obj);
    }

    ScopedRingBuffer(const ScopedRingBuffer&)            = delete;
    ScopedRingBuffer& operator=(const ScopedRingBuffer&) = delete;

    ScopedRingBuffer(ScopedRingBuffer&&) noexcept = default;

    ScopedRingBuffer& operator=(ScopedRingBuffer&& rhs) noexcept
    {
        if (this == &rhs)
            return *this;

        buffer.destroy(obj);
        obj    = ZA_MOVE(rhs.obj);
        buffer = ZA_MOVE(rhs.buffer);

        return *this;
    }
};


////////////////////////////////////////////////////////////
void fillPattern(unsigned char* const dst, const za::SizeT size, const unsigned char salt)
{
    for (za::SizeT i = 0u; i < size; ++i)
        dst[i] = static_cast<unsigned char>((i * 11u) ^ (salt + 0x17u));
}

} // namespace


TEST_CASE("[GLUtils] za::GLPersistentRingBuffer" * tst::skip(skipDisplayTests))
{
    [[maybe_unused]] auto& graphicsContext = TST_CASE_SHARED(za::GraphicsContext::create().value());

    SECTION("Type traits")
    {
        STATIC_CHECK(!za::isCopyConstructible<VRingBuffer>);
        STATIC_CHECK(!za::isCopyAssignable<VRingBuffer>);
        STATIC_CHECK(za::isNoThrowMoveConstructible<VRingBuffer>);
        STATIC_CHECK(za::isNoThrowMoveAssignable<VRingBuffer>);

        STATIC_CHECK(!za::isCopyConstructible<ERingBuffer>);
        STATIC_CHECK(!za::isCopyAssignable<ERingBuffer>);
        STATIC_CHECK(za::isNoThrowMoveConstructible<ERingBuffer>);
        STATIC_CHECK(za::isNoThrowMoveAssignable<ERingBuffer>);
    }

    SECTION("Default-constructed state has no mapping and zero capacity")
    {
        ScopedRingBuffer<VBO> sb;
        CHECK(sb.buffer.data() == nullptr);
        CHECK(sb.buffer.capacity() == 0u);

        const auto& cb = sb.buffer;
        CHECK(cb.data() == nullptr);
    }

    SECTION("destroy on a default-constructed buffer is a safe no-op")
    {
        ScopedRingBuffer<VBO> sb;

        sb.buffer.destroy(sb.obj);
        CHECK(sb.buffer.data() == nullptr);
        CHECK(sb.buffer.capacity() == 0u);

        // Idempotent
        sb.buffer.destroy(sb.obj);
    }

    SECTION("commit, rollback, reclaim, drain are safe no-ops with no state")
    {
        ScopedRingBuffer<VBO> sb;

        sb.buffer.commit();
        sb.buffer.rollback();
        sb.buffer.reclaim();
        sb.buffer.drain();

        CHECK(sb.buffer.data() == nullptr);
        CHECK(sb.buffer.capacity() == 0u);
    }

    SECTION("drain resets the cursor even when writes were never committed")
    {
        // Covers the GPU auto-batch path: the RenderTarget manages its own
        // per-slot fence externally, so the ring never sees a `commit()` --
        // `drain()` at the start of the next batch must still force the
        // cursor back to 0 so the batch's `offset == nVertices * sizeof(Vertex)`
        // invariant holds.
        ScopedRingBuffer<VBO> sb;

        (void)sb.buffer.beginWrite(sb.obj, 128u);
        (void)sb.buffer.beginWrite(sb.obj, 64u);
        // NOTE: no commit() here -- uncommitted writes are live in the ring.

        sb.buffer.drain();

        const auto offsetAfter = sb.buffer.beginWrite(sb.obj, 32u);
        CHECK(offsetAfter == 0u);
    }

    SECTION("drain resets the cursor after mixed committed + uncommitted writes")
    {
        ScopedRingBuffer<VBO> sb;

        (void)sb.buffer.beginWrite(sb.obj, 128u);
        sb.buffer.commit();

        // Writes after the commit: these are uncommitted.
        (void)sb.buffer.beginWrite(sb.obj, 64u);
        (void)sb.buffer.beginWrite(sb.obj, 32u);

        ringDrainGLCommandQueue();
        sb.buffer.drain();

        const auto offsetAfter = sb.buffer.beginWrite(sb.obj, 16u);
        CHECK(offsetAfter == 0u);
    }

    SECTION("First beginWrite allocates storage and returns offset 0")
    {
        ScopedRingBuffer<VBO> sb;

        const auto offset = sb.buffer.beginWrite(sb.obj, 256u);
        CHECK(offset == 0u);
        CHECK(sb.buffer.data() != nullptr);
        CHECK(sb.buffer.capacity() >= 256u);
    }

    SECTION("Successive beginWrite calls append sequentially within one commit cycle")
    {
        ScopedRingBuffer<VBO> sb;

        const auto offset0 = sb.buffer.beginWrite(sb.obj, 100u);
        CHECK(offset0 == 0u);

        const auto offset1 = sb.buffer.beginWrite(sb.obj, 50u);
        CHECK(offset1 == 100u);

        const auto offset2 = sb.buffer.beginWrite(sb.obj, 200u);
        CHECK(offset2 == 150u);

        CHECK(sb.buffer.capacity() >= 350u);
    }

    SECTION("beginWrite grows the buffer when a cycle exceeds capacity")
    {
        ScopedRingBuffer<VBO> sb;

        const auto offsetA = sb.buffer.beginWrite(sb.obj, 64u);
        CHECK(offsetA == 0u);
        CHECK(sb.buffer.capacity() >= 64u);

        // A large follow-up write within the same commit cycle must trigger
        // a growth that preserves the earlier bytes.
        const auto offsetB = sb.buffer.beginWrite(sb.obj, 2048u);
        CHECK(offsetB == 64u);
        CHECK(sb.buffer.capacity() >= 64u + 2048u);
    }

    SECTION("rollback without any commit rewinds the cursor to 0")
    {
        ScopedRingBuffer<VBO> sb;

        (void)sb.buffer.beginWrite(sb.obj, 100u);
        (void)sb.buffer.beginWrite(sb.obj, 50u);

        sb.buffer.rollback();

        // Next beginWrite should return 0 again.
        const auto offsetAfter = sb.buffer.beginWrite(sb.obj, 30u);
        CHECK(offsetAfter == 0u);
    }

    SECTION("Rollback never crosses the last commit boundary")
    {
        ScopedRingBuffer<VBO> sb;

        // Fill and commit a first batch. In a test environment the fence
        // signals instantly, so the next beginWrite's reclaim pass will
        // reset the cursors to 0; the point of this test is that rollback
        // must never undo the commit, regardless of whether reclaim has
        // run yet.
        (void)sb.buffer.beginWrite(sb.obj, 128u);
        sb.buffer.commit();

        const auto lastCommitSnapshot = sb.buffer.beginWrite(sb.obj, 64u);
        // Uncommitted: scribble some CPU writes.
        ZA_MEMCPY(static_cast<unsigned char*>(sb.buffer.data()) + lastCommitSnapshot, "ABCDEFGHIJKLMNOP", 16u);

        sb.buffer.rollback();

        // After rollback, the next beginWrite must be at a position <= the
        // write cursor before rollback -- i.e. it must NOT skip past where
        // we just rolled back to.
        const auto offsetAfter = sb.buffer.beginWrite(sb.obj, 32u);
        CHECK(offsetAfter == lastCommitSnapshot);
    }

    SECTION("commit + drain lets the next beginWrite reuse the region from zero")
    {
        ScopedRingBuffer<VBO> sb;

        const auto offset0 = sb.buffer.beginWrite(sb.obj, 100u);
        CHECK(offset0 == 0u);
        sb.buffer.commit();

        ringDrainGLCommandQueue();
        sb.buffer.drain();

        const auto offsetAfter = sb.buffer.beginWrite(sb.obj, 50u);
        CHECK(offsetAfter == 0u);
    }

    SECTION("commit + reclaim (opportunistic) lets the next beginWrite reuse the region from zero")
    {
        ScopedRingBuffer<VBO> sb;

        const auto offset0 = sb.buffer.beginWrite(sb.obj, 100u);
        CHECK(offset0 == 0u);
        sb.buffer.commit();

        ringDrainGLCommandQueue();

        // reclaim() is non-blocking but with a drained pipeline the fence has signaled.
        sb.buffer.reclaim();

        const auto offsetAfter = sb.buffer.beginWrite(sb.obj, 50u);
        CHECK(offsetAfter == 0u);
    }

    SECTION("Multiple drain-commit cycles reuse the buffer from zero")
    {
        ScopedRingBuffer<VBO> sb;

        for (int i = 0; i < 3; ++i)
        {
            sb.buffer.drain(); // no-op on the first iter, blocks on previous fence otherwise
            CHECK(sb.buffer.beginWrite(sb.obj, 100u) == 0u);
            CHECK(sb.buffer.beginWrite(sb.obj, 50u) == 100u);
            sb.buffer.commit();
            ringDrainGLCommandQueue();
        }
    }

    SECTION("CPU writes via beginWrite round-trip through glGetNamedBufferSubData")
    {
        ScopedRingBuffer<VBO> sb;

        const auto offset = sb.buffer.beginWrite(sb.obj, 512u);
        REQUIRE(offset == 0u);
        REQUIRE(sb.buffer.data() != nullptr);

        unsigned char pattern[512];
        fillPattern(pattern, 512u, /* salt */ 0x00u);

        ZA_MEMCPY(static_cast<unsigned char*>(sb.buffer.data()) + offset, pattern, 512u);
        sb.buffer.flushBytesToGPU(sb.obj, offset, 512u);

        unsigned char readback[512]{};
        ringReadbackBufferBytes(sb.obj, offset, 512u, readback);
        CHECK(ZA_MEMCMP(readback, pattern, 512u) == 0);
    }

    SECTION("Multiple uploads within one commit cycle round-trip independently")
    {
        ScopedRingBuffer<VBO> sb;

        unsigned char patternA[128];
        unsigned char patternB[64];
        fillPattern(patternA, 128u, /* salt */ 0x10u);
        fillPattern(patternB, 64u, /* salt */ 0xA0u);

        const auto offsetA = sb.buffer.beginWrite(sb.obj, 128u);
        REQUIRE(offsetA == 0u);
        ZA_MEMCPY(static_cast<unsigned char*>(sb.buffer.data()) + offsetA, patternA, 128u);
        sb.buffer.flushBytesToGPU(sb.obj, offsetA, 128u);

        const auto offsetB = sb.buffer.beginWrite(sb.obj, 64u);
        REQUIRE(offsetB == 128u);
        ZA_MEMCPY(static_cast<unsigned char*>(sb.buffer.data()) + offsetB, patternB, 64u);
        sb.buffer.flushBytesToGPU(sb.obj, offsetB, 64u);

        sb.buffer.commit();

        unsigned char readbackA[128]{};
        unsigned char readbackB[64]{};
        ringReadbackBufferBytes(sb.obj, 0u, 128u, readbackA);
        ringReadbackBufferBytes(sb.obj, 128u, 64u, readbackB);

        CHECK(ZA_MEMCMP(readbackA, patternA, 128u) == 0);
        CHECK(ZA_MEMCMP(readbackB, patternB, 64u) == 0);
    }

    SECTION("Growth during a commit cycle preserves already-written data")
    {
        ScopedRingBuffer<VBO> sb;

        unsigned char patternA[100];
        unsigned char patternB[1024];
        fillPattern(patternA, 100u, /* salt */ 0x33u);
        fillPattern(patternB, 1024u, /* salt */ 0x77u);

        // First write: sizes the buffer to exactly 100.
        const auto offsetA = sb.buffer.beginWrite(sb.obj, 100u);
        REQUIRE(offsetA == 0u);
        ZA_MEMCPY(static_cast<unsigned char*>(sb.buffer.data()) + offsetA, patternA, 100u);
        sb.buffer.flushBytesToGPU(sb.obj, offsetA, 100u);

        // Second write: forces `growForWrite` because capacity is exactly 100
        // and no markers exist to wait on. The grow path must preserve the
        // uncommitted bytes via `GLPersistentBuffer::reserve(preserve=true)`.
        const auto offsetB = sb.buffer.beginWrite(sb.obj, 1024u);
        REQUIRE(offsetB == 100u);
        ZA_MEMCPY(static_cast<unsigned char*>(sb.buffer.data()) + offsetB, patternB, 1024u);

        // Re-flush the preserved range in the new mapping, then flush the new write.
        sb.buffer.flushBytesToGPU(sb.obj, 0u, 100u);
        sb.buffer.flushBytesToGPU(sb.obj, offsetB, 1024u);
        sb.buffer.commit();

        unsigned char readbackA[100]{};
        unsigned char readbackB[1024]{};
        ringReadbackBufferBytes(sb.obj, 0u, 100u, readbackA);
        ringReadbackBufferBytes(sb.obj, 100u, 1024u, readbackB);

        CHECK(ZA_MEMCMP(readbackA, patternA, 100u) == 0);
        CHECK(ZA_MEMCMP(readbackB, patternB, 1024u) == 0);
    }

    SECTION("Growth re-flushes preserved bytes so earlier uploads stay visible")
    {
        ScopedRingBuffer<VBO> sb;

        unsigned char patternA[100];
        unsigned char patternB[1024];
        fillPattern(patternA, 100u, /* salt */ 0x21u);
        fillPattern(patternB, 1024u, /* salt */ 0x84u);

        const auto offsetA = sb.buffer.beginWrite(sb.obj, 100u);
        REQUIRE(offsetA == 0u);
        ZA_MEMCPY(static_cast<unsigned char*>(sb.buffer.data()) + offsetA, patternA, 100u);
        sb.buffer.flushBytesToGPU(sb.obj, offsetA, 100u);

        // This second allocation forces a grow. The ring buffer must re-flush
        // the copied prefix internally, so callers only need to flush the new write.
        const auto offsetB = sb.buffer.beginWrite(sb.obj, 1024u);
        REQUIRE(offsetB == 100u);
        ZA_MEMCPY(static_cast<unsigned char*>(sb.buffer.data()) + offsetB, patternB, 1024u);
        sb.buffer.flushBytesToGPU(sb.obj, offsetB, 1024u);

        unsigned char readbackA[100]{};
        unsigned char readbackB[1024]{};
        ringReadbackBufferBytes(sb.obj, 0u, 100u, readbackA);
        ringReadbackBufferBytes(sb.obj, 100u, 1024u, readbackB);

        CHECK(ZA_MEMCMP(readbackA, patternA, 100u) == 0);
        CHECK(ZA_MEMCMP(readbackB, patternB, 1024u) == 0);
    }

    SECTION("Rollback + fresh commit overwrites cleanly")
    {
        ScopedRingBuffer<VBO> sb;

        unsigned char garbage[256];
        unsigned char fresh[256];
        fillPattern(garbage, 256u, /* salt */ 0x55u);
        fillPattern(fresh, 256u, /* salt */ 0xEEu);

        // Start a cycle, write garbage, roll it back before commit.
        const auto offset0 = sb.buffer.beginWrite(sb.obj, 256u);
        ZA_MEMCPY(sb.buffer.data(), garbage, 256u);
        sb.buffer.flushBytesToGPU(sb.obj, offset0, 256u);

        sb.buffer.rollback();

        // Next cycle reuses [0, 256).
        const auto offset1 = sb.buffer.beginWrite(sb.obj, 256u);
        CHECK(offset1 == 0u);
        ZA_MEMCPY(sb.buffer.data(), fresh, 256u);
        sb.buffer.flushBytesToGPU(sb.obj, offset1, 256u);
        sb.buffer.commit();

        unsigned char readback[256]{};
        ringReadbackBufferBytes(sb.obj, 0u, 256u, readback);
        CHECK(ZA_MEMCMP(readback, fresh, 256u) == 0);
    }

    SECTION("DrawableBatch-style append pattern (drain at batch start, monotonic offsets)")
    {
        // Mirrors how `PersistentGPUDrawableBatch` uses the ring:
        // the batch drains at the start of each frame to force offset-from-0,
        // then reserveMoreVertices/Indices bump-allocate monotonically.
        ScopedRingBuffer<VBO> sb;

        constexpr za::SizeT chunkSize = 64u;
        constexpr za::SizeT numChunks = 8u;
        constexpr za::SizeT total     = chunkSize * numChunks;

        unsigned char expected[total];
        for (za::SizeT i = 0u; i < numChunks; ++i)
            fillPattern(expected + i * chunkSize, chunkSize, static_cast<unsigned char>(i));

        // Frame 1
        sb.buffer.drain(); // no-op, buffer is idle
        for (za::SizeT i = 0u; i < numChunks; ++i)
        {
            const auto writeOffset = sb.buffer.beginWrite(sb.obj, chunkSize);
            CHECK(writeOffset == i * chunkSize);
            ZA_MEMCPY(static_cast<unsigned char*>(sb.buffer.data()) + writeOffset, expected + writeOffset, chunkSize);
        }
        sb.buffer.flushBytesToGPU(sb.obj, 0u, total);
        sb.buffer.commit();

        unsigned char readback[total]{};
        ringReadbackBufferBytes(sb.obj, 0u, total, readback);
        CHECK(ZA_MEMCMP(readback, expected, total) == 0);

        // Frame 2: drain (simulating clear), fill again from 0.
        ringDrainGLCommandQueue();
        sb.buffer.drain();

        for (za::SizeT i = 0u; i < numChunks; ++i)
        {
            const auto writeOffset = sb.buffer.beginWrite(sb.obj, chunkSize);
            CHECK(writeOffset == i * chunkSize);
        }
    }

    SECTION("flushBytesToGPU accepts partial and degenerate ranges")
    {
        ScopedRingBuffer<VBO> sb;

        (void)sb.buffer.beginWrite(sb.obj, 256u);
        REQUIRE(sb.buffer.capacity() >= 256u);

        sb.buffer.flushBytesToGPU(sb.obj, 0u, 256u);
        sb.buffer.flushBytesToGPU(sb.obj, 32u, 64u);
        sb.buffer.flushBytesToGPU(sb.obj, 0u, 1u);
        sb.buffer.flushBytesToGPU(sb.obj, 128u, 0u);
    }

    SECTION("destroy after staging and committing cleans up without crashing")
    {
        ScopedRingBuffer<VBO> sb;

        (void)sb.buffer.beginWrite(sb.obj, 128u);
        sb.buffer.commit();

        (void)sb.buffer.beginWrite(sb.obj, 64u);
        sb.buffer.commit();

        // Mix: third write is uncommitted when scope ends.
        (void)sb.buffer.beginWrite(sb.obj, 32u);

        // `ScopedRingBuffer::~ScopedRingBuffer` calls `destroy`, which must
        // delete every fence and unmap cleanly regardless of committed/uncommitted mix.
    }

    SECTION("Move construction transfers mapping and state to the destination")
    {
        ScopedRingBuffer<VBO> source;

        (void)source.buffer.beginWrite(source.obj, 256u);
        source.buffer.commit();

        void* const originalPtr = source.buffer.data();
        const auto  originalCap = source.buffer.capacity();
        REQUIRE(originalPtr != nullptr);
        REQUIRE(originalCap >= 256u);

        ScopedRingBuffer<VBO> dest{ZA_MOVE(source)};

        CHECK(dest.buffer.data() == originalPtr);
        CHECK(dest.buffer.capacity() == originalCap);

        // Destination can drain + reuse normally.
        ringDrainGLCommandQueue();
        dest.buffer.drain();

        const auto offsetAfter = dest.buffer.beginWrite(dest.obj, 128u);
        CHECK(offsetAfter == 0u);
    }

    SECTION("Move assignment into an empty destination transfers state")
    {
        ScopedRingBuffer<VBO> source;

        (void)source.buffer.beginWrite(source.obj, 256u);

        void* const originalPtr = source.buffer.data();
        const auto  originalCap = source.buffer.capacity();
        REQUIRE(originalPtr != nullptr);

        ScopedRingBuffer<VBO> dest;
        dest = ZA_MOVE(source);

        CHECK(dest.buffer.data() == originalPtr);
        CHECK(dest.buffer.capacity() == originalCap);
    }

    SECTION("GLElementBufferObject instantiation supports the full lifecycle")
    {
        ScopedRingBuffer<EBO> sb;

        const auto offset = sb.buffer.beginWrite(sb.obj, 256u);
        REQUIRE(offset == 0u);
        REQUIRE(sb.buffer.data() != nullptr);

        unsigned char pattern[256];
        fillPattern(pattern, 256u, /* salt */ 0xC9u);

        ZA_MEMCPY(sb.buffer.data(), pattern, 256u);
        sb.buffer.flushBytesToGPU(sb.obj, 0u, 256u);
        sb.buffer.commit();

        unsigned char readback[256]{};
        ringReadbackBufferBytes(sb.obj, 0u, 256u, readback);
        CHECK(ZA_MEMCMP(readback, pattern, 256u) == 0);

        ringDrainGLCommandQueue();
        sb.buffer.drain();

        const auto offsetAfter = sb.buffer.beginWrite(sb.obj, 64u);
        CHECK(offsetAfter == 0u);
    }
}

#endif // !ZA_OPENGL_ES
