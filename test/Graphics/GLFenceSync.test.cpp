#include "Tst/Tst.hpp"
#include "WindowUtil.hpp"

#include "Zancle/GLUtils/GLFenceSync.hpp"

#include "Zancle/GLUtils/FenceUtils.hpp"
#include "Zancle/GLUtils/GLCheck.hpp"
#include "Zancle/GLUtils/Glad.hpp"

#include "Zancle/Graphics/GraphicsContext.hpp"

#include "Zancle/Window/WindowContext.hpp"

#include "Zancle/Base/Macros.hpp"

#include "Zancle/Trait/IsCopyAssignable.hpp"
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsNothrowMoveAssignable.hpp"
#include "Zancle/Trait/IsNothrowMoveConstructible.hpp"


#ifndef ZA_OPENGL_ES

namespace
{
using Fence = za::priv::GLFenceSync;


////////////////////////////////////////////////////////////
void drainGLCommandQueue()
{
    glCheck(glFinish());
}

} // namespace


TEST_CASE("[GLUtils] za::priv::GLFenceSync / FenceUtils" * tst::skip(skipDisplayTests))
{
    [[maybe_unused]] auto& graphicsContext = TST_CASE_SHARED(za::GraphicsContext::create().value());

    SECTION("Type traits")
    {
        STATIC_CHECK(!za::isCopyConstructible<Fence>);
        STATIC_CHECK(!za::isCopyAssignable<Fence>);
        STATIC_CHECK(za::isNoThrowMoveConstructible<Fence>);
        STATIC_CHECK(za::isNoThrowMoveAssignable<Fence>);
    }

    SECTION("Default-constructed state has no native fence")
    {
        Fence fence;
        CHECK(!fence.hasValue());
        CHECK(!static_cast<bool>(fence));
        CHECK(fence.getNativeHandle() == nullptr);
    }

    SECTION("reset and deleteFenceIfNeeded are safe no-ops on an empty fence")
    {
        Fence fence;

        fence.reset();
        CHECK(!fence);

        za::priv::deleteFenceIfNeeded(fence);
        CHECK(!fence);
    }

    SECTION("makeFence returns an owning fence")
    {
        Fence fence = za::priv::makeFence();
        CHECK(fence.hasValue());
        CHECK(static_cast<bool>(fence));
        CHECK(fence.getNativeHandle() != nullptr);
    }

    SECTION("deleteFenceIfNeeded consumes the fence and is idempotent")
    {
        Fence fence = za::priv::makeFence();
        REQUIRE(static_cast<bool>(fence));

        za::priv::deleteFenceIfNeeded(fence);
        CHECK(!fence);

        za::priv::deleteFenceIfNeeded(fence);
        CHECK(!fence);
    }

    SECTION("waitOnFence on an empty fence is a no-op")
    {
        Fence fence;
        za::priv::waitOnFence(fence);
        CHECK(!fence);
    }

    SECTION("waitOnFence consumes a signaled fence")
    {
        Fence fence = za::priv::makeFence();
        REQUIRE(static_cast<bool>(fence));

        drainGLCommandQueue();
        za::priv::waitOnFence(fence);

        CHECK(!fence);
    }

    SECTION("tryWaitOnFence returns true for an empty fence")
    {
        Fence fence;
        CHECK(za::priv::tryWaitOnFence(fence));
        CHECK(!fence);
    }

    SECTION("tryWaitOnFence consumes a signaled fence")
    {
        Fence fence = za::priv::makeFence();
        REQUIRE(static_cast<bool>(fence));

        drainGLCommandQueue();
        CHECK(za::priv::tryWaitOnFence(fence));
        CHECK(!fence);
    }

    SECTION("Move construction transfers ownership")
    {
        Fence source = za::priv::makeFence();
        REQUIRE(static_cast<bool>(source));

        const void* const originalHandle = source.getNativeHandle();
        Fence             dest{ZA_MOVE(source)};

        CHECK(!source);
        CHECK(static_cast<bool>(dest));
        CHECK(dest.getNativeHandle() == originalHandle);

        za::priv::deleteFenceIfNeeded(dest);
    }

    SECTION("Move assignment transfers ownership and clears the source")
    {
        Fence source = za::priv::makeFence();
        Fence dest   = za::priv::makeFence();

        REQUIRE(static_cast<bool>(source));
        REQUIRE(static_cast<bool>(dest));

        const void* const sourceHandle = source.getNativeHandle();
        dest                           = ZA_MOVE(source);

        CHECK(!source);
        CHECK(static_cast<bool>(dest));
        CHECK(dest.getNativeHandle() == sourceHandle);

        za::priv::deleteFenceIfNeeded(dest);
    }
}

#endif // !ZA_OPENGL_ES
