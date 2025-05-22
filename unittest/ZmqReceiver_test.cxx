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

BOOST_AUTO_TEST_CASE(Subscribe)
{
  auto the_receiver = make_ipm_subscriber("ZmqReceiver");
  BOOST_REQUIRE(the_receiver == nullptr);
}

BOOST_AUTO_TEST_CASE(Errors)
{
  auto the_receiver = make_ipm_receiver("ZmqReceiver");
  BOOST_REQUIRE(the_receiver != nullptr);
  BOOST_REQUIRE(!the_receiver->can_receive());

  nlohmann::json config_json;

  config_json["connection_string"] = "not a uri";
  BOOST_REQUIRE_EXCEPTION(the_receiver->connect_for_receives(config_json), ZmqOperationError, [&](ZmqOperationError e) {
    return std::string(e.what()).find("invalid URI") != std::string::npos;
  });
  BOOST_REQUIRE(!the_receiver->can_receive());

  config_json["connection_string"] = "tcp://thishostddoesnotexist";
  BOOST_REQUIRE_EXCEPTION(the_receiver->connect_for_receives(config_json), ZmqOperationError, [&](ZmqOperationError e) {
    return std::string(e.what()).find("Unable to resolve connection_string") != std::string::npos;
  });
  BOOST_REQUIRE(!the_receiver->can_receive());

  config_json["connection_string"] = "badproto://default";
  BOOST_REQUIRE_EXCEPTION(the_receiver->connect_for_receives(config_json), ZmqOperationError, [&](ZmqOperationError e) {
    return std::string(e.what()).find("while calling bind on the ZMQ receive socket") != std::string::npos;
  });
  BOOST_REQUIRE(!the_receiver->can_receive());
}
BOOST_AUTO_TEST_SUITE_END()
