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
  ZmqPublisher()
    : ZmqSenderImpl(ZmqSenderImpl::SenderType::Publisher)
  {}
};

} // namespace ipm
} // namespace dunedaq

DEFINE_DUNE_IPM_SENDER(dunedaq::ipm::ZmqPublisher)
