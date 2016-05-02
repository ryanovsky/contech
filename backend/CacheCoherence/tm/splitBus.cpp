#include "splitBus.hpp"

SplitBus::SplitBus(SimpleCache **c, Memory *m, Time *t, int np){
  num_proc = np;
  caches = (SimpleCache **) malloc(sizeof(SimpleCache *) * num_proc);
  for (int i = 0; i < num_proc; i++){
    caches[i] = c[i];
  }

  mem = m;
  timer = t;
  shared = false;
  dirty = false;
  snoop_pending = false;

  num_requests = 0;
  reqs = (struct requestTableElem *) malloc(MAX_OUTSTANDING_REQ * sizeof(struct requestTableElem));
  for (int i = 0; i < MAX_OUTSTANDING_REQ; i++){
    reqs[i].done = true;
    reqs[i].ACK = false;
    reqs[i].core_num = -1;
    reqs[i].time = 0;
    reqs[i].restart_cores = 0;
    reqs[i].addr = 0;
  }
}

SplitBus::~SplitBus(){
  free(caches);
  free(reqs);
}

struct requestTableElem *SplitBus::sendMsgToBus(int core_num, request_t request, uint64_t addr){
  timer->time++;
  snoop_pending = true;
  struct requestTableElem *ret = (struct requestTableElem *) malloc(sizeof(struct requestTableElem));
  ret->core_num = -1;
  ret->restart_cores = 0;

  //which transactions need to restart
  for (int i = 0; i < num_proc; i++){
    std::queue<Instruction, std::deque<Instruction>> temp;
    while (!caches[i]->rwset.empty()){
      if (caches[i]->rwset.front().addr == addr){
        ret->restart_cores |= (1<<i);
      }
      temp.push(caches[i]->rwset.front());
      caches[i]->rwset.pop();
    }
    while (!temp.empty()){
      caches[i]->rwset.push(temp.front());
      temp.pop();
    }
  }

  bool ack = (num_requests < MAX_OUTSTANDING_REQ) ? true : false;
  ret->ACK = ack;
  if (num_requests >= MAX_OUTSTANDING_REQ){
    printf("Too many outstanding requests!\n");
    // NACK to core to retry this request later
  }
  else {
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
  }


  for (int i = 0; i < MAX_OUTSTANDING_REQ; i++){
    if (!reqs[i].done){
      if (reqs[i].time < DELAY)
        reqs[i].time++;
      else {
        reqs[i].done = true;
        num_requests--;
        ret->done = reqs[i].done;
        ret->core_num = reqs[i].core_num;
        ret->time = reqs[i].time;
        ret->req = reqs[i].req;
        ret->addr = reqs[i].addr;
        ret->ACK = ack;
        break;
      }
    }
  }


  if(ret->core_num != -1){
    for (int i = 0; i < num_proc; i++){
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
  }

  snoop_pending = false;
  return ret;
}

struct requestTableElem *SplitBus::checkBusStatus(){
  timer->time++;

  struct requestTableElem *ret = (struct requestTableElem *) malloc(sizeof(struct requestTableElem));
  ret->core_num = -1;
  ret->restart_cores = 0;

  for (int i = 0; i < MAX_OUTSTANDING_REQ; i++){
    if (!reqs[i].done){
      if (reqs[i].time < DELAY)
        reqs[i].time++;
      else {
        reqs[i].done = true;
        num_requests--;
        ret->done = reqs[i].done;
        ret->core_num = reqs[i].core_num;
        ret->time = reqs[i].time;
        ret->req = reqs[i].req;
        ret->addr = reqs[i].addr;

        break;
      }
    }
  }

  if (ret->core_num != -1){
    for (int i = 0; i < num_proc; i++){
      if (i != ret->core_num){
        //update the status of the cache based on the message on the bus
        if(caches[i]->updateStatus(ret->req, ret->addr)){
          dirty = true;
          mem->flush();
          dirty = false;
        }
      }
    }
  }

  return ret;
}

