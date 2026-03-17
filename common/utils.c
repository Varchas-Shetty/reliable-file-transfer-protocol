#include <stdio.h>
#include <stdlib.h>
#include "constants.h"

// Example: log errors and exit
void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}