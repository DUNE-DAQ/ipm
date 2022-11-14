#include "ipm/Sender.hpp"
#include "ipm/ZmqContext.hpp"

#include <memory>
#include <chrono>
#include <cstdlib>

int main(int argc, char* argv[]){
  int npackets=1;
  if (argc>1) {
    npackets=atol(argv[1]);
  }
  int packetSize=100;
  if (argc>2) {
    packetSize=atol(argv[2]);
  }
  std::string conString="tcp://127.0.0.1:12345";
  if (argc>3) {
    conString=std::string(argv[3]);
  }
  // Sender side
  std::shared_ptr<dunedaq::ipm::Sender> sender=dunedaq::ipm::make_ipm_sender("ZmqSender");
  sender->connect_for_sends({ {"connection_string", conString} });

  std::vector message(packetSize,0);

  for (int p=0; p<npackets;p++) {
    // Last arg is send timeout
    sender->send((void*)message.data(), packetSize, std::chrono::milliseconds(100));
  }
  
}
