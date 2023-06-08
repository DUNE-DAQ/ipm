/**
 *
 * @file ZmqSender.cpp ZmqSender messaging class definitions
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ipm/Sender.hpp"
#include "ipm/ZmqContext.hpp"

#include "logging/Logging.hpp"
#include "utilities/Resolver.hpp"
#include "zmq.hpp"

#include <string>
#include <vector>

namespace dunedaq {
namespace ipm {
class ZmqSender : public Sender
{
public:
  explicit ZmqSender()
    : m_socket(ZmqContext::instance().GetContext(), zmq::socket_type::push)
  {
  }

  ~ZmqSender()
  {
    // Probably (cpp)zmq does this in the socket dtor anyway, but I guess it doesn't hurt to be explicit
    if (m_connection_string != "" && m_socket_connected) {
      try {
        m_socket.disconnect(m_connection_string);
        m_socket_connected = false;
      } catch (zmq::error_t const& err) {
        ers::error(ZmqOperationError(ERS_HERE, "disconnect", "send", err.what(), m_connection_string));
      }
    }
    m_socket.close();
  }

  bool can_send() const noexcept override { return m_socket_connected; }
  std::string connect_for_sends(const nlohmann::json& connection_info)
  {
    try {
      m_socket.set(zmq::sockopt::sndtimeo, 0); // Return immediately if we can't send
    } catch (zmq::error_t const& err) {
      throw ZmqOperationError(ERS_HERE,
                              "set timeout",
                              "send",
                              err.what(),
                              connection_info.value<std::string>("connection_string", "inproc://default"));
    }

    std::string connection_string = connection_info.value<std::string>("connection_string", "inproc://default");

    TLOG() << "Connection String is " << connection_string;
    try {
      m_socket.set(zmq::sockopt::immediate, 1); // Don't queue messages to incomplete connections
    } catch (zmq::error_t const& err) {
      throw ZmqOperationError(ERS_HERE, "set immediate mode", "send", err.what(), connection_string);
    }

    try {
      m_socket.connect(connection_string);
      m_connection_string = m_socket.get(zmq::sockopt::last_endpoint);
      m_socket_connected = true;
    } catch (zmq::error_t const& err) {
      ers::error(ZmqOperationError(ERS_HERE, "connect", "send", err.what(), connection_string));
    }

    if (!m_socket_connected) {
      throw ZmqOperationError(ERS_HERE, "connect", "send", "Operation failed for all resolved connection strings", "");
    }
    return m_connection_string;
  }

protected:
  bool send_(const void* message,
             int N,
             const duration_t& timeout,
             std::string const& topic,
             bool no_tmoexcept_mode) override
  {
    TLOG_DEBUG(10) << "Endpoint " << m_connection_string << ": Starting send of " << N << " bytes";
    auto start_time = std::chrono::steady_clock::now();
    zmq::send_result_t res{};
    do {

      zmq::message_t topic_msg(topic.c_str(), topic.size());
      try {
        res = m_socket.send(topic_msg, zmq::send_flags::sndmore);
      } catch (zmq::error_t const& err) {
        throw ZmqSendError(ERS_HERE, err.what(), topic.size(), topic);
      }

      if (!res || res != topic.size()) {
        TLOG_DEBUG(2) << "Endpoint " << m_connection_string << ": Unable to send message";
        continue;
      }

      zmq::message_t msg(message, N);
      try {
        res = m_socket.send(msg, zmq::send_flags::none);
      } catch (zmq::error_t const& err) {
        throw ZmqSendError(ERS_HERE, err.what(), N, topic);
      }

      if (!res && timeout > duration_t::zero()) {
        usleep(1000);
      }
    } while (std::chrono::duration_cast<duration_t>(std::chrono::steady_clock::now() - start_time) < timeout && !res);

    if (!res && !no_tmoexcept_mode) {
      throw SendTimeoutExpired(ERS_HERE, timeout.count());
    }

    TLOG_DEBUG(15) << "Endpoint " << m_connection_string << ": Completed send of " << N << " bytes";
    return res && res == N;
  }

private:
  zmq::socket_t m_socket;
  std::string m_connection_string;
  bool m_socket_connected{false};
};

} // namespace ipm
} // namespace dunedaq

DEFINE_DUNE_IPM_SENDER(dunedaq::ipm::ZmqSender)
