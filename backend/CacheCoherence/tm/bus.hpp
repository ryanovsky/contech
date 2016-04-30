#include "simpleCache.hpp"
#include "memory.hpp"

#define NUM_PROCESSORS 4

class Bus
{
  public:
    bool shared, dirty, snoop_pending;
    Time *timer;
    Memory *mem;
    SimpleCache *caches[NUM_PROCESSORS];

    Bus(SimpleCache *c[], Memory *, Time *);

    // returns success of write to bus
    int sendMsgToBus(int core_num, request_t request, uint64_t addr);
};

