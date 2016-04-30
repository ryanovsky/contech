#include <queue>
#include <deque>
#include <string>
#include <fstream>
#include <stdint.h>
#include <string.h>

enum instruction_type{
    BEGIN,
    WORK,
    COMMIT
};

class Instruction {
  public:
    int core_num;
    instruction_type instr;
    bool write;
    uint64_t addr;
};

class GraphTraverse {
  public:
    std::queue <Instruction, std::deque<Instruction>> core1;
    std::queue <Instruction, std::deque<Instruction>> core2;
    std::queue <Instruction, std::deque<Instruction>> core3;

    std::queue <Instruction, std::deque<Instruction>> memReqQ;

    GraphTraverse(char *);
    int getNextMemoryRequest(Instruction &nextReq);
};
