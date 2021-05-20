/**
 * @file ZmqPubSub_test.cxx Test ZmqPublisher to ZmqSubscriber transfer
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ipm/Sender.hpp"
#include "ipm/Subscriber.hpp"

#define BOOST_TEST_MODULE ZmqPubSub_test // NOLINT

#include "boost/test/unit_test.hpp"

#include <string>
#include <vector>

using namespace dunedaq::ipm;

BOOST_AUTO_TEST_SUITE(ZmqPubSub_test)

BOOST_AUTO_TEST_CASE(SendReceiveTest)
{
  auto the_receiver = make_ipm_subscriber("ZmqSubscriber");
  BOOST_REQUIRE(the_receiver != nullptr);
  BOOST_REQUIRE(!the_receiver->can_receive());

  auto the_sender = make_ipm_sender("ZmqPublisher");
  BOOST_REQUIRE(the_sender != nullptr);
  BOOST_REQUIRE(!the_sender->can_send());

  nlohmann::json empty_json = nlohmann::json::object();
  the_receiver->connect_for_receives(empty_json);
  the_sender->connect_for_sends(empty_json);

  BOOST_REQUIRE(the_receiver->can_receive());
  BOOST_REQUIRE(the_sender->can_send());

  the_receiver->subscribe("testTopic");

  std::vector<char> test_data{ 'T', 'E', 'S', 'T' };

  the_sender->send(test_data.data(), test_data.size(), Sender::s_no_block, "ignoredTopic");
  BOOST_REQUIRE_EXCEPTION(the_receiver->receive(std::chrono::milliseconds(100)),
                          dunedaq::ipm::ReceiveTimeoutExpired,
                          [&](dunedaq::ipm::ReceiveTimeoutExpired) { return true; });

  the_sender->send(test_data.data(), test_data.size(), Sender::s_no_block, "testTopic");
  auto response = the_receiver->receive(Receiver::s_block);
  BOOST_REQUIRE_EQUAL(response.data.size(), 4);
  BOOST_REQUIRE_EQUAL(response.data[0], 'T');
  BOOST_REQUIRE_EQUAL(response.data[1], 'E');
  BOOST_REQUIRE_EQUAL(response.data[2], 'S');
  BOOST_REQUIRE_EQUAL(response.data[3], 'T');

  the_receiver->unsubscribe("testTopic");
  the_sender->send(test_data.data(), test_data.size(), Sender::s_no_block, "testTopic");
  BOOST_REQUIRE_EXCEPTION(the_receiver->receive(std::chrono::milliseconds(100)),
                          dunedaq::ipm::ReceiveTimeoutExpired,
                          [&](dunedaq::ipm::ReceiveTimeoutExpired) { return true; });
}

BOOST_AUTO_TEST_SUITE_END()
