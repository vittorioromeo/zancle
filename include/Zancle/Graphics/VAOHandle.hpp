#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Export.hpp"

#include "Zancle/Vocabulary/InPlacePImpl.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
class RenderTarget;
struct GLVAOGroup;
} // namespace za


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Owning handle to an OpenGL Vertex Array Object
///
/// `za::VAOHandle` wraps a single OpenGL VAO. It is the VAO
/// that the instanced draw paths
/// (`za::RenderTarget::drawInstancedVertices` and
/// `za::RenderTarget::drawInstancedIndexedVertices`) bind
/// before issuing the draw call. Per-instance data is stored
/// in separate `za::VBOHandle` objects managed by the user.
///
/// `VAOHandle` is move-only. Construct one per "instance set"
/// you want to render and reuse it across frames; the
/// underlying GPU object stays alive for the lifetime of the
/// handle.
///
////////////////////////////////////////////////////////////
class [[nodiscard]] ZA_GRAPHICS_API VAOHandle
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Default constructor
    ///
    /// Creates a fresh OpenGL VAO with no attribute streams
    /// configured. Configure it from a `setupFn` callback passed
    /// to one of the instanced draw paths.
    ///
    ////////////////////////////////////////////////////////////
    VAOHandle();

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    /// Releases the underlying OpenGL VAO.
    ///
    ////////////////////////////////////////////////////////////
    ~VAOHandle();

    ////////////////////////////////////////////////////////////
    VAOHandle(const VAOHandle&)            = delete;
    VAOHandle& operator=(const VAOHandle&) = delete;

    ////////////////////////////////////////////////////////////
    VAOHandle(VAOHandle&&) noexcept;
    VAOHandle& operator=(VAOHandle&&) noexcept;

private:
    friend RenderTarget;

    ////////////////////////////////////////////////////////////
    /// \brief Internal accessor used by `za::RenderTarget` during draw
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const GLVAOGroup& asVAOGroup() const;

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    struct Impl;
    za::InPlacePImpl<Impl, 128> m_impl; //!< Implementation details (PImpl)
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::VAOHandle
/// \ingroup graphics
///
/// `za::VAOHandle` is the user-facing wrapper around an
/// OpenGL Vertex Array Object. It is used together with
/// `za::DrawInstancedVerticesSettings` and
/// `za::DrawInstancedIndexedVerticesSettings` to drive the
/// instanced rendering path of `za::RenderTarget`. Per-instance
/// data lives in user-managed `za::VBOHandle` objects, which are
/// passed to `za::InstanceAttributeBinder::uploadData` from
/// inside the `setupFn` callback that the draw functions take.
///
/// `VAOHandle` is move-only and should be cached -- recreating
/// it every frame defeats the entire point of instanced
/// rendering.
///
/// \see `za::RenderTarget`, `za::InstanceAttributeBinder`,
///      `za::VBOHandle`, `za::DrawInstancedVerticesSettings`,
///      `za::DrawInstancedIndexedVerticesSettings`
///
////////////////////////////////////////////////////////////
