
/**
 *
 * @file ZmqSubscriber.cpp ZmqSubscriber messaging class definitions
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "CallbackAdapter.hpp"
#include "ipm/Subscriber.hpp"
#include "ipm/ZmqContext.hpp"

#include "logging/Logging.hpp"
#include "utilities/Resolver.hpp"

#include <string>
#include <vector>

namespace dunedaq {
namespace ipm {

class ZmqSubscriber : public Subscriber
{
public:
  ZmqSubscriber()
    : m_socket(ZmqContext::instance().GetContext(), zmq::socket_type::sub)
  {
  }

  ~ZmqSubscriber()
  {
    unregister_callback();
    // Probably (cpp)zmq does this in the socket dtor anyway, but I guess it doesn't hurt to be explicit
    if (!m_connection_strings.empty() && m_socket_connected) {
      m_socket_connected = false;
      for (auto& conn_string : m_connection_strings) {
        try {
          m_socket.disconnect(conn_string);
        } catch (zmq::error_t const& err) {
          ers::error(ZmqOperationError(ERS_HERE, "disconnect", "receive", err.what(), conn_string));
        }
      }
    }
    m_socket.close();
  }

  std::string connect_for_receives(const nlohmann::json& connection_info) override
  {
    std::set<std::string> new_connection_strings;
    if (connection_info.contains("connection_string")) {
      if (m_connection_strings.count(connection_info.value<std::string>("connection_string", "")) == 0)
        new_connection_strings.insert(connection_info.value<std::string>("connection_string", ""));
    }

    for (auto& conn_string : connection_info.value<std::vector<std::string>>("connection_strings", {})) {
      if (m_connection_strings.count(conn_string) == 0)
        new_connection_strings.insert(conn_string);
    }

    if (m_connection_strings.size() == 0 && new_connection_strings.size() == 0) {
      throw ZmqOperationError(ERS_HERE, "resolve connections", "receive", "No valid connection strings passed", "");
    }

    if (!m_socket_connected) {
      TLOG_DEBUG(18) << "Setting socket options";
      try {
        m_socket.set(zmq::sockopt::rcvtimeo, 0); // Return immediately if we can't receive
      } catch (zmq::error_t const& err) {
        throw ZmqOperationError(ERS_HERE, "set timeout", "receive", err.what(), *m_connection_strings.begin());
      }
    }
    for (auto& conn_string : new_connection_strings) {
      try {
        TLOG_DEBUG(19) << "Connecting to publisher at " << conn_string;
        m_socket.connect(conn_string);
        m_connection_strings.insert(conn_string);
      } catch (zmq::error_t const& err) {
        ers::error(ZmqOperationError(ERS_HERE, "connect", "receive", err.what(), conn_string));
      }
    }
    m_socket_connected = true;
    m_callback_adapter.set_receiver(this);

    if (m_connection_strings.size() > 0) {
      return *m_connection_strings.begin();
    }
    return {};
  }

  bool can_receive() const noexcept override { return m_socket_connected; }

  void subscribe(std::string const& topic) override
  {
    try {
      m_socket.set(zmq::sockopt::subscribe, topic);
    } catch (zmq::error_t const& err) {
      throw ZmqSubscribeError(ERS_HERE, err.what(), topic);
    }
  }
  void unsubscribe(std::string const& topic) override
  {
    try {
      m_socket.set(zmq::sockopt::unsubscribe, topic);
    } catch (zmq::error_t const& err) {
      throw ZmqUnsubscribeError(ERS_HERE, err.what(), topic);
    }
  }

  void register_callback(std::function<void(Response&)> callback) { m_callback_adapter.set_callback(callback); }
  void unregister_callback() { m_callback_adapter.clear_callback(); }

protected:
  Receiver::Response receive_(const duration_t& timeout, bool no_tmoexcept_mode) override
  {
    Receiver::Response output;
    zmq::message_t hdr, msg;
    zmq::recv_result_t res{};

    auto start_time = std::chrono::steady_clock::now();
    do {

      try {
        TLOG_DEBUG(20) << "Subscriber: Going to receive header";
        res = m_socket.recv(hdr);
        TLOG_DEBUG(25) << "Subscriber: Recv res=" << res.value_or(0) << " for header (hdr.size() == " << hdr.size()
                       << ")";
      } catch (zmq::error_t const& err) {
        throw ZmqReceiveError(ERS_HERE, err.what(), "header");
      }
      if (res || hdr.more()) {
        TLOG_DEBUG(20) << "Subscriber: Going to receive data";
        output.metadata.resize(hdr.size());
        memcpy(&output.metadata[0], hdr.data(), hdr.size());

        // ZMQ guarantees that the entire message has arrived

        try {
          res = m_socket.recv(msg);
        } catch (zmq::error_t const& err) {
          throw ZmqReceiveError(ERS_HERE, err.what(), "data");
        }
        TLOG_DEBUG(25) << "Subscriber: Recv res=" << res.value_or(0) << " for data (msg.size() == " << msg.size()
                       << ")";
        output.data.resize(msg.size());
        memcpy(&output.data[0], msg.data(), msg.size());
      } else if (timeout > duration_t::zero()) {
        usleep(1000);
      }
    } while (std::chrono::duration_cast<duration_t>(std::chrono::steady_clock::now() - start_time) < timeout &&
             res.value_or(0) == 0);

    if (res.value_or(0) == 0 && !no_tmoexcept_mode) {
      throw ReceiveTimeoutExpired(ERS_HERE, timeout.count());
    }

    TLOG_DEBUG(15) << "Subscriber: Returning output with metadata size " << output.metadata.size() << " and data size "
                   << output.data.size();
    return output;
  }

private:
  zmq::socket_t m_socket;
  std::set<std::string> m_connection_strings{};
  bool m_socket_connected{ false };
  CallbackAdapter m_callback_adapter;
};
} // namespace ipm
} // namespace dunedaq

DEFINE_DUNE_IPM_RECEIVER(dunedaq::ipm::ZmqSubscriber)
