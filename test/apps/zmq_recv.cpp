#include "ipm/Receiver.hpp"
#include "ipm/ZmqContext.hpp"

#include "boost/program_options.hpp"

#include <memory>
#include <chrono>
using namespace dunedaq::ipm;

int main(int argc, char* argv[]){
  std::string conString="tcp://127.0.0.1:12345";
  size_t npackets=1;
  int nthreads=1;
  int timeout = 10;

  namespace po = boost::program_options;
  po::options_description desc("Simple test program for ZmqReceiver");
  desc.add_options()(
    "connection,c", po::value<std::string>(&conString)->default_value(conString), "Connection to listen on")(
    "threads,t", po::value<int>(&nthreads)->default_value(nthreads), "Number of ZMQ threads")(
    "packets,p", po::value<size_t>(&npackets)->default_value(npackets), "Number of packets per group for reporting")(
    "timeout,o", po::value<int>(&timeout)->default_value(timeout), "Timeout, in seconds");
  try {
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
  } catch (std::exception& ex) {
    std::cerr << "Error parsing command line " << ex.what() << std::endl;
    std::cerr << desc << std::endl;
    return 0;
  }

  dunedaq::logging::Logging::setup("ZMQ Test", "zmq_recv");
  if (nthreads > 1) {
    dunedaq::ipm::ZmqContext::instance().set_context_threads(nthreads);
  }

  // Receiver side
  std::shared_ptr<Receiver> receiver=make_ipm_receiver("ZmqReceiver");
  receiver->connect_for_receives({ {"connection_string", conString} });

  std::map<uint32_t, uint32_t> last_received_sequence;
  int64_t first_latency = 0;
  try {
    while (true) {
      // Last arg is receive timeout
      auto start=std::chrono::steady_clock::now();
      float bytesReceived=0;
      for (size_t p=0;p<npackets;p++) {
        Receiver::Response resp=receiver->receive(std::chrono::seconds(timeout));
        int64_t recvd_ts = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        bytesReceived+=resp.data.size();
        auto this_id = *(reinterpret_cast<uint32_t*>(resp.data.data()));
        auto this_sequence = *(reinterpret_cast<uint32_t*>(resp.data.data()) + 1);
        auto this_ts = *(reinterpret_cast<uint64_t*>(resp.data.data()) + 1);

        if(this_sequence < last_received_sequence[this_id] + 1) {
          TLOG() << "Received sequence ID " << this_sequence << " < expected sequence " << (last_received_sequence[this_id] + 1) << " from sender " << this_id;
        }
        int64_t this_latency = recvd_ts - this_ts;
        if(first_latency == 0) {
          first_latency = this_latency;
        }
        TLOG_DEBUG(6) << "Received message " << this_sequence << " from sender " << this_id << ", latency= " << this_latency << " us (diff= " << (this_latency - first_latency) << " us)";
        last_received_sequence[this_id] = this_sequence;
      }
      auto elapsed=std::chrono::steady_clock::now()-start;
      auto nano=std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
      float bw=bytesReceived/nano;
      TLOG() << "Received " << bytesReceived << " bytes in "
                << nano << " ns " << bw << " GB/s";
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
