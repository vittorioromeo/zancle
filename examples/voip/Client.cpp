////////////////////////////////////////////////////////////
// Headers
////////////////////////////////////////////////////////////
#include "Client.hpp"

#include "Zancle/Audio/CaptureDeviceHandle.hpp"
#include "Zancle/Audio/SoundRecorder.hpp"

#include "Zancle/Network/IpAddress.hpp"
#include "Zancle/Network/IpAddressUtils.hpp"
#include "Zancle/Network/Packet.hpp"
#include "Zancle/Network/Socket.hpp"
#include "Zancle/Network/TcpSocket.hpp"

#include "Zancle/Scn/ScnStdin.hpp"

#include "Zancle/Fmt/Fmt.hpp"

#include "Zancle/String/String.hpp"

#include "Zancle/Vocabulary/Optional.hpp"

#include "Zancle/Base/IntTypes.hpp"
#include "Zancle/Base/SizeT.hpp"


constexpr za::U8 clientAudioData   = 1;
constexpr za::U8 clientEndOfStream = 2;


////////////////////////////////////////////////////////////
/// Specialization of audio recorder for sending recorded audio
/// data through the network
////////////////////////////////////////////////////////////
class NetworkRecorder : public za::SoundRecorder
{
public:
    ////////////////////////////////////////////////////////////
    /// Constructor
    ///
    /// \param host Remote host to which send the recording data
    /// \param port Port of the remote host
    ///
    ////////////////////////////////////////////////////////////
    explicit NetworkRecorder(za::IpAddress host, unsigned short port) : m_host(host), m_port(port)
    {
    }

    ////////////////////////////////////////////////////////////
    /// Destructor
    ///
    /// \see SoundRecorder::~SoundRecorder()
    ///
    ////////////////////////////////////////////////////////////
    ~NetworkRecorder() override
    {
        if (!stop())
            za::printErrLn("Failed to stop network recorder on destruction");
    }

private:
    ////////////////////////////////////////////////////////////
    /// \see SoundRecorder::onStart
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool onStart(za::CaptureDevice&) override
    {
        m_socket = za::TcpSocket::create(/* isBlocking */ true);
        if (!m_socket.hasValue())
            return false;

        if (m_socket->connect(m_host, m_port) == za::Socket::Status::Done)
        {
            za::printLn("Connected to server {}", za::IpAddressUtils::toString(m_host));
            return true;
        }

        m_socket.reset();
        return false;
    }

    ////////////////////////////////////////////////////////////
    /// \see SoundRecorder::onProcessSamples
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool onProcessSamples(const za::I16* samples, za::SizeT sampleCount) override
    {
        // Pack the audio samples into a network packet
        za::Packet packet;
        packet << clientAudioData;
        packet.append(samples, sampleCount * sizeof(za::I16));

        // Send the audio packet to the server
        return m_socket->send(packet) == za::Socket::Status::Done;
    }

    ////////////////////////////////////////////////////////////
    /// \see SoundRecorder::onStop
    ///
    ////////////////////////////////////////////////////////////
    [[nodiscard]] bool onStop(za::CaptureDevice&) override
    {
        // Send an "end-of-stream" packet
        za::Packet packet;
        packet << clientEndOfStream;

        if (m_socket->send(packet) != za::Socket::Status::Done)
        {
            za::printErrLn("Failed to send end-of-stream packet");
            return false;
        }

        // Close the socket
        m_socket->disconnect();
        m_socket.reset();

        return true;
    }

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    za::IpAddress               m_host;   ///< Address of the remote host
    unsigned short              m_port;   ///< Remote port
    za::Optional<za::TcpSocket> m_socket; ///< Socket used to communicate with the server (created in onStart)
};


////////////////////////////////////////////////////////////
/// Create a client, connect it to a running server and
/// start sending it audio data
///
////////////////////////////////////////////////////////////
void doClient(za::CaptureDevice& captureDevice, unsigned short port)
{
    // Ask for server address
    za::Optional<za::IpAddress> server;
    do
    {
        za::print("Type address or name of the server to connect to: ");

        za::String addressStr;
        (void)za::scnStdinInto(addressStr);
        server = za::IpAddressUtils::resolve(addressStr);
    } while (!server.hasValue());

    // Create an instance of our custom recorder
    NetworkRecorder recorder(server.value(), port);

    // Wait for user input...
    za::scnStdinIgnoreLine();
    za::print("Press enter to start recording audio");
    za::scnStdinIgnoreLine();

    // Start capturing audio data
    if (!recorder.start(captureDevice, 44'100))
    {
        za::printErrLn("Failed to start recorder");
        return;
    }

    za::print("Recording... press enter to stop");
    za::scnStdinIgnoreLine();

    if (!recorder.stop())
        za::printErrLn("Failed to stop network recorder");
}
