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
        CacheCoherence* bcc = new CacheCoherence(argv[2], atoi(argv[1]),2);
        bcc->run();
        delete bcc;
    }
    return 0;
}
