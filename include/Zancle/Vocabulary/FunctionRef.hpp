#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Trait/IsSame.hpp"
#include "Zancle/Trait/RemoveCVRef.hpp"
#include "Zancle/Trait/RemoveReference.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/Macros.hpp"


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Non-owning, lightweight reference to a callable
///
////////////////////////////////////////////////////////////
template <typename TSignature>
class FunctionRef;


////////////////////////////////////////////////////////////
/// \brief Non-owning, lightweight reference to a callable
///
/// `FunctionRef` is a simplified alternative to `std::function_ref`
/// intended for use as a function parameter, to pass any callable
/// (free function, lambda, functor) without type-erasure allocation
/// and without copying the callable.
///
/// The referenced callable must outlive the `FunctionRef`. In typical
/// use as a function parameter this is automatically the case because
/// the caller's argument lives through the full-expression of the call.
///
////////////////////////////////////////////////////////////
template <typename TReturn, typename... Ts>
class FunctionRef<TReturn(Ts...)>
{
private:
    ////////////////////////////////////////////////////////////
    using FnPtrType = TReturn (*)(Ts...);
    using ThunkType = TReturn (*)(void*, Ts&&...);


    ////////////////////////////////////////////////////////////
    void*     m_obj;
    ThunkType m_thunk;

public:
    ////////////////////////////////////////////////////////////
    FunctionRef() = delete;


    ////////////////////////////////////////////////////////////
    /// \brief Construct from a function pointer (must be non-null)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline]] FunctionRef(FnPtrType fn) noexcept :
        m_obj{reinterpret_cast<void*>(fn)},
        m_thunk{[](void* o, Ts&&... args) -> TReturn { return reinterpret_cast<FnPtrType>(o)(ZA_FORWARD(args)...); }}
    {
        ZA_ASSERT(fn != nullptr);
    }


    ////////////////////////////////////////////////////////////
    /// \brief Construct from any callable object (lambda, functor, ...)
    ///
    /// The callable is referenced, not copied. Its lifetime must
    /// extend across any subsequent calls to `operator()`.
    ///
    ////////////////////////////////////////////////////////////
    template <typename TFFwd>
        requires(!za::isSame<za::RemoveCVRefIndirect<TFFwd>, FunctionRef> &&
                 !za::isSame<za::RemoveCVRefIndirect<TFFwd>, FnPtrType>)
    [[nodiscard, gnu::always_inline]] FunctionRef(TFFwd&& f) noexcept :
        m_obj{const_cast<void*>(static_cast<const void*>(&f))},
        m_thunk{[](void* o, Ts&&... args) -> TReturn
    {
        using UnrefType = ZA_REMOVE_REFERENCE(TFFwd);

        if constexpr (ZA_IS_SAME(TReturn, void))
            (*reinterpret_cast<UnrefType*>(o))(ZA_FORWARD(args)...); // Discard any result
        else
            return (*reinterpret_cast<UnrefType*>(o))(ZA_FORWARD(args)...);
    }}
    {
    }


    ////////////////////////////////////////////////////////////
    /// \brief Invoke the referenced callable
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] TReturn operator()(Ts... args) const
    {
        return m_thunk(m_obj, ZA_FORWARD(args)...);
    }
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::FunctionRef
/// \ingroup system
///
/// Non-owning, lightweight reference to a callable, intended for
/// use as a function parameter. Does not allocate, does not copy
/// the referenced callable.
///
////////////////////////////////////////////////////////////
