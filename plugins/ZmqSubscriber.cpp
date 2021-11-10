
/**
 *
 * @file ZmqSubscriber.cpp ZmqSubscriber messaging class definitions
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ZmqReceiverImpl.hpp"

#include "logging/Logging.hpp"

namespace dunedaq {
namespace ipm {

class ZmqSubscriber : public ZmqReceiverImpl
{
public:
  ZmqSubscriber()
    : ZmqReceiverImpl(ZmqReceiverImpl::ReceiverType::Subscriber)
  {}
};
} // namespace ipm
} // namespace dunedaq

DEFINE_DUNE_IPM_RECEIVER(dunedaq::ipm::ZmqSubscriber)
