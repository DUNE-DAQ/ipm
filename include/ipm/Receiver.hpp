/**
 * @file Receiver.hpp Receiver Class Interface
 *
 * Receiver defines the interface of objects which can receive messages
 * between processes
 *
 * Implementor of this interface is required to:
 *
 * - Implement the protected virtual receive_ function, called by public non-virtual receive
 * - Implement the public virtual can_receive function
 * - Implement the public virtual connect_for_receives function
 *
 * And is encouraged to:
 *
 * - Meaningfully implement the timeout feature in receive_, and have it
 *   throw the ReceiveTimeoutExpired exception if it occurs
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef IPM_INCLUDE_IPM_RECEIVER_HPP_
#define IPM_INCLUDE_IPM_RECEIVER_HPP_

#include "cetlib/BasicPluginFactory.h"
#include "cetlib/compiler_macros.h"
#include "ers/Issue.hpp"
#include "nlohmann/json.hpp"

#include <memory>
#include <string>
#include <vector>

namespace dunedaq {
ERS_DECLARE_ISSUE(ipm, KnownStateForbidsReceive, "Receiver not in a state to receive data", )
ERS_DECLARE_ISSUE(ipm,
                  UnexpectedNumberOfBytes,
                  "Expected " << bytes1 << " bytes in message but received " << bytes2,
                  ((int)bytes1)((int)bytes2)) // NOLINT
ERS_DECLARE_ISSUE(ipm,
                  ReceiveTimeoutExpired,
                  "Unable to receive within timeout period (timeout period was " << timeout << " milliseconds)",
                  ((int)timeout)) // NOLINT
} // namespace dunedaq

#ifndef EXTERN_C_FUNC_DECLARE_START
// NOLINTNEXTLINE(build/define_used)
#define EXTERN_C_FUNC_DECLARE_START                                                                                    \
  extern "C"                                                                                                           \
  {
#endif

/**
 * @brief Declare the function that will be called by the plugin loader
 * @param klass Class to be defined as a DUNE IPM Receiver
 */
// NOLINTNEXTLINE(build/define_used)
#define DEFINE_DUNE_IPM_RECEIVER(klass)                                                                                \
  EXTERN_C_FUNC_DECLARE_START                                                                                          \
  std::shared_ptr<dunedaq::ipm::Receiver> make() { return std::shared_ptr<dunedaq::ipm::Receiver>(new klass()); }      \
  }

namespace dunedaq::ipm {

class Receiver
{

public:
  using duration_t = std::chrono::milliseconds;
  static constexpr duration_t s_block = std::chrono::duration_values<duration_t>::max();
  static constexpr duration_t s_no_block = std::chrono::duration_values<duration_t>::zero();

  using message_size_t = int;
  static constexpr message_size_t s_any_size =
    0; // Since "I want 0 bytes" is pointless, "0" denotes "I don't care about the size"

  Receiver() = default;

  virtual ~Receiver() = default;

  virtual void connect_for_receives(const nlohmann::json& connection_info) = 0;

  virtual bool can_receive() const noexcept = 0;

  // receive() will perform some universally-desirable checks before calling user-implemented receive_:
  // -Throws KnownStateForbidsReceive if can_receive() == false
  // -Throws UnexpectedNumberOfBytes if the "nbytes" argument isn't anysize, and the
  //  received bytes inside the function aren't the same number as nbytes

  struct Response
  {
    std::string metadata{ "" };
    std::vector<char> data{};
  };

  Response receive(const duration_t& timeout, message_size_t num_bytes = s_any_size);

  Receiver(const Receiver&) = delete;
  Receiver& operator=(const Receiver&) = delete;

  Receiver(Receiver&&) = delete;
  Receiver& operator=(Receiver&&) = delete;

protected:
  virtual Response receive_(const duration_t& timeout) = 0;
};

inline std::shared_ptr<Receiver>
make_ipm_receiver(std::string const& plugin_name)
{
  static cet::BasicPluginFactory bpf("duneIPM", "make");
  return bpf.makePlugin<std::shared_ptr<Receiver>>(plugin_name);
}

} // namespace dunedaq::ipm

#endif // IPM_INCLUDE_IPM_RECEIVER_HPP_
