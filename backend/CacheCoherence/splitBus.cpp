#include "splitBus.hpp"

SplitBus::SplitBus(SimpleCache *c[], Memory *m, Time *t){
  for (int i = 0; i < NUM_PROCESSORS; i++){
    caches[i] = c[i];
  }
  mem = m;
  timer = t;
  shared = false;
  dirty = false;
  snoop_pending = false;

  next = 0;
  num_requests = 0;
  reqs = (struct requestTableElem *) malloc(MAX_OUTSTANDING_REQ * sizeof(struct requestTableElem));
  for (int i = 0; i < MAX_OUTSTANDING_REQ; i++){
    reqs[i].done = true;
  }
}

int SplitBus::sendMsgToBus(int core_num, request_t request, uint64_t addr){
  //if (core_num == 0) timer->time++;
  timer->time++;
  snoop_pending = true;

  if (num_requests == MAX_OUTSTANDING_REQ){
    printf("Too many outstanding requests!\n");
    // NACK to core to retry this request later
    return 1;
  }

  int next_tag = 0;
  for (int i = 0; i < MAX_OUTSTANDING_REQ; i++){
    next_tag = i;
    if (reqs[i].done) break;
  }

  //add request to request table
  reqs[next_tag].done = false;
  reqs[next_tag].core_num = core_num;
  reqs[next_tag].addr = addr;
  reqs[next_tag].req = request;
  num_requests++;

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

  reqs[next_tag].done = true;
  num_requests--;

  snoop_pending = false;
  return 0;
}
