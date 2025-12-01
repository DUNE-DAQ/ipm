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
#include "utilities/ZmqUri.hpp"
#include "zmq.hpp"

#include <string>
#include <vector>

namespace dunedaq::ipm {
class ZmqSender : public Sender
{
public:
  struct Sockinfo_t
  {
    zmq::socket_t socket;
    size_t bytes_sent{ 0 };
    std::string connection_string;
  };

  ZmqSender() {}

  ~ZmqSender()
  {
    // Probably (cpp)zmq does this in the socket dtor anyway, but I guess it doesn't hurt to be explicit
    if (can_send()) {
      for (auto& sock : m_sockets) {
        try {
          sock->socket.disconnect(sock->connection_string);
          sock->socket.close();
        } catch (zmq::error_t const& err) {
          ers::error(ZmqOperationError(ERS_HERE, "disconnect", "send", err.what(), sock->connection_string));
        }
      }
      m_sockets.clear();
    }
  }

  bool can_send() const noexcept override { return m_sockets.size() > 0; }

  std::string connect_for_sends(const nlohmann::json& connection_info) override
  {
    auto send_endpoints = connection_info.value<std::vector<std::string>>("send_endpoints", {});
    auto hwm = connection_info.value<int>("capacity", 0);
    m_connection_string = connection_info.value<std::string>("connection_string", "inproc://default");

    if (send_endpoints.size() > 0) {
      auto base_uri = utilities::ZmqUri(m_connection_string);
      for (auto& endpoint : send_endpoints) {
        auto connection_uri = base_uri;

        if (endpoint.find(":") != std::string::npos) {
          connection_uri.endpoint_host = endpoint.substr(0, endpoint.find(":"));
          connection_uri.endpoint_port = endpoint.substr(endpoint.find(":") + 1);
        } else {
          connection_uri.endpoint_host = endpoint;
        }
        create_and_connect_socket(connection_uri, hwm);
      }
    } else {
      create_and_connect_socket(utilities::ZmqUri(m_connection_string), hwm);
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
    TLOG_DEBUG(TLVL_ZMQSENDER_SEND_START)
      << "Endpoint " << m_connection_string << ": Starting send of " << N << " bytes";
    auto start_time = std::chrono::steady_clock::now();
    zmq::send_result_t res{};

    auto socket = get_socket();

    do {

      zmq::message_t topic_msg(topic.c_str(), topic.size());
      try {
        res = socket->socket.send(topic_msg, zmq::send_flags::sndmore);
      } catch (zmq::error_t const& err) {
        throw ZmqSendError(ERS_HERE, err.what(), topic.size(), topic);
      }

      if (!res || res != topic.size()) {
        TLOG_DEBUG(TLVL_ZMQSENDER_SEND_ERR) << "Endpoint " << m_connection_string << ": Unable to send message";
        continue;
      }

      zmq::message_t msg(message, N);
      try {
        res = socket->socket.send(msg, zmq::send_flags::none);
        socket->bytes_sent += N;
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

    TLOG_DEBUG(TLVL_ZMQSENDER_SEND_END) << "Endpoint " << m_connection_string << ": Completed send of " << N
                                        << " bytes";
    return res && res == N;
  }

private:
  void create_and_connect_socket(utilities::ZmqUri connection_uri, int capacity)
  {
    auto conn_uri = connection_uri.to_string();
    std::shared_ptr<Sockinfo_t> info = std::make_shared<Sockinfo_t>();
    info->socket = zmq::socket_t(ZmqContext::instance().GetContext(), zmq::socket_type::push);
    info->bytes_sent = 0;
    info->connection_string = conn_uri;

    try {
      info->socket.set(zmq::sockopt::sndtimeo, 0); // Return immediately if we can't send
    } catch (zmq::error_t const& err) {
      throw ZmqOperationError(ERS_HERE, "set timeout", "send", err.what(), conn_uri);
    }

    if (capacity > 0) {
      try {
        info->socket.set(zmq::sockopt::sndhwm, capacity);
      } catch (zmq::error_t const& err) {
        throw ZmqOperationError(ERS_HERE, "set hwm", "send", err.what(), conn_uri);
      }
    }

    TLOG_DEBUG(TLVL_CONNECTIONSTRING) << "Connection String is " << conn_uri;
    try {
      info->socket.set(zmq::sockopt::immediate, 1); // Don't queue messages to incomplete connections
    } catch (zmq::error_t const& err) {
      throw ZmqOperationError(ERS_HERE, "set immediate mode", "send", err.what(), conn_uri);
    }

    try {
      info->socket.connect(conn_uri);
    } catch (zmq::error_t const& err) {
      throw ZmqOperationError(ERS_HERE, "connect", "send", err.what(), conn_uri);
    }

    m_sockets.push_back(info);
  }

  std::shared_ptr<Sockinfo_t> get_socket()
  {
    // Typical case, so don't iterate
    if (m_sockets.size() == 1) {
      return m_sockets.front();
    }
    size_t min_sent = -1;
    std::shared_ptr<Sockinfo_t> sock = nullptr;

    for (auto& sock_info : m_sockets) {
      if (sock_info->bytes_sent < min_sent) {
        min_sent = sock_info->bytes_sent;
        sock = sock_info;
      }
    }

    return sock;
  }

  std::vector<std::shared_ptr<Sockinfo_t>> m_sockets;
  std::string m_connection_string;
};

} // namespace dunedaq::ipm

DEFINE_DUNE_IPM_SENDER(dunedaq::ipm::ZmqSender)
