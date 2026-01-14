/**
 * @file ZmqSender_test.cxx ZmqSender class Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ipm/Sender.hpp"
#include "ipm/ZmqContext.hpp"

#include "utilities/Issues.hpp"

#define BOOST_TEST_MODULE ZmqSender_test // NOLINT

#include "boost/test/unit_test.hpp"

#include <string>
#include <vector>

using namespace dunedaq::ipm;

BOOST_AUTO_TEST_SUITE(ZmqSender_test)

BOOST_AUTO_TEST_CASE(BasicTests)
{
  auto the_sender = make_ipm_sender("ZmqSender");
  BOOST_CHECK(the_sender != nullptr);
  BOOST_CHECK(!the_sender->can_send());
}

BOOST_AUTO_TEST_CASE(Errors)
{
  auto the_sender = make_ipm_sender("ZmqSender");
  BOOST_CHECK(the_sender != nullptr);
  BOOST_CHECK(!the_sender->can_send());

  nlohmann::json config_json;

  config_json["connection_string"] = "not a uri";
  BOOST_CHECK_EXCEPTION(
    the_sender->connect_for_sends(config_json), dunedaq::utilities::InvalidUri, [&](dunedaq::utilities::InvalidUri e) {
      return std::string(e.what()).find("not a uri") != std::string::npos;
    });
  BOOST_CHECK(!the_sender->can_send());

  config_json["connection_string"] = "tcp://thishostddoesnotexist";
  BOOST_CHECK_EXCEPTION(the_sender->connect_for_sends(config_json), ZmqOperationError, [&](ZmqOperationError e) {
    return std::string(e.what()).find("thishostddoesnotexist") != std::string::npos;
  });
  BOOST_CHECK(!the_sender->can_send());

  config_json["connection_string"] = "badproto://default";
  BOOST_CHECK_EXCEPTION(the_sender->connect_for_sends(config_json), ZmqOperationError, [&](ZmqOperationError e) {
    return std::string(e.what()).find("badproto") != std::string::npos;
  });
  BOOST_CHECK(!the_sender->can_send());
}
BOOST_AUTO_TEST_SUITE_END()
