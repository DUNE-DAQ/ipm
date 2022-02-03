/**
 *
 * @file CallbackAdapter.cpp ipm CallbackAdapter class
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "CallbackAdapter.hpp"

#include "logging/Logging.hpp"

#include <string>
#include <utility>

namespace dunedaq::ipm {

CallbackAdapter::~CallbackAdapter() noexcept
{
  {
    std::lock_guard<std::mutex> lk(m_callback_mutex);
    m_callback = nullptr;
  }
  shutdown();
  m_receiver_ptr = nullptr;
}

void
CallbackAdapter::set_receiver(Receiver* receiver_ptr)
{
  {
    std::lock_guard<std::mutex> lk(m_callback_mutex);
    m_receiver_ptr = receiver_ptr;
  }

  if (m_receiver_ptr != nullptr && m_callback != nullptr) {
    startup();
  }
}

void
CallbackAdapter::set_callback(std::function<void(Receiver::Response&)> callback)
{
  {
    std::lock_guard<std::mutex> lk(m_callback_mutex);
    m_callback = callback;
  }

  if (m_receiver_ptr != nullptr && m_callback != nullptr) {
    startup();
  }
}

void
CallbackAdapter::clear_callback()
{
  {
    std::lock_guard<std::mutex> lk(m_callback_mutex);
    m_callback = nullptr;
  }
  shutdown();
}

void
CallbackAdapter::shutdown()
{
  if (m_thread && m_thread->joinable())
    m_thread->join();

  m_is_listening = false;
  m_thread.reset(nullptr);
}

void
CallbackAdapter::startup()
{
  shutdown();
  m_is_listening = false;
  m_thread.reset(new std::thread([&] { thread_loop(); }));

  while (!m_is_listening.load()) {
    usleep(1000);
  }
}

void
CallbackAdapter::thread_loop()
{
  do {
    try {
      auto response = m_receiver_ptr->receive(Receiver::s_no_block);

      TLOG_DEBUG(25) << "Received " << response.data.size() << " bytes. Dispatching to callback.";
      {
        std::lock_guard<std::mutex> lk(m_callback_mutex);
        if (m_callback != nullptr) {
          m_callback(response);
        }
      }
    } catch (ipm::ReceiveTimeoutExpired const& tmo) {
      usleep(10000);
    }
    m_is_listening = true;
  } while (m_callback != nullptr && m_receiver_ptr != nullptr);
}

} // namespace dunedaq::ipm