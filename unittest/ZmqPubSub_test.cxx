/**
 * @file ZmqPubSub_test.cxx Test ZmqPublisher to ZmqSubscriber transfer
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ipm/Sender.hpp"
#include "ipm/Subscriber.hpp"

#include "logging/Logging.hpp"

#define BOOST_TEST_MODULE ZmqPubSub_test // NOLINT

#include "boost/test/unit_test.hpp"

#include <string>
#include <vector>

using namespace dunedaq::ipm;

BOOST_AUTO_TEST_SUITE(ZmqPubSub_test)

size_t
elapsed_time_milliseconds(std::chrono::steady_clock::time_point const& then)
{
  return static_cast<size_t>(
    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - then).count());
}

BOOST_AUTO_TEST_CASE(SendReceiveTest)
{
  auto the_receiver = make_ipm_subscriber("ZmqSubscriber");
  BOOST_REQUIRE(the_receiver != nullptr);
  BOOST_REQUIRE(!the_receiver->can_receive());

  auto the_sender = make_ipm_sender("ZmqPublisher");
  BOOST_REQUIRE(the_sender != nullptr);
  BOOST_REQUIRE(!the_sender->can_send());

  nlohmann::json config_json;
  config_json["connection_string"] = "inproc://default";
  the_sender->connect_for_sends(config_json);
  the_receiver->connect_for_receives(config_json);

  BOOST_REQUIRE(the_receiver->can_receive());
  BOOST_REQUIRE(the_sender->can_send());

  the_receiver->subscribe("testTopic");

  std::vector<char> test_data{ 'T', 'E', 'S', 'T' };

  the_sender->send(test_data.data(), test_data.size(), Sender::s_no_block, "ignoredTopic");
  auto before_recv = std::chrono::steady_clock::now();
  BOOST_REQUIRE_EXCEPTION(
    the_receiver->receive(std::chrono::milliseconds(100)),
    dunedaq::ipm::ReceiveTimeoutExpired,
    [&](dunedaq::ipm::ReceiveTimeoutExpired) { return elapsed_time_milliseconds(before_recv) >= 100; });

  the_sender->send(test_data.data(), test_data.size(), Sender::s_no_block, "testTopic");
  auto response = the_receiver->receive(Receiver::s_block);
  BOOST_REQUIRE_EQUAL(response.data.size(), 4);
  BOOST_REQUIRE_EQUAL(response.data[0], 'T');
  BOOST_REQUIRE_EQUAL(response.data[1], 'E');
  BOOST_REQUIRE_EQUAL(response.data[2], 'S');
  BOOST_REQUIRE_EQUAL(response.data[3], 'T');

  the_receiver->unsubscribe("testTopic");
  the_sender->send(test_data.data(), test_data.size(), Sender::s_no_block, "testTopic");
  BOOST_REQUIRE_EXCEPTION(
    the_receiver->receive(std::chrono::milliseconds(2000)),
    dunedaq::ipm::ReceiveTimeoutExpired,
    [&](dunedaq::ipm::ReceiveTimeoutExpired) { return elapsed_time_milliseconds(before_recv) >= 2000; });
}

BOOST_AUTO_TEST_CASE(CallbackTest)
{

  auto the_receiver = make_ipm_subscriber("ZmqSubscriber");
  BOOST_REQUIRE(the_receiver != nullptr);
  BOOST_REQUIRE(!the_receiver->can_receive());

  auto the_sender = make_ipm_sender("ZmqPublisher");
  BOOST_REQUIRE(the_sender != nullptr);
  BOOST_REQUIRE(!the_sender->can_send());

  nlohmann::json config_json;
  config_json["connection_string"] = "inproc://default";
  the_sender->connect_for_sends(config_json);
  the_receiver->connect_for_receives(config_json);

  BOOST_REQUIRE(the_receiver->can_receive());
  BOOST_REQUIRE(the_sender->can_send());

  the_receiver->subscribe("testTopic");

  std::vector<char> test_data{ 'T', 'E', 'S', 'T' };
  std::atomic<bool> message_received = false;

  auto callback_fun = [&](Receiver::Response& res) {
    TLOG() << "Callback function called with res.data.size() == " << static_cast<int>(res.data.size());
    BOOST_REQUIRE_EQUAL(res.data.size(), test_data.size());
    for (size_t ii = 0; ii < res.data.size(); ++ii) {
      BOOST_REQUIRE_EQUAL(res.data[ii], test_data[ii]);
    }
    message_received = true;
  };
  the_receiver->register_callback(callback_fun);

  the_sender->send(test_data.data(), test_data.size(), Sender::s_no_block);
  usleep(100000);
  BOOST_REQUIRE_EQUAL(message_received, false);

  message_received = false;
  test_data = { 'A', 'N', 'O', 'T', 'H', 'E', 'R', ' ', 'T', 'E', 'S', 'T' };
  the_sender->send(test_data.data(), test_data.size(), Sender::s_no_block, "testTopic");
  while (!message_received.load()) {
    usleep(15000);
  }

  message_received = false;
  test_data = { 'A', ' ', 'T', 'H', 'I', 'R', 'D', ' ', 'T', 'E', 'S', 'T' };
  the_sender->send(test_data.data(), test_data.size(), Sender::s_no_block, "ignoredTopic");
  usleep(100000);
  BOOST_REQUIRE_EQUAL(message_received, false);

  the_receiver->unregister_callback();
  message_received = false;
  test_data = { 'A', ' ', 'F', 'O', 'U', 'R', 'T', 'H', ' ', 'T', 'E', 'S', 'T' };
  the_sender->send(test_data.data(), test_data.size(), Sender::s_no_block, "testTopic");

  usleep(100000);
  BOOST_REQUIRE_EQUAL(message_received, false);
  auto response = the_receiver->receive(Receiver::s_block);
  BOOST_REQUIRE_EQUAL(response.data.size(), test_data.size());
}

BOOST_AUTO_TEST_CASE(MultiplePublishers)
{
  auto first_publisher = make_ipm_sender("ZmqPublisher");
  auto second_publisher = make_ipm_sender("ZmqPublisher");
  auto the_subscriber = make_ipm_subscriber("ZmqSubscriber");

  nlohmann::json first_json, second_json, sub_json;
  first_json["connection_string"] = "inproc://foo";
  first_publisher->connect_for_sends(first_json);
  second_json["connection_string"] = "inproc://bar";
  second_publisher->connect_for_sends(second_json);
  sub_json["connection_strings"] = { "inproc://foo", "inproc://bar" };
  the_subscriber->connect_for_receives(sub_json);

  the_subscriber->subscribe("testTopic");

  std::vector<char> test_data{ 'T', 'E', 'S', 'T' };
  first_publisher->send(test_data.data(), test_data.size(), Sender::s_no_block, "testTopic");
  auto response = the_subscriber->receive(Receiver::s_block);
  BOOST_REQUIRE_EQUAL(response.data.size(), 4);
  BOOST_REQUIRE_EQUAL(response.data[0], 'T');
  BOOST_REQUIRE_EQUAL(response.data[1], 'E');
  BOOST_REQUIRE_EQUAL(response.data[2], 'S');
  BOOST_REQUIRE_EQUAL(response.data[3], 'T');

  second_publisher->send(test_data.data(), test_data.size(), Sender::s_no_block, "testTopic");
  auto response2 = the_subscriber->receive(Receiver::s_block);
  BOOST_REQUIRE_EQUAL(response2.data.size(), 4);
  BOOST_REQUIRE_EQUAL(response2.data[0], 'T');
  BOOST_REQUIRE_EQUAL(response2.data[1], 'E');
  BOOST_REQUIRE_EQUAL(response2.data[2], 'S');
  BOOST_REQUIRE_EQUAL(response2.data[3], 'T');
}

BOOST_AUTO_TEST_SUITE_END()
