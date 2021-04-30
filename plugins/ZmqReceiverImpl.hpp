/**
 *
 * @file ZmqReceiverImpl.hpp Implementations of common routines for ZeroMQ Receivers
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */
#ifndef IPM_PLUGINS_ZMQRECEIVERIMPL_HPP_
#define IPM_PLUGINS_ZMQRECEIVERIMPL_HPP_

#include "ipm/Subscriber.hpp"
#include "ipm/ZmqContext.hpp"

#include "logging/Logging.hpp"
#include "zmq.hpp"

#include <string>
#include <vector>

namespace dunedaq {
namespace ipm {

// Remember that Subscriber is a superset of Receiver
class ZmqReceiverImpl : public Subscriber
{
public:
  enum class ReceiverType
  {
    Subscriber,
    Pull,
  };

  explicit ZmqReceiverImpl(ReceiverType type)
    : m_socket(ZmqContext::instance().GetContext(),
               type == ReceiverType::Pull ? zmq::socket_type::pull : zmq::socket_type::sub)
  {}
  bool can_receive() const noexcept override { return m_socket_connected; }
  void connect_for_receives(const nlohmann::json& connection_info) override
  {
    std::string connection_string = connection_info.value<std::string>("connection_string", "inproc://default");
    TLOG() << "Connection String is " << connection_string;
    try {
      m_socket.setsockopt(ZMQ_RCVTIMEO, 1); // 1 ms, we'll repeat until we reach timeout
      m_socket.connect(connection_string);
      m_socket_connected = true;
    } catch (zmq::error_t const& err) {
      throw ZmqReceiverConnectError(ERS_HERE, err.what(), connection_string);
    }
  }

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
        TLOG_DEBUG(3) << "Endpoint " << m_connection_string << ": Going to receive header";
        res = m_socket.recv(&hdr);
        TLOG_DEBUG(3) << "Endpoint " << m_connection_string << ": Recv res=" << res
                      << " for header (hdr.size() == " << hdr.size() << ")";
      } catch (zmq::error_t const& err) {
        throw ZmqReceiveError(ERS_HERE, err.what(), "header");
      }
      if (res > 0 || hdr.more()) {
        TLOG_DEBUG(3) << "Endpoint " << m_connection_string << ": Going to receive data";
        output.metadata.resize(hdr.size());
        memcpy(&output.metadata[0], hdr.data(), hdr.size());

        // ZMQ guarantees that the entire message has arrived

        try {
          res = m_socket.recv(&msg);
        } catch (zmq::error_t const& err) {
          throw ZmqReceiveError(ERS_HERE, err.what(), "data");
        }
        TLOG_DEBUG(3) << "Endpoint " << m_connection_string << ": Recv res=" << res
                      << " for data (msg.size() == " << msg.size() << ")";
        output.data.resize(msg.size());
        memcpy(&output.data[0], msg.data(), msg.size());
      } else {
        usleep(1000);
      }
    } while (std::chrono::steady_clock::now() - start_time < timeout && res == 0);

    if (res == 0) {
      throw ReceiveTimeoutExpired(ERS_HERE, timeout.count());
    }

    TLOG_DEBUG(2) << "Endpoint " << m_connection_string << ": Returning output with metadata size "
                  << output.metadata.size() << " and data size "
                         << output.data.size();
    return output;
  }

private:
  zmq::socket_t m_socket;
  std::string m_connection_string;
  bool m_socket_connected{ false };
};

} // namespace ipm
} // namespace dunedaq

#endif // IPM_PLUGINS_ZMQRECEIVERIMPL_HPP_
