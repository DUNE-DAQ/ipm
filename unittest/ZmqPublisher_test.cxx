/**
 * @file ZmqPublisher_test.cxx ZmqPublisher class Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ipm/Sender.hpp"

#define BOOST_TEST_MODULE ZmqPublisher_test // NOLINT

#include "boost/test/unit_test.hpp"

#include <string>
#include <vector>

using namespace dunedaq::ipm;

BOOST_AUTO_TEST_SUITE(ZmqPublisher_test)

BOOST_AUTO_TEST_CASE(BasicTests)
{
  auto the_sender = make_ipm_sender("ZmqPublisher");
  BOOST_REQUIRE(the_sender != nullptr);
  BOOST_REQUIRE(!the_sender->can_send());
}

BOOST_AUTO_TEST_SUITE_END()
