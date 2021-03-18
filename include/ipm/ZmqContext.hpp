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

#include "zmq.hpp"
#include "ers/Issue.hpp"

namespace dunedaq {

/**
 * @brief An ERS Error indicating that an exception was thrown from ZMQ
 * @param what The zmq::error_t exception message
 * @cond Doxygen doesn't like ERS macros
 */
ERS_DECLARE_ISSUE(ipm,
                  ZmqError,
                  "A zmq::error_t exception was thrown! what(): "
                                                     << what,
                  ((const char*)what)) // NOLINT
/// @endcond

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

private:
  ZmqContext() {}
  ~ZmqContext() { m_context.close(); }
  zmq::context_t m_context;

  ZmqContext(ZmqContext const&) = delete;
  ZmqContext(ZmqContext&&) = delete;
  ZmqContext& operator=(ZmqContext const&) = delete;
  ZmqContext& operator=(ZmqContext&&) = delete;
};
} // namespace ipm
} // namespace dundaq

#endif // IPM_INCLUDE_IPM_ZMQCONTEXT_HPP_
