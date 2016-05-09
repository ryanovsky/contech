#include "cacheCoherence.hpp"
#include <stdio.h>

int main(int argc, char** argv)
{
  if ((argc != 3) || atoi(argv[1]) < 8)
  {
    fprintf(stderr, "%s <size of cache, log2> <taskgraph>\n", argv[0]);
    fprintf(stderr, "size of cache must be greater than 7!\n");
    return 1;
  }

  else
  {
    CacheCoherence* cc = new CacheCoherence(argv[2], atoi(argv[1]),2);
    cc->run();
    delete cc;
  }
  return 0;
}
