#include <vector>
#include <map>
#include <deque>
#include "simpleCache.hpp"
//#include "bus.hpp"
#include "splitBus.hpp"

#define num_processors 4

class CacheCoherence
{
  public:
    bool visited[num_processors];
    //int num_processors;
    Time *timer;
    SplitBus *interconnect;
    Memory *mem;

    GraphTraverse* gt;
    std::map <contech::ContextId, SimpleCache> contextCacheState;
    SimpleCache * sharedCache[num_processors];
    cache_stats_t * p_stats[num_processors];

    CacheCoherence(char*, uint64_t, uint64_t s);
    void assert_correctness(bool rw, uint64_t ctid, uint64_t addr);
    virtual void run();
};
