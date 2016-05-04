#include <stdio.h>
#include "cacheCoherence.hpp"

using namespace contech;

CacheCoherence::CacheCoherence(char *fname, uint64_t c, uint64_t s){
  timer = new Time();
  gt = new GraphTraverse(fname);
  mem = new Memory(timer);
  num_processors = 3;
  p_stats = (cache_stats_t **) malloc(sizeof(cache_stats_t*) * num_processors);
  sharedCache = (SimpleCache **) malloc(sizeof(SimpleCache*) * num_processors);
  visited = (bool *) malloc(sizeof(bool) * num_processors);

  for(int i = 0; i < num_processors; i++){
    sharedCache[i] = new SimpleCache(c, s, i);
    p_stats[i] = (cache_stats_t *) malloc(sizeof(struct cache_stats_t));
    p_stats[i]->accesses = 0;
    p_stats[i]->misses = 0;
    visited[i] = false;
  }
  interconnect = new SplitBus(sharedCache, mem, timer, num_processors);
}

CacheCoherence::~CacheCoherence(){
  delete timer;
  delete gt;
  delete mem;
  for(int i = 0; i < num_processors; i++){
    delete sharedCache[i];
    free(p_stats[i]);
  }
  free(p_stats);
  free(sharedCache);
  free(visited);
  delete interconnect;
}

void CacheCoherence::run()
{
  Instruction mrc;
  uint32_t ctid = 0;
  request_t req;
  bool sendMsg = false;
  int g_inprogress = 0;
  int Q_iter = 0;

  while (1){
    if (!(g_inprogress = gt->getNextMemoryRequest(mrc)) && !interconnect->num_requests){
      bool all_empty = true;
      for (int i = 0; i < num_processors; i++){
        all_empty = all_empty && tempQ[i].empty();
      }
      if (all_empty) break;
    }
    ctid = (uint32_t)(mrc.core_num);
    bool rw = false;
    bool shared = false;

    if(g_inprogress){
      tempQ[ctid].push(mrc);
      mrc = tempQ[ctid].front();
      tempQ[ctid].pop();
      ctid = mrc.core_num;
    }
    else{
      bool all_empty = true;
      for (int i = 0; i < num_processors; i++){
        if (!tempQ[i].empty()){
          Q_iter = i;
          all_empty = false;
          break;
        }
      }
      if (!all_empty){
        mrc = tempQ[Q_iter].front();
        ctid = mrc.core_num;
        tempQ[Q_iter].pop();
        g_inprogress = 1;
      }
    }

    //update time
    if (visited[ctid] && g_inprogress){
      timer->time++;
      for(int i = 0; i < num_processors; i++){
        visited[i] = false;
      }
    }
    visited[ctid] = true;

    SimpleCache *curCache = sharedCache[ctid];
    curCache->rwset.push(mrc);

    if (mrc.instr == BEGIN){
      struct requestTableElem *req_result = interconnect->checkBusStatus();
      if (req_result) timer->time++;
      if (req_result->core_num != -1){
        bool write = req_result->req == BUSRDX;
        int cn = req_result->core_num;
        if (!sharedCache[cn]->updateCache(write, req_result->addr, p_stats[cn], shared))
          interconnect->mem->load();
        p_stats[cn]->accesses++;
        assert_correctness(write, cn, req_result->addr);
      }
      free(req_result);
    }
    else if (mrc.instr == WORK){
      struct requestTableElem *req_result = interconnect->checkBusStatus();
      if (req_result) timer->time++;
      if (req_result->core_num != -1){
        bool write = (req_result->req == BUSRDX);
        int cn = req_result->core_num;
        if (!sharedCache[cn]->updateCache(write, req_result->addr, p_stats[cn], shared))
          interconnect->mem->load();
        p_stats[cn]->accesses++;
        assert_correctness(write, cn, req_result->addr);
      }
      free(req_result);
    }
    else if (mrc.instr == COMMIT){
      while (!curCache->rwset.empty()){
        Instruction curmrc = curCache->rwset.front();
        curCache->rwset.pop();
        if(curmrc.instr != WORK){
          continue;
        }
        uint64_t address = curmrc.addr;
        if (curmrc.write){
          rw = true;
          req = BUSRDX;
        } else {
          rw = false;
          req = BUSRD;
        }

        // only send message too bus if (shared and write) or invalid
        cache_state came_from = curCache->checkState(address);
        struct requestTableElem *req_result;

        if ((g_inprogress) && (came_from == INVALID || (came_from == SHARED && rw))){
          sendMsg = true;
          req_result = interconnect->sendMsgToBus(ctid, req, address);
          if(!req_result->ACK) printf("NACK\n");
          if (req_result->restart_cores){
            //need to go through and restart transactions
            for(int i = 0; i < num_processors; i++){
              if((req_result->restart_cores >> i) & 1){
                //put everything in the rwset into tempQ
                std::queue<Instruction, std::deque<Instruction>> temp;
                while (!tempQ[i].empty()){
                  temp.push(tempQ[i].front());
                  tempQ[i].pop();
                }

                while (!sharedCache[i]->rwset.empty()){
                  tempQ[i].push(sharedCache[i]->rwset.front());
                  sharedCache[i]->rwset.pop();
                }
                while (!temp.empty()){
                  tempQ[i].push(temp.front());
                  temp.pop();
                }
              }
            }
          }
        } else {
          sendMsg = false;
          req_result = interconnect->checkBusStatus();
          if (req_result) timer->time++;
        }

        shared = interconnect->shared;

        if (req_result->core_num != -1){
          bool write = (req_result->req == BUSRDX);
          int cn = req_result->core_num;
          if (!sharedCache[cn]->updateCache(write, req_result->addr, p_stats[cn], shared))
            interconnect->mem->load();
          p_stats[cn]->accesses++;
          assert_correctness(write, cn, req_result->addr);
        }
        if (!sendMsg){
          if (!sharedCache[ctid]->updateCache(rw, address, p_stats[ctid], shared))
            interconnect->mem->load();
          p_stats[ctid]->accesses++;
          assert_correctness(rw, ctid, address);
        }

        free(req_result);

        while (!curCache->rwset.empty())
          curCache->rwset.pop();
        ////assert everything in the cache is valid
        for(int i = 0; i < num_processors; i++)
          assert(sharedCache[i]->checkValid());

      }
    }
  }

  int accesses = 0;
  for(int i = 0; i < num_processors; i++){
    printf("cache %d accesses:%d, misses:%d\n", i, (p_stats[i])->accesses, (p_stats[i])->misses);
    accesses = accesses + (p_stats[i])->accesses;
  }
  printf("total access:%d total time:%d\n", accesses, timer->time);
}


void CacheCoherence::assert_correctness(bool write, uint64_t ctid, uint64_t address){
  if(write){
    for(int i = 0; i < num_processors; i++){
      //assert on write that the current processor is in the MODIFIED state alone
      if(i == ctid) assert(sharedCache[i]->checkState(address) == MODIFIED);
      else {
        assert(sharedCache[i]->checkState(address) == INVALID);
      }
    }
  }
  else{
    //assert on read that no other processor is in the MODIFIED STATE
    for(int i = 0; i < num_processors; i++){
      if(i == ctid);
      else assert(sharedCache[i]->checkState(address) != MODIFIED);
    }
    //if a processor moved into EXCLUSIVE, assert others aren't in shared
    if(sharedCache[ctid]->checkState(address) == EXCLUSIVE){
      for(int i = 0; i < num_processors; i++){
        if(i == ctid);
        else {
          assert(sharedCache[i]->checkState(address) != SHARED);
        }
      }
    }
  }
}
