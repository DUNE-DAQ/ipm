/**
 * @file Receiver.hpp Receiver Class implementations
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ipm/Receiver.hpp"
#include "ipm/IPMInfo.pb.h"

dunedaq::ipm::Receiver::Response
dunedaq::ipm::Receiver::receive(const duration_t& timeout, message_size_t bytes, bool no_tmoexcept_mode)
{
  if (!can_receive()) {
    throw KnownStateForbidsReceive(ERS_HERE);
  }
  auto message = receive_(timeout, no_tmoexcept_mode);

  if (bytes != s_any_size) {
    auto received_size = static_cast<message_size_t>(message.data.size());
    if (received_size != bytes) {
      throw UnexpectedNumberOfBytes(ERS_HERE, received_size, bytes);
    }
  }

  m_bytes += message.data.size();
  ++m_messages;

  return message;
}

void
dunedaq::ipm::Receiver::generate_opmon_data()
{

  opmon::ReceiverInfo i;

  i.set_bytes(m_bytes.exchange(0));
  i.set_messages(m_messages.exchange(0));

  publish(std::move(i));
}
