#include "cacheCoherence.hpp"
#include <stdio.h>

int main(int argc, char** argv)
{
  if (argc != 3)
  {
    fprintf(stderr, "%s <size of cache, log2> <taskgraph>\n", argv[0]);
    return 1;
  }

  else
  {
    CacheCoherence* cc = new CacheCoherence(argv[2], atoi(argv[1]),2);
    printf("created cache coherence object\n");
    cc->run();
    delete cc;
  }/**
     else
     {
     for (int c = 10; c <= 28; c++)
     {
     CacheCoherenceBackend* bcc = new CacheCoherenceBackend(c, 2, 0);
     contech::SimpleBackendWrapper* sbw = new contech::SimpleBackendWrapper(argv[1], bcc);

     sbw->runBackend();
     sbw->completeRun(stdout);
     delete sbw;
     delete bcc;
     }
     }*/

  return 0;
}
