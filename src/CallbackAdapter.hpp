/**
 *
 * @file CallbackAdapter.hpp IPM CallbackAdapter class
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#ifndef IPM_SRC_CALLBACKADAPTER_HPP_
#define IPM_SRC_CALLBACKADAPTER_HPP_

#include "ipm/Receiver.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

namespace dunedaq {
namespace ipm {

class CallbackAdapter
{
public:
  CallbackAdapter() = default; // Explicitly defaulted

  virtual ~CallbackAdapter() noexcept;

  void set_receiver(Receiver* receiver_ptr);
  void set_callback(std::function<void(Receiver::Response&)> callback);
  void clear_callback();

private:
  void startup();
  void shutdown();
  void thread_loop();

  Receiver* m_receiver_ptr{ nullptr };
  std::function<void(Receiver::Response&)> m_callback{ nullptr };
  mutable std::mutex m_callback_mutex;
  std::unique_ptr<std::thread> m_thread{ nullptr };
  std::atomic<bool> m_is_listening{ false };
};
} // namespace ipm
} // namespace dunedaq

#endif // IPM_SRC_CALLBACKADAPTER_HPP_
