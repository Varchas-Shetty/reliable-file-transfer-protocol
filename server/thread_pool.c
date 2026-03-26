#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/server.h"

int spawn_detached_thread(void *(*routine)(void *), void *arg) {
    pthread_t thread_id;
    int rc = pthread_create(&thread_id, NULL, routine, arg);

    if (rc != 0) {
        fprintf(stderr, "Failed to create worker thread (rc=%d)\n", rc);
        return -1;
    }

    rc = pthread_detach(thread_id);
    if (rc != 0) {
        fprintf(stderr, "Failed to detach worker thread (rc=%d)\n", rc);
        return -1;
    }

    return 0;
}
