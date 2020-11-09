/**
 * @file ZmqSender_test.cxx ZmqSender class Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ipm/Sender.hpp"

#define BOOST_TEST_MODULE ZmqSender_test // NOLINT

#include <boost/test/unit_test.hpp>
#include <string>
#include <vector>

using namespace dunedaq::ipm;

BOOST_AUTO_TEST_SUITE(ZmqSender_test)

BOOST_AUTO_TEST_CASE(BasicTests)
{
  auto theSender = makeIPMSender("ZmqSender");
  BOOST_REQUIRE(theSender != nullptr);
  BOOST_REQUIRE(!theSender->can_send());
}


BOOST_AUTO_TEST_SUITE_END()
