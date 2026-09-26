#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Graphics/Export.hpp"

#include "Zancle/Graphics/Color.hpp"

#include "Zancle/Container/Vector.hpp"

#include "Zancle/Geometry/Priv/Vec2Base.hpp"
#include "Zancle/Geometry/Rect2.hpp"

#include "Zancle/Vocabulary/Optional.hpp"
#include "Zancle/Vocabulary/PassKey.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/SizeT.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
class InputStream;
class Path;
} // namespace za


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Class for loading, manipulating and saving images
///
////////////////////////////////////////////////////////////
class ZA_GRAPHICS_API Image
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Image encoding formats supported by `saveToMemory` / `saveToFile`
    ///
    ////////////////////////////////////////////////////////////
    enum class [[nodiscard]] SaveFormat : unsigned char
    {
        BMP, //!< Windows Bitmap (uncompressed)
        TGA, //!< Truevision TGA (lossless)
        PNG, //!< Portable Network Graphics (lossless, compressed)
        JPG, //!< JPEG (lossy, compressed)
        QOI  //!< Quite OK Image format (lossless, fast)
    };

    ////////////////////////////////////////////////////////////
    /// \brief Create an image filled with a uniform color
    ///
    /// Both `size.x` and `size.y` must be greater than zero.
    ///
    /// \param size  Width and height of the image, in pixels
    /// \param color Fill color (defaults to opaque black)
    ///
    /// \return Image on success, `za::nullOpt` on failure (e.g. zero size)
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<Image> create(Vec2u size, Color color = Color::Black);

    ////////////////////////////////////////////////////////////
    /// \brief Create an image from an existing array of pixels
    ///
    /// `pixels` must point to an array of `size.x * size.y` 32-bit
    /// RGBA pixels. The data is copied into the image; the source
    /// array can be freed after the call.
    ///
    /// If `pixels` is `nullptr`, the function returns `za::nullOpt`.
    ///
    /// \param size   Width and height of the image, in pixels
    /// \param pixels Source pixel array (RGBA, 8 bits per channel)
    ///
    /// \return Image on success, `za::nullOpt` on failure
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<Image> create(Vec2u size, const za::U8* pixels);

    ////////////////////////////////////////////////////////////
    /// \brief Load an image from a file on disk
    ///
    /// The supported image formats are bmp, png, tga, jpg, gif,
    /// psd, hdr, pic, pnm, and qoi. Some format options are not
    /// supported, such as JPEG with arithmetic coding or ASCII PNM.
    ///
    /// \param filename Path of the image file to load
    ///
    /// \return Image on success, `za::nullOpt` on failure
    ///
    /// \see `loadFromMemory`, `loadFromStream`, `saveToFile`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<Image> loadFromFile(const Path& filename);

    ////////////////////////////////////////////////////////////
    /// \brief Load an image from an encoded buffer in memory
    ///
    /// Same supported formats as `loadFromFile`. Useful when image
    /// data is bundled into the application binary or received over
    /// a network.
    ///
    /// \param data Pointer to the encoded file bytes in memory
    /// \param size Size of the data, in bytes
    ///
    /// \return Image on success, `za::nullOpt` on failure
    ///
    /// \see `loadFromFile`, `loadFromStream`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<Image> loadFromMemory(const void* data, za::SizeT size);

    ////////////////////////////////////////////////////////////
    /// \brief Load an image from a custom input stream
    ///
    /// Same supported formats as `loadFromFile`. Lets you plug in
    /// arbitrary sources (archives, encrypted blobs, ...).
    ///
    /// \param stream Source stream to read encoded image data from
    ///
    /// \return Image on success, `za::nullOpt` on failure
    ///
    /// \see `loadFromFile`, `loadFromMemory`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<Image> loadFromStream(InputStream& stream);

    ////////////////////////////////////////////////////////////
    /// \brief Return the size (width and height) of the image
    ///
    /// \return Size of the image, in pixels
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Vec2u getSize() const;

    ////////////////////////////////////////////////////////////
    /// \brief Create a transparency mask from a specified color-key
    ///
    /// This function sets the alpha value of every pixel matching
    /// the given color to `alpha` (0 by default), so that they
    /// become transparent.
    ///
    /// \param color Color to make transparent
    /// \param alpha Alpha value to assign to transparent pixels
    ///
    ////////////////////////////////////////////////////////////
    void createMaskFromColor(Color color, za::U8 alpha = 0);

    ////////////////////////////////////////////////////////////
    /// \brief Copy pixels from another image onto this one
    ///
    /// This function does a slow pixel copy and should not be
    /// used intensively. It can be used to prepare a complex
    /// static image from several others, but if you need this
    /// kind of feature in real-time you'd better use `za::RenderTexture`.
    ///
    /// If `sourceRect` is empty, the whole image is copied.
    /// If `applyAlpha` is set to `true`, alpha blending is
    /// applied from the source pixels to the destination pixels
    /// using the \b over operator. If it is `false`, the source
    /// pixels are copied unchanged with their alpha value.
    ///
    /// See https://en.wikipedia.org/wiki/Alpha_compositing for
    /// details on the \b over operator.
    ///
    /// Note that this function can fail if either image is invalid
    /// (i.e. zero-sized width or height), if `sourceRect` has a
    /// negative component or does not fit within the boundaries of
    /// the `source` parameter, or if the destination position is out
    /// of the boundaries of this image. If the source region extends
    /// past this image's bounds, the copy is clipped to fit instead
    /// of failing.
    ///
    /// `source` may be `*this` only if the source and destination
    /// regions do not overlap (debug-asserted).
    ///
    /// On failure, the destination image is left unchanged.
    ///
    /// \param source     Source image to copy
    /// \param dest       Coordinates of the destination position
    /// \param sourceRect Sub-rectangle of the source image to copy
    /// \param applyAlpha Should the copy take into account the source transparency?
    ///
    /// \return `true` if the operation was successful, `false` otherwise
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool copy(const Image& source, Vec2u dest, const Rect2i& sourceRect = {}, bool applyAlpha = false);

    ////////////////////////////////////////////////////////////
    /// \brief Change the color of a pixel
    ///
    /// This function doesn't check the validity of the pixel
    /// coordinates, using out-of-range values will result in
    /// an undefined behavior.
    ///
    /// \param coords Coordinates of pixel to change
    /// \param color  New color of the pixel
    ///
    /// \see `getPixel`
    ///
    ////////////////////////////////////////////////////////////
    void setPixel(Vec2u coords, Color color);

    ////////////////////////////////////////////////////////////
    /// \brief Get the color of a pixel
    ///
    /// This function doesn't check the validity of the pixel
    /// coordinates, using out-of-range values will result in
    /// an undefined behavior.
    ///
    /// \param coords Coordinates of pixel to change
    ///
    /// \return Color of the pixel at given coordinates
    ///
    /// \see `setPixel`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] Color getPixel(Vec2u coords) const;

    ////////////////////////////////////////////////////////////
    /// \brief Get a read-only pointer to the array of pixels
    ///
    /// The returned value points to an array of RGBA pixels made of
    /// 8 bit integer components. The size of the array is
    /// `width * height * 4 (getSize().x * getSize().y * 4)`.
    /// Warning: the returned pointer may become invalid if you
    /// modify the image, so you should never store it for too long.
    /// If the image is empty, a null pointer is returned.
    ///
    /// \return Read-only pointer to the array of pixels
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] const za::U8* getPixelsPtr() const;

    ////////////////////////////////////////////////////////////
    /// \brief Flip the image horizontally (left <-> right)
    ///
    ////////////////////////////////////////////////////////////
    void flipHorizontally();

    ////////////////////////////////////////////////////////////
    /// \brief Flip the image vertically (top <-> bottom)
    ///
    ////////////////////////////////////////////////////////////
    void flipVertically();

    ////////////////////////////////////////////////////////////
    /// \brief Apply a custom transformation to each pixel of the image
    ///
    /// This function iterates over all the pixels of the image and
    /// applies the provided callable `f` to each one.
    ///
    /// The callable `f` must take three arguments: the x-coordinate
    /// of the pixel (`unsigned int`), the y-coordinate of the pixel
    /// (`unsigned int`), and the current `za::Color` of the pixel.
    ///
    /// It must return an `za::Color` which will be the new color of that pixel.
    ///
    /// \param f A callable (e.g., lambda function) that takes
    ///          `(unsigned int x, unsigned int y, za::Color color)`
    ///          and returns `za::Color`.
    ///
    ////////////////////////////////////////////////////////////
    template <typename Func>
    void applyTransformation(Func&& f)
    {
        ZA_ASSERT(!m_pixels.empty());

        za::U8* ptr = m_pixels.data();

        for (unsigned int y = 0u; y < m_size.y; ++y)
            for (unsigned int x = 0u; x < m_size.x; ++x)
            {
                const Color currentColor{ptr[0], ptr[1], ptr[2], ptr[3]};
                const auto [newR, newG, newB, newA] = f(x, y, currentColor);

                *ptr++ = newR;
                *ptr++ = newG;
                *ptr++ = newB;
                *ptr++ = newA;
            }
    }

    ////////////////////////////////////////////////////////////
    /// \brief Rotate the hue of the image
    ///
    /// This function applies a hue rotation to each pixel of the image.
    ///
    /// The rotation is done in degrees, and the value must be
    /// between 0 and 360. A value of 0 means no rotation, while
    /// a value of 360 means a full rotation (which is equivalent to 0).
    /// The rotation is done in the HSL color space.
    ///
    /// The function uses the `Color::withRotatedHue` method to
    /// perform the rotation.
    ///
    /// \param degrees The angle in degrees to rotate the hue.
    ///
    ////////////////////////////////////////////////////////////
    void rotateHue(float degrees);

    ////////////////////////////////////////////////////////////
    /// \brief Save the image to a file on disk
    ///
    /// The output format is deduced from the file extension. The
    /// supported extensions are `.bmp`, `.png`, `.tga`, `.jpg`, and
    /// `.qoi`. The destination file is overwritten if it already
    /// exists.
    ///
    /// \param filename Path of the file to write
    ///
    /// \return `true` on success, `false` on failure (e.g. unknown extension)
    ///
    /// \see `create`, `loadFromFile`, `loadFromMemory`, `saveToMemory`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool saveToFile(const Path& filename) const;

    ////////////////////////////////////////////////////////////
    /// \brief Encode the image into an in-memory buffer
    ///
    /// The encoding format must be specified explicitly. See
    /// `SaveFormat` for the available options. The behavior is
    /// undefined if `format` is not a valid `SaveFormat` value.
    ///
    /// \param format Encoding format to use
    ///
    /// \return Encoded byte buffer; empty on failure
    ///
    /// \see `SaveFormat`, `saveToFile`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::Vector<za::U8> saveToMemory(SaveFormat format) const;

    ////////////////////////////////////////////////////////////
    /// \private
    ///
    /// \brief Directly initialize data members
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit Image(za::PassKey<Image>&&, Vec2u size, za::SizeT pixelCount);
    [[nodiscard]] explicit Image(za::PassKey<Image>&&, Vec2u size, const za::U8* itBegin, const za::U8* itEnd);

private:
    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    Vec2u              m_size;   //!< Image size
    za::Vector<za::U8> m_pixels; //!< Pixels of the image
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::Image
/// \ingroup graphics
///
/// `za::Image` is an abstraction to manipulate images
/// as bi-dimensional arrays of pixels. The class provides
/// functions to load, read, write and save pixels, as well
/// as many other useful functions.
///
/// `za::Image` can handle a unique internal representation of
/// pixels, which is RGBA 32 bits. This means that a pixel
/// must be composed of 8 bit red, green, blue and alpha
/// channels -- just like a `za::Color`.
/// All the functions that return an array of pixels follow
/// this rule, and all parameters that you pass to `za::Image`
/// functions (such as `loadFromMemory`) must use this
/// representation as well.
///
/// A `za::Image` can be copied, but it is a heavy resource and
/// if possible you should always use [const] references to
/// pass or return them to avoid useless copies.
///
/// Usage example:
/// \code
/// // Load an image file from disk.
/// const auto background = za::Image::loadFromFile("background.jpg").value();
///
/// // Create a 20x20 image filled with black.
/// auto image = za::Image::create({20u, 20u}, za::Color::Black).value();
///
/// // Copy 'background' onto 'image' at position (10, 10).
/// if (!image.copy(background, {10u, 10u}))
///     return -1;
///
/// // Make the top-left pixel transparent.
/// za::Color color = image.getPixel({0u, 0u});
/// color.a = 0u;
/// image.setPixel({0u, 0u}, color);
///
/// // Save the image to a file.
/// if (!image.saveToFile("result.png"))
///     return -1;
/// \endcode
///
/// \see `za::Texture`
///
////////////////////////////////////////////////////////////
