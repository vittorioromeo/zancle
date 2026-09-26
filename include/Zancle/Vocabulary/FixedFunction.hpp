#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Trait/IsCopyConstructible.hpp"
#include "Zancle/Trait/IsSame.hpp"
#include "Zancle/Trait/RemoveCVRef.hpp"

#include "Zancle/Base/Abort.hpp"
#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/Launder.hpp"
#include "Zancle/Base/Macros.hpp"
#include "Zancle/Base/MaxAlignT.hpp"
#include "Zancle/Base/PlacementNew.hpp"
#include "Zancle/Base/SizeT.hpp"

// TODO P1: provide triviallyrelocatable version

namespace za
{
////////////////////////////////////////////////////////////
/// \brief Non-allocating `std::function` alternative with fixed storage size
///
////////////////////////////////////////////////////////////
template <typename TSignature, SizeT TStorageSize>
class FixedFunction;


////////////////////////////////////////////////////////////
/// \brief Non-allocating `std::function` alternative with fixed storage size
///
/// Move-only callables are supported, but copying a `FixedFunction`
/// that holds one aborts at run time (the callable type is erased).
///
////////////////////////////////////////////////////////////
template <typename TReturn, typename... Ts, SizeT TStorageSize>
class FixedFunction<TReturn(Ts...), TStorageSize>
{
private:
    ////////////////////////////////////////////////////////////
    enum class Operation : unsigned char
    {
        Destroy       = 0u,
        MoveConstruct = 1u,
        CopyConstruct = 2u,
    };


    ////////////////////////////////////////////////////////////
    using RetType = TReturn;


    ////////////////////////////////////////////////////////////
    using FnPtrType  = RetType (*)(Ts...);
    using MethodType = RetType (*)(char*, FnPtrType, Ts&&...);
    using AllocType  = void (*)(char*, void* objectPtr, const Operation operation);


    ////////////////////////////////////////////////////////////
    union
    {
        alignas(MaxAlignT) char objStorage[TStorageSize];
        FnPtrType functionPtr;
    };


    ////////////////////////////////////////////////////////////
    MethodType m_methodPtr;
    AllocType  m_allocPtr;


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] void destroyIfNeeded() noexcept
    {
        if (m_allocPtr == nullptr)
            return;

        m_allocPtr(objStorage, nullptr, Operation::Destroy);
    }

public:
    ////////////////////////////////////////////////////////////
    [[nodiscard]] FixedFunction() noexcept : functionPtr{nullptr}, m_methodPtr{nullptr}, m_allocPtr{nullptr}
    {
    }


    ////////////////////////////////////////////////////////////
    /// \brief Construct from a callable, stored (decayed) in internal storage by copy or move
    ///
    ////////////////////////////////////////////////////////////
    template <typename TFFwd>
        requires(!za::isSame<za::RemoveCVRefIndirect<TFFwd>, FixedFunction>)
    [[nodiscard]] FixedFunction(TFFwd&& f) : FixedFunction()
    {
        using StoredType = ZA_REMOVE_CVREF(TFFwd);

        static_assert(sizeof(StoredType) <= TStorageSize);
        static_assert(alignof(StoredType) <= alignof(MaxAlignT));

        ZA_PLACEMENT_NEW(objStorage) StoredType(ZA_FORWARD(f));

        // NOLINTNEXTLINE(readability-non-const-parameter)
        m_methodPtr = [](char* s, FnPtrType, Ts&&... xs) -> RetType
        {
            if constexpr (ZA_IS_SAME(RetType, void))
                ZA_LAUNDER_CAST(StoredType*, s)->operator()(ZA_FORWARD(xs)...); // Discard any result
            else
                return ZA_LAUNDER_CAST(StoredType*, s)->operator()(ZA_FORWARD(xs)...);
        };

        // NOLINTNEXTLINE(readability-non-const-parameter)
        m_allocPtr = [](char* s, void* o, const Operation operation)
        {
            if (operation == Operation::Destroy)
            {
                ZA_ASSERT(s != nullptr);
                ZA_LAUNDER_CAST(StoredType*, s)->~StoredType();
            }
            else if (operation == Operation::MoveConstruct)
            {
                ZA_ASSERT(o != nullptr);
                ZA_PLACEMENT_NEW(s) StoredType(ZA_MOVE(*static_cast<StoredType*>(o)));
            }
            else
            {
                ZA_ASSERT(operation == Operation::CopyConstruct);
                ZA_ASSERT(o != nullptr);

                if constexpr (isCopyConstructible<StoredType>)
                {
                    ZA_PLACEMENT_NEW(s) StoredType(*static_cast<const StoredType*>(o));
                }
                else
                {
                    ZA_ASSERT(false && "Cannot copy a `FixedFunction` holding a move-only callable");
                    za::abort();
                }
            }
        };
    }


    ////////////////////////////////////////////////////////////
    /// \brief Construct from a function pointer; a null pointer results in an empty function
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] FixedFunction(FnPtrType f) noexcept : functionPtr{f}, m_methodPtr{nullptr}, m_allocPtr{nullptr}
    {
        if (f != nullptr)
            m_methodPtr = [](char* /* unused */, FnPtrType xf, Ts&&... xs) -> RetType { return xf(ZA_FORWARD(xs)...); };
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit FixedFunction(decltype(nullptr)) noexcept :
        functionPtr{nullptr},
        m_methodPtr{nullptr},
        m_allocPtr{nullptr}
    {
    }

    ////////////////////////////////////////////////////////////
    FixedFunction(const FixedFunction& rhs) : FixedFunction()
    {
        if (rhs.m_allocPtr == nullptr)
        {
            // Free-function path: no copy can throw, set both fields.
            m_methodPtr = rhs.m_methodPtr;
            functionPtr = rhs.functionPtr;
            return;
        }

        // Stored-callable path: the copy-construct can throw.
        // Update our metadata only after it succeeds, so that a
        // partially-constructed `*this` destroys cleanly during
        // exception unwind (`m_allocPtr == nullptr` -> no-op).
        rhs.m_allocPtr(objStorage, const_cast<char*>(rhs.objStorage), Operation::CopyConstruct);
        m_methodPtr = rhs.m_methodPtr;
        m_allocPtr  = rhs.m_allocPtr;
    }


    ////////////////////////////////////////////////////////////
    FixedFunction& operator=(const FixedFunction& rhs)
    {
        if (this == &rhs)
            return *this;

        destroyIfNeeded();

        m_allocPtr  = nullptr; // Safe empty state in case copy-construct throws
        m_methodPtr = nullptr;

        if (rhs.m_allocPtr == nullptr)
        {
            // Free-function path: no copy can throw, set both fields.
            m_methodPtr = rhs.m_methodPtr;
            functionPtr = rhs.functionPtr;
            return *this;
        }

        // Stored-callable path: the copy-construct can throw.
        // Update our metadata only after it succeeds.
        rhs.m_allocPtr(objStorage, const_cast<char*>(rhs.objStorage), Operation::CopyConstruct);
        m_methodPtr = rhs.m_methodPtr;
        m_allocPtr  = rhs.m_allocPtr;

        return *this;
    }


    ////////////////////////////////////////////////////////////
    FixedFunction& operator=(decltype(nullptr)) noexcept
    {
        destroyIfNeeded();

        m_methodPtr = nullptr;
        m_allocPtr  = nullptr;
        functionPtr = nullptr;

        return *this;
    }


    ////////////////////////////////////////////////////////////
    FixedFunction(FixedFunction&& rhs) noexcept : FixedFunction()
    {
        m_methodPtr = rhs.m_methodPtr;

        if (rhs.m_allocPtr == nullptr)
        {
            functionPtr     = rhs.functionPtr;
            rhs.m_methodPtr = nullptr;
            rhs.functionPtr = nullptr;

            return;
        }

        m_allocPtr = rhs.m_allocPtr;
        m_allocPtr(objStorage, rhs.objStorage, Operation::MoveConstruct);

        rhs.m_allocPtr(rhs.objStorage, nullptr, Operation::Destroy);
        rhs.m_methodPtr = nullptr;
        rhs.m_allocPtr  = nullptr;
    }


    ////////////////////////////////////////////////////////////
    FixedFunction& operator=(FixedFunction&& rhs) noexcept
    {
        if (this == &rhs)
            return *this;

        destroyIfNeeded();

        m_methodPtr = rhs.m_methodPtr;
        m_allocPtr  = nullptr;

        if (rhs.m_allocPtr == nullptr)
        {
            functionPtr     = rhs.functionPtr;
            rhs.m_methodPtr = nullptr;
            rhs.functionPtr = nullptr;

            return *this;
        }

        m_allocPtr = rhs.m_allocPtr;
        m_allocPtr(objStorage, rhs.objStorage, Operation::MoveConstruct);

        rhs.m_allocPtr(rhs.objStorage, nullptr, Operation::Destroy);
        rhs.m_methodPtr = nullptr;
        rhs.m_allocPtr  = nullptr;

        return *this;
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] ~FixedFunction() noexcept
    {
        destroyIfNeeded();
    }


    ////////////////////////////////////////////////////////////
    /// \brief Invoke the wrapped callable
    ///
    /// Mirrors `std::function::operator() const`. The wrapped callable
    /// is invoked through a non-`const` path regardless of `*this`'s
    /// const-ness, so a callable whose `operator()` is non-`const`
    /// will mutate its captured state. This is a *logical* const
    /// violation. Wrap mutable closures in a non-`const`
    /// `FixedFunction` if you don't want this behavior.
    ///
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] RetType operator()(Ts... args) const
    {
        ZA_ASSERT(m_methodPtr != nullptr);
        return m_methodPtr(const_cast<char*>(objStorage), functionPtr, ZA_FORWARD(args)...);
    }


    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::flatten]] explicit operator bool() const
    {
        return m_methodPtr != nullptr;
    }
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::FixedFunction
/// \ingroup system
///
/// Non-allocating `std::function` alternative with fixed storage size
///
////////////////////////////////////////////////////////////
