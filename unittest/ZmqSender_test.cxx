/**
 * @file ZmqSender_test.cxx ZmqSender class Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ipm/Sender.hpp"
#include "ipm/ZmqContext.hpp"

#define BOOST_TEST_MODULE ZmqSender_test // NOLINT

#include "boost/test/unit_test.hpp"

#include <string>
#include <vector>

using namespace dunedaq::ipm;

BOOST_AUTO_TEST_SUITE(ZmqSender_test)

BOOST_AUTO_TEST_CASE(BasicTests)
{
  auto the_sender = make_ipm_sender("ZmqSender");
  BOOST_REQUIRE(the_sender != nullptr);
  BOOST_REQUIRE(!the_sender->can_send());
}

BOOST_AUTO_TEST_CASE(Exceptions)
{
  auto the_sender = make_ipm_sender("ZmqSender");
  BOOST_REQUIRE(the_sender != nullptr);

  nlohmann::json config_json;
  config_json["connection_string"] = "invalid_connection_string";
  BOOST_REQUIRE_EXCEPTION(
    the_sender->connect_for_sends(config_json), ZmqSenderBindError, [&](ZmqSenderBindError const&) { return true; });
}
BOOST_AUTO_TEST_SUITE_END()
