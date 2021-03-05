/**
 * @file Sender_test.cxx Sender class Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ipm/Sender.hpp"

#define BOOST_TEST_MODULE Sender_test // NOLINT

#include "boost/test/unit_test.hpp"

#include <string>
#include <vector>

using namespace dunedaq::ipm;

BOOST_AUTO_TEST_SUITE(Sender_test)

namespace {

class SenderImpl : public Sender
{

public:
  SenderImpl()
    : m_can_send(false)
  {}

  void connect_for_sends(const nlohmann::json& /* connection_info */) {}
  bool can_send() const noexcept override { return m_can_send; }
  void make_me_ready_to_send() { m_can_send = true; }
  void sabotage_my_sending_ability() { m_can_send = false; }

protected:
  void send_(const void* /* message */,
             int /* N */,
             const duration_t& /* timeout */,
             const std::string& /* metadata */) override
  {
    // Pretty unexciting stub
  }

private:
  bool m_can_send;
};

} // namespace ""

BOOST_AUTO_TEST_CASE(CopyAndMoveSemantics)
{
  BOOST_REQUIRE(!std::is_copy_constructible_v<SenderImpl>);
  BOOST_REQUIRE(!std::is_copy_assignable_v<SenderImpl>);
  BOOST_REQUIRE(!std::is_move_constructible_v<SenderImpl>);
  BOOST_REQUIRE(!std::is_move_assignable_v<SenderImpl>);
}

BOOST_AUTO_TEST_CASE(StatusChecks)
{
  SenderImpl the_sender;
  std::vector<char> random_data{ 'T', 'E', 'S', 'T' };

  BOOST_REQUIRE(!the_sender.can_send());

  the_sender.make_me_ready_to_send();
  BOOST_REQUIRE(the_sender.can_send());

  BOOST_REQUIRE_NO_THROW(the_sender.send(random_data.data(), random_data.size(), Sender::s_no_block));

  the_sender.sabotage_my_sending_ability();
  BOOST_REQUIRE(!the_sender.can_send());

  BOOST_REQUIRE_EXCEPTION(the_sender.send(random_data.data(), random_data.size(), Sender::s_no_block),
                          dunedaq::ipm::KnownStateForbidsSend,
                          [&](dunedaq::ipm::KnownStateForbidsSend) { return true; });
}

BOOST_AUTO_TEST_CASE(BadInput)
{
  SenderImpl the_sender;
  the_sender.make_me_ready_to_send();

  const char* bad_bytes = nullptr;
  BOOST_REQUIRE_EXCEPTION(the_sender.send(bad_bytes, 10, Sender::s_no_block),
                          dunedaq::ipm::NullPointerPassedToSend,
                          [&](dunedaq::ipm::NullPointerPassedToSend) { return true; });

  std::vector<char> random_data{ 'T', 'E', 'S', 'T' };
  BOOST_REQUIRE_NO_THROW(the_sender.send(random_data.data(), 0, Sender::s_no_block));
}

BOOST_AUTO_TEST_CASE(StatusChecks_Multipart)
{
  SenderImpl the_sender;
  std::vector<std::pair<const void*, Sender::message_size_t>> random_data{
    { "T", 1 }, { "Ee", 2 }, { "S", 1 }, { "Tt", 2 }
  };

  BOOST_REQUIRE(!the_sender.can_send());

  the_sender.make_me_ready_to_send();
  BOOST_REQUIRE(the_sender.can_send());

  BOOST_REQUIRE_NO_THROW(the_sender.send_multipart(random_data, Sender::s_no_block));

  the_sender.sabotage_my_sending_ability();
  BOOST_REQUIRE(!the_sender.can_send());

  BOOST_REQUIRE_EXCEPTION(the_sender.send_multipart(random_data, Sender::s_no_block),
                          dunedaq::ipm::KnownStateForbidsSend,
                          [&](dunedaq::ipm::KnownStateForbidsSend) { return true; });
}

BOOST_AUTO_TEST_CASE(BadInput_Multipart)
{
  SenderImpl the_sender;
  the_sender.make_me_ready_to_send();

  std::vector<std::pair<const void*, Sender::message_size_t>> bad_data{ { nullptr, 10 } };
  BOOST_REQUIRE_EXCEPTION(the_sender.send_multipart(bad_data, Sender::s_no_block),
                          dunedaq::ipm::NullPointerPassedToSend,
                          [&](dunedaq::ipm::NullPointerPassedToSend) { return true; });

  std::vector<std::pair<const void*, Sender::message_size_t>> random_data{  };
  BOOST_REQUIRE_NO_THROW(the_sender.send_multipart(random_data, Sender::s_no_block));
}

BOOST_AUTO_TEST_SUITE_END()
