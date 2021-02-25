/**
 * @file VectorIntIPMSenderDAQModule.hpp
 *
 * VectorIntIPMSenderDAQModule is a simple DAQModule implementation that sends vectors of integers to
 * VectorIntIPMReceiverDAQModule
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef IPM_TEST_PLUGINS_VECTORINTIPMSENDERDAQMODULE_HPP_
#define IPM_TEST_PLUGINS_VECTORINTIPMSENDERDAQMODULE_HPP_

#include "ipm/Sender.hpp"
#include "ipm/vectorintipmsenderdaqmodule/Structs.hpp"

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSource.hpp"
#include "appfwk/ThreadHelper.hpp"

#include "ers/Issue.hpp"

#include <future>
#include <memory>
#include <string>
#include <vector>

namespace dunedaq {
namespace ipm {
/**
 * @brief VectorIntIPMSenderDAQModule creates vectors of ints and sends them
 * downstream
 */
class VectorIntIPMSenderDAQModule : public appfwk::DAQModule
{
public:
  /**
   * @brief VectorIntIPMSenderDAQModule Constructor
   * @param name Instance name for this VectorIntIPMSenderDAQModule instance
   */
  explicit VectorIntIPMSenderDAQModule(const std::string& name);

  VectorIntIPMSenderDAQModule(const VectorIntIPMSenderDAQModule&) =
    delete; ///< VectorIntIPMSenderDAQModule is not copy-constructible
  VectorIntIPMSenderDAQModule& operator=(const VectorIntIPMSenderDAQModule&) =
    delete; ///< VectorIntIPMSenderDAQModule is not copy-assignable
  VectorIntIPMSenderDAQModule(VectorIntIPMSenderDAQModule&&) =
    delete; ///< VectorIntIPMSenderDAQModule is not move-constructible
  VectorIntIPMSenderDAQModule& operator=(VectorIntIPMSenderDAQModule&&) =
    delete; ///< VectorIntIPMSenderDAQModule is not move-assignable

  void init(const data_t&) override;

private:
  // Commands
  void do_configure(const data_t&);
  void do_start(const data_t&);
  void do_stop(const data_t&);

  // Threading
  void do_work(std::atomic<bool>& running_flag);
  appfwk::ThreadHelper m_thread;

  // Configuration (for validation)

  vectorintipmsenderdaqmodule::Conf m_cfg;
  size_t m_num_ints_per_vector = 999;
  std::chrono::milliseconds m_queue_timeout;
  std::unique_ptr<appfwk::DAQSource<std::vector<int>>> m_input_queue;
  std::shared_ptr<Sender> m_output;
  std::string m_topic{ "" };
};

} // namespace ipm
ERS_DECLARE_ISSUE_BASE(ipm,
                       SenderProgressUpdate,
                       appfwk::GeneralDAQModuleIssue,
                       message,
                       ((std::string)name),
                       ((std::string)message))
} // namespace dunedaq

#endif // IPM_TEST_PLUGINS_VECTORINTIPMSENDERDAQMODULE_HPP_
