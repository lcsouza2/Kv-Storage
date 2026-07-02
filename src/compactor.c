#include <pthread.h>
#include "compactor.h"
#include "sstables.h"
#include "logging.h"

static pthread_t worker_thread;
static pthread_mutex_t compactor_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t compactor_cond = PTHREAD_COND_INITIALIZER;
static int needs_compaction = 0;
static int keep_running = 1;

static void *compactor_worker_loop(void *arg) {
    (void)arg; // Unused parameter
    while (keep_running) {
        pthread_mutex_lock(&compactor_mutex);
        while (!needs_compaction && keep_running) {
            pthread_cond_wait(&compactor_cond, &compactor_mutex);
        }
        if (!keep_running) {
            pthread_mutex_unlock(&compactor_mutex);
            break;
        }
        needs_compaction = 0;
        pthread_mutex_unlock(&compactor_mutex);
        check_and_compact_sstables(0);
    }
    return NULL;
}

void init_background_compactor() {
    info("Initializing background compactor...");
    pthread_create(&worker_thread, NULL, compactor_worker_loop, NULL);
}

void trigger_background_compaction() {
    pthread_mutex_lock(&compactor_mutex);
    needs_compaction = 1;
    pthread_cond_signal(&compactor_cond);
    pthread_mutex_unlock(&compactor_mutex);
}

void shutdown_background_compactor() {
    info("Shuting down background compactor...");

    pthread_mutex_lock(&compactor_mutex);
    keep_running = 0;

    pthread_cond_signal(&compactor_cond);
    pthread_mutex_unlock(&compactor_mutex);

    if (pthread_join(worker_thread, NULL) != 0) {
        error("Failed to join compactor thread.");
    }

    pthread_mutex_destroy(&compactor_mutex);
    pthread_cond_destroy(&compactor_cond);

    info("Background compactor shut down safely.");
}