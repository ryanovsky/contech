#include "simpleCache.hpp"
#include "memory.hpp"

#define NUM_PROCESSORS 4
#define MAX_OUTSTANDING_REQ 8

typedef struct requestTableElem {
  bool done;
  int core_num;
  int tag;
  request_t req;
  uint64_t addr;
} requestTableElem_t;

class SplitBus
{
  public:
    bool shared, dirty, snoop_pending;
    requestTableElem_t reqs[MAX_OUTSTANDING_REQ];
    int next; //for round robin
    int num_requests;
    Time *timer;
    Memory *mem;
    SimpleCache *caches[];

    SplitBus(SimpleCache *c[], Memory *, Time *);

    // returns success of write to bus
    int sendMsgToBus(int core_num, request_t request, uint64_t addr);
};

