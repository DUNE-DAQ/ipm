#ifndef IPM_INCLUDE_IPM_ZMQCONTEXT_HPP_
#define IPM_INCLUDE_IPM_ZMQCONTEXT_HPP_

/**
 *
 * @file ZmqContext.hpp ZmqContext Singleton class for hosting 0MQ context
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ers/Issue.hpp"
#include "zmq.hpp"

namespace dunedaq {

/**
 * @brief An ERS Error indicating that an exception was thrown from ZMQ while performing an operation
 * @param operation The operation that failed (connect/bind/disconnect/unbind)
 * @param direction The direction of the socket (send/receive)
 * @param what The zmq::error_t exception message
 * @param connection_string The connection string that caused the error
 * @cond Doxygen doesn't like ERS macros LCOV_EXCL_START
 */
ERS_DECLARE_ISSUE(ipm,
                  ZmqOperationError,
                  "An exception occured while calling " << operation << " on the ZMQ " << direction << " socket: "
                                                        << what << " (connection_string: " << connection_string << ")",
                  ((std::string)operation)((std::string)direction)((const char*)what)(
                    (std::string)connection_string)) // NOLINT
                                                     /// @endcond LCOV_EXCL_STOP

/**
 * @brief An ERS Error indicating that an exception was thrown from ZMQ while sending
 * @param what The zmq::error_t exception message
 * @param N number of bytes in attempted send
 * @param topic Send topic
 * @cond Doxygen doesn't like ERS macros LCOV_EXCL_START
 */
ERS_DECLARE_ISSUE(ipm,
                  ZmqSendError,
                  "An exception occurred while sending " << N << " bytes to " << topic << ": " << what,
                  ((const char*)what)((int)N)((std::string)topic)) // NOLINT
                                                                   /// @endcond LCOV_EXCL_STOP

/**
 * @brief An ERS Error indicating that an exception was thrown from ZMQ while receiving
 * @param what The zmq::error_t exception message
 * @param which Which receive had an issue
 * @cond Doxygen doesn't like ERS macros LCOV_EXCL_START
 */
ERS_DECLARE_ISSUE(ipm,
                  ZmqReceiveError,
                  "An exception occured while receiving " << which << ": " << what,
                  ((const char*)what)((const char*)which)) // NOLINT
                                                           /// @endcond LCOV_EXCL_STOP

/**
 * @brief An ERS Error indicating that an exception was thrown from ZMQ during a subscribe
 * @param what The zmq::error_t exception message
 * @param topic Topic being subscribed to
 * @cond Doxygen doesn't like ERS macros LCOV_EXCL_START
 */
ERS_DECLARE_ISSUE(ipm,
                  ZmqSubscribeError,
                  "An execption occured while subscribing to " << topic << ": " << what,
                  ((const char*)what)((std::string)topic)) // NOLINT
/// @endcond LCOV_EXCL_STOP

/**
 * @brief An ERS Error indicating that an exception was thrown from ZMQ during an unsubscribe
 * @param what The zmq::error_t exception message
 * @param topic Topic being unsubscribed from
 * @cond Doxygen doesn't like ERS macros LCOV_EXCL_START
 */
ERS_DECLARE_ISSUE(ipm,
                  ZmqUnsubscribeError,
                  "An execption occured while unsubscribing from " << topic << ": " << what,
                  ((const char*)what)((std::string)topic)) // NOLINT
/// @endcond LCOV_EXCL_STOP

namespace ipm {
class ZmqContext
{
public:
  static ZmqContext& instance()
  {
    static ZmqContext s_ctx;
    return s_ctx;
  }

  zmq::context_t& GetContext() { return m_context; }

  void set_context_threads(int nthreads) { m_context.set(zmq::ctxopt::io_threads, nthreads); }

private:
  ZmqContext()
  {
    auto threads_c = getenv("IPM_ZMQ_IO_THREADS");
    if (threads_c != nullptr) {
      auto threads = std::atoi(threads_c);
      if (threads > 1) {
        set_context_threads(threads);
      }
    }
  }
  ~ZmqContext() { m_context.close(); }
  zmq::context_t m_context;

  ZmqContext(ZmqContext const&) = delete;
  ZmqContext(ZmqContext&&) = delete;
  ZmqContext& operator=(ZmqContext const&) = delete;
  ZmqContext& operator=(ZmqContext&&) = delete;
};
} // namespace ipm
} // namespace dunedaq

#endif // IPM_INCLUDE_IPM_ZMQCONTEXT_HPP_
