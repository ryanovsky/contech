#include <ct_file.h>
#include <TaskGraph.hpp>

#include <queue>
#include <map>
#include <vector>

struct MemReq{
  unsigned int ctid;
  bool isWrite;
  char numOfBytes;
  unsigned long int address;
  unsigned int bbid;
};

typedef struct _ctid_current_state
{
  bool terminated;            // has this task terminated
  contech::Task* currentTask;          // pointer to task being processed
  contech::Task::basicBlockActionCollection::iterator currentBB;    //current basic block to next be processed
  contech::Task::basicBlockActionCollection currentBBCol;           //hold the basic block collection for the end iterator
} ctid_current_state, *pctid_current_state;

class MemReqContainer
{
  public:
    vector <contech::MemoryAction> mav;
    unsigned int bbid;
    unsigned int ctid;
    unsigned int pushedOps;
    bool locked;
};

class GraphTraverse
{
  public:
    GraphTraverse(char*);
    ~GraphTraverse();

    bool done;
    contech::TaskGraph* tg;
    std::deque <MemReqContainer, std::allocator<MemReqContainer>> memReqQ;
    map<contech::ContextId, ctid_current_state*> contechState;

    int populateQueue();
    contech::TaskId getSequenceTask(vector<contech::TaskId>& succ, contech::ContextId selfId);

    int getNextMemoryRequest(MemReqContainer&);
};
