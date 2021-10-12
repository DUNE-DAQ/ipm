/**
 * @file PluginInfo.hpp Holds information about recommended IPM plugins
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef IPM_INCLUDE_IPM_PLUGININFO_HPP_
#define IPM_INCLUDE_IPM_PLUGININFO_HPP_

#include "cetlib/BasicPluginFactory.h"
#include "cetlib/compiler_macros.h"
#include "ers/Issue.hpp"
#include "nlohmann/json.hpp"

#include <memory>
#include <string>
#include <vector>

namespace dunedaq {
namespace ipm {
enum class IpmPluginType
{
  Sender,
  Receiver,
  Publisher,
  Subscriber
};

const std::map<IpmPluginType, std::string> ZmqPluginNames{ { IpmPluginType::Sender, "ZmqSender" },
                                                           { IpmPluginType::Receiver, "ZmqReceiver" },
                                                           { IpmPluginType::Publisher, "ZmqPublisher" },
                                                           { IpmPluginType::Subscriber, "ZmqSubscriber" } };

std::string
get_recommended_plugin_name(IpmPluginType type)
{
  return ZmqPluginNames.at(type);
}

} // namespace ipm
} // namespace dunedaq

#endif // IPM_INCLUDE_IPM_PLUGININFO_HPP_
