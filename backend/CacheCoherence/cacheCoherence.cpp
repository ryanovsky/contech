#include <stdio.h>
#include "cacheCoherence.hpp"

using namespace contech;

CacheCoherence::CacheCoherence(char *fname, uint64_t c, uint64_t s){
  timer = new Time();
  gt = new GraphTraverse(fname);
  mem = new Memory(timer);
  num_processors = gt->tg->getNumberOfContexts();
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
  MemReqContainer mrc;
  uint32_t ctid = 0;
  uint32_t prev_ctid = ctid;
  request_t req;
  struct requestTableElem *req_result;
  bool sendMsg = false;
  bool next_cycle = false;
  while (gt->getNextMemoryRequest(mrc)) {
    ctid = (uint32_t)(mrc.ctid);
    bool rw = false;
    bool shared = false;
    unsigned int memOpPos = 0;
    unsigned long int lastBBID = mrc.bbid;

    if (visited[ctid]){
      timer->time++;
      next_cycle == true;
      for(int i = 0; i < num_processors; i++){
        visited[i] = false;
      }
    }
    visited[ctid] = true;


    if(mrc.locked == true){
      if(lockedVals.find(mrc.mav.begin()->addr) == lockedVals.end()){
        //if not in the locked struct put it in there
        lockedVals[mrc.mav.begin()->addr] = true;
      }
      else{
        //if in the locked struct put it in temp queue and continue
        tempQ[ctid] = mrc;
        continue;
      }
    }
    //tempQ gets priority over new mem requests
    if(!tempQ.empty()){
      //temp_ctid = (uint32_t)(tempQ.front().ctid)
      if(tempQ.find(ctid) != tempQ.end()){ //if ctid exists in map
        MemReqContainer temp_mrc;
        temp_mrc = tempQ.find(ctid)->second;
        tempQ.erase(tempQ.find(ctid));
        tempQ[mrc.ctid] = mrc;
        mrc = temp_mrc;
      }
    }
    for (auto iReq = mrc.mav.begin(), eReq = mrc.mav.end(); iReq != eReq; ++iReq, memOpPos++)
    {
      MemoryAction ma = *iReq;

      if (ma.type == action_type_malloc)
      {
        //printf("malloc -- %d\n", memOpPos);
        uint64_t addr = ((MemoryAction)(*iReq)).addr;
        ++iReq;

        continue;
      }
      if (ma.type == action_type_free) {continue;}
      if (ma.type == action_type_memcpy)
      {
        uint64_t dstAddress = ma.addr;
        uint64_t srcAddress = 0;
        uint64_t bytesToAccess = 0;

        //printf("memcpy -- %d\n", memOpPos);
        ++iReq;
        ma = *iReq;
        if (ma.type == action_type_memcpy)
        {
          srcAddress = ma.addr;
          ++iReq;
          ma = *iReq;
        }

        assert(ma.type == action_type_size);
        bytesToAccess = ma.addr;

        char accessSize = 0;

        do {
          accessSize = (bytesToAccess > 8)?8:bytesToAccess;
          bytesToAccess -= accessSize;
          if (srcAddress != 0)
          {
            // broadcast to bus if invalid state
            if(sharedCache[ctid]->checkState(srcAddress) == INVALID){
              req = BUSRD;
              sendMsg = true;
              req_result = interconnect->sendMsgToBus(ctid, req, srcAddress);
              if(req_result->ACK == false){
                printf("NACK\n");
                //push back onto the queue
                gt->memReqQ.push_front(mrc);
              }
            }
            else{
              sendMsg = false;
              req_result = interconnect->checkBusStatus();
              if (req_result) timer->time++;
            }

            shared = interconnect->shared;

            if (req_result->core_num != -1){
              bool write = req_result->req == BUSRDX;
              int cn = req_result->core_num;
              if (!(sharedCache[cn])->updateCache(write, req_result->addr, p_stats[cn], shared))
                interconnect->mem->load();

              srcAddress += accessSize;
              (p_stats[cn])->accesses++;
              assert_correctness(write, cn, req_result->addr);
            }
            if (!sendMsg){
              if (!(sharedCache[ctid])->updateCache(false, srcAddress, p_stats[ctid], shared))
                interconnect->mem->load();
              srcAddress += accessSize;
              (p_stats[ctid])->accesses++;
              assert_correctness(false, ctid, srcAddress);
            }
            free(req_result);
          }
          //if in invalid or shared state
          if(sharedCache[ctid]->checkState(dstAddress) == INVALID ||
              sharedCache[ctid]->checkState(dstAddress) == SHARED){
            req = BUSRDX;
            sendMsg = true;
            req_result = interconnect->sendMsgToBus(ctid, req, dstAddress);
            if(req_result->ACK == false){
              printf("NACK\n");
              //push back onto the queue
                gt->memReqQ.push_front(mrc);
            }
          }
          else {
            sendMsg = false;
            req_result = interconnect->checkBusStatus();
            if (req_result) timer->time++;
          }

          shared = interconnect->shared;

          if (req_result->core_num != -1){
            bool write = req_result->req == BUSRDX;
            int cn = req_result->core_num;
            if (!(sharedCache[cn])->updateCache(write, req_result->addr, p_stats[cn], shared))
              interconnect->mem->load();
            (p_stats[cn])->accesses++;
            assert_correctness(write, cn, req_result->addr);

          }
          if (!sendMsg){
            if (!(sharedCache[ctid])->updateCache(true, dstAddress, p_stats[ctid], shared))
              interconnect->mem->load();
            (p_stats[ctid])->accesses++;
            assert_correctness(true, ctid, dstAddress);
          }

          free(req_result);
          dstAddress += accessSize;
          (p_stats[ctid])->accesses++;
        } while (bytesToAccess > 0);
        continue;
      }

      char numOfBytes = (0x1 << ma.pow_size);
      uint64_t address = ma.addr;
      char accessBytes = 0;

      do {
        // Reduce the memory accesses into 8 byte requests
        accessBytes = (numOfBytes > 8)?8:numOfBytes;
        numOfBytes -= accessBytes;
        //(p_stats[ctid])->accesses++;

        if (ma.type == action_type_mem_write)
        {
          rw = true;
          req = BUSRDX;
        }
        else if (ma.type == action_type_mem_read)
        {
          rw = false;
          req = BUSRD;
        }

        // only send message too bus if (shared and write) or invalid
        cache_state came_from = sharedCache[ctid]->checkState(address);
        if(sharedCache[ctid]->checkState(address) == INVALID ||
            (sharedCache[ctid]->checkState(address) == SHARED && rw == true)){
          sendMsg = true;
          req_result = interconnect->sendMsgToBus(ctid, req, address);
          if(req_result->ACK == false){
            printf("NACK\n");
            //push back onto the queue
                gt->memReqQ.push_front(mrc);
          }
        }
        else {
          sendMsg = false;
          req_result = interconnect->checkBusStatus();
          if (req_result) timer->time++;
        }

        shared = interconnect->shared;

        if (req_result->core_num != -1){
          bool write = req_result->req == BUSRDX;
          int cn = req_result->core_num;
          if (!(sharedCache[cn])->updateCache(write, req_result->addr, p_stats[cn], shared))
            interconnect->mem->load();
          (p_stats[cn])->accesses++;
          assert_correctness(write, cn, req_result->addr);
        }
        if (!sendMsg){
          if (!(sharedCache[ctid])->updateCache(rw, address, p_stats[ctid], shared))
            interconnect->mem->load();
          (p_stats[ctid])->accesses++;
          assert_correctness(rw, ctid, address);
        }

        free(req_result);
        address += accessBytes;
      } while (numOfBytes > 0);
    }

    //assert everything in the cache is valid
    for(int i = 0; i < num_processors; i++){
      assert(sharedCache[i]->checkValid());
    }

    //printf("time:%d, processor:%d, misses=%d, accesses=%d\n"
    //    ,timer->time, ctid, p_stats[ctid]->misses, p_stats[ctid]->accesses);
    //prev_ctid = ctid;
    if(next_cycle){
      next_cycle = false;
      if(mrc.locked = true){
          printf("has locks?\n");
        //assert it was in the locked table
        assert(lockedVals.find(mrc.mav.begin()->addr) != lockedVals.end());
        //remove from the locked table
        lockedVals.erase(mrc.mav.begin()->addr);
      }
    }
  }
  int accesses = 0;
  for(int i = 0; i < num_processors; i++){
    printf("cache %d accesses:%d, misses:%d\n", i, (p_stats[i])->accesses, (p_stats[i])->misses);
    accesses = accesses + (p_stats[i])->accesses;
  }
  printf("total access:%d total time:%d \n", accesses, timer->time);
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
