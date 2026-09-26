#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Export.hpp"

#include "Zancle/Graphics/GlDataType.hpp"

#include "Zancle/Container/InPlaceVector.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/SizeT.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
class VBOHandle;

template <typename T>
struct Vec2;

template <typename T>
struct Vec3;

struct Color;
} // namespace za


namespace za::priv
{
////////////////////////////////////////////////////////////
// GL field info deduction
////////////////////////////////////////////////////////////
struct FieldInfo
{
    unsigned int size;
    GlDataType   type;
    bool         defaultNormalized;
};

template <typename>
extern FieldInfo fieldInfo;

// clang-format off
template <> inline constexpr FieldInfo fieldInfo<float>          = {1, GlDataType::Float, false};
template <> inline constexpr FieldInfo fieldInfo<double>         = {1, GlDataType::Double, false};
template <> inline constexpr FieldInfo fieldInfo<int>            = {1, GlDataType::Int, false};
template <> inline constexpr FieldInfo fieldInfo<unsigned int>   = {1, GlDataType::UnsignedInt, false};
template <> inline constexpr FieldInfo fieldInfo<short>          = {1, GlDataType::Short, false};
template <> inline constexpr FieldInfo fieldInfo<unsigned short> = {1, GlDataType::UnsignedShort, false};
template <> inline constexpr FieldInfo fieldInfo<signed char>    = {1, GlDataType::Byte, false};
template <> inline constexpr FieldInfo fieldInfo<unsigned char>  = {1, GlDataType::UnsignedByte, false};
template <> inline constexpr FieldInfo fieldInfo<Vec2<float>>    = {2, GlDataType::Float, false};
template <> inline constexpr FieldInfo fieldInfo<Vec2<int>>      = {2, GlDataType::Int, false};
template <> inline constexpr FieldInfo fieldInfo<Vec3<float>>    = {3, GlDataType::Float, false};
template <> inline constexpr FieldInfo fieldInfo<Vec3<int>>      = {3, GlDataType::Int, false};
template <> inline constexpr FieldInfo fieldInfo<Color>          = {4, GlDataType::UnsignedByte, true};
// clang-format on

} // namespace za::priv


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Helper used to bind per-instance attribute streams during instanced draws
///
/// `InstanceAttributeBinder` is the parameter type of the
/// `setupFn` callback that
/// `za::RenderTarget::drawInstancedVertices` and
/// `za::RenderTarget::drawInstancedIndexedVertices` invoke right
/// before issuing the draw call. Inside the callback, you bind
/// a VBO holding the per-instance data and configure one or
/// more vertex attribute streams that pull from it.
///
/// Attribute setup is intentionally deferred until the callback
/// completes. Streaming uploads may grow a `VBOHandle` and replace
/// its underlying GL buffer object; applying `glVertexAttribPointer`
/// immediately would capture the old buffer in the VAO and make the
/// subsequent draw read invalid state.
///
/// The binder is non-copyable and non-movable: it must be used
/// only within the scope of the callback that received it.
///
////////////////////////////////////////////////////////////
struct [[nodiscard]] ZA_GRAPHICS_API InstanceAttributeBinder
{
    ////////////////////////////////////////////////////////////
    explicit InstanceAttributeBinder(za::SizeT instanceCount);
    ~InstanceAttributeBinder();

    ////////////////////////////////////////////////////////////
    InstanceAttributeBinder(const InstanceAttributeBinder&)            = delete;
    InstanceAttributeBinder& operator=(const InstanceAttributeBinder&) = delete;

    ////////////////////////////////////////////////////////////
    InstanceAttributeBinder(InstanceAttributeBinder&&)            = delete;
    InstanceAttributeBinder& operator=(InstanceAttributeBinder&&) = delete;

    ////////////////////////////////////////////////////////////
    /// \brief Upload per-instance data into the given VBO
    ///
    /// Streams `instanceCount * stride` bytes from `data` into
    /// `vboHandle` and makes that uploaded range the source for
    /// subsequent `setup*()` calls until the next upload.
    ///
    /// \param vboHandle VBO to upload into
    /// \param data      Pointer to the source bytes
    /// \param stride    Size, in bytes, of one per-instance record
    ///
    ////////////////////////////////////////////////////////////
    void uploadData(VBOHandle& vboHandle, const void* data, za::SizeT stride);

    ////////////////////////////////////////////////////////////
    /// \brief Type-safe wrapper around `uploadData` for contiguous arrays
    ///
    /// Equivalent to `uploadData(vboHandle, data, sizeof(T))`.
    ///
    /// \tparam T         Per-instance record type
    /// \param vboHandle  VBO to upload into
    /// \param data       Pointer to the contiguous per-instance data
    ///
    ////////////////////////////////////////////////////////////
    template <typename T>
    void uploadContiguousData(VBOHandle& vboHandle, const T* const data)
    {
        uploadData(vboHandle, data, sizeof(T));
    }

    ////////////////////////////////////////////////////////////
    /// \brief Type-safe wrapper around `uploadData` for contiguous arrays
    ///
    /// Equivalent to `uploadContiguousData(vboHandle, range.data())`.
    /// Asserts that `range.size() >= instanceCount`.
    ///
    /// \tparam T         Per-instance record type
    /// \param vboHandle  VBO to upload into
    /// \param range      Reference to the contiguous per-instance data
    ///
    ////////////////////////////////////////////////////////////
    template <typename T>
    void uploadContiguousData(VBOHandle& vboHandle, const T& range)
        requires requires {
            range.data();
            range.size();
        }
    {
        ZA_ASSERT(range.size() >= m_instanceCount);
        uploadContiguousData(vboHandle, range.data());
    }

    ////////////////////////////////////////////////////////////
    /// \brief Configure a per-instance vertex attribute stream
    ///
    /// Calls `glVertexAttribPointer` and `glVertexAttribDivisor`
    /// (with divisor `1`) to set up an attribute that advances
    /// once per instance.
    ///
    /// The setup is recorded immediately but applied only after the
    /// callback completes, once all uploads for the draw are finalized.
    ///
    /// \param location    Shader attribute location to bind to
    /// \param size        Number of components (1-4)
    /// \param type        Data type of each component
    /// \param normalized  Whether integer types should be normalized to `[0, 1]` / `[-1, 1]`
    /// \param stride      Byte stride between consecutive instances
    /// \param fieldOffset Byte offset of this attribute within a single instance record
    ///
    ////////////////////////////////////////////////////////////
    void setup(unsigned int location, unsigned int size, GlDataType type, bool normalized, za::SizeT stride, za::SizeT fieldOffset);

    ////////////////////////////////////////////////////////////
    /// \brief Configure a per-instance attribute from a pointer-to-member
    ///
    /// Deduces the GL data type, component count, stride, and
    /// field offset automatically from the pointer-to-member.
    ///
    /// Supported field types: `float`, `double`,
    /// `int`, `unsigned int`, `short`, `unsigned short`,
    /// `signed char`, `unsigned char`,
    /// `Vec2<float>`, `Vec2<int>`, `Vec3<float>`, `Vec3<int>`,
    /// `Color` (4x `UnsignedByte`, normalized).
    ///
    /// \tparam MemberPtr Pointer-to-member (e.g. `&MyStruct::position`)
    ///
    /// \param location Shader attribute location to bind to
    ///
    ////////////////////////////////////////////////////////////
    template <auto MemberPtr>
    void setupField(const unsigned int location)
    {
        using StructType = MemberPtrStructType<decltype(MemberPtr)>;
        using FieldType  = MemberPtrFieldType<decltype(MemberPtr)>;

        constexpr auto glInfo = priv::fieldInfo<FieldType>;
        setup(location, glInfo.size, glInfo.type, glInfo.defaultNormalized, sizeof(StructType), memberOffset<MemberPtr>());
    }

    ////////////////////////////////////////////////////////////
    /// \brief Configure a per-instance attribute from a plain type
    ///
    /// For SOA layouts where each VBO contains a flat array of
    /// a single type. Deduces GL data type and component count
    /// from `T`, uses `sizeof(T)` as stride and `0` as offset.
    ///
    /// \tparam T Element type (e.g. `Vec2f`, `float`)
    ///
    /// \param location Shader attribute location to bind to
    ///
    ////////////////////////////////////////////////////////////
    template <typename T>
    void setupFlat(const unsigned int location)
    {
        constexpr auto glInfo = priv::fieldInfo<T>;
        setup(location, glInfo.size, glInfo.type, glInfo.defaultNormalized, sizeof(T), 0);
    }

    ////////////////////////////////////////////////////////////
    /// \brief Apply any deferred attribute setups before the draw
    ///
    /// `RenderTarget` calls this after the setup callback has finished,
    /// once all streaming uploads for the draw are known. This is required
    /// because a later upload can grow a VBO and replace its GL buffer
    /// object, which would invalidate attribute pointers configured earlier.
    ///
    ////////////////////////////////////////////////////////////
    void applySetups() const;

    ////////////////////////////////////////////////////////////
    /// \brief Mark that the draw consuming the staged uploads has been submitted
    ///
    /// `RenderTarget` calls this after issuing the instanced draw. The
    /// destructor then commits the staged uploads instead of rolling them
    /// back, ensuring the fence is created after the draw.
    ///
    ////////////////////////////////////////////////////////////
    void markDrawSubmitted() noexcept;

private:
    ////////////////////////////////////////////////////////////
    // Pointer-to-member decomposition
    ////////////////////////////////////////////////////////////
    template <typename>
    struct MemberPtrTraits;

    template <typename S, typename F>
    struct MemberPtrTraits<F S::*>
    {
        using Struct = S;
        using Field  = F;
    };

    template <typename MPtr>
    using MemberPtrStructType = typename MemberPtrTraits<MPtr>::Struct;

    template <typename MPtr>
    using MemberPtrFieldType = typename MemberPtrTraits<MPtr>::Field;

    ////////////////////////////////////////////////////////////
    // Offset of a member within its struct
    ////////////////////////////////////////////////////////////
    template <auto MPtr>
    [[nodiscard, gnu::const]] static za::SizeT memberOffset()
    {
        using S = MemberPtrStructType<decltype(MPtr)>;

        alignas(S) char storage[sizeof(S)];

        auto* const obj = reinterpret_cast<S*>(storage);
        return static_cast<za::SizeT>(reinterpret_cast<const char*>(&(obj->*MPtr)) - reinterpret_cast<const char*>(obj));
    }

    ////////////////////////////////////////////////////////////
    struct DeferredSetup
    {
        VBOHandle*   vboHandle;
        unsigned int location;
        unsigned int size;
        GlDataType   type;
        bool         normalized;
        za::SizeT    stride;
        za::SizeT    byteOffset;
    };

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    za::SizeT                             m_instanceCount;
    za::InPlaceVector<VBOHandle*, 8u>     m_touchedVBOHandles;
    za::InPlaceVector<DeferredSetup, 16u> m_deferredSetups;
    VBOHandle*                            m_currentVBOHandle{nullptr};
    za::SizeT                             m_currentUploadByteOffset{0u};
    bool                                  m_drawSubmitted{false};
};

} // namespace za


////////////////////////////////////////////////////////////
/// \struct za::InstanceAttributeBinder
/// \ingroup graphics
///
/// `za::InstanceAttributeBinder` is the configuration helper
/// passed to the `setupFn` callback of
/// `za::RenderTarget::drawInstancedVertices` and
/// `za::RenderTarget::drawInstancedIndexedVertices`. It exposes
/// the minimal API needed to bind a per-instance VBO, upload
/// per-instance data, and declare the vertex attribute streams
/// that will read from it during the upcoming instanced draw
/// call.
///
/// \see `za::RenderTarget`, `za::DrawInstancedVerticesSettings`,
///      `za::DrawInstancedIndexedVerticesSettings`,
///      `za::VAOHandle`, `za::VBOHandle`
///
////////////////////////////////////////////////////////////
