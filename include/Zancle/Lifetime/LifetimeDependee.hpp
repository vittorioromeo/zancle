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

class ZA_SYSTEM_API LifetimeDependee
{
public:
    struct ZA_SYSTEM_API TestingModeGuard
    {
        [[nodiscard]] TestingModeGuard(const char* dependeeName);
        ~TestingModeGuard();

        TestingModeGuard(const TestingModeGuard&) = delete;
        TestingModeGuard(TestingModeGuard&&)      = delete;

        TestingModeGuard& operator=(const TestingModeGuard&) = delete;
        TestingModeGuard& operator=(TestingModeGuard&&)      = delete;

        [[nodiscard]] static bool fatalErrorTriggered(const char* dependeeName);
    };

    explicit LifetimeDependee(const char* dependeeName, const char* dependantName);
    ~LifetimeDependee();

    LifetimeDependee(const LifetimeDependee& rhs);
    LifetimeDependee(LifetimeDependee&& rhs) noexcept;

    LifetimeDependee& operator=(const LifetimeDependee& rhs);
    LifetimeDependee& operator=(LifetimeDependee&& rhs) noexcept;

    void addDependant();
    void subDependant();

private:
    const char* m_dependeeName;                                        ///< Readable dependee type name
    const char* m_dependantName;                                       ///< Readable dependent type name
    alignas(unsigned int) char m_dependantCount[sizeof(unsigned int)]; ///< Current alive dependant count
};

} // namespace za::priv

    ////////////////////////////////////////////////////////////
    /// \brief Declare, in class `dependeeType`, that `dependantType` objects depend on it
    ///
    /// Reads as a sentence: `ZA_LIFETIME_DEPENDED_ON_BY(Font, Text)` means
    /// "`Font` is depended on by `Text`". At most once per class.
    ///
    ////////////////////////////////////////////////////////////
    // NOLINTBEGIN(bugprone-macro-parentheses)
    #define ZA_LIFETIME_DEPENDED_ON_BY(dependeeType, dependantType)   \
        friend dependantType;                                         \
        mutable ::za::priv::LifetimeDependee m_zaPrivLifetimeDependee \
        {                                                             \
            #dependeeType, #dependantType                             \
        }
// NOLINTEND(bugprone-macro-parentheses)

#else // ZA_ENABLE_LIFETIME_TRACKING

    #define ZA_LIFETIME_DEPENDED_ON_BY(dependeeType, dependantType) static_assert(true)

#endif // ZA_ENABLE_LIFETIME_TRACKING


////////////////////////////////////////////////////////////
/// \file
///
/// \brief Debug-only detection of objects destroyed while others still point to them
///
/// A *dependant* keeps a pointer to a *dependee* (e.g. `za::Text` points
/// to its `za::Font`). With `ZA_ENABLE_LIFETIME_TRACKING` defined (the
/// default in Debug builds), every dependee counts its dependants, and
/// destroying it while the count is non-zero prints a diagnostic with a
/// stack trace and aborts. Otherwise, all macros expand to nothing.
///
/// Usage, for `Text` depending on `Font`:
/// \code
/// class Font
/// {
///     ZA_LIFETIME_DEPENDED_ON_BY(Font, Text); // `Font` is depended on by `Text`
/// };
///
/// class Text
/// {
///     const Font* m_font;
///     ZA_LIFETIME_DEPENDS_ON(Font); // `Text` depends on `Font`
///
///     void setFont(const Font& font)
///     {
///         m_font = &font;
///         ZA_LIFETIME_UPDATE_DEPENDENCY(this, Font, m_font); // after every change of `m_font`
///     }
/// };
/// \endcode
///
/// Semantics:
/// - `ZA_LIFETIME_UPDATE_DEPENDENCY` unregisters the dependant from its
///   previous dependee and registers it with the new one (none if null).
/// - Copying a dependant registers the copy with the same dependee;
///   moving one transfers its registration. Defaulted copy and move
///   operations are therefore correct, as long as they also copy the
///   dependee pointer.
/// - A copy of a dependee starts with no dependants. Moving a dependee
///   does not transfer its dependants: they still point to the moved-from
///   object, so destroying it before they are updated is an error.
/// - Assigning to a dependee keeps its dependants, which still point to it.
///
/// Testing: while a `priv::LifetimeDependee::TestingModeGuard` for a
/// dependee type name is alive, errors for that type are recorded (see
/// `fatalErrorTriggered`) instead of aborting, and its counts are no
/// longer updated. Dependant destructors that access their dependee
/// should begin with `ZA_LIFETIME_RETURN_IF_TESTING_ERROR`.
///
////////////////////////////////////////////////////////////
