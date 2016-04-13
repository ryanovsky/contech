#include <stdio.h>
#include "cacheCoherence.hpp"

using namespace contech;

CacheCoherence::CacheCoherence(char *fname, uint64_t c, uint64_t s){
  timer = 0;
  tw = new TraceWrapper(fname); //use trace wrapper to order all memory actions
  Memory *mem = new Memory();

  for(int i = 0; i < NUM_PROCESSORS; i ++){
    sharedCache[i] = new SimpleCache(c, s, i);
    p_stats[i] = new cache_stats_t;
    (p_stats[i])->accesses = 0;
    (p_stats[i])->misses = 0;
    visited[i] = false;
  }
  interconnect = new Bus(sharedCache, mem);
}

void CacheCoherence::run()
{
  MemReqContainer mrc;
  uint32_t ctid = 0;
  uint32_t prev_ctid = ctid;
  request_t req;
  int req_result;

  while (tw->getNextMemoryRequest(mrc)) {
    ctid = (uint32_t)(mrc.ctid) - 1;
    bool rw = false;
    bool shared = false;
    unsigned int memOpPos = 0;
    unsigned long int lastBBID = mrc.bbid;

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
            // broadcast to bus about stuff
            req = BUSRD;
            req_result = interconnect->sendMsgToBus(ctid, req, srcAddress);
            timer++;

            shared = false;
            for(int i = 0; i < NUM_PROCESSORS; i ++){
              if(i != ctid){
                  if(sharedCache[i]->checkState(srcAddress) == SHARED) shared = true;
              }
            }


            (sharedCache[ctid])->updateCache(false, accessSize, srcAddress, p_stats[ctid], shared);
            srcAddress += accessSize;
            (p_stats[ctid])->accesses ++;
            for(int i = 0; i < NUM_PROCESSORS; i ++){
              if(i == ctid) assert(sharedCache[i]->checkState(srcAddress) == SHARED);
              else assert(sharedCache[i]->checkState(srcAddress) != MODIFIED);
            }
          }
          req = BUSRDX;
          req_result = interconnect->sendMsgToBus(ctid, req, dstAddress);
          timer++;
          shared = false;
          for(int i = 0; i < NUM_PROCESSORS; i ++){
              if(i != ctid){
                  if(sharedCache[i]->checkState(dstAddress) == SHARED) shared = true;
              }
          }
          (sharedCache[ctid])->updateCache(true, accessSize, dstAddress, p_stats[ctid], shared);
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

        req_result = interconnect->sendMsgToBus(ctid, req, address);
        timer++;
        shared = false;
          for(int i = 0; i < NUM_PROCESSORS; i ++){
              if(i != ctid){
                  if(sharedCache[i]->checkState(address) == SHARED) shared = true;
              }
          }

        (sharedCache[ctid])->updateCache(rw, accessBytes, address, p_stats[ctid], shared);
        if(rw){
         for(int i = 0; i < NUM_PROCESSORS; i ++){
              //printf (" cache %d is writing: state[%d]:%d\n",ctid, i, sharedCache[i]->checkState(address));
              //assert on write that the current processor is in the MODIFIED state alone
              if(i == ctid) assert(sharedCache[i]->checkState(address) == MODIFIED);
              else assert(sharedCache[i]->checkState(address) == INVALID);
          }
        }
        else{
         //assert on read that the current processor is in the SHARED state
         for(int i = 0; i < NUM_PROCESSORS; i ++){
              if(i == ctid); //assert(sharedCache[i]->checkState(address) == SHARED);
              else assert(sharedCache[i]->checkState(address) != MODIFIED);
          }
        }

        address += accessBytes;
      } while (numOfBytes > 0);
    }

    //assert everything in the cache is valid
    for(int i = 0; i < NUM_PROCESSORS; i ++){
        assert(sharedCache[i]->checkValid());
    }

    printf("time:%d, processor:%d, misses=%d, accesses=%d\n"
        ,timer, ctid, p_stats[ctid]->misses, p_stats[ctid]->accesses);
    //prev_ctid = ctid;
  }

  printf("ready for cleanup in run\n");
  // delete tw;
  // delete sharedCache;
  // delete p_stats;
  // for(int i = 0; i < NUM_PROCESSORS; i ++){
  //   delete sharedCache[i];
  //   delete p_stats[i];
  // }
}
