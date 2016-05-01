#include "graphTraverse.hpp"

GraphTraverse::GraphTraverse(char *str){
  std::ifstream input;
  input.open(str, std::fstream::in);

  for (std::string line; getline(input, line); ){
    char *threadnumber;
    char * instr;
    char *address;
    threadnumber = strtok((char*)line.c_str(), ":");
    instr = strtok(NULL, " ");
    address = strtok(NULL, "\n");
    int threadnum = atoi(threadnumber);
    uint64_t addr = atoi(address);

    //printf("%d %d %s\n", threadnum, addr, instr);

    Instruction cur;
    if (!strcmp(instr, "Xbegin")){
      cur.instr = BEGIN;
      cur.write = false;
      cur.addr = 0;
    }
    else if (!strcmp(instr,"load")){
      cur.instr = WORK;
      cur.write = false;
      cur.addr = addr;
    }
    else if (!strcmp(instr,"store")){
      cur.instr = WORK;
      cur.write = true;
      cur.addr = addr;
    }
    else if (!strcmp(instr,"Xcommit")){
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
  if(!memReqQ.empty()){
    nextReq = memReqQ.front();
     memReqQ.pop();
    return 1;
  }
  else {
      nextReq.instr = WORK;
      return 0;
  }
}
