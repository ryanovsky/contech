#include <vector>
#include <map>
#include <deque>
#include "simpleCache.hpp"
#include "splitBus.hpp"

class CacheCoherence
{
  public:
    int num_processors;
    int msgNotSent;
    int msg;
    bool *visited;
    Time *timer;
    SplitBus *interconnect;
    Memory *mem;

    GraphTraverse* gt;
    std::map <uint64_t, bool> lockedVals;
    SimpleCache **sharedCache;
    cache_stats_t **p_stats;
    std::map <int, std::queue<Instruction, deque<Instruction>>> tempQ;

    CacheCoherence(char*, uint64_t, uint64_t s);
    ~CacheCoherence();
    void assert_correctness(bool rw, uint64_t ctid, uint64_t addr);
    virtual void run();
};
