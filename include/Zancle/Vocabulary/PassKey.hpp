#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Generic implementation of the PassKey idiom
///
/// `PassKey<T>` is a tag type that only `T` can construct (because it
/// befriends `T`). Functions that take a `PassKey<T>` as a parameter
/// are therefore callable only by `T`, even when they are publicly
/// declared. This lets a class expose a function to a single specific
/// caller without forcing it to be a friend.
///
/// `PassKey` is non-copyable and non-movable to prevent third parties
/// from acquiring an instance through indirect means.
///
////////////////////////////////////////////////////////////
template <typename T>
class [[nodiscard]] PassKey
{
    friend T;

private:
    ////////////////////////////////////////////////////////////
    // Private, so only `T` can construct a key. Since C++20, any
    // user-declared constructor prevents `PassKey` from being an
    // aggregate, so defaulting it does not open a loophole.
    [[nodiscard]] explicit PassKey() noexcept = default;

public:
    ////////////////////////////////////////////////////////////
    PassKey(const PassKey&) = delete;
    PassKey(PassKey&&)      = delete;

    ////////////////////////////////////////////////////////////
    PassKey& operator=(const PassKey&) = delete;
    PassKey& operator=(PassKey&&)      = delete;
};

} // namespace za
