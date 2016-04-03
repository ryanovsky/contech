#include "cacheCoherence.hpp"
#include "simpleCache.hpp"
#include <stdio.h>

#define NUM_PROCESSORS 4

CacheCoherence::CacheCoherence(){
  this->sc1 = new SimpleCache();
  this->sc2 = new SimpleCache();
  this->sc3 = new SimpleCache();
  this->sc4 = new SimpleCache();
}


CacheCoherenceBackend::CacheCoherenceBackend(uint64_t c, uint64_t s, int printMissLoc) {
  sc1 = new SimpleCacheBackend(c,s,printMissLoc);
  sc2 = new SimpleCacheBackend(c,s,printMissLoc);
  sc3 = new SimpleCacheBackend(c,s,printMissLoc);
  sc4 = new SimpleCacheBackend(c,s,printMissLoc);
  SimpleCacheBackend *scb = new SimpleCacheBackend(c, s, printMissLoc);
}


void CacheCoherenceBackend::updateBackend(contech::Task* currentTask)
{
  uint32_t ctID = (uint32_t)currentTask->getContextId();
  if(ctID % NUM_PROCESSORS == 0) sc1->updateBackend(currentTask);
  if(ctID % NUM_PROCESSORS == 1)sc2->updateBackend(currentTask);
  if(ctID % NUM_PROCESSORS == 2)sc3->updateBackend(currentTask);
  if(ctID % NUM_PROCESSORS == 3)sc4->updateBackend(currentTask);
}

void CacheCoherenceBackend::resetBackend()
{
  sc1->resetBackend();
  sc2->resetBackend();
  sc3->resetBackend();
  sc4->resetBackend();
}

void CacheCoherenceBackend::completeBackend(FILE *f, contech::TaskGraphInfo* tgi)
{
  sc1->completeBackend(f,tgi);
  sc2->completeBackend(f,tgi);
  sc3->completeBackend(f,tgi);
  sc4->completeBackend(f,tgi);
}
