#include "ipm/Receiver.hpp"
#include "ipm/ZmqContext.hpp"

#include "boost/program_options.hpp"

#include <memory>
#include <chrono>
using namespace dunedaq::ipm;

int main(int argc, char* argv[]){
  std::string conString="tcp://127.0.0.1:12345";
  int npackets=1;
  int nthreads=1;

  namespace po = boost::program_options;
  po::options_description desc("Simple test program for ZmqReceiver");
  desc.add_options()(
    "connection,c", po::value<std::string>(&conString), "Connection to listen on")(
    "threads,t", po::value<int>(&nthreads), "Number of ZMQ threads")(
    "packets,p", po::value<int>(&npackets), "Number of packets per group for reporting");
  try {
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
  } catch (std::exception& ex) {
    std::cerr << "Error parsing command line " << ex.what() << std::endl;
    std::cerr << desc << std::endl;
    return 0;
  }


  zmq::context_t& context=dunedaq::ipm::ZmqContext::instance().GetContext();
  context.set(zmq::ctxopt::io_threads, nthreads);

  // Receiver side
  std::shared_ptr<Receiver> receiver=make_ipm_receiver("ZmqReceiver");
  receiver->connect_for_receives({ {"connection_string", conString} });

  try {
    while (true) {
      // Last arg is receive timeout
      auto start=std::chrono::steady_clock::now();
      float bytesReceived=0;
      for (int p=0;p<npackets;p++) {
        Receiver::Response resp=receiver->receive(std::chrono::seconds(10));
        bytesReceived+=resp.data.size();
      }
      auto elapsed=std::chrono::steady_clock::now()-start;
      auto nano=std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
      float bw=bytesReceived/nano;
      std::cout << "Received " << bytesReceived << " bytes in "
                << nano << " ns " << bw << " GB/s" << std::endl;
      // std::cout << "resp.data=";
      // for (auto d: resp.data) {
      //   std::cout << d << std::endl;
      // }
    }
  }
  catch(ReceiveTimeoutExpired const& exc) {
    std::cout << "Gave up waiting\n";
  }
}
