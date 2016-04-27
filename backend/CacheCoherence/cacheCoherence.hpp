#include <vector>
#include <map>
#include <deque>
#include "simpleCache.hpp"
//#include "bus.hpp"
#include "splitBus.hpp"


class CacheCoherence
{
  public:
    int num_processors;
    bool *visited;
    Time *timer;
    SplitBus *interconnect;
    Memory *mem;

    GraphTraverse* gt;
    std::map <contech::ContextId, SimpleCache> contextCacheState;
    SimpleCache ** sharedCache;
    cache_stats_t ** p_stats;

    CacheCoherence(char*, uint64_t, uint64_t s);
    void assert_correctness(bool rw, uint64_t ctid, uint64_t addr);
    virtual void run();
};
