/**
 * @file ZmqReceiver_test.cxx ZmqReceiver class Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ipm/Receiver.hpp"
#include "ipm/Subscriber.hpp"
#include "ipm/ZmqContext.hpp"

#define BOOST_TEST_MODULE ZmqReceiver_test // NOLINT

#include "boost/test/unit_test.hpp"

#include <string>
#include <vector>

using namespace dunedaq::ipm;

BOOST_AUTO_TEST_SUITE(ZmqReceiver_test)

BOOST_AUTO_TEST_CASE(BasicTests)
{
  auto the_receiver = make_ipm_receiver("ZmqReceiver");
  BOOST_REQUIRE(the_receiver != nullptr);
  BOOST_REQUIRE(!the_receiver->can_receive());
}

BOOST_AUTO_TEST_CASE(Subscribe) {

  auto the_receiver = make_ipm_subscriber("ZmqReceiver");
  BOOST_REQUIRE(the_receiver != nullptr);
  BOOST_REQUIRE(!the_receiver->can_receive());
  the_receiver->subscribe("foo"); // Should not throw
  the_receiver->unsubscribe("foo"); // Should not throw
}

BOOST_AUTO_TEST_CASE(Exceptions)
{
  auto the_receiver = make_ipm_receiver("ZmqReceiver");
  BOOST_REQUIRE(the_receiver != nullptr);

  nlohmann::json config_json;
  config_json["connection_string"] = "invalid_connection_string";
  BOOST_REQUIRE_EXCEPTION(the_receiver->connect_for_receives(config_json),
                          ZmqReceiverConnectError,
                          [&](ZmqReceiverConnectError const&) { return true; });
}

BOOST_AUTO_TEST_SUITE_END()
