/**
 * @file VectorIntIPMReceiverDAQModule.cc VectorIntIPMReceiverDAQModule class
 * implementation
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "VectorIntIPMReceiverDAQModule.hpp"

#include "ipm/vectorintipmreceiverdaqmodule/Nljs.hpp"

#include "appfwk/app/Nljs.hpp"

#include "logging/Logging.hpp"

#include <chrono>
#include <string>
#include <thread>
#include <utility>
#include <vector>

/**
 * @brief Name used by TRACE TLOG calls from this source file
 */
#define TRACE_NAME "VectorIntIPMReceiver" // NOLINT

namespace dunedaq {
namespace ipm {

VectorIntIPMReceiverDAQModule::VectorIntIPMReceiverDAQModule(const std::string& name)
  : appfwk::DAQModule(name)
  , m_thread(std::bind(&VectorIntIPMReceiverDAQModule::do_work, this, std::placeholders::_1))
  , m_output_queue(nullptr)
{

  register_command("conf", &VectorIntIPMReceiverDAQModule::do_configure);
  register_command("start", &VectorIntIPMReceiverDAQModule::do_start);
  register_command("stop", &VectorIntIPMReceiverDAQModule::do_stop);
}

void
VectorIntIPMReceiverDAQModule::init(const data_t& init_data)
{
  auto ini = init_data.get<appfwk::app::ModInit>();
  for (const auto& qi : ini.qinfos) {
    if (qi.name == "output") {
      TLOG() << "VIIRDM: output queue is " << qi.inst;
      m_output_queue.reset(new appfwk::DAQSink<std::vector<int>>(qi.inst));
    }
  }
}

void
VectorIntIPMReceiverDAQModule::do_configure(const data_t& config_data)
{
  m_cfg = config_data.get<vectorintipmreceiverdaqmodule::Conf>();

  m_num_ints_per_vector = m_cfg.nIntsPerVector;
  m_queue_timeout = static_cast<std::chrono::milliseconds>(m_cfg.queue_timeout_ms);

  m_input = make_ipm_receiver(m_cfg.receiver_type);

  m_input->connect_for_receives(m_cfg.connection_info);
}

void
VectorIntIPMReceiverDAQModule::do_start(const data_t& /*args*/)
{
  m_thread.start_working_thread();
}

void
VectorIntIPMReceiverDAQModule::do_stop(const data_t& /*args*/)
{
  m_thread.stop_working_thread();
}

void
VectorIntIPMReceiverDAQModule::do_work(std::atomic<bool>& running_flag)
{
  size_t counter = 0;
  std::ostringstream oss;

  while (running_flag.load()) {
    if (m_input->can_receive()) {

      TLOG_DEBUG(1) << get_name() << ": Creating output vector";
      std::vector<int> output(m_num_ints_per_vector);

      try {

        auto recvd = m_input->receive(m_queue_timeout);

        if (recvd.data.size() == 0) {
          TLOG_DEBUG(1) << "No data received, moving to next loop iteration";
          continue;
        }

        assert(recvd.data.size() == m_num_ints_per_vector * sizeof(int));
        memcpy(&output[0], &recvd.data[0], sizeof(int) * m_num_ints_per_vector);
      } catch (ReceiveTimeoutExpired const& rte) {
        TLOG_DEBUG(1) << "ReceiveTimeoutExpired: " << rte.what();
        continue;
      }
      oss << ": Received vector " << counter << " with size " << output.size();
      ers::info(ReceiverProgressUpdate(ERS_HERE, get_name(), oss.str()));
      oss.str("");

      TLOG_DEBUG(1) << get_name() << ": Pushing vector into output_queue";
      try {
        m_output_queue->push(std::move(output), m_queue_timeout);
      } catch (const appfwk::QueueTimeoutExpired& ex) {
        ers::warning(ex);
      }

      TLOG_DEBUG(1) << get_name() << ": End of do_work loop";
      counter++;
    } else {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
}

} // namespace ipm
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::ipm::VectorIntIPMReceiverDAQModule)
