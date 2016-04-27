#include <vector>
#include <map>
#include <deque>
#include "simpleCache.hpp"
//#include "bus.hpp"
#include "splitBus.hpp"

#define NUM_PROCESSORS 4

class CacheCoherence
{
  public:
    bool visited[NUM_PROCESSORS];
    Time *timer;
    SplitBus *interconnect;
    Memory *mem;

    GraphTraverse* gt;
    std::map <contech::ContextId, SimpleCache> contextCacheState;
    SimpleCache * sharedCache[NUM_PROCESSORS];
    cache_stats_t * p_stats[NUM_PROCESSORS];

    CacheCoherence(char*, uint64_t, uint64_t s);
    virtual void run();
};
