/**
 *
 * @file ZmqPublisher.cpp ZmqPublisher messaging class definitions
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ZmqSenderImpl.hpp"

#include "logging/Logging.hpp"

namespace dunedaq {
namespace ipm {
class ZmqPublisher : public ZmqSenderImpl
{
public:
  ZmqPublisher(bool resolve_ips)
    : ZmqSenderImpl(ZmqSenderImpl::SenderType::Publisher, resolve_ips)
  {}
};

} // namespace ipm
} // namespace dunedaq

DEFINE_DUNE_IPM_SENDER(dunedaq::ipm::ZmqPublisher)
