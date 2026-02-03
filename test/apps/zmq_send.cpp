/**
 * @file zmq_send.cpp ZeroMQ Send Test Application
 *
 * Used in conjunction with zmq_receive, this test application instantiates a ZmqSender plugin,
 * sends the given number of packets, and reports the outgoing data rate.
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "boost/program_options.hpp"
#include "ipm/Sender.hpp"
#include "ipm/ZmqContext.hpp"
#include "logging/Logging.hpp"

#include <chrono>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

int
main(int argc, char* argv[])
{
  uint32_t npackets = 1; // NOLINT(build/unsigned)
  size_t packetSize = 100;
  size_t interval = 0;
  std::string conString = "tcp://127.0.0.1:12345";
  int nthreads = 1;
  uint32_t id = 0; // NOLINT(build/unsigned)

  namespace po = boost::program_options;
  po::options_description desc("Simple test program for ZmqSender");
  desc.add_options()(
    "connection,c", po::value<std::string>(&conString)->default_value(conString), "Connection to listen on")(
    "threads,t", po::value<int>(&nthreads)->default_value(nthreads), "Number of ZMQ threads")(
    "packets,p",
    po::value<uint32_t>(&npackets)->default_value(npackets), // NOLINT(build/unsigned)
    "Number of packets to send")(
    "packetSize,s", po::value<size_t>(&packetSize)->default_value(packetSize), "Number of bytes per packet")(
    "interval,i", po::value<size_t>(&interval)->default_value(interval), "Microseconds to sleep between messages")(
    "id", po::value<uint32_t>(&id)->default_value(id), "Identifier for this Sender"); // NOLINT(build/unsigned)
  try {
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
  } catch (std::exception& ex) {
    std::cerr << "Error parsing command line " << ex.what() << std::endl; // NOLINT(runtime/output_format)
    std::cerr << desc << std::endl;                                       // NOLINT(runtime/output_format)
    return 0;
  }

  dunedaq::logging::Logging::setup("ZMQ Test", "zmq_send");
  if (nthreads > 1) {
    dunedaq::ipm::ZmqContext::instance().set_context_threads(nthreads);
  }

  // Set the minimum packet size to 16 bytes, 8 bytes for sequence number and 8 bytes for current time
  if (packetSize < 16) {
    packetSize = 16;
  }

  std::shared_ptr<dunedaq::ipm::Sender> sender = dunedaq::ipm::make_ipm_sender("ZmqSender");
  dunedaq::ipm::Sender::ConnectionInfo conn_info("zmq_send", conString);
  sender->connect_for_sends(conn_info);

  std::vector<char> message(packetSize, 0);
  *reinterpret_cast<uint32_t*>(message.data()) = id; // NOLINT

  auto start = std::chrono::steady_clock::now();
  for (uint64_t p = 0; p < npackets; p++) { // NOLINT(build/unsigned)
    TLOG_DEBUG(3) << "Preparing Message " << p;
    // Last arg is send timeout
    int attempt = 0;
    bool success = false;

    *(reinterpret_cast<uint32_t*>(message.data()) + 1) = p; // NOLINT
    uint64_t time_since_epoch =                             // NOLINT(build/unsigned)
      std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
    *(reinterpret_cast<uint64_t*>(message.data()) + 1) = time_since_epoch; // NOLINT

    TLOG_DEBUG(4) << "Sending Message " << p;
    do {
      success = sender->send(static_cast<void*>(message.data()), packetSize, std::chrono::milliseconds(2), "", true);
      attempt++;
      if (success == false && attempt == 1)
        TLOG() << "bad omen";
    } while (success == false);

    TLOG_DEBUG(5) << "Message sent " << p;
    if (interval > 0) {
      usleep(interval);
    }
  }

  auto elapsed = std::chrono::steady_clock::now() - start;
  auto nano = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
  auto bw = (packetSize * npackets) / static_cast<double>(nano);
  TLOG() << "Sent " << packetSize * npackets << " bytes in " << nano << " ns " << bw << " GB/s";
}
