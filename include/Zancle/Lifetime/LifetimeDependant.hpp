#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md

#ifdef ZA_ENABLE_LIFETIME_TRACKING

    ////////////////////////////////////////////////////////////
    // Headers
    ////////////////////////////////////////////////////////////
    #include "Zancle/Config.hpp"


namespace za::priv
{
class LifetimeDependee;

class ZA_SYSTEM_API LifetimeDependant
{
public:
    explicit LifetimeDependant(const char* dependeeName, LifetimeDependee* dependee = nullptr) noexcept;
    ~LifetimeDependant();

    LifetimeDependant(const LifetimeDependant& rhs) noexcept;
    LifetimeDependant(LifetimeDependant&& rhs) noexcept;

    LifetimeDependant& operator=(const LifetimeDependant& rhs) noexcept;
    LifetimeDependant& operator=(LifetimeDependant&& rhs) noexcept;

    void update(LifetimeDependee* dependee) noexcept;

    [[nodiscard]] bool isTestingModeErrorTriggered() const noexcept;

private:
    void addSelfAsDependant();
    void subSelfAsDependant();

    const char*       m_dependeeName;
    LifetimeDependee* m_dependee;
};

} // namespace za::priv

    ////////////////////////////////////////////////////////////
    /// \brief Declare that the enclosing class depends on a `dependeeType` object
    ///
    /// Reads as a sentence: `ZA_LIFETIME_DEPENDS_ON(Font)` in `Text` means
    /// "`Text` depends on `Font`". Once per dependee type.
    ///
    ////////////////////////////////////////////////////////////
    // NOLINTBEGIN(bugprone-macro-parentheses)
    #define ZA_LIFETIME_DEPENDS_ON(dependeeType)                                         \
        mutable ::za::priv::LifetimeDependant m_zaPrivLifetimeDependencyOn##dependeeType \
        {                                                                                \
            #dependeeType                                                                \
        }

    ////////////////////////////////////////////////////////////
    /// \brief Make `*objectPtr` depend on `*dependeePtr` (a `dependeeType`), or on nothing if null
    ///
    /// Must be used after every change of the pointer to the dependee,
    /// e.g. `ZA_LIFETIME_UPDATE_DEPENDENCY(this, Font, m_font)`.
    ///
    ////////////////////////////////////////////////////////////
    #define ZA_LIFETIME_UPDATE_DEPENDENCY(objectPtr, dependeeType, dependeePtr) \
        (objectPtr)->m_zaPrivLifetimeDependencyOn##dependeeType.update(         \
            (dependeePtr) == nullptr ? nullptr : &(dependeePtr)->m_zaPrivLifetimeDependee)

    ////////////////////////////////////////////////////////////
    /// \brief In testing mode, return if the `dependeeType` object was destroyed too early
    ///
    /// Use at the start of dependant destructors that access their dependee.
    ///
    ////////////////////////////////////////////////////////////
    #define ZA_LIFETIME_RETURN_IF_TESTING_ERROR(dependeeType)                             \
        do                                                                                \
        {                                                                                 \
            if (m_zaPrivLifetimeDependencyOn##dependeeType.isTestingModeErrorTriggered()) \
                return;                                                                   \
        } while (false)
// NOLINTEND(bugprone-macro-parentheses)

#else // ZA_ENABLE_LIFETIME_TRACKING

    #define ZA_LIFETIME_DEPENDS_ON(dependeeType) static_assert(true)

    #define ZA_LIFETIME_UPDATE_DEPENDENCY(...) (void)0

    #define ZA_LIFETIME_RETURN_IF_TESTING_ERROR(dependeeType) (void)0

#endif // ZA_ENABLE_LIFETIME_TRACKING


////////////////////////////////////////////////////////////
/// \file
///
/// \brief Dependant side of lifetime tracking (see `Zancle/Lifetime/LifetimeDependee.hpp`)
///
////////////////////////////////////////////////////////////
