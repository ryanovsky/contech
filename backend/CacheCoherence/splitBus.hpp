#include "simpleCache.hpp"
#include "memory.hpp"

#define MAX_OUTSTANDING_REQ 8
#define DELAY 4

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
    int num_proc;
    int num_requests;
    Time *timer;
    Memory *mem;
    struct requestTableElem *reqs;
    SimpleCache **caches;

    SplitBus(SimpleCache **c, Memory *, Time *, int np);

    // returns success of write to bus
    struct requestTableElem *sendMsgToBus(int core_num, request_t request, uint64_t addr);
    struct requestTableElem *checkBusStatus();

};

