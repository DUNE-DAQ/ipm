/**
 * @file VectorIntIPMReceiverDAQModule.hpp
 *
 * VectorIntIPMReceiverDAQModule receives vectors of integers from VectorIntIPMSenderDAQModule
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef IPM_TEST_PLUGINS_VECTORINTIPMRECEIVERDAQMODULE_HPP_
#define IPM_TEST_PLUGINS_VECTORINTIPMRECEIVERDAQMODULE_HPP_

#include "appfwk/DAQModule.hpp"
#include "appfwk/DAQSink.hpp"
#include "appfwk/ThreadHelper.hpp"
#include "ipm/Receiver.hpp"

#include "ipm/viir/Structs.hpp"

#include <future>
#include <memory>
#include <string>
#include <vector>

namespace dunedaq {
namespace ipm {
/**
 * @brief VectorIntIPMReceiverDAQModule creates vectors of ints and sends them
 * downstream
 */
class VectorIntIPMReceiverDAQModule : public appfwk::DAQModule
{
public:
  /**
   * @brief VectorIntIPMReceiverDAQModule Constructor
   * @param name Instance name for this VectorIntIPMReceiverDAQModule instance
   */
  explicit VectorIntIPMReceiverDAQModule(const std::string& name);

  VectorIntIPMReceiverDAQModule(const VectorIntIPMReceiverDAQModule&) =
    delete; ///< VectorIntIPMReceiverDAQModule is not copy-constructible
  VectorIntIPMReceiverDAQModule& operator=(const VectorIntIPMReceiverDAQModule&) =
    delete; ///< VectorIntIPMReceiverDAQModule is not copy-assignable
  VectorIntIPMReceiverDAQModule(VectorIntIPMReceiverDAQModule&&) =
    delete; ///< VectorIntIPMReceiverDAQModule is not move-constructible
  VectorIntIPMReceiverDAQModule& operator=(VectorIntIPMReceiverDAQModule&&) =
    delete; ///< VectorIntIPMReceiverDAQModule is not move-assignable

  void init(const data_t& ) override;

private:
  // Commands
  void do_configure(const data_t& );
  void do_start(const data_t& );
  void do_stop(const data_t& );

  // Threading
  appfwk::ThreadHelper thread_;
  void do_work(std::atomic<bool>& running_flag);

  // Configuration
  viir::Conf cfg_;
  std::shared_ptr<Receiver> input_;
  std::unique_ptr<appfwk::DAQSink<std::vector<int>>> outputQueue_;
  std::chrono::milliseconds queueTimeout_;
  size_t nIntsPerVector_ = 999;

};
} // namespace ipm

ERS_DECLARE_ISSUE_BASE(ipm,
                       ReceiverProgressUpdate,
                       appfwk::GeneralDAQModuleIssue,
                       message,
                       ((std::string)name),
                       ((std::string)message))
} // namespace dunedaq

#endif // IPM_TEST_PLUGINS_VECTORINTIPMRECEIVERDAQMODULE_HPP_
