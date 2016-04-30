#include "bus.hpp"

Bus::Bus(SimpleCache *c[], Memory *m, Time *t){
  for (int i = 0; i < NUM_PROCESSORS; i++){
    caches[i] = c[i];
  }
  mem = m;
  timer = t;
  shared = false;
  dirty = false;
  snoop_pending = false;
}

int Bus::sendMsgToBus(int core_num, request_t request, uint64_t addr){
  timer->time++;
  snoop_pending = true;
  for (int i = 0; i < NUM_PROCESSORS; i++){
    if (i != core_num){
      //update the status of the cache based on the message on the bus
      if(caches[i]->updateStatus(request, addr)){
        dirty = true;
        mem->flush();
        dirty = false;
      }
        shared = shared || (caches[i]->checkState(addr) == SHARED);
    }
  }
  snoop_pending = false;
  return 0;
}

