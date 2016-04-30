GraphTraverse::GraphTraverse(){
  std::ifstream input("input.txt");
  for (std::string line; getline(input, line); ){
    int threadnum;
    std::string instr;
    uint64_t addr;
    sscanf(line, "%d: %s %ld", threadnum, instr, addr);
  }
}
