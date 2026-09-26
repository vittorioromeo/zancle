#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Vocabulary/InPlacePImpl.hpp"

#include "Zancle/Base/SizeT.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
struct InstanceAttributeBinder;
} // namespace za


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Owning handle to an OpenGL Vertex Buffer Object
///
/// `za::VBOHandle` wraps a single OpenGL VBO. It is passed to
/// `za::InstanceAttributeBinder::uploadData` inside the
/// `setupFn` callback of an instanced draw call, which uploads
/// per-instance data into it. On desktop GL, streaming uploads may
/// grow and replace the underlying buffer object, which is why
/// `InstanceAttributeBinder` defers the final `glVertexAttribPointer`
/// calls until every upload for the draw is complete.
///
/// `VBOHandle` is non-copyable and non-movable (it cannot live in
/// relocatable containers); cache it across frames, e.g. behind a
/// `za::UniquePtr` or as a long-lived member.
///
////////////////////////////////////////////////////////////
class [[nodiscard]] VBOHandle
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Default constructor
    ///
    /// Creates a fresh OpenGL VBO with no data.
    ///
    ////////////////////////////////////////////////////////////
    VBOHandle();

    ////////////////////////////////////////////////////////////
    /// \brief Destructor
    ///
    /// Releases the underlying OpenGL VBO.
    ///
    ////////////////////////////////////////////////////////////
    ~VBOHandle();

    ////////////////////////////////////////////////////////////
    VBOHandle(const VBOHandle&)            = delete;
    VBOHandle& operator=(const VBOHandle&) = delete;

    ////////////////////////////////////////////////////////////
    VBOHandle(VBOHandle&&) noexcept            = delete;
    VBOHandle& operator=(VBOHandle&&) noexcept = delete;

private:
    friend InstanceAttributeBinder;

    ////////////////////////////////////////////////////////////
    /// \brief Internal helper that binds the VBO as `GL_ARRAY_BUFFER`
    ///
    ////////////////////////////////////////////////////////////
    void bind();

    ////////////////////////////////////////////////////////////
    /// \brief Internal helper for per-frame streaming uploads
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::SizeT uploadStreamingData(const void* data, za::SizeT byteCount);

    ////////////////////////////////////////////////////////////
    /// \brief Commit the staged uploads associated with the current draw
    ///
    ////////////////////////////////////////////////////////////
    void commitPendingUploads();

    ////////////////////////////////////////////////////////////
    /// \brief Roll back staged uploads when a draw never gets submitted
    ///
    ////////////////////////////////////////////////////////////
    void rollbackPendingUploads() noexcept;

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    struct Impl;
    za::InPlacePImpl<Impl, 320> m_impl; //!< Implementation details (PImpl)
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::VBOHandle
/// \ingroup graphics
///
/// `za::VBOHandle` is the user-facing wrapper around an
/// OpenGL Vertex Buffer Object. It is the storage that
/// `za::InstanceAttributeBinder::uploadData` writes per-instance
/// data into and that `za::InstanceAttributeBinder::setup`
/// then reads from.
///
/// `VBOHandle` is non-copyable and non-movable; cache it across
/// frames to avoid unnecessary OpenGL allocations.
///
/// \see `za::VAOHandle`, `za::InstanceAttributeBinder`,
///      `za::RenderTarget`
///
////////////////////////////////////////////////////////////
