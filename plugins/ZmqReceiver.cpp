/**
 *
 * @file ZmqReceiver.cpp ZmqReceiver messaging class definitions
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "CallbackAdapter.hpp"
#include "ipm/Receiver.hpp"
#include "ipm/ZmqContext.hpp"

#include "logging/Logging.hpp"
#include "utilities/Resolver.hpp"

#include <string>
#include <vector>

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
    unregister_callback();
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

  std::string connect_for_receives(const nlohmann::json& connection_info) override
  {
    try {
      m_socket.set(zmq::sockopt::rcvtimeo, 0); // Return immediately if we can't receive
    } catch (zmq::error_t const& err) {
      throw ZmqOperationError(ERS_HERE,
                              "set timeout",
                              "receive",
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
                              "receive",
                              "An invalid URI was passed",
                              connection_info.value<std::string>("connection_string", "inproc://default"),
                              err);
    }

    if (resolved.size() == 0) {
      throw ZmqOperationError(ERS_HERE,
                              "resolve connection_string",
                              "receive",
                              "Unable to resolve connection_string",
                              connection_info.value<std::string>("connection_string", "inproc://default"));
    }
    for (auto& connection_string : resolved) {
      TLOG() << "Connection String is " << connection_string;
      try {
        m_socket.bind(connection_string);
        m_connection_string = m_socket.get(zmq::sockopt::last_endpoint);
        m_socket_connected = true;
        break;
      } catch (zmq::error_t const& err) {
        ers::error(ZmqOperationError(ERS_HERE, "bind", "receive", err.what(), connection_string));
      }
    }
    if (!m_socket_connected) {
      throw ZmqOperationError(ERS_HERE, "bind", "receive", "Bind failed for all resolved connection strings", "");
    }

    m_callback_adapter.set_receiver(this);

    return m_connection_string;
  }

  bool can_receive() const noexcept override { return m_socket_connected; }

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
        TLOG_DEBUG(20) << "Endpoint " << m_connection_string << ": Going to receive header";
        res = m_socket.recv(hdr);
        TLOG_DEBUG(25) << "Endpoint " << m_connection_string << ": Recv res=" << res.value_or(0)
                       << " for header (hdr.size() == " << hdr.size() << ")";
      } catch (zmq::error_t const& err) {
        throw ZmqReceiveError(ERS_HERE, err.what(), "header");
      }
      if (res || hdr.more()) {
        TLOG_DEBUG(20) << "Endpoint " << m_connection_string << ": Going to receive data";
        output.metadata.resize(hdr.size());
        memcpy(&output.metadata[0], hdr.data(), hdr.size());

        // ZMQ guarantees that the entire message has arrived

        try {
          res = m_socket.recv(msg);
        } catch (zmq::error_t const& err) {
          throw ZmqReceiveError(ERS_HERE, err.what(), "data");
        }
        TLOG_DEBUG(25) << "Endpoint " << m_connection_string << ": Recv res=" << res.value_or(0)
                       << " for data (msg.size() == " << msg.size() << ")";
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

    TLOG_DEBUG(15) << "Endpoint " << m_connection_string << ": Returning output with metadata size "
                   << output.metadata.size() << " and data size " << output.data.size();
    return output;
  }

private:
  zmq::socket_t m_socket;
  std::string m_connection_string;
  bool m_socket_connected{ false };
  CallbackAdapter m_callback_adapter;
};
} // namespace ipm
} // namespace dunedaq

DEFINE_DUNE_IPM_RECEIVER(dunedaq::ipm::ZmqReceiver)
