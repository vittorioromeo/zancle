#pragma once
// LICENSE AND COPYRIGHT (C) INFORMATION
// https://github.com/vittorioromeo/Zancle/blob/master/license.md


////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Zancle/Network/Export.hpp"

#include "Zancle/Chrono/Time.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Base/IntTypes.hpp"


////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////
namespace za
{
class IpAddressUtils;
} // namespace za


namespace za
{
////////////////////////////////////////////////////////////
/// \brief Encapsulate an IPv4 network address
///
////////////////////////////////////////////////////////////
class ZA_NETWORK_API IpAddress
{
public:
    ////////////////////////////////////////////////////////////
    /// \brief Construct the address from 4 bytes
    ///
    /// Calling `IpAddress(a, b, c, d)` is equivalent to calling
    /// `IpAddressUtils::resolve("a.b.c.d")`, but safer as it doesn't
    /// have to parse a string to get the address components.
    ///
    /// \param byte0 First byte of the address
    /// \param byte1 Second byte of the address
    /// \param byte2 Third byte of the address
    /// \param byte3 Fourth byte of the address
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] IpAddress(za::U8 byte0, za::U8 byte1, za::U8 byte2, za::U8 byte3);

    ////////////////////////////////////////////////////////////
    /// \brief Construct the address from a 32-bits integer
    ///
    /// This constructor uses the internal representation of
    /// the address directly. It should be used for optimization
    /// purposes, and only if you got that representation from
    /// `IpAddress::toInteger()`.
    ///
    /// \param address 4 bytes of the address packed into a 32-bits integer
    ///
    /// \see `toInteger`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] explicit IpAddress(za::U32 address);

    ////////////////////////////////////////////////////////////
    /// \brief Get an integer representation of the address
    ///
    /// The returned number is the internal representation of the
    /// address, and should be used for optimization purposes only
    /// (like sending the address through a socket).
    /// The integer produced by this function can then be converted
    /// back to a `za::IpAddress` with the proper constructor.
    ///
    /// \return 32-bits unsigned integer representation of the address
    ///
    /// \see `toString`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] za::U32 toInteger() const;

    ////////////////////////////////////////////////////////////
    /// \brief Get the computer's local address
    ///
    /// The local address is the address of the computer from the
    /// LAN point of view, i.e. something like 192.168.1.56. It is
    /// meaningful only for communications over the local network.
    /// Unlike getPublicAddress, this function is fast and may be
    /// used safely anywhere.
    ///
    /// \return Local IP address of the computer on success, `za::nullOpt` otherwise
    ///
    /// \see `getPublicAddress`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<IpAddress> getLocalAddress();

    ////////////////////////////////////////////////////////////
    /// \brief Get the computer's public address
    ///
    /// The public address is the address of the computer from the
    /// internet point of view, i.e. something like 89.54.1.169.
    /// It is necessary for communications over the world wide web.
    /// The only way to get a public address is to ask it to a
    /// distant website; as a consequence, this function depends on
    /// both your network connection and the server, and may be
    /// very slow. You should use it as few as possible. Because
    /// this function depends on the network connection and on a distant
    /// server, you may use a time limit if you don't want your program
    /// to be possibly stuck waiting in case there is a problem; this
    /// limit is deactivated by default.
    ///
    /// \param timeout Maximum time to wait
    ///
    /// \return Public IP address of the computer on success, `za::nullOpt` otherwise
    ///
    /// \see `getLocalAddress`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] static za::Optional<IpAddress> getPublicAddress(Time timeout = {});


    ////////////////////////////////////////////////////////////
    /// \brief Overload of `operator==` to compare two IP addresses
    ///
    /// \param lhs  Left operand (a IP address)
    /// \param rhs Right operand (a IP address)
    ///
    /// \return `true` if both addresses are equal
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool operator==(const IpAddress& rhs) const = default;


    ////////////////////////////////////////////////////////////
    /// \relates IpAddress
    /// \brief Overload of `operator<` to compare two IP addresses
    ///
    /// \param lhs  Left operand (a IP address)
    /// \param rhs Right operand (a IP address)
    ///
    /// \return `true` if `lhs` is lesser than `rhs`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] friend bool operator<(const IpAddress lhs, const IpAddress rhs)
    {
        return lhs.m_address < rhs.m_address;
    }


    ////////////////////////////////////////////////////////////
    /// \relates IpAddress
    /// \brief Overload of `operator>` to compare two IP addresses
    ///
    /// \param lhs  Left operand (a IP address)
    /// \param rhs Right operand (a IP address)
    ///
    /// \return `true` if `lhs` is greater than `rhs`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] friend bool operator>(const IpAddress lhs, const IpAddress rhs)
    {
        return lhs.m_address > rhs.m_address;
    }


    ////////////////////////////////////////////////////////////
    /// \relates IpAddress
    /// \brief Overload of `operator<=` to compare two IP addresses
    ///
    /// \param lhs  Left operand (a IP address)
    /// \param rhs Right operand (a IP address)
    ///
    /// \return `true` if `lhs` is lesser or equal than `rhs`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] friend bool operator<=(const IpAddress lhs, const IpAddress rhs)
    {
        return lhs.m_address <= rhs.m_address;
    }


    ////////////////////////////////////////////////////////////
    /// \relates IpAddress
    /// \brief Overload of `operator>=` to compare two IP addresses
    ///
    /// \param lhs  Left operand (a IP address)
    /// \param rhs Right operand (a IP address)
    ///
    /// \return `true` if `lhs` is greater or equal than `rhs`
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard, gnu::always_inline, gnu::pure]] friend bool operator>=(const IpAddress lhs, const IpAddress rhs)
    {
        return lhs.m_address >= rhs.m_address;
    }


    ////////////////////////////////////////////////////////////
    // Static member data
    ////////////////////////////////////////////////////////////
    // NOLINTBEGIN(readability-identifier-naming)
    static const IpAddress Any;       //!< Value representing any address (0.0.0.0)
    static const IpAddress LocalHost; //!< The "localhost" address (for connecting a computer to itself locally)
    static const IpAddress Broadcast; //!< The "broadcast" address (for sending UDP messages to everyone on a local network)
    // NOLINTEND(readability-identifier-naming)

private:
    friend IpAddressUtils;

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    za::U32 m_address; //!< Address stored as an unsigned 32 bit integer
};

} // namespace za


////////////////////////////////////////////////////////////
/// \class za::IpAddress
/// \ingroup network
///
/// `za::IpAddress` is a utility class for manipulating network
/// addresses. It provides a set a implicit constructors and
/// conversion functions to easily build or transform an IP
/// address from/to various representations.
///
/// Usage example:
/// \code
/// auto a2 = za::IpAddressUtils::resolve("127.0.0.1");      // the local host address
/// auto a3 = za::IpAddress::Broadcast;                 // the broadcast address
/// za::IpAddress a4(192, 168, 1, 56);                  // a local address
/// auto a5 = za::IpAddressUtils::resolve("my_computer");    // a local address created from a network name
/// auto a6 = za::IpAddressUtils::resolve("89.54.1.169");    // a distant address
/// auto a7 = za::IpAddressUtils::resolve("www.google.com"); // a distant address created from a network name
/// auto a8 = za::IpAddress::getLocalAddress();         // my address on the local network
/// auto a9 = za::IpAddress::getPublicAddress();        // my address on the internet
/// \endcode
///
/// Note that `za::IpAddress` currently doesn't support IPv6
/// nor other types of network addresses.
///
////////////////////////////////////////////////////////////
