/**
 * @file Sender.hpp Sender Class implementations
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ipm/Sender.hpp"
#include "ipm/opmon/ipm.pb.h"

#include <string>
#include <vector>

bool
dunedaq::ipm::Sender::send(const void* message,
                           message_size_t message_size,
                           const duration_t& timeout,
                           std::string const& metadata,
                           bool no_tmoexcept_mode)
{
  if (message_size == 0) {
    return true;
  }

  if (!can_send()) {
    throw KnownStateForbidsSend(ERS_HERE);
  }

  if (!message) {
    throw NullPointerPassedToSend(ERS_HERE);
  }

  auto res = send_(message, message_size, timeout, metadata, no_tmoexcept_mode);

  m_bytes += message_size;
  ++m_messages;

  return res;
}

void
dunedaq::ipm::Sender::generate_opmon_data()
{

  opmon::SenderInfo i;

  i.set_bytes(m_bytes.exchange(0));
  i.set_messages(m_messages.exchange(0));

  publish(std::move(i));
}
