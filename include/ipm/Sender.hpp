/**
 * @file Sender.hpp Sender Class Interface
 *
 * Sender defines the interface of objects which can send messages
 * between processes
 *
 * Implementor of this interface is required to:
 *
 * - Implement the protected virtual send_ function, called by public non-virtual send
 * - Implement the public virtual can_send function
 * - Implement the public virtual connect_for_sends function
 *
 * And is encouraged to:
 *
 * - Meaningfully implement the timeout feature in send_, and have it
 *   throw the SendTimeoutExpired exception if it occurs
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef IPM_INCLUDE_IPM_SENDER_HPP_
#define IPM_INCLUDE_IPM_SENDER_HPP_

#include "cetlib/BasicPluginFactory.h"
#include "cetlib/compiler_macros.h"
#include "ers/Issue.hpp"
#include "nlohmann/json.hpp"
#include "opmonlib/InfoCollector.hpp"

#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace dunedaq {
// Disable coverage collection LCOV_EXCL_START
ERS_DECLARE_ISSUE(ipm, KnownStateForbidsSend, "Sender not in a state to send data", )
ERS_DECLARE_ISSUE(ipm, NullPointerPassedToSend, "An null pointer to memory was passed to Sender::send", )
ERS_DECLARE_ISSUE(ipm,
                  SendTimeoutExpired,
                  "Unable to send within timeout period (timeout period was " << timeout << " milliseconds)",
                  ((int)timeout)) // NOLINT

// Reenable coverage collection LCOV_EXCL_STOP
} // namespace dunedaq

#ifndef EXTERN_C_FUNC_DECLARE_START
// NOLINTNEXTLINE(build/define_used)
#define EXTERN_C_FUNC_DECLARE_START                                                                                    \
  extern "C"                                                                                                           \
  {
#endif

/**
 * @brief Declare the function that will be called by the plugin loader
 * @param klass Class to be defined as a DUNE IPM Sender
 */
// NOLINTNEXTLINE(build/define_used)
#define DEFINE_DUNE_IPM_SENDER(klass)                                                                                  \
  EXTERN_C_FUNC_DECLARE_START                                                                                          \
  std::shared_ptr<dunedaq::ipm::Sender> make() { return std::shared_ptr<dunedaq::ipm::Sender>(new klass()); }          \
  }

namespace dunedaq::ipm {

class Sender
{

public:
  using duration_t = std::chrono::milliseconds;
  static constexpr duration_t s_block = duration_t::max();
  static constexpr duration_t s_no_block = duration_t::zero();

  using message_size_t = int;

  Sender() = default;
  virtual ~Sender() = default;

  virtual void connect_for_sends(const nlohmann::json& connection_info) = 0;

  virtual bool can_send() const noexcept = 0;

  // send() will perform some universally-desirable checks before calling user-implemented send_()
  // -Throws KnownStateForbidsSend if can_send() == false
  // -Throws NullPointerPassedToSend if message is a null pointer
  // -If message_size == 0, function is a no-op

  bool send(const void* message,
            message_size_t message_size,
            const duration_t& timeout,
            std::string const& metadata = "",
            bool noexcept_mode = false);

  Sender(const Sender&) = delete;
  Sender& operator=(const Sender&) = delete;

  Sender(Sender&&) = delete;
  Sender& operator=(Sender&&) = delete;

  void get_info(opmonlib::InfoCollector& ci, int /*level*/);

protected:
  virtual bool send_(const void* message,
                     message_size_t N,
                     const duration_t& timeout,
                     std::string const& metadata,
                     bool noexcept_mode) = 0;

private:
  mutable std::atomic<size_t> m_bytes = { 0 };
  mutable std::atomic<size_t> m_messages = { 0 };
};

inline std::shared_ptr<Sender>
make_ipm_sender(std::string const& plugin_name)
{
  static cet::BasicPluginFactory bpf("duneIPM", "make");
  return bpf.makePlugin<std::shared_ptr<Sender>>(plugin_name);
}

} // namespace dunedaq::ipm

#endif // IPM_INCLUDE_IPM_SENDER_HPP_
