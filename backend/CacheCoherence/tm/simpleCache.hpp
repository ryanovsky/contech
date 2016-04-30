#ifndef SIMPLECACHE_HPP
#define SIMPLECACHE_HPP

#include <Backend.hpp>
#include <vector>
#include <deque>
#include <map>
#include "graphTraverse.hpp"

enum cache_state {
  MODIFIED,
  EXCLUSIVE,
  SHARED,
  INVALID
};

enum request_t {
  BUSRDX,
  BUSRD,
  FLUSH,
  NOTHING,
};

struct cache_stats_t {
  uint64_t accesses;
  uint64_t misses;
};

class SimpleCache
{
  struct cache_line
  {
    uint64_t tag;
    uint64_t lastAccess;
    bool dirty;
    char valid_bits;
    cache_state state;
  };

  uint64_t read_misses;
  uint64_t write_misses;
  uint64_t accesses;

  std::vector< std::deque<cache_line> > cacheBlocks;

  bool updateCacheLine(uint64_t idx, uint64_t tag, uint64_t offset, uint64_t num, bool, bool shared);
  void printIndex(uint64_t idx);

  public:
    uint64_t global_c;
    static const uint64_t global_b = 6;
    uint64_t global_s;
    int core_num;

    SimpleCache();
    SimpleCache(uint64_t, uint64_t);
    SimpleCache(uint64_t, uint64_t, int);
    double getMissRate();
    bool updateCache(bool rw, uint64_t address, cache_stats_t* p_stats, bool shared);

    cache_line *addressToCacheline(uint64_t address);
    bool updateStatus(request_t request, uint64_t addr);
    cache_state checkState(uint64_t addr);
    bool checkValid();
};

#endif
