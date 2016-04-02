#include <Backend.hpp>
#include <vector>
#include <map>
#include <deque>
#include "simpleCache.hpp"



class CacheCoherence
{
public:
    CacheCoherence();
    SimpleCache *sc1;
    SimpleCache *sc2;
    SimpleCache *sc3;
    SimpleCache *sc4;
    //FSM stuff will go here
};
class CacheCoherenceBackend : public contech::Backend
{
public:
    virtual void resetBackend();
    virtual void updateBackend(contech::Task*);
    virtual void completeBackend(FILE*, contech::TaskGraphInfo*);

    SimpleCacheBackend *sc1;
    SimpleCacheBackend *sc2;
    SimpleCacheBackend *sc3;
    SimpleCacheBackend *sc4;
    CacheCoherenceBackend(uint64_t c, uint64_t s, int printMissLoc);
};
