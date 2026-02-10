/**
 *
 * @file ZmqPublisher.cpp ZmqPublisher messaging class definitions
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ipm/Sender.hpp"
#include "ipm/ZmqContext.hpp"

#include "logging/Logging.hpp"
#include "utilities/ZmqUri.hpp"
#include "zmq.hpp"

#include <string>
#include <vector>

namespace dunedaq::ipm {
class ZmqPublisher : public Sender
{
public:
  ZmqPublisher()
    : m_socket(ZmqContext::instance().GetContext(), zmq::socket_type::pub)
  {
  }

  ~ZmqPublisher()
  {
    // Probably (cpp)zmq does this in the socket dtor anyway, but I guess it doesn't hurt to be explicit
    if (m_connection_info.connection_string != "" && m_socket_connected) {
      try {
        TLOG_DEBUG(TLVL_ZMQPUBLISHER_DESTRUCTOR) << m_connection_info.connection_name << ": Setting socket HWM to zero";
        m_socket.set(zmq::sockopt::sndhwm, 1);

        TLOG_DEBUG(TLVL_ZMQPUBLISHER_DESTRUCTOR)
          << m_connection_info.connection_name
          << ": Waiting up to 10s for socket to become writable before disconnecting";
        auto start_time = std::chrono::steady_clock::now();
        while (
          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count() <
          10000) {
          auto events = m_socket.get(zmq::sockopt::events);
          if ((events & ZMQ_POLLOUT) != 0) {
            break;
          }
          usleep(1000);
        }
        TLOG_DEBUG(TLVL_ZMQPUBLISHER_DESTRUCTOR)
          << m_connection_info.connection_name << ": Disconnecting socket from " << m_connection_info.connection_string;

        m_socket.unbind(m_connection_info.connection_string);
        m_socket_connected = false;
      } catch (zmq::error_t const& err) {
        ers::error(ZmqOperationError(ERS_HERE,
                                     m_connection_info.connection_name,
                                     "unbind",
                                     "send",
                                     err.what(),
                                     m_connection_info.connection_string));
      }
    }
    m_socket.close();
  }

  bool can_send() const noexcept override { return m_socket_connected; }
  std::string connect_for_sends(const ConnectionInfo& connection_info) override
  {
    m_connection_info = connection_info;
    try {
      m_socket.set(zmq::sockopt::sndtimeo, 0); // Return immediately if we can't send
    } catch (zmq::error_t const& err) {
      throw ZmqOperationError(ERS_HERE,
                              m_connection_info.connection_name,
                              "set timeout",
                              "send",
                              err.what(),
                              m_connection_info.connection_string);
    }

    auto hwm = connection_info.capacity;
    if (hwm > 0) {
      try {
        m_socket.set(zmq::sockopt::sndhwm, hwm);
      } catch (zmq::error_t const& err) {
        throw ZmqOperationError(ERS_HERE,
                                m_connection_info.connection_name,
                                "set hwm",
                                "send",
                                err.what(),
                                m_connection_info.connection_string);
      }
    }

    std::vector<std::string> resolved;
    try {
      utilities::ZmqUri this_uri(m_connection_info.connection_string);
      resolved = this_uri.get_uri_ip_addresses();
    } catch (utilities::InvalidUri const& err) {
      throw ZmqOperationError(ERS_HERE,
                              m_connection_info.connection_name,
                              "resolve connection_string",
                              "send",
                              "An invalid URI was passed",
                              m_connection_info.connection_string,
                              err);
    }

    if (resolved.size() == 0) {
      throw ZmqOperationError(ERS_HERE,
                              m_connection_info.connection_name,
                              "resolve connection_string",
                              "send",
                              "Unable to resolve connection_string",
                              m_connection_info.connection_string);
    }
    for (auto& connection_string : resolved) {
      TLOG_DEBUG(TLVL_CONNECTIONSTRING) << m_connection_info.connection_name << ": Connection String is "
                                        << connection_string;
      try {
        m_socket.bind(connection_string);
        m_connection_info.connection_string = m_socket.get(zmq::sockopt::last_endpoint);
        m_socket_connected = true;
        break;
      } catch (zmq::error_t const& err) {
        ers::error(ZmqOperationError(
          ERS_HERE, m_connection_info.connection_name, "bind", "send", err.what(), connection_string));
      }
    }
    if (!m_socket_connected) {
      throw ZmqOperationError(ERS_HERE,
                              m_connection_info.connection_name,
                              "bind",
                              "send",
                              "Bind failed for all resolved connection strings",
                              "");
    }

    return m_connection_info.connection_string;
  }

protected:
  bool send_(const void* message,
             int N,
             const duration_t& timeout,
             std::string const& topic,
             bool no_tmoexcept_mode) override
  {
    TLOG_DEBUG(TLVL_ZMQPUBLISHER_SEND_START)
      << m_connection_info.connection_name << ": Starting send of " << N << " bytes";
    auto start_time = std::chrono::steady_clock::now();
    zmq::send_result_t res{};
    do {

      zmq::message_t topic_msg(topic.c_str(), topic.size());
      try {
        res = m_socket.send(topic_msg, zmq::send_flags::sndmore);
      } catch (zmq::error_t const& err) {
        throw ZmqSendError(ERS_HERE, m_connection_info.connection_name, err.what(), topic.size(), topic);
      }

      if (!res || res != topic.size()) {
        TLOG_DEBUG(TLVL_ZMQPUBLISHER_SEND_ERR) << m_connection_info.connection_name << ": Unable to send message";
        continue;
      }

      zmq::message_t msg(message, N);
      try {
        res = m_socket.send(msg, zmq::send_flags::none);
      } catch (zmq::error_t const& err) {
        throw ZmqSendError(ERS_HERE, m_connection_info.connection_name, err.what(), N, topic);
      }

      if (!res && timeout > duration_t::zero()) {
        usleep(1000);
      }
    } while (std::chrono::duration_cast<duration_t>(std::chrono::steady_clock::now() - start_time) < timeout && !res);

    if (!res && !no_tmoexcept_mode) {
      throw SendTimeoutExpired(ERS_HERE, m_connection_info.connection_name, timeout.count());
    }

    TLOG_DEBUG(TLVL_ZMQPUBLISHER_SEND_END)
      << m_connection_info.connection_name << ": Completed send of " << N << " bytes";
    return res && res == N;
  }

private:
  zmq::socket_t m_socket;
  bool m_socket_connected{ false };
};

} // namespace dunedaq::ipm

DEFINE_DUNE_IPM_SENDER(dunedaq::ipm::ZmqPublisher)
