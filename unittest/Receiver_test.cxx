/**
 * @file Receiver_test.cxx Receiver class Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ipm/Receiver.hpp"

#define BOOST_TEST_MODULE Receiver_test // NOLINT

#include <boost/test/unit_test.hpp>
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
  {}

  void connect_for_receives(const nlohmann::json& /* connection_info */) {}
  bool can_receive() const noexcept override { return m_can_receive; }
  void make_me_ready_to_receive() { m_can_receive = true; }
  void sabotage_my_receiving_ability() { m_can_receive = false; }

protected:
  Receiver::Response receive_(const duration_t& /* timeout */) override
  {
    Receiver::Response output;
    output.m_data = std::vector<char>(s_bytes_on_each_receive, 'A');
    output.m_metadata = "";
    return output;
  }

private:
  bool m_can_receive;
};

} // namespace ""

BOOST_AUTO_TEST_CASE(CopyAndMoveSemantics)
{
  BOOST_REQUIRE(!std::is_copy_constructible_v<ReceiverImpl>);
  BOOST_REQUIRE(!std::is_copy_assignable_v<ReceiverImpl>);
  BOOST_REQUIRE(!std::is_move_constructible_v<ReceiverImpl>);
  BOOST_REQUIRE(!std::is_move_assignable_v<ReceiverImpl>);
}

BOOST_AUTO_TEST_CASE(StatusChecks)
{
  ReceiverImpl the_receiver;

  BOOST_REQUIRE(!the_receiver.can_receive());

  the_receiver.make_me_ready_to_receive();
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

BOOST_AUTO_TEST_SUITE_END()
