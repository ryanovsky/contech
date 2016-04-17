#include "time.hpp"

class Memory
{
  public:
    Time *timer;
    Memory(Time *);
    void flush(void);
};

