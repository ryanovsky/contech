#include <stdio.h>
#include "cacheCoherence.hpp"

using namespace contech;

CacheCoherence::CacheCoherence(char *fname, uint64_t c, uint64_t s){
  timer = new Time();
  gt = new GraphTraverse(fname);
  Memory *mem = new Memory(timer);

  for(int i = 0; i < NUM_PROCESSORS; i ++){
    sharedCache[i] = new SimpleCache(c, s, i);
    p_stats[i] = new cache_stats_t;
    (p_stats[i])->accesses = 0;
    (p_stats[i])->misses = 0;
    visited[i] = false;
  }
  interconnect = new SplitBus(sharedCache, mem, timer);
}

void CacheCoherence::run()
{
  MemReqContainer mrc;
  uint32_t ctid = 0;
  uint32_t prev_ctid = ctid;
  request_t req;
  int req_result;

  while (gt->getNextMemoryRequest(mrc)) {
    ctid = (uint32_t)(mrc.ctid);
    bool rw = false;
    bool shared = false;
    unsigned int memOpPos = 0;
    unsigned long int lastBBID = mrc.bbid;

    if (visited[ctid]){
      timer->time++;
      for(int i = 0; i < NUM_PROCESSORS; i++){
        visited[i] = false;
      }
    }
    visited[ctid] = true;


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
              req_result = interconnect->sendMsgToBus(ctid, req, srcAddress);
            }

            shared = interconnect->shared;

            if (!(sharedCache[ctid])->updateCache(false, accessSize, srcAddress, p_stats[ctid], shared))
              interconnect->mem->load();
            srcAddress += accessSize;
            (p_stats[ctid])->accesses ++;
            for(int i = 0; i < NUM_PROCESSORS; i ++){
              if(srcAddress == 5275376)
                if(i == ctid) assert(sharedCache[i]->checkState(srcAddress) == SHARED);
                else assert(sharedCache[i]->checkState(srcAddress) != MODIFIED);
            }
          }
          //if in invalid or shared state
          if(sharedCache[ctid]->checkState(dstAddress) == INVALID ||
              sharedCache[ctid]->checkState(dstAddress) == SHARED){
            req = BUSRDX;
            req_result = interconnect->sendMsgToBus(ctid, req, dstAddress);
          }
          shared = interconnect->shared;
          if (!(sharedCache[ctid])->updateCache(true, accessSize, dstAddress, p_stats[ctid], shared))
            interconnect->mem->load();
          for(int i = 0; i < NUM_PROCESSORS; i ++){
            if(i == ctid) assert(sharedCache[i]->checkState(dstAddress) == MODIFIED);
            else assert(sharedCache[i]->checkState(dstAddress) == INVALID);
          }


          dstAddress += accessSize;
          (p_stats[ctid])->accesses ++;
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
        (p_stats[ctid])->accesses++;

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
        bool send_mesg = false;
        cache_state came_from = sharedCache[ctid]->checkState(address);
        if(sharedCache[ctid]->checkState(address) == INVALID ||
            (sharedCache[ctid]->checkState(address) == SHARED && rw == true)){
          req_result = interconnect->sendMsgToBus(ctid, req, address);
          send_mesg = true;
        }
        shared = interconnect->shared;
        if (!(sharedCache[ctid])->updateCache(rw, accessBytes, address, p_stats[ctid], shared))
          interconnect->mem->load();
        if(rw){
          for(int i = 0; i < NUM_PROCESSORS; i ++){
            //assert on write that the current processor is in the MODIFIED state alone
            if(i == ctid) assert(sharedCache[i]->checkState(address) == MODIFIED);
            else {

                //if(sharedCache[i]->checkState(address) != INVALID) printf("state = %d\n, came_from=%d send_msg=%d\n", sharedCache[i]->checkState(address), came_from, send_mesg);
                assert(sharedCache[i]->checkState(address) == INVALID);
            }
          }
        }
        else{
          //assert on read that no other processor is in the MODIFIED STATE
          for(int i = 0; i < NUM_PROCESSORS; i ++){
            if(i == ctid); //assert(sharedCache[i]->checkState(address) == SHARED);
            else assert(sharedCache[i]->checkState(address) != MODIFIED);
          }
          //if a processor moved into EXCLUSIVE, assert others aren't in shared
          if(sharedCache[ctid]->checkState(address) == EXCLUSIVE){
            for(int i = 0; i < NUM_PROCESSORS; i ++){
                if(i == ctid); //assert(sharedCache[i]->checkState(address) == SHARED);
                else {
                    assert(sharedCache[i]->checkState(address) != SHARED);
                }
            }
          }
        }

        address += accessBytes;
      } while (numOfBytes > 0);
    }

    //assert everything in the cache is valid
    for(int i = 0; i < NUM_PROCESSORS; i ++){
      assert(sharedCache[i]->checkValid());
    }

    //printf("time:%d, processor:%d, misses=%d, accesses=%d\n"
    //    ,timer->time, ctid, p_stats[ctid]->misses, p_stats[ctid]->accesses);
    //prev_ctid = ctid;
  }
  int accesses = 0;
  for(int i = 0; i < NUM_PROCESSORS; i ++){
    printf("cache %d accesses:%d, misses:%d\n", i, (p_stats[i])->accesses, (p_stats[i])->misses);
    accesses = accesses + (p_stats[i])->accesses;
  }
  printf("total access:%d total time:%d \n", accesses, timer->time);

  //delete gt;
  //delete mem;
  //delete interconnect;
  for(int i = 0; i < NUM_PROCESSORS; i ++){
     delete sharedCache[i];
     delete p_stats[i];
   }
}
