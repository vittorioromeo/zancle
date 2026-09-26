#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "../src/Zancle/GLUtils/GlContext.hpp" // IWYU pragma: keep

#include "Zancle/Window/ContextSettings.hpp"
#include "Zancle/Window/WindowContext.hpp"

#include "Zancle/Err/Err.hpp"


////////////////////////////////////////////////////////////
/// \brief Test-only helper that owns a fresh GL context distinct from
///        the shared graphics context.
///
/// Used by both the Window and Graphics test suites to spin up
/// additional GL contexts and exercise multi-context paths
/// (context-id-aware caches, `setActive` swaps, deferred deletion under
/// shared resources, etc.). Wraps the friend-protected
/// `WindowContext::createGlContext` and the thread-local active-context
/// API.
///
/// Activates itself on construction and deactivates on destruction --
/// so a stack-scoped `TestContext` always restores the prior active
/// context when it goes out of scope.
///
////////////////////////////////////////////////////////////
struct TestContext
{
    decltype(za::WindowContext::createGlContext(za::ContextSettings{})) glContext;

    TestContext() : glContext(za::WindowContext::createGlContext(za::ContextSettings{}))
    {
        if (!za::WindowContext::setActiveThreadLocalGlContext(*glContext, true))
            za::priv::errMsg("Failed to activate TestContext on construction");
    }

    ~TestContext()
    {
        if (glContext != nullptr && !za::WindowContext::setActiveThreadLocalGlContext(*glContext, false))
            za::priv::errMsg("Failed to deactivate TestContext on destruction");
    }

    TestContext(const TestContext&)                = delete;
    TestContext& operator=(const TestContext&)     = delete;
    TestContext(TestContext&&) noexcept            = default;
    TestContext& operator=(TestContext&&) noexcept = default;

    [[nodiscard]] bool setActive(bool active) const
    {
        return za::WindowContext::setActiveThreadLocalGlContext(*glContext, active);
    }

    [[nodiscard]] const za::ContextSettings& getSettings() const
    {
        return glContext->getSettings();
    }

    [[nodiscard]] unsigned int getId() const
    {
        return glContext->getId();
    }

    [[nodiscard]] static unsigned int getActiveThreadLocalGlContextId()
    {
        return za::WindowContext::getActiveThreadLocalGlContextId();
    }

    [[nodiscard]] static bool hasActiveThreadLocalGlContext()
    {
        return za::WindowContext::hasActiveThreadLocalGlContext();
    }

    [[nodiscard]] static bool isActiveGlContextSharedContext()
    {
        return za::WindowContext::isActiveGlContextSharedContext();
    }

    [[nodiscard]] static bool isExtensionAvailable(const char* name)
    {
        return za::WindowContext::isExtensionAvailable(name);
    }

    [[nodiscard]] static auto getFunction(const char* name)
    {
        return za::WindowContext::getFunction(name);
    }
};
