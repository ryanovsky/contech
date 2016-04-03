#define NUM_CORES 4

typedef enum {
  BUSRD;
  BUSRDX;I
} bus_msg;

typedef struct {
  bus_msg *msg;
  uint64_t *cacheline;
} bus_t;

class Bus
{
  public:
    bus_t cores[NUM_CORES];
};

