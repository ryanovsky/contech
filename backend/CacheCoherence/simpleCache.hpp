#ifndef SIMPLECACHE_HPP
#define SIMPLECACHE_HPP

#include <Backend.hpp>
#include <vector>
#include <deque>
#include <map>
#include "bus.hpp"

#define NUM_PROCESSORS 4

typedef enum {
  MODIFIED;
  EXCLUSIVE;
  SHARED;
} cache_line_state;

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
    cache_line_state state[NUM_PROCESSORS];
};

    uint64_t read_misses;
    uint64_t write_misses;
    uint64_t accesses;


    bool updateCacheLine(uint64_t idx, uint64_t tag, uint64_t offset, uint64_t num, bool);
    void printIndex(uint64_t idx);

public:
    int core;
    std::vector< std::deque<cache_line> > cacheBlocks;

    SimpleCache(int c);
    double getMissRate();
    bool updateCache(bool rw, char numOfBytes, uint64_t address, cache_stats_t* p_stats);
    void updateState(bool write, cache_line cacheline);
};

struct mallocStats
{
    uint32_t bbid;
    uint32_t size;
    uint32_t misses;
};

class SimpleCacheBackend  : public contech::Backend
{
    std::map <contech::ContextId, SimpleCache> contextCacheState;
    std::map <uint64_t, unsigned int> basicBlockMisses;
    std::map <uint64_t, mallocStats> allocBlocks;
    cache_stats_t* p_stats;
    bool printMissLines;

public:
    virtual void resetBackend();
    virtual void updateBackend(contech::Task*);
    virtual void completeBackend(FILE*, contech::TaskGraphInfo*);

    SimpleCacheBackend(uint64_t c, uint64_t s, int printMissLoc);
};

#endif
