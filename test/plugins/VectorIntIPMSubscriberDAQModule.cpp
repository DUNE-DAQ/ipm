/**
 * @file VectorIntIPMSubscriberDAQModule.cc VectorIntIPMSubscriberDAQModule class
 * implementation
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "VectorIntIPMSubscriberDAQModule.hpp"

#include "ipm/vectorintipmreceiverdaqmodule/Nljs.hpp"

#include "appfwk/cmd/Nljs.hpp"

#include "TRACE/trace.h"

#include <chrono>
#include <string>
#include <thread>
#include <utility>
#include <vector>

/**
 * @brief Name used by TRACE TLOG calls from this source file
 */
#define TRACE_NAME "VectorIntIPMSubscriber" // NOLINT

namespace dunedaq {
namespace ipm {

VectorIntIPMSubscriberDAQModule::VectorIntIPMSubscriberDAQModule(const std::string& name)
  : appfwk::DAQModule(name)
  , m_thread(std::bind(&VectorIntIPMSubscriberDAQModule::do_work, this, std::placeholders::_1))
  , m_output_queue(nullptr)
{

  register_command("conf", &VectorIntIPMSubscriberDAQModule::do_configure);
  register_command("start", &VectorIntIPMSubscriberDAQModule::do_start);
  register_command("stop", &VectorIntIPMSubscriberDAQModule::do_stop);
}

void
VectorIntIPMSubscriberDAQModule::init(const data_t& init_data)
{
  auto ini = init_data.get<appfwk::cmd::ModInit>();
  for (const auto& qi : ini.qinfos) {
    if (qi.name == "output") {
      ERS_INFO("VIIRDM: output queue is " << qi.inst);
      m_output_queue.reset(new appfwk::DAQSink<std::vector<int>>(qi.inst));
    }
  }
}

void
VectorIntIPMSubscriberDAQModule::do_configure(const data_t& config_data)
{
  m_cfg = config_data.get<vectorintipmreceiverdaqmodule::Conf>();

  m_num_ints_per_vector = m_cfg.nIntsPerVector;
  m_queue_timeout = static_cast<std::chrono::milliseconds>(m_cfg.queue_timeout_ms);

  m_input = make_ipm_subscriber(m_cfg.receiver_type);

  std::string topic = m_cfg.topic;
  ERS_INFO("VIISubDM: topic is " << topic);

  m_input->subscribe(topic);
  m_input->connect_for_receives(m_cfg.connection_info);
}

void
VectorIntIPMSubscriberDAQModule::do_start(const data_t& /*args*/)
{
  m_thread.start_working_thread();
}

void
VectorIntIPMSubscriberDAQModule::do_stop(const data_t& /*args*/)
{
  m_thread.stop_working_thread();
}

void
VectorIntIPMSubscriberDAQModule::do_work(std::atomic<bool>& running_flag)
{
  size_t counter = 0;
  std::ostringstream oss;

  while (running_flag.load()) {
    if (m_input->can_receive()) {

      TLOG(TLVL_TRACE) << get_name() << ": Creating output vector";
      std::vector<int> output(m_num_ints_per_vector);

      try {
        auto recvd = m_input->receive(m_queue_timeout);
        if (recvd.m_data.size() == 0) {
          TLOG(TLVL_TRACE) << "No data received, moving to next loop iteration";
          continue;
        }

        assert(recvd.m_data.size() == m_num_ints_per_vector * sizeof(int));
        memcpy(&output[0], &recvd.m_data[0], sizeof(int) * m_num_ints_per_vector);

        oss << ": Received vector " << counter << " with size " << output.size() << " on topic " << recvd.m_metadata;
        ers::info(SubscriberProgressUpdate(ERS_HERE, get_name(), oss.str()));
        oss.str("");
      } catch (ReceiveTimeoutExpired const& rte) {
        TLOG(TLVL_TRACE) << "ReceiveTimeoutExpired: " << rte.what();
        continue;
      }

      TLOG(TLVL_TRACE) << get_name() << ": Pushing vector into output_queue";
      try {
        m_output_queue->push(std::move(output), m_queue_timeout);
      } catch (const appfwk::QueueTimeoutExpired& ex) {
        ers::warning(ex);
      }

      TLOG(TLVL_TRACE) << get_name() << ": End of do_work loop";
      counter++;
    } else {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
}

} // namespace ipm
} // namespace dunedaq

DEFINE_DUNE_DAQ_MODULE(dunedaq::ipm::VectorIntIPMSubscriberDAQModule)
