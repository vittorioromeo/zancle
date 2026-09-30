// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/IO/FileInputStream.hpp"

#include "Zancle/Vocabulary/Optional.hpp"
#include "Zancle/Vocabulary/PassKey.hpp"
#include "Zancle/Vocabulary/UniquePtr.hpp"

#ifdef ZA_SYSTEM_ANDROID
    #include "Zancle/Window/Android/Activity.hpp"
    #include "Zancle/Window/Android/ResourceStream.hpp"
#endif

#include "Zancle/IO/FileUtils.hpp"
#include "Zancle/IO/Path.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/Exchange.hpp"
#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/Macros.hpp"
#include "Zancle/Base/SizeT.hpp"

#include <cstdio>


namespace
{
////////////////////////////////////////////////////////////
[[nodiscard, gnu::always_inline]] inline std::FILE* asFile(void* const file) noexcept
{
    return static_cast<std::FILE*>(file);
}

} // namespace


namespace za
{
////////////////////////////////////////////////////////////
FileInputStream::~FileInputStream()
{
    if (m_file != nullptr)
        std::fclose(asFile(m_file));
}


////////////////////////////////////////////////////////////
FileInputStream::FileInputStream(FileInputStream&& rhs) noexcept :
#ifdef ZA_SYSTEM_ANDROID
    m_androidFile(ZA_MOVE(rhs.m_androidFile)),
#endif
    m_file(za::exchange(rhs.m_file, nullptr))
{
}


////////////////////////////////////////////////////////////
FileInputStream& FileInputStream::operator=(FileInputStream&& rhs) noexcept
{
    if (&rhs == this)
        return *this;

    if (m_file != nullptr)
        std::fclose(asFile(m_file));

#ifdef ZA_SYSTEM_ANDROID
    m_androidFile = ZA_MOVE(rhs.m_androidFile);
#endif

    m_file = za::exchange(rhs.m_file, nullptr);
    return *this;
}


////////////////////////////////////////////////////////////
za::Optional<FileInputStream> FileInputStream::open(const Path& filename)
{
#ifdef ZA_SYSTEM_ANDROID
    if (priv::getActivityStatesPtr() != nullptr)
    {
        auto androidFile = za::makeUnique<priv::ResourceStream>();
        if (!androidFile->open(filename))
            return za::nullOpt;

        return androidFile->tell().hasValue()
                   ? za::makeOptional<FileInputStream>(za::PassKey<FileInputStream>{}, ZA_MOVE(androidFile))
                   : za::nullOpt;
    }
#endif

    if (std::FILE* const file = openFile(filename, "rb"))
        return za::makeOptional<FileInputStream>(za::PassKey<FileInputStream>{}, static_cast<void*>(file));

    return za::nullOpt;
}


////////////////////////////////////////////////////////////
za::Optional<za::SizeT> FileInputStream::read(void* data, za::SizeT size)
{
#ifdef ZA_SYSTEM_ANDROID
    if (m_androidFile != nullptr)
    {
        return m_androidFile->read(data, size);
    }
#endif

    ZA_ASSERT(m_file != nullptr);

    // A short read is either the end of the file (not an error) or an I/O error
    const za::SizeT count = std::fread(data, 1u, size, asFile(m_file));
    if (count != size && std::ferror(asFile(m_file)) != 0)
        return za::nullOpt;

    return za::makeOptional(count);
}


////////////////////////////////////////////////////////////
za::Optional<za::SizeT> FileInputStream::seek(za::SizeT position)
{
#ifdef ZA_SYSTEM_ANDROID
    if (m_androidFile != nullptr)
    {
        return m_androidFile->seek(position);
    }
#endif

    ZA_ASSERT(m_file != nullptr);

    if (static_cast<za::I64>(position) < 0 || !seekFile(asFile(m_file), static_cast<za::I64>(position), SEEK_SET))
        return za::nullOpt;

    return tell();
}


////////////////////////////////////////////////////////////
za::Optional<za::SizeT> FileInputStream::tell()
{
#ifdef ZA_SYSTEM_ANDROID
    if (m_androidFile != nullptr)
    {
        return m_androidFile->tell();
    }
#endif

    ZA_ASSERT(m_file != nullptr);

    const za::I64 position = tellFile(asFile(m_file));

    // `SizeT` may be 32-bit
    if (position < 0 || static_cast<za::I64>(static_cast<za::SizeT>(position)) != position)
        return za::nullOpt;

    return za::makeOptional(static_cast<za::SizeT>(position));
}


////////////////////////////////////////////////////////////
za::Optional<za::SizeT> FileInputStream::getSize()
{
#ifdef ZA_SYSTEM_ANDROID
    if (m_androidFile != nullptr)
    {
        return m_androidFile->getSize();
    }
#endif

    ZA_ASSERT(m_file != nullptr);

    const za::Optional<za::SizeT> position = tell();
    if (!position.hasValue() || !seekFile(asFile(m_file), 0, SEEK_END))
        return za::nullOpt;

    za::Optional<za::SizeT> size = tell(); // Use a single local variable for NRVO

    if (!seek(*position).hasValue())
        size.reset();

    return size;
}


////////////////////////////////////////////////////////////
FileInputStream::FileInputStream(za::PassKey<FileInputStream>&&, void* const file) noexcept : m_file(file)
{
}


////////////////////////////////////////////////////////////
#ifdef ZA_SYSTEM_ANDROID
FileInputStream::FileInputStream(za::PassKey<FileInputStream>&&, za::UniquePtr<priv::ResourceStream>&& androidFile) :
    m_androidFile(ZA_MOVE(androidFile))
{
}
#endif

} // namespace za
