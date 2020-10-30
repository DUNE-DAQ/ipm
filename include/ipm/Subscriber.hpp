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

#include "ers/Issue.h"
#include "nlohmann/json.hpp"

#include <cetlib/BasicPluginFactory.h>
#include <cetlib/compiler_macros.h>

#include <memory>
#include <string>
#include <vector>

namespace dunedaq::ipm {

class Subscriber : public Receiver
{

public:
  using duration_type = std::chrono::milliseconds;
  static constexpr duration_type block = std::chrono::duration_values<duration_type>::max();
  static constexpr duration_type noblock = std::chrono::duration_values<duration_type>::zero();

  using size_type = int;
  static constexpr size_type anysize =
    0; // Since "I want 0 bytes" is pointless, "0" denotes "I don't care about the size"

  Subscriber() = default;

  void connect_for_receives(const nlohmann::json& connection_info) final
  {
    connect_for_subscribes(connection_info);
    subscribe("");
  }
  virtual void connect_for_subscribes(const nlohmann::json& connection_info) = 0;

  // receive() will perform some universally-desirable checks before calling user-implemented receive_:
  // -Throws KnownStateForbidsReceive if can_receive() == false
  // -Throws UnexpectedNumberOfBytes if the "nbytes" argument isn't anysize, and the
  //  received bytes inside the function aren't the same number as nbytes

  std::vector<char> receive(const duration_type& timeout, std::string& topic, size_type nbytes = anysize);

  virtual void subscribe(std::string const& topic) = 0;
  virtual void unsubscribe(std::string const& topic) = 0;

  Subscriber(const Subscriber&) = delete;
  Subscriber& operator=(const Subscriber&) = delete;

  Subscriber(Subscriber&&) = delete;
  Subscriber& operator=(Subscriber&&) = delete;

protected:
  std::vector<char> receive_(const duration_type& timeout) final;
  virtual std::vector<char> receive_(const duration_type& timeout, std::string& topic) = 0;
};

inline std::vector<char>
Subscriber::receive(const duration_type& timeout, std::string& topic, size_type bytes)
{
  if (!can_receive()) {
    throw KnownStateForbidsReceive(ERS_HERE);
  }
  std::vector<char> message = receive_(timeout, topic);

  if (bytes != anysize) {
    auto received_size = static_cast<size_type>(message.size());
    if (received_size != bytes) {
      throw UnexpectedNumberOfBytes(ERS_HERE, received_size, bytes);
    }
  }

  return message;
}

inline std::vector<char>
Subscriber::receive_(const duration_type& timeout)
{
  std::string topic = "";
  return receive_(timeout, topic);
}

std::shared_ptr<Subscriber>
makeIPMSubscriber(std::string const& plugin_name)
{
  static cet::BasicPluginFactory bpf("duneIPM", "make");
  auto receiver_ptr = bpf.makePlugin<std::shared_ptr<Receiver>>(plugin_name);
  return std::dynamic_pointer_cast<Subscriber>(receiver_ptr);
}

} // namespace dunedaq::ipm

#endif // IPM_INCLUDE_IPM_SUBSCRIBER_HPP_
