#include "simpleCache.hpp"
#include "memory.hpp"

#define NUM_PROCESSORS 4

class Bus
{
  public:
    bool shared, dirty, snoop_pending;
    Memory *mem;
    SimpleCache *caches[];

    Bus(SimpleCache *c[], Memory *);

    // returns success of write to bus
    int sendMsgToBus(int core_num, request_t request, uint64_t addr);
};

