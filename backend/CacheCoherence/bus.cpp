#include "bus.hpp"

Bus::Bus(SimpleCache *c[], Memory *m){
  for (int i = 0; i < NUM_PROCESSORS; i++){
    caches[i] = c[i];
  }
  mem = m;
  shared = false;
  dirty = false;
  snoop_pending = false;
}

int Bus::sendMsgToBus(int core_num, request_t request, uint64_t addr){

  snoop_pending = true;
  for (int i = 0; i < NUM_PROCESSORS; i++){
    if (i != core_num){
      //update the status of the cache based on the message on the bus
      if(caches[i]->updateStatus(request, addr)){
        dirty = true;
        shared = shared || caches[i]->checkState(addr) == SHARED;
        mem->flush();
        dirty = false;
      }
    }
  }
  snoop_pending = false;
  return 0;
}

