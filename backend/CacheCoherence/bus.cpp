#include "bus.hpp"

Bus::Bus(SimpleCache *c[], Memory *m){
  for (int i = 0; i < NUM_PROCESSORS; i++){
    caches[i] = c[i];
  }
  mem = m;
}

int Bus::sendMsgToBus(int core_num, request_t request, uint64_t addr){

  for (int i = 0; i < NUM_PROCESSORS; i++){
    if (i != core_num){
      //update the status of the cache based on the message on the bus
      if(caches[i]->updateStatus(request, addr)) mem->flush();
    }
  }

  //next = (next+1) % NUM_CORES;i
}

