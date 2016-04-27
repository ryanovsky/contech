#include "simpleCache.hpp"
#include "memory.hpp"

#define NUM_PROCESSORS 4
#define MAX_OUTSTANDING_REQ 8

struct requestTableElem {
  bool done;
  int core_num;
  int tag;
  int time;
  request_t req;
  uint64_t addr;
};

class SplitBus
{
  public:
    bool shared, dirty, snoop_pending;
    int next; //for round robin
    int num_requests;
    Time *timer;
    Memory *mem;
    struct requestTableElem *reqs;
    SimpleCache *caches[NUM_PROCESSORS];

    SplitBus(SimpleCache *c[], Memory *, Time *);

    // returns success of write to bus
    uint64_t sendMsgToBus(int core_num, request_t request, uint64_t addr);
    uint64_t checkBusStatus();

};

