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

uint64_t SplitBus::sendMsgToBus(int core_num, request_t request, uint64_t addr){
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

  request_t retReq = NOTHING;
  uint64_t retAddr = 0;

  int i;
  for (i = 0; i < MAX_OUTSTANDING_REQ; i++){
    if (!reqs[i].done){
      if (reqs[i].time < NUM_PROCESSORS)
        reqs[i].time++;
      else {
        reqs[i].done = true;
        num_requests--;
        retReq = reqs[i].req;
        retAddr = reqs[i].addr;
        break;
      }
    }
  }

  if (retReq == NOTHING)
    return 0;

  for (int i = 0; i < NUM_PROCESSORS; i++){
    if (i != reqs[i].core_num){
      //update the status of the cache based on the message on the bus
      if(caches[i]->updateStatus(retReq, retAddr)){
        dirty = true;
        mem->flush();
        dirty = false;
      }
      shared = shared || (caches[i]->checkState(retAddr) == SHARED);
    }
  }

  snoop_pending = false;
  return retAddr;
}

uint64_t SplitBus::checkBusStatus(){
  for (int i = 0; i < MAX_OUTSTANDING_REQ; i++){
    if (!reqs[i].done){
      if (reqs[i].time < NUM_PROCESSORS)
        reqs[i].time++;
      else {
        reqs[i].done = true;
        num_requests--;
        return reqs[i].addr;
      }
    }
  }
  return 0;
}
