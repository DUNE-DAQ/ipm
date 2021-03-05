/**
 * @file VectorIntIPMSenderDAQModule.cxx VectorIntIPMSenderDAQModule class
 * implementation
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "VectorIntIPMSenderDAQModule.hpp"

#include "ipm/vectorintipmsenderdaqmodule/Nljs.hpp"

#include "appfwk/app/Nljs.hpp"

#include "logging/Logging.hpp"

#include <chrono>
#include <functional>
#include <string>
#include <thread>
#include <vector>

namespace dunedaq::ipm {

VectorIntIPMSenderDAQModule::VectorIntIPMSenderDAQModule(const std::string& name)
  : appfwk::DAQModule(name)
  , m_thread(std::bind(&VectorIntIPMSenderDAQModule::do_work, this, std::placeholders::_1))
  , m_queue_timeout(100)
  , m_input_queue(nullptr)
{

  register_command("conf", &VectorIntIPMSenderDAQModule::do_configure);
  register_command("start", &VectorIntIPMSenderDAQModule::do_start);
  register_command("stop", &VectorIntIPMSenderDAQModule::do_stop);
}

void
VectorIntIPMSenderDAQModule::init(const data_t& init_data)
{
  auto ini = init_data.get<appfwk::app::ModInit>();
  for (const auto& qi : ini.qinfos) {
    if (qi.name == "input") {
      TLOG() <<"VIISDM: input queue is " << qi.inst;
      m_input_queue.reset(new appfwk::DAQSource<std::vector<int>>(qi.inst));
    }
  }

  // TODO: John Freeman (jcfree@fnal.gov), Oct-22-2020
  // In the next week, determine what to do if sender_type isn't known
}

void
VectorIntIPMSenderDAQModule::do_configure(const data_t& config_data)
{
  m_cfg = config_data.get<vectorintipmsenderdaqmodule::Conf>();

  m_num_ints_per_vector = m_cfg.nIntsPerVector;
  m_queue_timeout = static_cast<std::chrono::milliseconds>(m_cfg.queue_timeout_ms);
  m_topic = m_cfg.topic;

  m_output = make_ipm_sender(m_cfg.sender_type);
  m_output->connect_for_sends(m_cfg.connection_info);
}

void
VectorIntIPMSenderDAQModule::do_start(const data_t& /*args*/)
{
  m_thread.start_working_thread();
}

void
VectorIntIPMSenderDAQModule::do_stop(const data_t& /*args*/)
{
  m_thread.stop_working_thread();
}

void
VectorIntIPMSenderDAQModule::do_work(std::atomic<bool>& running_flag)
{
  int counter = 0;
  std::vector<int> vec;
  std::ostringstream oss;

  while (running_flag.load()) {
    if (m_input_queue->can_pop() && m_output->can_send()) {

      TLOG_DEBUG(1) << get_name() << ": Going to receive data from input_queue";

      try {
        m_input_queue->pop(vec, m_queue_timeout);
      } catch (const dunedaq::appfwk::QueueTimeoutExpired& excpt) {
        continue;
      }

      TLOG_DEBUG(1) << get_name() << ": Received vector of size " << vec.size() << " from queue, sending";
      m_output->send(&vec[0], vec.size() * sizeof(int), m_queue_timeout, m_topic);

      counter++;
      oss << ": Sent " << counter << " vectors";
      ers::info(SenderProgressUpdate(ERS_HERE, get_name(), oss.str()));
      oss.str("");
    } else {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
}

} // namespace dunedaq::ipm

DEFINE_DUNE_DAQ_MODULE(dunedaq::ipm::VectorIntIPMSenderDAQModule)
