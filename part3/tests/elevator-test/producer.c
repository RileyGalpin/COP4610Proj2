#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "wrappers.h"

static int random_number(int minimum, int maximum)
{
    return rand() % (maximum - minimum + 1) + minimum;
}

int main(int argc, char **argv)
{
    char *end;
    int request_count;
    int request;

    if (argc != 2) {
        fprintf(stderr, "usage: %s number_of_requests\n", argv[0]);
        return EXIT_FAILURE;
    }

    errno = 0;
    request_count = (int)strtol(argv[1], &end, 10);
    if (errno != 0 || *end != '\0' || request_count < 0) {
        fprintf(stderr, "number_of_requests must be a non-negative integer\n");
        return EXIT_FAILURE;
    }

    srand((unsigned int)time(NULL));

    for (request = 0; request < request_count; request++) {
        int type = random_number(0, 3);
        int start_floor = random_number(1, 5);
        int destination_floor;
        long result;

        do {
            destination_floor = random_number(1, 5);
        } while (destination_floor == start_floor);

        result = issue_request(start_floor, destination_floor, type);
        printf("Issue (%d, %d, %d) returned %ld\n",
               start_floor, destination_floor, type, result);
    }

    return EXIT_SUCCESS;
}
