#include "simpleCache.hpp"

#define NUM_PROCESSORS 4

class Bus
{
  public:
    SimpleCache *caches[];

    Bus(SimpleCache *c[]);

    // returns success of write to bus
    int sendMsgToBus(int core_num, request_t request, uint64_t addr);
};

