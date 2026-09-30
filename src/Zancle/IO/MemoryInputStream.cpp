// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/IO/MemoryInputStream.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Math/MinMax.hpp"

#include "Zancle/Base/Assert.hpp"
#include "Zancle/Base/Memcpy.hpp"
#include "Zancle/Base/SizeT.hpp"


namespace za
{
////////////////////////////////////////////////////////////
MemoryInputStream::MemoryInputStream(const void* data, za::SizeT sizeInBytes) :
    m_data(static_cast<const unsigned char*>(data)),
    m_size(sizeInBytes)
{
    ZA_ASSERT((m_data != nullptr || m_size == 0u) && "MemoryInputStream must be initialized with non-null data");
}


////////////////////////////////////////////////////////////
za::Optional<za::SizeT> MemoryInputStream::read(void* data, za::SizeT size)
{
    const za::SizeT count = za::min(size, m_size - m_offset);

    if (count > 0)
    {
        ZA_MEMCPY(data, m_data + m_offset, static_cast<za::SizeT>(count));
        m_offset += count;
    }

    return za::makeOptional(count);
}


////////////////////////////////////////////////////////////
za::Optional<za::SizeT> MemoryInputStream::seek(za::SizeT position)
{
    m_offset = position < m_size ? position : m_size;
    return za::makeOptional(m_offset);
}


////////////////////////////////////////////////////////////
za::Optional<za::SizeT> MemoryInputStream::tell()
{
    return za::makeOptional(m_offset);
}


////////////////////////////////////////////////////////////
za::Optional<za::SizeT> MemoryInputStream::getSize()
{
    return za::makeOptional(m_size);
}

} // namespace za
