#include <vector>
#include <map>
#include <deque>
#include "simpleCache.hpp"
#include "bus.hpp"

#define NUM_PROCESSORS 4

class CacheCoherence
{
  public:
    virtual void run();
    Time *timer;

    GraphTraverse* gt;
    std::map <contech::ContextId, SimpleCache> contextCacheState;
    SimpleCache * sharedCache[NUM_PROCESSORS];
    cache_stats_t * p_stats[NUM_PROCESSORS];
    bool visited[NUM_PROCESSORS];
    Bus *interconnect;

    //FSM stuff will go here
    CacheCoherence(char*, uint64_t, uint64_t s);
};
