/**
 * @file Receiver_test.cxx Receiver class Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "CallbackAdapter.hpp"
#include "ipm/Receiver.hpp"

#define BOOST_TEST_MODULE Receiver_test // NOLINT

#include "boost/test/unit_test.hpp"

#include <string>
#include <vector>

using namespace dunedaq::ipm;

BOOST_AUTO_TEST_SUITE(Receiver_test)

namespace {

class ReceiverImpl : public Receiver
{

public:
  static const message_size_t s_bytes_on_each_receive = 10;

  ReceiverImpl()
    : m_can_receive(false)
  {
  }

  void register_callback(std::function<void(Response&)> callback) { m_callback_adapter.set_callback(callback); }
  void unregister_callback() { m_callback_adapter.clear_callback(); }

  std::string connect_for_receives(const nlohmann::json& /* connection_info */)
  {
    m_can_receive = true;
    m_callback_adapter.set_receiver(this);
    return "";
  }
  bool can_receive() const noexcept override { return m_can_receive; }
  void sabotage_my_receiving_ability()
  {
    unregister_callback();
    m_can_receive = false;
  }

protected:
  Receiver::Response receive_(const duration_t& /* timeout */, bool /*no_tmoexcept_mode*/) override
  {
    Receiver::Response output;
    output.data = std::vector<char>(s_bytes_on_each_receive, 'A');
    output.metadata = "";
    return output;
  }

private:
  bool m_can_receive;
  CallbackAdapter m_callback_adapter;
};

} // namespace ""

BOOST_AUTO_TEST_CASE(CopyAndMoveSemantics)
{
  BOOST_REQUIRE(!std::is_copy_constructible_v<ReceiverImpl>);
  BOOST_REQUIRE(!std::is_copy_assignable_v<ReceiverImpl>);
  BOOST_REQUIRE(!std::is_move_constructible_v<ReceiverImpl>);
  BOOST_REQUIRE(!std::is_move_assignable_v<ReceiverImpl>);
}

BOOST_AUTO_TEST_CASE(Timeouts)
{
  BOOST_REQUIRE_EQUAL(Receiver::s_no_block.count(), 0);
  BOOST_REQUIRE_GT(Receiver::s_block.count(), 3600000); // "Blocking" must be at least an hour!
}

BOOST_AUTO_TEST_CASE(StatusChecks)
{
  ReceiverImpl the_receiver;

  BOOST_REQUIRE(!the_receiver.can_receive());

  nlohmann::json j;
  the_receiver.connect_for_receives(j);
  BOOST_REQUIRE(the_receiver.can_receive());

  BOOST_REQUIRE_NO_THROW(the_receiver.receive(Receiver::s_no_block));
  BOOST_REQUIRE_NO_THROW(the_receiver.receive(Receiver::s_no_block, ReceiverImpl::s_bytes_on_each_receive));

  BOOST_REQUIRE_EXCEPTION(the_receiver.receive(Receiver::s_no_block, ReceiverImpl::s_bytes_on_each_receive - 1),
                          dunedaq::ipm::UnexpectedNumberOfBytes,
                          [&](dunedaq::ipm::UnexpectedNumberOfBytes) { return true; });

  the_receiver.sabotage_my_receiving_ability();
  BOOST_REQUIRE(!the_receiver.can_receive());

  BOOST_REQUIRE_EXCEPTION(the_receiver.receive(Receiver::s_no_block),
                          dunedaq::ipm::KnownStateForbidsReceive,
                          [&](dunedaq::ipm::KnownStateForbidsReceive) { return true; });
}

BOOST_AUTO_TEST_CASE(Callback)
{
  ReceiverImpl the_receiver;

  nlohmann::json j;
  the_receiver.connect_for_receives(j);
  BOOST_REQUIRE(the_receiver.can_receive());

  std::atomic<size_t> callback_call_count = 0;
  auto callback_fun = [&](Receiver::Response&) { callback_call_count++; }; // NOLINT

  the_receiver.register_callback(callback_fun);
  usleep(10000);
  the_receiver.unregister_callback();

  BOOST_REQUIRE_GT(callback_call_count, 0);
}

BOOST_AUTO_TEST_SUITE_END()
