/**
 *
 * @file ZmqReceiver.cpp ZmqReceiver messaging class definitions
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ipm/Receiver.hpp"
#include "ipm/ZmqContext.hpp"

#include "logging/Logging.hpp"

#include <string>

namespace dunedaq {
namespace ipm {

class ZmqReceiver : public Receiver
{
public:
  ZmqReceiver()
    : m_socket(ZmqContext::instance().GetContext(), zmq::socket_type::pull)
  {}

  ~ZmqReceiver()
  {
    // Probably (cpp)zmq does this in the socket dtor anyway, but I guess it doesn't hurt to be explicit
    if (m_connection_string != "" && m_socket_connected) {
      try {
        m_socket.unbind(m_connection_string);
        m_socket_connected = false;
      } catch (zmq::error_t const& err) {
        ers::error(ZmqOperationError(ERS_HERE, "unbind", "receive", err.what(), m_connection_string));
      }
    }
    m_socket.close();
  }

  void connect_for_receives(const nlohmann::json& connection_info) override
  {
    m_connection_string = connection_info.value<std::string>("connection_string", "inproc://default");
    TLOG() << "Connection String is " << m_connection_string;
    try {
      m_socket.setsockopt(ZMQ_RCVTIMEO, 1); // 1 ms, we'll repeat until we reach timeout
      m_socket.bind(m_connection_string);
      m_socket_connected = true;
    } catch (zmq::error_t const& err) {
      throw ZmqOperationError(ERS_HERE, "bind", "receive", err.what(), m_connection_string);
    }
  }

  bool can_receive() const noexcept override { return m_socket_connected; }

protected:
  Receiver::Response receive_(const duration_t& timeout) override
  {
    Receiver::Response output;
    zmq::message_t hdr, msg;
    size_t res = 0;

    auto start_time = std::chrono::steady_clock::now();
    do {

      try {
        TLOG_DEBUG(11) << "Endpoint " << m_connection_string << ": Going to receive header";
        res = m_socket.recv(&hdr);
        TLOG_DEBUG(21) << "Endpoint " << m_connection_string << ": Recv res=" << res
                       << " for header (hdr.size() == " << hdr.size() << ")";
      } catch (zmq::error_t const& err) {
        throw ZmqReceiveError(ERS_HERE, err.what(), "header");
      }
      if (res > 0 || hdr.more()) {
        TLOG_DEBUG(11) << "Endpoint " << m_connection_string << ": Going to receive data";
        output.metadata.resize(hdr.size());
        memcpy(&output.metadata[0], hdr.data(), hdr.size());

        // ZMQ guarantees that the entire message has arrived

        try {
          res = m_socket.recv(&msg);
        } catch (zmq::error_t const& err) {
          throw ZmqReceiveError(ERS_HERE, err.what(), "data");
        }
        TLOG_DEBUG(21) << "Endpoint " << m_connection_string << ": Recv res=" << res
                       << " for data (msg.size() == " << msg.size() << ")";
        output.data.resize(msg.size());
        memcpy(&output.data[0], msg.data(), msg.size());
      } else {
        usleep(1000);
      }
    } while (std::chrono::duration_cast<duration_t>(std::chrono::steady_clock::now() - start_time) < timeout &&
             res == 0);

    if (res == 0) {
      throw ReceiveTimeoutExpired(ERS_HERE, timeout.count());
    }

    TLOG_DEBUG(5) << "Endpoint " << m_connection_string << ": Returning output with metadata size "
                  << output.metadata.size() << " and data size " << output.data.size();
    return output;
  }

private:
  zmq::socket_t m_socket;
  std::string m_connection_string;
  bool m_socket_connected{ false };
};
} // namespace ipm
} // namespace dunedaq

DEFINE_DUNE_IPM_RECEIVER(dunedaq::ipm::ZmqReceiver)
