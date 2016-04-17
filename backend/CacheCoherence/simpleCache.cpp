#include "simpleCache.hpp"
#include <vector>
#include <algorithm>

using namespace std;
using namespace contech;

enum {BLOCKING, SUBBLOCKING};
enum {LRU, NMRU_FIFO};

// This would be better stored as static fields in the class
static const char global_st = BLOCKING;
static const char global_r = LRU;

static uint32_t dbg_count[17] = {0};

SimpleCache::SimpleCache()
{
  //cacheBlocks.resize(0x1 << (SC_CACHE_SIZE - (SC_CACHE_ASSOC + SC_CACHE_LINE)));
  cacheBlocks.resize(0x1 << (global_c - (global_s + global_b)));
  read_misses = 0;
  write_misses = 0;
  accesses = 0;
  //printf("Cache created: %d of %d\n", cacheBlocks.size(), 0x1 << global_s);
}

SimpleCache::SimpleCache(uint64_t c, uint64_t s)
{
  global_c = c;
  global_s = s;
  cacheBlocks.resize(0x1 << (global_c - (global_s + global_b)));
  read_misses = 0;
  write_misses = 0;
  accesses = 0;
}

SimpleCache::SimpleCache(uint64_t c, uint64_t s, int cn)
{
  global_c = c;
  global_s = s;
  cacheBlocks.resize(0x1 << (global_c - (global_s + global_b)));
  read_misses = 0;
  write_misses = 0;
  accesses = 0;

  core_num = cn;
}

void SimpleCache::printIndex(uint64_t idx)
{
  for (auto it = cacheBlocks[idx].begin(), et = cacheBlocks[idx].end(); it != et; ++it)
  {
    printf("%llx (%d)\t", it->tag, it->lastAccess);
  }
  printf("\n");
}

bool SimpleCache::updateCacheLine(uint64_t idx, uint64_t tag, uint64_t offset, uint64_t num, bool write, bool shared)
{
  deque<cache_line>::iterator oldest;
  uint64_t tAccess = ~0x0;

  if (idx >= cacheBlocks.size()) idx -= cacheBlocks.size();
  assert(idx < cacheBlocks.size());
  oldest = cacheBlocks[idx].end();

  for (auto it = cacheBlocks[idx].begin(), et = cacheBlocks[idx].end(); it != et; ++it)
  {
    if (it->tag == tag)
    {
      it->dirty = (it->dirty || write);
      it->lastAccess = num;

      if (write) it->state = MODIFIED;
      //else it->state = SHARED;
      return true;
    }
    // Is this the LRU block?
    if (it->lastAccess < tAccess)
    {
      tAccess = it->lastAccess;
      oldest = it;
    }
  }

  // The block is not in the cache
  //   First check if there is space to place it in the cache
  if (cacheBlocks[idx].size() < (0x1<<global_s))
  {
    cache_line t;

    t.tag = tag;
    t.dirty = write;
    t.lastAccess = num;
    if (write) t.state = MODIFIED;
    else{
        if(shared) t.state = SHARED;
        else t.state = EXCLUSIVE;
    }

    cacheBlocks[idx].push_back(t);
    return false;
  }

  // No space for the block, something needs to be evicted
  bool writeBack = oldest->dirty;
  assert(oldest != cacheBlocks[idx].end());
  if (global_r == LRU)
  {
    cacheBlocks[idx].erase(oldest);
    assert(cacheBlocks[idx].size() < (0x1 << global_s));
  }

  {
    cache_line t;

    t.tag = tag;
    t.dirty = write;
    t.lastAccess = num;

    cacheBlocks[idx].push_back(t);
  }

  return false;
}

bool SimpleCache::updateCache(bool write, char numOfBytes, uint64_t address, cache_stats_t* p_stats, bool shared)
{
  unsigned int bbMissCount = 0;
  uint64_t cacheIdx = address >> global_b;
  uint64_t offset = address & ((0x1<<global_b) - 1);
  uint64_t size = numOfBytes;
  uint64_t tag = address >> ((global_c - global_s));
  uint64_t accessCount = p_stats->accesses;

  assert(offset < (0x1 << global_b));

  cacheIdx &= ((0x1 << (global_c - (global_b + global_s))) - 1);

  assert (cacheIdx < cacheBlocks.size());
  accesses++;


  {
    bbMissCount += !updateCacheLine(cacheIdx, tag, offset, accessCount, write, shared);

    // split block access
    uint64_t idx = cacheIdx + 1;
    int64_t sz = offset + size - (0x1 << global_b) - 1;
    while (sz >= 0)
    {
      if (idx >= cacheBlocks.size()) {idx -= cacheBlocks.size(); tag++;}
      bbMissCount += !updateCacheLine(idx, tag, 0, accessCount, write, shared);
      idx++;
      sz -= (0x1 << global_b);
    }
  }

  if (bbMissCount > 0)
  {
    if (write) {write_misses++;}
    else {read_misses++;}
    p_stats->misses ++;

    return false;
  }

  return true;
}

double SimpleCache::getMissRate()
{
  return (double)(read_misses + write_misses) / (double)(accesses);
}

bool SimpleCache::updateStatus(request_t request, uint64_t addr){
  uint64_t idx = (addr >> global_b);// % cacheBlocks.size();
  uint64_t offset = addr & ((0x1<<global_b) - 1);
  uint64_t tag = addr >> ((global_c - global_s));
  bool flush = false;
  bool inCache = false;
  idx &= ((0x1 << (global_c - (global_b + global_s))) - 1);

  for (auto it = cacheBlocks[idx].begin(), et = cacheBlocks[idx].end(); it != et; ++it)
  {
    if (it->tag == tag){
      inCache = true;
      if (request == BUSRDX){
        if (it->state == MODIFIED){
          //mem->flush();
          flush = true;
        }
        it->state = INVALID;
        //remove this from the cache
        cacheBlocks[idx].erase(it);
      }
      else if (request == BUSRD){
        if (it->state == MODIFIED){
          //mem->flush();
          flush = true;
        }
        it->state = SHARED;
      }
      else {
        printf("INVALID REQUEST\n");
      }
      break;
    }
  }
  return flush;
}


cache_state SimpleCache::checkState(uint64_t addr){
  uint64_t idx = (addr >> global_b);
  uint64_t offset = addr & ((0x1<<global_b) - 1);
  uint64_t tag = addr >> ((global_c - global_s));
  bool inCache = false;
  idx &= ((0x1 << (global_c - (global_b + global_s))) - 1);

  for (auto it = cacheBlocks[idx].begin(), et = cacheBlocks[idx].end(); it != et; ++it)
  {
    if (it->tag == tag){
      inCache = true;
      return it->state;
    }
  }
  return INVALID;
}

bool SimpleCache::checkValid(){

  for(int idx = 0; idx < cacheBlocks.size(); idx ++){
    for (auto it = cacheBlocks[idx].begin(), et = cacheBlocks[idx].end(); it != et; ++it)
    {
        if(it->state == INVALID) return false;
    }
  }
  return true;
}
