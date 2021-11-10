/**
 * @file ZmqSubscriber_test.cxx ZmqSubscriber class Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ipm/Subscriber.hpp"

#define BOOST_TEST_MODULE ZmqSubscriber_test // NOLINT

#include "boost/test/unit_test.hpp"

#include <string>
#include <vector>

using namespace dunedaq::ipm;

BOOST_AUTO_TEST_SUITE(ZmqSubscriber_test)

BOOST_AUTO_TEST_CASE(BasicTests)
{
  auto the_subscriber = make_ipm_subscriber("ZmqSubscriber");
  BOOST_REQUIRE(the_subscriber != nullptr);
  BOOST_REQUIRE(!the_subscriber->can_receive());

  auto the_receiver = make_ipm_receiver("ZmqSubscriber");
  BOOST_REQUIRE(the_receiver != nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
