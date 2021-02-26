/**
 * @file Subscriber.hpp Subscriber Class Interface
 *
 * Subscriber defines the interface of objects which can receive messages
 * between processes that are sent to topics that have been subscribed to.
 * Subscribers can also be Receivers
 *
 * Implementor of this interface is required to:
 *
 * - Implement the protected virtual receive_ function, called by public non-virtual receive
 * - Implement the public virtual can_receive function
 * - Implement the public virtual connect_for_receives function
 * - Implement the public virtual subscribe function
 * - Implement the public virtual unsubscribe function
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

#ifndef IPM_INCLUDE_IPM_SUBSCRIBER_HPP_
#define IPM_INCLUDE_IPM_SUBSCRIBER_HPP_

#include "ipm/Receiver.hpp"

#include "cetlib/BasicPluginFactory.h"
#include "cetlib/compiler_macros.h"
#include "ers/Issue.hpp"
#include "nlohmann/json.hpp"

#include <memory>
#include <string>
#include <vector>

namespace dunedaq::ipm {

class Subscriber : public Receiver
{

public:
  Subscriber() = default;

  virtual void subscribe(std::string const& topic) = 0;
  virtual void unsubscribe(std::string const& topic) = 0;

  Subscriber(const Subscriber&) = delete;
  Subscriber& operator=(const Subscriber&) = delete;

  Subscriber(Subscriber&&) = delete;
  Subscriber& operator=(Subscriber&&) = delete;
};

inline std::shared_ptr<Subscriber>
make_ipm_subscriber(std::string const& plugin_name)
{
  static cet::BasicPluginFactory bpf("duneIPM", "make");
  auto receiver_ptr = bpf.makePlugin<std::shared_ptr<Receiver>>(plugin_name);
  return std::dynamic_pointer_cast<Subscriber>(receiver_ptr);
}

} // namespace dunedaq::ipm

#endif // IPM_INCLUDE_IPM_SUBSCRIBER_HPP_
