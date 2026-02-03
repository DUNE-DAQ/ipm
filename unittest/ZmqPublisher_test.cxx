/**
 * @file ZmqPublisher_test.cxx ZmqPublisher class Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ipm/Sender.hpp"
#include "ipm/ZmqContext.hpp"

#define BOOST_TEST_MODULE ZmqPublisher_test // NOLINT

#include "boost/test/unit_test.hpp"
#include "nlohmann/json.hpp"

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

BOOST_AUTO_TEST_CASE(Errors)
{
  auto the_sender = make_ipm_sender("ZmqPublisher");
  BOOST_REQUIRE(the_sender != nullptr);
  BOOST_REQUIRE(!the_sender->can_send());

  Sender::ConnectionInfo config;

  config.connection_string = "not a uri";
  BOOST_REQUIRE_EXCEPTION(the_sender->connect_for_sends(config), ZmqOperationError, [&](ZmqOperationError e) {
    return std::string(e.what()).find("invalid URI") != std::string::npos;
  });
  BOOST_REQUIRE(!the_sender->can_send());

  config.connection_string = "tcp://thishostddoesnotexist";
  BOOST_REQUIRE_EXCEPTION(the_sender->connect_for_sends(config), ZmqOperationError, [&](ZmqOperationError e) {
    return std::string(e.what()).find("Unable to resolve connection_string") != std::string::npos;
  });
  BOOST_REQUIRE(!the_sender->can_send());

  config.connection_string = "badproto://default";
  BOOST_REQUIRE_EXCEPTION(the_sender->connect_for_sends(config), ZmqOperationError, [&](ZmqOperationError e) {
    return std::string(e.what()).find("while calling bind on the ZMQ send socket") != std::string::npos;
  });
  BOOST_REQUIRE(!the_sender->can_send());
}

BOOST_AUTO_TEST_SUITE_END()
