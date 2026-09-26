#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/GLUtils/GLCheck.hpp"
#include "Zancle/GLUtils/GLUniqueResource.hpp"
#include "Zancle/GLUtils/GlContext.hpp"
#include "Zancle/GLUtils/Glad.hpp"

#include "Zancle/Base/Assert.hpp"


namespace za::priv
{
////////////////////////////////////////////////////////////
/// \brief Internal policy struct exposing OpenGL functions for a specific buffer type
///
/// Acts as a policy class for `GLUniqueResource`, providing the static
/// `create`, `destroy`, `bind` and `get` functions tailored to a specific
/// type of OpenGL buffer object (e.g. VBO or EBO). The buffer kind is
/// selected at compile time via the `BufferType` (e.g. `GL_ARRAY_BUFFER`)
/// and matching `BindingType` (e.g. `GL_ARRAY_BUFFER_BINDING`) template
/// parameters.
///
/// Buffer objects are shareable across all GL contexts in the same share
/// group, so `create` / `destroy` are valid from any currently active
/// context -- the caller is required to ensure one is current. Binding and
/// querying likewise happen on whatever context is currently active.
///
/// \tparam BufferType  GL buffer target enum (e.g. `GL_ARRAY_BUFFER`)
/// \tparam BindingType GL binding-point query enum (e.g. `GL_ARRAY_BUFFER_BINDING`)
///
////////////////////////////////////////////////////////////
template <GLenum BufferType, GLenum BindingType>
struct GLBufferObjectFuncs
{
    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] static void create(unsigned int& id)
    {
        // Buffers are shareable across the share group, so creation is valid
        // from any current context. Caller must have one active -- saves the
        // two `makeCurrent` syscalls of a `GLSharedContextGuard`.
        ZA_ASSERT(GlContext::hasActiveThreadLocalGlContext());
        glCheck(glGenBuffers(1, &id));
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] static void destroy(unsigned int& id)
    {
        // See `create` -- destruction is also valid from any current context.
        ZA_ASSERT(GlContext::hasActiveThreadLocalGlContext());
        glCheck(glDeleteBuffers(1, &id));
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] static void bind(unsigned int id)
    {
        glCheck(glBindBuffer(BufferType, id));
    }


    ////////////////////////////////////////////////////////////
    [[gnu::always_inline, gnu::flatten]] static void get(unsigned int& id)
    {
        glCheck(glGetIntegerv(BindingType, reinterpret_cast<GLint*>(&id)));
    }
};

////////////////////////////////////////////////////////////
/// \brief RAII wrapper for a generic OpenGL buffer object (e.g. VBO, EBO)
/// \ingroup glutils
///
/// Manages the lifecycle (creation, destruction) and binding of an
/// OpenGL buffer object specified by `BufferType` (e.g. `GL_ARRAY_BUFFER`)
/// and its corresponding binding point `BindingType` (e.g.
/// `GL_ARRAY_BUFFER_BINDING`). Inherits from `GLUniqueResource` to handle
/// the underlying OpenGL ID and exposes the two enums via the nested
/// `bufferType` / `bindingType` constants for compile-time introspection.
///
/// User code should normally use one of the prebuilt aliases such as
/// `za::GLVertexBufferObject` or `za::GLElementBufferObject` rather than
/// instantiating this template directly.
///
/// \tparam BufferType  GL buffer target enum
/// \tparam BindingType GL binding-point query enum
///
/// \see `GLUniqueResource`, `GLVertexBufferObject`, `GLElementBufferObject`
///
////////////////////////////////////////////////////////////
template <GLenum BufferType, GLenum BindingType>
struct GLBufferObject : GLUniqueResource<GLBufferObjectFuncs<BufferType, BindingType>>
{
    enum : GLenum
    {
        bufferType  = BufferType, //!< GL buffer target enum (mirrors the template parameter)
        bindingType = BindingType //!< GL binding-point query enum (mirrors the template parameter)
    };

    using GLUniqueResource<GLBufferObjectFuncs<BufferType, BindingType>>::GLUniqueResource;
};

} // namespace za::priv


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Specialization of `GLBufferObject` for Vertex Buffer Objects (VBOs)
/// \ingroup glutils
///
/// Provides RAII management for buffers used to store vertex attribute
/// data, bound to the `GL_ARRAY_BUFFER` target.
///
////////////////////////////////////////////////////////////
struct GLVertexBufferObject : priv::GLBufferObject<GL_ARRAY_BUFFER, GL_ARRAY_BUFFER_BINDING>
{
    using priv::GLBufferObject<GL_ARRAY_BUFFER, GL_ARRAY_BUFFER_BINDING>::GLBufferObject;
};

////////////////////////////////////////////////////////////
/// \brief Specialization of `GLBufferObject` for Element/Index Buffer Objects (EBOs/IBOs)
/// \ingroup glutils
///
/// Provides RAII management for buffers used to store vertex indices,
/// bound to the `GL_ELEMENT_ARRAY_BUFFER` target.
///
////////////////////////////////////////////////////////////
struct GLElementBufferObject : priv::GLBufferObject<GL_ELEMENT_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER_BINDING>
{
    using priv::GLBufferObject<GL_ELEMENT_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER_BINDING>::GLBufferObject;
};

} // namespace za
