/**
 * @file ZmqPublisher_test.cxx ZmqPublisher class Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ipm/Publisher.hpp"

#define BOOST_TEST_MODULE ZmqPublisher_test // NOLINT

#include <boost/test/unit_test.hpp>
#include <string>
#include <vector>

using namespace dunedaq::ipm;

BOOST_AUTO_TEST_SUITE(ZmqPublisher_test)

BOOST_AUTO_TEST_CASE(BasicTests)
{
  auto theSender = makeIPMSender("ZmqPublisher");
  BOOST_REQUIRE(theSender != nullptr);
  BOOST_REQUIRE(!theSender->can_send());

  auto thePublisher = makeIPMPublisher("ZmqPublisher");
  BOOST_REQUIRE(thePublisher != nullptr);
  BOOST_REQUIRE(!thePublisher->can_send());

}

BOOST_AUTO_TEST_SUITE_END()
