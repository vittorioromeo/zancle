// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Network/Packet.hpp"

#include "Zancle/String/String.hpp"
#include "Zancle/String/Utf8String.hpp"

#include "Zancle/Base/AssertAndAssume.hpp"
#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/Memcpy.hpp"
#include "Zancle/Base/SizeT.hpp"

#include <string>


namespace za
{
////////////////////////////////////////////////////////////
Packet& Packet::append(const void* data, za::SizeT sizeInBytes)
{
    ZA_ASSERT_AND_ASSUME(data != nullptr);
    ZA_ASSERT_AND_ASSUME(sizeInBytes > 0);

    m_data.emplaceBackRange(reinterpret_cast<const unsigned char*>(data), sizeInBytes);
    return *this;
}


////////////////////////////////////////////////////////////
za::SizeT Packet::getReadPosition() const
{
    return m_readPos;
}


////////////////////////////////////////////////////////////
void Packet::clear()
{
    m_data.clear();
    m_readPos = 0;
    m_isValid = true;
}


////////////////////////////////////////////////////////////
const void* Packet::getData() const
{
    return !m_data.empty() ? m_data.data() : nullptr;
}


////////////////////////////////////////////////////////////
za::SizeT Packet::getDataSize() const
{
    return m_data.size();
}


////////////////////////////////////////////////////////////
bool Packet::endOfPacket() const
{
    return m_readPos >= m_data.size();
}


////////////////////////////////////////////////////////////
Packet::operator bool() const
{
    return m_isValid;
}


////////////////////////////////////////////////////////////
Packet& Packet::readBytes(void* dst, za::SizeT size)
{
    if (checkSize(size))
    {
        ZA_MEMCPY(dst, &m_data[m_readPos], size);
        m_readPos += size;
    }

    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator>>(bool& data)
{
    za::U8 value = 0;
    if (*this >> value)
        data = (value != 0);

    return *this;
}


////////////////////////////////////////////////////////////
// clang-format off
Packet& Packet::operator>>(za::I8& data)  { return readBytes(&data, sizeof(data)); }
Packet& Packet::operator>>(za::U8& data)  { return readBytes(&data, sizeof(data)); }
Packet& Packet::operator>>(za::I16& data) { return readBytes(&data, sizeof(data)); }
Packet& Packet::operator>>(za::U16& data) { return readBytes(&data, sizeof(data)); }
Packet& Packet::operator>>(za::I32& data) { return readBytes(&data, sizeof(data)); }
Packet& Packet::operator>>(za::U32& data) { return readBytes(&data, sizeof(data)); }
Packet& Packet::operator>>(za::I64& data) { return readBytes(&data, sizeof(data)); }
Packet& Packet::operator>>(za::U64& data) { return readBytes(&data, sizeof(data)); }
Packet& Packet::operator>>(float& data)     { return readBytes(&data, sizeof(data)); }
Packet& Packet::operator>>(double& data)    { return readBytes(&data, sizeof(data)); }
// clang-format on


////////////////////////////////////////////////////////////
Packet& Packet::operator>>(std::string& data)
{
    za::U32 length = 0;
    *this >> length;

    data.clear();
    if ((length > 0) && checkSize(length))
    {
        data.assign(reinterpret_cast<char*>(&m_data[m_readPos]), length);
        m_readPos += length;
    }

    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator>>(za::String& data)
{
    za::U32 length = 0;
    *this >> length;

    data.clear();
    if ((length > 0) && checkSize(length))
    {
        data.assign(reinterpret_cast<char*>(&m_data[m_readPos]), length);
        m_readPos += length;
    }

    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator>>(std::wstring& data)
{
    za::U32 length = 0;
    *this >> length;

    data.clear();

    // Bound length so `length * sizeof(U32)` cannot wrap on 32-bit SizeT;
    // also reject lengths larger than the remaining bytes in one shot.
    if (length > 0 && length <= remaining() / sizeof(za::U32))
    {
        // Per-element `checkSize` is unnecessary: the bound above already
        // guarantees `length * 4` bytes are available from `m_readPos`.
        data.reserve(length);
        for (za::U32 i = 0; i < length; ++i)
        {
            za::U32 character = 0;
            ZA_MEMCPY(&character, &m_data[m_readPos], sizeof(character));
            m_readPos += sizeof(character);
            data += static_cast<wchar_t>(character);
        }
    }
    else if (length > 0)
    {
        m_isValid = false;
    }

    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator>>(Utf8String& data)
{
    za::U32 length = 0;
    *this >> length;

    data.clear();
    if ((length > 0) && checkSize(length))
    {
        data.reserve(length);
        data.append(reinterpret_cast<const char*>(&m_data[m_readPos]), length);
        m_readPos += length;
    }

    return *this;
}


////////////////////////////////////////////////////////////
// clang-format off
Packet& Packet::operator<<(bool data)      { return *this << static_cast<za::U8>(data); }
Packet& Packet::operator<<(za::I8 data)  { return append(&data, sizeof(data)); }
Packet& Packet::operator<<(za::U8 data)  { return append(&data, sizeof(data)); }
Packet& Packet::operator<<(za::I16 data) { return append(&data, sizeof(data)); }
Packet& Packet::operator<<(za::U16 data) { return append(&data, sizeof(data)); }
Packet& Packet::operator<<(za::I32 data) { return append(&data, sizeof(data)); }
Packet& Packet::operator<<(za::U32 data) { return append(&data, sizeof(data)); }
Packet& Packet::operator<<(za::I64 data) { return append(&data, sizeof(data)); }
Packet& Packet::operator<<(za::U64 data) { return append(&data, sizeof(data)); }
Packet& Packet::operator<<(float data)     { return append(&data, sizeof(data)); }
Packet& Packet::operator<<(double data)    { return append(&data, sizeof(data)); }
// clang-format on


////////////////////////////////////////////////////////////
Packet& Packet::operator<<(const std::string& data)
{
    const auto length = static_cast<za::U32>(data.size());
    *this << length;

    if (length > 0)
        append(data.data(), length);

    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator<<(const za::String& data)
{
    const auto length = static_cast<za::U32>(data.size());
    *this << length;

    if (length > 0)
        append(data.data(), length);

    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator<<(const std::wstring& data)
{
    const auto length = static_cast<za::U32>(data.size());
    *this << length;

    if (length > 0)
    {
        m_data.reserveMore(length * sizeof(za::U32));
        for (const wchar_t c : data)
            *this << static_cast<za::U32>(c);
    }

    return *this;
}


////////////////////////////////////////////////////////////
Packet& Packet::operator<<(const Utf8String& data)
{
    const auto length = static_cast<za::U32>(data.byteSize());
    *this << length;

    if (length > 0)
        append(data.data(), length);

    return *this;
}


////////////////////////////////////////////////////////////
za::SizeT Packet::remaining() const
{
    // Invariant: every mutator of `m_readPos` advances only after
    // `checkSize` succeeds, so `m_readPos <= m_data.size()` always holds.
    return m_data.size() - m_readPos;
}


////////////////////////////////////////////////////////////
bool Packet::checkSize(za::SizeT size)
{
    // `remaining()` is bounded by `m_data.size()`, so `size <= remaining()`
    // cannot wrap and subsumes the previous manual overflow check.
    m_isValid = m_isValid && size <= remaining();
    return m_isValid;
}


////////////////////////////////////////////////////////////
za::SizeT& Packet::getSendPos()
{
    return m_sendPos;
}


////////////////////////////////////////////////////////////
const void* Packet::onSend(za::SizeT& size)
{
    size = getDataSize();
    return getData();
}


////////////////////////////////////////////////////////////
void Packet::onReceive(const void* data, za::SizeT size)
{
    append(data, size);
}

} // namespace za
