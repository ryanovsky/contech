#include "bus.hpp"

Bus::Bus(SimpleCache *c[]){
  for (int i = 0; i < NUM_PROCESSORS; i++){
    caches[i] = c[i];
  }
}

int Bus::sendMsgToBus(int core_num, request_t request, uint64_t addr){

  for (int i = 0; i < NUM_PROCESSORS; i++){
    if (i != core_num){
      //update the status of the cache based on the message on the bus
      caches[i]->updateStatus(request, addr);
    }
  }

  //next = (next+1) % NUM_CORES;i
}

