
/**
 *
 * @file ZmqSubscriber.cpp ZmqSubscriber messaging class definitions
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ipm/Subscriber.hpp"
#include "ipm/ZmqContext.hpp"

#include "logging/Logging.hpp"

namespace dunedaq {
namespace ipm {

class ZmqSubscriber : public Subscriber
{
public:
  ZmqSubscriber()
    : m_socket(ZmqContext::instance().GetContext(), zmq::socket_type::sub)
  {}

  ~ZmqSubscriber()
  {
    // Probably (cpp)zmq does this in the socket dtor anyway, but I guess it doesn't hurt to be explicit
    if (!m_connection_strings.empty() && m_socket_connected) {
      m_socket_connected = false;
      for (auto& conn_string : m_connection_strings) {
        try {
          m_socket.disconnect(conn_string);
        } catch (zmq::error_t const& err) {
          ers::error(ZmqReceiverConnectError(ERS_HERE, err.what(), conn_string));
        }
      }
    }
    m_socket.close();
  }

  void connect_for_receives(const nlohmann::json& connection_info) override
  {
    if (connection_info.contains("connection_string")) {
      m_connection_strings.push_back(connection_info.value<std::string>("connection_string", ""));
    }

    for (auto& conn_string : connection_info.value<std::vector<std::string>>("connection_strings", {})) {
      m_connection_strings.push_back(conn_string);
    }
    TLOG() << "Setting socket options";
    try {
      m_socket.setsockopt(ZMQ_RCVTIMEO, 1); // 1 ms, we'll repeat until we reach timeout
    } catch (zmq::error_t const& err) {
      throw ZmqReceiverConnectError(ERS_HERE, err.what(), m_connection_strings[0]);
    }
    for (auto& conn_string : m_connection_strings) {
      try {
        TLOG() << "Connecting to publisher at " << conn_string;
        m_socket.connect(conn_string);
      } catch (zmq::error_t const& err) {
        throw ZmqReceiverConnectError(ERS_HERE, err.what(), conn_string);
      }
    }
    m_socket_connected = true;
  }

  bool can_receive() const noexcept override { return m_socket_connected; }

  void subscribe(std::string const& topic) override
  {
    try {
      m_socket.setsockopt(ZMQ_SUBSCRIBE, topic.c_str(), topic.size());
    } catch (zmq::error_t const& err) {
      throw ZmqSubscribeError(ERS_HERE, err.what(), topic);
    }
  }
  void unsubscribe(std::string const& topic) override
  {
    try {
      m_socket.setsockopt(ZMQ_UNSUBSCRIBE, topic.c_str(), topic.size());
    } catch (zmq::error_t const& err) {
      throw ZmqUnsubscribeError(ERS_HERE, err.what(), topic);
    }
  }

protected:
  Receiver::Response receive_(const duration_t& timeout) override
  {
    Receiver::Response output;
    zmq::message_t hdr, msg;
    size_t res = 0;

    auto start_time = std::chrono::steady_clock::now();
    do {

      try {
        TLOG_DEBUG(11) << "Subscriber: Going to receive header";
        res = m_socket.recv(&hdr);
        TLOG_DEBUG(21) << "Subscriber: Recv res=" << res << " for header (hdr.size() == " << hdr.size() << ")";
      } catch (zmq::error_t const& err) {
        throw ZmqReceiveError(ERS_HERE, err.what(), "header");
      }
      if (res > 0 || hdr.more()) {
        TLOG_DEBUG(11) << "Subscriber: Going to receive data";
        output.metadata.resize(hdr.size());
        memcpy(&output.metadata[0], hdr.data(), hdr.size());

        // ZMQ guarantees that the entire message has arrived

        try {
          res = m_socket.recv(&msg);
        } catch (zmq::error_t const& err) {
          throw ZmqReceiveError(ERS_HERE, err.what(), "data");
        }
        TLOG_DEBUG(21) << "Subscriber: Recv res=" << res << " for data (msg.size() == " << msg.size() << ")";
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

    TLOG_DEBUG(5) << "Subscriber: Returning output with metadata size " << output.metadata.size() << " and data size "
                  << output.data.size();
    return output;
  }

private:
  zmq::socket_t m_socket;
  std::vector<std::string> m_connection_strings;
  bool m_socket_connected{ false };
};
} // namespace ipm
} // namespace dunedaq

DEFINE_DUNE_IPM_RECEIVER(dunedaq::ipm::ZmqSubscriber)
