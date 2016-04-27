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

struct requestTableElem *SplitBus::sendMsgToBus(int core_num, request_t request, uint64_t addr){
  timer->time++;
  snoop_pending = true;

  if (num_requests == MAX_OUTSTANDING_REQ){
    printf("Too many outstanding requests!\n");
    // NACK to core to retry this request later
    return NULL;
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


  struct requestTableElem *ret = NULL;

  for (int i = 0; i < MAX_OUTSTANDING_REQ; i++){
    if (!reqs[i].done){
      if (reqs[i].time < NUM_PROCESSORS)
        reqs[i].time++;
      else {
        reqs[i].done = true;
        num_requests--;
        ret = &reqs[i];
        break;
      }
    }
  }

  if (ret == NULL)
    return NULL;

  for (int i = 0; i < NUM_PROCESSORS; i++){
    if (i != ret->core_num){
      //update the status of the cache based on the message on the bus
      if(caches[i]->updateStatus(ret->req, ret->addr)){
        dirty = true;
        mem->flush();
        dirty = false;
      }
      shared = shared || (caches[i]->checkState(ret->addr) == SHARED);
    }
  }

  snoop_pending = false;
  return ret;
}

struct requestTableElem *SplitBus::checkBusStatus(){
  timer->time++;

  struct requestTableElem *ret = NULL;

  for (int i = 0; i < MAX_OUTSTANDING_REQ; i++){
    if (!reqs[i].done){
      if (reqs[i].time < NUM_PROCESSORS)
        reqs[i].time++;
      else {
        reqs[i].done = true;
        num_requests--;
        ret = &reqs[i];
        break;
      }
    }
  }

  if (ret == NULL)
    return NULL;

  for (int i = 0; i < NUM_PROCESSORS; i++){
    if (i != ret->core_num){
      //update the status of the cache based on the message on the bus
      if(caches[i]->updateStatus(ret->req, ret->addr)){
        dirty = true;
        mem->flush();
        dirty = false;
      }
    }
  }

  return ret;
}
