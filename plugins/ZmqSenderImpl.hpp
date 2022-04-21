/**
 *
 * @file ZmqSenderImpl.cpp Implementations of common routines for ZeroMQ Senders
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef IPM_PLUGINS_ZMQSENDERIMPL_HPP_
#define IPM_PLUGINS_ZMQSENDERIMPL_HPP_

#include "ipm/Sender.hpp"
#include "ipm/ZmqContext.hpp"

#include "logging/Logging.hpp"
#include "utilities/Resolver.hpp"
#include "zmq.hpp"

#include <string>

namespace dunedaq {
namespace ipm {
class ZmqSenderImpl : public Sender
{
public:
  enum class SenderType
  {
    Publisher,
    Push,
  };

  explicit ZmqSenderImpl(SenderType type)
    : m_socket(ZmqContext::instance().GetContext(),
               type == SenderType::Push ? zmq::socket_type::push : zmq::socket_type::pub)
    , m_sender_type(type)
  {}

  ~ZmqSenderImpl()
  {
    // Probably (cpp)zmq does this in the socket dtor anyway, but I guess it doesn't hurt to be explicit
    if (m_connection_string != "" && m_socket_connected) {
      try {
        if (m_sender_type == SenderType::Push) {
          m_socket.disconnect(m_connection_string);
        } else {
          m_socket.unbind(m_connection_string);
        }
        m_socket_connected = false;
      } catch (zmq::error_t const& err) {
        auto operation = m_sender_type == SenderType::Push ? "disconnect" : "unbind";
        ers::error(ZmqOperationError(ERS_HERE, operation, "send", err.what(), m_connection_string));
      }
    }
    m_socket.close();
  }

  bool can_send() const noexcept override { return m_socket_connected; }
  void connect_for_sends(const nlohmann::json& connection_info)
  {
    try {
      m_socket.setsockopt(ZMQ_SNDTIMEO, 1); // 1 ms, we'll repeat until we reach timeout
    } catch (zmq::error_t const& err) {
      throw ZmqOperationError(ERS_HERE,
                              "set timeout",
                              "send",
                              err.what(),
                              connection_info.value<std::string>("connection_string", "inproc://default"));
    }

    std::vector<std::string> resolved;
    try {

      resolved =
        utilities::resolve_uri_hostname(connection_info.value<std::string>("connection_string", "inproc://default"));
    } catch (utilities::InvalidUri const& err) {
      throw ZmqOperationError(ERS_HERE,
                              "resolve connection_string",
                              "send",
                              "An invalid URI was passed",
                              connection_info.value<std::string>("connection_string", "inproc://default"),
                              err);
    }

    if (resolved.size() == 0) {
      throw ZmqOperationError(ERS_HERE,
                              "resolve connection_string",
                              "send",
                              "Unable to resolve connection_string",
                              connection_info.value<std::string>("connection_string", "inproc://default"));
    }
    for (auto& connection_string : resolved) {
      TLOG() << "Connection String is " << connection_string;
      try {
        if (m_sender_type == SenderType::Push) {
          m_socket.connect(connection_string);
        } else {
          m_socket.bind(connection_string);
        }
        m_connection_string = connection_string;
        m_socket_connected = true;
        break;
      } catch (zmq::error_t const& err) {
        auto operation = m_sender_type == SenderType::Push ? "connect" : "bind";
        ers::error(ZmqOperationError(ERS_HERE, operation, "send", err.what(), connection_string));
      }
    }
    if (!m_socket_connected) {
      auto operation = m_sender_type == SenderType::Push ? "connect" : "bind";
      throw ZmqOperationError(ERS_HERE, operation, "send", "Operation failed for all resolved connection strings", "");
    }
  }

protected:
  void send_(const void* message, int N, const duration_t& timeout, std::string const& topic) override
  {
    TLOG_DEBUG(10) << "Endpoint " << m_connection_string << ": Starting send of " << N << " bytes";
    auto start_time = std::chrono::steady_clock::now();
    bool res = false;
    do {

      zmq::message_t topic_msg(topic.c_str(), topic.size());
      try {
        res = m_socket.send(topic_msg, ZMQ_SNDMORE);
      } catch (zmq::error_t const& err) {
        throw ZmqSendError(ERS_HERE, err.what(), topic.size(), topic);
      }

      if (!res) {
        TLOG_DEBUG(2) << "Endpoint " << m_connection_string << ": Unable to send message";
        continue;
      }

      zmq::message_t msg(message, N);
      try {
        res = m_socket.send(msg);
      } catch (zmq::error_t const& err) {
        throw ZmqSendError(ERS_HERE, err.what(), N, topic);
      }
    } while (std::chrono::duration_cast<duration_t>(std::chrono::steady_clock::now() - start_time) < timeout && !res);

    if (!res) {
      throw SendTimeoutExpired(ERS_HERE, timeout.count());
    }

    TLOG_DEBUG(15) << "Endpoint " << m_connection_string << ": Completed send of " << N << " bytes";
  }

private:
  zmq::socket_t m_socket;
  std::string m_connection_string;
  SenderType m_sender_type;
  bool m_socket_connected;
};

} // namespace ipm
} // namespace dunedaq

#endif // IPM_PLUGINS_ZMQSENDERIMPL_HPP_
