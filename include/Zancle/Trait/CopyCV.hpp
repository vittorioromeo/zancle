#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


namespace za::priv
{
////////////////////////////////////////////////////////////
template <typename From>
struct CopyCVImpl
{
    template <typename To>
    using type = To;
};


////////////////////////////////////////////////////////////
template <typename From>
struct CopyCVImpl<const From>
{
    template <typename To>
    using type = const To;
};


////////////////////////////////////////////////////////////
template <typename From>
struct CopyCVImpl<volatile From>
{
    template <typename To>
    using type = volatile To;
};


////////////////////////////////////////////////////////////
template <typename From>
struct CopyCVImpl<const volatile From>
{
    template <typename To>
    using type = const volatile To;
};

} // namespace za::priv


namespace za
{
////////////////////////////////////////////////////////////
template <typename From, typename To>
using CopyCV = typename priv::CopyCVImpl<From>::template type<To>;

} // namespace za
