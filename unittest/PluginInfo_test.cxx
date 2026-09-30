/**
 * @file PluginInfo_test.cxx PluginInfo Unit Tests
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ipm/PluginInfo.hpp"

#define BOOST_TEST_MODULE PluginInfo_test // NOLINT

#include "boost/test/unit_test.hpp"

using namespace dunedaq::ipm;

BOOST_AUTO_TEST_SUITE(PluginInfo_test)

BOOST_AUTO_TEST_CASE(GetRecommendedPluginName)
{
  BOOST_REQUIRE_EQUAL(get_recommended_plugin_name(IpmPluginType::Sender), "ZmqSender");
  BOOST_REQUIRE_EQUAL(get_recommended_plugin_name(IpmPluginType::Receiver), "ZmqReceiver");
  BOOST_REQUIRE_EQUAL(get_recommended_plugin_name(IpmPluginType::Publisher), "ZmqPublisher");
  BOOST_REQUIRE_EQUAL(get_recommended_plugin_name(IpmPluginType::Subscriber), "ZmqSubscriber");
}

BOOST_AUTO_TEST_SUITE_END()
