#include <stdio.h>
#include "cacheCoherence.hpp"

using namespace contech;

CacheCoherence::CacheCoherence(char *fname, uint64_t c, uint64_t s){
  timer = new Time();
  gt = new GraphTraverse(fname);
  mem = new Memory(timer);
  num_processors = 3;//gt->tg->getNumberOfContexts();
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
  uint32_t prev_ctid = ctid;
  request_t req;
  bool sendMsg = false;
  bool next_cycle = false;
  int g_inprogress = 0;

  while ((g_inprogress = gt->getNextMemoryRequest(mrc)) || interconnect->num_requests){
    printf("g_inprogress:%d, num_requests:%d\n", g_inprogress, interconnect->num_requests);
    ctid = (uint32_t)(mrc.core_num);
    bool rw = false;
    bool shared = false;

    tempQ[ctid].push(mrc);
    mrc = tempQ[ctid].front();
    tempQ[ctid].pop();

    //update time
    if (visited[ctid] && g_inprogress){
      timer->time++;
      next_cycle == true;
      for(int i = 0; i < num_processors; i++){
        visited[i] = false;
      }
    }
    visited[ctid] = true;

    SimpleCache *curCache = sharedCache[ctid];

    if (mrc.instr == BEGIN){
      curCache->rwset.clear();

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
      curCache->rwset[mrc.addr] = mrc;

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
      // update cache for everything in rwset
      for (auto it = curCache->rwset.begin(); it != curCache->rwset.end(); ++it){
        uint64_t address = it->first;
        Instruction curmrc = it->second;

        if (curmrc.write)
        {
          rw = true;
          req = BUSRDX;
        }
        else
        {
          rw = false;
          req = BUSRD;
        }

        // only send message too bus if (shared and write) or invalid
        cache_state came_from = curCache->checkState(address);
        struct requestTableElem *req_result;

        if (g_inprogress && (came_from == INVALID || (came_from == SHARED && rw))){
          sendMsg = true;
          req_result = interconnect->sendMsgToBus(ctid, req, address);
          if(!req_result->ACK){
            printf("NACK\n");
            //push back onto the queue
          }
          if (req_result->restart_cores){
            //need to go through and restart transactions
            for(int i = 0; i < num_processors; i ++){
              if((req_result->restart_cores >> i) & 1){
                //put everything in the rwset into tempQ
                for(int iterate = 0; iterate < sharedCache[i]->rwset.size(); iterate ++){
                  tempQ[ctid].push(sharedCache[i]->rwset[iterate]);
                }
                sharedCache[i]->rwset.clear();
              }
            }
          }
        }
        else {
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
        if (!sendMsg && g_inprogress){
          if (!sharedCache[ctid]->updateCache(rw, address, p_stats[ctid], shared))
            interconnect->mem->load();
          p_stats[ctid]->accesses++;
          assert_correctness(rw, ctid, address);
        }

        free(req_result);

        //assert everything in the cache is valid
        for(int i = 0; i < num_processors; i++){
          assert(sharedCache[i]->checkValid());
        }
      }
    }

    //printf("time:%d, processor:%d, misses=%d, accesses=%d\n"
    //    ,timer->time, ctid, p_stats[ctid]->misses, p_stats[ctid]->accesses);
    //prev_ctid = ctid;
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
      if(i == ctid); //assert(sharedCache[i]->checkState(address) == SHARED);
      else assert(sharedCache[i]->checkState(address) != MODIFIED);
    }
    //if a processor moved into EXCLUSIVE, assert others aren't in shared
    if(sharedCache[ctid]->checkState(address) == EXCLUSIVE){
      for(int i = 0; i < num_processors; i++){
        if(i == ctid); //assert(sharedCache[i]->checkState(address) == SHARED);
        else {
          assert(sharedCache[i]->checkState(address) != SHARED);
        }
      }
    }
  }
}
