/**
 * @file Sender.hpp Sender Class implementations
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ipm/Sender.hpp"
#include "ipm/senderinfo/InfoNljs.hpp"


#include <string>
#include <vector>

void
dunedaq::ipm::Sender::send(const void* message,
                           message_size_t message_size,
                           const duration_t& timeout,
                           std::string const& metadata)
{
  if (message_size == 0) {
    return;
  }

  if (!can_send()) {
    throw KnownStateForbidsSend(ERS_HERE);
  }

  if (!message) {
    throw NullPointerPassedToSend(ERS_HERE);
  }

  send_(message, message_size, timeout, metadata);

  m_bytes+= message_size;
  ++m_messages;
}


void
dunedaq::ipm::Sender::get_info(opmonlib::InfoCollector& ci, int /*level*/) {

  senderinfo::Info i;

  i.bytes = m_bytes.exchange(0);
  i.messages = m_messages.exchange(0);

  ci.add(i);
}
