/**
 * @file Subscriber_test.cxx Subscriber class Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ipm/Subscriber.hpp"

#define BOOST_TEST_MODULE Subscriber_test // NOLINT

#include "boost/test/unit_test.hpp"

#include <set>
#include <string>
#include <vector>

using namespace dunedaq::ipm;

BOOST_AUTO_TEST_SUITE(Subscriber_test)

namespace {

class SubscriberImpl : public Subscriber
{

public:
  static const message_size_t s_bytes_on_each_receive = 10;

  SubscriberImpl()
    : m_can_receive(false)
    , m_subscriptions()
  {}

  void connect_for_receives(const nlohmann::json& /* connection_info */) { m_can_receive = true; }
  bool can_receive() const noexcept override { return m_can_receive; }
  void sabotage_my_receiving_ability() { m_can_receive = false; }

  void subscribe(std::string const& topic) override { m_subscriptions.insert(topic); }
  void unsubscribe(std::string const& topic) override { m_subscriptions.erase(topic); }

  std::set<std::string> get_subscriptions() const { return m_subscriptions; }

protected:
  Receiver::Response receive_(const duration_t& /* timeout */) override
  {
    Receiver::Response output;
    output.data = std::vector<char>(s_bytes_on_each_receive, 'A');
    output.metadata = "TEST";

    return output;
  }

private:
  bool m_can_receive;
  std::set<std::string> m_subscriptions;
};

} // namespace ""

BOOST_AUTO_TEST_CASE(CopyAndMoveSemantics)
{
  BOOST_REQUIRE(!std::is_copy_constructible_v<SubscriberImpl>);
  BOOST_REQUIRE(!std::is_copy_assignable_v<SubscriberImpl>);
  BOOST_REQUIRE(!std::is_move_constructible_v<SubscriberImpl>);
  BOOST_REQUIRE(!std::is_move_assignable_v<SubscriberImpl>);
}

BOOST_AUTO_TEST_CASE(StatusChecks)
{
  SubscriberImpl the_subscriber;

  BOOST_REQUIRE(!the_subscriber.can_receive());

  nlohmann::json j;
  the_subscriber.connect_for_receives(j);
  BOOST_REQUIRE(the_subscriber.can_receive());

  the_subscriber.subscribe("TEST");
  auto subs = the_subscriber.get_subscriptions();
  BOOST_REQUIRE_EQUAL(subs.size(), 1);
  BOOST_REQUIRE_EQUAL(subs.count("TEST"), 1);

  the_subscriber.unsubscribe("TEST");
  subs = the_subscriber.get_subscriptions();
  BOOST_REQUIRE_EQUAL(subs.size(), 0);

  BOOST_REQUIRE_NO_THROW(the_subscriber.receive(Subscriber::s_no_block));
  BOOST_REQUIRE_NO_THROW(the_subscriber.receive(Subscriber::s_no_block, SubscriberImpl::s_bytes_on_each_receive));

  BOOST_REQUIRE_EXCEPTION(the_subscriber.receive(Subscriber::s_no_block, SubscriberImpl::s_bytes_on_each_receive - 1),
                          dunedaq::ipm::UnexpectedNumberOfBytes,
                          [&](dunedaq::ipm::UnexpectedNumberOfBytes) { return true; });

  the_subscriber.sabotage_my_receiving_ability();
  BOOST_REQUIRE(!the_subscriber.can_receive());

  BOOST_REQUIRE_EXCEPTION(the_subscriber.receive(Subscriber::s_no_block),
                          dunedaq::ipm::KnownStateForbidsReceive,
                          [&](dunedaq::ipm::KnownStateForbidsReceive) { return true; });
}

BOOST_AUTO_TEST_SUITE_END()
