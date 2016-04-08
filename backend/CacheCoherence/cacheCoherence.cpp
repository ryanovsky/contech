#include "cacheCoherence.hpp"
#include "simpleCache.hpp"
#include "TraceWrapper.hpp"
#include <stdio.h>

using namespace contech;

CacheCoherence::CacheCoherence(char *fname, uint64_t c, uint64_t s){
  timer = 0;
  tw = new TraceWrapper(fname); //use trace wrapper to order all memory actions
  for(int i = 0; i < NUM_PROCESSORS; i ++){
    sharedCache[i] = new SimpleCache(c, s);
    p_stats[i] = new cache_stats_t;
    (p_stats[i])->accesses = 0;
    (p_stats[i])->misses = 0;
    visited[i] = false;
  }
}

void CacheCoherence::run()
{
  MemReqContainer mrc;
  uint32_t ctid = 0;
  uint32_t prev_ctid = ctid;
   while (tw->getNextMemoryRequest(mrc)) {
        ctid = (uint32_t)(mrc.ctid) - 1;
      if(visited[ctid]){ //already visited at this time step
          timer ++;
          for(int i = 0;  i < NUM_PROCESSORS; i ++){ //reset visited array
              visited[i] = false;
          }
      }
      visited[ctid] = true;
      bool rw = false;
    printf("time:%d, processor:%d, misses=%d, accesses=%d\n"
    ,timer, ctid, p_stats[ctid]->misses, p_stats[ctid]->accesses);
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
                    (sharedCache[ctid])->updateCache(false, accessSize, srcAddress, p_stats[ctid]);
                    srcAddress += accessSize;
                    (p_stats[ctid])->accesses ++;
                }

                (sharedCache[ctid])->updateCache(true, accessSize, dstAddress, p_stats[ctid]);
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
            }
            else if (ma.type == action_type_mem_read)
            {
                rw = false;
            }

            (sharedCache[ctid])->updateCache(rw, accessBytes, address, p_stats[ctid]);

            address += accessBytes;
        } while (numOfBytes > 0);
      }

      prev_ctid = ctid;
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
