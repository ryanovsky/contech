enum instruction_type{
    BEGIN,
    WORK,
    END
};
struct instruction {
    instruction_type instr;
    bool write;
    uint64_t addr;
}


struct MemReq{
  unsigned int ctid;
  bool isWrite;
  char numOfBytes;
  unsigned long int address;

}

class graphTraverse {
    instruction *core1;
    instruction *core2;
    instruction *core3;

    graphTraverse(char *)
}
