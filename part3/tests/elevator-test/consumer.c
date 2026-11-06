#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "wrappers.h"

int main(int argc, char **argv)
{
    long result;

    if (argc != 2) {
        fprintf(stderr, "usage: %s --start|--stop\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "--start") == 0)
        result = start_elevator();
    else if (strcmp(argv[1], "--stop") == 0)
        result = stop_elevator();
    else {
        fprintf(stderr, "argument must be --start or --stop\n");
        return EXIT_FAILURE;
    }

    printf("%s elevator returned %ld\n",
           strcmp(argv[1], "--start") == 0 ? "Start" : "Stop", result);
    return result < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
