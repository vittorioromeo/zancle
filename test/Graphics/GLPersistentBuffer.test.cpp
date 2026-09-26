#include "Tst/Tst.hpp"
#include "WindowUtil.hpp"

#include "Zancle/GLUtils/GLPersistentBuffer.hpp"

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
#include "Zancle/Base/Memset.hpp"
#include "Zancle/Base/SizeT.hpp"


#ifndef ZA_OPENGL_ES

namespace
{
using VBO     = za::GLVertexBufferObject;
using EBO     = za::GLElementBufferObject;
using VBuffer = za::GLPersistentBuffer<VBO>;
using EBuffer = za::GLPersistentBuffer<EBO>;


////////////////////////////////////////////////////////////
/// Read bytes out of a buffer object via `glGetNamedBufferSubData`.
/// This works even when the buffer is persistently mapped, which is
/// what lets the tests verify byte-for-byte contents without needing
/// a readable mapping (`GL_MAP_READ_BIT`).
////////////////////////////////////////////////////////////
template <typename TBufferObject>
void readbackBufferBytes(const TBufferObject& obj, const za::SizeT offset, const za::SizeT size, unsigned char* const out)
{
    glCheck(glGetNamedBufferSubData(obj.getId(), static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), out));
}


////////////////////////////////////////////////////////////
/// RAII pair that owns both the underlying buffer object and the
/// `GLPersistentBuffer` that maps into it, so tests never need to
/// manually unmap before destruction.
////////////////////////////////////////////////////////////
template <typename TBufferObject>
struct ScopedPersistentBuffer
{
    TBufferObject                         obj;
    za::GLPersistentBuffer<TBufferObject> buffer;

    ScopedPersistentBuffer() : obj{za::tryCreateGLUniqueResource<TBufferObject>().value()}
    {
    }

    ~ScopedPersistentBuffer()
    {
        buffer.unmapIfNeeded(obj);
    }

    ScopedPersistentBuffer(const ScopedPersistentBuffer&)            = delete;
    ScopedPersistentBuffer& operator=(const ScopedPersistentBuffer&) = delete;

    ScopedPersistentBuffer(ScopedPersistentBuffer&&) noexcept = default;

    ScopedPersistentBuffer& operator=(ScopedPersistentBuffer&& rhs) noexcept
    {
        if (this == &rhs)
            return *this;

        // Caller contract: unmap the current mapping before the underlying
        // buffer object is replaced, otherwise the driver mapping leaks.
        buffer.unmapIfNeeded(obj);

        obj    = ZA_MOVE(rhs.obj);
        buffer = ZA_MOVE(rhs.buffer);

        return *this;
    }
};

} // namespace


TEST_CASE("[GLUtils] za::GLPersistentBuffer" * tst::skip(skipDisplayTests))
{
    [[maybe_unused]] auto& graphicsContext = TST_CASE_SHARED(za::GraphicsContext::create().value());

    SECTION("Type traits")
    {
        STATIC_CHECK(!za::isCopyConstructible<VBuffer>);
        STATIC_CHECK(!za::isCopyAssignable<VBuffer>);
        STATIC_CHECK(za::isNoThrowMoveConstructible<VBuffer>);
        STATIC_CHECK(za::isNoThrowMoveAssignable<VBuffer>);

        STATIC_CHECK(!za::isCopyConstructible<EBuffer>);
        STATIC_CHECK(!za::isCopyAssignable<EBuffer>);
        STATIC_CHECK(za::isNoThrowMoveConstructible<EBuffer>);
        STATIC_CHECK(za::isNoThrowMoveAssignable<EBuffer>);
    }

    SECTION("Default-constructed state has no mapping")
    {
        ScopedPersistentBuffer<VBO> sb;
        CHECK(sb.buffer.data() == nullptr);

        const auto& cb = sb.buffer;
        CHECK(cb.data() == nullptr);
    }

    SECTION("unmapIfNeeded on default-constructed buffer is a no-op")
    {
        ScopedPersistentBuffer<VBO> sb;

        sb.buffer.unmapIfNeeded(sb.obj);
        CHECK(sb.buffer.data() == nullptr);

        sb.buffer.unmapIfNeeded(sb.obj);
        CHECK(sb.buffer.data() == nullptr);
    }

    SECTION("Zero-byte reserve is a no-op on a default-constructed buffer")
    {
        ScopedPersistentBuffer<VBO> sb;

        CHECK(!sb.buffer.reserve(sb.obj, 0u, /* preserveExistingData */ false));
        CHECK(sb.buffer.data() == nullptr);
    }

    SECTION("First reserve allocates storage and creates the mapping")
    {
        ScopedPersistentBuffer<VBO> sb;

        CHECK(sb.buffer.reserve(sb.obj, 128u, /* preserveExistingData */ false));
        CHECK(sb.buffer.data() != nullptr);
    }

    SECTION("Reserve takes the fast path when capacity already suffices")
    {
        ScopedPersistentBuffer<VBO> sb;
        REQUIRE(sb.buffer.reserve(sb.obj, 1024u, false));

        void* const ptrBefore = sb.buffer.data();
        REQUIRE(ptrBefore != nullptr);

        CHECK(!sb.buffer.reserve(sb.obj, 0u, false));
        CHECK(!sb.buffer.reserve(sb.obj, 1u, false));
        CHECK(!sb.buffer.reserve(sb.obj, 1023u, false));
        CHECK(!sb.buffer.reserve(sb.obj, 1024u, false));

        // Fast path must not remap the storage.
        CHECK(sb.buffer.data() == ptrBefore);
    }

    SECTION("Reserve triggers growth when byteCount exceeds capacity")
    {
        ScopedPersistentBuffer<VBO> sb;

        REQUIRE(sb.buffer.reserve(sb.obj, 64u, false));
        CHECK(sb.buffer.reserve(sb.obj, 65u, false));
        CHECK(sb.buffer.data() != nullptr);
    }

    SECTION("Growth applies a 1.5x geometric factor when the request is smaller")
    {
        ScopedPersistentBuffer<VBO> sb;

        // Establish capacity = 100.
        REQUIRE(sb.buffer.reserve(sb.obj, 100u, false));
        CHECK(!sb.buffer.reserve(sb.obj, 100u, false));

        // Request 110 -> should grow to max(110, 100 + 50) = 150.
        REQUIRE(sb.buffer.reserve(sb.obj, 110u, false));
        CHECK(!sb.buffer.reserve(sb.obj, 150u, false));

        // Request 151 -> should grow to max(151, 150 + 75) = 225.
        REQUIRE(sb.buffer.reserve(sb.obj, 151u, false));
        CHECK(!sb.buffer.reserve(sb.obj, 225u, false));
    }

    SECTION("Growth honors the exact size when it exceeds the geometric target")
    {
        ScopedPersistentBuffer<VBO> sb;

        REQUIRE(sb.buffer.reserve(sb.obj, 100u, false));

        // 1.5 * 100 = 150, but 10000 > 150 -> capacity becomes exactly 10000.
        REQUIRE(sb.buffer.reserve(sb.obj, 10'000u, false));
        CHECK(!sb.buffer.reserve(sb.obj, 10'000u, false));
        CHECK(!sb.buffer.reserve(sb.obj, 9999u, false));

        // Next growth uses the new baseline: 10000 + 5000 = 15000.
        REQUIRE(sb.buffer.reserve(sb.obj, 10'001u, false));
        CHECK(!sb.buffer.reserve(sb.obj, 15'000u, false));
    }

    SECTION("Writing through the mapped pointer and flushing the range succeeds")
    {
        ScopedPersistentBuffer<VBO> sb;
        REQUIRE(sb.buffer.reserve(sb.obj, 1024u, false));

        auto* const bytes = static_cast<unsigned char*>(sb.buffer.data());
        REQUIRE(bytes != nullptr);

        ZA_MEMSET(bytes, 0x5A, 1024u);

        sb.buffer.flushBytesToGPU(sb.obj, /* byteOffset */ 0u, /* byteCount */ 1024u);
    }

    SECTION("flushBytesToGPU accepts partial and degenerate ranges")
    {
        ScopedPersistentBuffer<VBO> sb;
        REQUIRE(sb.buffer.reserve(sb.obj, 256u, false));
        REQUIRE(sb.buffer.data() != nullptr);

        // Middle slice.
        sb.buffer.flushBytesToGPU(sb.obj, 32u, 64u);

        // Single byte at the start.
        sb.buffer.flushBytesToGPU(sb.obj, 0u, 1u);

        // Zero-length slice at the end.
        sb.buffer.flushBytesToGPU(sb.obj, 256u, 0u);

        // Arbitrary byte range.
        sb.buffer.flushBytesToGPU(sb.obj, /* byteOffset */ 32u, /* byteCount */ 64u);
    }

    SECTION("Growth with preserveExistingData=true retains the original bytes")
    {
        ScopedPersistentBuffer<VBO> sb;
        REQUIRE(sb.buffer.reserve(sb.obj, 128u, false));

        auto* const writePtr = static_cast<unsigned char*>(sb.buffer.data());
        REQUIRE(writePtr != nullptr);

        unsigned char pattern[128];
        for (za::SizeT i = 0u; i < 128u; ++i)
            pattern[i] = static_cast<unsigned char>((i * 17u) ^ 0xABu);

        ZA_MEMCPY(writePtr, pattern, 128u);
        sb.buffer.flushBytesToGPU(sb.obj, 0u, 128u);

        // Grow: internally memcpys the old [0, 128) range into the new mapping.
        REQUIRE(sb.buffer.reserve(sb.obj, 4096u, /* preserveExistingData */ true));
        REQUIRE(sb.buffer.data() != nullptr);

        // The internal memcpy is done through the CPU-side mapping; the new
        // mapping must be flushed before `glGetNamedBufferSubData` is allowed
        // to see the bytes.
        sb.buffer.flushBytesToGPU(sb.obj, 0u, 128u);

        unsigned char readback[128]{};
        readbackBufferBytes(sb.obj, /* offset */ 0u, /* size */ 128u, readback);

        CHECK(ZA_MEMCMP(readback, pattern, 128u) == 0);

        // The rest of the grown buffer remains writable and round-trips through
        // the GL server.
        unsigned char tail[256];
        for (za::SizeT i = 0u; i < 256u; ++i)
            tail[i] = static_cast<unsigned char>(i);

        ZA_MEMCPY(static_cast<unsigned char*>(sb.buffer.data()) + 128u, tail, 256u);
        sb.buffer.flushBytesToGPU(sb.obj, 128u, 256u);

        unsigned char tailReadback[256]{};
        readbackBufferBytes(sb.obj, 128u, 256u, tailReadback);
        CHECK(ZA_MEMCMP(tailReadback, tail, 256u) == 0);
    }

    SECTION("Growth with preserveExistingData=false leaves a writable mapping")
    {
        ScopedPersistentBuffer<VBO> sb;
        REQUIRE(sb.buffer.reserve(sb.obj, 128u, false));

        // The pre-growth contents are intentionally discarded by the grow path;
        // the spec only guarantees that the post-growth bytes are indeterminate,
        // so we don't assert anything about them.
        ZA_MEMSET(sb.buffer.data(), 0xAB, 128u);
        sb.buffer.flushBytesToGPU(sb.obj, 0u, 128u);

        REQUIRE(sb.buffer.reserve(sb.obj, 4096u, /* preserveExistingData */ false));
        REQUIRE(sb.buffer.data() != nullptr);

        unsigned char fresh[256];
        for (za::SizeT i = 0u; i < 256u; ++i)
            fresh[i] = static_cast<unsigned char>((i + 7u) * 13u);

        ZA_MEMCPY(sb.buffer.data(), fresh, 256u);
        sb.buffer.flushBytesToGPU(sb.obj, 0u, 256u);

        unsigned char readback[256]{};
        readbackBufferBytes(sb.obj, 0u, 256u, readback);

        CHECK(ZA_MEMCMP(readback, fresh, 256u) == 0);
    }

    SECTION("CPU writes through the mapping round-trip through glGetNamedBufferSubData")
    {
        ScopedPersistentBuffer<VBO> sb;
        REQUIRE(sb.buffer.reserve(sb.obj, 512u, false));

        unsigned char pattern[512];
        for (za::SizeT i = 0u; i < 512u; ++i)
            pattern[i] = static_cast<unsigned char>(i & 0xFFu);

        ZA_MEMCPY(sb.buffer.data(), pattern, 512u);
        sb.buffer.flushBytesToGPU(sb.obj, 0u, 512u);

        unsigned char readback[512]{};
        readbackBufferBytes(sb.obj, 0u, 512u, readback);
        CHECK(ZA_MEMCMP(readback, pattern, 512u) == 0);

        // Partial overwrite + partial readback.
        for (za::SizeT i = 0u; i < 64u; ++i)
            pattern[128u + i] = static_cast<unsigned char>(0xF0u - i);

        ZA_MEMCPY(static_cast<unsigned char*>(sb.buffer.data()) + 128u, pattern + 128u, 64u);
        sb.buffer.flushBytesToGPU(sb.obj, 128u, 64u);

        unsigned char slice[64]{};
        readbackBufferBytes(sb.obj, 128u, 64u, slice);
        CHECK(ZA_MEMCMP(slice, pattern + 128u, 64u) == 0);
    }

    SECTION("Repeated grow-write-flush cycles succeed")
    {
        ScopedPersistentBuffer<VBO> sb;

        for (za::SizeT size = 64u; size <= 16u * 1024u; size *= 2u)
        {
            (void)sb.buffer.reserve(sb.obj, size, /* preserveExistingData */ true);
            REQUIRE(sb.buffer.data() != nullptr);

            ZA_MEMSET(sb.buffer.data(), static_cast<int>(size & 0xFFu), size);
            sb.buffer.flushBytesToGPU(sb.obj, 0u, size);
        }
    }

    SECTION("unmapIfNeeded after mapping clears the data pointer and is idempotent")
    {
        ScopedPersistentBuffer<VBO> sb;
        REQUIRE(sb.buffer.reserve(sb.obj, 256u, false));
        REQUIRE(sb.buffer.data() != nullptr);

        sb.buffer.unmapIfNeeded(sb.obj);
        CHECK(sb.buffer.data() == nullptr);

        sb.buffer.unmapIfNeeded(sb.obj);
        CHECK(sb.buffer.data() == nullptr);
    }

    SECTION("Move construction transfers the mapping and empties the source")
    {
        ScopedPersistentBuffer<VBO> source;
        REQUIRE(source.buffer.reserve(source.obj, 512u, false));

        void* const originalPtr = source.buffer.data();
        REQUIRE(originalPtr != nullptr);

        ScopedPersistentBuffer<VBO> dest{ZA_MOVE(source)};

        CHECK(source.buffer.data() == nullptr);
        CHECK(dest.buffer.data() == originalPtr);

        // The destination still honors the inherited capacity.
        CHECK(!dest.buffer.reserve(dest.obj, 512u, false));

        // And can still grow further.
        CHECK(dest.buffer.reserve(dest.obj, 2048u, /* preserveExistingData */ true));
        CHECK(dest.buffer.data() != nullptr);
    }

    SECTION("Move assignment into an empty destination transfers the mapping")
    {
        ScopedPersistentBuffer<VBO> source;
        REQUIRE(source.buffer.reserve(source.obj, 512u, false));

        void* const originalPtr = source.buffer.data();
        REQUIRE(originalPtr != nullptr);

        ScopedPersistentBuffer<VBO> dest;
        dest = ZA_MOVE(source);

        CHECK(source.buffer.data() == nullptr);
        CHECK(dest.buffer.data() == originalPtr);
    }

    SECTION("Self move assignment is a no-op and preserves the mapping")
    {
        ScopedPersistentBuffer<VBO> sb;
        REQUIRE(sb.buffer.reserve(sb.obj, 256u, false));

        void* const ptrBefore = sb.buffer.data();
        REQUIRE(ptrBefore != nullptr);

    #if defined(__GNUC__)
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wself-move"
    #endif
        sb.buffer = ZA_MOVE(sb.buffer);
    #if defined(__GNUC__)
        #pragma GCC diagnostic pop
    #endif

        CHECK(sb.buffer.data() == ptrBefore);
    }

    SECTION("GLElementBufferObject instantiation supports the full lifecycle")
    {
        ScopedPersistentBuffer<EBO> sb;

        REQUIRE(sb.buffer.reserve(sb.obj, 128u, false));
        REQUIRE(sb.buffer.data() != nullptr);

        auto* const bytes = static_cast<unsigned char*>(sb.buffer.data());
        ZA_MEMSET(bytes, 0x7F, 128u);
        sb.buffer.flushBytesToGPU(sb.obj, 0u, 128u);

        REQUIRE(sb.buffer.reserve(sb.obj, 4096u, /* preserveExistingData */ true));
        CHECK(sb.buffer.data() != nullptr);

        sb.buffer.unmapIfNeeded(sb.obj);
        CHECK(sb.buffer.data() == nullptr);
    }
}

#endif // !ZA_OPENGL_ES
