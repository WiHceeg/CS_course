#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{

    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) 
    {
        switch (opt) 
        {
            case 'n':
                flags = 1;
                break;
            case 't':
                nsecs = atoi(optarg);
                tfnd = 1;
                break;
            default: /* '?' */
                fprintf(stderr, "Usage: %s [-t nsecs] [-n] name\n",
                    argv[0]);
                exit(EXIT_FAILURE);
        }
    }


    printSummary(0, 0, 0);
    return 0;
}
