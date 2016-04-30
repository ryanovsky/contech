#include "graphTraverse.hpp"

GraphTraverse::GraphTraverse(char *str){
  std::ifstream input;
  input.open(str, std::fstream::in);

  for (std::string line; getline(input, line); ){
    int threadnum;
    char tmp[50];
    uint64_t addr;
    sscanf(line.c_str(), "%d: %s %ld", threadnum, tmp, addr);
    std::string instr(tmp);

    Instruction cur;
    if (instr == "Xbegin"){
      cur.instr = BEGIN;
      cur.write = false;
      cur.addr = 0;
    }
    else if (instr == "load"){
      cur.instr = WORK;
      cur.write = false;
      cur.addr = 0;
    }
    else if (instr == "store"){
      cur.instr = WORK;
      cur.write = true;
      cur.addr = 0;
    }
    else if (instr == "Xcommit"){
      cur.instr = COMMIT;
      cur.write = false;
      cur.addr = 0;
    }
    if (threadnum == 1){
      cur.core_num = 0;
      core1.push(cur);
    }
    else if (threadnum == 2){
      cur.core_num = 1;
      core2.push(cur);
    }
    else if (threadnum == 3){
      cur.core_num = 2;
      core3.push(cur);
    }
  }

  // interleave instruction streams into single queue
  while (!core1.empty() && !core2.empty() && !core3.empty()){
    if (!core1.empty()){
      memReqQ.push(core1.front());
      core1.pop();
    }
    if (!core2.empty()){
      memReqQ.push(core2.front());
      core2.pop();
    }
    if (!core3.empty()){
      memReqQ.push(core3.front());
      core3.pop();
    }
  }
}

int GraphTraverse::getNextMemoryRequest(Instruction &nextReq)
{
  nextReq = memReqQ.front();
  memReqQ.pop();
  return 1;
}
