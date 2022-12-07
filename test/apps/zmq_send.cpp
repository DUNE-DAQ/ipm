#include "ipm/Sender.hpp"
#include "ipm/ZmqContext.hpp"

#include "boost/program_options.hpp"

#include <memory>
#include <chrono>
#include <cstdlib>
#include <cerrno>

int main(int argc, char* argv[]){
  int npackets=1;
  int packetSize=100;
  std::string conString="tcp://127.0.0.1:12345";
  int nthreads=1;

  namespace po = boost::program_options;
  po::options_description desc("Simple test program for ZmqSender");
  desc.add_options()(
    "connection,c", po::value<std::string>(&conString), "Connection to listen on")(
    "threads,t", po::value<int>(&nthreads), "Number of ZMQ threads")(
    "packets,p", po::value<int>(&npackets), "Number of packets to send")(
    "packetSize,s", po::value<int>(&packetSize), "Number of bytes per packet");
  try {
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
  } catch (std::exception& ex) {
    std::cerr << "Error parsing command line " << ex.what() << std::endl;
    std::cerr << desc << std::endl;
    return 0;
  }

  if (nthreads > 1) {
    dunedaq::ipm::ZmqContext::instance().set_context_threads(nthreads);
  }
 
  std::shared_ptr<dunedaq::ipm::Sender> sender=dunedaq::ipm::make_ipm_sender("ZmqSender");
  sender->connect_for_sends({ {"connection_string", conString} });

  std::vector message(packetSize,0);

  auto start=std::chrono::steady_clock::now();
  for (int p=0; p<npackets;p++) {
    // Last arg is send timeout
    sender->send((void*)message.data(), packetSize, std::chrono::milliseconds(100));
  }

  auto elapsed=std::chrono::steady_clock::now()-start;
  auto nano=std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
  float bw=((float)packetSize*npackets)/nano;
  std::cout << "Sent " << packetSize*npackets << " bytes in "
                << nano << " ns " << bw << " GB/s" << std::endl;
  
}
