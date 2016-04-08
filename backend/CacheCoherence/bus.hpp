#include "cacheCoherence.hpp"
#include "simpleCache.hpp"

class Bus
{
  public:
    Bus(SimpleCache **c);

    // returns success of write to bus
    int sendMsgToBus(int core_num, request_t request, uint64_t addr);

    SimpleCache **caches;

    // logic for deciding which cache gets to broadcast on the bus
    int next_cache;
};

