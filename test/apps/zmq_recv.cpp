/**
 * @file zmq_recv.cpp ZeroMQ Receive Test Application
 *
 * Used in conjunction with zmq_send, this test application instantiates a ZmqReceiver plugin and reports the incoming
 * data rate.
 *
 * This is part of the DUNE DAQ Application Framework, copyright 2020.
 * Licensing/copyright details are in the COPYING file that you should have
 * received with this code.
 */

#include "ipm/Receiver.hpp"
#include "ipm/ZmqContext.hpp"

#include "boost/program_options.hpp"

#include <chrono>
#include <memory>
#include <string>

using namespace dunedaq::ipm;

int
main(int argc, char* argv[])
{
  std::string conString = "tcp://127.0.0.1:12345";
  int npackets = 1;
  int nthreads = 1;
  int timeout = 10;

  namespace po = boost::program_options;
  po::options_description desc("Simple test program for ZmqReceiver");
  desc.add_options()("connection,c", po::value<std::string>(&conString), "Connection to listen on")(
    "threads,t", po::value<int>(&nthreads), "Number of ZMQ threads")(
    "packets,p", po::value<int>(&npackets), "Number of packets per group for reporting")(
    "timeout,o", po::value<int>(&timeout), "Timeout, in seconds");
  try {
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
  } catch (std::exception& ex) {
    std::cerr << "Error parsing command line " << ex.what() << std::endl; // NOLINT(runtime/output_format)
    std::cerr << desc << std::endl;                                       // NOLINT(runtime/output_format)
    return 0;
  }

  if (nthreads > 1) {
    dunedaq::ipm::ZmqContext::instance().set_context_threads(nthreads);
  }

  // Receiver side
  std::shared_ptr<Receiver> receiver = make_ipm_receiver("ZmqReceiver");
  receiver->connect_for_receives({ { "connection_string", conString } });

  try {
    while (true) {
      // Last arg is receive timeout
      auto start = std::chrono::steady_clock::now();
      double bytesReceived = 0;
      for (int p = 0; p < npackets; p++) {
        Receiver::Response resp = receiver->receive(std::chrono::seconds(timeout));
        bytesReceived += resp.data.size();
      }
      auto elapsed = std::chrono::steady_clock::now() - start;
      auto nano = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
      auto bw = bytesReceived / static_cast<double>(nano);
      // NOLINTNEXTLINE(runtime/output_format)
      std::cout << "Received " << bytesReceived << " bytes in " << nano << " ns " << bw << " GB/s" << std::endl;
      // std::cout << "resp.data=";
      // for (auto d: resp.data) {
      //   std::cout << d << std::endl;
      // }
    }
  } catch (ReceiveTimeoutExpired const& exc) {
    std::cout << "Gave up waiting\n"; // NOLINT(runtime/output_format)
  }
}
