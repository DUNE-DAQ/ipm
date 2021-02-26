/**
 * @file ZmqSendReceive_test.cxx Test ZmqSender to ZmqReceiver transfer
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ipm/Receiver.hpp"
#include "ipm/Sender.hpp"

#define BOOST_TEST_MODULE ZmqSendReceive_test // NOLINT

#include "boost/test/unit_test.hpp"

#include <string>
#include <vector>

using namespace dunedaq::ipm;

BOOST_AUTO_TEST_SUITE(ZmqSendReceive_test)

BOOST_AUTO_TEST_CASE(SendReceiveTest)
{
  auto the_receiver = make_ipm_receiver("ZmqReceiver");
  BOOST_REQUIRE(the_receiver != nullptr);
  BOOST_REQUIRE(!the_receiver->can_receive());

  auto the_sender = make_ipm_sender("ZmqSender");
  BOOST_REQUIRE(the_sender != nullptr);
  BOOST_REQUIRE(!the_sender->can_send());

  nlohmann::json empty_json = nlohmann::json::object();
  the_receiver->connect_for_receives(empty_json);
  the_sender->connect_for_sends(empty_json);

  BOOST_REQUIRE(the_receiver->can_receive());
  BOOST_REQUIRE(the_sender->can_send());

  std::vector<char> test_data{ 'T', 'E', 'S', 'T' };

  the_sender->send(test_data.data(), test_data.size(), Sender::s_no_block);
  auto response = the_receiver->receive(Receiver::s_block);
  BOOST_REQUIRE_EQUAL(response.data.size(), 4);
  BOOST_REQUIRE_EQUAL(response.data[0], 'T');
  BOOST_REQUIRE_EQUAL(response.data[1], 'E');
  BOOST_REQUIRE_EQUAL(response.data[2], 'S');
  BOOST_REQUIRE_EQUAL(response.data[3], 'T');
}

BOOST_AUTO_TEST_SUITE_END()
